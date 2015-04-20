#ifndef UA_SECURECHANNEL_H_
#define UA_SECURECHANNEL_H_

#include "ua_types_generated.h"
#include "ua_transport_generated.h"
#include "ua_connection.h"

/**
 *  @ingroup communication
 *
 * @{
 */

struct UA_Session;
typedef struct UA_Session UA_Session;

struct UA_SecureChannel {
    UA_MessageSecurityMode  securityMode;
    UA_ChannelSecurityToken securityToken; // the channelId is contained in the securityToken
    UA_AsymmetricAlgorithmSecurityHeader clientAsymAlgSettings;
    UA_AsymmetricAlgorithmSecurityHeader serverAsymAlgSettings;
    UA_ByteString  clientNonce;
    UA_ByteString  serverNonce;
    UA_UInt32      requestId;
    UA_UInt32      sequenceNumber;
    UA_Connection *connection;
    UA_Session    *session;
};

void UA_SecureChannel_init(UA_SecureChannel *channel);
void UA_SecureChannel_deleteMembers(UA_SecureChannel *channel);

UA_StatusCode UA_SecureChannel_generateNonce(UA_ByteString *nonce);
UA_StatusCode UA_SecureChannel_updateRequestId(UA_SecureChannel *channel, UA_UInt32 requestId);
UA_StatusCode UA_SecureChannel_updateSequenceNumber(UA_SecureChannel *channel, UA_UInt32 sequenceNumber);

void UA_SecureChannel_attachSession(UA_SecureChannel *channel, UA_Session *session);
void UA_SecureChannel_detachSession(UA_SecureChannel *channel);

/** @} */

#endif /* UA_SECURECHANNEL_H_ */
