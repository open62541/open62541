/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include "ua_server_internal.h"
#include <open62541/plugin/log_stdout.h>

#ifdef UA_ENABLE_GDS_PUSHMANAGEMENT

#define UA_SHA1_LENGTH 20

typedef struct UA_FileContext UA_FileContext;
struct UA_FileContext {
    LIST_ENTRY(UA_FileContext) listEntry;
    UA_ByteString *file;
    UA_UInt32 fileHandle;
    UA_NodeId sessionId;
    UA_UInt64 currentPos;
    UA_Byte openFileMode;
};

typedef struct UA_FileInfo UA_FileInfo;
struct UA_FileInfo {
    UA_UInt16 openCount;
    LIST_HEAD(, UA_FileContext)fileContext;
};

/* TODO: Optimize search */
static UA_StatusCode
createFileHandleId(UA_FileInfo *fileInfo, UA_UInt32 *fileHandle) {
    if(fileInfo == NULL || fileHandle == NULL)
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
    if(fileInfo == NULL || sessionId == NULL)
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
    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST);
    UA_NodeId defaultUserTokenGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST);
    if(UA_NodeId_equal(objectId, &defaultApplicationGroup)) {
        return &server->config.secureChannelPKI;
    }
    if(UA_NodeId_equal(objectId, &defaultUserTokenGroup)) {
        return &server->config.sessionPKI;
    }
    return NULL;
}

static UA_StatusCode
writeGDSNs0VariableArray(UA_Server *server, const UA_NodeId id, void *v,
                         size_t length, const UA_DataType *type) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return writeValueAttribute(server, id, &var);
}

static UA_StatusCode
writeGDSNs0Variable(UA_Server *server, const UA_NodeId id,
                    void *v, const UA_DataType *type) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, v, type);
    return writeValueAttribute(server, id, &var);
}

static UA_Boolean
compareThumbprint(const UA_String *str1, const UA_String *str2) {
    if (str1->length != str2->length)
        return false;

    for (size_t i = 0; i < str1->length; i++) {
        char ch1 = str1->data[i];
        char ch2 = str2->data[i];

        // Convert characters to lowercase if they are uppercase
        if (ch1 >= 'A' && ch1 <= 'Z')
            ch1 += ('a' - 'A');
        if (ch2 >= 'A' && ch2 <= 'Z')
            ch2 += ('a' - 'A');

        // Compare the characters
        if (ch1 != ch2)
            return false;
    }
    return true;
}

