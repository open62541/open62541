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

/* Default implementation that accepts all certificates
 * Any plugin implementation should invalidate an existing certificate verification
 * by first calling the internal clear() method if it is not NULL.
 * Refer to the default implementation in src/plugins/crypto/ua_pki_none.c */
UA_EXPORT void
UA_CertificateVerification_AcceptAll(UA_CertificateVerification *cv);

#ifdef UA_ENABLE_ENCRYPTION

/* Accept certificates based on a trust-list and a revocation-list. Based on mbedTLS.
 * Any plugin implementation should invalidate an existing certificate verification
 * by first calling the internal clear() method if it is not NULL.
 * Refer to the default implementation in src/plugins/crypto/mbedtls/ua_pki_mbedtls.c
 * or src/plugins/crypto/openssl/ua_pki_openssl.c */
UA_EXPORT UA_StatusCode
UA_CertificateVerification_Trustlist(UA_CertificateVerification *cv,
                                     const UA_ByteString *certificateTrustList,
                                     size_t certificateTrustListSize,
                                     const UA_ByteString *certificateIssuerList,
                                     size_t certificateIssuerListSize,
                                     const UA_ByteString *certificateRevocationList,
                                     size_t certificateRevocationListSize);

#ifdef __linux__ /* Linux only so far */

#ifdef UA_ENABLE_CERT_REJECTED_DIR
UA_EXPORT UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateVerification *cv,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder,
                                       const char *rejectedListFolder);
#else
UA_EXPORT UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateVerification *cv,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder);
#endif
#endif

#endif

_UA_END_DECLS

#endif /* UA_PKI_CERTIFICATE_H_ */
