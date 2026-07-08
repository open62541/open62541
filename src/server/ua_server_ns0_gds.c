/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_GDS_PUSHMANAGEMENT

static UA_GDSManager *
gdsManager(UA_Server *server) {
    return (UA_GDSManager*)server->gdsPushReceiveDriver;
}

UA_FileContext*
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

UA_CertificateGroup*
getCertGroup(UA_Server *server, const UA_NodeId *objectId) {
    static UA_NodeId defaultApplicationTrustList =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    static UA_NodeId defaultUserTokenTrustList =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST);
    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    static UA_NodeId defaultUserTokenGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

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

UA_StatusCode
writeOpenCountVariable(UA_Server *server, UA_CertificateGroup *group) {
    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    static UA_NodeId defaultUserTokenGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsManager(server), group->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultApplicationGroup)) {
        static UA_NodeId appGroupOpenCount =
            STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENCOUNT);
        return writeGDSNs0Variable(server, appGroupOpenCount, &fileInfo->openCount,
                                   &UA_TYPES[UA_TYPES_UINT16]);
    }

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultUserTokenGroup)) {
        static UA_NodeId tokenGroupOpenCount =
            STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENCOUNT);
        return writeGDSNs0Variable(server, tokenGroupOpenCount, &fileInfo->openCount,
                                   &UA_TYPES[UA_TYPES_UINT16]);
    }

    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

UA_StatusCode
writeLastUpdateVariable(UA_Server *server, UA_CertificateGroup *group) {
    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    static UA_NodeId defaultUserTokenGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsManager(server), group->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultApplicationGroup)) {
        static UA_NodeId appGroupLastUpdate =
            STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_LASTUPDATETIME);
        return writeGDSNs0Variable(server, appGroupLastUpdate, &fileInfo->lastUpdateTime,
                                   &UA_TYPES[UA_TYPES_UTCTIME]);
    }

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultUserTokenGroup)) {
        static UA_NodeId tokenGroupLastUpdate =
            STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_LASTUPDATETIME);
        return writeGDSNs0Variable(server, tokenGroupLastUpdate, &fileInfo->lastUpdateTime,
                                   &UA_TYPES[UA_TYPES_UTCTIME]);
    }

    return UA_STATUSCODE_BADINVALIDARGUMENT;
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

    UA_GDSManager *gdsm = gdsManager(server);
    UA_FileInfo *fileInfo =
        UA_GDSManager_getFileInfo(gdsm, certGroup->certificateGroupId);
    if(!fileInfo)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);
    if(!fileContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    return UA_Variant_setScalarCopy(output, &fileContext->currentPos, &UA_TYPES[UA_TYPES_UINT64]);
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

    fileInfoContext->certificateGroupId = UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    LIST_INIT(&fileInfoContext->fileInfo.fileContext);
    fileInfoContext->fileInfo.lastUpdateTime = lastUpdateTime;

    fileInfoContext->next->certificateGroupId = UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
    LIST_INIT(&fileInfoContext->next->fileInfo.fileContext);
    fileInfoContext->next->fileInfo.lastUpdateTime = lastUpdateTime;

    gdsManager(server)->fileInfoContext = fileInfoContext;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeGroupVariables(UA_Server *server) {
    UA_NodeId certificateTypes[2] = {
        UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE)
    };
    size_t certificateTypesSize = 2;

    UA_String supportedPrivateKeyFormats[2] =
        {UA_STRING("PEM"), UA_STRING("DER")};
    size_t supportedPrivateKeyFormatsSize = 2;
    UA_UInt32  maxTrustListSize = 0;

    /* Set variables */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= writeGDSNs0VariableArray(server, UA_NS0ID(SERVERCONFIGURATION_SUPPORTEDPRIVATEKEYFORMATS),
                                       supportedPrivateKeyFormats, supportedPrivateKeyFormatsSize,
                                       &UA_TYPES[UA_TYPES_STRING]);

    retval |= writeGDSNs0Variable(server, UA_NS0ID(SERVERCONFIGURATION_MAXTRUSTLISTSIZE),
                                  &maxTrustListSize, &UA_TYPES[UA_TYPES_UINT32]);

    retval |= writeGDSNs0VariableArray(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_CERTIFICATETYPES),
                                       certificateTypes, certificateTypesSize,
                                       &UA_TYPES[UA_TYPES_NODEID]);

    /* DefaultApplicationGroup */
    UA_FileInfo *fileInfoApplicationGroup =
        UA_GDSManager_getFileInfo(gdsManager(server),
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP));
    if(!fileInfoApplicationGroup)
        return UA_STATUSCODE_BADINTERNALERROR;

    retval |= writeGDSNs0Variable(server,
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENCOUNT),
                                  &fileInfoApplicationGroup->openCount, &UA_TYPES[UA_TYPES_UINT16]);
    retval |= writeGDSNs0Variable(server,
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_LASTUPDATETIME),
                                  &fileInfoApplicationGroup->lastUpdateTime, &UA_TYPES[UA_TYPES_UTCTIME]);

    /* DefaultUserTokenGroup */
    UA_FileInfo *fileInfoUserTokenGroup =
        UA_GDSManager_getFileInfo(gdsManager(server),
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP));
    if(!fileInfoUserTokenGroup)
        return UA_STATUSCODE_BADINTERNALERROR;

    retval |= writeGDSNs0Variable(server,
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENCOUNT),
                                  &fileInfoUserTokenGroup->openCount, &UA_TYPES[UA_TYPES_UINT16]);
    retval |= writeGDSNs0Variable(server,
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_LASTUPDATETIME),
                                  &fileInfoUserTokenGroup->lastUpdateTime, &UA_TYPES[UA_TYPES_UTCTIME]);

    return retval;
}