static UA_StatusCode
updateCertificate(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionHandle,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

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
    UA_ByteString *issuerCertificates = ((UA_ByteString *)input[3].data);
    size_t issuerCertificatesSize = input[3].arrayLength;
    UA_String *privateKeyFormat = (UA_String *)input[4].data;
    UA_ByteString *privateKey = (UA_ByteString *)input[5].data;

    retval = UA_Server_updateCertificate(server, certificateGroupId, certificateTypeId,
                                         certificate, issuerCertificates, issuerCertificatesSize,
                                         privateKey, privateKeyFormat);

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
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

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

    retval = UA_Server_createSigningRequest(server, certificateGroupId,
                                            certificateTypeId, subjectName,
                                            regenerateKey, nonce, csr);

    if (retval != UA_STATUSCODE_GOOD) {
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
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    size_t rejectedListSize = 0;
    UA_CertificateGroup certGroup = server->config.secureChannelPKI;

    /* Default Application Group */
    UA_ByteString *rejectedListSecureChannel = NULL;
    size_t rejectedListSecureChannelSize = 0;
    certGroup.getRejectedList(&certGroup, &rejectedListSecureChannel, &rejectedListSecureChannelSize);
    rejectedListSize += rejectedListSecureChannelSize;

    /* User Token Group */
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
    if(rejectedList == NULL) {
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

    return retval;
}

static UA_StatusCode
addCertificate(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTESTRING]) || /*Certificate*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN])) /*IsTrustedCertificate*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_ByteString certificate = *(UA_ByteString *)input[0].data;
    UA_Boolean isTrustedCertificate = *(UA_Boolean *)input[1].data;

    if(!isTrustedCertificate || certificate.length == 0)
        return UA_STATUSCODE_BADCERTIFICATEINVALID;

    /* TODO: check if TrustList Object is already open */
    /* TODO: check if TrustList Object is read only */

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    UA_ByteString certificates[1];
    certificates[0] = certificate;

    trustList.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    trustList.trustedCertificates = certificates;
    trustList.trustedCertificatesSize = 1;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_CertificateGroup *transactionCertGroup =
        UA_Transaction_getCertificateGroup(server->transaction, certGroup);
    if(transactionCertGroup == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(transactionCertGroup->verifyCertificate(transactionCertGroup, &certificate, NULL, 0) != UA_STATUSCODE_GOOD) {
        return UA_STATUSCODE_BADCERTIFICATEINVALID;
    }

    return transactionCertGroup->addToTrustList(transactionCertGroup, &trustList);
}

static UA_StatusCode
removeCertificate(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionHandle,
                const UA_NodeId *methodId, void *methodContext,
                const UA_NodeId *objectId, void *objectContext,
                size_t inputSize, const UA_Variant *input,
                size_t outputSize, UA_Variant *output) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) || /*Thumbprint*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BOOLEAN])) /*IsTrustedCertificate*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_String thumbprint = *(UA_String *)input[0].data;
    UA_Boolean isTrustedCertificate = *(UA_Boolean *)input[1].data;

    /* TODO: check if TrustList Object is already open */
    /* TODO: check if TrustList Object is read only */
    /* TODO: If the Certificate is a CA Certificate that has CRLs then all CRLs for that CA are removed as well */

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

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

    UA_String thumbpr = UA_STRING_NULL;
    thumbpr.length = (UA_SHA1_LENGTH * 2);
    thumbpr.data = (UA_Byte*)malloc(sizeof(UA_Byte)*thumbpr.length);
    for(size_t i = 0; i < certificatesSize; i++) {
        UA_CertificateUtils_getThumbprint( &certificates[i], &thumbpr);
        if(compareThumbprint(&thumbprint, &thumbpr)) {
            UA_ByteString certificate[1];
            certificate[0] = certificates[i];

            if(isTrustedCertificate) {
                list.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
                list.trustedCertificates = certificate;
                list.trustedCertificatesSize = 1;
            } else {
                list.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;
                list.issuerCertificates = certificate;
                list.issuerCertificatesSize = 1;
            }
            break;
        }
    }

    UA_CertificateGroup *transactionCertGroup =
        UA_Transaction_getCertificateGroup(server->transaction, certGroup);
    if(transactionCertGroup == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(list.specifiedLists != UA_TRUSTLISTMASKS_NONE) {
        retval = transactionCertGroup->removeFromTrustList(transactionCertGroup, &list);
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "The certificate to remove was not found");
        retval = UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_String_clear(&thumbpr);
    UA_TrustListDataType_clear(&trustList);

    return retval;
}

