/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_GDS_PUSHMANAGEMENT

#define UA_SHA1_LENGTH 20
#define CHECKACTIVESESSIONINTERVAL 10000 /* 10sec */

typedef struct UA_FileContext {
    LIST_ENTRY(UA_FileContext) listEntry;
    UA_ByteString file;
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


static UA_StatusCode applyChangesToServer(UA_Server *server);

static UA_FileInfo*
getFileInfo(UA_GDSManager *gdsManager, UA_NodeId certificateGroupId) {
    UA_FileInfoContext *fileInfoContext = (UA_FileInfoContext*)gdsManager->fileInfoContext;
    while(fileInfoContext) {
        if(UA_NodeId_equal(&fileInfoContext->certificateGroupId, &certificateGroupId))
            return &fileInfoContext->fileInfo;
        fileInfoContext = fileInfoContext->next;
    }
    return NULL;
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

static UA_FileContext*
getFileContext(UA_FileInfo *fileInfo, const UA_NodeId *sessionId, const UA_UInt32 fileHandle) {
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

static UA_CertificateGroup*
getCertGroup(UA_Server *server, const UA_NodeId *objectId) {
    UA_NodeId defaultApplicationTrustList =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    UA_NodeId defaultUserTokenTrustList =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST);
    UA_NodeId defaultApplicationGroup =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId defaultUserTokenGroup =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    if(UA_NodeId_equal(objectId, &defaultApplicationGroup) ||
       UA_NodeId_equal(objectId, &defaultApplicationTrustList)) {
        return &server->config.secureChannelPKI;
    }
    if(UA_NodeId_equal(objectId, &defaultUserTokenGroup) ||
       UA_NodeId_equal(objectId, &defaultUserTokenTrustList)) {
        return &server->config.sessionPKI;
    }

    return NULL;
}

static UA_StatusCode
writeGDSNs0VariableArray(UA_Server *server, const UA_NodeId id, void *v,
                         size_t length, const UA_DataType *type) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return writeValueAttribute(server, id, &var);
}

static UA_StatusCode
writeGDSNs0Variable(UA_Server *server, const UA_NodeId id,
                    void *v, const UA_DataType *type) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, v, type);
    return writeValueAttribute(server, id, &var);
}

static UA_StatusCode
writeOpenCountVariable(UA_Server *server, UA_CertificateGroup *group) {
    UA_NodeId defaultApplicationGroup =
    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId defaultUserTokenGroup =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    UA_FileInfo *fileInfo = getFileInfo(&server->gdsManager, group->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultApplicationGroup)) {
        return writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENCOUNT),
                           &fileInfo->openCount, &UA_TYPES[UA_TYPES_UINT16]);
    }
    if(UA_NodeId_equal(&group->certificateGroupId, &defaultUserTokenGroup)) {
        return writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENCOUNT),
                           &fileInfo->openCount, &UA_TYPES[UA_TYPES_UINT16]);
    }

    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

static UA_StatusCode
writeLastUpdateVariable(UA_Server *server, UA_CertificateGroup *group) {
    UA_NodeId defaultApplicationGroup =
    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    UA_NodeId defaultUserTokenGroup =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    UA_FileInfo *fileInfo = getFileInfo(&server->gdsManager, group->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultApplicationGroup)) {
        return writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_LASTUPDATETIME),
                                  &fileInfo->lastUpdateTime, &UA_TYPES[UA_TYPES_UTCTIME]);
    }
    if(UA_NodeId_equal(&group->certificateGroupId, &defaultUserTokenGroup)) {
        return writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_LASTUPDATETIME),
                                  &fileInfo->lastUpdateTime, &UA_TYPES[UA_TYPES_UTCTIME]);
    }

    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

/* This callback is triggered at regular intervals as long as a transaction is ongoing
 * or a client has a TrustList open for reading or writing.
 * If the session is no longer active, the current transaction is cancelled,
 * and all open file handles are closed. */
