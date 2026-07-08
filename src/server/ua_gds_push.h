/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#ifndef UA_GDS_PUSH_H_
#define UA_GDS_PUSH_H_

#ifdef UA_ENABLE_GDS_PUSHMANAGEMENT

#include <open62541/types.h>

_UA_BEGIN_DECLS

/********************/
/* GDS Transaction  */
/********************/

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

    /* Callback to close all SecureChannels after calling applyChanges
     * and freeing the transaction. */
    UA_DelayedCallback dc;
} UA_GDSTransaction;

UA_StatusCode
UA_GDSTransaction_init(UA_GDSTransaction *transaction,
                       UA_Server *server,
                       const UA_NodeId sessionId);

/* Returns the appropriate CertificateGroup from the transaction.
 * If the CertificateGroup does not exist in the transaction, it will be created. */
UA_CertificateGroup*
UA_GDSTransaction_getCertificateGroup(UA_GDSTransaction *transaction,
                                      const UA_CertificateGroup *certGroup);

UA_StatusCode
UA_GDSTransaction_addCertificateInfo(UA_GDSTransaction *transaction,
                                     const UA_NodeId certificateGroupId,
                                     const UA_NodeId certificateTypeId,
                                     const UA_ByteString *certificate,
                                     const UA_ByteString *privateKey);

void
UA_GDSTransaction_clear(UA_GDSTransaction *transaction);

void
UA_GDSTransaction_delete(UA_GDSTransaction *transaction);

typedef enum UA_GDSTransactionChanges {
    UA_GDSTRANSACTIONCHANGES_NOTHING = 0,
    UA_GDSTRANSACTIONCHANGES_TRUSTLIST,
    UA_GDSTRANSACTIONCHANGES_CERTIFICATE,
    UA_GDSTRANSACTIONCHANGES_BOTH,
} UA_GDSTransactionChanges;

/***************/
/* FileContext */
/***************/

#define UA_SHA1_LENGTH 20
#define CHECKACTIVESESSIONINTERVAL 10000 /* 10sec */

typedef struct UA_FileContext {
    LIST_ENTRY(UA_FileContext) listEntry;
    UA_ByteString file;
    /* Caches any data to be written using the Write method.
     * With a CloseAndUpdate, the data to be written is applied to the transaction. */
    UA_ByteString dataToWrite;
    UA_UInt32 fileHandle;
    UA_NodeId sessionId;
    UA_UInt64 currentPos;
    UA_Byte openFileMode;
} UA_FileContext;

typedef struct UA_FileInfo {
    UA_UInt16 openCount;
    UA_UtcTime lastUpdateTime;
    LIST_HEAD(, UA_FileContext)fileContext;
} UA_FileInfo;

typedef struct UA_FileInfoContext {
    struct UA_FileInfoContext *next;
    UA_NodeId certificateGroupId;
    UA_FileInfo fileInfo;
} UA_FileInfoContext;

/********************/
/*   GDS Manager    */
/********************/

typedef struct {
    UA_Driver drv;

    UA_Boolean initialized; /* NS0 was added */

    /* Transaction for certificate management */
    UA_GDSTransaction transaction;
    /* Contains context information necessary for reading and writing the TrustList as a file type */
    void *fileInfoContext;
    /* Holds the ID for the repeated callback that verifies the presence of sessions
     * with an active transaction or an open trust list */
    UA_UInt64 checkSessionCallbackId;
} UA_GDSManager;

UA_Driver *
UA_GDSPushReceiveManager_new(void);

UA_StatusCode
initNS0PushManagement(UA_Server *server);

UA_CertificateGroup*
getCertGroup(UA_Server *server, const UA_NodeId *objectId);

UA_StatusCode
writeOpenCountVariable(UA_Server *server, UA_CertificateGroup *group);

UA_StatusCode
writeLastUpdateVariable(UA_Server *server, UA_CertificateGroup *group);

UA_FileContext*
getFileContext(UA_FileInfo *fileInfo, const UA_NodeId *sessionId,
               const UA_UInt32 fileHandle);

UA_FileInfo *
UA_GDSManager_getFileInfo(UA_GDSManager *gdsm, UA_NodeId certificateGroupId);

UA_StatusCode
UA_GDSManager_openTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                            const UA_NodeId *sessionId, UA_Byte fileOpenMode,
                            UA_Variant *output);

UA_StatusCode
UA_GDSManager_closeTrustList(UA_GDSManager *gdsm,
                             UA_CertificateGroup *certGroup,
                             const UA_NodeId *sessionId,
                             UA_UInt32 fileHandle);

UA_StatusCode
UA_GDSManager_closeAndUpdateTrustList(UA_GDSManager *gdsm,
                                      UA_CertificateGroup *certGroup,
                                      const UA_NodeId *sessionId,
                                      UA_UInt32 fileHandle,
                                      UA_Variant *output);

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

/* TODO: Process issuer certificates */
/* UA_ByteString *issuerCertificates */
/* size_t issuerCertificatesSize */
UA_StatusCode
UA_GDSManager_updateCertificate(UA_GDSManager *gdsm,
                                const UA_NodeId *sessionId,
                                const UA_NodeId *certificateGroupId,
                                const UA_NodeId *certificateTypeId,
                                const UA_ByteString *certificate,
                                const UA_String *privateKeyFormat,
                                const UA_ByteString *privateKey);

UA_StatusCode
UA_GDSManager_addCertificate(UA_GDSManager *gdsm,
                             UA_CertificateGroup *certGroup,
                             UA_ByteString *certificate,
                             const UA_Boolean *isTrustedCertificate);

UA_StatusCode
UA_GDSManager_removeCertificate(UA_GDSManager *gdsm,
                                UA_CertificateGroup *certGroup,
                                const UA_NodeId *sessionId,
                                const UA_String *thumbprint,
                                const UA_Boolean *isTrustedCertificate);

UA_StatusCode
UA_GDSManager_applyChanges(UA_GDSManager *gdsm);

_UA_END_DECLS

#endif /* UA_ENABLE_GDS_PUSHMANAGEMENT */

#endif /* UA_GDS_PUSH_H_ */
