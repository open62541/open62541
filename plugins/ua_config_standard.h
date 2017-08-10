/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifndef UA_CONFIG_STANDARD_H_
#define UA_CONFIG_STANDARD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"
#include "ua_client.h"
#include "ua_client_highlevel.h"
#include "ua_types.h"

extern const UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_standard;
extern const UA_EXPORT UA_ServerConfig UA_ServerConfig_standard;
extern const UA_EXPORT UA_ClientConfig UA_ClientConfig_standard;

/**
 * \brief Creates a new server config with one endpoint.
 * 
 * The config will set the tcp network layer and the security policy
 * SecurityPolicy#None, creating one endpoint. A certificate may be supplied but can be omitted.
 *
 * \param portNumber the port number for the tcp network layer
 * \param certificate an optional certificate for the endpoint
 */
UA_EXPORT UA_ServerConfig*
UA_ServerConfig_standard_parametrized_new(UA_UInt16 portNumber,
                                          const UA_ByteString *certificate);

UA_EXPORT UA_ServerConfig*
UA_ServerConfig_standard_new(void);

UA_EXPORT void UA_ServerConfig_standard_delete(UA_ServerConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* UA_CONFIG_STANDARD_H_ */
