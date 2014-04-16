#ifndef OPCUA_TRANSPORT_BINARY_SECURE_H_
#define OPCUA_TRANSPORT_BINARY_SECURE_H_
#include "opcua.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"
#include "ua_transport_binary_secure.h"
#include "ua_application.h"

typedef struct {
	UA_UInt32 secureChannelId;
	UA_SymmetricAlgorithmSecurityHeader tokenId;
	UA_DateTime createdAt;
	UA_Int32 revisedLifetime;
} SL_ChannelSecurityToken;

typedef struct SL_Channel_T {
	UA_String secureChannelId;
	TL_connection* tlConnection;
	UA_Session *session; // equals UA_Null iff no session is active
	UA_AsymmetricAlgorithmSecurityHeader remoteAsymAlgSettings;
	UA_AsymmetricAlgorithmSecurityHeader localAsymAlgSettings;
	UA_SequenceHeader sequenceHeader;
	UA_UInt32 securityMode;
	UA_ByteString remoteNonce;
	UA_ByteString localNonce;
	UA_UInt32 connectionState;
	SL_ChannelSecurityToken securityToken;

	UA_Application *application;
} SL_Channel;

UA_Int32 SL_initConnectionObject(SL_Channel *connection);
UA_Int32 SL_process(SL_Channel* channel, UA_ByteString* msg, UA_Int32* pos);
UA_Int32 SL_Channel_new(TL_connection *connection, UA_ByteString* msg, UA_Int32* pos);

#endif /* OPCUA_TRANSPORT_BINARY_SECURE_H_ */
