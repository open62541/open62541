/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "ua_gds_push.h"
#include <open62541/plugin/nodestore.h>

#ifdef UA_ENABLE_GDS_PUSHMANAGEMENT

UA_GDSManager *
gdsManager(UA_Server *server) {
    UA_Driver *drv = UA_Server_getDrivers(server);
    while(drv && drv->start != UA_GDSManager_start)
        drv = drv->next;
    return (UA_GDSManager *)drv;
}

UA_CertificateGroup*
getCertGroup(UA_Server *server, const UA_NodeId *objectId) {
    UA_ServerConfig *sc = UA_Server_getConfig(server);

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
        return &sc->secureChannelPKI;
    }

    if(UA_NodeId_equal(objectId, &defaultUserTokenGroup) ||
       UA_NodeId_equal(objectId, &defaultUserTokenTrustList)) {
        return &sc->sessionPKI;
    }

    return NULL;
}

static UA_StatusCode
writeGDSNs0VariableArray(UA_Server *server, const UA_NodeId id, void *v,
                         size_t length, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return UA_Server_writeValue(server, id, var);
}

static UA_StatusCode
writeGDSNs0Variable(UA_Server *server, const UA_NodeId id,
                    void *v, const UA_DataType *type) {
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, v, type);
    return UA_Server_writeValue(server, id, var);
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
createFileInfos(UA_Server *server) {
    /* The server currently only supports the DefaultApplicationGroup and
     * UserTokenGroup */
    UA_UtcTime lastUpdateTime = UA_DateTime_now();

    /* Allocate two linked-list entries */
    UA_FileInfo *fi = (UA_FileInfo*)UA_calloc(1, sizeof(UA_FileInfo));
    if(!fi)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_FileInfo *fi2 = (UA_FileInfo*)UA_calloc(1, sizeof(UA_FileInfo));
    if(!fi2) {
        UA_free(fi);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    fi->next = fi2;

    fi->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    LIST_INIT(&fi->fileContext);
    fi->lastUpdateTime = lastUpdateTime;

    fi2->certificateGroupId =
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);
    LIST_INIT(&fi2->fileContext);
    fi2->lastUpdateTime = lastUpdateTime;

    gdsManager(server)->fileInfos = fi;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeGroupVariables(UA_Server *server) {
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_NodeId certificateTypes[2] = {
        UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE)
    };
    size_t certificateTypesSize = 2;

    UA_String supportedPrivateKeyFormats[2] =
        {UA_STRING("PEM"), UA_STRING("DER")};
    size_t supportedPrivateKeyFormatsSize = 2;
    UA_UInt32 maxTrustListSize = config->maxTrustListSize;

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

    /* Get the type of the called object */
    UA_NodeId typeId;
    UA_StatusCode retval = UA_Server_getNodeType(server, *objectId, &typeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&typeId, &trustListType)) {
        UA_GDSManager *gdsm = gdsManager(server);
        retval = UA_GDSManager_openTrustList(gdsm, certGroup, sessionId,
                                             fileOpenMode, output);
    } else {
        UA_ServerConfig *sc = UA_Server_getConfig(server);
        retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NodeId_clear(&typeId);
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

    /* Get the type of the called object */
    UA_NodeId typeId;
    UA_StatusCode res = UA_Server_getNodeType(server, *objectId, &typeId);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&typeId, &trustListType)) {
        /* Method was called on a trustlist */
        UA_GDSManager *gdsm = gdsManager(server);
        res = UA_GDSManager_readTrustList(gdsm, certGroup, sessionId,
                                          fileHandle, length, output);
    } else {
        UA_ServerConfig *sc = UA_Server_getConfig(server);
        res = UA_STATUSCODE_BADNOTIMPLEMENTED;
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NodeId_clear(&typeId);
    return res;
}

static UA_StatusCode
writeFileAction(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionHandle,
                const UA_NodeId *methodId, void *methodContext,
                const UA_NodeId *objectId, void *objectContext,
                size_t inputSize, const UA_Variant *input,
                size_t outputSize, UA_Variant *output) {
    UA_ServerConfig *sc = UA_Server_getConfig(server);

    /* Check input */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /* FileHandle */
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BYTESTRING])) /* Data */
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_ByteString data = *(UA_ByteString*)input[1].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Get the type of the called object */
    UA_NodeId typeId;
    UA_StatusCode retval = UA_Server_getNodeType(server, *objectId, &typeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&typeId, &trustListType)) {
        UA_GDSManager *gdsm = gdsManager(server);
        retval = UA_GDSManager_writeTrustList(gdsm, certGroup, sessionId,
                                              fileHandle, data);
    } else {
        retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NodeId_clear(&typeId);
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

    /* Get the type of the called object */
    UA_NodeId typeId;
    UA_StatusCode retval = UA_Server_getNodeType(server, *objectId, &typeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&typeId, &trustListType)) {
        UA_GDSManager *gdsm = gdsManager(server);
        retval = UA_GDSManager_closeTrustList(gdsm, certGroup, sessionId, fileHandle);
    } else {
        UA_ServerConfig *sc = UA_Server_getConfig(server);
        retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NodeId_clear(&typeId);
    return retval;
}

static UA_StatusCode
getPositionFileAction(UA_Server *server,
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

    /* Get the type of the called object */
    UA_NodeId typeId;
    UA_StatusCode retval = UA_Server_getNodeType(server, *objectId, &typeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&typeId, &trustListType)) {
        UA_GDSManager *gdsm = gdsManager(server);
        retval = UA_GDSManager_getPositionTrustList(gdsm, certGroup, sessionId,
                                                    fileHandle, output);
    } else {
        UA_ServerConfig *sc = UA_Server_getConfig(server);
        retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NodeId_clear(&typeId);
    return retval;
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

    /* Get the type of the called object */
    UA_NodeId typeId;
    UA_StatusCode retval = UA_Server_getNodeType(server, *objectId, &typeId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    static UA_NodeId trustListType = STATIC_NS0ID(TRUSTLISTTYPE);
    if(UA_NodeId_equal(&typeId, &trustListType)) {
        UA_GDSManager *gdsm = gdsManager(server);
        retval = UA_GDSManager_setPositionTrustList(gdsm, certGroup,
                                                    sessionId, fileHandle, position);
    } else {
        UA_ServerConfig *sc = UA_Server_getConfig(server);
        retval = UA_STATUSCODE_BADNOTIMPLEMENTED;
        UA_LOG_ERROR(sc->logging, UA_LOGCATEGORY_SERVER,
                     "File type functions are currently only supported "
                     "for TrustList types");
    }

    UA_NodeId_clear(&typeId);
    return retval;
}

UA_StatusCode
initNS0PushManagement(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Create FileInfo */
    retval |= createFileInfos(server);

    /* Set variables */
    retval |= writeGroupVariables(server);

    /* Set method callbacks */
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_UPDATECERTIFICATE), updateCertificateAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CREATESIGNINGREQUEST), createSigningRequestAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_GETREJECTEDLIST), getRejectedListAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_APPLYCHANGES), applyChangesAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificateAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificateAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE), removeCertificateAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_REMOVECERTIFICATE), removeCertificateAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMaskAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMaskAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustListAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustListAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPEN), openFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPEN), openFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ), readFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_READ), readFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_WRITE), writeFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_WRITE), writeFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSE), closeFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSE), closeFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_GETPOSITION), getPositionFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_GETPOSITION), getPositionFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SETPOSITION), setPositionFileAction);
    retval |= UA_Server_setMethodNodeCallback(server, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_SETPOSITION), setPositionFileAction);
    return retval;
}

#endif
