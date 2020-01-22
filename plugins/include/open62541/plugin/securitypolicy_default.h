/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2017-2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Daniel Feist, Precitec GmbH & Co. KG
 */

#ifndef UA_SECURITYPOLICIES_H_
#define UA_SECURITYPOLICIES_H_

#include <open62541/plugin/securitypolicy.h>

_UA_BEGIN_DECLS

UA_EXPORT UA_StatusCode
UA_SecurityPolicy_None(UA_SecurityPolicy *policy,
                       UA_CertificateVerification *certificateVerification,
                       const UA_ByteString localCertificate, const UA_Logger *logger);

#ifdef UA_ENABLE_ENCRYPTION

UA_EXPORT UA_StatusCode
UA_SecurityPolicy_Basic128Rsa15(UA_SecurityPolicy *policy,
                                UA_CertificateVerification *certificateVerification,
                                const UA_ByteString localCertificate,
                                const UA_ByteString localPrivateKey,
                                const UA_Logger *logger);

UA_EXPORT UA_StatusCode
UA_SecurityPolicy_Basic256(UA_SecurityPolicy *policy,
                           UA_CertificateVerification *certificateVerification,
                           const UA_ByteString localCertificate,
                           const UA_ByteString localPrivateKey, const UA_Logger *logger);

UA_EXPORT UA_StatusCode
UA_SecurityPolicy_Basic256Sha256(UA_SecurityPolicy *policy,
                                 UA_CertificateVerification *certificateVerification,
                                 const UA_ByteString localCertificate,
                                 const UA_ByteString localPrivateKey,
                                 const UA_Logger *logger);

#endif

_UA_END_DECLS

#endif /* UA_SECURITYPOLICIES_H_ */
