#ifndef OPCUA_TRANSPORT_BINARY_SECURE_H_
#define OPCUA_TRANSPORT_BINARY_SECURE_H_
#include "ua_types.h"
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

UA_Int32 SL_Process(const UA_ByteString* msg, UA_UInt32* pos);

/**
 * @brief Wrapper function, to encapsulate handleRequest for openSecureChannel requests
 * @param channel A secure Channel structure, which receives the information for the new created secure channel
 * @param msg Message which holds the binary encoded request
 * @param pos Position in the message at which the request begins
 * @return Returns UA_SUCCESS if successful executed, UA_ERROR in any other case

 */
UA_Int32 SL_ProcessOpenChannel(SL_Channel channel, const UA_ByteString* msg,
		UA_UInt32 *pos);
UA_Int32 SL_ProcessCloseChannel(SL_Channel channel, const UA_ByteString* msg,
		UA_UInt32 *pos);
#endif /* OPCUA_TRANSPORT_BINARY_SECURE_H_ */
