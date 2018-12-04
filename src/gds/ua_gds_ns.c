/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Markus Karch, Fraunhofer IOSB
 */


#include "ua_registration_manager.h"
#include "server/ua_server_internal.h"

#ifdef  UA_ENABLE_GDS

/**********************************************/
/*         RegistrationManager-Callbacks      */
/**********************************************/

static UA_StatusCode
registerApplicationMethodCallback(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionHandle,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    UA_NodeId applicationId;
    //The DefaultApplicationGroup is the default value for new applications
    UA_NodeId dag =  UA_NODEID_NUMERIC(2, 615);
    UA_StatusCode retval =
            UA_GDS_registerApplication(server, (UA_ApplicationRecordDataType *)input->data, 1, &dag, &applicationId);

    if (retval == UA_STATUSCODE_GOOD)
        UA_Variant_setScalarCopy(output, &applicationId, &UA_TYPES[UA_TYPES_NODEID]);

    return retval;
}

static UA_StatusCode
findApplicationMethodCallback(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionHandle,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output) {
    size_t length = 0;
    UA_ApplicationRecordDataType *array;
    UA_StatusCode retval =
            UA_GDS_findApplication(server, (UA_String *)input->data, &length, &array);

    if (length > 0){
        UA_Variant_setArrayCopy(output, array, length, &ApplicationRecordDataType);
        UA_free(array);
    }
    return retval;
}


static UA_StatusCode
unregisterApplicationMethodCallback(UA_Server *server,
                              const UA_NodeId *sessionId, void *sessionHandle,
                              const UA_NodeId *methodId, void *methodContext,
                              const UA_NodeId *objectId, void *objectContext,
                              size_t inputSize, const UA_Variant *input,
                              size_t outputSize, UA_Variant *output) {
    return UA_GDS_unregisterApplication(server, (UA_NodeId *) input->data);
}


/**********************************************/
/*         RegistrationManager-Methods        */
/**********************************************/

static void
addFindApplicationsMethod(UA_Server *server, UA_UInt16 ns_index, UA_NodeId directoryTypeId) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "String");
    inputArgument.name = UA_STRING("InputArguments");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = -1; /* scalar */

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "ApplicationRecordDataType");
    outputArgument.name = UA_STRING("OutputArguments");
    outputArgument.dataType = ApplicationRecordDataType.typeId;
    outputArgument.valueRank = 1;

    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US","FindApplications");
    mAttr.executable = true;
    mAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(ns_index, 143),
                            directoryTypeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns_index, "FindApplications"),
                            mAttr, &findApplicationMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);

    UA_Server_addReference(server, UA_NODEID_NUMERIC(ns_index, 143),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}

static void
addRegisterApplicationMethod(UA_Server *server, UA_UInt16 ns_index, UA_NodeId directoryTypeId) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "ApplicationRecordDataType");
    inputArgument.name = UA_STRING("InputArguments");
    inputArgument.dataType = ApplicationRecordDataType.typeId;
    inputArgument.valueRank = -1; /* scalar */

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "ApplicationId");
    outputArgument.name = UA_STRING("OutputArguments");
    outputArgument.dataType = UA_TYPES[UA_TYPES_NODEID].typeId;
    outputArgument.valueRank = -1; /* scalar */

    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US","RegisterApplication");
    mAttr.executable = true;
    mAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(ns_index, 146),
                            directoryTypeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns_index, "RegisterApplication"),
                            mAttr, &registerApplicationMethodCallback,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);

    UA_Server_addReference(server, UA_NODEID_NUMERIC(ns_index, 146),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}

static void
addUnregisterApplicationMethod(UA_Server *server, UA_UInt16 ns_index, UA_NodeId directoryTypeId) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "ApplicationRecordDataType");
    inputArgument.name = UA_STRING("InputArguments");
    inputArgument.dataType = UA_TYPES[UA_TYPES_NODEID].typeId;
    inputArgument.valueRank = -1; /* scalar */

    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US","UnregisterApplication");
    mAttr.executable = true;
    mAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(ns_index, 149),
                            directoryTypeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns_index, "UnregisterApplication"),
                            mAttr, &unregisterApplicationMethodCallback,
                            1, &inputArgument, 0, NULL, NULL, NULL);

    UA_Server_addReference(server, UA_NODEID_NUMERIC(ns_index, 149),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}

static void
addUpdateApplicationMethod(UA_Server *server, UA_UInt16 ns_index, UA_NodeId directoryTypeId) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "ApplicationRecordDataType");
    inputArgument.name = UA_STRING("InputArguments");
    inputArgument.dataType = ApplicationRecordDataType.typeId;
    inputArgument.valueRank = -1; /* scalar */

    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US","UpdateApplication");
    mAttr.executable = true;
    mAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(ns_index, 200),
                            directoryTypeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns_index, "UpdateApplication"),
                            mAttr, NULL,
                            1, &inputArgument, 0, NULL, NULL, NULL);

    UA_Server_addReference(server, UA_NODEID_NUMERIC(ns_index, 200),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}

