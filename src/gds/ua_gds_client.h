/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */

#ifndef OPEN62541_UA_GDS_CLIENT_H
#define OPEN62541_UA_GDS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ua_types.h"
#include "ua_record_datatype.h"
#include "ua_types_generated.h"
#include "ua_client.h"

#ifdef UA_ENABLE_GDS_CLIENT /* conditional compilation */

UA_StatusCode
UA_GDS_call_registerApplication(UA_Client *client,
                                UA_ApplicationRecordDataType *record,
                                UA_NodeId *newNodeId);

UA_StatusCode
UA_GDS_call_unregisterApplication(UA_Client *client,
                                  UA_NodeId *newNodeId);

UA_StatusCode
UA_GDS_call_findApplication(UA_Client *client,
                            UA_String uri,
                            size_t *length,
                            UA_ApplicationRecordDataType *records);

UA_StatusCode
UA_GDS_call_startSigningRequest(UA_Client *client,
                                UA_NodeId *applicationId,
                                const UA_NodeId *certificateGroupId,
                                const UA_NodeId *certificateTypeId,
                                UA_ByteString *csr,
                                UA_NodeId *requestId);

UA_StatusCode
UA_GDS_call_startNewKeyPairRequest(UA_Client *client,
                                          UA_NodeId *applicationId,
                                          const UA_NodeId *certificateGroupId,
                                          const UA_NodeId *certificateTypeId,
                                          UA_String *subjectName,
                                          size_t domainNameLength,
                                          UA_String *domainNames,
                                          const UA_String *privateKeyFormat,
                                          const UA_String *privateKeyPassword,
                                          UA_NodeId *requestId);

UA_StatusCode
UA_GDS_call_finishRequest(UA_Client *client,
                          UA_NodeId *applicationId,
                          UA_NodeId *requestId,
                          UA_ByteString *certificate,
                          UA_ByteString *privateKey,
                          UA_ByteString *issuerCertificate);

UA_StatusCode
UA_GDS_call_getCertificateGroups(UA_Client *client,
                                 UA_NodeId *applicationId,
                                 size_t *cg_size,
                                 UA_NodeId **certificateGroups);

UA_StatusCode
UA_GDS_call_getTrustList(UA_Client *client,
                         UA_NodeId *applicationId,
                         const UA_NodeId *certificateGroupId,
                         UA_NodeId *trustListId);


UA_StatusCode
UA_GDS_call_openTrustList(UA_Client *client,
                          UA_Byte *mode,
                          UA_UInt32 *fileHandle);

UA_StatusCode
UA_GDS_call_closeTrustList(UA_Client *client,
                           UA_UInt32 *fileHandle);

UA_StatusCode
UA_GDS_call_readTrustList(UA_Client *client,
                          UA_UInt32 *fileHandle,
                          UA_Int32 *length,
                          UA_TrustListDataType *trustList);

UA_StatusCode
UA_GDS_Client_init(UA_Client *client);

UA_StatusCode
UA_GDS_Client_deinit(UA_Client *client);

#endif // UA_ENABLE_GDS_CLIENT

#ifdef __cplusplus
} // extern "C"
#endif

#endif //OPEN62541_UA_GDS_CLIENT_H