static void
checkSessionActive(UA_Server *server, void *data) {
    UA_LOCK(&server->serviceMutex);
    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_GDSTransaction *transaction = &gdsManager->transaction;
    UA_Boolean removingCallback = false;
    if(transaction->state != UA_GDSTRANSACIONSTATE_FRESH) {
        UA_Boolean foundSession = false;
        session_list_entry *session;
        LIST_FOREACH(session, &server->sessions, pointers) {
            if(UA_NodeId_equal(&session->session.sessionId, &transaction->sessionId)) {
                foundSession = true;
                break;
            }
        }
        if(!foundSession) {
            UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "Session with an open transaction has ended. "
                "The transaction has been discarded.");
            UA_GDSTransaction_clear(transaction);
            removingCallback = true;
        }
    } else {
        removingCallback = true;
    }

    UA_FileInfoContext *fileInfoContext = (UA_FileInfoContext*)gdsManager->fileInfoContext;
    while(fileInfoContext) {
        UA_FileInfo fileInfo = fileInfoContext->fileInfo;

        if(fileInfo.openCount == 0) {
            fileInfoContext = fileInfoContext->next;
            continue;
        }

        removingCallback = false;

        UA_CertificateGroup *certGroup = getCertGroup(server, &fileInfoContext->certificateGroupId);

        UA_FileContext *fileContext, *fileContextTmp;
        LIST_FOREACH_SAFE(fileContext, &fileInfo.fileContext, listEntry, fileContextTmp) {
            UA_Boolean foundSession = false;
            session_list_entry *session;
            LIST_FOREACH(session, &server->sessions, pointers) {
                if(UA_NodeId_equal(&session->session.sessionId, &fileContext->sessionId)) {
                    foundSession = true;
                    break;
                }
            }

            if(foundSession)
                continue;

            UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                    "Session with an open trust list has ended. "
                    "All file handlers for the open trust lists have been closed.");

            LIST_REMOVE(fileContext, listEntry);
            fileInfoContext->fileInfo.openCount -= 1;

            UA_ByteString_clear(&fileContext->file);
            UA_free(fileContext);

            /* Updating OpenCount Variable in the information model */
            writeOpenCountVariable(server, certGroup);
        }

        fileInfoContext = fileInfoContext->next;
    }

    if(removingCallback) {
        removeCallback(server, gdsManager->checkSessionCallbackId);
        gdsManager->checkSessionCallbackId = 0;
    }
    UA_UNLOCK(&server->serviceMutex);
}

