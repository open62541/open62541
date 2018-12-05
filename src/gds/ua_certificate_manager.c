/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */

#include <ua_plugin_ca.h>
#include "ua_certificate_manager.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_GDS

#define UA_GDS_CM_CHECK_MALLOC(pointer) \
                    if (!pointer) {    \
                        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "malloc failed"); \
                        return UA_STATUSCODE_BADOUTOFMEMORY; \
                    }

#define UA_GDS_CM_CHECK_ALLOC(errorcode) \
                    if (errorcode) {    \
                        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "malloc failed"); \
                        return errorcode; \
                    }

#define UA_GDS_READ_MODE (0x01<<0)

static
void delete_CertificateManager_entry(gds_cm_entry *newEntry){
    if (!UA_ByteString_equal(&newEntry->certificate, &UA_BYTESTRING_NULL))
        UA_ByteString_deleteMembers(&newEntry->certificate);

    if (!UA_ByteString_equal(&newEntry->privateKey, &UA_BYTESTRING_NULL))
        UA_ByteString_deleteMembers(&newEntry->privateKey);

    if (newEntry->issuerCertificateSize > 0){
        size_t index = 0;
        while (index < newEntry->issuerCertificateSize){
            if (!UA_ByteString_equal(&newEntry->issuerCertificates[index],&UA_BYTESTRING_NULL))
                UA_ByteString_deleteMembers(&newEntry->issuerCertificates[index]);
            index++;
        }
        UA_ByteString_delete(newEntry->issuerCertificates);
    }

    UA_free(newEntry);
}

