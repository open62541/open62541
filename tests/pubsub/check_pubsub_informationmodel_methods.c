/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2020-2021 Kalycito Infotech Private Limited
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>

#include "test_helpers.h"
#include "check.h"
#include "thread_wrapper.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>

UA_Server *server = NULL;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
    while(running) {
        UA_Server_run_iterate(server, false);
    }
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
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
        UA_BrowsePathResult_clear(&bpr);
        return UA_NODEID_NULL;
    }
    UA_BrowsePathResult_clear(&bpr);
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
    pubSubConnection.publisherId = publisherId;
    pubSubConnection.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    UA_ExtensionObject eo;
    UA_NetworkAddressUrlDataType networkAddressDataType = {UA_STRING(""), UA_STRING("opc.udp://224.0.0.22:4840/")};
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
    UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
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
    ck_assert_uint_eq(1, result.outputArgumentsSize);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
    if(result.outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
        connectionId =  *((UA_NodeId *) result.outputArguments->data);
    UA_ExtensionObject_clear(&eo);
    callMethodRequest.inputArguments = NULL;
    callMethodRequest.inputArgumentsSize = 0;
    UA_CallMethodRequest_clear(&callMethodRequest);
    UA_CallMethodResult_clear(&result);
    return connectionId;
}

static void addPublishedDataSets(void){
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
    variablesToAdd[0].publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    variablesToAdd[0].attributeId = UA_ATTRIBUTEID_VALUE;
    variablesToAdd[1].publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
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
    ck_assert_uint_eq(3, result.outputArgumentsSize);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_CallMethodResult_clear(&result);
    UA_free(inputArguments);
    UA_free(fieldNameAliases);
    UA_free(dataSetFieldFlags);
    UA_free(variablesToAdd);
}

static UA_StatusCode CallReserveIds(UA_Client *client, UA_String *transportProfileUri,
    UA_UInt16 numRegWriterGroupIds, UA_UInt16 numRegDataSetWriterIds,
    UA_Variant* defaultPublisherId, UA_Variant* regWriterGroupIds, UA_Variant *regDataSetWriterIds)
{
    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    UA_Variant *inputArguments = (UA_Variant *) UA_calloc(3, (sizeof(UA_Variant)));
    UA_Variant_setScalar(&inputArguments[0], transportProfileUri, &UA_TYPES[UA_TYPES_STRING]);
    UA_Variant_setScalar(&inputArguments[1], &numRegWriterGroupIds, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setScalar(&inputArguments[2], &numRegDataSetWriterIds, &UA_TYPES[UA_TYPES_UINT16]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 3;
    callMethodRequest.inputArguments = inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBSUBCONFIGURATION);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBSUBCONFIGURATION_RESERVEIDS);

    UA_CallRequest callReserveIds;
    UA_CallRequest_init(&callReserveIds);
    callReserveIds.methodsToCallSize = 1;
    callReserveIds.methodsToCall = &callMethodRequest;

    UA_CallResponse response = UA_Client_Service_call(client, callReserveIds);
    ck_assert_uint_eq(1, response.resultsSize);
    ck_assert_int_eq(response.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(3, response.results[0].outputArgumentsSize);
    ret = response.results->statusCode;

    UA_Variant_copy(&response.results[0].outputArguments[0], defaultPublisherId);
    UA_Variant_copy(&response.results[0].outputArguments[1], regWriterGroupIds);
    UA_Variant_copy(&response.results[0].outputArguments[2], regDataSetWriterIds);

    UA_free(inputArguments);
    UA_CallResponse_clear(&response);

    return ret;
}

START_TEST(AddandRemoveNewPubSubConnectionWithWriterGroup){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Variant publisherId;
        UA_Variant_init(&publisherId);
        UA_UInt32 publisherIdValue = 100;
        UA_Variant_setScalar(&publisherId, &publisherIdValue , &UA_TYPES[UA_TYPES_UINT32]);

        UA_PubSubConnectionDataType pubSubConnection;
        UA_PubSubConnectionDataType_init(&pubSubConnection);
        pubSubConnection.name = UA_STRING("Model Connection 2");
        pubSubConnection.publisherId = publisherId;
        pubSubConnection.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

        UA_ExtensionObject eo;
        UA_NetworkAddressUrlDataType networkAddressDataType = {UA_STRING(""), UA_STRING("opc.udp://224.0.0.22:4840/")};
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
        UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
        pubSubConnection.connectionProperties = connectionOptions;

        pubSubConnection.writerGroupsSize = 1;
        pubSubConnection.writerGroups = (UA_WriterGroupDataType *)UA_calloc(pubSubConnection.writerGroupsSize, sizeof(UA_WriterGroupDataType));
        UA_UadpWriterGroupMessageDataType *writerGroupMessage = \
            (UA_UadpWriterGroupMessageDataType *)UA_calloc(pubSubConnection.writerGroupsSize, sizeof(UA_UadpWriterGroupMessageDataType));
        UA_ExtensionObject extensionObjectWG;
        pubSubConnection.writerGroups->name = UA_STRING("WriterGroup 1");
        pubSubConnection.writerGroups->publishingInterval = 5;
        pubSubConnection.writerGroups->writerGroupId = 150;
        /* Change message settings of writerGroup to send PublisherId,
        * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
        * of NetworkMessage */
        writerGroupMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        extensionObjectWG.encoding = UA_EXTENSIONOBJECT_DECODED;
        extensionObjectWG.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        extensionObjectWG.content.decoded.data = writerGroupMessage;
        pubSubConnection.writerGroups->messageSettings = extensionObjectWG;

        UA_Variant inputArguments;
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &pubSubConnection, &UA_TYPES[UA_TYPES_PUBSUBCONNECTIONDATATYPE]);

        //Call method from client
        UA_CallRequest callMethodRequestFromClient;
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest item;
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_ADDCONNECTION);
        item.inputArguments = (UA_Variant*)&inputArguments;
        item.inputArgumentsSize = 1;

        UA_NodeId connectionId = UA_NODEID_NULL;
        UA_CallResponse response;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(1, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);
        if(response.results->outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            connectionId =  *((UA_NodeId *) response.results->outputArguments->data);
        UA_CallResponse_clear(&response);

        //Remove the connection
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &connectionId, &UA_TYPES[UA_TYPES_NODEID]);
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest_init(&item);
        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_REMOVECONNECTION);
        item.inputArguments = (UA_Variant*)&inputArguments;
        item.inputArgumentsSize = 1;

        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(0, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);

        UA_ExtensionObject_clear(&eo);
        UA_free(writerGroupMessage);
        UA_free(pubSubConnection.writerGroups);
        UA_CallResponse_clear(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
}END_TEST

