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

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER

_UA_BEGIN_DECLS

typedef struct UA_GDSPushReceiverContext UA_GDSPushReceiverContext;

UA_StatusCode
initNS0PushManagement(UA_GDSPushReceiverContext *ctx);

void
clearNS0PushManagement(UA_GDSPushReceiverContext *ctx);

#ifdef UA_ENABLE_RBAC
/* Restrict the GDS PushManagement methods to the SecurityAdmin role
 * (OPC UA Part 12 v1.05 §7.2, §7.10.4) */
UA_StatusCode
initGDSRolePermissions(UA_Server *server);
#endif

UA_CertificateGroup *
getCertGroup(UA_Server *server, const UA_NodeId *objectId);

UA_StatusCode
writeOpenCountVariable(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *group);

UA_StatusCode
writeLastUpdateVariable(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *group);

UA_StatusCode
UA_GDSPushReceiver_initFileInfos(UA_GDSPushReceiverContext *ctx, UA_UtcTime lastUpdateTime);

UA_StatusCode
UA_GDSPushReceiver_getFileInfoMetadata(UA_GDSPushReceiverContext *ctx,
                                  const UA_NodeId certificateGroupId,
                                  UA_UInt16 *openCount,
                                  UA_UtcTime *lastUpdateTime);

UA_Boolean
UA_GDSPushReceiver_transactionPending(UA_GDSPushReceiverContext *ctx);

UA_StatusCode
UA_GDSPushReceiver_applyChangesForSession(UA_GDSPushReceiverContext *ctx,
                                     const UA_NodeId *sessionId);

UA_StatusCode
UA_GDSPushReceiver_openTrustList(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *certGroup,
                            const UA_NodeId *sessionId, UA_Byte fileOpenMode,
                            UA_Variant *output);
UA_StatusCode
UA_GDSPushReceiver_getPositionTrustList(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *certGroup,
                                   const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                                   UA_Variant *output);
UA_StatusCode
UA_GDSPushReceiver_closeTrustList(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *certGroup,
                             const UA_NodeId *sessionId, UA_UInt32 fileHandle);
UA_StatusCode
UA_GDSPushReceiver_closeAndUpdateTrustList(UA_GDSPushReceiverContext *ctx,
                                      UA_CertificateGroup *certGroup,
                                      const UA_NodeId *sessionId,
                                      UA_UInt32 fileHandle, UA_Variant *output);
UA_StatusCode
UA_GDSPushReceiver_setPositionTrustList(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *certGroup,
                                   const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                                   UA_UInt64 position);
UA_StatusCode
UA_GDSPushReceiver_writeTrustList(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *certGroup,
                             const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                             const UA_ByteString data);
UA_StatusCode
UA_GDSPushReceiver_openTrustListWithMask(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *certGroup,
                                    const UA_NodeId *sessionId, UA_UInt32 mask,
                                    UA_Variant *output);
UA_StatusCode
UA_GDSPushReceiver_getRejectedList(UA_GDSPushReceiverContext *ctx, size_t outputSize,
                              UA_Variant *output);
UA_StatusCode
UA_GDSPushReceiver_readTrustList(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *certGroup,
                            const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                            UA_Int32 length, UA_Variant *output);
UA_StatusCode
UA_GDSPushReceiver_stageCertificateUpdate(UA_GDSPushReceiverContext *ctx, const UA_NodeId *sessionId,
                                const UA_NodeId *certificateGroupId,
                                const UA_NodeId *certificateTypeId,
                                const UA_ByteString *certificate,
                                const UA_String *privateKeyFormat,
                                const UA_ByteString *privateKey);
UA_StatusCode
UA_GDSPushReceiver_addCertificate(UA_GDSPushReceiverContext *ctx, UA_CertificateGroup *certGroup,
                             UA_ByteString *certificate,
                             const UA_Boolean *isTrustedCertificate);
UA_StatusCode
UA_GDSPushReceiver_removeCertificate(UA_GDSPushReceiverContext *ctx,
                                UA_CertificateGroup *certGroup,
                                const UA_NodeId *sessionId,
                                const UA_String *thumbprint,
                                const UA_Boolean *isTrustedCertificate);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER */

#endif /* UA_DRIVER_GDS_RECEIVER_INTERNAL_H_ */