static void
addGetApplicationMethod(UA_Server *server, UA_UInt16 ns_index, UA_NodeId directoryTypeId) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "applicationId");
    inputArgument.name = UA_STRING("InputArguments");
    inputArgument.dataType = UA_TYPES[UA_TYPES_NODEID].typeId;
    inputArgument.valueRank = -1; /* scalar */

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "application");
    outputArgument.name = UA_STRING("OutputArguments");
    outputArgument.dataType = ApplicationRecordDataType.typeId;
    outputArgument.valueRank = -1; /* scalar */

    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US","GetApplication");
    mAttr.executable = true;
    mAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(ns_index, 216),
                            directoryTypeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns_index, "GetApplication"),
                            mAttr, NULL,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);

    UA_Server_addReference(server, UA_NODEID_NUMERIC(ns_index, 216),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}

static void
addQueryApplicationsMethod(UA_Server *server, UA_UInt16 ns_index, UA_NodeId directoryTypeId) {
    UA_Argument inputArguments[7];

    UA_Argument_init(&inputArguments[0]);
    inputArguments[0].description = UA_LOCALIZEDTEXT("en-US", "startingRecordId");
    inputArguments[0].name = UA_STRING("startingRecordId");
    inputArguments[0].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    inputArguments[0].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[1]);
    inputArguments[1].description = UA_LOCALIZEDTEXT("en-US", "maxRecordsToReturn");
    inputArguments[1].name = UA_STRING("maxRecordsToReturn");
    inputArguments[1].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    inputArguments[1].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[2]);
    inputArguments[2].description = UA_LOCALIZEDTEXT("en-US", "applicationName");
    inputArguments[2].name = UA_STRING("applicationName");
    inputArguments[2].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[2].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[3]);
    inputArguments[3].description = UA_LOCALIZEDTEXT("en-US", "applicationUri");
    inputArguments[3].name = UA_STRING("applicationUri");
    inputArguments[3].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[3].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[4]);
    inputArguments[4].description = UA_LOCALIZEDTEXT("en-US", "applicationType");
    inputArguments[4].name = UA_STRING("applicationType");
    inputArguments[4].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    inputArguments[4].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[5]);
    inputArguments[5].description = UA_LOCALIZEDTEXT("en-US", "productUri");
    inputArguments[5].name = UA_STRING("productUri");
    inputArguments[5].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[5].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[6]);
    inputArguments[6].description = UA_LOCALIZEDTEXT("en-US", "capabilities");
    inputArguments[6].name = UA_STRING("capabilities");
    inputArguments[6].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[6].valueRank = 1;


    UA_Argument outputArguments[3];

    UA_Argument_init(&outputArguments[0]);
    outputArguments[0].description = UA_LOCALIZEDTEXT("en-US", "lastCounterResetTime");
    outputArguments[0].name = UA_STRING("lastCounterResetTime");
    outputArguments[0].dataType = UA_TYPES[UA_TYPES_DATETIME].typeId;
    outputArguments[0].valueRank = -1; /* scalar */

    UA_Argument_init(&outputArguments[1]);
    outputArguments[1].description = UA_LOCALIZEDTEXT("en-US", "nextRecordId");
    outputArguments[1].name = UA_STRING("nextRecordId");
    outputArguments[1].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    outputArguments[1].valueRank = -1; /* scalar */

    UA_Argument_init(&outputArguments[2]);
    outputArguments[2].description = UA_LOCALIZEDTEXT("en-US", "applications");
    outputArguments[2].name = UA_STRING("applications");
    outputArguments[2].dataType = UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION].typeId;
    outputArguments[2].valueRank = 1;

    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US","QueryApplications");
    mAttr.executable = true;
    mAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(ns_index, 992),
                            directoryTypeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns_index, "QueryApplications"),
                            mAttr, NULL,
                            7, inputArguments, 3, outputArguments, NULL, NULL);

    UA_Server_addReference(server, UA_NODEID_NUMERIC(ns_index, 992),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}

