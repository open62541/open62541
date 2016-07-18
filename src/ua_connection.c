#include "ua_util.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_securechannel.h"

// max message size is 64k
const UA_ConnectionConfig UA_ConnectionConfig_standard =
    {.protocolVersion = 0, .sendBufferSize = 65535, .recvBufferSize = 65535,
     .maxMessageSize = 1048576, .maxChunkCount = 16};

void UA_Connection_init(UA_Connection *connection) {
    connection->state = UA_CONNECTION_CLOSED;
    connection->localConf = UA_ConnectionConfig_standard;
    connection->remoteConf = UA_ConnectionConfig_standard;
    connection->channel = NULL;
    connection->sockfd = 0;
    connection->handle = NULL;
    UA_ByteString_init(&connection->incompleteMessage);
    connection->send = NULL;
    connection->recv = NULL;
    connection->close = NULL;
    connection->getSendBuffer = NULL;
    connection->releaseSendBuffer = NULL;
    connection->releaseRecvBuffer = NULL;
}

void UA_Connection_deleteMembers(UA_Connection *connection) {
    UA_ByteString_deleteMembers(&connection->incompleteMessage);
}

UA_StatusCode
UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString * UA_RESTRICT message,
                              UA_Boolean * UA_RESTRICT realloced) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString *current = message; /* points to either the network message or the copied message */

    /* We have a stored half-chunk. Concat the received message to the end */
    if(connection->incompleteMessage.length > 0) {
        UA_Byte *data = UA_realloc(connection->incompleteMessage.data,
                                   connection->incompleteMessage.length + message->length);
        if(!data) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        memcpy(&data[connection->incompleteMessage.length], message->data, message->length);
        connection->incompleteMessage.data = data;
        connection->incompleteMessage.length += message->length;
        connection->releaseRecvBuffer(connection, message);
        current = &connection->incompleteMessage;
    }

    /* Loop over the chunks in the received buffer */
    size_t complete_until = 0; /* the received complete chunks end at this point */
    UA_Boolean garbage_end = false; /* garbage after the last complete message */
    while(current->length - complete_until >= 8) {
        /* Check the message type */
        UA_UInt32 msgtype = (UA_UInt32)current->data[complete_until] +
            ((UA_UInt32)current->data[complete_until+1] << 8) +
            ((UA_UInt32)current->data[complete_until+2] << 16);
        if(msgtype != ('M' + ('S' << 8) + ('G' << 16)) &&
           msgtype != ('O' + ('P' << 8) + ('N' << 16)) &&
           msgtype != ('H' + ('E' << 8) + ('L' << 16)) &&
           msgtype != ('A' + ('C' << 8) + ('K' << 16)) &&
           msgtype != ('C' + ('L' << 8) + ('O' << 16))) {
            garbage_end = true; /* the message type is not recognized */
            break;
        }

        /* Decode the length of the chunk */
        UA_UInt32 chunk_length = 0;
        size_t length_pos = complete_until + 4;
        UA_StatusCode decode_retval = UA_UInt32_decodeBinary(current, &length_pos, &chunk_length);

        /* The message size is not allowed. Throw the remaining bytestring away */
        if(decode_retval != UA_STATUSCODE_GOOD || chunk_length < 16 || chunk_length > connection->localConf.recvBufferSize) {
            garbage_end = true;
            break;
        }

        /* The chunk is okay but incomplete. Store the end. */
        if(chunk_length + complete_until > current->length)
            break;

        /* Go to the next chunk */
        complete_until += chunk_length;
    }

    /* Separate complete from incomplete chunks */
    if(complete_until != current->length) {
        size_t incomplete_length = current->length - complete_until;

        /* No complete chunk to return */
        if(complete_until == 0) {
            if(garbage_end) { /* All garbage, reset all */
                retval = UA_STATUSCODE_BADDECODINGERROR;
                goto cleanup;
            }

            /* Store the incomplete message and return UA_BYTESTRING_NULL */
            if(current == message) { /* Incomplete message lives in network layer */
                retval = UA_ByteString_allocBuffer(&connection->incompleteMessage, incomplete_length);
                if(retval != UA_STATUSCODE_GOOD)
                    goto cleanup;
                memcpy(connection->incompleteMessage.data, message->data, incomplete_length);
                connection->releaseRecvBuffer(connection, message);
            } else { /* Incomplete message already copied to the buffer */
                current = message; // message is already set to UA_BYTESTRING_NULL
            }
            goto cleanup;
        }

        /* Just ignore garbage after a complete message */
        if(garbage_end) {
            current->length = complete_until;
            goto cleanup;
        }

        /* We have an incomplete chunk at the end that needs to be stored */
        current->length = complete_until;
        UA_ByteString oldIncompleteMessage = connection->incompleteMessage;
        retval = UA_ByteString_allocBuffer(&connection->incompleteMessage, incomplete_length);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        memcpy(&connection->incompleteMessage.data, &current->data[complete_until], incomplete_length);
        if(current != message) { /* Message was already on the incompleteMessage buffer */
            *message = oldIncompleteMessage;
            current = message;
        }
    }

 cleanup:
    /* Error, reset the connection */
    if(retval != UA_STATUSCODE_GOOD) {
        connection->releaseRecvBuffer(connection, message);
        UA_ByteString_deleteMembers(&connection->incompleteMessage);
        connection->incompleteMessage = UA_BYTESTRING_NULL;
        return retval;
    }

    /* If the original message was moved, delete on the network layer */
    *realloced = (current != message);
    if(*realloced) {
        connection->releaseRecvBuffer(connection, message);
        *message = *current;
    }
    return UA_STATUSCODE_GOOD;
}

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

void UA_Connection_detachSecureChannel(UA_Connection *connection) {
#ifdef UA_ENABLE_MULTITHREADING
    UA_SecureChannel *channel = connection->channel;
    if(channel)
        uatomic_cmpxchg(&channel->connection, connection, NULL);
    uatomic_set(&connection->channel, NULL);
#else
    if(connection->channel)
        connection->channel->connection = NULL;
    connection->channel = NULL;
#endif
}

void UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel) {
#ifdef UA_ENABLE_MULTITHREADING
    if(uatomic_cmpxchg(&channel->connection, NULL, connection) == NULL)
        uatomic_set((void**)&connection->channel, (void*)channel);
#else
    if(channel->connection != NULL)
        return;
    channel->connection = connection;
    connection->channel = channel;
#endif
}

#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
