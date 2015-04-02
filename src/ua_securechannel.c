#include "ua_util.h"
#include "ua_securechannel.h"
#include "ua_statuscodes.h"
#include "ua_types_encoding_binary.h"

// max message size is 64k
const UA_ConnectionConfig UA_ConnectionConfig_standard =
    {.protocolVersion = 0, .sendBufferSize = 65536, .recvBufferSize  = 65536,
     .maxMessageSize = 65536, .maxChunkCount   = 1};

UA_ByteString UA_Connection_completeMessages(UA_Connection *connection, UA_ByteString received)
{
    /* concat received to the incomplete message we have */
    UA_ByteString current;
    if(connection->incompleteMessage.length < 0)
        current = received;
    else {
        UA_Byte *longer = UA_realloc(connection->incompleteMessage.data,
                                     connection->incompleteMessage.length + received.length);
        if(!longer) {
            UA_ByteString_deleteMembers(&received);
            UA_ByteString_deleteMembers(&connection->incompleteMessage);
            connection->incompleteMessage.length = -1;
            received.length = -1;
            return received;
        }
        UA_memcpy(&longer[connection->incompleteMessage.length], received.data, received.length);
        current.data = longer;
        current.length = connection->incompleteMessage.length + received.length;
    }

    /* find the first non-complete message */
    size_t end_pos = 0;
    while(current.length - end_pos >= 32) {
        if(!(current.data[0] == 'M' && current.data[1] == 'S' && current.data[2] == 'G') &&
           !(current.data[0] == 'O' && current.data[1] == 'P' && current.data[2] == 'N') &&
           !(current.data[0] == 'H' && current.data[1] == 'E' && current.data[2] == 'L') &&
           !(current.data[0] == 'A' && current.data[1] == 'C' && current.data[2] == 'K') &&
           !(current.data[0] == 'C' && current.data[1] == 'L' && current.data[2] == 'O')) {
            current.length = end_pos; // throw the remaining bytestring away
            break;
        }
        UA_Int32 length;
        size_t pos = end_pos + 4;
        UA_Int32_decodeBinary(&received, &pos, &length);
        if(length < 32 || length > (UA_Int32)connection->localConf.maxMessageSize) {
            current.length = end_pos; // throw the remaining bytestring away
            break;
        }
        if(length + (UA_Int32)end_pos > current.length)
            break; // the message is not complete
        end_pos += length;
    }

    /* return all complete messages, retain the (last) incomplete one */
    if(current.length == 0) {
        UA_String_deleteMembers(&current);
        current.length = -1;
    } else {
        if(current.length - end_pos > 0) {
            connection->incompleteMessage.data = UA_malloc(current.length - end_pos);
            if(!connection->incompleteMessage.data)
                UA_ByteString_init(&connection->incompleteMessage);
            else {
                UA_memcpy(&current.data[end_pos], connection->incompleteMessage.data,
                          current.length - end_pos);
                connection->incompleteMessage.length = current.length - end_pos;
            }
        }
        current.length = end_pos;
    }
    return current;
}

void UA_SecureChannel_init(UA_SecureChannel *channel) {
    UA_MessageSecurityMode_init(&channel->securityMode);
    UA_ChannelSecurityToken_init(&channel->securityToken);
    UA_AsymmetricAlgorithmSecurityHeader_init(&channel->clientAsymAlgSettings);
    UA_AsymmetricAlgorithmSecurityHeader_init(&channel->serverAsymAlgSettings);
    UA_ByteString_init(&channel->clientNonce);
    UA_ByteString_init(&channel->serverNonce);
    channel->requestId = 0;
    channel->sequenceNumber = 0;
    channel->connection = UA_NULL;
    channel->session    = UA_NULL;
}

void UA_SecureChannel_deleteMembers(UA_SecureChannel *channel) {
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->serverAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->serverNonce);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->clientAsymAlgSettings);
    UA_ByteString_deleteMembers(&channel->clientNonce);
    UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
}

void UA_SecureChannel_delete(UA_SecureChannel *channel) {
    UA_SecureChannel_deleteMembers(channel);
    UA_free(channel);
}

UA_Boolean UA_SecureChannel_compare(UA_SecureChannel *sc1, UA_SecureChannel *sc2) {
    return (sc1->securityToken.channelId == sc2->securityToken.channelId);
}

//TODO implement real nonce generator - DUMMY function
UA_StatusCode UA_SecureChannel_generateNonce(UA_ByteString *nonce) {
    if(!(nonce->data = UA_malloc(1)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    nonce->length  = 1;
    nonce->data[0] = 'a';
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_SecureChannel_updateRequestId(UA_SecureChannel *channel, UA_UInt32 requestId) {
    //TODO review checking of request id
    if(channel->requestId+1 != requestId)
        return UA_STATUSCODE_BADINTERNALERROR;
    channel->requestId++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_SecureChannel_updateSequenceNumber(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
    //TODO review checking of sequence
    if(channel->sequenceNumber+1  != sequenceNumber)
        return UA_STATUSCODE_BADINTERNALERROR;
    channel->sequenceNumber++;
    return UA_STATUSCODE_GOOD;
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
