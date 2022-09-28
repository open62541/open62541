/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#ifndef UA_SERVER_CONFIG_DEFAULT_H_
#define UA_SERVER_CONFIG_DEFAULT_H_

#include <open62541/server.h>

_UA_BEGIN_DECLS

/* Create a new server with default plugins for logging etc. used during
 * initialization. No network layer and SecurityPolicies are set so far. */
UA_Server UA_EXPORT * UA_Server_new(void);

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
                                               const UA_ByteString *issuerList,
                                               size_t issuerListSize,
                                               const UA_ByteString *revocationList,
                                               size_t revocationListSize);

#endif

/* Creates a server config on the default port 4840 with no server
 * certificate. */
static UA_INLINE UA_StatusCode
UA_ServerConfig_setDefault(UA_ServerConfig *config) {
    return UA_ServerConfig_setMinimal(config, 4840, NULL);
}

/* Creates a new server config with no network layer and no endpoints.
 *
 * It initializes reasonable defaults for many things, but does not
 * add any network layer, security policies and endpoints.
 * Use the various UA_ServerConfig_addXxx functions to add them.
 *
 * @param conf The configuration to manipulate
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_setBasics(UA_ServerConfig *conf);

#ifdef UA_ENABLE_WEBSOCKET_SERVER
/* Adds a Websocket network layer with custom buffer sizes
 *
 * @param conf The configuration to manipulate
 * @param portNumber The port number for the tcp network layer
 * @param sendBufferSize The size in bytes for the network send buffer. Pass 0
 *        to use defaults.
 * @param recvBufferSize The size in bytes for the network receive buffer.
 *        Pass 0 to use defaults.
 * @param certificate  certificate data. Pass NULL to disable WS security
 * @param privateKey   privateKey data. Pass NULL to disable WS security
 */

UA_EXPORT UA_StatusCode
UA_ServerConfig_addNetworkLayerWS(UA_ServerConfig *conf, UA_UInt16 portNumber,
                                  UA_UInt32 sendBufferSize, UA_UInt32 recvBufferSize, const UA_ByteString* certificate, const UA_ByteString* privateKey);
#endif

/* Adds the security policy ``SecurityPolicy#None`` to the server. A
 * server certificate may be supplied but is optional.
 *
 * @param config The configuration to manipulate
 * @param certificate The optional server certificate.
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyNone(UA_ServerConfig *config,
                                      const UA_ByteString *certificate);

#ifdef UA_ENABLE_ENCRYPTION

/* Adds the security policy ``SecurityPolicy#Basic128Rsa15`` to the server. A
 * server certificate may be supplied but is optional.
 *
 * Certificate verification should be configured before calling this
 * function. See PKI plugin.
 *
 * @param config The configuration to manipulate
 * @param certificate The server certificate.
 * @param privateKey The private key that corresponds to the certificate.
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic128Rsa15(UA_ServerConfig *config,
                                               const UA_ByteString *certificate,
                                               const UA_ByteString *privateKey);

/* Adds the security policy ``SecurityPolicy#Basic256`` to the server. A
 * server certificate may be supplied but is optional.
 *
 * Certificate verification should be configured before calling this
 * function. See PKI plugin.
 *
 * @param config The configuration to manipulate
 * @param certificate The server certificate.
 * @param privateKey The private key that corresponds to the certificate.
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic256(UA_ServerConfig *config,
                                          const UA_ByteString *certificate,
                                          const UA_ByteString *privateKey);

/* Adds the security policy ``SecurityPolicy#Basic256Sha256`` to the server. A
 * server certificate may be supplied but is optional.
 *
 * Certificate verification should be configured before calling this
 * function. See PKI plugin.
 *
 * @param config The configuration to manipulate
 * @param certificate The server certificate.
 * @param privateKey The private key that corresponds to the certificate.
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyBasic256Sha256(UA_ServerConfig *config,
                                                const UA_ByteString *certificate,
                                                const UA_ByteString *privateKey);

/* Adds the security policy ``SecurityPolicy#Aes128Sha256RsaOaep`` to the server. A
 * server certificate may be supplied but is optional.
 *
 * Certificate verification should be configured before calling this
 * function. See PKI plugin.
 *
 * @param config The configuration to manipulate
 * @param certificate The server certificate.
 * @param privateKey The private key that corresponds to the certificate.
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(UA_ServerConfig *config,
                                                     const UA_ByteString *certificate,
                                                     const UA_ByteString *privateKey);

/* Adds all supported security policies and sets up certificate
 * validation procedures.
 *
 * Certificate verification should be configured before calling this
 * function. See PKI plugin.
 *
 * @param config The configuration to manipulate
 * @param certificate The server certificate.
 * @param privateKey The private key that corresponds to the certificate.
 * @param trustList The trustList for client certificate validation.
 * @param trustListSize The trustList size.
 * @param revocationList The revocationList for client certificate validation.
 * @param revocationListSize The revocationList size.
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_addAllSecurityPolicies(UA_ServerConfig *config,
                                       const UA_ByteString *certificate,
                                       const UA_ByteString *privateKey);

#endif

/* Adds an endpoint for the given security policy and mode. The security
 * policy has to be added already. See UA_ServerConfig_addXxx functions.
 *
 * @param config The configuration to manipulate
 * @param securityPolicyUri The security policy for which to add the endpoint.
 * @param securityMode The security mode for which to add the endpoint.
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_addEndpoint(UA_ServerConfig *config, const UA_String securityPolicyUri,
                            UA_MessageSecurityMode securityMode);

/* Adds endpoints for all configured security policies in each mode.
 *
 * @param config The configuration to manipulate
 */
UA_EXPORT UA_StatusCode
UA_ServerConfig_addAllEndpoints(UA_ServerConfig *config);

_UA_END_DECLS

#endif /* UA_SERVER_CONFIG_DEFAULT_H_ */
