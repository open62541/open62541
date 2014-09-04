#ifndef UA_SECURECHANNEL_H_
#define UA_SECURECHANNEL_H_

#include <stdio.h>
#include <memory.h> // memcpy

#include "ua_types_generated.h"
#include "ua_transport.h"
#include "ua_connection.h"

/**
 *  @ingroup internal
 *
 *  @defgroup securechannel SecureChannel
 */

struct UA_Session;
typedef struct UA_Session UA_Session;

struct UA_SecureChannel {
	UA_MessageSecurityMode securityMode;
	UA_ChannelSecurityToken securityToken; // the channelId is contained in the securityToken
	UA_AsymmetricAlgorithmSecurityHeader clientAsymAlgSettings;
	UA_AsymmetricAlgorithmSecurityHeader serverAsymAlgSettings;
	UA_ByteString clientNonce;
	UA_ByteString serverNonce;
	UA_UInt32 requestId;
	UA_UInt32 sequenceNumber;
	UA_Connection *connection; // make this more generic when http connections exist
	UA_Session *session;
};

UA_Int32 UA_SecureChannel_init(UA_SecureChannel *channel);
UA_Int32 UA_SecureChannel_deleteMembers(UA_SecureChannel *channel);
UA_Int32 UA_SecureChannel_delete(UA_SecureChannel *channel);
UA_Boolean UA_SecureChannel_compare(UA_SecureChannel *sc1, UA_SecureChannel *sc2);

UA_Int32 UA_SecureChannel_generateNonce(UA_ByteString *nonce);
UA_Int32 UA_SecureChannel_updateRequestId(UA_SecureChannel *channel, UA_UInt32 requestId);
UA_Int32 UA_SecureChannel_updateSequenceNumber(UA_SecureChannel *channel, UA_UInt32 sequenceNumber);

#endif /* UA_SECURECHANNEL_H_ */
