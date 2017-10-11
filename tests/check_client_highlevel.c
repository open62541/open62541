/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ua_types.h>

#include "ua_server.h"
#include "ua_client.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "check.h"

UA_Server *server;
UA_Boolean *running;
UA_ServerNetworkLayer nl;
pthread_t server_thread;

UA_Client *client;

#define CUSTOM_NS "http://open62541.org/ns/test"
#define CUSTOM_NS_UPPER "http://open62541.org/ns/Test"


static void *serverloop(void *_) {
    while (*running)
        UA_Server_run_iterate(server, true);
    return NULL;
}

static void setup(void) {
    running = UA_Boolean_new();
    *running = true;
    UA_ServerConfig config = UA_ServerConfig_standard;
    nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    server = UA_Server_new(config);

    ck_assert_uint_eq(2, UA_Server_addNamespace(server, CUSTOM_NS));

    UA_Server_run_startup(server);
    pthread_create(&server_thread, NULL, serverloop, NULL);

    client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:16664");

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    *running = false;
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    pthread_join(server_thread, NULL);
    UA_Server_run_shutdown(server);
    UA_Boolean_delete(running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
}

START_TEST(Misc_State) {
    UA_ClientState state = UA_Client_getState(client);
    ck_assert_uint_eq(state, UA_CLIENTSTATE_CONNECTED);
} END_TEST

START_TEST(Misc_NamespaceGetIndex) {
    UA_String ns = UA_STRING(CUSTOM_NS);
    UA_UInt16 idx;
    UA_StatusCode retval = UA_Client_NamespaceGetIndex(client, &ns, &idx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(idx, 2);

    /* namespace uri is case sensitive */
    ns = UA_STRING(CUSTOM_NS_UPPER);
    retval = UA_Client_NamespaceGetIndex(client, &ns, &idx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

UA_NodeId newReferenceTypeId;
UA_NodeId newObjectTypeId;
UA_NodeId newDataTypeId;
UA_NodeId newVariableTypeId;
UA_NodeId newObjectId;
UA_NodeId newVariableId;
UA_NodeId newMethodId;
UA_NodeId newViewId;

START_TEST(Node_Add) {
    UA_StatusCode retval;

    // Create custom reference type 'HasSubSubType' as child of HasSubtype
    {
        UA_ReferenceTypeAttributes attr;
        UA_ReferenceTypeAttributes_init(&attr);
        attr.description = UA_LOCALIZEDTEXT("en_US", "Some HasSubSubType");
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "HasSubSubType");
        retval = UA_Client_addReferenceTypeNode(client, UA_NODEID_NULL,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                UA_QUALIFIEDNAME(1, "HasSubSubType"), attr,
                                                &newReferenceTypeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // Create TestObjectType SubType within BaseObjectType
    {
        UA_ObjectTypeAttributes attr;
        UA_ObjectTypeAttributes_init(&attr);
        attr.description = UA_LOCALIZEDTEXT("en_US", "Some TestObjectType");
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "TestObjectType");

        retval = UA_Client_addObjectTypeNode(client, UA_NODEID_NULL,
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                             UA_QUALIFIEDNAME(1, "TestObjectType"), attr, &newObjectTypeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }


    // Create Int128 DataType within Integer Datatype
    {
        UA_DataTypeAttributes attr;
        UA_DataTypeAttributes_init(&attr);
        attr.description = UA_LOCALIZEDTEXT("en_US", "Some Int128");
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "Int128");
        retval = UA_Client_addDataTypeNode(client, UA_NODEID_NULL,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_INTEGER),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                           UA_QUALIFIEDNAME(1, "Int128"), attr, &newDataTypeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }



    // Create PointType VariableType within BaseDataVariableType
    {
        UA_VariableTypeAttributes attr;
        UA_VariableTypeAttributes_init(&attr);
        attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
        attr.valueRank = 1; /* array with one dimension */
        UA_UInt32 arrayDims[1] = {2};
        attr.arrayDimensions = arrayDims;
        attr.arrayDimensionsSize = 1;
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "PointType");

        /* a matching default value is required */
        UA_Double zero[2] = {0.0, 0.0};
        UA_Variant_setArray(&attr.value, zero, 2, &UA_TYPES[UA_TYPES_INT32]);
        retval = UA_Client_addVariableTypeNode(client, UA_NODEID_NULL,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_BASEVARIABLETYPE),
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                               UA_QUALIFIEDNAME(1, "PointType"), attr, &newVariableTypeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // create Coordinates Object within ObjectsFolder
    {
        UA_ObjectAttributes attr;
        UA_ObjectAttributes_init(&attr);
        attr.description = UA_LOCALIZEDTEXT("en_US", "Some Coordinates");
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "Coordinates");

        retval = UA_Client_addObjectNode(client, UA_NODEID_NULL,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         UA_QUALIFIEDNAME(1, "Coordinates"),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), attr, &newObjectId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }


    // create Variable 'Top' within Coordinates Object
    {
        UA_VariableAttributes attr;
        UA_VariableAttributes_init(&attr);
        attr.description = UA_LOCALIZEDTEXT("en_US", "Top Coordinate");
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "Top");

        UA_Int32 values[2] = {10, 20};

        UA_Variant_setArray(&attr.value, values, 2, &UA_TYPES[UA_TYPES_INT32]);
        attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
        attr.valueRank = 1; /* array with one dimension */
        UA_UInt32 arrayDims[1] = {2};
        attr.arrayDimensions = arrayDims;
        attr.arrayDimensionsSize = 1;

        retval = UA_Client_addVariableNode(client, UA_NODEID_NULL,
                                           newObjectId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                           UA_QUALIFIEDNAME(1, "Top"),
                                           newVariableTypeId, attr, &newVariableId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // create Method 'Dummy' within Coordinates Object. Fails with BADNODECLASSINVALID
    {
        // creating a method from a client does not yet make much sense since the corresponding
        // action code can not be set from the client side
        UA_MethodAttributes attr;
        UA_MethodAttributes_init(&attr);
        attr.description = UA_LOCALIZEDTEXT("en_US", "Dummy method");
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "Dummy");
        attr.executable = true;
        attr.userExecutable = true;
        retval = UA_Client_addMethodNode(client, UA_NODEID_NULL,
                                         newObjectId,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                                         UA_QUALIFIEDNAME(1, "Dummy"),
                                         attr, &newMethodId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
    }

    // create View 'AllTopCoordinates' whithin Views Folder
    {
        UA_ViewAttributes attr;
        UA_ViewAttributes_init(&attr);
        attr.description = UA_LOCALIZEDTEXT("en_US", "List of all top coordinates");
        attr.displayName = UA_LOCALIZEDTEXT("en_US", "AllTopCoordinates");

        retval = UA_Client_addViewNode(client, UA_NODEID_NULL,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(1, "AllTopCoordinates"),
                                       attr, &newViewId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }


    // Add 'Top' to view
    retval = UA_Client_addReference(client, newViewId, UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_TRUE, UA_STRING_NULL,
                                    UA_EXPANDEDNODEID_NUMERIC(1, newObjectId.identifier.numeric),
                                    UA_NODECLASS_VARIABLE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // Delete 'Top' from view
    retval = UA_Client_deleteReference(client, newViewId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_TRUE,
                                       UA_EXPANDEDNODEID_NUMERIC(1, newObjectId.identifier.numeric), UA_TRUE);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);


    // Delete 'AllTopCoordinates' view
    retval = UA_Client_deleteNode(client, newViewId, UA_TRUE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

}
END_TEST

unsigned int iteratedNodeCount = 0;
UA_NodeId iteratedNodes[2];

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    if (isInverse || (referenceTypeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
                      referenceTypeId.identifier.numeric == UA_NS0ID_HASTYPEDEFINITION))
        return UA_STATUSCODE_GOOD;

    if (iteratedNodeCount >= 2)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    UA_NodeId_copy(&childId, &iteratedNodes[iteratedNodeCount]);

    iteratedNodeCount++;

    return UA_STATUSCODE_GOOD;
}

START_TEST(Node_ReadWrite)
    {
        UA_StatusCode retval;
        // Create a folder with two variables for testing

        UA_NodeId unitTestNodeId;

        UA_NodeId nodeArrayId;
        UA_NodeId nodeIntId;

        // create Coordinates Object within ObjectsFolder
        {
            UA_ObjectAttributes attr;
            UA_ObjectAttributes_init(&attr);
            attr.description = UA_LOCALIZEDTEXT("en_US", "UnitTest");
            attr.displayName = UA_LOCALIZEDTEXT("en_US", "UnitTest");

            retval = UA_Client_addObjectNode(client, UA_NODEID_NULL,
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                             UA_QUALIFIEDNAME(1, "UnitTest"),
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), attr, &unitTestNodeId);
            ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        }

        // create Variable 'Top' within UnitTest Object
        {
            UA_VariableAttributes attr;
            UA_VariableAttributes_init(&attr);
            attr.description = UA_LOCALIZEDTEXT("en_US", "Array");
            attr.displayName = UA_LOCALIZEDTEXT("en_US", "Array");

            /*UA_Int32 values[2];
            values[1] = 10;
            values[2] = 20;

            UA_Variant_setArray(&attr.value, values, 2, &UA_TYPES[UA_TYPES_INT32]);*/
            attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
            attr.valueRank = 1; /* array with one dimension */
            UA_UInt32 arrayDims[1] = {2};
            attr.arrayDimensions = arrayDims;
            attr.arrayDimensionsSize = 1;

            retval = UA_Client_addVariableNode(client, UA_NODEID_NULL,
                                               unitTestNodeId,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                               UA_QUALIFIEDNAME(1, "Array"),
                                               UA_NODEID_NULL, attr, &nodeArrayId);
            ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        }

        // create Variable 'Bottom' within UnitTest Object
        {
            UA_VariableAttributes attr;
            UA_VariableAttributes_init(&attr);
            attr.description = UA_LOCALIZEDTEXT("en_US", "Int");
            attr.displayName = UA_LOCALIZEDTEXT("en_US", "Int");

            UA_Int32 int_value = 5678;

            UA_Variant_setScalar(&attr.value, &int_value, &UA_TYPES[UA_TYPES_INT32]);
            attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

            retval = UA_Client_addVariableNode(client, UA_NODEID_NULL,
                                               unitTestNodeId,
                                               UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                               UA_QUALIFIEDNAME(1, "Int"),
                                               UA_NODEID_NULL, attr, &nodeIntId);

            ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        }

        // iterate over children
        {
            retval = UA_Client_forEachChildNodeCall(client, unitTestNodeId,
                                                    nodeIter, NULL);
            ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

            ck_assert(UA_NodeId_equal(&nodeArrayId, &iteratedNodes[0]));
            ck_assert(UA_NodeId_equal(&nodeIntId, &iteratedNodes[1]));
        }


        /* Read attribute */
        UA_Int32 value = 0;
        UA_Variant *val = UA_Variant_new();
        retval = UA_Client_readValueAttribute(client, nodeIntId, val);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        ck_assert(UA_Variant_isScalar(val) && val->type == &UA_TYPES[UA_TYPES_INT32]);
        value = *(UA_Int32*)val->data;
        ck_assert_int_eq(value, 5678);
        UA_Variant_delete(val);

        /* Write attribute */
        value++;
        val = UA_Variant_new();
        UA_Variant_setScalarCopy(val, &value, &UA_TYPES[UA_TYPES_INT32]);
        retval = UA_Client_writeValueAttribute(client, nodeIntId, val);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        UA_Variant_delete(val);

        /* Read again to check value */
        val = UA_Variant_new();
        retval = UA_Client_readValueAttribute(client, nodeIntId, val);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        ck_assert(UA_Variant_isScalar(val) && val->type == &UA_TYPES[UA_TYPES_INT32]);
        value = *(UA_Int32*)val->data;
        ck_assert_int_eq(value, 5679);
        UA_Variant_delete(val);

        /* Read an empty Variable */
        UA_Variant v;
        UA_Variant_init(&v);
        retval = UA_Client_writeValueAttribute(client, nodeIntId, &v);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        retval = UA_Client_readValueAttribute(client, nodeIntId, &v);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        /* Write Array Dimensions */
        UA_UInt32 arrayDimsNew[] = {3};
        retval = UA_Client_writeArrayDimensionsAttribute(client, nodeArrayId, arrayDimsNew , 1);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

        UA_UInt32 *arrayDimsRead;
        size_t arrayDimsReadSize;
        retval = UA_Client_readArrayDimensionsAttribute(client, nodeArrayId,
                                                        &arrayDimsRead , &arrayDimsReadSize);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert_int_eq(arrayDimsReadSize, 1);
        ck_assert_int_eq(arrayDimsRead[0], 3);
        UA_Array_delete(arrayDimsRead, arrayDimsReadSize, &UA_TYPES[UA_TYPES_UINT32]);

    }
END_TEST

START_TEST(Node_Browse)
    {

        // Browse node in server folder
        {
            UA_BrowseRequest bReq;
            UA_BrowseRequest_init(&bReq);
            // normally is set to 0, to get all the nodes, but we want to test browse next
            bReq.requestedMaxReferencesPerNode = 1;
            bReq.nodesToBrowse = UA_BrowseDescription_new();
            bReq.nodesToBrowseSize = 1;
            bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
            bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;


            UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);


            ck_assert_uint_eq(bResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(bResp.resultsSize, 1);
            ck_assert_uint_eq(bResp.results[0].statusCode, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(bResp.results[0].referencesSize, 1);

            UA_ReferenceDescription *ref = &(bResp.results[0].references[0]);

            ck_assert_uint_eq(ref->nodeId.nodeId.identifier.numeric, UA_NS0ID_SERVERTYPE);


            // browse next
            UA_BrowseNextRequest bNextReq;
            UA_BrowseNextRequest_init(&bNextReq);
            // normally is set to 0, to get all the nodes, but we want to test browse next
            bNextReq.releaseContinuationPoints = UA_FALSE;
            bNextReq.continuationPoints = &bResp.results[0].continuationPoint;
            bNextReq.continuationPointsSize = 1;

            UA_BrowseNextResponse bNextResp = UA_Client_Service_browseNext(client, bNextReq);

            ck_assert_uint_eq(bNextResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(bNextResp.resultsSize, 1);
            ck_assert_uint_eq(bNextResp.results[0].statusCode, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(bNextResp.results[0].referencesSize, 1);

            ref = &(bNextResp.results[0].references[0]);
            ck_assert_uint_eq(ref->nodeId.nodeId.identifier.numeric, UA_NS0ID_SERVER_NAMESPACEARRAY);

            UA_BrowseNextResponse_deleteMembers(&bNextResp);

            bNextResp = UA_Client_Service_browseNext(client, bNextReq);

            ck_assert_uint_eq(bNextResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(bNextResp.resultsSize, 1);
            ck_assert_uint_eq(bNextResp.results[0].statusCode, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(bNextResp.results[0].referencesSize, 1);

            ref = &(bNextResp.results[0].references[0]);
            ck_assert_uint_eq(ref->nodeId.nodeId.identifier.numeric, UA_NS0ID_SERVER_SERVERARRAY);

            UA_BrowseNextResponse_deleteMembers(&bNextResp);

            // release continuation point. Result is then empty
            bNextReq.releaseContinuationPoints = UA_TRUE;
            bNextResp = UA_Client_Service_browseNext(client, bNextReq);
            UA_BrowseNextResponse_deleteMembers(&bNextResp);
            ck_assert_uint_eq(bNextResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(bNextResp.resultsSize, 0);

            UA_BrowseRequest_deleteMembers(&bReq);
            UA_BrowseResponse_deleteMembers(&bResp);
            // already deleted by browse request
            bNextReq.continuationPoints = NULL;
            bNextReq.continuationPointsSize = 0;
            UA_BrowseNextRequest_deleteMembers(&bNextReq);
        }
    }
END_TEST

START_TEST(Node_Register)
    {
        {
            UA_RegisterNodesRequest req;
            UA_RegisterNodesRequest_init(&req);

            req.nodesToRegister = UA_NodeId_new();
            req.nodesToRegister[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
            req.nodesToRegisterSize = 1;

            UA_RegisterNodesResponse res = UA_Client_Service_registerNodes(client, req);

            ck_assert_uint_eq(res.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
            ck_assert_uint_eq(res.registeredNodeIdsSize, 1);

            UA_UnregisterNodesRequest reqUn;
            UA_UnregisterNodesRequest_init(&reqUn);

            reqUn.nodesToUnregister = UA_NodeId_new();
            reqUn.nodesToUnregister[0] = res.registeredNodeIds[0];
            reqUn.nodesToUnregisterSize = 1;

            UA_UnregisterNodesResponse resUn = UA_Client_Service_unregisterNodes(client, reqUn);

            ck_assert_uint_eq(resUn.responseHeader.serviceResult, UA_STATUSCODE_GOOD);


            UA_UnregisterNodesRequest_deleteMembers(&reqUn);
            UA_UnregisterNodesResponse_deleteMembers(&resUn);
            UA_RegisterNodesRequest_deleteMembers(&req);
            UA_RegisterNodesResponse_deleteMembers(&res);
        }
    }
END_TEST


static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Client Highlevel");
    TCase *tc_misc = tcase_create("Client Highlevel Misc");
    tcase_add_checked_fixture(tc_misc, setup, teardown);
    tcase_add_test(tc_misc, Misc_State);
    tcase_add_test(tc_misc, Misc_NamespaceGetIndex);
    suite_add_tcase(s, tc_misc);

    TCase *tc_nodes = tcase_create("Client Highlevel Node Management");
    tcase_add_checked_fixture(tc_nodes, setup, teardown);
    tcase_add_test(tc_nodes, Node_Add);
    tcase_add_test(tc_nodes, Node_Browse);
    tcase_add_test(tc_nodes, Node_ReadWrite);
    tcase_add_test(tc_nodes, Node_Register);
    suite_add_tcase(s, tc_nodes);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
