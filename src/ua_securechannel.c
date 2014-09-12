#include <time.h>
#include <stdlib.h>
#include "ua_securechannel.h"
#include "util/ua_util.h"

UA_Int32 UA_SecureChannel_init(UA_SecureChannel *channel) {
    UA_AsymmetricAlgorithmSecurityHeader_init(&channel->clientAsymAlgSettings);
    UA_AsymmetricAlgorithmSecurityHeader_init(&channel->serverAsymAlgSettings);
    UA_ByteString_init(&channel->clientNonce);
    UA_ByteString_init(&channel->serverNonce);
    channel->connection = UA_NULL;
    channel->session    = UA_NULL;
    return UA_SUCCESS;
}

UA_Int32 UA_SecureChannel_deleteMembers(UA_SecureChannel *channel) {
    UA_Int32 retval = UA_SUCCESS;
    retval |= UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->serverAsymAlgSettings);
    retval |= UA_ByteString_deleteMembers(&channel->serverNonce);
    retval |= UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&channel->clientAsymAlgSettings);
    retval |= UA_ByteString_deleteMembers(&channel->clientNonce);
    retval |= UA_ChannelSecurityToken_deleteMembers(&channel->securityToken);
    return retval;
}
UA_Int32 UA_SecureChannel_delete(UA_SecureChannel *channel) {
    UA_Int32 retval = UA_SUCCESS;
    retval |= UA_SecureChannel_deleteMembers(channel);
    retval |= UA_free(channel);
    return retval;
}

UA_Boolean UA_SecureChannel_compare(UA_SecureChannel *sc1, UA_SecureChannel *sc2) {
    return (sc1->securityToken.channelId == sc2->securityToken.channelId) ?
           UA_TRUE : UA_FALSE;
}

//TODO implement real nonce generator - DUMMY function
UA_Int32 UA_SecureChannel_generateNonce(UA_ByteString *nonce) {
    //UA_ByteString_new(&nonce);
    UA_alloc((void ** )&nonce->data, 1);
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
