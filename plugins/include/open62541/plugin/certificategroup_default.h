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
#include <open62541/util.h>

_UA_BEGIN_DECLS

/* Default implementation that accepts all certificates */
UA_EXPORT void
UA_CertificateGroup_AcceptAll(UA_CertificateGroup *certGroup);

#ifdef UA_ENABLE_ENCRYPTION
/*
 * Initialises and configures a certificate group with an in-memory backend.
 * The trustList parameter allows for the pre-configuration of the group with certificate files.
 *
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
                                const UA_TrustListDataType *trustList,
                                const UA_Logger *logger,
                                const UA_KeyValueMap *params);

#ifdef UA_ENABLE_CERTIFICATE_FILESTORE
/*
 * Initialises and configures a certificate group with a filestore backend.
 *
 * The path to the PKI (Public Key Infrastructure) folder can be specified.
 * If the folder (pki) does not exist at the specified location, it will be created automatically.
 * The folder structure follows the guidelines outlined in the OPC UA Part 12 F.1 specification.
 * If no path is provided, the default location used will be the current execution path.
 *
 * **Configuration parameters for the Certificate Filestore backend**
 *
 * 0:max-trust-listsize [uint32]
 *    The maximum trust list size in bytes. (default 64KiB).
 *
 * 0:max-rejected-listsize [uint32]
 *    The maximum number of certificate files that can be stored in the rejected list.
 *    (default: 100).
 *
 * **PKI folder structure**
 *
 * pki
 * ├── ApplCerts
 * │   ├── issuer
 * │   │   ├── certs
 * │   │   └── crl
 * │   ├── own
 * │   │   ├── certs
 * │   │   └── private
 * │   ├── rejected
 * │   │   └── certs
 * │   └── trusted
 * │       ├── certs
 * │       └── crl
 * ├── UserTokenCerts
 * │   ├── issuer
 * │   │   ├── certs
 * │   │   └── crl
 * │   ├── own
 * │   │   ├── certs
 * │   │   └── private
 * │   ├── rejected
 * │   │   └── certs
 * │   └── trusted
 * │       ├── certs
 * │       └── crl
 *
 */
UA_EXPORT UA_StatusCode
UA_CertificateGroup_Filestore(UA_CertificateGroup *certGroup,
                              UA_NodeId *certificateGroupId,
                              const UA_String storePath,
                              const UA_Logger *logger,
                              const UA_KeyValueMap *params);
#endif /* UA_ENABLE_CERTIFICATE_FILESTORE */

#endif /* UA_ENABLE_ENCRYPTION */

_UA_END_DECLS

#endif /* UA_CERTIFICATEGROUP_CERTIFICATE_H_ */