START_TEST(AddNewPubSubConnectionWithWriterGroupAndDataSetWriter){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        addPublishedDataSets();
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Variant publisherId;
        UA_Variant_init(&publisherId);
        UA_UInt32 publisherIdValue = 100;
        UA_Variant_setScalar(&publisherId, &publisherIdValue , &UA_TYPES[UA_TYPES_UINT32]);

        UA_PubSubConnectionDataType pubSubConnection;
        UA_PubSubConnectionDataType_init(&pubSubConnection);
        pubSubConnection.name = UA_STRING("Model Connection 2");
        pubSubConnection.publisherId = publisherId;
        pubSubConnection.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

        UA_ExtensionObject eo;
        UA_NetworkAddressUrlDataType networkAddressDataType = {UA_STRING(""), UA_STRING("opc.udp://224.0.0.22:4840/")};
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
        UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
        pubSubConnection.connectionProperties = connectionOptions;

        pubSubConnection.writerGroupsSize = 1;
        pubSubConnection.writerGroups = (UA_WriterGroupDataType *)UA_calloc(pubSubConnection.writerGroupsSize, sizeof(UA_WriterGroupDataType));
        UA_UadpWriterGroupMessageDataType *writerGroupMessage = \
            (UA_UadpWriterGroupMessageDataType *)UA_calloc(pubSubConnection.writerGroupsSize, sizeof(UA_UadpWriterGroupMessageDataType));
        UA_ExtensionObject extensionObjectWG;
        pubSubConnection.writerGroups->name = UA_STRING("WriterGroup 1");
        pubSubConnection.writerGroups->publishingInterval = 5;
        pubSubConnection.writerGroups->writerGroupId = 150;
        pubSubConnection.writerGroups->dataSetWritersSize = 1;
        /* Change message settings of writerGroup to send PublisherId,
        * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
        * of NetworkMessage */
        writerGroupMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        extensionObjectWG.encoding = UA_EXTENSIONOBJECT_DECODED;
        extensionObjectWG.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        extensionObjectWG.content.decoded.data = writerGroupMessage;
        pubSubConnection.writerGroups->messageSettings = extensionObjectWG;

        pubSubConnection.writerGroups->dataSetWriters = \
                (UA_DataSetWriterDataType*)UA_calloc(pubSubConnection.writerGroups->dataSetWritersSize, sizeof(UA_DataSetWriterDataType));
        pubSubConnection.writerGroups->dataSetWriters->name = UA_STRING("DataSetWriter");
        pubSubConnection.writerGroups->dataSetWriters->dataSetWriterId = 62541;
        pubSubConnection.writerGroups->dataSetWriters->keyFrameCount = 10;
        UA_String pdsName = UA_STRING("Test PDS");
        pubSubConnection.writerGroups->dataSetWriters->dataSetName = pdsName;

        UA_Variant inputArguments;
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &pubSubConnection, &UA_TYPES[UA_TYPES_PUBSUBCONNECTIONDATATYPE]);

        //Call method from client
        UA_CallRequest callMethodRequestFromClient;
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest item;
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_ADDCONNECTION);
        item.inputArguments = (UA_Variant*)&inputArguments;
        item.inputArgumentsSize = 1;

        UA_CallResponse response;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(1, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);
        UA_ExtensionObject_clear(&eo);
        UA_free(pubSubConnection.writerGroups->dataSetWriters);
        UA_free(writerGroupMessage);
        UA_free(pubSubConnection.writerGroups);
        UA_CallResponse_clear(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
}END_TEST


