/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_CERTIFICATE_MANAGER_H
#define OPEN62541_UA_CERTIFICATE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_util_internal.h"
#include "ua_server.h"
#include "ua_plugin_ca.h"
#include "open62541_queue.h"

#ifdef UA_ENABLE_GDS_CM

typedef struct gds_cm_entry {
    LIST_ENTRY(gds_cm_entry) pointers;
    UA_NodeId requestId;
    UA_NodeId applicationId;
    UA_Boolean isApproved;
    UA_ByteString certificate;
    UA_ByteString privateKey;
    size_t issuerCertificateSize;
    UA_ByteString *issuerCertificates;
} gds_cm_entry;

typedef struct gds_cm_tl_entry {
    LIST_ENTRY(gds_cm_tl_entry) pointers;
    UA_TrustListDataType trustList;
    UA_NodeId sessionId;
    UA_GDS_CertificateGroup *cg;
    UA_UInt32 fileHandle;
    UA_Boolean isOpen;
} gds_cm_tl_entry;

typedef struct{
    LIST_HEAD(gds_cm__list, gds_cm_entry) gds_cm_list;
    size_t counter;
    LIST_HEAD(gds_cm__tl, gds_cm_tl_entry) gds_cm_trustList;
} UA_GDS_CertificateManager;

UA_StatusCode
UA_GDS_CertificateManager_init(UA_Server *server);

UA_StatusCode
UA_GDS_FinishRequest(UA_Server *server,
                  UA_NodeId *applicationId,
                  UA_NodeId *requestId,
                  UA_ByteString *certificate,
                  UA_ByteString *privKey,
                  size_t *length,
                  UA_ByteString **issuerCertificate);

UA_StatusCode
UA_GDS_GetTrustList(UA_Server *server,
                 const UA_NodeId *sessionId,
                 UA_NodeId *applicationId,
                 UA_NodeId *certificateGroupId,
                 UA_NodeId *trustListId);

UA_StatusCode
UA_GDS_StartNewKeyPairRequest(UA_Server *server,
                           UA_NodeId *applicationId,
                           UA_NodeId *certificateGroupId,
                           UA_NodeId *certificateTypeId,
                           UA_String *subjectName,
                           size_t  domainNameSize,
                           UA_String *domainNames,
                           UA_String *privateKeyFormat,
                           UA_String *privateKeyPassword,
                           UA_NodeId *requestId);

UA_StatusCode
UA_GDS_StartSigningRequest(UA_Server *server,
                        UA_NodeId *applicationId,
                        UA_NodeId *certificateGroupId,
                        UA_NodeId *certificateTypeId,
                        UA_ByteString *certificateRequest,
                        UA_NodeId *requestId);

UA_StatusCode
UA_GDS_GetCertificateGroups(UA_Server *server,
                         UA_NodeId *applicationId,
                         size_t *outputSize,
                         UA_NodeId **certificateGroupIds);

UA_StatusCode
UA_GDS_OpenTrustList(UA_Server *server,
                  const UA_NodeId *sessionId,
                  const UA_NodeId *objectId,
                  UA_Byte  *mode,
                  UA_UInt32 *fileHandle);

UA_StatusCode
UA_GDS_ReadTrustList(UA_Server *server,
                  const UA_NodeId *sessionId,
                  const UA_NodeId *trustListNodeId,
                  UA_UInt32 *fileHandle,
                  UA_Int32 *length,
                  UA_TrustListDataType *trustList);

UA_StatusCode
UA_GDS_CloseTrustList(UA_Server *server,
                   const UA_NodeId *sessionId,
                   const UA_NodeId *objectId,
                   UA_UInt32 *fileHandle);

UA_StatusCode
UA_GDS_CertificateManager_close(UA_Server *server);

#endif /* UA_ENABLE_GDS_CM */

#ifdef __cplusplus
}
#endif

#endif //OPEN62541_UA_CERTIFICATE_MANAGER_H
