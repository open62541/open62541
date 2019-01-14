/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */


#include "ua_registration_manager.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_GDS

#define UA_GDS_RM_INVALIDARGUMENT                       \
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "Invalid Argument"); \
            ret = UA_STATUSCODE_BADINVALIDARGUMENT;     \
            goto error;


#define UA_GDS_RM_CHECK_MALLOC(pointer)         \
        if (!pointer) {   \
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "Malloc failed"); \
            ret = UA_STATUSCODE_BADINVALIDARGUMENT; \
            goto error;                             \
        }


//TODO replacement for string localhost in discoveryurl
UA_StatusCode
UA_GDS_registerApplication(UA_Server *server, UA_ApplicationRecordDataType *input,
                         size_t certificateGroupSize, UA_NodeId *certificateGroupIds, UA_NodeId *output) {
    size_t index = 0;
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    gds_registeredServer_entry *newEntry = (gds_registeredServer_entry *)
             UA_malloc(sizeof(gds_registeredServer_entry));
    if (!newEntry) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "malloc failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_ApplicationRecordDataType *record = &newEntry->gds_registeredServer;
    memset(record, 0, sizeof(UA_ApplicationRecordDataType));

    //ApplicationUri
    if (UA_String_equal(&input->applicationUri, &UA_STRING_NULL)) {
        UA_GDS_RM_INVALIDARGUMENT;
    }
    record->applicationUri.length = input->applicationUri.length;
    record->applicationUri.data = (UA_Byte *) UA_malloc(input->applicationUri.length * sizeof(UA_Byte));
    UA_GDS_RM_CHECK_MALLOC(record->applicationUri.data);
    memcpy(record->applicationUri.data, input->applicationUri.data,input->applicationUri.length);

    //check and set ApplicationType
    if (input->applicationType != UA_APPLICATIONTYPE_SERVER
        && input->applicationType != UA_APPLICATIONTYPE_CLIENT
        && input->applicationType != UA_APPLICATIONTYPE_DISCOVERYSERVER) {
        UA_GDS_RM_INVALIDARGUMENT;
    }
    record->applicationType = input->applicationType;

    //ApplicationNames
    if(input->applicationNamesSize == 0) {
        UA_GDS_RM_INVALIDARGUMENT;
    }

    record->applicationNamesSize = input->applicationNamesSize;
    record->applicationNames = (UA_LocalizedText *)
             UA_calloc(record->applicationNamesSize, sizeof(UA_LocalizedText));
    UA_GDS_RM_CHECK_MALLOC(record->applicationNames);

    while(index < input->applicationNamesSize) {
        if(UA_String_equal(&input->applicationNames[index].locale, &UA_STRING_NULL)
           || UA_String_equal(&input->applicationNames[index].text, &UA_STRING_NULL)) {
            UA_GDS_RM_INVALIDARGUMENT;
        }
        UA_LocalizedText_init(&record->applicationNames[index]);

        size_t locale_length = input->applicationNames[index].locale.length;
        record->applicationNames[index].locale.length = locale_length;
        record->applicationNames[index].locale.data = (UA_Byte *)
                UA_malloc(locale_length * sizeof (UA_Byte));
        UA_GDS_RM_CHECK_MALLOC(record->applicationNames[index].locale.data);
        memcpy(record->applicationNames[index].locale.data, input->applicationNames[index].locale.data, locale_length);

        size_t text_length = input->applicationNames[index].text.length;
        record->applicationNames[index].text.length = text_length;
        record->applicationNames[index].text.data = (UA_Byte *)
                UA_malloc(text_length * sizeof (UA_Byte));
        UA_GDS_RM_CHECK_MALLOC(record->applicationNames[index].text.data);
        memcpy(record->applicationNames[index].text.data, input->applicationNames[index].text.data, text_length);

        index++;
    }

    //ProductUri
    if (UA_String_equal(&input->productUri, &UA_STRING_NULL)) {
        UA_GDS_RM_INVALIDARGUMENT;
    }
    record->productUri.length = input->productUri.length;
    record->productUri.data = (UA_Byte *) UA_malloc(input->productUri.length * sizeof(UA_Byte));
    UA_GDS_RM_CHECK_MALLOC(record->productUri.data);
    memcpy(record->productUri.data, input->productUri.data, input->productUri.length);

    //DiscoveryUrls
    //For servers it is mandatory to specify at least one discoveryUrl.
    //For Clients it is only required if they support reverse connect TODO(inv+ as prefix)
    if(record->applicationType != UA_APPLICATIONTYPE_CLIENT && input->discoveryUrlsSize == 0) {
        UA_GDS_RM_INVALIDARGUMENT;
    }

    if (input->discoveryUrlsSize > 0) {
        index = 0;
        record->discoveryUrlsSize = input->discoveryUrlsSize;
        record->discoveryUrls = (UA_String *)
                UA_calloc(record->discoveryUrlsSize, sizeof(UA_String));
        UA_GDS_RM_CHECK_MALLOC(record->discoveryUrls);
        while(index < record->discoveryUrlsSize) {
            if (UA_String_equal(&input->discoveryUrls[index], &UA_STRING_NULL)) {
                UA_GDS_RM_INVALIDARGUMENT;
            }
            UA_String_init(&record->discoveryUrls[index]);

            size_t discoveryLength = input->discoveryUrls[index].length;
            record->discoveryUrls[index].length = discoveryLength;
            record->discoveryUrls[index].data =
                     (UA_Byte *) UA_malloc(discoveryLength * sizeof(UA_Byte));
            UA_GDS_RM_CHECK_MALLOC(record->discoveryUrls[index].data);
            memcpy(record->discoveryUrls[index].data, input->discoveryUrls[index].data, discoveryLength);
            index++;
         }
     }

     //ServerCapabilities
     if(record->applicationType != UA_APPLICATIONTYPE_CLIENT && input->serverCapabilitiesSize == 0) {
         UA_GDS_RM_INVALIDARGUMENT;
     }

     if (input->serverCapabilitiesSize > 0) {
         index = 0;
         record->serverCapabilitiesSize = input->serverCapabilitiesSize;
         record->serverCapabilities = (UA_String *)
                 UA_calloc(record->serverCapabilitiesSize, sizeof(UA_String));
         while(index < record->serverCapabilitiesSize) {
             if (UA_String_equal(&input->serverCapabilities[index], &UA_STRING_NULL)) {
                 UA_GDS_RM_INVALIDARGUMENT;
             }
             UA_String_init(&record->serverCapabilities[index]);

             size_t capLength = input->serverCapabilities[index].length;
             record->serverCapabilities[index].length = capLength;
             record->serverCapabilities[index].data =
                     (UA_Byte *) UA_malloc(capLength * sizeof(UA_Byte));
             UA_GDS_RM_CHECK_MALLOC(record->serverCapabilities[index].data);
             memcpy(record->serverCapabilities[index].data, input->serverCapabilities[index].data, capLength);

             index++;
         }
     }

     //CertificateGroup
     if (certificateGroupSize > 0) {
         index = 0;
         newEntry->certificateGroupSize = certificateGroupSize;
         newEntry->certificateGroups = (UA_NodeId *)
                 UA_calloc(certificateGroupSize, sizeof(UA_NodeId));
         UA_GDS_RM_CHECK_MALLOC(newEntry->certificateGroups);
         while(index < certificateGroupSize) {
             memcpy(&newEntry->certificateGroups[index], &certificateGroupIds[index], sizeof(UA_NodeId));
             index++;
         }
     }

     record->applicationId = UA_NODEID_GUID(2, UA_Guid_random());
     *output = record->applicationId;
     LIST_INSERT_HEAD(&server->gds_registeredServers_list, newEntry, pointers);
     server->gds_registeredServersSize++;
     UA_LOG_INFO(&server->config.logger,
                 UA_LOGCATEGORY_SERVER,
                 "New Application registered (NodeId): " UA_PRINTF_GUID_FORMAT "",
                 UA_PRINTF_GUID_DATA(record->applicationId.identifier.guid));
     return UA_STATUSCODE_GOOD;

error:
     UA_deleteMembers(record, &ApplicationRecordDataType);
     UA_free(newEntry);

     return ret;
}


