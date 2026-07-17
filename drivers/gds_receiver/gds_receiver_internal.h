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

typedef struct UA_GDSManager UA_GDSManager;

UA_GDSManager *
gdsManager(UA_Server *server);

UA_StatusCode
initNS0PushManagement(UA_Server *server);

UA_CertificateGroup *
getCertGroup(UA_Server *server, const UA_NodeId *objectId);

UA_StatusCode
writeOpenCountVariable(UA_Server *server, UA_CertificateGroup *group);

UA_StatusCode
writeLastUpdateVariable(UA_Server *server, UA_CertificateGroup *group);

UA_StatusCode
UA_GDSManager_initFileInfos(UA_GDSManager *gdsm, UA_UtcTime lastUpdateTime);

UA_StatusCode
UA_GDSManager_getFileInfoMetadata(UA_GDSManager *gdsm,
                                  const UA_NodeId certificateGroupId,
                                  UA_UInt16 *openCount,
                                  UA_UtcTime *lastUpdateTime);

UA_Boolean
UA_GDSManager_transactionPending(UA_GDSManager *gdsm);

UA_StatusCode
UA_GDSManager_applyChangesForSession(UA_GDSManager *gdsm,
                                     const UA_NodeId *sessionId);

UA_StatusCode
UA_GDSManager_openTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                            const UA_NodeId *sessionId, UA_Byte fileOpenMode,
                            UA_Variant *output);
UA_StatusCode
UA_GDSManager_getPositionTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                                   const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                                   UA_Variant *output);
UA_StatusCode
UA_GDSManager_closeTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                             const UA_NodeId *sessionId, UA_UInt32 fileHandle);
UA_StatusCode
UA_GDSManager_closeAndUpdateTrustList(UA_GDSManager *gdsm,
                                      UA_CertificateGroup *certGroup,
                                      const UA_NodeId *sessionId,
                                      UA_UInt32 fileHandle, UA_Variant *output);
UA_StatusCode
UA_GDSManager_setPositionTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                                   const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                                   UA_UInt64 position);
UA_StatusCode
UA_GDSManager_writeTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                             const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                             const UA_ByteString data);
UA_StatusCode
UA_GDSManager_openTrustListWithMask(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                                    const UA_NodeId *sessionId, UA_UInt32 mask,
                                    UA_Variant *output);
UA_StatusCode
UA_GDSManager_getRejectedList(UA_GDSManager *gdsm, size_t outputSize,
                              UA_Variant *output);
UA_StatusCode
UA_GDSManager_readTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                            const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                            UA_Int32 length, UA_Variant *output);
UA_StatusCode
UA_GDSManager_updateCertificate(UA_GDSManager *gdsm, const UA_NodeId *sessionId,
                                const UA_NodeId *certificateGroupId,
                                const UA_NodeId *certificateTypeId,
                                const UA_ByteString *certificate,
                                const UA_String *privateKeyFormat,
                                const UA_ByteString *privateKey);
UA_StatusCode
UA_GDSManager_addCertificate(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                             UA_ByteString *certificate,
                             const UA_Boolean *isTrustedCertificate);
UA_StatusCode
UA_GDSManager_removeCertificate(UA_GDSManager *gdsm,
                                UA_CertificateGroup *certGroup,
                                const UA_NodeId *sessionId,
                                const UA_String *thumbprint,
                                const UA_Boolean *isTrustedCertificate);

_UA_END_DECLS

#endif /* UA_ENABLE_DRIVER_GDS_RECEIVER */

#endif /* UA_DRIVER_GDS_RECEIVER_INTERNAL_H_ */
