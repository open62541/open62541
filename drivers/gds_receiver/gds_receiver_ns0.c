/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "gds_receiver_internal.h"
#include <open62541/plugin/nodestore.h>

#ifdef UA_ENABLE_DRIVER_GDS_RECEIVER

#define STATIC_NS0ID(ID) {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_##ID}}

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
writeOpenCountVariable(UA_GDSReceiverContext *ctx, UA_CertificateGroup *group) {
    UA_Server *server = ((UA_GDSReceiver*)ctx)->drv.server;
    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    static UA_NodeId defaultUserTokenGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    UA_UInt16 openCount;
    UA_UtcTime lastUpdateTime;
    UA_StatusCode res = UA_GDSReceiver_getFileInfoMetadata(
        ctx, group->certificateGroupId, &openCount, &lastUpdateTime);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultApplicationGroup)) {
        static UA_NodeId appGroupOpenCount =
            STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENCOUNT);
        return writeGDSNs0Variable(server, appGroupOpenCount, &openCount,
                                   &UA_TYPES[UA_TYPES_UINT16]);
    }

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultUserTokenGroup)) {
        static UA_NodeId tokenGroupOpenCount =
            STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENCOUNT);
        return writeGDSNs0Variable(server, tokenGroupOpenCount, &openCount,
                                   &UA_TYPES[UA_TYPES_UINT16]);
    }

    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

UA_StatusCode
writeLastUpdateVariable(UA_GDSReceiverContext *ctx, UA_CertificateGroup *group) {
    UA_Server *server = ((UA_GDSReceiver*)ctx)->drv.server;
    static UA_NodeId defaultApplicationGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    static UA_NodeId defaultUserTokenGroup =
        STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP);

    UA_UInt16 openCount;
    UA_UtcTime lastUpdateTime;
    UA_StatusCode res = UA_GDSReceiver_getFileInfoMetadata(
        ctx, group->certificateGroupId, &openCount, &lastUpdateTime);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultApplicationGroup)) {
        static UA_NodeId appGroupLastUpdate =
            STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_LASTUPDATETIME);
        return writeGDSNs0Variable(server, appGroupLastUpdate, &lastUpdateTime,
                                   &UA_TYPES[UA_TYPES_UTCTIME]);
    }

    if(UA_NodeId_equal(&group->certificateGroupId, &defaultUserTokenGroup)) {
        static UA_NodeId tokenGroupLastUpdate =
            STATIC_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_LASTUPDATETIME);
        return writeGDSNs0Variable(server, tokenGroupLastUpdate, &lastUpdateTime,
                                   &UA_TYPES[UA_TYPES_UTCTIME]);
    }

    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

static UA_StatusCode
createFileInfos(UA_GDSReceiverContext *ctx) {
    /* The server currently only supports the DefaultApplicationGroup and
     * UserTokenGroup */
    UA_UtcTime lastUpdateTime = UA_DateTime_now();

    return UA_GDSReceiver_initFileInfos(ctx, lastUpdateTime);
}