static UA_StatusCode
updateCertificateAction(UA_Server *server,
                        const UA_NodeId *sessionId, void *sessionHandle,
                        const UA_NodeId *methodId, void *methodContext,
                        const UA_NodeId *objectId, void *objectContext,
                        size_t inputSize, const UA_Variant *input,
                        size_t outputSize, UA_Variant *output) {
    /* Check for input types */
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

    UA_StatusCode res =
        UA_GDSManager_updateCertificate(gdsManager(server),
                                        sessionId, certificateGroupId,
                                        certificateTypeId, certificate,
                                        privateKeyFormat, privateKey);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Output arg, indicates that the ApplyChanges Method shall be called before
     * the new Certificate will be used. */
    UA_Boolean applyChangesRequired = true;
    return UA_Variant_setScalarCopy(output, &applyChangesRequired,
                                    &UA_TYPES[UA_TYPES_BOOLEAN]);
}

static UA_StatusCode
createSigningRequestAction(UA_Server *server,
                           const UA_NodeId *sessionId, void *sessionHandle,
                           const UA_NodeId *methodId, void *methodContext,
                           const UA_NodeId *objectId, void *objectContext,
                           size_t inputSize, const UA_Variant *input,
                           size_t outputSize, UA_Variant *output) {
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
    UA_Variant_setScalar(output, csr, &UA_TYPES[UA_TYPES_BYTESTRING]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getRejectedListAction(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionHandle,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    UA_GDSManager *gdsm = gdsManager(server);
    return UA_GDSManager_getRejectedList(gdsm, outputSize, output);
}

static UA_StatusCode
applyChangesAction(UA_Server *server,
                   const UA_NodeId *sessionId, void *sessionHandle,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    UA_GDSManager *gdsm = gdsManager(server);
    UA_GDSTransaction *transaction = &gdsm->transaction;

    /* Check that the current transaction belongs to the session */
    if(!UA_NodeId_equal(&transaction->sessionId, sessionId))
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    /* Special non-good statuscode only for the public method */
    if(transaction->state == UA_GDSTRANSACTIONSTATE_FRESH)
        return UA_STATUSCODE_BADNOTHINGTODO;

    /* Do it */
    return UA_GDSManager_applyChanges(gdsm);
}

static UA_StatusCode
addCertificateAction(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionHandle,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    /* Check input types */
    if(inputSize != 2 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTESTRING]) || /* Certificate */
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN]))      /* IsTrustedCertificate */
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_ByteString *certificate = (UA_ByteString *)input[0].data;
    UA_Boolean *isTrustedCertificate = (UA_Boolean *)input[1].data;

    if(!*isTrustedCertificate || certificate->length == 0)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    UA_GDSManager *gdsm = gdsManager(server);
    if(gdsm->transaction.state != UA_GDSTRANSACTIONSTATE_FRESH)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    return UA_GDSManager_addCertificate(gdsm, certGroup, certificate,
                                        isTrustedCertificate);
}