static
UA_StatusCode isApplicationAssignedToCertificateGroup(UA_Server *server,
                                                      UA_NodeId *applicationId,
                                                      UA_GDS_CertificateGroup *certificateGroup) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if (UA_NodeId_equal(applicationId, &UA_NODEID_NULL)
        || certificateGroup == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if (certificateGroup == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_Boolean registered = UA_FALSE;
    if (server->gds_registeredServersSize > 0) {
        gds_registeredServer_entry* current;
        //Looking for application
        LIST_FOREACH(current, &server->gds_registeredServers_list, pointers) {
            if(UA_NodeId_equal(&current->gds_registeredServer.applicationId, applicationId)) {
                //Iterate through certificate groups of the registered application
                for (size_t i = 0; i < current->certificateGroupSize; i++) {
                    if (UA_NodeId_equal(&current->certificateGroups[i], &certificateGroup->certificateGroupId)) {
                        registered = UA_TRUE;
                    }
                }
            }
        }
    }

    if (!registered) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    return ret;
}

static UA_StatusCode
getCertificateGroup(UA_Server *server,
                    UA_NodeId *applicationId,
                    UA_NodeId *certificateGroupId,
                    UA_GDS_CertificateGroup **certificateGroup) {
    if (UA_NodeId_equal(applicationId,&UA_NODEID_NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if (UA_NodeId_equal(certificateGroupId, &UA_NODEID_NULL)){
        //First ApplicationGroup represents DefaultApplicationGroup
        *certificateGroup =  &server->config.gds_certificateGroups[0];
     //   ret = isAppAssignedToCertificateGroup(server,applicationId,&server->config.gds_certificateGroups[0].certificateGroupId,certificateGroup);
    }
    else {
        //Looking for the CertificateGroup
        UA_Boolean isValidNodeId = UA_FALSE;
        for(size_t i = 0; i < server->config.gds_certificateGroupSize; i++) {
            if (UA_NodeId_equal(certificateGroupId, &server->config.gds_certificateGroups[i].certificateGroupId)){
                isValidNodeId = UA_TRUE;
                *certificateGroup =  &server->config.gds_certificateGroups[i];
                break;
            }
        }

        if (!isValidNodeId) {
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    }

    return isApplicationAssignedToCertificateGroup(server,applicationId, *certificateGroup);
}

UA_StatusCode
UA_GDS_GetTrustList(UA_Server *server,
                 const UA_NodeId *sessionId,
                 UA_NodeId *applicationId,
                 UA_NodeId *certificateGroupId,
                 UA_NodeId *trustListId) {

    UA_StatusCode ret;
    UA_GDS_CertificateGroup *cg;
    ret = getCertificateGroup(server,applicationId, certificateGroupId, &cg);

    if (ret != UA_STATUSCODE_GOOD) {
        return ret;
    }

    gds_cm_tl_entry *newEntry = (gds_cm_tl_entry *)UA_calloc(1, sizeof(gds_cm_tl_entry));
    UA_GDS_CM_CHECK_MALLOC(newEntry);
    newEntry->sessionId = *sessionId;
    newEntry->cg = cg;
    LIST_INSERT_HEAD(&server->certificateManager.gds_cm_trustList, newEntry, pointers);

    *trustListId = cg->trustListId;

    return ret;
}

UA_StatusCode
UA_GDS_OpenTrustList(UA_Server *server,
                  const UA_NodeId *sessionId,
                  const UA_NodeId *objectId,
                  UA_Byte  *mode,
                  UA_UInt32 *fileHandle) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    //Just reading is supported at the moment
    if (*mode != UA_GDS_READ_MODE){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    gds_cm_tl_entry *gds_tl, *gds_tl_tmp;
    UA_Boolean validCall = UA_FALSE;
    LIST_FOREACH_SAFE(gds_tl, &server->certificateManager.gds_cm_trustList, pointers, gds_tl_tmp) {
       if (UA_NodeId_equal(&gds_tl->sessionId, sessionId)
           && UA_NodeId_equal(&gds_tl->cg->trustListId, objectId)){
           validCall = UA_TRUE;
           break;
       }
    }

    if (!validCall) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if (gds_tl->fileHandle){ //file is already open
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_GDS_CA *ca = gds_tl->cg->ca;
    ca->getTrustList(ca, &gds_tl->trustList);

    *fileHandle = 1; //represents the position in the file (part 5, p. 100)
    gds_tl->fileHandle = *fileHandle;
    gds_tl->isOpen = UA_TRUE;

    return ret;
}

UA_StatusCode
UA_GDS_ReadTrustList(UA_Server *server,
                  const UA_NodeId *sessionId,
                  const UA_NodeId *trustListNodeId,
                  UA_UInt32 *fileHandle,
                  UA_Int32 *length,
                  UA_TrustListDataType *trustList) {

    UA_StatusCode ret = UA_STATUSCODE_GOOD;


    if (UA_NodeId_equal(sessionId, &UA_NODEID_NULL)
        || UA_NodeId_equal(trustListNodeId, &UA_NODEID_NULL)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    gds_cm_tl_entry *gds_tl, *gds_tl_tmp;
    UA_Boolean validCall = UA_FALSE;
    LIST_FOREACH_SAFE(gds_tl, &server->certificateManager.gds_cm_trustList, pointers, gds_tl_tmp) {
        if (UA_NodeId_equal(&gds_tl->sessionId, sessionId)
            && UA_NodeId_equal(&gds_tl->cg->trustListId, trustListNodeId)
            && *fileHandle == gds_tl->fileHandle
            && gds_tl->isOpen){
            validCall = UA_TRUE;
            break;
        }
    }

    if (!validCall) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    memcpy(trustList, &gds_tl->trustList, sizeof(UA_TrustListDataType));

    return  ret;
}

UA_StatusCode
UA_GDS_CloseTrustList(UA_Server *server,
                  const UA_NodeId *sessionId,
                  const UA_NodeId *objectId,
                  UA_UInt32 *fileHandle) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    gds_cm_tl_entry *gds_tl, *gds_tl_tmp;
    UA_Boolean validCall = UA_FALSE;
    LIST_FOREACH_SAFE(gds_tl, &server->certificateManager.gds_cm_trustList, pointers, gds_tl_tmp) {
        if (UA_NodeId_equal(&gds_tl->sessionId, sessionId)
            && UA_NodeId_equal(&gds_tl->cg->trustListId, objectId)
            && (*fileHandle == gds_tl->fileHandle)){
            validCall = UA_TRUE;
            break;
        }
    }

    if (!validCall) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    LIST_REMOVE(gds_tl, pointers);
    UA_TrustListDataType_deleteMembers(&gds_tl->trustList);
    UA_free(gds_tl);

    return ret;
}


UA_StatusCode
UA_GDS_StartNewKeyPairRequest(UA_Server *server,
                           UA_NodeId *applicationId,
                           UA_NodeId *certificateGroupId,
                           UA_NodeId *certificateTypeId, //every certificateTypeId supports 2048 BitKey
                           UA_String *subjectName,
                           size_t  domainNameSize,
                           UA_String *domainNames,
                           UA_String *privateKeyFormat,
                           UA_String *privateKeyPassword,
                           UA_NodeId *requestId) {
    UA_GDS_CertificateGroup *cg;
    UA_StatusCode retval =
            getCertificateGroup(server, applicationId, certificateGroupId, &cg);

    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_GDS_CA *ca = cg->ca;
    gds_cm_entry *newEntry = (gds_cm_entry *)UA_calloc(1, sizeof(gds_cm_entry));
    UA_GDS_CM_CHECK_MALLOC(newEntry);

    retval = ca->createNewKeyPair(ca, subjectName,
                                  privateKeyFormat, privateKeyPassword,
                                  2048, domainNameSize, domainNames,
                                  &newEntry->certificate, &newEntry->privateKey,
                                  &newEntry->issuerCertificateSize, &newEntry->issuerCertificates);

    if (retval == UA_STATUSCODE_GOOD){
        *requestId = newEntry->requestId = UA_NODEID_GUID(2, UA_Guid_random());
        printf("RequestID: " UA_PRINTF_GUID_FORMAT "\n",
               UA_PRINTF_GUID_DATA(requestId->identifier.guid));
        newEntry->applicationId = *applicationId;
        newEntry->isApproved = UA_TRUE;
        LIST_INSERT_HEAD(&server->certificateManager.gds_cm_list, newEntry, pointers);
        server->certificateManager.counter++;
    }
    else {
        delete_CertificateManager_entry(newEntry);
    }
    return retval;
}

UA_StatusCode
UA_GDS_StartSigningRequest(UA_Server *server,
                        UA_NodeId *applicationId,
                        UA_NodeId *certificateGroupId,
                        UA_NodeId *certificateTypeId,
                        UA_ByteString *certificateRequest,
                        UA_NodeId *requestId){
    UA_GDS_CertificateGroup *cg;
    UA_StatusCode retval =
            getCertificateGroup(server, applicationId, certificateGroupId, &cg);

    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_GDS_CA *ca = cg->ca;
    gds_cm_entry *newEntry = (gds_cm_entry *) UA_calloc(1, sizeof(gds_cm_entry));
    UA_GDS_CM_CHECK_MALLOC(newEntry);

    retval = ca->certificateSigningRequest(ca, 0, certificateRequest,
                                           &newEntry->certificate,
                                           &newEntry->issuerCertificateSize,
                                           &newEntry->issuerCertificates);

    if (retval == UA_STATUSCODE_GOOD){
        *requestId = newEntry->requestId = UA_NODEID_GUID(2, UA_Guid_random());
        newEntry->applicationId = *applicationId;
        newEntry->isApproved = UA_TRUE;
        newEntry->privateKey = UA_BYTESTRING_NULL;
        LIST_INSERT_HEAD(&server->certificateManager.gds_cm_list, newEntry, pointers);
        server->certificateManager.counter++;
    }
    else {
        delete_CertificateManager_entry(newEntry);
    };

    return retval;
}


UA_StatusCode
UA_GDS_FinishRequest(UA_Server *server,
                  UA_NodeId *applicationId,
                  UA_NodeId *requestId,
                  UA_ByteString *certificate,
                  UA_ByteString *privKey,
                  size_t *length,
                  UA_ByteString **issuerCertificate) {
    gds_cm_entry *entry, *entry_tmp;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    LIST_FOREACH_SAFE(entry, &server->certificateManager.gds_cm_list, pointers, entry_tmp) {
       if (UA_NodeId_equal(&entry->requestId, requestId)
           && UA_NodeId_equal(&entry->applicationId, applicationId)
           && entry->isApproved) {
           ret = UA_ByteString_allocBuffer(certificate, entry->certificate.length);
           UA_GDS_CM_CHECK_ALLOC(ret);
           memcpy(certificate->data, entry->certificate.data, entry->certificate.length);

           if (!UA_ByteString_equal(&entry->privateKey, &UA_BYTESTRING_NULL)) {
               UA_ByteString_allocBuffer(privKey, entry->privateKey.length);
               memcpy(privKey->data, entry->privateKey.data, entry->privateKey.length);
           }

           *length = entry->issuerCertificateSize;
           size_t index = 0;
           *issuerCertificate = (UA_ByteString *)
                   UA_calloc (entry->issuerCertificateSize, sizeof(UA_ByteString));
           UA_GDS_CM_CHECK_MALLOC(*issuerCertificate);
           while (index < entry->issuerCertificateSize) {
               ret = UA_ByteString_allocBuffer(issuerCertificate[index],
                                         entry->issuerCertificates[index].length);
               UA_GDS_CM_CHECK_ALLOC(ret);
               memcpy(issuerCertificate[index]->data,
                      entry->issuerCertificates[index].data,
                      entry->issuerCertificates[index].length);
               index++;
           }
           return ret;

       }
    }
    return UA_STATUSCODE_BADINVALIDARGUMENT;
}


UA_StatusCode
UA_GDS_GetCertificateGroups(UA_Server *server,
                         UA_NodeId *applicationId,
                         size_t *outputSize,
                         UA_NodeId **certificateGroupIds) {
    if (server->gds_registeredServersSize > 0) {
        gds_registeredServer_entry* current;
        LIST_FOREACH(current, &server->gds_registeredServers_list, pointers) {
            if(UA_NodeId_equal(&current->gds_registeredServer.applicationId, applicationId)) {
                if (current->certificateGroupSize){
                    *outputSize = current->certificateGroupSize;
                    *certificateGroupIds =
                            (UA_NodeId *) UA_calloc(current->certificateGroupSize, sizeof(UA_NodeId));
                    UA_GDS_CM_CHECK_MALLOC(*certificateGroupIds);
                    memcpy(*certificateGroupIds,
                           current->certificateGroups,
                           sizeof(UA_NodeId) * current->certificateGroupSize);
                    break;
                }
            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDS_CertificateManager_close(UA_Server *server){
    gds_cm_entry *gds_rs, *gds_rs_tmp;
    LIST_FOREACH_SAFE(gds_rs, &server->certificateManager.gds_cm_list, pointers, gds_rs_tmp) {
        LIST_REMOVE(gds_rs, pointers);
        UA_ByteString_deleteMembers(&gds_rs->certificate);
        UA_ByteString_deleteMembers(&gds_rs->privateKey);
        size_t index = 0;
        while (index < gds_rs->issuerCertificateSize) {
            UA_ByteString_deleteMembers(&gds_rs->issuerCertificates[index]);
            index++;
        }
        UA_ByteString_delete(gds_rs->issuerCertificates);
        server->certificateManager.counter--;
        UA_free(gds_rs);
    }
    gds_cm_tl_entry *gds_tl, *gds_tl_tmp;
    LIST_FOREACH_SAFE(gds_tl, &server->certificateManager.gds_cm_trustList, pointers, gds_tl_tmp) {
        LIST_REMOVE(gds_tl, pointers);
        UA_TrustListDataType_deleteMembers(&gds_tl->trustList);
        //evtl tl noch freigeben
        UA_free(gds_tl);
    }
    return UA_STATUSCODE_GOOD;

}

UA_StatusCode
UA_GDS_CertificateManager_init(UA_Server *server) {
    LIST_INIT(&server->certificateManager.gds_cm_list);
    server->certificateManager.counter = 0;
    LIST_INIT(&server->certificateManager.gds_cm_trustList);
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_GDS */