START_TEST(AddNewPubSubConnectionUsingTheInformationModelMethod){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
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
        UA_Variant_clear(&serverPubSubConnectionValues);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        UA_LocalizedText_clear(&connectionDisplayName);
} END_TEST

START_TEST(AddAndRemovePublishedDataSetFoldersUsingServer){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
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
        ck_assert_uint_eq(1, result.outputArgumentsSize);
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
        UA_CallMethodResult_clear(&result);
        UA_LocalizedText_clear(&connectionDisplayName);

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
        ck_assert_uint_eq(1, result.outputArgumentsSize);
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
        UA_CallMethodResult_clear(&result);

        //delete the folder
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &createdFolder, &UA_TYPES[UA_TYPES_NODEID]);
        UA_CallMethodRequest_init(&callMethodRequest);
        callMethodRequest.inputArgumentsSize = 1;
        callMethodRequest.inputArguments = &inputArguments;
        callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS);
        callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEDATASETFOLDER);

        result = UA_Server_call(server, &callMethodRequest);
        ck_assert_uint_eq(0, result.outputArgumentsSize);
        ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
        //check if the node is deleted
        retVal = UA_Server_readNodeId(server, createdFolder, NULL);
        ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);

        UA_CallMethodResult_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        UA_LocalizedText_clear(&connectionDisplayName);
    } END_TEST

START_TEST(AddAndRemovePublishedDataSetFoldersUsingClient){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_String folderName = UA_STRING("TestFolder");
        UA_Variant inputArguments;
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &folderName, &UA_TYPES[UA_TYPES_STRING]);

        //Call method from client
        UA_CallRequest callMethodRequestFromClient;
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest item;
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS);
        item.methodId =  UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDDATASETFOLDER);
        item.inputArguments = (UA_Variant*)&inputArguments;
        item.inputArgumentsSize = 1;

        UA_CallResponse response;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(1, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);

        UA_NodeId createdFolder = UA_NODEID_NULL;
        if(response.results->outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            createdFolder = *((UA_NodeId *) response.results->outputArguments->data);
        UA_LocalizedText connectionDisplayName;
        UA_LocalizedText_init(&connectionDisplayName);
        retVal = UA_Server_readDisplayName(server, createdFolder, &connectionDisplayName);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_String compareText = UA_STRING("TestFolder");
        ck_assert(UA_String_equal(&connectionDisplayName.text, &compareText) == UA_TRUE);
        retVal = UA_Server_readNodeId(server, createdFolder, &createdFolder);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_CallResponse_clear(&response);
        UA_LocalizedText_clear(&connectionDisplayName);

        //create folder inside the new folder
        folderName = UA_STRING("TestFolder2");
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &folderName, &UA_TYPES[UA_TYPES_STRING]);
        //Call method from client
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = createdFolder;
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDDATASETFOLDER);
        item.inputArguments = (UA_Variant*)&inputArguments;
        item.inputArgumentsSize = 1;

        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(1, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);

        UA_NodeId createdFolder2 = UA_NODEID_NULL;
        if(response.results->outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            createdFolder2 = *((UA_NodeId *) response.results->outputArguments->data);
        UA_LocalizedText_init(&connectionDisplayName);
        retVal = UA_Server_readDisplayName(server, createdFolder2, &connectionDisplayName);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        compareText = UA_STRING("TestFolder2");
        ck_assert(UA_String_equal(&connectionDisplayName.text, &compareText) == UA_TRUE);
        retVal = UA_Server_readNodeId(server, createdFolder2, &createdFolder2);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_CallResponse_clear(&response);

        //delete the folder
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &createdFolder, &UA_TYPES[UA_TYPES_NODEID]);

        //Call method from client
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS);
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEDATASETFOLDER);
        item.inputArguments = (UA_Variant*)&inputArguments;
        item.inputArgumentsSize = 1;

        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(0, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);

        //check if the node is deleted
        retVal = UA_Server_readNodeId(server, createdFolder, NULL);
        ck_assert_int_eq(retVal, UA_STATUSCODE_BADNODEIDUNKNOWN);

        UA_CallResponse_clear(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        UA_LocalizedText_clear(&connectionDisplayName);
} END_TEST

