/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/certificategroup_default.h>
#include "gds_receive_internal.h"

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVE

typedef enum UA_GDSTransactionChanges {
    UA_GDSTRANSACTIONCHANGES_NOTHING = 0,
    UA_GDSTRANSACTIONCHANGES_TRUSTLIST,
    UA_GDSTRANSACTIONCHANGES_CERTIFICATE,
    UA_GDSTRANSACTIONCHANGES_BOTH,
} UA_GDSTransactionChanges;

static UA_FileContext*
getFileContext(UA_FileInfo *fileInfo, const UA_NodeId *sessionId,
               const UA_UInt32 fileHandle) {
    if(!fileInfo || !sessionId)
        return NULL;

    UA_FileContext *fileContext = NULL;
    LIST_FOREACH(fileContext, &fileInfo->fileContext, listEntry) {
        if(fileContext->fileHandle == fileHandle &&
           UA_NodeId_equal(&fileContext->sessionId, sessionId)){
            return fileContext;
        }
    }
    return NULL;
}

/********************/
/* GDS Transaction  */
/********************/

static UA_StatusCode
UA_GDSTransaction_init(UA_GDSTransaction *transaction, UA_Server *server,
                       const UA_NodeId sessionId) {
    if(!transaction || !server)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ByteString csr = UA_BYTESTRING_NULL;
    if(transaction->localCsrCertificate.length > 0)
        csr = transaction->localCsrCertificate;

    memset(transaction, 0, sizeof(UA_GDSTransaction));

    transaction->state = UA_GDSTRANSACTIONSTATE_PENDING;
    UA_NodeId_copy(&sessionId, &transaction->sessionId);
    transaction->server = server;
    transaction->localCsrCertificate = csr;

    return UA_STATUSCODE_GOOD;
}

/* Returns the appropriate CertificateGroup from the transaction.
 * If the CertificateGroup does not exist in the transaction, it will be created. */
static UA_CertificateGroup*
UA_GDSTransaction_getCertificateGroup(UA_GDSTransaction *transaction,
                                      const UA_CertificateGroup *certGroup) {
    if(!transaction || !certGroup)
        return NULL;

    /* Check if transaction was initialized */
    if(transaction->state != UA_GDSTRANSACTIONSTATE_PENDING)
        return NULL;

    for(size_t i = 0; i < transaction->certGroupSize; i++) {
        UA_CertificateGroup *group = &transaction->certGroups[i];
        if(UA_NodeId_equal(&group->certificateGroupId, &certGroup->certificateGroupId))
            return group;
    }

    /* If the certGroup does not exist, create a new one */
    transaction->certGroups = (UA_CertificateGroup*)
        UA_realloc(transaction->certGroups,
                   (transaction->certGroupSize + 1) * sizeof(UA_CertificateGroup));
    if(!transaction->certGroups)
        return NULL;

    transaction->certGroupSize++;

    UA_CertificateGroup* newGroup = (UA_CertificateGroup*)
        &transaction->certGroups[transaction->certGroupSize-1];
    memset(newGroup, 0, sizeof(UA_CertificateGroup));

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    certGroup->getTrustList((UA_CertificateGroup*)(uintptr_t)certGroup, &trustList);

    /* Set up the parameters */
    UA_KeyValuePair params[1] = {{{0, UA_STRING_STATIC("max-trust-listsize")}, {0}}};
    UA_KeyValueMap paramsMap = {1, params};
    UA_ServerConfig *config = UA_Server_getConfig(transaction->server);
    UA_Variant_setScalar(&params[0].value, &config->maxTrustListSize,
                         &UA_TYPES[UA_TYPES_UINT32]);

    /* Initialize the CertificateGroup */
    UA_CertificateGroup_Memorystore(newGroup,
                                    (UA_NodeId*)(uintptr_t)&certGroup->certificateGroupId,
                                    &trustList, certGroup->logging, &paramsMap);

    UA_TrustListDataType_clear(&trustList);
    return newGroup;
}

static UA_StatusCode
UA_GDSTransaction_addCertificateInfo(UA_GDSTransaction *transaction,
                                     const UA_NodeId certificateGroupId,
                                     const UA_NodeId certificateTypeId,
                                     const UA_ByteString *certificate,
                                     const UA_ByteString *privateKey) {
    if(!transaction || !certificate)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check if transaction was initialized */
    if(transaction->state != UA_GDSTRANSACTIONSTATE_PENDING)
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* Check if an entry with certificateGroupId and certificateTypeId already exists */
    for(size_t i = 0; i < transaction->certificateInfosSize; i++) {
        UA_GDSCertificateInfo *certInfo = &transaction->certificateInfos[i];
        if(!UA_NodeId_equal(&certInfo->certificateGroup, &certificateGroupId) ||
           !UA_NodeId_equal(&certInfo->certificateType, &certificateTypeId))
            continue;

        UA_ByteString_clear(&certInfo->certificate);
        UA_ByteString_clear(&certInfo->privateKey);
        UA_ByteString_copy(certificate, &certInfo->certificate);
        if(privateKey)
            UA_ByteString_copy(privateKey, &certInfo->privateKey);
        return UA_STATUSCODE_GOOD;
    }

    UA_GDSCertificateInfo *newCertInfos = (UA_GDSCertificateInfo *)
        UA_realloc(transaction->certificateInfos,
                   (transaction->certificateInfosSize + 1) * sizeof(UA_GDSCertificateInfo));
    if(!newCertInfos)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    transaction->certificateInfos = newCertInfos;

    UA_GDSCertificateInfo *newCertInfo =
        &transaction->certificateInfos[transaction->certificateInfosSize];
    UA_ByteString_copy(certificate, &newCertInfo->certificate);
    UA_NodeId_copy(&certificateGroupId, &newCertInfo->certificateGroup);
    UA_NodeId_copy(&certificateTypeId, &newCertInfo->certificateType);
    newCertInfo->privateKey = UA_BYTESTRING_NULL;
    if(privateKey)
        UA_ByteString_copy(privateKey, &newCertInfo->privateKey);

    transaction->certificateInfosSize++;

    return UA_STATUSCODE_GOOD;
}

