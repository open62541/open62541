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
#include "open62541_queue.h"

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

/* Returns the CertificateGroup of the server with the NodeId, or NULL */
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

/* The server does not verify the certificates of its open SecureChannels again
 * when a TrustList or a certificate of a SecurityPolicy is replaced. The GDS
 * drivers close the affected SecureChannels themselves (Part 12, 7.10.9). The
 * SecureChannels of the server cannot be enumerated, so the drivers track them
 * from the SecureChannel notifications. */

typedef struct UA_GDSChannel {
    LIST_ENTRY(UA_GDSChannel) pointers;
    UA_UInt32 channelId;
    UA_NodeId certificateTypeId; /* null without a SecurityPolicy */
    UA_ByteString certificate; /* empty without a client certificate */
} UA_GDSChannel;

typedef LIST_HEAD(UA_GDSChannels, UA_GDSChannel) UA_GDSChannels;

/* Add the SecureChannel of a SECURECHANNEL_OPENED notification and remove the
 * one of a SECURECHANNEL_CLOSED notification. SecurityPolicy None channels
 * have nothing to close them for and are not added. */
void
UA_GDSChannels_notify(UA_GDSChannels *channels, UA_Server *server,
                      UA_ApplicationNotificationType type,
                      const UA_KeyValueMap payload);

/* A SecureChannel keeps its SecurityPolicy when the client renews it. The
 * renewal is checked against the new certificate and key of the
 * SecurityPolicy and fails. Close the SecureChannels of the CertificateType,
 * so that the clients reconnect with the new certificate. */
void
UA_GDSChannels_closeCertificateType(UA_GDSChannels *channels, UA_Server *server,
                                    const UA_NodeId *certificateTypeId);

/* Close the SecureChannels whose client certificate does not pass the
 * verification with the new TrustList of the CertificateGroup. The server does
 * not verify the certificate again when a SecureChannel is renewed. */
void
UA_GDSChannels_closeUntrusted(UA_GDSChannels *channels, UA_Server *server,
                              UA_CertificateGroup *certGroup);

void
UA_GDSChannels_clear(UA_GDSChannels *channels);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER || UA_ENABLE_DRIVER_GDS_PULL */

#endif /* UA_DRIVER_GDS_COMMON_H_ */