START_TEST(AddAndRemovePublishedDataSetItemsUsingServer){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
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
        variablesToAdd[0].publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        variablesToAdd[0].attributeId = UA_ATTRIBUTEID_VALUE;
        variablesToAdd[1].publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
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
        ck_assert_uint_eq(3, result.outputArgumentsSize);
        ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);

        //TODO checked correctness of created items
        UA_CallMethodResult_clear(&result);
        UA_free(inputArguments);
        UA_free(fieldNameAliases);
        UA_free(dataSetFieldFlags);
        UA_free(variablesToAdd);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
} END_TEST

START_TEST(AddAndRemovePublishedDataSetItemsUsingClient){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
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
        variablesToAdd[0].publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
        variablesToAdd[0].attributeId = UA_ATTRIBUTEID_VALUE;
        variablesToAdd[1].publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
        variablesToAdd[1].attributeId = UA_ATTRIBUTEID_VALUE;
        UA_Variant_setArray(&inputArguments[3], variablesToAdd, 2, &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);

        //Call method from client
        UA_CallRequest callMethodRequestFromClient;
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest item;
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS);
        item.methodId =  UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS);
        item.inputArguments = (UA_Variant*)inputArguments;
        item.inputArgumentsSize = 4;

        UA_NodeId dataSetItemsNodeId;
        UA_CallResponse response;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(3, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);
        if(response.results->outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            dataSetItemsNodeId = *((UA_NodeId *) response.results->outputArguments->data);
        UA_CallResponse_clear(&response);

        //Remove the publishedDataItems
        UA_Variant inputArgument;
        UA_Variant_init(&inputArgument);
        UA_Variant_setScalar(&inputArgument, &dataSetItemsNodeId, &UA_TYPES[UA_TYPES_NODEID]);
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest_init(&item);
        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBLISHEDDATASETS);
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET);
        item.inputArguments = (UA_Variant*)&inputArgument;
        item.inputArgumentsSize = 1;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(0, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);

        UA_CallResponse_clear(&response);
        UA_free(inputArguments);
        UA_free(fieldNameAliases);
        UA_free(dataSetFieldFlags);
        UA_free(variablesToAdd);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
} END_TEST

START_TEST(AddAndRemoveWriterGroupsUsingServer){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
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
        ck_assert_uint_eq(1, result.outputArgumentsSize);

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
        UA_CallMethodResult_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        UA_LocalizedText_clear(&writerGroupDisplayName);
} END_TEST

START_TEST(AddAndRemoveWriterGroupsUsingClient){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
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
        writerGroupDataType.publishingInterval = 500;
        writerGroupDataType.writerGroupId = 1234;
        UA_Variant_setScalar(inputArgument, &writerGroupDataType, &UA_TYPES[UA_TYPES_WRITERGROUPDATATYPE]);

        //Call method from client
        UA_CallRequest callMethodRequestFromClient;
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest item;
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = createdConnection;
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDWRITERGROUP);
        item.inputArguments = (UA_Variant*)inputArgument;
        item.inputArgumentsSize = 1;

        UA_CallResponse response;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(1, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);

        UA_NodeId createdWriterGroup = UA_NODEID_NULL;
        if(response.results->outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            createdWriterGroup = *((UA_NodeId *) response.results->outputArguments->data);
        UA_LocalizedText writerGroupDisplayName;
        UA_LocalizedText_init(&writerGroupDisplayName);
        retVal = UA_Server_readDisplayName(server, createdWriterGroup, &writerGroupDisplayName);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_String compareText = UA_STRING("TestWriterGroup");
        ck_assert(UA_String_equal(&writerGroupDisplayName.text, &compareText) == UA_TRUE);
        UA_CallResponse_clear(&response);

        //Remove the Writergroup
        UA_Variant_init(inputArgument);
        UA_Variant_setScalar(inputArgument, &createdWriterGroup, &UA_TYPES[UA_TYPES_NODEID]);
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest_init(&item);
        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = createdConnection;
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_REMOVEGROUP);
        item.inputArguments = (UA_Variant*)inputArgument;
        item.inputArgumentsSize = 1;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);
        UA_free(inputArgument);

        UA_CallResponse_clear(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        UA_LocalizedText_clear(&writerGroupDisplayName);
} END_TEST