static void
UA_GDSTransaction_clear(UA_GDSTransaction *transaction) {
    if(!transaction)
        return;

    transaction->state = UA_GDSTRANSACTIONSTATE_FRESH;
    transaction->server = NULL;
    UA_NodeId_clear(&transaction->sessionId);
    UA_ByteString_clear(&transaction->localCsrCertificate);

    if(transaction->certGroups) {
        for(size_t i = 0; i < transaction->certGroupSize; i++) {
            transaction->certGroups[i].clear(&transaction->certGroups[i]);
        }
        UA_free(transaction->certGroups);
        transaction->certGroupSize = 0;
        transaction->certGroups = NULL;
    }

    if(transaction->certificateInfos) {
        for(size_t i = 0; i < transaction->certificateInfosSize; i++) {
            UA_ByteString_clear(&transaction->certificateInfos[i].certificate);
            UA_ByteString_clear(&transaction->certificateInfos[i].privateKey);
            UA_NodeId_clear(&transaction->certificateInfos[i].certificateGroup);
            UA_NodeId_clear(&transaction->certificateInfos[i].certificateType);
        }
        UA_free(transaction->certificateInfos);
        transaction->certificateInfosSize = 0;
        transaction->certificateInfos = NULL;
    }
}

/********************/
/*   GDS Manager    */
/********************/

UA_FileInfo *
UA_GDSManager_getFileInfo(UA_GDSManager *gdsm,
                          UA_NodeId certificateGroupId) {
    UA_FileInfo *fi = (UA_FileInfo*)gdsm->fileInfos;
    while(fi) {
        if(UA_NodeId_equal(&fi->certificateGroupId, &certificateGroupId))
            return fi;
        fi = fi->next;
    }
    return NULL;
}

/* This callback is triggered at regular intervals as long as a transaction is ongoing
 * or a client has a TrustList open for reading or writing.
 * If the session is no longer active, the current transaction is cancelled,
 * and all open file handles are closed. */
static void
checkSessionActive(UA_Server *server, void *data) {
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    UA_EventLoop *el = sc->eventLoop;
    el->lock(el);

    UA_GDSManager *gdsm = gdsManager(server);
    UA_GDSTransaction *transaction = &gdsm->transaction;
    UA_Boolean removingCallback = false;
    if(transaction->state != UA_GDSTRANSACTIONSTATE_FRESH) {
        /* Check if the session is still active */
        UA_Variant tmp;
        UA_StatusCode res =
            UA_Server_getSessionAttribute(server, &transaction->sessionId,
                                          UA_QUALIFIEDNAME(0, "sessionName"), &tmp);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(sc->logging, UA_LOGCATEGORY_SERVER,
                "Session with an open transaction has ended. "
                "The transaction has been discarded.");
            UA_GDSTransaction_clear(transaction);
            removingCallback = true;
        }
    } else {
        removingCallback = true;
    }

    UA_FileInfo *fi = (UA_FileInfo*)gdsm->fileInfos;
    for(; fi; fi = fi->next) {
        if(fi->openCount == 0)
            continue;

        removingCallback = false;

        UA_CertificateGroup *certGroup =
            getCertGroup(server, &fi->certificateGroupId);
        if(!certGroup)
            continue;

        UA_FileContext *fileContext, *fileContextTmp;
        LIST_FOREACH_SAFE(fileContext, &fi->fileContext, listEntry, fileContextTmp) {
            UA_Variant tmp;
            UA_StatusCode res =
                UA_Server_getSessionAttribute(server, &fileContext->sessionId,
                                              UA_QUALIFIEDNAME(0, "sessionName"), &tmp);
            if(res == UA_STATUSCODE_GOOD)
                continue; /* Session still exists */

            UA_LOG_INFO(sc->logging, UA_LOGCATEGORY_SERVER,
                        "Session with an open trust list has ended. "
                        "All file handlers for the open trust lists have been closed.");

            LIST_REMOVE(fileContext, listEntry);
            fi->openCount -= 1;

            UA_ByteString_clear(&fileContext->file);
            UA_ByteString_clear(&fileContext->dataToWrite);
            UA_free(fileContext);

            /* Updating OpenCount Variable in the information model */
            writeOpenCountVariable(server, certGroup);
        }
    }

    if(removingCallback) {
        UA_Server_removeCallback(server, gdsm->checkSessionCallbackId);
        gdsm->checkSessionCallbackId = 0;
    }

    el->unlock(el);
}

