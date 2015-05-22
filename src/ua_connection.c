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
    connection->localConf = (UA_ConnectionConfig){0,0,0,0,0};
    connection->remoteConf = (UA_ConnectionConfig){0,0,0,0,0};
    connection->channel = UA_NULL;
    connection->sockfd = 0;
    connection->handle = UA_NULL;
    UA_ByteString_init(&connection->incompleteMessage);
    connection->write = UA_NULL;
    connection->close = UA_NULL;
    connection->recv = UA_NULL;
    connection->getBuffer = UA_NULL;
    connection->releaseBuffer = UA_NULL;
}

void UA_Connection_deleteMembers(UA_Connection *connection) {
    UA_ByteString_deleteMembers(&connection->incompleteMessage);
}

UA_ByteString UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString received)
{
    if(received.length == -1)
        return received;

    /* concat the existing incomplete message with the new message */
    UA_ByteString current;
    if(connection->incompleteMessage.length < 0)
        current = received;
    else {
        current.data = UA_realloc(connection->incompleteMessage.data,
                                  connection->incompleteMessage.length + received.length);
        if(!current.data) {
            /* not enough memory */
            UA_ByteString_deleteMembers(&connection->incompleteMessage);
            connection->incompleteMessage.length = -1;
            UA_ByteString_deleteMembers(&received);
            received.length = -1;
            return received;
        }
        UA_memcpy(current.data + connection->incompleteMessage.length, received.data, received.length);
        current.length = connection->incompleteMessage.length + received.length;
        UA_ByteString_deleteMembers(&received);
        UA_ByteString_init(&connection->incompleteMessage);
    }

    /* find the first non-complete message */
    size_t end_pos = 0; // the end of the last complete message
    while(current.length - end_pos >= 16) {
        if(!(current.data[0] == 'M' && current.data[1] == 'S' && current.data[2] == 'G') &&
           !(current.data[0] == 'O' && current.data[1] == 'P' && current.data[2] == 'N') &&
           !(current.data[0] == 'H' && current.data[1] == 'E' && current.data[2] == 'L') &&
           !(current.data[0] == 'A' && current.data[1] == 'C' && current.data[2] == 'K') &&
           !(current.data[0] == 'C' && current.data[1] == 'L' && current.data[2] == 'O')) {
            current.length = end_pos; // throw the remaining bytestring away
            break;
        }
        UA_Int32 length = 0;
        size_t pos = end_pos + 4;
        UA_StatusCode retval = UA_Int32_decodeBinary(&current, &pos, &length);
        if(retval != UA_STATUSCODE_GOOD || length < 16 ||
           length > (UA_Int32)connection->localConf.maxMessageSize) {
            current.length = end_pos; // throw the remaining bytestring away
            break;
        }
        if(length + (UA_Int32)end_pos > current.length)
            break; // the message is incomplete
        end_pos += length;
    }

    if(current.length == 0) {
        /* throw everything away */
        UA_ByteString_deleteMembers(&current);
        current.length = -1;
        return current;
    }

    if(end_pos == 0) {
        /* no complete message in current */
        connection->incompleteMessage = current;
        UA_ByteString_init(&current);
    } else if(current.length != (UA_Int32)end_pos) {
        /* there is an incomplete message at the end of current */
        connection->incompleteMessage.data = UA_malloc(current.length - end_pos);
        if(connection->incompleteMessage.data) {
            UA_memcpy(connection->incompleteMessage.data, &current.data[end_pos], current.length - end_pos);
            connection->incompleteMessage.length = current.length - end_pos;
        }
        current.length = end_pos;
    }
    return current;
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
