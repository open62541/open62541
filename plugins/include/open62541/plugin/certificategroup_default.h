/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#ifndef UA_CERTIFICATEGROUP_CERTIFICATE_H_
#define UA_CERTIFICATEGROUP_CERTIFICATE_H_

#include <open62541/plugin/certificategroup.h>

_UA_BEGIN_DECLS

/* Default implementation that accepts all certificates */
UA_EXPORT void
UA_CertificateVerification_AcceptAll(UA_CertificateGroup *certGroup);

#ifdef UA_ENABLE_ENCRYPTION

/* Accept certificates based on a trust-list and a revocation-list. Based on
 * mbedTLS. */
UA_EXPORT UA_StatusCode
UA_CertificateVerification_Trustlist(UA_CertificateGroup *certGroup,
                                     const UA_ByteString *certificateTrustList,
                                     size_t certificateTrustListSize,
                                     const UA_ByteString *certificateIssuerList,
                                     size_t certificateIssuerListSize,
                                     const UA_ByteString *certificateRevocationList,
                                     size_t certificateRevocationListSize);

/*
 * **Configuration parameters for the Certificate Memorystore backend**
 *
 * 0:max-trust-listsize [uint32]
 *    The maximum trust list size in bytes. (default 64KiB).
 *
 * 0:max-rejected-listsize [uint32]
 *    The maximum number of certificate files that can be stored in the rejected list.
 *    (default: 100).
 */
UA_EXPORT UA_StatusCode
UA_CertificateGroup_Memorystore(UA_CertificateGroup *certGroup,
                                UA_NodeId *certificateGroupId,
                                const UA_TrustListDataType *trustList);

#ifdef __linux__ /* Linux only so far */

#ifdef UA_ENABLE_CERT_REJECTED_DIR
UA_EXPORT UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateGroup *certGroup,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder,
                                       const char *rejectedListFolder);
#else
UA_EXPORT UA_StatusCode
UA_CertificateVerification_CertFolders(UA_CertificateGroup *certGroup,
                                       const char *trustListFolder,
                                       const char *issuerListFolder,
                                       const char *revocationListFolder);
#endif

/*
 * **Configuration parameters for the Certificate Filestore backend**
 *
 * 0:max-trust-listsize [uint32]
 *    The maximum trust list size in bytes. (default 64KiB).
 *
 * 0:max-rejected-listsize [uint32]
 *    The maximum number of certificate files that can be stored in the rejected list.
 *    (default: 100).
 */
UA_EXPORT UA_StatusCode
UA_CertificateGroup_Filestore(UA_CertificateGroup *certGroup,
                              UA_NodeId *certificateGroupId,
                              const UA_String *storePath);
#endif

#endif

_UA_END_DECLS

#endif /* UA_CERTIFICATEGROUP_CERTIFICATE_H_ */
