/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_SERVER_CONFIG_DEFAULT_H_
#define UA_SERVER_CONFIG_DEFAULT_H_

#include <open62541/server_config.h>

_UA_BEGIN_DECLS

/**********************/
/* Default Connection */
/**********************/

extern const UA_EXPORT
UA_ConnectionConfig UA_ConnectionConfig_default;

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
UA_EXPORT UA_StatusCode
UA_ServerConfig_setMinimalCustomBuffer(UA_ServerConfig *config,
                                       UA_UInt16 portNumber,
                                       const UA_ByteString *certificate,
                                       UA_UInt32 sendBufferSize,
                                       UA_UInt32 recvBufferSize);

/* Creates a new server config with one endpoint.
 *
 * The config will set the tcp network layer to the given port and adds a single
 * endpoint with the security policy ``SecurityPolicy#None`` to the server. A
 * server certificate may be supplied but is optional. */
static UA_INLINE UA_StatusCode
UA_ServerConfig_setMinimal(UA_ServerConfig *config, UA_UInt16 portNumber,
                           const UA_ByteString *certificate) {
    return UA_ServerConfig_setMinimalCustomBuffer(config, portNumber,
                                                  certificate, 0, 0);
}

#ifdef UA_ENABLE_ENCRYPTION

UA_EXPORT UA_StatusCode
UA_ServerConfig_setDefaultWithSecurityPolicies(UA_ServerConfig *conf,
                                               UA_UInt16 portNumber,
                                               const UA_ByteString *certificate,
                                               const UA_ByteString *privateKey,
                                               const UA_ByteString *trustList,
                                               size_t trustListSize,
                                               const UA_ByteString *revocationList,
                                               size_t revocationListSize);

#endif

/* Creates a server config on the default port 4840 with no server
 * certificate. */
static UA_INLINE UA_StatusCode
UA_ServerConfig_setDefault(UA_ServerConfig *config) {
    return UA_ServerConfig_setMinimal(config, 4840, NULL);
}

_UA_END_DECLS

#endif /* UA_SERVER_CONFIG_DEFAULT_H_ */
