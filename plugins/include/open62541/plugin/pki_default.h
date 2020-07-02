/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#ifndef UA_PKI_CERTIFICATE_H_
#define UA_PKI_CERTIFICATE_H_

#include <open62541/plugin/pki.h>

_UA_BEGIN_DECLS

/* Default implementation that accepts all certificates */
UA_EXPORT void
UA_CertificateVerification_AcceptAll(UA_CertificateVerification *cv);

#ifdef UA_ENABLE_ENCRYPTION

/* Accept certificates based on a trust-list and a revocation-list. Based on
 * mbedTLS. */
UA_EXPORT UA_StatusCode
UA_CertificateVerification_Trustlist(UA_CertificateVerification *cv,
                                     const UA_ByteString *certificateTrustList,
                                     size_t certificateTrustListSize,
                                     const UA_ByteString *certificateIssuerList,
                                     size_t certificateIssuerListSize,
                                     const UA_ByteString *certificateRevocationList,
                                     size_t certificateRevocationListSize);

#ifdef __linux__ /* Linux only so far */
UA_EXPORT UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateVerification *cv,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder);
#endif

#endif

_UA_END_DECLS

#endif /* UA_PKI_CERTIFICATE_H_ */
