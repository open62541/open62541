/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2023 (c) Fraunhofer IOSB (Author: Kai Huebl)
 */

#ifndef UA_CLIENT_CONFIG_DEFAULT_H_
#define UA_CLIENT_CONFIG_DEFAULT_H_

#include <open62541/client.h>

_UA_BEGIN_DECLS

UA_Client UA_EXPORT * UA_Client_new(void);

UA_StatusCode UA_EXPORT
UA_ClientConfig_setDefault(UA_ClientConfig *config);

UA_StatusCode UA_EXPORT
UA_ClientConfig_setAuthenticationUsername(UA_ClientConfig *config,
                                          const char *username, const char *password);

/* If certificates are used for authentication, this is only possible when
 * openssl or mbedtls is used. Libressl is currently not supported.*/
#if defined(UA_ENABLE_ENCRYPTION_OPENSSL) || defined(UA_ENABLE_ENCRYPTION_MBEDTLS)
UA_StatusCode UA_EXPORT
UA_ClientConfig_setAuthenticationCert(UA_ClientConfig *config,
                                      UA_ByteString certificateAuth, UA_ByteString privateKeyAuth);
#endif

UA_EXPORT UA_PKIStore*
UA_ClientConfig_PKIStore_getDefault(UA_Client* client);

UA_EXPORT UA_PKIStore*
UA_ClientConfig_PKIStore_get(UA_Client* client, const UA_NodeId* certificateGroupId);

UA_EXPORT UA_StatusCode
UA_ClientConfig_PKIStore_removeContentAll(UA_PKIStore* pkiStore);

UA_EXPORT UA_StatusCode
UA_ClientConfig_PKIStore_storeTrustList(UA_PKIStore *pkiStore,
		                         size_t trustedCertificatesSize,
                                 UA_ByteString *trustedCertificates,
                                 size_t trustedCrlsSize,
                                 UA_ByteString *trustedCrls,
                                 size_t issuerCertificatesSize,
                                 UA_ByteString *issuerCertificates,
                                 size_t issuerCrlsSize,
                                 UA_ByteString *issuerCrls);
UA_EXPORT UA_StatusCode
UA_ClientConfig_PKIStore_storeRejectList(UA_PKIStore *pkiStore,
                                 const UA_ByteString *rejectedList,
                                 size_t rejectedListSize);

UA_EXPORT UA_StatusCode
UA_ClientConfig_PKIStore_appendRejectCertificate(UA_PKIStore *pkiStore,
                                 const UA_ByteString *certificate);

UA_EXPORT UA_StatusCode
UA_ClientConfig_PKIStore_loadRejectCertificates(UA_PKIStore *pkiStore,
		                         UA_ByteString **rejectedList,
		                         size_t *rejectedListSize);

UA_EXPORT UA_StatusCode
UA_ClientConfig_PKIStore_storeCertificate(UA_PKIStore *pkiStore,
		                         const UA_NodeId certType,
								 const UA_ByteString *cert);

UA_EXPORT UA_StatusCode
UA_ClientConfig_PKIStore_storePrivateKey(UA_PKIStore *pkiStore,
		                         const UA_NodeId certType,
								 const UA_ByteString *privateKey);

#ifdef UA_ENABLE_ENCRYPTION
UA_StatusCode UA_EXPORT
UA_ClientConfig_setDefaultEncryption(UA_ClientConfig *config);
#endif

_UA_END_DECLS

#endif /* UA_CLIENT_CONFIG_DEFAULT_H_ */
