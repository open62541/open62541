/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * @author Waheed Ejaz (IMD, University of Rostock)
 */



#include "ua_amqp_adaptor.h"
#include <open62541/util.h>
#include <open62541/plugin/pubsub.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/network_tcp.h>
#include <open62541/util.h>
#include <proton/delivery.h>
#include <proton/link.h>
#include <proton/message.h>
#include <proton/proactor.h>
#include <proton/session.h>
#include <proton/transport.h>
#include <proton/event.h>


/**
 *  Sends data to socket. Resets the AMQP write buffer
 *
 */
static UA_StatusCode _write(UA_Connection *c, pn_connection_driver_t *d)
{
    pn_bytes_t bytes = pn_connection_driver_write_buffer(d);
    UA_ByteString buffer;
    buffer.data = (UA_Byte*)UA_malloc(bytes.size);
    buffer.length = bytes.size;
    memcpy(buffer.data, bytes.start, bytes.size);
    /* send the buffer. This deletes the sendBuffer */
    /* TODO: Send API with "const" input args. Avoid memory allocation and copying*/
    UA_StatusCode ret = c->send(c, &buffer);
    if (ret == UA_STATUSCODE_GOOD) {
        /* reset the write buffer */
        pn_connection_driver_write_done(d, bytes.size);
    }

    return ret;
}

/**
 *  Receive data from socket. Copy to AMQP read buffer
 */
static UA_StatusCode _read(UA_Connection *c, pn_connection_driver_t *d, UA_UInt32 timeout)
{
    UA_ByteString inBuffer;

    pn_rwbytes_t pn_read_buffer = pn_connection_driver_read_buffer(d);
    inBuffer.data = (UA_Byte*)pn_read_buffer.start;
    inBuffer.length = pn_read_buffer.size;

    UA_StatusCode ret = c->recv(c, &inBuffer, timeout);

    if(ret == UA_STATUSCODE_GOOD ) {
        /* Process the received bytes and free recv buffer */
        pn_connection_driver_read_done(d, inBuffer.length);
    }

    return ret;
}


/**
 * @brief Connects to the given address and initializes AMQP stack and evetns
 * @param ctx   Pointer to the AMQP context
 * @param address url to connect to
 * @return UA_STATUCODE_GOOD, UA_STATUSCODE_BADOUTOFMEMORY, UA_STATUSCODE_BADCOMMUNICATIONERROR
 */
