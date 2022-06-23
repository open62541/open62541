/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 */

#ifndef UA_PKI_CERTIFICATE_H_
#define UA_PKI_CERTIFICATE_H_

#include <open62541/plugin/certificate_manager.h>
#include <open62541/plugin/certstore.h>

_UA_BEGIN_DECLS

/* Default implementation that accepts all certificates */
UA_EXPORT void
UA_CertificateManager_AcceptAll(UA_CertificateManager *cv);

#ifdef UA_ENABLE_ENCRYPTION

/* Accept certificates based on a trust-list and a revocation-list. Based on
 * mbedTLS. */
UA_EXPORT UA_StatusCode
UA_CertificateManager_Trustlist(UA_CertificateManager *certificateManager);

#ifdef __linux__ /* Linux only so far */
UA_EXPORT UA_StatusCode
UA_CertificateManager_CertFolders(UA_CertificateManager *certificateManager,
                                  const char *trustListFolder,
                                  const char *issuerListFolder,
                                  const char *revocationListFolder);
#endif

#endif

_UA_END_DECLS

#endif /* UA_PKI_CERTIFICATE_H_ */