static UA_StatusCode
removeCertificateAction(UA_Server *server,
                        const UA_NodeId *sessionId, void *sessionHandle,
                        const UA_NodeId *methodId, void *methodContext,
                        const UA_NodeId *objectId, void *objectContext,
                        size_t inputSize, const UA_Variant *input,
                        size_t outputSize, UA_Variant *output) {
    /* Check input types */
    if(inputSize != 2 ||
       !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) || /* Thumbprint */
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN]))  /* IsTrustedCertificate */
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_String *thumbprint = (UA_String *)input[0].data;
    UA_Boolean *isTrustedCertificate = (UA_Boolean *)input[1].data;

    UA_GDSManager *gdsm = gdsManager(server);
    UA_GDSTransaction *transaction = &gdsm->transaction;
    if(transaction->state != UA_GDSTRANSACTIONSTATE_FRESH)
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    return UA_GDSManager_removeCertificate(gdsm, certGroup, sessionId,
                                           thumbprint, isTrustedCertificate);
}

static UA_StatusCode
openTrustListWithMaskAction(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionHandle,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    /* Check input */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /* Mask */
        return UA_STATUSCODE_BADTYPEMISMATCH;
    UA_UInt32 mask = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsm = gdsManager(server);
    return UA_GDSManager_openTrustListWithMask(gdsm, certGroup,
                                               sessionId, mask, output);
}

static UA_StatusCode
closeAndUpdateTrustListAction(UA_Server *server,
                              const UA_NodeId *sessionId, void *sessionHandle,
                              const UA_NodeId *methodId, void *methodContext,
                              const UA_NodeId *objectId, void *objectContext,
                              size_t inputSize, const UA_Variant *input,
                              size_t outputSize, UA_Variant *output) {
    /* Check input */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /* FileHandle */
        return UA_STATUSCODE_BADTYPEMISMATCH;
    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_GDSManager *gdsm = gdsManager(server);
    return UA_GDSManager_closeAndUpdateTrustList(gdsm, certGroup, sessionId,
                                                 fileHandle, output);
}

