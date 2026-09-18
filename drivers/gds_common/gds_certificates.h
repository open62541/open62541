/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_DRIVER_GDS_CERTIFICATES_H_
#define UA_DRIVER_GDS_CERTIFICATES_H_

#include <open62541/server.h>

#if defined(UA_ENABLE_DRIVER_GDS_RECEIVER) || defined(UA_ENABLE_DRIVER_GDS_PULL)

_UA_BEGIN_DECLS

UA_CertificateGroup *
UA_GDS_getCertificateGroup(UA_ServerConfig *sc,
                           const UA_NodeId *certificateGroupId);

/* Applying a new ApplicationInstanceCertificate is shared between the GDS
 * drivers. The certificate is not stored in the CertificateGroup. It is held
 * by every SecurityPolicy with a matching CertificateType and mirrored into
 * the EndpointDescriptions that use those policies. */

UA_SecurityPolicy *
UA_GDS_getSecPolicyByUri(UA_ServerConfig *sc, const UA_String *securityPolicyUri);

/* Update every SecurityPolicy and endpoint that uses the certificate type.
 * First resolve all endpoint policies and allocate the replacement endpoint
 * certificates. This ensures configuration and allocation errors are reported
 * before any live policy is changed. A SecurityPolicy referenced by multiple
 * endpoints is updated only once. After all policy updates succeed, commit the
 * preallocated endpoint certificates without further fallible operations.
 *
 * SecurityPolicy implementations update their private state directly and do
 * not expose the previous private key for rollback. An unexpected failure from
 * a policy after an earlier policy succeeded is therefore logged explicitly. */
UA_StatusCode
UA_GDS_applyCertificateToPolicies(UA_ServerConfig *sc,
                                  const UA_NodeId *certificateTypeId,
                                  const UA_ByteString certificate,
                                  const UA_ByteString privateKey);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER || UA_ENABLE_DRIVER_GDS_PULL */

#endif /* UA_DRIVER_GDS_CERTIFICATES_H_ */
