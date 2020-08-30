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
#include <proton/connection.h>
#include <proton/delivery.h>
#include <proton/link.h>
#include <proton/message.h>
#include <proton/session.h>
#include <proton/event.h>


/**
 *  Sends data to socket. Resets the AMQP write buffer
 *
 * @return UA_STATUSCODE_GOOD,
 *         UA_STATUSCODE_BADCONNECTIONCLOSED
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
 *
 * @return UA_STATUSCODE_GOOD,
 *         UA_STATUSCODE_GOODNONCRITICALTIMEOUT,
 *         UA_STATUSCODE_BADCONNECTIONCLOSED
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
 * @brief UA_AmqpDisconnect
 * @param ctx pointer to the AMQP context
 * @return UA_STATUSCODE
 */
void UA_AmqpDisconnect(UA_AmqpContext *ctx)  {
    UA_assert(ctx);

    pn_connection_close(ctx->driver->connection);

    if (UA_STATUSCODE_GOOD != _write(ctx->ua_connection, ctx->driver) )  {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Write to socket failed when closing AMQP connection: %s",
                     pn_error_text(pn_message_error(ctx->message)) );
    }
    pn_collector_free(ctx->driver->collector);
    pn_connection_driver_close(ctx->driver);
    pn_connection_driver_destroy(ctx->driver);

    UA_free(ctx->message_buffer.start);
    pn_message_free(ctx->message);
}

/**
 * @brief Connects to the given address and initializes AMQP stack and evetns
 * @param ctx   Pointer to the AMQP context
 * @param address url to connect to
 * @return UA_STATUCODE_GOOD, UA_STATUSCODE_BADOUTOFMEMORY, UA_STATUSCODE_BADCOMMUNICATIONERROR
 */
UA_StatusCode UA_AmqpConnect(UA_AmqpContext *ctx, UA_NetworkAddressUrlDataType address)  {
    UA_assert(ctx != NULL);

    /*****************************************
     *      Initialize the AMQP stack
     ****************************************/
    int retVal = pn_connection_driver_init(ctx->driver, NULL, NULL);
    if (PN_OK != retVal)  {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    pn_connection_open(ctx->driver->connection);
    ctx->openLink = true;
    ctx->sender_link_ready = false;
    ctx->session = pn_session(ctx->driver->connection);
    pn_session_open(ctx->session);

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
        /**
         * Only open link for the first time after connection or reconnection
         */
        ctx->openLink = false;
        ctx->sender_link_ready = false;
        sender_l = pn_sender(ctx->session, (const char*)queue.data);
         /* Update the linker  */
        ctx->links[SENDER_LINK] = sender_l;

        pn_terminus_set_address(pn_link_target(sender_l), (const char*)queue.data);
        pn_link_open(sender_l);

        ctx->message = pn_message();

        /* yield once to let the initial messages flow and receive link creidt from the peer*/
        UA_StatusCode ret = yieldAmqp(ctx, 5);

        if (UA_STATUSCODE_GOOD == ret) {
            if (!ctx->sender_link_ready) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AMQP Link: No credit received from peer");
                return UA_STATUSCODE_BADWAITINGFORRESPONSE;
            }
        }
    }

    /* Check if we have credit to send messages, otherwise discard messages.
     * AMQP lib can buffer messages if credit is not available. For now UA AMQP Adaptor
     * doesn't support this feature
     */
    int credit = pn_link_credit(sender_l);
    if (credit <= 0) {
        ctx->sender_link_ready = false;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "publishAmqp: AMQP link credit not availabe");
        return UA_STATUSCODE_BADWAITINGFORRESPONSE;
    }
    else {
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
        ssize_t ret = pn_message_send(ctx->message, sender_l, &ctx->message_buffer);

        if(ret < 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "publishAmqp: Send message encoding failed: %s",
                         pn_error_text(pn_message_error(ctx->message)) );

            if (PN_OUT_OF_MEMORY == ret) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            } else {
                return UA_STATUSCODE_BADINVALIDARGUMENT;
            }
        }

        /* Immediately send data on the wire*/
        if (UA_STATUSCODE_GOOD != _write(ctx->ua_connection, ctx->driver)) {
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
    }

    return UA_STATUSCODE_GOOD;

}

static void _handle_reciver_link_delivery(UA_AmqpContext *ctx, pn_delivery_t *d, pn_link_t *l)
{
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AMQP receive not implemented");
    (void)ctx;
    (void)d;
    (void)l;
}

/**
 * Helper function to handle events
 */
static void __handle(UA_AmqpContext *ctx, pn_event_t *e)
{
    /* Note: Not all events needs to be handled.
        Only link events are sent to wire transport*/
    switch (pn_event_type(e)) {
        case PN_LINK_FLOW: {
            /* Credit received, ready to send messages */
            ctx->sender_link_ready = true;
        }
        break;
        case PN_DELIVERY: {
            pn_link_t *l = pn_event_link(e);

            if (pn_link_is_sender(l)) {
                /* Acknowledgement received */
                pn_delivery_t *d = pn_event_delivery(e);
                if(pn_delivery_remote_state(d) == PN_ACCEPTED) {
                    ctx->acknowledged_no++;
                }
            } else {
                _handle_reciver_link_delivery(ctx, pn_event_delivery(e), l);
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
    UA_StatusCode ret = _write(ctx->ua_connection, ctx->driver);
    if (UA_STATUSCODE_GOOD != ret) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "yieldAmqp: _write failed. ret_code=%s", UA_StatusCode_name(ret));
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

    /*
     * Read Buffer
     * All pending events must be handled before _read() call otherwise some unprocessed events may never complete
     */
    ret = _read(ctx->ua_connection, ctx->driver, timeout);
    if (UA_STATUSCODE_BADCONNECTIONCLOSED == ret) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "yieldAmqp: _read failed. ret_code=%s", UA_StatusCode_name(ret));
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }


    return UA_STATUSCODE_GOOD;
}
