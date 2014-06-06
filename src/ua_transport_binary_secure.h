#ifndef OPCUA_TRANSPORT_BINARY_SECURE_H_
#define OPCUA_TRANSPORT_BINARY_SECURE_H_
#include "ua_types.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"

typedef struct {
	UA_UInt32 secureChannelId;
	UA_SymmetricAlgorithmSecurityHeader tokenId;
	UA_DateTime createdAt;
	UA_Int32 revisedLifetime;
} SL_ChannelSecurityToken;

typedef struct SL_Channel {
	UA_String secureChannelId;
	TL_Connection* tlConnection;
	Session *session; // equals UA_Null iff no session is active
	UA_AsymmetricAlgorithmSecurityHeader remoteAsymAlgSettings;
	UA_AsymmetricAlgorithmSecurityHeader localAsymAlgSettings;
	UA_SequenceHeader sequenceHeader;
	UA_UInt32 securityMode;
	UA_ByteString remoteNonce;
	UA_ByteString localNonce;
	UA_UInt32 connectionState;
	SL_ChannelSecurityToken securityToken;
} SL_Channel;

UA_Int32 SL_Process(SL_Channel* channel, const UA_ByteString* msg, UA_UInt32* offset);
UA_Int32 SL_Channel_new(TL_Connection *connection, const UA_ByteString* msg, UA_UInt32* offset); // this function is called from the OpenSecureChannel service

#endif /* OPCUA_TRANSPORT_BINARY_SECURE_H_ */
