/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_DRIVER_GDS_RECEIVER_INTERNAL_H_
#define UA_DRIVER_GDS_RECEIVER_INTERNAL_H_

#include <open62541/driver/gds_receiver.h>
#include <open62541/plugin/certificategroup.h>
#include "open62541_queue.h"

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER

_UA_BEGIN_DECLS

typedef struct UA_GDSReceiverContext UA_GDSReceiverContext;

typedef enum {
    UA_GDSTRANSACTIONSTATE_FRESH,
    UA_GDSTRANSACTIONSTATE_PENDING,
} UA_GDSTransactionState;

typedef struct {
    UA_ByteString certificate;
    UA_ByteString privateKey;
    UA_NodeId certificateGroup;
    UA_NodeId certificateType;
} UA_GDSCertificateInfo;

typedef struct {
    UA_Server *server;
    UA_NodeId sessionId;
    UA_GDSTransactionState state;
    UA_ByteString localCsrCertificate;
    size_t certGroupSize;
    UA_CertificateGroup *certGroups;
    size_t certificateInfosSize;
    UA_GDSCertificateInfo *certificateInfos;
    UA_DelayedCallback dc;
    UA_Boolean applyChangesQueued;
} UA_GDSTransaction;

typedef struct ChannelMetadata {
    LIST_ENTRY(ChannelMetadata) pointers;
    UA_UInt32 channelId;
    UA_NodeId certificateTypeId;
    UA_ByteString certificate;
} ChannelMetadata;

/* An open TrustList: the encoded snapshot of a read or the staged bytes of a
 * write. The file transfer driver owns the FileHandle of the client. */
typedef struct UA_GDSTrustListHandle {
    LIST_ENTRY(UA_GDSTrustListHandle) entry;
    UA_UInt32 id; /* Backend handle, never 0 */
    UA_Byte mode; /* Read, or Write + EraseExisting */
    UA_ByteString data;
    size_t position;
} UA_GDSTrustListHandle;

/* A TrustList Object of a CertificateGroup, served by a file transfer driver
 * while the receiver is started */
typedef struct {
    UA_GDSReceiverContext *receiver;
    UA_NodeId objectId;
    UA_NodeId certificateGroupId;
    UA_UtcTime lastUpdateTime;
    UA_Driver *driver;
    UA_UInt32 nextHandle;
    size_t handlesSize;  /* OpenCount */
    LIST_HEAD(, UA_GDSTrustListHandle) handles;
} UA_GDSTrustList;

#define UA_GDSRECEIVER_TRUSTLISTS 2

struct UA_GDSReceiverContext {
    UA_Driver drv;
    UA_Boolean initialized;
    UA_GDSTransaction transaction;
    UA_GDSTrustList trustLists[UA_GDSRECEIVER_TRUSTLISTS];
    LIST_HEAD(, ChannelMetadata) secureChannels;
    UA_UInt64 checkSessionCallbackId;
    size_t pendingDelayedCallbacks;
};

UA_StatusCode
UA_GDSTransaction_init(UA_GDSTransaction *transaction, UA_Server *server,
                       const UA_NodeId sessionId);

void
UA_GDSTransaction_clear(UA_GDSTransaction *transaction);

/* Discard the transaction once its Session is gone */
UA_StatusCode
UA_GDSReceiver_watchTransaction(UA_GDSReceiverContext *ctx);

void
UA_GDSReceiver_initTrustLists(UA_GDSReceiverContext *ctx);

/* The TrustList of a TrustList Object or CertificateGroup */
UA_GDSTrustList *
UA_GDSReceiver_getTrustList(UA_GDSReceiverContext *ctx, const UA_NodeId *id);

UA_GDSTrustListHandle *
UA_GDSTrustList_findHandle(UA_GDSTrustList *tl, UA_UInt32 id);

/* Is any TrustList open for writing */
UA_Boolean
UA_GDSReceiver_trustListOpenForWrite(UA_GDSReceiverContext *ctx);

/* Register and start the file transfer drivers of the TrustLists */
UA_StatusCode
UA_GDSReceiver_startTrustLists(UA_GDSReceiverContext *ctx);

/* Stop, remove and free the file transfer drivers again */
void
UA_GDSReceiver_stopTrustLists(UA_GDSReceiverContext *ctx);

UA_StatusCode
initNS0PushManagement(UA_GDSReceiverContext *ctx);

void
clearNS0PushManagement(UA_GDSReceiverContext *ctx);

#ifdef UA_ENABLE_RBAC
/* Restrict the GDS PushManagement methods to the SecurityAdmin role
 * (OPC UA Part 12 v1.05 §7.2, §7.10.4) */
UA_StatusCode
initGDSRolePermissions(UA_Server *server);
#endif

UA_CertificateGroup *
getCertGroup(UA_Server *server, const UA_NodeId *objectId);

/* Can certificates of the type be updated in the DefaultApplicationGroup */
UA_Boolean
UA_GDSReceiver_certificateTypeSupported(UA_ServerConfig *sc,
                                        const UA_NodeId *certificateTypeId);

UA_StatusCode
writeLastUpdateVariable(UA_GDSReceiverContext *ctx, UA_GDSTrustList *tl);

UA_Boolean
UA_GDSReceiver_transactionPending(UA_GDSReceiverContext *ctx);

UA_StatusCode
UA_GDSReceiver_applyChangesForSession(UA_GDSReceiverContext *ctx,
                                     const UA_NodeId *sessionId);

UA_StatusCode
UA_GDSReceiver_closeAndUpdateTrustList(UA_GDSReceiverContext *ctx,
                                      UA_GDSTrustList *tl,
                                      const UA_NodeId *sessionId,
                                      UA_UInt32 fileHandle, UA_Variant *output);
UA_StatusCode
UA_GDSReceiver_openTrustListWithMask(UA_GDSReceiverContext *ctx, UA_GDSTrustList *tl,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    UA_UInt32 mask, UA_Variant *output);
UA_StatusCode
UA_GDSReceiver_getRejectedList(UA_GDSReceiverContext *ctx, size_t outputSize,
                              UA_Variant *output);
UA_StatusCode
UA_GDSReceiver_stageCertificateUpdate(UA_GDSReceiverContext *ctx, const UA_NodeId *sessionId,
                                const UA_NodeId *certificateGroupId,
                                const UA_NodeId *certificateTypeId,
                                const UA_ByteString *certificate,
                                const UA_String *privateKeyFormat,
                                const UA_ByteString *privateKey);
UA_StatusCode
UA_GDSReceiver_addCertificate(UA_GDSReceiverContext *ctx, UA_CertificateGroup *certGroup,
                             UA_ByteString *certificate,
                             const UA_Boolean *isTrustedCertificate);
UA_StatusCode
UA_GDSReceiver_removeCertificate(UA_GDSReceiverContext *ctx,
                                UA_CertificateGroup *certGroup,
                                const UA_NodeId *sessionId,
                                const UA_String *thumbprint,
                                const UA_Boolean *isTrustedCertificate);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER */

#endif /* UA_DRIVER_GDS_RECEIVER_INTERNAL_H_ */