static UA_StatusCode
openFileAction(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    /* Check input */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTE])) /* FileMode */
        return UA_STATUSCODE_BADTYPEMISMATCH;
    UA_Byte fileOpenMode = *(UA_Byte*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    if(!object)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const UA_Node *objectType =
        getNodeType(server, &object->head, ~(UA_UInt32)0,
                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    if(!objectType) {
        UA_NODESTORE_RELEASE(server, object);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_GDSManager *gdsm = gdsManager(server);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = UA_GDSManager_openTrustList(gdsm, certGroup, sessionId,
                                             fileOpenMode, output);
    } else {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);
    return retval;
}

static UA_StatusCode
readFileAction(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    /* Check inputs */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /* FileHandle */
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_INT32]))    /* Length */
        return UA_STATUSCODE_BADTYPEMISMATCH;
    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_Int32 length = *(UA_Int32*)input[1].data;
    if(length < 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Get the certgroup */
    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Get object type */
    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    if(!object)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const UA_Node *objectType =
        getNodeType(server, &object->head, ~(UA_UInt32)0,
                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    if(!objectType) {
        UA_NODESTORE_RELEASE(server, object);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode res;
    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        /* Method was called on a trustlist */
        UA_GDSManager *gdsm = gdsManager(server);
        res = UA_GDSManager_readTrustList(gdsm, certGroup, sessionId,
                                          fileHandle, length, output);
    } else {
        res = UA_STATUSCODE_BADNOTIMPLEMENTED;
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported for TrustList types");
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);
    return res;
}

static UA_StatusCode
writeFileAction(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionHandle,
                const UA_NodeId *methodId, void *methodContext,
                const UA_NodeId *objectId, void *objectContext,
                size_t inputSize, const UA_Variant *input,
                size_t outputSize, UA_Variant *output) {
    /* Check input */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /* FileHandle */
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BYTESTRING])) /* Data */
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_ByteString data = *(UA_ByteString*)input[1].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    if(!object)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const UA_Node *objectType =
        getNodeType(server, &object->head, ~(UA_UInt32)0,
                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    if(!objectType) {
        UA_NODESTORE_RELEASE(server, object);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_GDSManager *gdsm = gdsManager(server);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = UA_GDSManager_writeTrustList(gdsm, certGroup, sessionId,
                                              fileHandle, data);
    } else {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);
    return retval;
}

static UA_StatusCode
closeFileAction(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionHandle,
                const UA_NodeId *methodId, void *methodContext,
                const UA_NodeId *objectId, void *objectContext,
                size_t inputSize, const UA_Variant *input,
                size_t outputSize, UA_Variant *output) {
    /* Check input */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /* FileHandle */
        return UA_STATUSCODE_BADTYPEMISMATCH;
    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    if(!object)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const UA_Node *objectType =
        getNodeType(server, &object->head, ~(UA_UInt32)0,
                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    if(!objectType) {
        UA_NODESTORE_RELEASE(server, object);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_GDSManager *gdsm = gdsManager(server);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = UA_GDSManager_closeTrustList(gdsm, certGroup, sessionId, fileHandle);
    } else {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);
    return retval;
}

static UA_StatusCode
getPositionFileAction(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionHandle,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    if(!object)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const UA_Node *objectType =
        getNodeType(server, &object->head, ~(UA_UInt32)0,
                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);

    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
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
setPositionFileAction(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionHandle,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    /* Check input */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /* FileHandle */
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_UINT64]))   /* Position */
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_UInt64 position = *(UA_UInt32*)input[1].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    const UA_Node *object = UA_NODESTORE_GET(server, objectId);
    if(!object)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const UA_Node *objectType =
        getNodeType(server, &object->head, ~(UA_UInt32)0,
                    UA_REFERENCETYPESET_ALL, UA_BROWSEDIRECTION_BOTH);
    if(!objectType) {
        UA_NODESTORE_RELEASE(server, objectType);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_GDSManager *gdsm = gdsManager(server);
    UA_StatusCode retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        retval = UA_GDSManager_setPositionTrustList(gdsm, certGroup,
                                                    sessionId, fileHandle, position);
    } else {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NODESTORE_RELEASE(server, object);
    UA_NODESTORE_RELEASE(server, objectType);
    return retval;
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
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_UPDATECERTIFICATE), updateCertificateAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CREATESIGNINGREQUEST), createSigningRequestAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_GETREJECTEDLIST), getRejectedListAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_APPLYCHANGES), applyChangesAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificateAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificateAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE), removeCertificateAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_REMOVECERTIFICATE), removeCertificateAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMaskAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMaskAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustListAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustListAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPEN), openFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPEN), openFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ), readFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_READ), readFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_WRITE), writeFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_WRITE), writeFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSE), closeFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSE), closeFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_GETPOSITION), getPositionFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_GETPOSITION), getPositionFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SETPOSITION), setPositionFileAction);
    retval |= setMethodNode_callback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_SETPOSITION), setPositionFileAction);
    return retval;
}

#endif
