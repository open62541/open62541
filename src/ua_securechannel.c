#include <time.h>
#include <stdlib.h>
#include "ua_securechannel.h"
#include "ua_util.h"
#include "ua_statuscodes.h"

void UA_SecureChannel_init(UA_SecureChannel *channel) {
    UA_AsymmetricAlgorithmSecurityHeader_init(&channel->clientAsymAlgSettings);
    UA_AsymmetricAlgorithmSecurityHeader_init(&channel->serverAsymAlgSettings);
    UA_ByteString_init(&channel->clientNonce);
    UA_ByteString_init(&channel->serverNonce);
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
UA_Int32 UA_SecureChannel_generateNonce(UA_ByteString *nonce) {
    if(!(nonce->data = UA_alloc(1)))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    nonce->length  = 1;
    nonce->data[0] = 'a';
    return UA_SUCCESS;
}

UA_Int32 UA_SecureChannel_updateRequestId(UA_SecureChannel *channel, UA_UInt32 requestId) {
    //TODO review checking of request id
    if(channel->requestId+1  == requestId) {
        channel->requestId++;
        return UA_SUCCESS;
    }
    return UA_ERROR;
}

UA_Int32 UA_SecureChannel_updateSequenceNumber(UA_SecureChannel *channel,
                                               UA_UInt32         sequenceNumber) {
    //TODO review checking of sequence
    if(channel->sequenceNumber+1  == sequenceNumber) {
        channel->sequenceNumber++;
        return UA_SUCCESS;
    }
    return UA_ERROR;

}
