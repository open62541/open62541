/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <string.h>
#include <math.h>
#include <src_generated/ua_types_generated.h>
#include <ua_types.h>
#include <src_generated/ua_types_generated_encoding_binary.h>
#include "ua_types.h"
#include "ua_server_pubsub.h"
#include "src_generated/ua_types_generated.h"
#include "ua_network_pubsub_udp.h"
#include "ua_server_internal.h"
#include "check.h"
#include "ua_plugin_pubsub.h"
#include "ua_config_default.h"
#include "thread_wrapper.h"


UA_NodeId connection1, connection2, writerGroup1, writerGroup2, writerGroup3,
        publishedDataSet1, publishedDataSet2, dataSetWriter1, dataSetWriter2, dataSetWriter3;

UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
UA_Boolean running;
THREAD_HANDLE server_thread;

THREAD_CALLBACK(serverloop) {
        while (running)
        UA_Server_run_iterate(server, true);
        return 0;
}

static void setup(void) {
    running = true;
    config = UA_ServerConfig_new_default();
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);
}

static void teardown(void) {
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
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

/*
static UA_StatusCode startServer(void){
    config = UA_ServerConfig_new_default();
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    server = UA_Server_new(config);
    return UA_Server_run_startup(server);

}*/

START_TEST(AddNewPubSubConnectionUsingTheInformationModelMethod){
    UA_StatusCode retVal;
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);
    retVal = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
    }
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

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
    eo.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    UA_NetworkAddressUrlDataType networkAddressDataType = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_ByteString_allocBuffer(&eo.content.encoded.body, UA_NetworkAddressUrlDataType_calcSizeBinary(&networkAddressDataType));
    UA_Byte *bufPos = eo.content.encoded.body.data;
    UA_NetworkAddressUrlDataType_encodeBinary(&networkAddressDataType, &bufPos, &(eo.content.encoded.body.data[eo.content.encoded.body.length]));
    eo.content.encoded.typeId = UA_NODEID_NUMERIC(0, UA_TYPES_NETWORKADDRESSURLDATATYPE);
    pubSubConnection.address = eo;

    UA_Variant inputArguments;
    UA_Variant_init(&inputArguments);
    UA_Variant_setScalar(&inputArguments, &pubSubConnection, &UA_TYPES[UA_TYPES_PUBSUBCONNECTIONDATATYPE]);

    UA_CallMethodRequest callMethodRequest;
    UA_CallMethodRequest_init(&callMethodRequest);
    callMethodRequest.inputArgumentsSize = 1;
    callMethodRequest.inputArguments = &inputArguments;
    callMethodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE);
    callMethodRequest.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_PUBLISHSUBSCRIBE_ADDCONNECTION);

    UA_CallMethodResult result;
    UA_CallMethodResult_init(&result);
    result = UA_Server_call(server, &callMethodRequest);
    ck_assert_int_eq(1, result.outputArgumentsSize);
    ck_assert_int_eq(result.statusCode, UA_STATUSCODE_GOOD);
    UA_NodeId createdConnection;
    if(result.outputArguments->type == &UA_TYPES[UA_TYPES_NODEID])
        createdConnection = *((UA_NodeId *) result.outputArguments->data);
    UA_LocalizedText connectionDisplayName;
    UA_LocalizedText_init(&connectionDisplayName);
    retVal = UA_Server_readDisplayName(server, createdConnection, &connectionDisplayName);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_str_eq((const char *) connectionDisplayName.text.data, "Model Connection 1");
    //todo browse and check childs

    UA_Variant serverPubSubConnectionValues;
    UA_Variant_init(&serverPubSubConnectionValues);
    /*UA_NodeId connectionAddress = findSingleChildNode(UA_QUALIFIEDNAME(0, "Address"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                      createdConnection);*/
    UA_NodeId connectionPublisherId = findSingleChildNode(UA_QUALIFIEDNAME(0, "PublisherId"),
                                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                                      createdConnection);
    ck_assert_int_eq(UA_Server_readValue(server, connectionPublisherId, &serverPubSubConnectionValues),
                     UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(*((UA_UInt32 *) serverPubSubConnectionValues.data), publisherIdValue);
    //TODO create nd add a connection with string pulisherId



    } END_TEST

int main(void) {
    //startServer(); //TODO CHECK IF SERVER IS RUNNING
    TCase *tc_add_pubsub_informationmodel_methods_connection = tcase_create("PubSub connection delete and creation using the information model methods");
    tcase_add_checked_fixture(tc_add_pubsub_informationmodel_methods_connection, setup, teardown);
    tcase_add_test(tc_add_pubsub_informationmodel_methods_connection, AddNewPubSubConnectionUsingTheInformationModelMethod);

    Suite *s = suite_create("PubSub CRUD configuration by the information model functions");
    suite_add_tcase(s, tc_add_pubsub_informationmodel_methods_connection);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