static UA_StatusCode
openTrustList(UA_Server *server,
              const UA_NodeId *sessionId, void *sessionHandle,
              const UA_NodeId *methodId, void *methodContext,
              const UA_NodeId *objectId, void *objectContext,
              size_t inputSize, const UA_Variant *input,
              size_t outputSize, UA_Variant *output) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_BYTE]))/*FileMode*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_Byte fileOpenMode = *(UA_Byte*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(UA_Transaction_containsCertificateGroup(server->transaction, certGroup->certificateGroupId) &&
       fileOpenMode == (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING)) {
        return UA_STATUSCODE_BADTRANSACTIONPENDING;
    }

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    trustList.specifiedLists = UA_TRUSTLISTMASKS_ALL;

    certGroup->getTrustList(certGroup, &trustList);

    UA_ByteString *encTrustList = UA_ByteString_new();
    UA_ByteString_init(encTrustList);
    retval = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], encTrustList);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(encTrustList);
        UA_TrustListDataType_clear(&trustList);
        return retval;
    }

    UA_TrustListDataType_clear(&trustList);

    if(certGroup->applicationContext == NULL) {
        UA_FileInfo *fileInfo = (UA_FileInfo*)malloc(sizeof(UA_FileInfo));
        fileInfo->openCount = 0;
        LIST_INIT(&fileInfo->fileContext);
        certGroup->applicationContext = (void*)fileInfo;
    }

    UA_FileInfo *fileInfo = (UA_FileInfo*)certGroup->applicationContext;

    UA_FileContext *fileContext = (UA_FileContext*)malloc(sizeof(UA_FileContext));
    fileContext->file = encTrustList;
    fileContext->sessionId = *sessionId;
    fileContext->openFileMode = fileOpenMode;
    fileContext->currentPos = 0;
    retval = createFileHandleId(fileInfo, &fileContext->fileHandle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_delete(fileContext->file);
        UA_free(fileContext);
        if(fileInfo->openCount == 0) {
            UA_free(fileInfo);
            certGroup->applicationContext = NULL;
        }
        return retval;
    }

    if(fileOpenMode == UA_OPENFILEMODE_READ) {
        UA_FileContext *fileContextTmp = NULL;
        LIST_FOREACH(fileContextTmp, &fileInfo->fileContext, listEntry) {
            if(fileContextTmp->openFileMode & (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
                retval = UA_STATUSCODE_BADNOTREADABLE;
        }
    } else if(fileOpenMode == (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING)) {
        if(fileInfo->openCount != 0)
            retval = UA_STATUSCODE_BADNOTWRITABLE;
    } else {
        retval = UA_STATUSCODE_BADINVALIDSTATE;
    }

    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_delete(fileContext->file);
        UA_free(fileContext);
        if(fileInfo->openCount == 0) {
            UA_free(fileInfo);
            certGroup->applicationContext = NULL;
        }
        return retval;
    }

    fileInfo->openCount += 1;
    UA_Variant_setScalarCopy(output, &fileContext->fileHandle, &UA_TYPES[UA_TYPES_UINT32]);

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

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /*Mask*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 mask = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    trustList.specifiedLists = mask;

    certGroup->getTrustList(certGroup, &trustList);

    UA_ByteString *encTrustList = UA_ByteString_new();
    UA_ByteString_init(encTrustList);
    retval = UA_encodeBinary(&trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], encTrustList);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(encTrustList);
        UA_TrustListDataType_clear(&trustList);
        return retval;
    }

    UA_TrustListDataType_clear(&trustList);

    if(certGroup->applicationContext == NULL) {
        UA_FileInfo *fileInfo = (UA_FileInfo*)malloc(sizeof(UA_FileInfo));
        fileInfo->openCount = 0;
        LIST_INIT(&fileInfo->fileContext);
        certGroup->applicationContext = (void*)fileInfo;
    }

    UA_FileInfo *fileInfo = (UA_FileInfo*)certGroup->applicationContext;

    UA_FileContext *fileContext = (UA_FileContext*)malloc(sizeof(UA_FileContext));
    fileContext->file = encTrustList;
    fileContext->sessionId = *sessionId;
    fileContext->openFileMode = UA_OPENFILEMODE_READ;
    fileContext->currentPos = 0;
    retval = createFileHandleId(fileInfo, &fileContext->fileHandle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_delete(fileContext->file);
        UA_free(fileContext);
        if(fileInfo->openCount == 0) {
            UA_free(fileInfo);
            certGroup->applicationContext = NULL;
        }
        return retval;
    }

    UA_FileContext *fileContextTmp = NULL;
    LIST_FOREACH(fileContextTmp, &fileInfo->fileContext, listEntry) {
        if(fileContextTmp->openFileMode & (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
            retval = UA_STATUSCODE_BADNOTREADABLE;
    }

    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_delete(fileContext->file);
        UA_free(fileContext);
        if(fileInfo->openCount == 0) {
            UA_free(fileInfo);
            certGroup->applicationContext = NULL;
        }
        return retval;
    }

    fileInfo->openCount += 1;
    UA_Variant_setScalarCopy(output, &fileContext->fileHandle, &UA_TYPES[UA_TYPES_UINT32]);

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

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /*FileHandle*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_INT32])) /*Length*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_Int32 length = *(UA_Int32*)input[1].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(certGroup->applicationContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileInfo *fileInfo = (UA_FileInfo*)certGroup->applicationContext;
    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);

    if(fileContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(fileContext->openFileMode & (UA_OPENFILEMODE_WRITE | UA_OPENFILEMODE_ERASEEXISTING))
        return UA_STATUSCODE_BADINVALIDSTATE;

    /* check boundaries */
    if((size_t)length >= fileContext->file->length) {
        length = fileContext->file->length;
    }
    if((size_t)length >= (fileContext->file->length - fileContext->currentPos)) {
        length = (fileContext->file->length - fileContext->currentPos);
    }

    UA_ByteString *readBuffer = UA_ByteString_new();
    UA_ByteString_init(readBuffer);

    if(length > 0) {
        readBuffer->length = length;
        readBuffer->data = (UA_Byte*) UA_malloc(readBuffer->length * sizeof(UA_Byte));
        memcpy(readBuffer->data, fileContext->file->data+fileContext->currentPos, readBuffer->length);
        fileContext->currentPos += length;
    }

    UA_Variant_setScalarCopy(output, readBuffer, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_ByteString_delete(readBuffer);

    return retval;
}