START_TEST(AddNewPubSubConnectionWithReaderGroup){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Variant publisherId;
        UA_Variant_init(&publisherId);
        UA_UInt32 publisherIdValue = 100;
        UA_Variant_setScalar(&publisherId, &publisherIdValue , &UA_TYPES[UA_TYPES_UINT32]);

        UA_PubSubConnectionDataType pubSubConnection;
        UA_PubSubConnectionDataType_init(&pubSubConnection);
        pubSubConnection.name = UA_STRING("Model Connection 2");
        pubSubConnection.publisherId = publisherId;
        pubSubConnection.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

        UA_ExtensionObject eo;
        UA_NetworkAddressUrlDataType networkAddressDataType = {UA_STRING(""), UA_STRING("opc.udp://224.0.0.22:4840/")};
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
        UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
        pubSubConnection.connectionProperties = connectionOptions;
        pubSubConnection.readerGroupsSize = 1;
        pubSubConnection.readerGroups = \
            (UA_ReaderGroupDataType *)UA_calloc(pubSubConnection.readerGroupsSize, sizeof(UA_ReaderGroupDataType));
        pubSubConnection.readerGroups->name = UA_STRING("TestReaderGroup");

        UA_Variant inputArguments;
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &pubSubConnection, &UA_TYPES[UA_TYPES_PUBSUBCONNECTIONDATATYPE]);

        //Call method from client
        UA_CallRequest callMethodRequestFromClient;
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest item;
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_ADDCONNECTION);
        item.inputArguments = (UA_Variant*)&inputArguments;
        item.inputArgumentsSize = 1;

        UA_CallResponse response;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(1, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);
        UA_ExtensionObject_clear(&eo);
        UA_free(pubSubConnection.readerGroups);
        UA_CallResponse_clear(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
} END_TEST

START_TEST(AddNewPubSubConnectionWithReaderGroupandDataSetReader){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Variant publisherId;
        UA_Variant_init(&publisherId);
        UA_UInt32 publisherIdValue = 100;
        UA_Variant_setScalar(&publisherId, &publisherIdValue , &UA_TYPES[UA_TYPES_UINT32]);

        UA_PubSubConnectionDataType pubSubConnection;
        UA_PubSubConnectionDataType_init(&pubSubConnection);
        pubSubConnection.name = UA_STRING("Model Connection 2");
        pubSubConnection.publisherId = publisherId;
        pubSubConnection.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

        UA_ExtensionObject eo;
        UA_NetworkAddressUrlDataType networkAddressDataType = {UA_STRING(""), UA_STRING("opc.udp://224.0.0.22:4840/")};
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
        UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
        pubSubConnection.connectionProperties = connectionOptions;
        pubSubConnection.readerGroupsSize = 1;
        pubSubConnection.readerGroups = \
            (UA_ReaderGroupDataType *)UA_calloc(pubSubConnection.readerGroupsSize, sizeof(UA_ReaderGroupDataType));
        UA_TargetVariablesDataType targetVars;
        pubSubConnection.readerGroups->name = UA_STRING("TestReaderGroup");
        pubSubConnection.readerGroups->dataSetReadersSize = 1;
        pubSubConnection.readerGroups->dataSetReaders = \
                (UA_DataSetReaderDataType*)UA_calloc(pubSubConnection.readerGroups->dataSetReadersSize, sizeof(UA_DataSetReaderDataType));
        pubSubConnection.readerGroups->dataSetReaders->name = UA_STRING("DataReader");
        pubSubConnection.readerGroups->dataSetReaders->publisherId = publisherId;
        pubSubConnection.readerGroups->dataSetReaders->dataSetWriterId = 62541;
        pubSubConnection.readerGroups->dataSetReaders->writerGroupId = 150;
        pubSubConnection.readerGroups->dataSetReaders->dataSetMetaData.name = UA_STRING("DataSet1");
        UA_DataSetMetaDataType *pMetaData = &pubSubConnection.readerGroups->dataSetReaders->dataSetMetaData;
        UA_DataSetMetaDataType_init (pMetaData);
        pMetaData->name = UA_STRING ("DataSet 1");

        /* Static definition of number of fields size to 4 to create four different
        * targetVariables of distinct datatype
        * Currently the publisher sends only DateTime data type */
        UA_FieldMetaData fields[4] = {0};
        pMetaData->fieldsSize = 4;
        pMetaData->fields = fields;

        /* DateTime DataType */
        UA_FieldMetaData_init (&pMetaData->fields[0]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
                        &pMetaData->fields[0].dataType);
        pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
        pMetaData->fields[0].name =  UA_STRING ("DateTime");
        pMetaData->fields[0].valueRank = -1; /* scalar */

        /* Int32 DataType */
        UA_FieldMetaData_init (&pMetaData->fields[1]);
        UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId,
                    &pMetaData->fields[1].dataType);
        pMetaData->fields[1].builtInType = UA_NS0ID_INT32;
        pMetaData->fields[1].name =  UA_STRING ("Int 32");
        pMetaData->fields[1].valueRank = -1; /* scalar */

        /* Int64 DataType */
        UA_FieldMetaData_init (&pMetaData->fields[2]);
        UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT64].typeId,
                    &pMetaData->fields[2].dataType);
        pMetaData->fields[2].builtInType = UA_NS0ID_INT64;
        pMetaData->fields[2].name =  UA_STRING ("Int64");
        pMetaData->fields[2].valueRank = -1; /* scalar */

        /* Boolean DataType */
        UA_FieldMetaData_init (&pMetaData->fields[3]);
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                        &pMetaData->fields[3].dataType);
        pMetaData->fields[3].builtInType = UA_NS0ID_BOOLEAN;
        pMetaData->fields[3].name =  UA_STRING ("BoolToggle");
        pMetaData->fields[3].valueRank = -1; /* scalar */
        targetVars.targetVariablesSize = 4;
        targetVars.targetVariables = (UA_FieldTargetDataType *)
            UA_calloc(targetVars.targetVariablesSize, sizeof(UA_FieldTargetDataType));
        UA_ExtensionObject extensionObjectTargetVars;
        for(size_t i = 0; i < targetVars.targetVariablesSize; i++) {
            /* For creating Targetvariables */
            UA_FieldTargetDataType_init(&targetVars.targetVariables[i]);
            targetVars.targetVariables[i].attributeId  = UA_ATTRIBUTEID_VALUE;
            targetVars.targetVariables[i].targetNodeId = UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000);
            extensionObjectTargetVars.encoding = UA_EXTENSIONOBJECT_DECODED;
            extensionObjectTargetVars.content.decoded.type = &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE];
        }
        extensionObjectTargetVars.content.decoded.data = &targetVars;
        pubSubConnection.readerGroups->dataSetReaders->subscribedDataSet = extensionObjectTargetVars;
        UA_Variant inputArguments;
        UA_Variant_init(&inputArguments);
        UA_Variant_setScalar(&inputArguments, &pubSubConnection, &UA_TYPES[UA_TYPES_PUBSUBCONNECTIONDATATYPE]);

        //Call method from client
        UA_CallRequest callMethodRequestFromClient;
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest item;
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_ADDCONNECTION);
        item.inputArguments = (UA_Variant*)&inputArguments;
        item.inputArgumentsSize = 1;

        UA_CallResponse response;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(1, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);
        UA_ExtensionObject_clear(&eo);
        UA_free(targetVars.targetVariables);
        UA_free(pubSubConnection.readerGroups->dataSetReaders);
        UA_free(pubSubConnection.readerGroups);
        UA_CallResponse_clear(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
} END_TEST