static UA_StatusCode
updateCertificate(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]) || /*CertificateGroupId*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_NODEID]) || /*CertificateTypeId*/
       !UA_Variant_hasScalarType(&input[2], &UA_TYPES[UA_TYPES_BYTESTRING]) || /*Certificate*/
       !UA_Variant_hasArrayType(&input[3], &UA_TYPES[UA_TYPES_BYTESTRING]) || /*IssuerCertificates*/
       !UA_Variant_hasScalarType(&input[4], &UA_TYPES[UA_TYPES_STRING]) || /*PrivateKeyFormat*/
       !UA_Variant_hasScalarType(&input[5], &UA_TYPES[UA_TYPES_BYTESTRING])) /*PrivateKey*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_NodeId *certificateGroupId = (UA_NodeId *)input[0].data;
    UA_NodeId *certificateTypeId = (UA_NodeId *)input[1].data;
    UA_ByteString *certificate = (UA_ByteString *)input[2].data;
    /* TODO: Process issuer certificates */
    /* UA_ByteString *issuerCertificates = ((UA_ByteString *)input[3].data); */
    /* size_t issuerCertificatesSize = input[3].arrayLength; */
    UA_String *privateKeyFormat = (UA_String *)input[4].data;
    UA_ByteString *privateKey = (UA_ByteString *)input[5].data;

    /* The server currently only supports the DefaultApplicationGroup */
    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    if(!UA_NodeId_equal(certificateGroupId, &defaultApplicationGroup))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* The server currently only supports the following certificate type */
    UA_NodeId certTypRsaMin = UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE);
    UA_NodeId certTypRsaSha256 = UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE);
    if(!UA_NodeId_equal(certificateTypeId, &certTypRsaSha256) && !UA_NodeId_equal(certificateTypeId, &certTypRsaMin))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* Verify that the privateKey is in a supported format and
     * that it matches the specified certificate */
    if(privateKey) {
        const UA_String pemFormat = UA_STRING("PEM");
        const UA_String pfxFormat = UA_STRING("PFX");
        if(!UA_String_equal(&pemFormat, privateKeyFormat) && !UA_String_equal(&pfxFormat, privateKeyFormat))
            return UA_STATUSCODE_BADNOTSUPPORTED;
        if(UA_CertificateUtils_checkKeyPair(certificate, privateKey) != UA_STATUSCODE_GOOD)
            return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_GDSTransaction *transaction = &gdsManager->transaction;
    if(transaction->state == UA_GDSTRANSACIONSTATE_FRESH) {
        retval = UA_GDSTransaction_init(transaction, server, *sessionId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    if(gdsManager->checkSessionCallbackId == 0) {
        retval = addRepeatedCallback(server, (UA_ServerCallback)checkSessionActive, NULL,
                                     CHECKACTIVESESSIONINTERVAL, &gdsManager->checkSessionCallbackId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    if(!UA_NodeId_equal(&transaction->sessionId, sessionId))
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    retval = UA_GDSTransaction_addCertificateInfo(transaction, *certificateGroupId,
                                                  *certificateTypeId, certificate, privateKey);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Output arg, indicates that the ApplyChanges Method shall be called before the new Certificate will be used. */
    UA_Boolean applyChangesRequired = true;
    UA_Variant_setScalarCopy(output, &applyChangesRequired, &UA_TYPES[UA_TYPES_BOOLEAN]);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createSigningRequest(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionHandle,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_NODEID]) || /*CertificateGroupId*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_NODEID]) || /*CertificateTypeId*/
       !UA_Variant_hasScalarType(&input[2], &UA_TYPES[UA_TYPES_STRING]) || /*SubjectName*/
       !UA_Variant_hasScalarType(&input[3], &UA_TYPES[UA_TYPES_BOOLEAN]) || /*RegeneratePrivateKey*/
       !UA_Variant_hasScalarType(&input[4], &UA_TYPES[UA_TYPES_BYTESTRING]))  /*Nonce*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_NodeId *certificateGroupId = (UA_NodeId *)input[0].data;
    UA_NodeId *certificateTypeId = (UA_NodeId *)input[1].data;
    UA_String *subjectName = (UA_String *)input[2].data;
    UA_Boolean *regenerateKey = ((UA_Boolean *)input[3].data);
    UA_ByteString *nonce = (UA_ByteString *)input[4].data;
    UA_ByteString *csr = UA_ByteString_new();

    UA_StatusCode retval =
        UA_Server_createSigningRequest(server, *certificateGroupId,
                                       *certificateTypeId, subjectName,
                                       regenerateKey, nonce, csr);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_delete(csr);
        return retval;
    }

    /* Output arg, the PKCS #10 DER encoded Certificate Request (CSR) */
    UA_Variant_setScalarCopy(output, csr, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_ByteString_delete(csr);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getRejectedList(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionHandle,
                const UA_NodeId *methodId, void *methodContext,
                const UA_NodeId *objectId, void *objectContext,
                size_t inputSize, const UA_Variant *input,
                size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    size_t rejectedListSize = 0;
    UA_CertificateGroup certGroup = server->config.secureChannelPKI;

    /* DefaultApplicationGroup */
    UA_ByteString *rejectedListSecureChannel = NULL;
    size_t rejectedListSecureChannelSize = 0;
    certGroup.getRejectedList(&certGroup, &rejectedListSecureChannel, &rejectedListSecureChannelSize);
    rejectedListSize += rejectedListSecureChannelSize;

    /* DefaultUserTokenGroup */
    certGroup = server->config.sessionPKI;
    UA_ByteString *rejectedListSession = NULL;
    size_t rejectedListSessionSize = 0;
    certGroup.getRejectedList(&certGroup, &rejectedListSession, &rejectedListSessionSize);
    rejectedListSize += rejectedListSessionSize;

    if(rejectedListSize == 0) {
        UA_Variant_setArray(&output[0], NULL, rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
        return UA_STATUSCODE_GOOD;
    }

    UA_ByteString *rejectedList = (UA_ByteString*)UA_Array_new(rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    if(!rejectedList) {
        UA_Array_delete(rejectedListSecureChannel, rejectedListSecureChannelSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_Array_delete(rejectedListSession, rejectedListSessionSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    memcpy(rejectedList, rejectedListSecureChannel, rejectedListSecureChannelSize * sizeof(UA_ByteString));
    memcpy(rejectedList + rejectedListSecureChannelSize, rejectedListSession, rejectedListSessionSize * sizeof(UA_ByteString));

    UA_Variant_setArrayCopy(&output[0], rejectedList,
                        rejectedListSize, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_Array_delete(rejectedListSecureChannel, rejectedListSecureChannelSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Array_delete(rejectedListSession, rejectedListSessionSize, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_free(rejectedList);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addCertificate(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTESTRING]) || /*Certificate*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN])) /*IsTrustedCertificate*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_ByteString certificate = *(UA_ByteString *)input[0].data;
    UA_Boolean isTrustedCertificate = *(UA_Boolean *)input[1].data;

    if(!isTrustedCertificate || certificate.length == 0)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    UA_GDSManager *gdsManager = &server->gdsManager;
    if(gdsManager->transaction.state != UA_GDSTRANSACIONSTATE_FRESH)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Set up the parameters for the verification */
    UA_KeyValuePair params[1];
    size_t paramsSize = 1;

    /* CA certificates cannot be added using this method because it does not support adding CRLs */
    UA_Boolean isCaCertificate = false;
    params[0].key = UA_QUALIFIEDNAME(0, "is-ca-certificate");
    UA_Variant_setScalar(&params[0].value, &isCaCertificate, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = paramsSize;

    /* Validity check of the certificate */
    UA_StatusCode retval = certGroup->verifyCertificateValidity(certGroup, &certificate, &paramsMap);
    if(retval == UA_STATUSCODE_BADCERTIFICATEUSENOTALLOWED) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
            "The certificate could not be added because it is a CA certificate. "
            "CA certificates must be added using the FileType methods.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* This Method cannot be called if the containing TrustList Object is open */
    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(fileInfo->openCount > 0)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    UA_ByteString certificates[1];
    certificates[0] = certificate;

    trustList.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    trustList.trustedCertificates = certificates;
    trustList.trustedCertificatesSize = 1;

    retval = certGroup->addToTrustList(certGroup, &trustList);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Updating LastUpdateTime Variable in the information model */
    fileInfo->lastUpdateTime = UA_DateTime_now();
    writeLastUpdateVariable(server, certGroup);

    return retval;
}

static UA_StatusCode
removeCertificate(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionHandle,
                const UA_NodeId *methodId, void *methodContext,
                const UA_NodeId *objectId, void *objectContext,
                size_t inputSize, const UA_Variant *input,
                size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) || /*Thumbprint*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN])) /*IsTrustedCertificate*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_String thumbprint = *(UA_String *)input[0].data;
    UA_Boolean isTrustedCertificate = *(UA_Boolean *)input[1].data;

    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_GDSTransaction *transaction = &gdsManager->transaction;
    if(transaction->state != UA_GDSTRANSACIONSTATE_FRESH)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    /* When a certificate is removed, a transaction is created which is then executed directly.
     * No apply cahnges is required */
    UA_StatusCode retval = UA_GDSTransaction_init(transaction, server, *sessionId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* This Method cannot be called if the containing TrustList Object is open */
    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(fileInfo->openCount > 0)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    UA_ByteString *certificates;
    size_t certificatesSize = 0;
    certGroup->getTrustList(certGroup, &trustList);

    if(isTrustedCertificate) {
        certificates = trustList.trustedCertificates;
        certificatesSize = trustList.trustedCertificatesSize;
    } else {
        certificates = trustList.issuerCertificates;
        certificatesSize = trustList.issuerCertificatesSize;
    }

    UA_TrustListDataType list;
    memset(&list, 0, sizeof(UA_TrustListDataType));

    UA_ByteString *crls = NULL;
    size_t crlsSize = 0;

    UA_String thumbpr = UA_STRING_NULL;
    thumbpr.length = (UA_SHA1_LENGTH * 2);
    thumbpr.data = (UA_Byte*)malloc(sizeof(UA_Byte)*thumbpr.length);

    for(size_t i = 0; i < certificatesSize; i++) {
        UA_CertificateUtils_getThumbprint( &certificates[i], &thumbpr);
        /* Compare thumbprint */
        if(!UA_String_equal_ignorecase(&thumbprint, &thumbpr))
            continue;

        UA_ByteString certificate = certificates[i];
        retval = certGroup->getCertificateCrls(certGroup, &certificate, isTrustedCertificate,
                                               &crls, &crlsSize);
        if(retval != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }

        if(isTrustedCertificate) {
            list.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_TRUSTEDCRLS;
            list.trustedCertificates = &certificate;
            list.trustedCertificatesSize = 1;
            list.trustedCrls = crls;
            list.trustedCrlsSize = crlsSize;
        } else {
            list.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCRLS;
            list.issuerCertificates = &certificate;
            list.issuerCertificatesSize = 1;
            list.issuerCrls = crls;
            list.issuerCrlsSize = crlsSize;
        }
        break;
    }

    UA_CertificateGroup *transactionCertGroup =
        UA_GDSTransaction_getCertificateGroup(transaction, certGroup);
    if(!transactionCertGroup) {
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(list.specifiedLists != UA_TRUSTLISTMASKS_NONE) {
        retval = transactionCertGroup->removeFromTrustList(transactionCertGroup, &list);
        if(retval != UA_STATUSCODE_GOOD) {
            goto cleanup;
        }
    } else {
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER, "The certificate to remove was not found");
        retval = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto cleanup;
    }

    retval = applyChangesToServer(server);

cleanup:
    UA_String_clear(&thumbpr);
    UA_TrustListDataType_clear(&trustList);
    UA_Array_delete(crls, crlsSize, &UA_TYPES[UA_TYPES_BYTESTRING]);

    return retval;
}

static UA_StatusCode
openTrustList(UA_Server *server,
              const UA_NodeId *sessionId, void *sessionHandle,
              const UA_NodeId *methodId, void *methodContext,
              const UA_NodeId *objectId, void *objectContext,
              size_t inputSize, const UA_Variant *input,
              size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTE]))/*FileMode*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_Byte fileOpenMode = *(UA_Byte*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_GDSTransaction *transaction = &gdsManager->transaction;
    /* Cannot be opened when a transaction is running */
    if(transaction->state == UA_GDSTRANSACIONSTATE_PENDING)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
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

    if(gdsManager->checkSessionCallbackId == 0) {
        retval = addRepeatedCallback(server, (UA_ServerCallback)checkSessionActive, NULL,
                                     CHECKACTIVESESSIONINTERVAL, &gdsManager->checkSessionCallbackId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
    certGroup->getTrustList(certGroup, &trustList);

    UA_ByteString encTrustList = UA_BYTESTRING_NULL;
    retval = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], &encTrustList);
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
    retval = createFileHandleId(fileInfo, &fileContext->fileHandle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&fileContext->file);
        UA_free(fileContext);
        return retval;
    }

    fileInfo->openCount += 1;
    UA_Variant_setScalarCopy(output, &fileContext->fileHandle, &UA_TYPES[UA_TYPES_UINT32]);

    /* Updating OpenCount Variable in the information model */
    writeOpenCountVariable(server, certGroup);

    LIST_INSERT_HEAD(&fileInfo->fileContext, fileContext, listEntry);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
openTrustListWithMask(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionHandle,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /*Mask*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 mask = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsManager = &server->gdsManager;
    if(gdsManager->transaction.state == UA_GDSTRANSACIONSTATE_PENDING)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(gdsManager->checkSessionCallbackId == 0) {
        retval = addRepeatedCallback(server, (UA_ServerCallback)checkSessionActive, NULL,
                                     CHECKACTIVESESSIONINTERVAL, &gdsManager->checkSessionCallbackId);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    trustList.specifiedLists = mask;
    certGroup->getTrustList(certGroup, &trustList);

    UA_ByteString encTrustList = UA_BYTESTRING_NULL;
    retval = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], &encTrustList);
    UA_TrustListDataType_clear(&trustList);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&encTrustList);
        return retval;
    }

    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = (UA_FileContext*)malloc(sizeof(UA_FileContext));
    fileContext->file = encTrustList;
    fileContext->sessionId = *sessionId;
    fileContext->openFileMode = UA_OPENFILEMODE_READ;
    fileContext->currentPos = 0;
    retval = createFileHandleId(fileInfo, &fileContext->fileHandle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&fileContext->file);
        UA_free(fileContext);
        return retval;
    }

    fileInfo->openCount += 1;
    UA_Variant_setScalarCopy(output, &fileContext->fileHandle, &UA_TYPES[UA_TYPES_UINT32]);

    /* Updating OpenCount Variable in the information model */
    writeOpenCountVariable(server, certGroup);

    LIST_INSERT_HEAD(&fileInfo->fileContext, fileContext, listEntry);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readTrustList(UA_Server *server,
              const UA_NodeId *sessionId, void *sessionHandle,
              const UA_NodeId *methodId, void *methodContext,
              const UA_NodeId *objectId, void *objectContext,
              size_t inputSize, const UA_Variant *input,
              size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /*FileHandle*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_INT32])) /*Length*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_Int32 length = *(UA_Int32*)input[1].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
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
        readBuffer.length = length;
        readBuffer.data = fileContext->file.data+fileContext->currentPos;
        fileContext->currentPos += length;
    }

    UA_Variant_setScalarCopy(output, &readBuffer, &UA_TYPES[UA_TYPES_BYTESTRING]);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeTrustList(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /*FileHandle*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BYTESTRING])) /*Data*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_ByteString data = *(UA_ByteString*)input[1].data;

    if(data.length == 0)
        return UA_STATUSCODE_GOOD;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_GDSTransaction *transaction = &gdsManager->transaction;
    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(fileContext->openFileMode != (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
     UA_StatusCode retval =
         UA_decodeBinary(&data, &trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], NULL);

    if(retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_CertificateGroup *transactionCertGroup =
        UA_GDSTransaction_getCertificateGroup(transaction, certGroup);
    if(!transactionCertGroup)
        return UA_STATUSCODE_BADINTERNALERROR;

    return transactionCertGroup->setTrustList(transactionCertGroup, &trustList);
}

static UA_StatusCode
closeTrustList(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /*FileHandle*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_GDSTransaction *transaction = &gdsManager->transaction;
    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* If a close is called, a current transaction is cancelled.
     * If the list was opened in read mode, there are no changes to discard. */
    if(fileContext->openFileMode == (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING)) {
        UA_GDSTransaction_clear(transaction);
    }

    LIST_REMOVE(fileContext, listEntry);
    fileInfo->openCount -= 1;

    UA_ByteString_clear(&fileContext->file);
    UA_free(fileContext);

    /* Updating OpenCount Variable in the information model */
    writeOpenCountVariable(server, certGroup);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
closeAndUpdateTrustList(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /*FileHandle*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    LIST_REMOVE(fileContext, listEntry);
    fileInfo->openCount -= 1;

    UA_ByteString_clear(&fileContext->file);
    UA_free(fileContext);

    /* Updating OpenCount Variable in the information model */
    writeOpenCountVariable(server, certGroup);

    /* Output arg, indicates that the ApplyChanges Method shall be called before the new trust list will be used. */
    UA_Boolean applyChangesRequired = true;
    UA_Variant_setScalarCopy(output, &applyChangesRequired, &UA_TYPES[UA_TYPES_BOOLEAN]);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getPositionTrustList(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionHandle,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /*FileHandle*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_Variant_setScalarCopy(output, &fileContext->currentPos, &UA_TYPES[UA_TYPES_UINT64]);
}

static UA_StatusCode
setPositionTrustList(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionHandle,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /*FileHandle*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_UINT64])) /*Position*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_UInt64 position = *(UA_UInt32*)input[1].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
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

typedef enum UA_GDSTransactionChanges {
    UA_GDSTRANSACTIONCHANGES_NOTHING = 0,
    UA_GDSTRANSACTIONCHANGES_TRUSTLIST,
    UA_GDSTRANSACTIONCHANGES_CERTIFICATE,
    UA_GDSTRANSACTIONCHANGES_BOTH,
} UA_GDSTransactionChanges;

static void
secureChannel_delayedClose(void *application, void *context) {
    UA_Server *server = (UA_Server*)context;
    UA_GDSTransactionChanges *changes = (UA_GDSTransactionChanges*)application;

    if(*changes == UA_GDSTRANSACTIONCHANGES_NOTHING)
        goto cleanup;

    if(*changes == UA_GDSTRANSACTIONCHANGES_BOTH ||
       *changes == UA_GDSTRANSACTIONCHANGES_CERTIFICATE ) {
        UA_SecureChannel *channel;
        TAILQ_FOREACH(channel, &server->channels, serverEntry) {
            if(channel->state == UA_SECURECHANNELSTATE_CLOSED || channel->state == UA_SECURECHANNELSTATE_CLOSING)
                continue;
            UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_CLOSE);
        }
        goto cleanup;
    }

    /* Set up the parameters for the verification */
    UA_KeyValuePair params[1];
    size_t paramsSize = 1;

    /* Cannot be a CA certificate */
    UA_Boolean isCaCertificate = false;
    params[0].key = UA_QUALIFIEDNAME(0, "is-ca-certificate");
    UA_Variant_setScalar(&params[0].value, &isCaCertificate, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_KeyValueMap paramsMap;
    paramsMap.map = params;
    paramsMap.mapSize = paramsSize;

    UA_CertificateGroup certGroup = server->config.secureChannelPKI;
    UA_SecureChannel *channel;
    TAILQ_FOREACH(channel, &server->channels, serverEntry) {
        if(channel->state == UA_SECURECHANNELSTATE_CLOSED || channel->state == UA_SECURECHANNELSTATE_CLOSING)
            continue;
        if(certGroup.verifyCertificateTrust(&certGroup, &channel->remoteCertificate,
                                            &paramsMap) != UA_STATUSCODE_GOOD)
            UA_SecureChannel_shutdown(channel, UA_SHUTDOWNREASON_CLOSE);
    }

cleanup:
    UA_free(changes);
    UA_GDSTransaction_clear(&server->gdsManager.transaction);
}

static UA_StatusCode
applyChangesToServer(UA_Server *server) {
    if(!server)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_GDSTransaction *transaction = &gdsManager->transaction;
    UA_GDSTransactionChanges *changes = (UA_GDSTransactionChanges*)UA_calloc(1, sizeof(UA_GDSTransactionChanges));

    /* Apply Trust list changes */
    for(size_t i = 0; i < transaction->certGroupSize; i++) {
        *changes = UA_GDSTRANSACTIONCHANGES_TRUSTLIST;
        UA_CertificateGroup transactionCertGroup = transaction->certGroups[i];
        UA_TrustListDataType trustList;
        UA_TrustListDataType_init(&trustList);
        trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;
        transactionCertGroup.getTrustList(&transactionCertGroup, &trustList);

        UA_CertificateGroup *certGroup = getCertGroup(server, &transactionCertGroup.certificateGroupId);
        if(!certGroup) {
            UA_TrustListDataType_clear(&trustList);
            goto cleanup;
        }
        retval = certGroup->setTrustList(certGroup, &trustList);
        UA_TrustListDataType_clear(&trustList);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;

        UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
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

        for(size_t i = 0; i < server->config.endpointsSize; i++) {
            UA_EndpointDescription *ed = &server->config.endpoints[i];
            UA_SecurityPolicy *sp = getSecurityPolicyByUri(server,
                                &server->config.endpoints[i].securityPolicyUri);
            UA_CHECK_MEM(sp, return UA_STATUSCODE_BADINTERNALERROR);

            if(!UA_NodeId_equal(&sp->certificateTypeId, &certTypeId))
                continue;

            retval = sp->updateCertificateAndPrivateKey(sp, certificate, privateKey);
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;

            UA_ByteString_clear(&ed->serverCertificate);
            retval = UA_ByteString_copy(&certificate, &ed->serverCertificate);
            if(retval != UA_STATUSCODE_GOOD)
                goto cleanup;
        }
    }

    /* Add to the delayed callback list. Will be cleaned up in the next iteration. */
    UA_DelayedCallback *dc = &transaction->dc;
    dc->callback = secureChannel_delayedClose;
    dc->application = changes;
    dc->context = server;

    UA_EventLoop *el = server->config.eventLoop;
    el->addDelayedCallback(el, dc);

    return UA_STATUSCODE_GOOD;

cleanup:
    UA_GDSTransaction_clear(transaction);
    UA_free(changes);

    return retval;
}

static UA_StatusCode
applyChanges(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionHandle,
             const UA_NodeId *methodId, void *methodContext,
             const UA_NodeId *objectId, void *objectContext,
             size_t inputSize, const UA_Variant *input,
             size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_GDSManager *gdsManager = &server->gdsManager;
    UA_GDSTransaction *transaction = &gdsManager->transaction;
    if(transaction->state == UA_GDSTRANSACIONSTATE_FRESH)
        return UA_STATUSCODE_BADNOTHINGTODO;

    if(!UA_NodeId_equal(&transaction->sessionId, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    /* Check if a TrustList is still open */
    for(size_t i = 0; i < transaction->certGroupSize; i++) {
        UA_CertificateGroup *certGroup = &transaction->certGroups[i];
        UA_FileInfo *fileInfo = getFileInfo(gdsManager, certGroup->certificateGroupId);
        if(!fileInfo)
            return UA_STATUSCODE_BADINTERNALERROR;
        if(fileInfo->openCount > 0)
            return UA_STATUSCODE_BADINVALIDSTATE;
    }

    return applyChangesToServer(server);
}

static UA_StatusCode
createFileInfoContexts(UA_Server *server) {
    /* The server currently only supports the DefaultApplicationGroup and UserTokenGroup */
    UA_UtcTime lastUpdateTime = UA_DateTime_now();

    UA_FileInfoContext *fileInfoContext = (UA_FileInfoContext*)UA_calloc(1, sizeof(UA_FileInfoContext));
    if(!fileInfoContext)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    fileInfoContext->next = (UA_FileInfoContext*)UA_calloc(1, sizeof(UA_FileInfoContext));
    if(!fileInfoContext->next) {
        UA_free(fileInfoContext);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    fileInfoContext->certificateGroupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    LIST_INIT(&fileInfoContext->fileInfo.fileContext);
    fileInfoContext->fileInfo.lastUpdateTime = lastUpdateTime;

    fileInfoContext->next->certificateGroupId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
    LIST_INIT(&fileInfoContext->next->fileInfo.fileContext);
    fileInfoContext->next->fileInfo.lastUpdateTime = lastUpdateTime;

    server->gdsManager.fileInfoContext = fileInfoContext;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeGroupVariables(UA_Server *server) {
    UA_NodeId certificateTypes[2] = {UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE)};
    size_t certificateTypesSize = 2;

    UA_String supportedPrivateKeyFormats[2] = {UA_STRING("PEM"),
                                               UA_STRING("PFX")};
    size_t supportedPrivateKeyFormatsSize = 2;

    UA_UInt32  maxTrustListSize = 0;

    /* Set variables */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= writeGDSNs0VariableArray(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_SUPPORTEDPRIVATEKEYFORMATS),
                                       supportedPrivateKeyFormats, supportedPrivateKeyFormatsSize,
                                       &UA_TYPES[UA_TYPES_STRING]);

    retval |= writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_MAXTRUSTLISTSIZE),
                                  &maxTrustListSize, &UA_TYPES[UA_TYPES_UINT32]);

    retval |= writeGDSNs0VariableArray(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_CERTIFICATETYPES),
                                       certificateTypes, certificateTypesSize,
                                       &UA_TYPES[UA_TYPES_NODEID]);

    /* DefaultApplicationGroup */
    UA_FileInfo *fileInfoApplicationGroup = getFileInfo(&server->gdsManager, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP));
    if(!fileInfoApplicationGroup)
        return UA_STATUSCODE_BADINTERNALERROR;

    retval |= writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENCOUNT),
                                  &fileInfoApplicationGroup->openCount, &UA_TYPES[UA_TYPES_UINT16]);
    retval |= writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_LASTUPDATETIME),
                                  &fileInfoApplicationGroup->lastUpdateTime, &UA_TYPES[UA_TYPES_UTCTIME]);

    /* DefaultUserTokenGroup */
    UA_FileInfo *fileInfoUserTokenGroup = getFileInfo(&server->gdsManager, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP));
    if(!fileInfoUserTokenGroup)
        return UA_STATUSCODE_BADINTERNALERROR;

    retval |= writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENCOUNT),
                                  &fileInfoUserTokenGroup->openCount, &UA_TYPES[UA_TYPES_UINT16]);
    retval |= writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_LASTUPDATETIME),
                                  &fileInfoUserTokenGroup->lastUpdateTime, &UA_TYPES[UA_TYPES_UTCTIME]);

    return retval;
}

static UA_StatusCode
openFile(UA_Server *server,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    const UA_Node *objectType = getNodeType(server, &object->head);

    UA_NodeId trustListType = UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = openTrustList(server, sessionId, sessionHandle, methodId, methodContext,
                               objectId, objectContext, inputSize, input, outputSize, output);
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);

    if(retval != UA_STATUSCODE_BADNOTIMPLEMENTED)
        return retval;

    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "File type functions are currently only supported for TrustList types");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
readFile(UA_Server *server,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    const UA_Node *objectType = getNodeType(server, &object->head);

    UA_NodeId trustListType = UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval =  readTrustList(server, sessionId, sessionHandle, methodId, methodContext,
                                objectId, objectContext, inputSize, input, outputSize, output);
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);

    if(retval != UA_STATUSCODE_BADNOTIMPLEMENTED)
        return retval;

    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "File type functions are currently only supported for TrustList types");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