static void
addQueryServersMethod(UA_Server *server, UA_UInt16 ns_index, UA_NodeId directoryTypeId) {
    UA_Argument inputArguments[6];

    UA_Argument_init(&inputArguments[0]);
    inputArguments[0].description = UA_LOCALIZEDTEXT("en-US", "startingRecordId");
    inputArguments[0].name = UA_STRING("startingRecordId");
    inputArguments[0].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    inputArguments[0].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[1]);
    inputArguments[1].description = UA_LOCALIZEDTEXT("en-US", "maxRecordsToReturn");
    inputArguments[1].name = UA_STRING("maxRecordsToReturn");
    inputArguments[1].dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    inputArguments[1].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[2]);
    inputArguments[2].description = UA_LOCALIZEDTEXT("en-US", "applicationName");
    inputArguments[2].name = UA_STRING("applicationName");
    inputArguments[2].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[2].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[3]);
    inputArguments[3].description = UA_LOCALIZEDTEXT("en-US", "applicationUri");
    inputArguments[3].name = UA_STRING("applicationUri");
    inputArguments[3].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[3].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[4]);
    inputArguments[4].description = UA_LOCALIZEDTEXT("en-US", "productUri");
    inputArguments[4].name = UA_STRING("productUri");
    inputArguments[4].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[4].valueRank = -1; /* scalar */

    UA_Argument_init(&inputArguments[5]);
    inputArguments[5].description = UA_LOCALIZEDTEXT("en-US", "serverCapabilities");
    inputArguments[5].name = UA_STRING("serverCapabilities");
    inputArguments[5].dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments[5].valueRank = 1;


    UA_Argument outputArguments[2];

    UA_Argument_init(&outputArguments[0]);
    outputArguments[0].description = UA_LOCALIZEDTEXT("en-US", "lastCounterResetTime");
    outputArguments[0].name = UA_STRING("lastCounterResetTime");
    outputArguments[0].dataType = UA_TYPES[UA_TYPES_DATETIME].typeId;
    outputArguments[0].valueRank = -1; /* scalar */

    UA_Argument_init(&outputArguments[1]);
    outputArguments[1].description = UA_LOCALIZEDTEXT("en-US", "servers");
    outputArguments[1].name = UA_STRING("applications");
    outputArguments[1].dataType = UA_TYPES[UA_TYPES_SERVERONNETWORK].typeId;
    outputArguments[1].valueRank = 1;

    UA_MethodAttributes mAttr = UA_MethodAttributes_default;
    mAttr.displayName = UA_LOCALIZEDTEXT("en-US","QueryServers");
    mAttr.executable = true;
    mAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(ns_index, 151),
                            directoryTypeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns_index, "QueryServers"),
                            mAttr, NULL,
                            6, inputArguments, 2, outputArguments, NULL, NULL);

    UA_Server_addReference(server, UA_NODEID_NUMERIC(ns_index, 151),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
}


/**********************************************/
/*                 ObjectTypes                */
/**********************************************/

static void
addDirectoryType(UA_Server *server, UA_UInt16 ns_index){
    UA_NodeId directoryTypeId = UA_NODEID_NUMERIC(ns_index, 13);
    UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DirectoryType");
    UA_Server_addObjectTypeNode(server, directoryTypeId,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                UA_QUALIFIEDNAME(ns_index, "DirectoryType"), dtAttr,
                                NULL, NULL);

    UA_NodeId applicationsId = UA_NODEID_NUMERIC(ns_index, 14);
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Applications");
    UA_Server_addObjectNode(server, applicationsId,
                            directoryTypeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(ns_index, "Applications"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            oAttr, NULL, NULL);

    UA_Server_addReference(server, applicationsId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                           UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);

    addFindApplicationsMethod(server, ns_index, directoryTypeId);

    addRegisterApplicationMethod(server, ns_index, directoryTypeId);

    addUpdateApplicationMethod(server, ns_index, directoryTypeId);

    addUnregisterApplicationMethod(server, ns_index, directoryTypeId);

    addGetApplicationMethod(server, ns_index, directoryTypeId);

    addQueryApplicationsMethod(server, ns_index, directoryTypeId);

    addQueryServersMethod(server, ns_index, directoryTypeId);
}

static void
createDirectoryTypeObject(UA_Server *server, UA_UInt16 ns_index){

    UA_ObjectAttributes directory = UA_ObjectAttributes_default;
    directory.displayName = UA_LOCALIZEDTEXT("en-US", "Directory");

    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(ns_index, 141),
                             UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                             UA_QUALIFIEDNAME(ns_index, "Directory"),
                             UA_NODEID_NUMERIC(ns_index, 13), directory, NULL, NULL);
}


static
UA_DataTypeArray *UA_GDS_DataTypeArray;

static
UA_StatusCode addApplicationRecordDataType(UA_Server *server) {
    UA_DataTypeArray init = { server->config.customDataTypes, 1, &ApplicationRecordDataType};
    UA_GDS_DataTypeArray = (UA_DataTypeArray*) UA_malloc(sizeof(UA_DataTypeArray));
    memcpy(UA_GDS_DataTypeArray, &init, sizeof(init));
    server->config.customDataTypes = UA_GDS_DataTypeArray;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_GDS_deinitNS(UA_Server *server) {
    server->config.customDataTypes = UA_GDS_DataTypeArray->next;
    UA_free(UA_GDS_DataTypeArray);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_GDS_initNS(UA_Server *server) {
    UA_UInt16 ns_index = UA_Server_addNamespace(server, "http://opcfoundation.org/UA/GDS/");
    addApplicationRecordDataType(server);
    //Part 12, page 14
    addDirectoryType(server, ns_index);
    createDirectoryTypeObject(server, ns_index);

    return UA_STATUSCODE_GOOD;

}


#endif