START_TEST(AddandRemoveReaderGroup){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_NodeId createdConnection = addPubSubConnection();

        UA_Variant *inputArgument = (UA_Variant *) UA_calloc(1, (sizeof(UA_Variant)));
        UA_ReaderGroupDataType readerGroupDataType;
        UA_ReaderGroupDataType_init(&readerGroupDataType);
        readerGroupDataType.name = UA_STRING("TestReaderGroup");
        UA_Variant_setScalar(inputArgument, &readerGroupDataType, &UA_TYPES[UA_TYPES_READERGROUPDATATYPE]);

        //Call method from client
        UA_CallRequest callMethodRequestFromClient;
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest item;
        UA_CallMethodRequest_init(&item);

        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = createdConnection;
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_ADDREADERGROUP);
        item.inputArguments = (UA_Variant*)inputArgument;
        item.inputArgumentsSize = 1;

        UA_CallResponse response;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_uint_eq(1, response.results->outputArgumentsSize);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);

        UA_NodeId createdReaderGroup = UA_NODEID_NULL;
        if(response.results->outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
            createdReaderGroup = *((UA_NodeId *) response.results->outputArguments->data);
        UA_LocalizedText readerGroupDisplayName;
        UA_LocalizedText_init(&readerGroupDisplayName);
        retVal = UA_Server_readDisplayName(server, createdReaderGroup, &readerGroupDisplayName);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_String compareText = UA_STRING("TestReaderGroup");
        ck_assert(UA_String_equal(&readerGroupDisplayName.text, &compareText) == UA_TRUE);
        UA_CallResponse_clear(&response);

        //Remove the ReaderGroup
        UA_Variant_init(inputArgument);
        UA_Variant_setScalar(inputArgument, &createdReaderGroup, &UA_TYPES[UA_TYPES_NODEID]);
        UA_CallRequest_init(&callMethodRequestFromClient);
        UA_CallMethodRequest_init(&item);
        callMethodRequestFromClient.methodsToCall = &item;
        callMethodRequestFromClient.methodsToCallSize = 1;
        item.objectId = createdConnection;
        item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBSUBCONNECTIONTYPE_REMOVEGROUP);
        item.inputArguments = (UA_Variant*)inputArgument;
        item.inputArgumentsSize = 1;
        response = UA_Client_Service_call(client, callMethodRequestFromClient);
        ck_assert_int_eq(response.results->statusCode, UA_STATUSCODE_GOOD);
        UA_free(inputArgument);

        UA_CallResponse_clear(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        UA_LocalizedText_clear(&readerGroupDisplayName);
} END_TEST