writeFile(UA_Server *server,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    const UA_Node *objectType = getNodeType(server, &object->head);

    UA_NodeId trustListType = UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = writeTrustList(server, sessionId, sessionHandle, methodId, methodContext,
                                objectId, objectContext, inputSize, input, outputSize, output);
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);

    if(retval != UA_STATUSCODE_BADNOTIMPLEMENTED)
        return retval;

    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "File type functions are currently only supported for TrustList types");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
closeFile(UA_Server *server,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    const UA_Node *objectType = getNodeType(server, &object->head);

    UA_NodeId trustListType = UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = closeTrustList(server, sessionId, sessionHandle, methodId, methodContext,
                                objectId, objectContext, inputSize, input, outputSize, output);
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);

    if(retval != UA_STATUSCODE_BADNOTIMPLEMENTED)
        return retval;

    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "File type functions are currently only supported for TrustList types");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
getPositionFile(UA_Server *server,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    const UA_Node *objectType = getNodeType(server, &object->head);

    UA_NodeId trustListType = UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = getPositionTrustList(server, sessionId, sessionHandle, methodId, methodContext,
                                      objectId, objectContext, inputSize, input, outputSize, output);
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);

    if(retval != UA_STATUSCODE_BADNOTIMPLEMENTED)
        return retval;

    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "File type functions are currently only supported for TrustList types");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
