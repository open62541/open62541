#include "ua_securechannel.h"
#include "ua_util.h"
#include "ua_statuscodes.h"

#ifdef UA_MULTITHREADING
#include <urcu/uatomic.h>
#endif

// max message size is 64k
const UA_ConnectionConfig UA_ConnectionConfig_standard =
    {.protocolVersion = 0, .sendBufferSize = 65536, .recvBufferSize  = 65536,
     .maxMessageSize = 65536, .maxChunkCount   = 1};

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

UA_Int32 UA_SecureChannel_updateSequenceNumber(UA_SecureChannel *channel, UA_UInt32 sequenceNumber) {
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