static UA_StatusCode
writeTrustList(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /*FileHandle*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_BYTESTRING])) /*Data*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_ByteString data = *(UA_ByteString*)input[1].data;

    if(data.length == 0)
        return UA_STATUSCODE_GOOD;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(certGroup->applicationContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileInfo *fileInfo = (UA_FileInfo*)certGroup->applicationContext;
    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);

    if(fileContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(fileContext->openFileMode & UA_OPENFILEMODE_READ)
        return UA_STATUSCODE_BADINVALIDSTATE;

    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    retval = UA_decodeBinary(&data, &trustList, &UA_TYPES[UA_TYPES_TRUSTLISTDATATYPE], NULL);

    if(retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* TODO: 1. Get current trust list 2. Write current trust list 3. Save the edited list in the transaction object. */

    /* TODO: Currently added directly, later this should be added after called closeAndUpdate function. */
    retval = certGroup->setTrustList(certGroup, &trustList);

    return retval;
}

static UA_StatusCode
closeTrustList(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /*FileHandle*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(!certGroup->applicationContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileInfo *fileInfo = (UA_FileInfo*)certGroup->applicationContext;
    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);

    if(fileContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    LIST_REMOVE(fileContext, listEntry);
    fileInfo->openCount -= 1;

    UA_ByteString_delete(fileContext->file);
    UA_free(fileContext);
    if(fileInfo->openCount == 0) {
        UA_free(fileInfo);
        certGroup->applicationContext = NULL;
    }

    return retval;
}

static UA_StatusCode
closeAndUpdateTrustList(UA_Server *server,
               const UA_NodeId *sessionId, void *sessionHandle,
               const UA_NodeId *methodId, void *methodContext,
               const UA_NodeId *objectId, void *objectContext,
               size_t inputSize, const UA_Variant *input,
               size_t outputSize, UA_Variant *output) {
    /* TODO: The function currently only calls close. This will change when transactions are implemented. */
    UA_StatusCode retval =
            closeTrustList(server, sessionId, sessionHandle, methodId, methodContext,
                           objectId, objectContext, inputSize, input, outputSize, output);
    return retval;
}

