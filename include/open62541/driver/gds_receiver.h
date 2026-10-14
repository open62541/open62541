/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_DRIVER_GDS_RECEIVER_H_
#define UA_DRIVER_GDS_RECEIVER_H_

#include <open62541/server.h>

/**
 * GDS Receiver Driver
 * -------------------
 *
 * The GDS Receiver driver implements the server side of OPC UA
 * PushManagement. It exposes the ServerConfiguration methods for updating the
 * application certificate and the CertificateGroup trust lists. Changes are
 * collected in a transaction and applied together with the ApplyChanges
 * method. The driver also tracks active sessions and SecureChannels so that
 * abandoned transactions can be discarded and affected channels can be
 * revalidated or closed after an update.
 *
 * Build open62541 with ``UA_ENABLE_DRIVER_GDS_RECEIVER`` enabled.
 * PushManagement additionally requires encryption, the Method service set
 * and the full Namespace Zero.
 *
 * Two file transfer drivers, registered while the receiver is started, serve
 * the TrustList Objects of the DefaultApplicationGroup and the
 * DefaultUserTokenGroup with the FileType Methods and Properties. Like every
 * file transfer driver, they also serve the FileType Methods of Namespace
 * Zero. A file transfer driver of the application on a TrustList Object makes
 * the start fail with Bad_NodeIdExists. A TrustList opened for reading returns
 * the encoded TrustListDataType at the time of the Open. A TrustList opened
 * for writing is applied with CloseAndUpdate and discarded with Close
 * (Part 12, 7.8.2). A MaxTrustListSize above 1 MiB raises the
 * MaxByteStringLength of the TrustLists, so that a TrustList fits into one
 * Write.
 *
 * Set ``drv.params`` before the start; all keys use namespace 0.
 *
 * 0:activity-timeout [Double]
 *    Close a TrustList without a Method call for longer than this many
 *    milliseconds and discard its changes (default: 60000, Part 12, 7.8.2.1).
 *    Published as the ActivityTimeout Property of the TrustLists. 0 disables
 *    it and removes the Property the receiver added; otherwise at least
 *    0.0001. Another type fails the start with Bad_InvalidArgument.
 *
 * Create the driver with UA_GDSReceiver_new() and attach its ``drv`` member to
 * the server with UA_Server_addDriver() before calling
 * UA_Server_run_startup(). A server can have at most one GDS Receiver driver.
 * The server takes ownership of the receiver after it has been added
 * successfully. */

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER

_UA_BEGIN_DECLS

typedef struct UA_GDSReceiver {
    UA_Driver drv; /* Must be the first member */
} UA_GDSReceiver;

/* Create a GDS Receiver driver. The returned driver is heap-allocated and
 * must either be passed to UA_Server_addDriver() or released with its ``free``
 * callback. Returns NULL if allocation fails. */
UA_EXPORT UA_GDSReceiver *
UA_GDSReceiver_new(void);

/* Update the application certificate used by the server endpoints. The GDS
 * Receiver driver must be attached and started. If certificateGroupId is
 * null, the DefaultApplicationGroup is used. Returns BadInvalidArgument for a
 * null receiver or empty certificate and BadInvalidState if the receiver is
 * not attached and started. This function is thread-safe. */
UA_EXPORT UA_StatusCode
UA_GDSReceiver_updateCertificate(UA_GDSReceiver *receiver,
                                const UA_NodeId certificateGroupId,
                                const UA_NodeId certificateTypeId,
                                const UA_ByteString certificate,
                                const UA_ByteString *privateKey);

/* Create a PKCS #10 DER-encoded certificate signing request. The GDS Receiver
 * driver must be attached and started. If certificateGroupId is null,
 * the DefaultApplicationGroup is used. Returns BadInvalidArgument for a null
 * receiver or output pointer and BadInvalidState if the receiver is not
 * attached and started. This function is thread-safe. */
UA_EXPORT UA_StatusCode
UA_GDSReceiver_createSigningRequest(UA_GDSReceiver *receiver,
                                   const UA_NodeId certificateGroupId,
                                   const UA_NodeId certificateTypeId,
                                   const UA_String *subjectName,
                                   const UA_Boolean *regenerateKey,
                                   const UA_ByteString *nonce,
                                   UA_ByteString *csr);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER */

#endif /* UA_DRIVER_GDS_RECEIVER_H_ */