static UA_StatusCode
writeGroupVariables(UA_GDSReceiverContext *ctx) {
    UA_Server *server = ((UA_GDSReceiver*)ctx)->drv.server;
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

    UA_UInt16 openCount;
    UA_UtcTime lastUpdateTime;
    UA_StatusCode res = UA_GDSReceiver_getFileInfoMetadata(
        ctx,
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP),
        &openCount, &lastUpdateTime);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    retval |= writeGDSNs0Variable(server,
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENCOUNT),
                                  &openCount, &UA_TYPES[UA_TYPES_UINT16]);
    retval |= writeGDSNs0Variable(server,
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_LASTUPDATETIME),
                                  &lastUpdateTime, &UA_TYPES[UA_TYPES_UTCTIME]);

    /* DefaultUserTokenGroup */
    res = UA_GDSReceiver_getFileInfoMetadata(
        ctx,
        UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP),
        &openCount, &lastUpdateTime);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    retval |= writeGDSNs0Variable(server,
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENCOUNT),
                                  &openCount, &UA_TYPES[UA_TYPES_UINT16]);
    retval |= writeGDSNs0Variable(server,
                                  UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_LASTUPDATETIME),
                                  &lastUpdateTime, &UA_TYPES[UA_TYPES_UTCTIME]);

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

    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
    UA_StatusCode res =
        UA_GDSReceiver_stageCertificateUpdate(ctx, sessionId,
                                              certificateGroupId,
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
    if(!csr)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_GDSReceiver *receiver = (UA_GDSReceiver*)methodContext;
    UA_StatusCode retval =
        UA_GDSReceiver_createSigningRequest(receiver, *certificateGroupId,
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
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
    return UA_GDSReceiver_getRejectedList(ctx, outputSize, output);
}

static UA_StatusCode
applyChangesAction(UA_Server *server,
                   const UA_NodeId *sessionId, void *sessionHandle,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output) {
    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
    return UA_GDSReceiver_applyChangesForSession(ctx, sessionId);
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

    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
    if(UA_GDSReceiver_transactionPending(ctx))
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    return UA_GDSReceiver_addCertificate(ctx, certGroup, certificate,
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

    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
    if(UA_GDSReceiver_transactionPending(ctx))
        return UA_STATUSCODE_BADTRANSACTIONPENDING;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(!certGroup)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    return UA_GDSReceiver_removeCertificate(ctx, certGroup, sessionId,
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

    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
    return UA_GDSReceiver_openTrustListWithMask(ctx, certGroup,
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

    UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
    return UA_GDSReceiver_closeAndUpdateTrustList(ctx, certGroup, sessionId,
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
        UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
        retval = UA_GDSReceiver_openTrustList(ctx, certGroup, sessionId,
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
        UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
        res = UA_GDSReceiver_readTrustList(ctx, certGroup, sessionId,
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
        UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
        retval = UA_GDSReceiver_writeTrustList(ctx, certGroup, sessionId,
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
        UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
        retval = UA_GDSReceiver_closeTrustList(ctx, certGroup, sessionId, fileHandle);
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
        UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
        retval = UA_GDSReceiver_getPositionTrustList(ctx, certGroup, sessionId,
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
        UA_GDSReceiverContext *ctx = (UA_GDSReceiverContext*)methodContext;
        retval = UA_GDSReceiver_setPositionTrustList(ctx, certGroup,
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

static UA_StatusCode
setMethodCallback(UA_GDSReceiverContext *ctx, UA_NodeId methodId,
                  UA_MethodCallback callback) {
    UA_Server *server = ((UA_GDSReceiver*)ctx)->drv.server;
    UA_StatusCode res = UA_Server_setNodeContext(server, methodId, ctx);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return UA_Server_setMethodNodeCallback(server, methodId, callback);
}

UA_StatusCode
initNS0PushManagement(UA_GDSReceiverContext *ctx) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Create the persistent FileInfo state only once. The Namespace Zero
     * callbacks themselves are installed again when the driver is restarted. */
    UA_UInt16 openCount;
    UA_UtcTime lastUpdateTime;
    UA_StatusCode res = UA_GDSReceiver_getFileInfoMetadata(
        ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP),
        &openCount, &lastUpdateTime);
    if(res == UA_STATUSCODE_BADNOTFOUND)
        retval |= createFileInfos(ctx);
    else
        retval |= res;

    /* Set variables */
    retval |= writeGroupVariables(ctx);

    /* Set method callbacks */
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_UPDATECERTIFICATE), updateCertificateAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CREATESIGNINGREQUEST), createSigningRequestAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_GETREJECTEDLIST), getRejectedListAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_APPLYCHANGES), applyChangesAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificateAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificateAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE), removeCertificateAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_REMOVECERTIFICATE), removeCertificateAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMaskAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMaskAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustListAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustListAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPEN), openFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPEN), openFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ), readFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_READ), readFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_WRITE), writeFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_WRITE), writeFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSE), closeFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSE), closeFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_GETPOSITION), getPositionFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_GETPOSITION), getPositionFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SETPOSITION), setPositionFileAction);
    retval |= setMethodCallback(ctx, UA_NS0ID(SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_SETPOSITION), setPositionFileAction);
    if(retval != UA_STATUSCODE_GOOD)
        clearNS0PushManagement(ctx);
    return retval;
}