UA_StatusCode UA_AmqpConnect(UA_AmqpContext *ctx, UA_NetworkAddressUrlDataType address)  {
    UA_assert(ctx != NULL);

    int retVal = PN_OK;

    /*****************************************
     *      Initialize the AMQP stack
     ****************************************/
    retVal = pn_connection_driver_init(ctx->driver, NULL, NULL);
    if (PN_OK != retVal)  {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    pn_connection_open(ctx->driver->connection);
    ctx->openLink = true;
    pn_session_t *s = pn_session(ctx->driver->connection);
    ctx->session = s;
    pn_session_open(s);
    //sender_l = pn_sender(s, "AMQP_CLIENT_SENDER");
    //receiver_l = pn_receiver(s, "AMQP_CLIENT_RECEIVER");

    /*******************************************************
     *          UA Socket Connection
     *******************************************************/
    /* Config with default parameters */
    UA_ConnectionConfig conf = {
            0,     /* .protocolVersion */
            1024, /* .sendBufferSize, 64k per chunk */
            2048, /* .recvBufferSize, 64k per chunk */
            1024,     /* .localMaxMessageSize, 0 -> unlimited */
            1024,     /* .remoteMaxMessageSize, 0 -> unlimited */
            1,     /* .localMaxChunkCount, 0 -> unlimited */
            1      /* .remoteMaxChunkCount, 0 -> unlimited */
    };

    /* Create TCP connection: open the blocking TCP socket (connecting to the broker) */
    UA_Connection connection = UA_ClientConnectionTCP( conf, address.url, 1000, NULL);
    if(connection.state != UA_CONNECTIONSTATE_ESTABLISHED &&
       connection.state != UA_CONNECTIONSTATE_OPENING){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                     "PubSub AMQP: Connection creation failed. Tcp connection failed!");
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    /* Set socket to nonblocking!*/
    UA_socket_set_nonblocking(connection.sockfd);
    /* Copy UA_Connection */
    memcpy(ctx->ua_connection, &connection, sizeof(UA_Connection));


    return UA_STATUSCODE_GOOD;
}

/**
 * @brief Sends a data message to queue specified by sender link
 * @param ctx Pointer to the ctx
 * @param sender PN_link associated with queue
 * @param buf message data to be sent
 * @return UA_STATUSCODE
 */
UA_StatusCode publishAmqp(UA_AmqpContext *ctx, UA_String queue,
            const UA_ByteString *buf)
{
    pn_link_t *sender_l = ctx->links[SENDER_LINK];

    if (ctx->openLink) {
        /*
         * TODO: Move this code to registerAmqp
         */
        /**
         * Only open link for the first time after connection or reconnection
         */
        ctx->openLink = false;
        sender_l = pn_sender(ctx->session, "AMQP_CLIENT_SENDER");
         /* Update the linker  */
        ctx->links[SENDER_LINK] = sender_l;

        pn_terminus_set_address(pn_link_target(sender_l), (const char*)queue.data);
        pn_link_open(sender_l);

        ctx->message = pn_message();
        if (UA_STATUSCODE_GOOD != _write(ctx->ua_connection, ctx->driver) ) {
            return UA_STATUSCODE_BADCOMMUNICATIONERROR;
        }
    }

    /* Check if we have credit to send messages, otherwise discard messages.
     * AMQP lib can buffer messages if credit is not available. For now UA AMQP Adaptor
     * doesn't support this feature
     */
    if (pn_link_credit(sender_l) > 0) {
        pn_delivery(sender_l, pn_dtag((const char *)&ctx->sequence_no,
                                    sizeof(ctx->sequence_no)));

        pn_data_t *body;

        pn_message_clear(ctx->message);
        body = pn_message_body(ctx->message);
        /* Set the message_id also */
        pn_data_put_int(pn_message_id(ctx->message), (int32_t)ctx->sequence_no);
        pn_data_put_map(body);
        pn_data_enter(body);
        pn_data_put_string(body, pn_bytes(sizeof("sequence") - 1, "sequence"));
        pn_data_put_int(body, (int32_t)ctx->sequence_no); /* The sequence number */
        pn_data_exit(body);
        if(pn_message_send(ctx->message, sender_l,
                           &ctx->message_buffer) < 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error sending data: %s",
                         pn_error_text(pn_message_error(ctx->message)) );

            /* TODO: Could be invalid data input that caused encoding failure. No real
             * network communication is doen in pn_message_send(). Only buffers are filled.
             * Look for other error codes than BADCONNECTIONCLOSED
             */
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }

        /* Immediately send data */
        if (UA_STATUSCODE_GOOD != _write(ctx->ua_connection, ctx->driver)) {
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
    } else {
        return UA_STATUSCODE_BADWAITINGFORRESPONSE;
    }

    return UA_STATUSCODE_GOOD;

}


/**
 * Helper function to handle events
 */
static void __handle(UA_AmqpContext *aP, pn_event_t *e)
{
    /* Note: Not all events needs to be handled. */
    switch (pn_event_type(e)) {
        case PN_DELIVERY: {
            /* Acknowledgement received */
            pn_delivery_t *d = pn_event_delivery(e);
            if(pn_delivery_remote_state(d) == PN_ACCEPTED) {
                aP->acknowledged_no++;
            }
        }
        break;

        default:break;
    }
}

/**
 * @brief Sends out all tx buffer on the socket
 *        Receive any incoming traffic on the socket
 *        Handle any pending events
 * @param ctx pointer AMQP context
 * @param timeout timeout for listening on the socket
 */
UA_StatusCode yieldAmqp(UA_AmqpContext *ctx, UA_UInt16 timeout)
{
    UA_Connection *connection = (UA_Connection*) ctx->ua_connection;
    UA_assert(connection != NULL);
    /*
     * Write flush
     */
    if (UA_STATUSCODE_GOOD != _write(ctx->ua_connection, ctx->driver) ) {
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    /*
     * Read Buffer
     */
    if (UA_STATUSCODE_GOOD != _read(ctx->ua_connection, ctx->driver, timeout) ) {
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    /*
     * Handle Pending Events
     */

    /* pn_connection_driver_next_event() returns NULL if no event is pending */
    pn_event_t *e = pn_connection_driver_next_event(ctx->driver);
    for (; e; e = pn_connection_driver_next_event(ctx->driver)) {
        __handle(ctx, e);
    }

    return UA_STATUSCODE_GOOD;
}
