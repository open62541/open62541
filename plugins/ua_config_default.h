/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_CONFIG_DEFAULT_H_
#define UA_CONFIG_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_server.h"
#include "ua_server_config.h"
#include "ua_client.h"

/**********************/
/* Default Connection */
/**********************/

extern const UA_EXPORT UA_ConnectionConfig UA_ConnectionConfig_default;

/*************************/
/* Default Server Config */
/*************************/



/* Creates a new server config with one endpoint and custom buffer size.
 *
 * The config will set the tcp network layer to the given port and adds a single
 * endpoint with the security policy ``SecurityPolicy#None`` to the server. A
 * server certificate may be supplied but is optional.
 * Additionally you can define a custom buffer size for send and receive buffer.
 *
 * @param portNumber The port number for the tcp network layer
 * @param certificate Optional certificate for the server endpoint. Can be
 *        ``NULL``.
 * @param sendBufferSize The size in bytes for the network send buffer
 * @param recvBufferSize The size in bytes for the network receive buffer
 *
 */
UA_EXPORT UA_ServerConfig *
UA_ServerConfig_new_customBuffer(UA_UInt16 portNumber, const UA_ByteString *certificate, UA_UInt32 sendBufferSize, UA_UInt32 recvBufferSize);

/* Creates a new server config with one endpoint.
 * 
 * The config will set the tcp network layer to the given port and adds a single
 * endpoint with the security policy ``SecurityPolicy#None`` to the server. A
 * server certificate may be supplied but is optional.
 *
 * @param portNumber The port number for the tcp network layer
 * @param certificate Optional certificate for the server endpoint. Can be
 *        ``NULL``. */
static UA_INLINE UA_ServerConfig *
UA_ServerConfig_new_minimal(UA_UInt16 portNumber, const UA_ByteString *certificate) {
    return UA_ServerConfig_new_customBuffer(portNumber, certificate, 0 ,0);
}

#ifdef UA_ENABLE_ENCRYPTION

UA_EXPORT UA_ServerConfig *
UA_ServerConfig_new_basic128rsa15(UA_UInt16 portNumber,
                                  const UA_ByteString *certificate,
                                  const UA_ByteString *privateKey,
                                  const UA_ByteString *trustList,
                                  size_t trustListSize,
                                  const UA_ByteString *revocationList,
                                  size_t revocationListSize);

UA_EXPORT UA_ServerConfig *
UA_ServerConfig_new_basic256sha256(UA_UInt16 portNumber,
                                   const UA_ByteString *certificate,
                                   const UA_ByteString *privateKey,
                                   const UA_ByteString *trustList,
                                   size_t trustListSize,
                                   const UA_ByteString *revocationList,
                                   size_t revocationListSize);

UA_EXPORT UA_ServerConfig *
UA_ServerConfig_new_allSecurityPolicies(UA_UInt16 portNumber,
                                        const UA_ByteString *certificate,
                                        const UA_ByteString *privateKey,
                                        const UA_ByteString *trustList,
                                        size_t trustListSize,
                                        const UA_ByteString *revocationList,
                                        size_t revocationListSize);

#endif

/* Creates a server config on the default port 4840 with no server
 * certificate. */
static UA_INLINE UA_ServerConfig *
UA_ServerConfig_new_default(void) {
    return UA_ServerConfig_new_minimal(4840, NULL);
}

/* Set a custom hostname in server configuration
 *
 * @param config A valid server configuration
 * @param customHostname The custom hostname used by the server */

UA_EXPORT void
UA_ServerConfig_set_customHostname(UA_ServerConfig *config,
                                   const UA_String customHostname);

/* Frees allocated memory in the server config */
UA_EXPORT void
UA_ServerConfig_delete(UA_ServerConfig *config);

/*************************/
/* Default Client Config */
/*************************/

extern const UA_EXPORT UA_ClientConfig UA_ClientConfig_default;

#ifdef __cplusplus
}
#endif

#endif /* UA_CONFIG_DEFAULT_H_ */
