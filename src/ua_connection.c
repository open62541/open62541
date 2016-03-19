#include "ua_util.h"
#include "ua_connection.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_securechannel.h"

// max message size is 64k
const UA_ConnectionConfig UA_ConnectionConfig_standard =
    {.protocolVersion = 0, .sendBufferSize = 65536, .recvBufferSize  = 65536,
     .maxMessageSize = 65536, .maxChunkCount   = 1};

void UA_Connection_init(UA_Connection *connection) {
    connection->state = UA_CONNECTION_CLOSED;
    connection->localConf = UA_ConnectionConfig_standard;
    connection->remoteConf = UA_ConnectionConfig_standard;
    connection->channel = NULL;
    connection->sockfd = 0;
    connection->handle = NULL;
    UA_ByteString_init(&connection->incompleteMessage);
    connection->send = NULL;
    connection->close = NULL;
    connection->recv = NULL;
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
    UA_ByteString *current = message;
    *realloced = false;
    if(connection->incompleteMessage.length > 0) {
        /* concat the existing incomplete message with the new message */
        UA_Byte *data = UA_realloc(connection->incompleteMessage.data,
                                   connection->incompleteMessage.length + message->length);
        if(!data) {
            /* not enough memory */
            UA_ByteString_deleteMembers(&connection->incompleteMessage);
            connection->releaseRecvBuffer(connection, message);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        memcpy(&data[connection->incompleteMessage.length], message->data, message->length);
        connection->incompleteMessage.data = data;
        connection->incompleteMessage.length += message->length;
        connection->releaseRecvBuffer(connection, message);
        current = &connection->incompleteMessage;
        *realloced = true;
    }

    /* the while loop sets pos to the first element after the last complete message. if a message
       contains garbage, the buffer length is set to contain only the "good" messages before. */
    size_t pos = 0;
    size_t delete_at = current->length-1; // garbled message after this point
    while(current->length - pos >= 16) {
        UA_UInt32 msgtype = (UA_UInt32)current->data[pos] +
            ((UA_UInt32)current->data[pos+1] << 8) +
            ((UA_UInt32)current->data[pos+2] << 16);
        if(msgtype != ('M' + ('S' << 8) + ('G' << 16)) &&
           msgtype != ('O' + ('P' << 8) + ('N' << 16)) &&
           msgtype != ('H' + ('E' << 8) + ('L' << 16)) &&
           msgtype != ('A' + ('C' << 8) + ('K' << 16)) &&
           msgtype != ('C' + ('L' << 8) + ('O' << 16))) {
            /* the message type is not recognized */
            delete_at = pos; // throw the remaining message away
            break;
        }
        UA_UInt32 length = 0;
        size_t length_pos = pos + 4;
        UA_StatusCode retval = UA_UInt32_decodeBinary(current, &length_pos, &length);
        if(retval != UA_STATUSCODE_GOOD || length < 16 || length > connection->localConf.recvBufferSize) {
            /* the message size is not allowed. throw the remaining bytestring away */
            delete_at = pos;
            break;
        }
        if(length + pos > current->length)
            break; /* the message is incomplete. keep the beginning */
        pos += length;
    }

    /* throw the message away */
    if(delete_at == 0) {
        if(!*realloced) {
            connection->releaseRecvBuffer(connection, message);
            *realloced = true;
        } else
            UA_ByteString_deleteMembers(current);
        return UA_STATUSCODE_GOOD;
    }

    /* no complete message at all */
    if(pos == 0) {
        if(!*realloced) {
            /* store the buffer in the connection */
            UA_ByteString_copy(current, &connection->incompleteMessage);
            connection->releaseRecvBuffer(connection, message);
            *realloced = true;
        } 
        return UA_STATUSCODE_GOOD;
    }

    /* there remains an incomplete message at the end */
    if(current->length != pos) {
        UA_Byte *data = UA_malloc(current->length - pos);
        if(!data) {
            UA_ByteString_deleteMembers(&connection->incompleteMessage);
            if(!*realloced) {
                connection->releaseRecvBuffer(connection, message);
                *realloced = true;
            }
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        size_t newlength = current->length - pos;
        memcpy(data, &current->data[pos], newlength);
        current->length = pos;
        if(*realloced)
            *message = *current;
        connection->incompleteMessage.data = data;
        connection->incompleteMessage.length = newlength;
        return UA_STATUSCODE_GOOD;
    }

    if(current == &connection->incompleteMessage) {
        *message = *current;
        connection->incompleteMessage = UA_BYTESTRING_NULL;
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
