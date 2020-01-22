/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <string.h>
#include <math.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/types_generated_encoding_binary.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include "check.h"
#include "thread_wrapper.h"

UA_Server *server = NULL;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
        while (running)
        UA_Server_run_iterate(server, true);
        return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
        UA_malloc(sizeof(UA_PubSubTransportLayer));
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static UA_NodeId
findSingleChildNode(UA_QualifiedName targetName,
                    UA_NodeId referenceTypeId, UA_NodeId startingNode){
    UA_NodeId resultNodeId;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = referenceTypeId;
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = targetName;
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = startingNode;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    UA_BrowsePathResult bpr =
            UA_Server_translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;
    if(UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId) != UA_STATUSCODE_GOOD){
        UA_BrowsePathResult_deleteMembers(&bpr);
        return UA_NODEID_NULL;
    }
    UA_BrowsePathResult_deleteMembers(&bpr);
    return resultNodeId;
}

static UA_NodeId addPubSubConnection(void){
    UA_Variant publisherId;
    UA_Variant_init(&publisherId);
    UA_UInt32 publisherIdValue = 13245;
    UA_Variant_setScalar(&publisherId, &publisherIdValue , &UA_TYPES[UA_TYPES_UINT32]);

    UA_PubSubConnectionDataType pubSubConnection;
    UA_PubSubConnectionDataType_init(&pubSubConnection);
    pubSubConnection.name = UA_STRING("Model Connection 1");
    pubSubConnection.enabled = UA_TRUE;
    pubSubConnection.publisherId = publisherId;
    pubSubConnection.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");


    UA_ExtensionObject eo;
    UA_NetworkAddressUrlDataType networkAddressDataType = {UA_STRING("eth0"), UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_NetworkAddressUrlDataType* identityToken = UA_NetworkAddressUrlDataType_new();
    UA_NetworkAddressUrlDataType_init(identityToken);
    UA_NetworkAddressUrlDataType_copy(&networkAddressDataType, identityToken);
    eo.encoding = UA_EXTENSIONOBJECT_DECODED;
    eo.content.decoded.type = &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE];
    eo.content.decoded.data = identityToken;
    pubSubConnection.address = eo;
    pubSubConnection.connectionPropertiesSize = 2;
    UA_KeyValuePair connectionOptions[2];
    memset(connectionOptions, 0, sizeof(UA_KeyValuePair)* 2);
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "ttl");
    UA_UInt32 ttl = 10;
    UA_Variant_setScalar(&connectionOptions[0].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "loopback");
    UA_Boolean loopback = UA_FALSE;
    UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_UINT32]);
    pubSubConnection.connectionProperties = connectionOptions;

    UA_Variant inputArguments;
    UA_Variant_init(&inputArguments);
    UA_Variant_setScalar(&inputArguments, &pubSubConnection, &UA_TYPES[UA_TYPES_PUBSUBCONNECTIONDATATYPE]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = &inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_ADDCONNECTION);

    UA_NodeId connectionId = UA_NODEID_NULL;
    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(1, result.outputArgumentsSize);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
    if(result.outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
        connectionId =  *((UA_NodeId *) result.outputArguments->data);
    UA_ExtensionObject_deleteMembers(&eo);
    callMethodRequest.inputArguments = NULL;
    callMethodRequest.inputArgumentsSize = 0;
    UA_CallMethodRequest_deleteMembers(&callMethodRequest);
    UA_CallMethodResult_deleteMembers(&result);
    return connectionId;
}

