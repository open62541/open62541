#include "ua_util.h"
#include "ua_connection.h"
#include "ua_types_encoding_binary.h"
#include "ua_securechannel.h"

// max message size is 64k
const UA_ConnectionConfig UA_ConnectionConfig_standard =
    {.protocolVersion = 0, .sendBufferSize = 65536, .recvBufferSize  = 65536,
     .maxMessageSize = 65536, .maxChunkCount   = 1};

void UA_Connection_init(UA_Connection *connection) {
    connection->state = UA_CONNECTION_CLOSED;
    connection->localConf = UA_ConnectionConfig_standard;
    connection->remoteConf = UA_ConnectionConfig_standard;
    connection->channel = UA_NULL;
    connection->sockfd = 0;
    connection->handle = UA_NULL;
    UA_ByteString_init(&connection->incompleteMessage);
    connection->send = UA_NULL;
    connection->close = UA_NULL;
    connection->recv = UA_NULL;
    connection->getSendBuffer = UA_NULL;
    connection->releaseSendBuffer = UA_NULL;
    connection->releaseRecvBuffer = UA_NULL;
}

void UA_Connection_deleteMembers(UA_Connection *connection) {
    UA_ByteString_deleteMembers(&connection->incompleteMessage);
}

UA_Job UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString received) {
    UA_Job job = (UA_Job){.type = UA_JOBTYPE_NOTHING};
    if(received.length == -1)
        return job;

    UA_ByteString current;
    if(connection->incompleteMessage.length <= 0)
        current = received;
    else {
        /* concat the existing incomplete message with the new message */
        current.data = UA_realloc(connection->incompleteMessage.data,
                                  connection->incompleteMessage.length + received.length);
        if(!current.data) {
            /* not enough memory */
            UA_ByteString_deleteMembers(&connection->incompleteMessage);
            connection->releaseRecvBuffer(connection, &received);
            return job;
        }
        UA_memcpy(current.data + connection->incompleteMessage.length, received.data, received.length);
        current.length = connection->incompleteMessage.length + received.length;
        connection->releaseRecvBuffer(connection, &received);
        UA_ByteString_init(&connection->incompleteMessage);
    }

    /* the while loop sets pos to the first element after the last complete message. if a message
       contains garbage, the buffer length is set to contain only the "good" messages before. */
    size_t pos = 0;
    while(current.length - pos >= 16) {
        UA_UInt32 msgtype = current.data[pos] + (current.data[pos+1] << 8) + (current.data[pos+2] << 16);
        if(msgtype != ('M' + ('S' << 8) + ('G' << 16)) &&
           msgtype != ('O' + ('P' << 8) + ('N' << 16)) &&
           msgtype != ('H' + ('E' << 8) + ('L' << 16)) &&
           msgtype != ('A' + ('C' << 8) + ('K' << 16)) &&
           msgtype != ('C' + ('L' << 8) + ('O' << 16))) {
            /* the message type is not recognized. throw the remaining bytestring away */
            current.length = pos;
            break;
        }
        UA_Int32 length = 0;
        size_t length_pos = pos + 4;
        UA_StatusCode retval = UA_Int32_decodeBinary(&current, &length_pos, &length);
        if(retval != UA_STATUSCODE_GOOD || length < 16 ||
           length > (UA_Int32)connection->localConf.maxMessageSize) {
            /* the message size is not allowed. throw the remaining bytestring away */
            current.length = pos;
            break;
        }
        if(length + (UA_Int32)pos > current.length)
            break; /* the message is incomplete. keep the beginning */
        pos += length;
    }

    if(current.length == 0) {
        /* throw everything away */
        if(current.data == received.data)
            connection->releaseRecvBuffer(connection, &received);
        else
            UA_ByteString_deleteMembers(&current);
        return job;
    }

    if(pos == 0) {
        /* no complete message in current */
        if(current.data == received.data) {
            /* copy the data into the connection */
            UA_ByteString_copy(&current, &connection->incompleteMessage);
            connection->releaseRecvBuffer(connection, &received);
        } else {
            /* the data is already copied off the network stack */
            connection->incompleteMessage = current;
        }
        return job;
    }

    if(current.length != (UA_Int32)pos) {
        /* there is an incomplete message at the end of current */
        connection->incompleteMessage.data = UA_malloc(current.length - pos);
        if(connection->incompleteMessage.data) {
            UA_memcpy(connection->incompleteMessage.data, &current.data[pos], current.length - pos);
            connection->incompleteMessage.length = current.length - pos;
        }
        current.length = pos;
    }

    job.job.binaryMessage.message = current;
    job.job.binaryMessage.connection = connection;
    if(current.data == received.data)
        job.type = UA_JOBTYPE_BINARYMESSAGE_NETWORKLAYER;
    else
        job.type = UA_JOBTYPE_BINARYMESSAGE_ALLOCATED;
    return job;
}

void UA_Connection_detachSecureChannel(UA_Connection *connection) {
#ifdef UA_MULTITHREADING
    UA_SecureChannel *channel = connection->channel;
    if(channel)
        uatomic_cmpxchg(&channel->connection, connection, UA_NULL);
    uatomic_set(&connection->channel, UA_NULL);
#else
    if(connection->channel)
        connection->channel->connection = UA_NULL;
    connection->channel = UA_NULL;
#endif
}

void UA_Connection_attachSecureChannel(UA_Connection *connection, UA_SecureChannel *channel) {
#ifdef UA_MULTITHREADING
    if(uatomic_cmpxchg(&channel->connection, UA_NULL, connection) == UA_NULL)
        uatomic_set(&connection->channel, channel);
#else
    if(channel->connection != UA_NULL)
        return;
    channel->connection = connection;
    connection->channel = channel;
#endif
}