UA_StatusCode
UA_GDS_findApplication(UA_Server *server, UA_String *applicationUri,
                    size_t *outputSize, UA_ApplicationRecordDataType **output) {

    /* Temporarily store all the pointers which we found to avoid reiterating
     * through the list */
    if (server->gds_registeredServersSize > 0) {
        size_t foundServersSize = 0;
        UA_STACKARRAY(UA_ApplicationRecordDataType*, gds_foundServers, server->gds_registeredServersSize);
        gds_registeredServer_entry* current;
        LIST_FOREACH(current, &server->gds_registeredServers_list, pointers) {
            if(UA_String_equal(&current->gds_registeredServer.applicationUri, applicationUri)) {
                gds_foundServers[foundServersSize] = &current->gds_registeredServer;
                foundServersSize++;
            }
        }
        *outputSize = foundServersSize;
        if (foundServersSize > 0) {
            *output = (UA_ApplicationRecordDataType *) UA_calloc(foundServersSize, sizeof(UA_ApplicationRecordDataType));
            if (*output == NULL) {
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER, "Malloc failed");
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            for(size_t i = 0; i < foundServersSize; i++) {
                memcpy(output[i], gds_foundServers[i], sizeof(UA_ApplicationRecordDataType));
                i++;
            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDS_unregisterApplication(UA_Server *server, UA_NodeId *nodeId) {
    gds_registeredServer_entry *gds_rs, *gds_rs_tmp;
    LIST_FOREACH_SAFE(gds_rs, &server->gds_registeredServers_list, pointers, gds_rs_tmp) {
        if(UA_NodeId_equal(&gds_rs->gds_registeredServer.applicationId, nodeId)) {
            LIST_REMOVE(gds_rs, pointers);
            UA_deleteMembers(&gds_rs->gds_registeredServer, &ApplicationRecordDataType);
            if(gds_rs->certificateGroupSize > 0)
                UA_free(gds_rs->certificateGroups);
            UA_free(gds_rs);
            server->gds_registeredServersSize--;
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDS_RegistrationManager_close(UA_Server *rm) {
    gds_registeredServer_entry *gds_rs, *gds_rs_tmp;
    LIST_FOREACH_SAFE(gds_rs, &rm->gds_registeredServers_list, pointers, gds_rs_tmp) {
        LIST_REMOVE(gds_rs, pointers);
        UA_deleteMembers(&gds_rs->gds_registeredServer, &ApplicationRecordDataType);
        if(gds_rs->certificateGroupSize > 0)
            UA_free(gds_rs->certificateGroups);
        UA_free(gds_rs);
        rm->gds_registeredServersSize--;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_GDS_RegistrationManager_init(UA_Server *server) {
    LIST_INIT(&server->gds_registeredServers_list);
    server->gds_registeredServersSize = 0;
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_GDS */