START_TEST(AddNewPubSubConnectionUsingTheInformationModelMethod){
    UA_StatusCode retVal;
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
    }
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_NodeId createdConnection = addPubSubConnection();
    UA_LocalizedText connectionDisplayName;
    UA_LocalizedText_init(&connectionDisplayName);
    retVal = UA_Server_readDisplayName(server, createdConnection, &connectionDisplayName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_String compareText = UA_STRING("Model Connection 1");
    ck_assert(UA_String_equal(&connectionDisplayName.text, &compareText) == UA_TRUE);
    //todo browse and check childs

    UA_Variant serverPubSubConnectionValues;
    UA_Variant_init(&serverPubSubConnectionValues);
    UA_NodeId connectionPublisherId = findSingleChildNode(UA_QUALIFIEDNAME(0, "PublisherId"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                                      createdConnection);
    ck_assert_int_eq(UA_Server_readValue(server, connectionPublisherId, &serverPubSubConnectionValues),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*((UA_UInt32 *) serverPubSubConnectionValues.data), 13245);
    UA_Variant_deleteMembers(&serverPubSubConnectionValues);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    UA_LocalizedText_deleteMembers(&connectionDisplayName);
    } END_TEST

START_TEST(AddAndRemovePublishedDataSetFolders){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_String folderName = UA_STRING("TestFolder");
        UA_Variant inputArguments;
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &folderName, &UA_TYPES[UA_TYPES_STRING]);

        UA_CallMethodRequest callMethodRequest;
        UA_CallMethodRequest_init(&callMethodRequest);
        callMethodRequest.inputArgumentsSize = 1;
        callMethodRequest.inputArguments = &inputArguments;
        callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS);
        callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDDATASETFOLDER);

        UA_CallMethodResult result;
        UA_CallMethodResult_init(&result);
        result = UA_Server_call(server, &callMethodRequest);
        ck_assert_int_eq(1, result.outputArgumentsSize);
        ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);

        UA_NodeId createdFolder = UA_NODEID_NULL;
        if(result.outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            createdFolder = *((UA_NodeId *) result.outputArguments->data);
        UA_LocalizedText connectionDisplayName;
        UA_LocalizedText_init(&connectionDisplayName);
        retVal = UA_Server_readDisplayName(server, createdFolder, &connectionDisplayName);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_String compareText = UA_STRING("TestFolder");
        ck_assert(UA_String_equal(&connectionDisplayName.text, &compareText) == UA_TRUE);
        retVal = UA_Server_readNodeId(server, createdFolder, &createdFolder);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_CallMethodResult_deleteMembers(&result);
        UA_LocalizedText_deleteMembers(&connectionDisplayName);

        //create folder inside the new folder
        folderName = UA_STRING("TestFolder2");
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &folderName, &UA_TYPES[UA_TYPES_STRING]);
        UA_CallMethodRequest_init(&callMethodRequest);
        callMethodRequest.inputArgumentsSize = 1;
        callMethodRequest.inputArguments = &inputArguments;
        callMethodRequest.objectId = createdFolder;
        callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDDATASETFOLDER);
        UA_CallMethodResult_init(&result);
        result = UA_Server_call(server, &callMethodRequest);
        ck_assert_int_eq(1, result.outputArgumentsSize);
        ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
        UA_NodeId createdFolder2 = UA_NODEID_NULL;
        if(result.outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            createdFolder2 = *((UA_NodeId *) result.outputArguments->data);
        UA_LocalizedText_init(&connectionDisplayName);
        retVal = UA_Server_readDisplayName(server, createdFolder2, &connectionDisplayName);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        compareText = UA_STRING("TestFolder2");
        ck_assert(UA_String_equal(&connectionDisplayName.text, &compareText) == UA_TRUE);
        retVal = UA_Server_readNodeId(server, createdFolder2, &createdFolder2);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_CallMethodResult_deleteMembers(&result);

        //delete the folder
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &createdFolder, &UA_TYPES[UA_TYPES_NODEID]);
        UA_CallMethodRequest_init(&callMethodRequest);
        callMethodRequest.inputArgumentsSize = 1;
        callMethodRequest.inputArguments = &inputArguments;
        callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS);
        callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEDATASETFOLDER);

        result = UA_Server_call(server, &callMethodRequest);
        ck_assert_int_eq(0, result.outputArgumentsSize);
        ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
        //check if the node is deleted
        retVal = UA_Server_readNodeId(server, createdFolder, NULL);
        ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);

        UA_CallMethodResult_deleteMembers(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        UA_LocalizedText_deleteMembers(&connectionDisplayName);
    } END_TEST