START_TEST(ReserveIdsMultipleTimes){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_Variant defaultPublisherId;
        const UA_UInt16 numRegWriterGroupIds = 3;
        const UA_UInt16 numRegDataSetWriterIds = 3;
        const UA_UInt16 firstId = 0x8000;
        const UA_UInt16 lastId = UA_UINT16_MAX;

        UA_Variant regWriterGroupIds;
        UA_Variant regDataSetWriterIds;
        UA_String transportProfileUri =
            UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

        /* Call ReserveIds 1st time - reserve some IDs. */
        retVal = CallReserveIds(client, &transportProfileUri, numRegWriterGroupIds, numRegDataSetWriterIds,
            &defaultPublisherId, &regWriterGroupIds, &regDataSetWriterIds);
        ck_assert(UA_StatusCode_isGood(retVal));

        /* Create PubSubConfiguration using some of the reserved IDs. */
        UA_PubSubConnectionConfig connectionConfig;
        memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
        connectionConfig.name = UA_STRING("TestConnection");
        connectionConfig.transportProfileUri = transportProfileUri;
        UA_NetworkAddressUrlDataType networkAddressUrl;
        UA_NetworkAddressUrlDataType_init(&networkAddressUrl);
        networkAddressUrl.url = UA_STRING("opc.udp://224.0.0.22:4840");
        UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                            &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
        UA_NodeId connectionNodeId, writerGroupNodeId, dataSetWriterNodeId, publishedDataSetNodeId;
        retVal |= UA_Server_addPubSubConnection (server, &connectionConfig, &connectionNodeId);

        UA_WriterGroupConfig writerGroupConfig;
        memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
        writerGroupConfig.name = UA_STRING("TestWriterGroup");
        writerGroupConfig.writerGroupId = ((UA_UInt16 *)regWriterGroupIds.data)[0]; // Here use the reserved WriterGroupId

        retVal |= UA_Server_addWriterGroup(server, connectionNodeId, &writerGroupConfig, &writerGroupNodeId);

        UA_PublishedDataSetConfig publishedDataSetConfig;
        memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
        publishedDataSetConfig.name = UA_STRING("TestPublishedDataSet");
        retVal |= UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetNodeId).addResult;

        UA_DataSetWriterConfig dataSetWriterConfig;
        memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
        dataSetWriterConfig.name = UA_STRING("TestDataSetWriter");
        dataSetWriterConfig.dataSetWriterId = ((UA_UInt16 *)regDataSetWriterIds.data)[0]; // Here use the reserved DataSetWriterId
        retVal |= UA_Server_addDataSetWriter(server, writerGroupNodeId, publishedDataSetNodeId, &dataSetWriterConfig, &dataSetWriterNodeId);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        UA_Variant_clear(&defaultPublisherId);
        UA_Variant_clear(&regWriterGroupIds);
        UA_Variant_clear(&regDataSetWriterIds);

        /* Call ReserveIds 2nd time within the same session and try to reserve
         * as many IDs as needed to consume the whole pool (and roll over the ID counter). */
        retVal = CallReserveIds(client, &transportProfileUri,
            (UA_UInt16)(lastId - firstId - numRegWriterGroupIds + 1),
            (UA_UInt16)(lastId - firstId - numRegDataSetWriterIds + 1),
            &defaultPublisherId, &regWriterGroupIds, &regDataSetWriterIds);
        ck_assert(UA_StatusCode_isGood(retVal));

        UA_Variant_clear(&defaultPublisherId);
        UA_Variant_clear(&regWriterGroupIds);
        UA_Variant_clear(&regDataSetWriterIds);

        /* Call ReserveIds 3rd time within the same session and try to reserve one more ID from each category -
         * check if the already reserved IDs are blocked. */

        /* Currently the return value is not checked, once the lower assert statement is re-inserted,
         * the return value can also be saved. Otherwise there will be an error in the Clang static analyzer
         * (warning: value stored in 'retVal' is never read). */
        CallReserveIds(client, &transportProfileUri, 1, 1,
            &defaultPublisherId, &regWriterGroupIds, &regDataSetWriterIds);
        /* TODO: Currently Part 14 doesn't define what should happen if the ID cannot be reserved.
         * There is an open Mantis issue #8415. */
