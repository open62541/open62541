/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/certificategroup_default.h>
#include "ua_server_internal.h"

#ifdef UA_ENABLE_GDS_PUSHMANAGEMENT

/********************/
/* GDS Transaction  */
/********************/

UA_StatusCode
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

UA_CertificateGroup*
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

    memset(&transaction->certGroups[transaction->certGroupSize-1],
           0, sizeof(UA_CertificateGroup));

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

    UA_CertificateGroup_Memorystore(&transaction->certGroups[transaction->certGroupSize-1],
                                    (UA_NodeId*)(uintptr_t)&certGroup->certificateGroupId,
                                    &trustList, certGroup->logging, &paramsMap);

    UA_TrustListDataType_clear(&trustList);

    return &transaction->certGroups[transaction->certGroupSize-1];
}

UA_StatusCode
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
        certInfo->privateKey = UA_BYTESTRING_NULL;
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

void UA_GDSTransaction_clear(UA_GDSTransaction *transaction) {
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

void UA_GDSTransaction_delete(UA_GDSTransaction *transaction) {
    UA_GDSTransaction_clear(transaction);
    UA_free(transaction);
}

/********************/
/*   GDS Manager    */
/********************/

UA_FileInfo *
UA_GDSManager_getFileInfo(UA_GDSManager *gdsm, UA_NodeId certificateGroupId) {
    UA_FileInfoContext *fileInfoContext = (UA_FileInfoContext*)gdsm->fileInfoContext;
    while(fileInfoContext) {
        if(UA_NodeId_equal(&fileInfoContext->certificateGroupId, &certificateGroupId))
            return &fileInfoContext->fileInfo;
        fileInfoContext = fileInfoContext->next;
    }
    return NULL;
}

/* TODO: Handle isTrustedCertificate */
UA_StatusCode
UA_GDSManager_addCertificate(UA_GDSManager *gdsm,
                             UA_CertificateGroup *certGroup,
                             UA_ByteString *certificate,
                             const UA_Boolean *isTrustedCertificate) {
    UA_Server *server = gdsm->drv.server;
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* CA certificates cannot be added using this method because it does not
     * support adding CRLs */
    if(UA_CertificateUtils_checkCA(certificate) == UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
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
    UA_LOCK_ASSERT(&gdsm->drv.server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* The server currently only supports the DefaultApplicationGroup */
    UA_NodeId defaultApplicationGroup =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    if(!UA_NodeId_equal(certificateGroupId, &defaultApplicationGroup))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* The server currently only supports the following certificate type */
    UA_NodeId certTypRsaMin = UA_NS0ID(RSAMINAPPLICATIONCERTIFICATETYPE);
    UA_NodeId certTypRsaSha256 = UA_NS0ID(RSASHA256APPLICATIONCERTIFICATETYPE);
    if(!UA_NodeId_equal(certificateTypeId, &certTypRsaSha256) &&
       !UA_NodeId_equal(certificateTypeId, &certTypRsaMin))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* Verify that the privateKey is in a supported format and
     * that it matches the specified certificate */
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
        retval = addRepeatedCallback(gdsm->drv.server, checkSessionActive,
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

static UA_StatusCode
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
        removeCallback(drv->server, gdsm->checkSessionCallbackId);
        gdsm->checkSessionCallbackId = 0;
    }
    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

/* TODO: Remove NS0 entries here for true "driver" semantics */
static UA_StatusCode
UA_GDSManager_free(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(drv->server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Cannot delete the GDSPushReceive Driver because "
                     "it is not stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_GDSManager *gdsManager = (UA_GDSManager*)drv;
    gdsManager->checkSessionCallbackId = 0;
    UA_GDSTransaction_clear(&gdsManager->transaction);
    UA_FileInfoContext *fileInfoContext = (UA_FileInfoContext*)
        gdsManager->fileInfoContext;

    /* free all fileInfoContexts */
    while(fileInfoContext) {
        UA_FileInfoContext *next = fileInfoContext->next;
        UA_FileInfo *fileInfo = &(fileInfoContext->fileInfo);
        UA_FileContext *fileContext = NULL;
        UA_FileContext *fileContextTmp = NULL;

        /* free all fileContexts in this fileInfoContext */
        LIST_FOREACH_SAFE(fileContext, &fileInfo->fileContext,
                          listEntry, fileContextTmp) {
            UA_ByteString_clear(&(fileContext->file));
            UA_ByteString_clear(&(fileContext->dataToWrite));
            LIST_REMOVE(fileContext, listEntry);
            UA_free(fileContext);
        }

        UA_free(fileInfoContext);
        fileInfoContext = next;
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

    return &gdsm->drv;
}

#endif