setPositionFile(UA_Server *server,
         const UA_NodeId *sessionId, void *sessionHandle,
         const UA_NodeId *methodId, void *methodContext,
         const UA_NodeId *objectId, void *objectContext,
         size_t inputSize, const UA_Variant *input,
         size_t outputSize, UA_Variant *output) {

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    const UA_Node *objectType = getNodeType(server, &object->head);

    UA_NodeId trustListType = UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = setPositionTrustList(server, sessionId, sessionHandle, methodId, methodContext,
                                      objectId, objectContext, inputSize, input, outputSize, output);
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);

    if(retval != UA_STATUSCODE_BADNOTIMPLEMENTED)
        return retval;

    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "File type functions are currently only supported for TrustList types");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode
updateCertificateAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = updateCertificate(server, sessionId, sessionHandle,
                                          methodId, methodContext,
                                          objectId, objectContext,
                                          inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
createSigningRequestAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = createSigningRequest(server, sessionId, sessionHandle,
                                             methodId, methodContext,
                                             objectId, objectContext,
                                             inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
getRejectedListAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = getRejectedList(server, sessionId, sessionHandle,
                                        methodId, methodContext,
                                        objectId, objectContext,
                                        inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
applyChangesAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = applyChanges(server, sessionId, sessionHandle,
                                     methodId, methodContext,
                                     objectId, objectContext,
                                     inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
addCertificateAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = addCertificate(server, sessionId, sessionHandle,
                                       methodId, methodContext,
                                       objectId, objectContext,
                                       inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
removeCertificateAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = removeCertificate(server, sessionId, sessionHandle,
                                          methodId, methodContext,
                                          objectId, objectContext,
                                          inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
openTrustListWithMaskAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = openTrustListWithMask(server, sessionId, sessionHandle,
                                              methodId, methodContext,
                                              objectId, objectContext,
                                              inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
closeAndUpdateTrustListAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = closeAndUpdateTrustList(server, sessionId, sessionHandle,
                                                methodId, methodContext,
                                                objectId, objectContext,
                                                inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
openFileAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = openFile(server, sessionId, sessionHandle,
                                 methodId, methodContext,
                                 objectId, objectContext,
                                 inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
readFileAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = readFile(server, sessionId, sessionHandle,
                                 methodId, methodContext,
                                 objectId, objectContext,
                                 inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
writeFileAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = writeFile(server, sessionId, sessionHandle,
                                  methodId, methodContext,
                                  objectId, objectContext,
                                  inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
closeFileAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = closeFile(server, sessionId, sessionHandle,
                                  methodId, methodContext,
                                  objectId, objectContext,
                                  inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
getPositionFileAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = getPositionFile(server, sessionId, sessionHandle,
                                        methodId, methodContext,
                                        objectId, objectContext,
                                        inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
setPositionFileAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = setPositionFile(server, sessionId, sessionHandle,
                                        methodId, methodContext,
                                        objectId, objectContext,
                                        inputSize, input, outputSize, output);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
initNS0PushManagement(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Create FileInfo */
    retval |= createFileInfoContexts(server);

    /* Set variables */
    retval |= writeGroupVariables(server);

    /* Set method callbacks */
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE), updateCertificateAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATIONTYPE_UPDATECERTIFICATE), updateCertificateAction);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST), createSigningRequestAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATIONTYPE_CREATESIGNINGREQUEST), createSigningRequestAction);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_GETREJECTEDLIST), getRejectedListAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATIONTYPE_GETREJECTEDLIST), getRejectedListAction);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES), applyChangesAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATIONTYPE_APPLYCHANGES), applyChangesAction);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificateAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificateAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE_ADDCERTIFICATE), addCertificateAction);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE), removeCertificateAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE), removeCertificateAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE_REMOVECERTIFICATE), removeCertificateAction);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMaskAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMaskAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE_OPENWITHMASKS), openTrustListWithMaskAction);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustListAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustListAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE_CLOSEANDUPDATE), closeAndUpdateTrustListAction);

    /* Open */
    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPEN),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN), true);

    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPEN),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN), true);

    /* Read */
    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_READ), true);

    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_READ),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_READ), true);

    /* Write */
    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_WRITE),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_WRITE), true);

    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_WRITE),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_WRITE), true);

    /* Close */
    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSE),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE), true);

    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSE),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE), true);

    /* GetPosition */
    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_GETPOSITION),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_GETPOSITION), true);

    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_GETPOSITION),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_GETPOSITION), true);

    /* SetPosition */
    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SETPOSITION),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_SETPOSITION), true);

    retval |= deleteNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_SETPOSITION),true);
    retval |= addRef(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_SETPOSITION), true);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN), openFileAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_READ), readFileAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_WRITE), writeFileAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE), closeFileAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_GETPOSITION), getPositionFileAction);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_SETPOSITION), setPositionFileAction);

    return retval;
}

#endif
