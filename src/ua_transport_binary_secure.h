#ifndef OPCUA_TRANSPORT_BINARY_SECURE_H_
#define OPCUA_TRANSPORT_BINARY_SECURE_H_
#include "opcua.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"
#include "ua_stack_channel.h"
#include "ua_stack_channel_manager.h"

/*inputs for secure Channel which must be provided once
endPointUrl
securityPolicyUrl
securityMode
revisedLifetime
*/

/*inputs for secure Channel Manager which must be provided once
 maxChannelCount

 */



typedef struct {
	UA_UInt32 secureChannelId;
	UA_SymmetricAlgorithmSecurityHeader tokenId;
	UA_DateTime createdAt;
	UA_Int32 revisedLifetime;
} SL_ChannelSecurityToken;
/*
typedef struct SL_Channel_T {
	UA_String secureChannelId;
	UA_TL_Connection1 tlConnection;
	Session *session; // equals UA_Null if no session is active
	UA_AsymmetricAlgorithmSecurityHeader remoteAsymAlgSettings;
	UA_AsymmetricAlgorithmSecurityHeader localAsymAlgSettings;
	UA_SequenceHeader sequenceHeader;
	UA_UInt32 securityMode;
	UA_ByteString remoteNonce;
	UA_ByteString localNonce;
	UA_UInt32 connectionState;
	SL_ChannelSecurityToken securityToken;
} SL_secureChannel;

*/
UA_Int32 SL_Process(const UA_ByteString* msg, UA_Int32* pos);

/**
 * @brief Wrapper function, to encapsulate handleRequest for openSecureChannel requests
 * @param channel A secure Channel structure, which receives the information for the new created secure channel
 * @param msg Message which holds the binary encoded request
 * @param pos Position in the message at which the request begins
 * @return Returns UA_SUCCESS if successful executed, UA_ERROR in any other case

 */
UA_Int32 SL_ProcessOpenChannel(SL_secureChannel channel, const UA_ByteString* msg,
		UA_Int32 *pos);
#endif /* OPCUA_TRANSPORT_BINARY_SECURE_H_ */