START_TEST(AddAndRemovePublishedDataSetItems){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_Variant *inputArguments = (UA_Variant *) UA_calloc(4, (sizeof(UA_Variant)));

        UA_String pdsName = UA_STRING("Test PDS");
        UA_Variant_setScalar(&inputArguments[0], &pdsName, &UA_TYPES[UA_TYPES_STRING]);

        UA_String *fieldNameAliases = (UA_String *) UA_calloc(2, sizeof(UA_String));
        fieldNameAliases[0] = UA_STRING("field1");
        fieldNameAliases[1] = UA_STRING("field2");
        UA_Variant_setArray(&inputArguments[1], fieldNameAliases, 2, &UA_TYPES[UA_TYPES_STRING]);

        UA_DataSetFieldFlags *dataSetFieldFlags = (UA_DataSetFieldFlags *) UA_calloc(2, sizeof(UA_DataSetFieldFlags));
        dataSetFieldFlags[0] = UA_DATASETFIELDFLAGS_PROMOTEDFIELD;
        dataSetFieldFlags[1] = UA_DATASETFIELDFLAGS_PROMOTEDFIELD;
        UA_Variant_setArray(&inputArguments[2], dataSetFieldFlags, 2, &UA_TYPES[UA_TYPES_DATASETFIELDFLAGS]);

        UA_PublishedVariableDataType *variablesToAdd = (UA_PublishedVariableDataType *) UA_calloc(2, sizeof(UA_PublishedVariableDataType));
        variablesToAdd[0].publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_LOCALTIME);
        variablesToAdd[0].attributeId = UA_ATTRIBUTEID_VALUE;
        variablesToAdd[1].publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERREDUNDANCY_CURRENTSERVERID);
        variablesToAdd[1].attributeId = UA_ATTRIBUTEID_VALUE;
        UA_Variant_setArray(&inputArguments[3], variablesToAdd, 2, &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);

        UA_CallMethodRequest callMethodRequest;
        UA_CallMethodRequest_init(&callMethodRequest);
        callMethodRequest.inputArgumentsSize = 4;
        callMethodRequest.inputArguments = inputArguments;
        callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS);
        callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS);

        UA_CallMethodResult result;
        UA_CallMethodResult_init(&result);
        result = UA_Server_call(server, &callMethodRequest);
        ck_assert_int_eq(3, result.outputArgumentsSize);
        ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);

        //TODO checked correctness of created items
        UA_CallMethodResult_deleteMembers(&result);
        UA_free(inputArguments);
        UA_free(fieldNameAliases);
        UA_free(dataSetFieldFlags);
        UA_free(variablesToAdd);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
} END_TEST

START_TEST(AddAndRemoveWriterGroups){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_NodeId createdConnection = addPubSubConnection();

        UA_Variant *inputArgument = (UA_Variant *) UA_calloc(1, (sizeof(UA_Variant)));
        UA_WriterGroupDataType writerGroupDataType;
        UA_WriterGroupDataType_init(&writerGroupDataType);
        writerGroupDataType.name = UA_STRING("TestWriterGroup");
        writerGroupDataType.enabled = UA_TRUE;
        writerGroupDataType.publishingInterval = 500;
        writerGroupDataType.writerGroupId = 1234;
        UA_Variant_setScalar(inputArgument, &writerGroupDataType, &UA_TYPES[UA_TYPES_WRITERGROUPDATATYPE]);

        UA_CallMethodRequest callMethodRequest;
        UA_CallMethodRequest_init(&callMethodRequest);
        callMethodRequest.inputArgumentsSize = 1;
        callMethodRequest.inputArguments = inputArgument;
        callMethodRequest.objectId = createdConnection;
        callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDWRITERGROUP);

        UA_CallMethodResult result;
        UA_CallMethodResult_init(&result);
        result = UA_Server_call(server, &callMethodRequest);
        ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(1, result.outputArgumentsSize);

        UA_NodeId createdWriterGroup = UA_NODEID_NULL;
        if(result.outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            createdWriterGroup = *((UA_NodeId *) result.outputArguments->data);
        UA_LocalizedText writerGroupDisplayName;
        UA_LocalizedText_init(&writerGroupDisplayName);
        retVal = UA_Server_readDisplayName(server, createdWriterGroup, &writerGroupDisplayName);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_String compareText = UA_STRING("TestWriterGroup");
        ck_assert(UA_String_equal(&writerGroupDisplayName.text, &compareText) == UA_TRUE);
        UA_free(inputArgument);
        UA_CallMethodResult_deleteMembers(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        UA_LocalizedText_deleteMembers(&writerGroupDisplayName);
} END_TEST

int main(void) {
    TCase *tc_add_pubsub_informationmodel_methods_connection = tcase_create("PubSub connection delete and creation using the information model methods");
    tcase_add_checked_fixture(tc_add_pubsub_informationmodel_methods_connection, setup, teardown);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddNewPubSubConnectionUsingTheInformationModelMethod);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemovePublishedDataSetFolders);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemovePublishedDataSetItems);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemoveWriterGroups);
    //TODO TestCase add publishedDataItems and removePublishedDataItems, writergroup remove

    Suite *s = suite_create("PubSub CRUD configuration by the information model functions");
    suite_add_tcase(s, tc_add_pubsub_informationmodel_methods_connection);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