void
clearNS0PushManagement(UA_GDSReceiverContext *ctx) {
    UA_Server *server = ((UA_GDSReceiver*)ctx)->drv.server;
    static const UA_UInt32 methodIds[] = {
        UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST,
        UA_NS0ID_SERVERCONFIGURATION_GETREJECTEDLIST,
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_REMOVECERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENWITHMASKS,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSEANDUPDATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPEN,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPEN,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_READ,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_WRITE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_WRITE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_GETPOSITION,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_GETPOSITION,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SETPOSITION,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_SETPOSITION
    };

    for(size_t i = 0; i < sizeof(methodIds) / sizeof(methodIds[0]); i++) {
        UA_NodeId methodId = UA_NODEID_NUMERIC(0, methodIds[i]);
        UA_Server_setMethodNodeCallback(server, methodId, NULL);
        UA_Server_setNodeContext(server, methodId, NULL);
    }
}

#ifdef UA_ENABLE_RBAC
/* OPC UA Part 12 v1.05 §7.2 and §7.10.4: PushManagement is restricted to
 * SecurityAdmin. ServerConfiguration and its immediate children stay visible,
 * the CertificateGroups children only to SecurityAdmin. */
UA_StatusCode
initGDSRolePermissions(UA_Server *server) {
    const UA_NodeId secAdmin =
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_SECURITYADMIN);
    const UA_NodeId publicRoles[] = {
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_ANONYMOUS),
        UA_NODEID_NUMERIC(0, UA_NS0ID_WELLKNOWNROLE_AUTHENTICATEDUSER)
    };

    const UA_UInt32 publicVisibleNodes[] = {
        UA_NS0ID_SERVERCONFIGURATION,
        UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST,
        UA_NS0ID_SERVERCONFIGURATION_GETREJECTEDLIST,
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS
    };

    const UA_UInt32 protectedSubtrees[] = {
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP
    };

    const UA_UInt32 callObjectIds[] = {
        UA_NS0ID_SERVERCONFIGURATION,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST
    };

    const UA_UInt32 methodIds[] = {
        UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST,
        UA_NS0ID_SERVERCONFIGURATION_GETREJECTEDLIST,
        UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_REMOVECERTIFICATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENWITHMASKS,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSEANDUPDATE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPEN,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPEN,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_READ,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_READ,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_WRITE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_WRITE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSE,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_GETPOSITION,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_GETPOSITION,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_SETPOSITION,
        UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_SETPOSITION
    };

    /* Apply the permissions, aborting on the first failure. StatusCodes are
     * not bit flags, so they are checked individually instead of OR-ed. */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < sizeof(publicVisibleNodes) / sizeof(publicVisibleNodes[0]); i++) {
        retval = UA_Server_addRolePermissions(server,
                                              UA_NODEID_NUMERIC(0, publicVisibleNodes[i]),
                                              secAdmin,
                                              UA_PERMISSIONTYPE_BROWSE |
                                              UA_PERMISSIONTYPE_READROLEPERMISSIONS,
                                              false, false);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
        for(size_t j = 0; j < sizeof(publicRoles) / sizeof(publicRoles[0]); j++) {
            retval = UA_Server_addRolePermissions(server,
                                                  UA_NODEID_NUMERIC(0, publicVisibleNodes[i]),
                                                  publicRoles[j],
                                                  UA_PERMISSIONTYPE_BROWSE,
                                                  false, false);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }
    }

    for(size_t i = 0; i < sizeof(protectedSubtrees) / sizeof(protectedSubtrees[0]); i++) {
        retval = UA_Server_addRolePermissions(server,
                                              UA_NODEID_NUMERIC(0, protectedSubtrees[i]),
                                              secAdmin,
                                              UA_PERMISSIONTYPE_BROWSE |
                                              UA_PERMISSIONTYPE_READ |
                                              UA_PERMISSIONTYPE_READROLEPERMISSIONS,
                                              false, true);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    for(size_t i = 0; i < sizeof(callObjectIds) / sizeof(callObjectIds[0]); i++) {
        retval = UA_Server_addRolePermissions(server,
                                              UA_NODEID_NUMERIC(0, callObjectIds[i]),
                                              secAdmin,
                                              UA_PERMISSIONTYPE_CALL,
                                              false, false);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    for(size_t i = 0; i < sizeof(methodIds) / sizeof(methodIds[0]); i++) {
        retval = UA_Server_addRolePermissions(server,
                                              UA_NODEID_NUMERIC(0, methodIds[i]),
                                              secAdmin,
                                              UA_PERMISSIONTYPE_CALL,
                                              false, false);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }
    return retval;
}
#endif

#endif
