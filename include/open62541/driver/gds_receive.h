/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_DRIVER_GDS_RECEIVE_H_
#define UA_DRIVER_GDS_RECEIVE_H_

#include <open62541/server.h>

/**
 * GDS Push Receive Driver
 * -----------------------
 *
 * The GDS Push Receive driver implements the server side of OPC UA
 * PushManagement. It exposes the ServerConfiguration methods for updating the
 * application certificate and the CertificateGroup trust lists. Changes are
 * collected in a transaction and applied together with the ApplyChanges
 * method. The driver also tracks active sessions and SecureChannels so that
 * abandoned transactions can be discarded and affected channels can be
 * revalidated or closed after an update.
 *
 * Build open62541 with ``UA_ENABLE_DRIVER_GDS_RECEIVE`` enabled.
 * PushManagement additionally requires encryption and the full Namespace
 * Zero.
 *
 * Create the driver with UA_GDSPushReceiveManager_new() and attach it to the
 * server with UA_Server_addDriver() before calling UA_Server_run_startup(). A
 * server can have at most one GDS Push Receive driver. The server takes
 * ownership of the driver after it has been added successfully. */

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVE

_UA_BEGIN_DECLS

/* Create a GDS Push Receive driver. The returned driver is heap-allocated and
 * must either be passed to UA_Server_addDriver() or released with its ``free``
 * callback. Returns NULL if allocation fails. */
UA_EXPORT UA_Driver *
UA_GDSPushReceiveManager_new(void);

/* Update the application certificate used by the server endpoints. The GDS
 * Push Receive driver must be attached and started. If certificateGroupId is
 * null, the DefaultApplicationGroup is used. */
UA_EXPORT UA_StatusCode
UA_GDSReceive_updateCertificate(UA_Server *server,
                                const UA_NodeId certificateGroupId,
                                const UA_NodeId certificateTypeId,
                                const UA_ByteString certificate,
                                const UA_ByteString *privateKey);

/* Create a PKCS #10 DER-encoded certificate signing request. The GDS Push
 * Receive driver must be attached and started. If certificateGroupId is null,
 * the DefaultApplicationGroup is used. */
UA_EXPORT UA_StatusCode
UA_GDSReceive_createSigningRequest(UA_Server *server,
                                   const UA_NodeId certificateGroupId,
                                   const UA_NodeId certificateTypeId,
                                   const UA_String *subjectName,
                                   const UA_Boolean *regenerateKey,
                                   const UA_ByteString *nonce,
                                   UA_ByteString *csr);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVE */

#endif /* UA_DRIVER_GDS_RECEIVE_H_ */