static UA_StatusCode
getPositionTrustList(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionHandle,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32])) /*FileHandle*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(!certGroup->applicationContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileInfo *fileInfo = (UA_FileInfo*)certGroup->applicationContext;
    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);

    if(fileContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Variant_setScalarCopy(output, &fileContext->currentPos, &UA_TYPES[UA_TYPES_UINT64]);

    return retval;

}

static UA_StatusCode
setPositionTrustList(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionHandle,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /*check for input types*/
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_UINT32]) || /*FileHandle*/
       !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_UINT64])) /*Position*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_UInt32 fileHandle = *(UA_UInt32*)input[0].data;
    UA_UInt64 position = *(UA_UInt32*)input[1].data;

    UA_CertificateGroup *certGroup = getCertGroup(server, objectId);
    if(certGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(!certGroup->applicationContext)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_FileInfo *fileInfo = (UA_FileInfo*)certGroup->applicationContext;
    UA_FileContext *fileContext = getFileContext(fileInfo, sessionId, fileHandle);

    if(fileContext == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(fileContext->file->length < position) {
        fileContext->currentPos = fileContext->file->length;
    } else {
        fileContext->currentPos = position;
    }

    return retval;
}

static UA_StatusCode
applyChanges(UA_Server *server,
             const UA_NodeId *sessionId, void *sessionHandle,
             const UA_NodeId *methodId, void *methodContext,
             const UA_NodeId *objectId, void *objectContext,
             size_t inputSize, const UA_Variant *input,
             size_t outputSize, UA_Variant *output) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    if(server->transaction) {
        if(!UA_NodeId_equal(&server->transaction->sessionId, sessionId))
            return UA_STATUSCODE_BADUSERACCESSDENIED;
    }
    retval = UA_Server_applyChanges(server);

    return retval;
}

static UA_StatusCode
writeGroupVariables(UA_Server *server) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* The server currently only supports the DefaultApplicationGroup */
    UA_CertificateGroup certGroup = server->config.secureChannelPKI;

    UA_NodeId defaultApplicationGroup = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP);
    if(!UA_NodeId_equal(&certGroup.certificateGroupId, &defaultApplicationGroup))
        return UA_STATUSCODE_BADINTERNALERROR;

    /* TODO: Get CertifcateTypes from corresponding group */
    UA_NodeId certificateTypes[2] = {UA_NODEID_NUMERIC(0, UA_NS0ID_RSAMINAPPLICATIONCERTIFICATETYPE),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_RSASHA256APPLICATIONCERTIFICATETYPE)};
    size_t certificateTypesSize = 2;

    UA_String supportedPrivateKeyFormats[2] = {UA_STRING("PEM"),
                                               UA_STRING("PFX")};
    size_t supportedPrivateKeyFormatsSize = 2;

    UA_UInt32  maxTrustListSize = 0;

    /* Set variables */
    retval |= writeGDSNs0VariableArray(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_SUPPORTEDPRIVATEKEYFORMATS),
                                       supportedPrivateKeyFormats, supportedPrivateKeyFormatsSize,
                                       &UA_TYPES[UA_TYPES_STRING]);

    retval |= writeGDSNs0Variable(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_MAXTRUSTLISTSIZE),
                                  &maxTrustListSize, &UA_TYPES[UA_TYPES_UINT32]);

    retval |= writeGDSNs0VariableArray(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_CERTIFICATETYPES),
                                       certificateTypes, certificateTypesSize,
                                       &UA_TYPES[UA_TYPES_NODEID]);

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
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        return openTrustList(server, sessionId, sessionHandle, methodId, methodContext, objectId, objectContext,
                             inputSize, input, outputSize, output);
    }

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
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        return readTrustList(server, sessionId, sessionHandle, methodId, methodContext, objectId, objectContext,
                             inputSize, input, outputSize, output);
    }

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
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        return writeTrustList(server, sessionId, sessionHandle, methodId, methodContext, objectId, objectContext,
                             inputSize, input, outputSize, output);
    }

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
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        return closeTrustList(server, sessionId, sessionHandle, methodId, methodContext, objectId, objectContext,
                             inputSize, input, outputSize, output);
    }

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
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        return getPositionTrustList(server, sessionId, sessionHandle, methodId, methodContext, objectId, objectContext,
                                    inputSize, input, outputSize, output);
    }

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
    if(UA_NodeId_equal(&objectType->head.nodeId, &trustListType)) {
        return setPositionTrustList(server, sessionId, sessionHandle, methodId, methodContext, objectId, objectContext,
                                    inputSize, input, outputSize, output);
    }

    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "File type functions are currently only supported for TrustList types");
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

UA_StatusCode
initNS0PushManagement(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Set variables */
    retval |= writeGroupVariables(server);

    /* Set method callbacks */
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_UPDATECERTIFICATE), updateCertificate);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATIONTYPE_UPDATECERTIFICATE), updateCertificate);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CREATESIGNINGREQUEST), createSigningRequest);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATIONTYPE_CREATESIGNINGREQUEST), createSigningRequest);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_GETREJECTEDLIST), getRejectedList);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATIONTYPE_GETREJECTEDLIST), getRejectedList);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_APPLYCHANGES), applyChanges);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATIONTYPE_APPLYCHANGES), applyChanges);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificate);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE), addCertificate);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE_ADDCERTIFICATE), addCertificate);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_REMOVECERTIFICATE), removeCertificate);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_ADDCERTIFICATE), removeCertificate);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE_REMOVECERTIFICATE), removeCertificate);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMask);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_OPENWITHMASKS), openTrustListWithMask);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE_OPENWITHMASKS), openTrustListWithMask);

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTAPPLICATIONGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustList);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERCONFIGURATION_CERTIFICATEGROUPS_DEFAULTUSERTOKENGROUP_TRUSTLIST_CLOSEANDUPDATE), closeAndUpdateTrustList);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_TRUSTLISTTYPE_CLOSEANDUPDATE), closeAndUpdateTrustList);

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

    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_OPEN), openFile);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_READ), readFile);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_WRITE), writeFile);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_CLOSE), closeFile);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_GETPOSITION), getPositionFile);
    retval |= setMethodNode_callback(server, UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE_SETPOSITION), setPositionFile);

    return retval;
}

#endif
