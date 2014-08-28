#ifndef OPCUA_TRANSPORT_BINARY_SECURE_H_
#define OPCUA_TRANSPORT_BINARY_SECURE_H_
#include "ua_types.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"
#include "ua_channel.h"
#include "ua_channel_manager.h"
#include "ua_server.h"

UA_Int32 SL_Process(UA_Server *server, const UA_ByteString* msg, UA_UInt32* pos);

/**
 * @brief Wrapper function, to encapsulate handleRequest for openSecureChannel requests
 * @param channel A secure Channel structure, which receives the information for the new created secure channel
 * @param msg Message which holds the binary encoded request
 * @param pos Position in the message at which the request begins
 * @return Returns UA_SUCCESS if successful executed, UA_ERROR in any other case
 */
UA_Int32 SL_ProcessOpenChannel(SL_Channel *channel, UA_Server *server, const UA_ByteString* msg, UA_UInt32 *pos);
UA_Int32 SL_ProcessCloseChannel(SL_Channel *channel, const UA_ByteString* msg, UA_UInt32 *pos);

#endif /* OPCUA_TRANSPORT_BINARY_SECURE_H_ */
