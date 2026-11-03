/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 */

#ifndef UA_CLIENT_CONFIG_DEFAULT_H_
#define UA_CLIENT_CONFIG_DEFAULT_H_

#include <open62541/client.h>

_UA_BEGIN_DECLS

UA_StatusCode UA_EXPORT
UA_ClientConfig_setDefault(UA_ClientConfig *config);

#ifdef UA_ENABLE_ENCRYPTION
/* Replaces the configured encryption policies. Previous policy arrays are
 * retained until the configuration is cleared, preserving references held by
 * open SecureChannels and Sessions, including across repeated calls. */
UA_StatusCode UA_EXPORT
UA_ClientConfig_setDefaultEncryption(UA_ClientConfig *config,
                                     UA_ByteString localCertificate,
                                     UA_ByteString privateKey,
                                     const UA_ByteString *trustList,
                                     size_t trustListSize,
                                     const UA_ByteString *revocationList,
                                     size_t revocationListSize);
#endif

#if defined(UA_ENABLE_ENCRYPTION_OPENSSL)
/* The ECC AEAD SecurityPolicies of OPC UA Part 7. Each function adds the policy
 * with the given application certificate (which must be on the curve of the
 * policy, Ed25519/Ed448 for curve25519/curve448) to the SecureChannel policies
 * of the client. Most of them are not part of the default policy set of
 * UA_ClientConfig_setDefaultEncryption, which replaces the policies: call them
 * afterwards. UA_ClientConfig_setAuthenticationCert adds an instance with the
 * user certificate for every ECC AEAD policy configured at that point. Open
 * SecureChannels and Sessions keep working with the previous policy array. */
UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccNistP256AesGcm(UA_ClientConfig *config,
                                                   const UA_ByteString *certificate,
                                                   const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccNistP256ChaChaPoly(UA_ClientConfig *config,
                                                       const UA_ByteString *certificate,
                                                       const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccNistP384AesGcm(UA_ClientConfig *config,
                                                   const UA_ByteString *certificate,
                                                   const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccNistP384ChaChaPoly(UA_ClientConfig *config,
                                                       const UA_ByteString *certificate,
                                                       const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccBrainpoolP256r1AesGcm(UA_ClientConfig *config,
                                                          const UA_ByteString *certificate,
                                                          const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccBrainpoolP256r1ChaChaPoly(UA_ClientConfig *config,
                                                              const UA_ByteString *certificate,
                                                              const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccBrainpoolP384r1AesGcm(UA_ClientConfig *config,
                                                          const UA_ByteString *certificate,
                                                          const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccBrainpoolP384r1ChaChaPoly(UA_ClientConfig *config,
                                                              const UA_ByteString *certificate,
                                                              const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccCurve25519AesGcm(UA_ClientConfig *config,
                                                     const UA_ByteString *certificate,
                                                     const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccCurve25519ChaChaPoly(UA_ClientConfig *config,
                                                         const UA_ByteString *certificate,
                                                         const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccCurve448AesGcm(UA_ClientConfig *config,
                                                   const UA_ByteString *certificate,
                                                   const UA_ByteString *privateKey);

UA_StatusCode UA_EXPORT
UA_ClientConfig_addSecurityPolicyEccCurve448ChaChaPoly(UA_ClientConfig *config,
                                                       const UA_ByteString *certificate,
                                                       const UA_ByteString *privateKey);
#endif

_UA_END_DECLS

#endif /* UA_CLIENT_CONFIG_DEFAULT_H_ */