//        ck_assert(UA_StatusCode_isBad(retVal));
        ck_assert_uint_eq(regWriterGroupIds.arrayLength, 1);
        ck_assert_uint_eq(((UA_UInt16 *)regWriterGroupIds.data)[0], 0);
        ck_assert_uint_eq(regDataSetWriterIds.arrayLength, 1);
        ck_assert_uint_eq(((UA_UInt16 *)regDataSetWriterIds.data)[0], 0);

        UA_Variant_clear(&defaultPublisherId);
        UA_Variant_clear(&regWriterGroupIds);
        UA_Variant_clear(&regDataSetWriterIds);

        /* Reconnect client - previous reservations shall be cancelled. */
        UA_Client_disconnect(client);
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        /* Call ReserveIds again - check if the new reservation takes into account
         * the IDs alrady being used by PubSub configuration. */
        retVal = CallReserveIds(client, &transportProfileUri, numRegWriterGroupIds, numRegDataSetWriterIds,
            &defaultPublisherId, &regWriterGroupIds, &regDataSetWriterIds);
        ck_assert(UA_StatusCode_isGood(retVal));

        /* Check reserved WriterGroupIds */
        ck_assert_str_eq("UInt16", regWriterGroupIds.type->typeName);
        ck_assert_uint_eq(numRegWriterGroupIds, regWriterGroupIds.arrayLength);
        for(size_t i = 0; i < regWriterGroupIds.arrayLength; i++) {
            ck_assert_uint_eq(firstId + i + 1, ((UA_UInt16 *)regWriterGroupIds.data)[i]); // Here check if the reservations started from 0x8000 + 1
        }

        /* Check reserved DataSetWriterIds */
        ck_assert_str_eq("UInt16", regDataSetWriterIds.type->typeName);
        ck_assert_uint_eq(numRegDataSetWriterIds, regDataSetWriterIds.arrayLength);
        for(size_t i = 0; i < regDataSetWriterIds.arrayLength; i++) {
            ck_assert_uint_eq(firstId + i + 1, ((UA_UInt16 *)regDataSetWriterIds.data)[i]); // Here check if the reservations started from 0x8000 + 1
        }

        UA_Variant_clear(&defaultPublisherId);
        UA_Variant_clear(&regWriterGroupIds);
        UA_Variant_clear(&regDataSetWriterIds);

        UA_Client_disconnect(client);
        UA_Client_delete(client);
} END_TEST

START_TEST(ReserveIdsInvalidTransportUri){
        UA_StatusCode retVal;
        UA_Client *client = UA_Client_newForUnitTest();
        retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
        }
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

        UA_Variant *inputArguments = (UA_Variant *) UA_calloc(3, (sizeof(UA_Variant)));
        UA_String transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/mqtt");
        UA_Variant_setScalar(&inputArguments[0], &transportProfileUri, &UA_TYPES[UA_TYPES_STRING]);
        UA_UInt16 numRegWriterGroupIds = 6;
        UA_Variant_setScalar(&inputArguments[1], &numRegWriterGroupIds, &UA_TYPES[UA_TYPES_UINT16]);
        UA_UInt16 numRegDataSetWriterIds = 5;
        UA_Variant_setScalar(&inputArguments[2], &numRegDataSetWriterIds, &UA_TYPES[UA_TYPES_UINT16]);

        UA_CallMethodRequest callMethodRequest;
        UA_CallMethodRequest_init(&callMethodRequest);
        callMethodRequest.inputArgumentsSize = 3;
        callMethodRequest.inputArguments = inputArguments;
        callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBSUBCONFIGURATION);
        callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_PUBSUBCONFIGURATION_RESERVEIDS);

        UA_CallMethodResult result;
        UA_CallMethodResult_init(&result);
        result = UA_Server_call(server, &callMethodRequest);
        ck_assert_int_eq(result.statusCode, UA_STATUSCODE_BADINVALIDARGUMENT);

        UA_free(inputArguments);

        UA_CallMethodResult_clear(&result);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    } END_TEST

int main(void) {
    TCase *tc_add_pubsub_informationmodel_methods_connection = tcase_create("PubSub connection delete and creation using the information model methods");
    tcase_add_checked_fixture(tc_add_pubsub_informationmodel_methods_connection, setup, teardown);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddNewPubSubConnectionUsingTheInformationModelMethod);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemovePublishedDataSetFoldersUsingServer);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemovePublishedDataSetFoldersUsingClient);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemovePublishedDataSetItemsUsingServer);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemovePublishedDataSetItemsUsingClient);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemoveWriterGroupsUsingServer);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddAndRemoveWriterGroupsUsingClient);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddandRemoveNewPubSubConnectionWithWriterGroup);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddNewPubSubConnectionWithWriterGroupAndDataSetWriter);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddNewPubSubConnectionWithReaderGroupandDataSetReader);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddNewPubSubConnectionWithReaderGroup);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddandRemoveReaderGroup);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, ReserveIdsMultipleTimes);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, ReserveIdsInvalidTransportUri);

    Suite *s = suite_create("PubSub CRUD configuration by the information model functions");
    suite_add_tcase(s, tc_add_pubsub_informationmodel_methods_connection);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
