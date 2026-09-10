/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Bastian Engel)
 */

#ifndef UA_DRIVER_GDS_COMMON_H_
#define UA_DRIVER_GDS_COMMON_H_

#include <open62541/server.h>

#if defined(UA_ENABLE_DRIVER_GDS_RECEIVER) || defined(UA_ENABLE_DRIVER_GDS_PULL)

_UA_BEGIN_DECLS

/* Helpers of the GDS drivers. They duplicate functionality that the core has
 * but does not expose. */

/* Returns the leaf certificate of a ByteString that contains a chain of
 * concatenated DER or PEM certificates. The leaf is the first certificate. The
 * result points into the chain and is not allocated. A ByteString in another
 * format is returned as is, so that the crypto backend reports the parse
 * error. A null ByteString is returned if the leaf certificate is truncated.
 *
 * The DER detection is the same as getLeafCertificate of the core. */
UA_ByteString
UA_GDSReceiver_getLeafCertificate(UA_ByteString chain);

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
 * Returns Bad_InvalidArgument if no endpoint uses the certificate type.
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

#endif /* UA_DRIVER_GDS_COMMON_H_ */