UA_StatusCode
UA_GDSManager_getPositionTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                                   const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                                   UA_Variant *output) {
    UA_assert(certGroup != NULL);
    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_Variant_setScalarCopy(output, &fileContext->currentPos,
                                    &UA_TYPES[UA_TYPES_UINT64]);
}

UA_StatusCode
UA_GDSManager_setPositionTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                                   const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                                   UA_UInt64 position) {
    UA_assert(certGroup != NULL);
    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(fileContext->file.length < position) {
        fileContext->currentPos = fileContext->file.length;
    } else {
        fileContext->currentPos = position;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDSManager_writeTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                             const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                             const UA_ByteString data) {
    UA_assert(certGroup != NULL);
    UA_Server *server = gdsm->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);

    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(fileContext->openFileMode !=
       (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* Abort when TrustList size would exceed the maximum allowed value (0 =
     * unlimited) */
    size_t newLen = fileContext->dataToWrite.length + data.length;
    if(sc->maxTrustListSize != 0 && newLen > sc->maxTrustListSize) {
        UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_SERVER,
                       "Write on trust list exceeds limit");
        return UA_STATUSCODE_BADREQUESTTOOLARGE;
    }

    return UA_String_append(&fileContext->dataToWrite, data);
}

UA_StatusCode
UA_GDSManager_closeAndUpdateTrustList(UA_GDSManager *gdsm,
                                      UA_CertificateGroup *certGroup,
                                      const UA_NodeId *sessionId,
                                      UA_UInt32 fileHandle,
                                      UA_Variant *output) {
    UA_assert(certGroup != NULL);
    UA_Server *server = gdsm->drv.server;

    UA_GDSTransaction *transaction = &gdsm->transaction;
    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(fileContext->openFileMode !=
       (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_CertificateGroup *transactionCertGroup =
        UA_GDSTransaction_getCertificateGroup(transaction, certGroup);
    if(!transactionCertGroup)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_TrustListDataType trustList;
    UA_StatusCode retval =
        UA_decodeBinary(&fileContext->dataToWrite, &trustList,
                        &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_TrustListDataType_clear(&trustList);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    retval = transactionCertGroup->setTrustList(transactionCertGroup, &trustList);
    UA_TrustListDataType_clear(&trustList);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    LIST_REMOVE(fileContext, listEntry);
    fileInfo->openCount -= 1;

    UA_ByteString_clear(&fileContext->file);
    UA_ByteString_clear(&fileContext->dataToWrite);
    UA_free(fileContext);

    /* Updating OpenCount Variable in the information model */
    writeOpenCountVariable(server, certGroup);

    /* Output arg, indicates that the ApplyChanges Method shall be called before
     * the new trust list will be used. */
    UA_Boolean applyChangesRequired = true;
    UA_Variant_setScalarCopy(output, &applyChangesRequired, &UA_TYPES[UA_TYPES_BOOLEAN]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDSManager_closeTrustList(UA_GDSManager *gdsm,
                             UA_CertificateGroup *certGroup,
                             const UA_NodeId *sessionId,
                             UA_UInt32 fileHandle) {
    UA_assert(certGroup != NULL);
    UA_Server *server = gdsm->drv.server;

    UA_GDSTransaction *transaction = &gdsm->transaction;
    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* If a close is called, a current transaction is cancelled.
     * If the list was opened in read mode, there are no changes to discard. */
    if(fileContext->openFileMode ==
       (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
        UA_GDSTransaction_clear(transaction);

    LIST_REMOVE(fileContext, listEntry);
    fileInfo->openCount -= 1;

    UA_ByteString_clear(&fileContext->file);
    UA_ByteString_clear(&fileContext->dataToWrite);
    UA_free(fileContext);

    /* Updating OpenCount Variable in the information model */
    writeOpenCountVariable(server, certGroup);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDSManager_readTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                            const UA_NodeId *sessionId, UA_UInt32 fileHandle,
                            UA_Int32 length, UA_Variant *output) {
    UA_assert(certGroup != NULL);

    /* UA_GDSManager *gdsm = gdsManager(server); */
    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(fileContext->openFileMode != UA_OPENFILEMODE_READ)
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* check boundaries */
    if((size_t)length >= fileContext->file.length)
        length = (UA_Int32)fileContext->file.length;

    if((size_t)length >= (fileContext->file.length - fileContext->currentPos))
        length = (UA_Int32)(fileContext->file.length - fileContext->currentPos);

    UA_ByteString readBuffer = UA_BYTESTRING_NULL;
    if(length > 0) {
        readBuffer.length = (size_t)length;
        readBuffer.data = fileContext->file.data+fileContext->currentPos;
        fileContext->currentPos += (UA_UInt64)length;
    }

    UA_Variant_setScalarCopy(output, &readBuffer, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return UA_STATUSCODE_GOOD;
}

/* TODO: Handle isTrustedCertificate */
UA_StatusCode
UA_GDSManager_addCertificate(UA_GDSManager *gdsm,
                             UA_CertificateGroup *certGroup,
                             UA_ByteString *certificate,
                             const UA_Boolean *isTrustedCertificate) {
    UA_assert(certGroup != NULL);
    UA_Server *server = gdsm->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);

    /* CA certificates cannot be added using this method because it does not
     * support adding CRLs */
    if(UA_CertificateUtils_checkCA(certificate) == UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "The certificate could not be added because it is a CA certificate. "
                     "CA certificates must be added using the FileType methods.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* This method cannot be called if the containing TrustList Object is open */
    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(fileInfo->openCount > 0)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    trustList.trustedCertificates = certificate;
    trustList.trustedCertificatesSize = 1;

    UA_StatusCode res = certGroup->addToTrustList(certGroup, &trustList);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Updating LastUpdateTime Variable in the information model */
    fileInfo->lastUpdateTime = UA_DateTime_now();
    writeLastUpdateVariable(server, certGroup);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDSManager_updateCertificate(UA_GDSManager *gdsm,
                                const UA_NodeId *sessionId,
                                const UA_NodeId *certificateGroupId,
                                const UA_NodeId *certificateTypeId,
                                const UA_ByteString *certificate,
                                const UA_String *privateKeyFormat,
                                const UA_ByteString *privateKey) {
    /* The server currently only supports the DefaultApplicationGroup */
    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    if(!UA_NodeId_equal(certificateGroupId, &defaultApplicationGroup))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* The server currently only supports the following certificate type */
    static UA_NodeId certTypRsaMin = STATIC_NS0ID(RSAMINAPPLICATIONCERTIFICATETYPE);
    static UA_NodeId certTypRsaSha256 = STATIC_NS0ID(RSASHA256APPLICATIONCERTIFICATETYPE);
    if(!UA_NodeId_equal(certificateTypeId, &certTypRsaSha256) &&
       !UA_NodeId_equal(certificateTypeId, &certTypRsaMin))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* Verify that the privateKey is in a supported format and
     * that it matches the specified certificate */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(privateKey && privateKey->length > 0) {
        const UA_String pemFormat = UA_STRING("PEM");
        const UA_String derFormat = UA_STRING("DER");
        if(!UA_String_equal(&pemFormat, privateKeyFormat) &&
           !UA_String_equal(&derFormat, privateKeyFormat))
            return UA_STATUSCODE_BADNOTSUPPORTED;
        retval = UA_CertificateUtils_checkKeyPair(certificate, privateKey);
        if(retval != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    UA_GDSTransaction *transaction = &gdsm->transaction;
    if(transaction->state == UA_GDSTRANSACTIONSTATE_FRESH) {
        retval = UA_GDSTransaction_init(transaction, gdsm->drv.server, *sessionId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    if(gdsm->checkSessionCallbackId == 0) {
        retval = UA_Server_addRepeatedCallback(gdsm->drv.server, checkSessionActive,
                                               NULL, CHECKACTIVESESSIONINTERVAL,
                                               &gdsm->checkSessionCallbackId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    if(!UA_NodeId_equal(&transaction->sessionId, sessionId))
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    return UA_GDSTransaction_addCertificateInfo(transaction, *certificateGroupId,
                                                *certificateTypeId, certificate,
                                                privateKey);
}

/* TODO: Expose implementation in core open62541 */

static UA_Byte
lc(UA_Byte c) {
    if(((int)c) - 'A' < 26) return c | 32;
    return c;
}
static int
case_cmp(const UA_Byte *l, const UA_Byte *r, size_t n) {
    if(!n--) return 0;
    for(; *l && *r && n && (*l == *r || lc(*l) == lc(*r)); l++, r++, n--);
    return lc(*l) - lc(*r);
}
static UA_Boolean
String_equal_ignorecase(const UA_String *s1, const UA_String *s2) {
    if(s1->length != s2->length)
        return false;
    if(s1->length == 0)
        return true;
    if(s2->data == NULL)
        return false;
    return case_cmp(s1->data, s2->data, s1->length) == 0;
}

UA_StatusCode
UA_GDSManager_removeCertificate(UA_GDSManager *gdsm,
                                UA_CertificateGroup *certGroup,
                                const UA_NodeId *sessionId,
                                const UA_String *thumbprint,
                                const UA_Boolean *isTrustedCertificate) {
    UA_assert(certGroup != NULL);
    UA_Server *server = gdsm->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);

    UA_GDSTransaction *transaction = &gdsm->transaction;
    if(transaction->state != UA_GDSTRANSACTIONSTATE_FRESH)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    /* This Method cannot be called if the containing TrustList Object is open */
    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(fileInfo->openCount > 0)
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* When a certificate is removed, a transaction is created which is then
     * executed directly. No apply cahnges is required */
    UA_StatusCode retval = UA_GDSTransaction_init(transaction, server, *sessionId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_CertificateGroup *transactionCG =
        UA_GDSTransaction_getCertificateGroup(transaction, certGroup);
    if(!transactionCG)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    UA_ByteString *certificates;
    size_t certificatesSize = 0;
    certGroup->getTrustList(certGroup, &trustList);

    if(*isTrustedCertificate) {
        certificates = trustList.trustedCertificates;
        certificatesSize = trustList.trustedCertificatesSize;
    } else {
        certificates = trustList.issuerCertificates;
        certificatesSize = trustList.issuerCertificatesSize;
    }

    UA_TrustListDataType list;
    UA_TrustListDataType_init(&list);

    UA_ByteString *crls = NULL;
    size_t crlsSize = 0;
    UA_ByteString certificate = UA_BYTESTRING_NULL;

    UA_Byte buf[UA_SHA1_LENGTH * 2];
    UA_String thumbpr = {UA_SHA1_LENGTH * 2, buf};
    for(size_t i = 0; i < certificatesSize; i++) {
        /* Compare thumbprint */
        certificate = certificates[i];
        UA_CertificateUtils_getThumbprint(&certificate, &thumbpr);
        if(!String_equal_ignorecase(thumbprint, &thumbpr))
            continue;

        retval = certGroup->getCertificateCrls(certGroup, &certificate,
                                               isTrustedCertificate,
                                               &crls, &crlsSize);

        /* Tolerate "Bad_NoMatch" to support removing CA certificates that do
         * not have an associated CRL. */
        if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADNOMATCH)
            goto cleanup;

        if(*isTrustedCertificate) {
            list.specifiedLists =
                UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_TRUSTEDCRLS;
            list.trustedCertificates = &certificate;
            list.trustedCertificatesSize = 1;
            list.trustedCrls = crls;
            list.trustedCrlsSize = crlsSize;
        } else {
            list.specifiedLists =
                UA_TRUSTLISTMASKS_ISSUERCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCRLS;
            list.issuerCertificates = &certificate;
            list.issuerCertificatesSize = 1;
            list.issuerCrls = crls;
            list.issuerCrlsSize = crlsSize;
        }
        break;
    }

    /* Thumbprint not found */
    if(list.specifiedLists == UA_TRUSTLISTMASKS_NONE) {
        UA_LOG_INFO(sc->logging, UA_LOGCATEGORY_SERVER,
                    "The certificate to remove was not found");
        retval = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto cleanup;
    }

    /* Add to the transaction */
    retval = transactionCG->removeFromTrustList(transactionCG, &list);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Apply */
    retval = UA_GDSManager_applyChanges(gdsm);

cleanup:
    UA_TrustListDataType_clear(&trustList);
    UA_Array_delete(crls, crlsSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return retval;
}

UA_StatusCode
UA_GDSManager_getRejectedList(UA_GDSManager *gdsm, size_t outputSize,
                              UA_Variant *output) {
    UA_Server *server = gdsm->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);

    size_t rejectedListSize = 0;

    /* DefaultApplicationGroup */
    UA_CertificateGroup *certGroup = &sc->secureChannelPKI;
    UA_ByteString *rejectedListSecureChannel = NULL;
    size_t rejectedListSecureChannelSize = 0;
    certGroup->getRejectedList(certGroup, &rejectedListSecureChannel,
                               &rejectedListSecureChannelSize);
    rejectedListSize += rejectedListSecureChannelSize;

    /* DefaultUserTokenGroup */
    certGroup = &sc->sessionPKI;
    UA_ByteString *rejectedListSession = NULL;
    size_t rejectedListSessionSize = 0;
    certGroup->getRejectedList(certGroup, &rejectedListSession, &rejectedListSessionSize);
    rejectedListSize += rejectedListSessionSize;

    if(rejectedListSize == 0) {
        UA_Variant_setArray(&output[0], NULL, 0, &UA_TYPES[UA_TYPES_BYTESTRING]);
        return UA_STATUSCODE_GOOD;
    }

    /* Create a temp array (shallow) */
    UA_ByteString *rejectedList = (UA_ByteString*)
        UA_Array_new(rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    if(!rejectedList) {
        UA_Array_delete(rejectedListSecureChannel,
                        rejectedListSecureChannelSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_Array_delete(rejectedListSession,
                        rejectedListSessionSize,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memcpy(rejectedList, rejectedListSecureChannel,
           rejectedListSecureChannelSize * sizeof(UA_ByteString));
    memcpy(rejectedList + rejectedListSecureChannelSize,
           rejectedListSession, rejectedListSessionSize * sizeof(UA_ByteString));

    /* Set the array in the output */
    UA_Variant_setArrayCopy(&output[0], rejectedList, rejectedListSize,
                            &UA_TYPES[UA_TYPES_BYTESTRING]);

    /* Clean up */
    UA_Array_delete(rejectedListSecureChannel, rejectedListSecureChannelSize,
                    &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Array_delete(rejectedListSession, rejectedListSessionSize,
                    &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_free(rejectedList);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createFileHandleId(UA_FileInfo *fileInfo, UA_UInt32 *fileHandle) {
    if(!fileInfo || !fileHandle)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_UInt32 id;
    UA_Boolean isFree = true;
    for(id = 1; id < UA_UINT32_MAX; id++) {
        UA_FileContext *fileContext = NULL;
        LIST_FOREACH(fileContext, &fileInfo->fileContext, listEntry) {
            if(fileContext->fileHandle == id){
                isFree = false;
                break;
            }
        }
        if(isFree) {
            *fileHandle = id;
            return UA_STATUSCODE_GOOD;
        }
        isFree = true;
    }
    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode
UA_GDSManager_openTrustListWithMask(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                                    const UA_NodeId *sessionId, UA_UInt32 mask,
                                    UA_Variant *output) {
    UA_assert(certGroup != NULL);
    UA_Server *server = gdsm->drv.server;

    if(gdsm->transaction.state == UA_GDSTRANSACTIONSTATE_PENDING)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(gdsm->checkSessionCallbackId == 0) {
        retval = UA_Server_addRepeatedCallback(server, checkSessionActive, NULL,
                                               CHECKACTIVESESSIONINTERVAL,
                                               &gdsm->checkSessionCallbackId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);
    trustList.specifiedLists = mask;
    certGroup->getTrustList(certGroup, &trustList);

    UA_ByteString encTrustList = UA_BYTESTRING_NULL;
    retval = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                             &encTrustList, NULL);
    UA_TrustListDataType_clear(&trustList);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&encTrustList);
        return retval;
    }

    UA_FileContext *fileContext = (UA_FileContext*)
        UA_calloc(1, sizeof(UA_FileContext));
    if(!fileContext) {
        UA_ByteString_clear(&fileContext->file);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
        
    fileContext->file = encTrustList;
    fileContext->sessionId = *sessionId;
    fileContext->openFileMode = UA_OPENFILEMODE_READ;
    fileContext->currentPos = 0;
    fileContext->dataToWrite = UA_BYTESTRING_NULL;
    retval = createFileHandleId(fileInfo, &fileContext->fileHandle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&fileContext->file);
        UA_free(fileContext);
        return retval;
    }

    fileInfo->openCount += 1;
    UA_Variant_setScalarCopy(output, &fileContext->fileHandle,
                             &UA_TYPES[UA_TYPES_UINT32]);

    /* Updating OpenCount Variable in the information model */
    writeOpenCountVariable(server, certGroup);

    LIST_INSERT_HEAD(&fileInfo->fileContext, fileContext, listEntry);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDSManager_openTrustList(UA_GDSManager *gdsm, UA_CertificateGroup *certGroup,
                            const UA_NodeId *sessionId, UA_Byte fileOpenMode,
                            UA_Variant *output) {
    UA_assert(certGroup != NULL);
    UA_Server *server = gdsm->drv.server;

    UA_GDSTransaction *transaction = &gdsm->transaction;
    /* Cannot be opened when a transaction is running */
    if(transaction->state == UA_GDSTRANSACTIONSTATE_PENDING)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Check that the list can be opened in the specified mode */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(fileOpenMode == (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING)) {
        if(fileInfo->openCount != 0)
            return UA_STATUSCODE_BADNOTWRITABLE;
    } else if(fileOpenMode == UA_OPENFILEMODE_READ) {
        /* Nothing to check.
         * If the list is already open for writing,
         * the previous check for a current transaction will fail. */
    } else {
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* If the list is opened for writing, a transaction must be created */
    if(fileOpenMode == (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING)) {
        retval = UA_GDSTransaction_init(transaction, server, *sessionId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    if(gdsm->checkSessionCallbackId == 0) {
        retval = UA_Server_addRepeatedCallback(server, checkSessionActive, NULL,
                                               CHECKACTIVESESSIONINTERVAL,
                                               &gdsm->checkSessionCallbackId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    certGroup->getTrustList(certGroup, &trustList);

    UA_ByteString encTrustList = UA_BYTESTRING_NULL;
    retval = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE],
                             &encTrustList, NULL);
    UA_TrustListDataType_clear(&trustList);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&encTrustList);
        return retval;
    }

    UA_FileContext *fileContext = (UA_FileContext*)UA_calloc(1, sizeof(UA_FileContext));
    fileContext->file = encTrustList;
    fileContext->sessionId = *sessionId;
    fileContext->openFileMode = fileOpenMode;
    fileContext->currentPos = 0;
    fileContext->dataToWrite = UA_BYTESTRING_NULL;
    retval = createFileHandleId(fileInfo, &fileContext->fileHandle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&fileContext->file);
        UA_free(fileContext);
        return retval;
    }

    fileInfo->openCount += 1;
    UA_Variant_setScalarCopy(output, &fileContext->fileHandle,
                             &UA_TYPES[UA_TYPES_UINT32]);

    /* Updating OpenCount Variable in the information model */
    writeOpenCountVariable(server, certGroup);

    LIST_INSERT_HEAD(&fileInfo->fileContext, fileContext, listEntry);

    return UA_STATUSCODE_GOOD;
}

static void
secureChannel_delayedClose(void *application, void *context) {
    UA_Server *server = (UA_Server*)context;
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    UA_GDSManager *gdsm = gdsManager(server);
    UA_GDSTransactionChanges *changes = (UA_GDSTransactionChanges*)application;

    ChannelMetadata *cm;
    UA_CertificateGroup *certGroup = &sc->secureChannelPKI;
    switch(*changes) {
    case UA_GDSTRANSACTIONCHANGES_NOTHING:
        break;

    case UA_GDSTRANSACTIONCHANGES_BOTH:
    case UA_GDSTRANSACTIONCHANGES_CERTIFICATE:
        /* Shutdown all SecureChannels */
        LIST_FOREACH(cm, &gdsm->secureChannels, pointers) {
            UA_Server_closeSecureChannel(server, cm->channelId,
                                         UA_SHUTDOWNREASON_CLOSE);
        }
        break;

    default:
        /* Re-verify remote certificates. Close the SecureChannel on failure. */
        LIST_FOREACH(cm, &gdsm->secureChannels, pointers) {
            UA_StatusCode res =
                certGroup->verifyCertificate(certGroup, &cm->certificate);
            if(res != UA_STATUSCODE_GOOD)
                UA_Server_closeSecureChannel(server, cm->channelId,
                                             UA_SHUTDOWNREASON_CLOSE);
        }
        break;
    }

    UA_free(changes);
    UA_GDSTransaction_clear(&gdsm->transaction);
}

static UA_SecurityPolicy *
getSecPolicyByUri(UA_ServerConfig *sc, const UA_String *securityPolicyUri) {
    for(size_t i = 0; i < sc->securityPoliciesSize; i++) {
        UA_SecurityPolicy *sp = &sc->securityPolicies[i];
        if(UA_String_equal(securityPolicyUri, &sp->policyUri))
            return sp;
    }
    return NULL;
}

UA_StatusCode
UA_GDSManager_applyChanges(UA_GDSManager *gdsm) {
    UA_Server *server = gdsm->drv.server;
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    UA_GDSTransaction *transaction = &gdsm->transaction;

    /* Check if a TrustList is still open */
    for(size_t i = 0; i < transaction->certGroupSize; i++) {
        UA_CertificateGroup *certGroup = &transaction->certGroups[i];
        UA_FileInfo *fileInfo =
            UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
        if(!fileInfo)
            return UA_STATUSCODE_BADINTERNALERROR;
        if(fileInfo->openCount > 0)
            return UA_STATUSCODE_BADINVALIDSTATE;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_GDSTransactionChanges *changes = (UA_GDSTransactionChanges*)
        UA_calloc(1, sizeof(UA_GDSTransactionChanges));
    if(!changes)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Apply Trust list changes */
    for(size_t i = 0; i < transaction->certGroupSize; i++) {
        *changes = UA_GDSTRANSACTIONCHANGES_TRUSTLIST;
        UA_CertificateGroup transactionCertGroup = transaction->certGroups[i];
        UA_TrustListDataType trustList;
        UA_TrustListDataType_init(&trustList);
        trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
        transactionCertGroup.getTrustList(&transactionCertGroup, &trustList);

        UA_CertificateGroup *certGroup =
            getCertGroup(server, &transactionCertGroup.certificateGroupId);
        if(!certGroup) {
            UA_TrustListDataType_clear(&trustList);
            goto cleanup;
        }
        retval = certGroup->setTrustList(certGroup, &trustList);
        UA_TrustListDataType_clear(&trustList);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;

        UA_FileInfo *fileInfo =
            UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
        if(!fileInfo) {
            retval = UA_STATUSCODE_BADINTERNALERROR;
            goto cleanup;
        }

        /* Updating LastUpdateTime Variable in the information model */
        fileInfo->lastUpdateTime = UA_DateTime_now();
        writeLastUpdateVariable(server, certGroup);
    }

    /* Apply Server certificate changes */
    for(size_t i = 0; i < transaction->certificateInfosSize; i++) {
        if(*changes != UA_GDSTRANSACTIONCHANGES_NOTHING) {
            *changes = UA_GDSTRANSACTIONCHANGES_BOTH;
        } else {
            *changes = UA_GDSTRANSACTIONCHANGES_CERTIFICATE;
        }
        UA_GDSCertificateInfo certInfo = transaction->certificateInfos[i];
        UA_NodeId certTypeId = certInfo.certificateType;
        UA_ByteString certificate = certInfo.certificate;
        UA_ByteString privateKey = certInfo.privateKey;

        for(size_t j = 0; j < sc->endpointsSize; j++) {
            UA_EndpointDescription *ed = &sc->endpoints[j];
            UA_SecurityPolicy *sp = getSecPolicyByUri(sc, &ed->securityPolicyUri);
            if(!sp) {
                retval = UA_STATUSCODE_BADINTERNALERROR;
                goto cleanup;
            }

            if(!UA_NodeId_equal(&sp->certificateTypeId, &certTypeId))
                continue;

            retval = sp->updateCertificate(sp, certificate, privateKey);
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;

            UA_ByteString_clear(&ed->serverCertificate);
            retval = UA_ByteString_copy(&certificate, &ed->serverCertificate);
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;
        }
    }

    /* Add to the delayed callback list. Will be cleaned up in the next
     * eventloop iteration. This is required so that the apply function can
     * return a statuscode before the SecureChannel is closed. */
    UA_DelayedCallback *dc = &transaction->dc;
    dc->callback = secureChannel_delayedClose;
    dc->application = changes;
    dc->context = server;

    UA_EventLoop *el = sc->eventLoop;
    el->addDelayedCallback(el, dc);
    return UA_STATUSCODE_GOOD;

cleanup:
    UA_GDSTransaction_clear(transaction);
    UA_free(changes);
    return retval;
}

static void
secureChannelNotificationCallback(UA_Driver *drv,
                                  UA_ApplicationNotificationType type,
                                  const UA_KeyValueMap payload) {
    UA_GDSManager *gdsm = (UA_GDSManager*)drv;
    if(type == UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_OPENED) {
        UA_UInt32 channelId = *(const UA_UInt32*)
            UA_KeyValueMap_getScalar(&payload,
                                     UA_QUALIFIEDNAME(0, "securechannel-id"),
                                     &UA_TYPES[UA_TYPES_UINT32]);
        UA_ByteString certificate = *(const UA_ByteString*)
            UA_KeyValueMap_getScalar(&payload,
                                     UA_QUALIFIEDNAME(0, "remote-certificate"),
                                     &UA_TYPES[UA_TYPES_BYTESTRING]);

        ChannelMetadata *cm = (ChannelMetadata*)UA_calloc(1, sizeof(ChannelMetadata));
        if(!cm) {
            UA_ServerConfig *sc = UA_Server_getConfig(drv->server);
            UA_LOG_WARNING(sc->logging, UA_LOGCATEGORY_SERVER,
                           "Could not register SecureChannel (oom)");
            return;
        }
        cm->channelId = channelId;
        UA_ByteString_copy(&certificate, &cm->certificate);
        LIST_INSERT_HEAD(&gdsm->secureChannels, cm, pointers);
    } else if(type == UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL_CLOSED) {
        UA_UInt32 channelId = *(const UA_UInt32*)
            UA_KeyValueMap_getScalar(&payload,
                                     UA_QUALIFIEDNAME(0, "securechannel-id"),
                                     &UA_TYPES[UA_TYPES_UINT32]);
        ChannelMetadata *cm;
        LIST_FOREACH(cm, &gdsm->secureChannels, pointers) {
            if(cm->channelId != channelId)
                continue;
            LIST_REMOVE(cm, pointers);
            UA_ByteString_clear(&cm->certificate);
            UA_free(cm);
            break;
        }
    }
}

UA_StatusCode
UA_GDSManager_start(UA_Driver *drv) {
    UA_GDSManager *gdsm = (UA_GDSManager*)drv;

    /* Initialize ns0 entries only once */
    if(!gdsm->initialized) {
        UA_StatusCode res = initNS0PushManagement(drv->server);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        gdsm->initialized = true;
    }

    drv->state = UA_LIFECYCLESTATE_STARTED;

    return UA_STATUSCODE_GOOD;
}

static void
UA_GDSManager_stop(UA_Driver *drv) {
    UA_GDSManager *gdsm = (UA_GDSManager*)drv;
    if(gdsm->checkSessionCallbackId != 0) {
        UA_Server_removeCallback(drv->server, gdsm->checkSessionCallbackId);
        gdsm->checkSessionCallbackId = 0;
    }
    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

/* TODO: Remove NS0 entries here for true "driver" semantics */
static UA_StatusCode
UA_GDSManager_free(UA_Driver *drv) {
    UA_ServerConfig *sc = UA_Server_getConfig(drv->server);
    if(drv->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "Cannot delete the GDSPushReceive Driver because "
                     "it is not stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_GDSManager *gdsManager = (UA_GDSManager*)drv;
    gdsManager->checkSessionCallbackId = 0;
    UA_GDSTransaction_clear(&gdsManager->transaction);
    UA_FileInfo *fi = (UA_FileInfo*)gdsManager->fileInfos;

    /* free all fileInfoContexts */
    while(fi) {
        UA_FileInfo *next = fi->next;
        UA_FileContext *fileContext = NULL;
        UA_FileContext *fileContextTmp = NULL;

        /* free all fileContexts in this fileInfo */
        LIST_FOREACH_SAFE(fileContext, &fi->fileContext, listEntry, fileContextTmp) {
            UA_ByteString_clear(&fileContext->file);
            UA_ByteString_clear(&fileContext->dataToWrite);
            LIST_REMOVE(fileContext, listEntry);
            UA_free(fileContext);
        }

        UA_free(fi);
        fi = next;
    }

    /* Free SecureChannel Metadata */
    ChannelMetadata *cm, *cm_tmp;
    LIST_FOREACH_SAFE(cm, &gdsManager->secureChannels, pointers, cm_tmp) {
        LIST_REMOVE(cm, pointers);
        UA_ByteString_clear(&cm->certificate);
        UA_free(cm);
    }

    UA_free(drv);
    return UA_STATUSCODE_GOOD;
}

UA_Driver *
UA_GDSPushReceiveManager_new(void) {
    UA_GDSManager *gdsm = (UA_GDSManager*)UA_calloc(1, sizeof(UA_GDSManager));
    if(!gdsm)
        return NULL;

    gdsm->drv.name = UA_STRING("gds-push-receive");
    gdsm->drv.start = UA_GDSManager_start;
    gdsm->drv.stop = UA_GDSManager_stop;
    gdsm->drv.free = UA_GDSManager_free;

    gdsm->drv.notificationCallback = secureChannelNotificationCallback;
    gdsm->drv.notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_SECURECHANNEL;

    return &gdsm->drv;
}

#endif
