/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "thread_wrapper.h"

UA_Client *client;
UA_Server *server;
UA_Boolean running;
THREAD_HANDLE server_thread;

#define CUSTOM_NS "http://open62541.org/ns/test"
#define CUSTOM_NS_UPPER "http://open62541.org/ns/Test"

THREAD_CALLBACK(serverloop) {
    while (running)
        UA_Server_run_iterate(server, true);
    return 0;
}

static void setup(void) {
    running = true;
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);

    ck_assert_uint_eq(2, UA_Server_addNamespace(server, CUSTOM_NS));

    UA_Server_run_startup(server);
    THREAD_CREATE(server_thread, serverloop);

    client = UA_Client_newForUnitTest();
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    running = false;
    THREAD_JOIN(server_thread);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Misc_State) {
    UA_SessionState ss;
    UA_Client_getState(client, NULL, &ss, NULL);
    ck_assert_uint_eq(ss, UA_SESSIONSTATE_ACTIVATED);
}
END_TEST

START_TEST(Misc_NamespaceGetIndex) {
    UA_UInt16 idx;
    UA_String ns = UA_STRING(CUSTOM_NS);
    UA_StatusCode retval = UA_Client_NamespaceGetIndex(client, &ns, &idx);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(idx, 2);

    // namespace uri is case sensitive
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

#ifdef UA_ENABLE_NODEMANAGEMENT

START_TEST(Node_Add) {
    UA_StatusCode retval;

    // Create custom reference type 'HasSubSubType' as child of HasSubtype
    {
        UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Some HasSubSubType");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "HasSubSubType");
        retval = UA_Client_addReferenceTypeNode(client, UA_NODEID_NULL,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                UA_QUALIFIEDNAME(1, "HasSubSubType"), attr,
                                                &newReferenceTypeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // Create TestObjectType SubType within BaseObjectType
    {
        UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Some TestObjectType");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestObjectType");
        retval = UA_Client_addObjectTypeNode(client, UA_NODEID_NULL,
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                             UA_QUALIFIEDNAME(1, "TestObjectType"), attr, &newObjectTypeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    /* Minimal nodeset does not contain UA_NS0ID_INTEGER node */
    #ifdef UA_GENERATED_NAMESPACE_ZERO
    // Create Int128 DataType within Integer Datatype
    {
        UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Some Int128");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Int128");
        retval = UA_Client_addDataTypeNode(client, UA_NODEID_NULL,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_INTEGER),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                           UA_QUALIFIEDNAME(1, "Int128"), attr, &newDataTypeId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
    #endif

    // Create PointType VariableType within BaseDataVariableType
    {
        UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
        attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
        attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
        UA_UInt32 arrayDims[1] = {2};
        attr.arrayDimensions = arrayDims;
        attr.arrayDimensionsSize = 1;
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "PointType");

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
        UA_ObjectAttributes attr = UA_ObjectAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Some Coordinates");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Coordinates");
        retval = UA_Client_addObjectNode(client, UA_NODEID_NULL,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         UA_QUALIFIEDNAME(1, "Coordinates"),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), attr, &newObjectId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // create Variable 'Top' within Coordinates Object
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Top Coordinate");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Top");
        UA_Int32 values[2] = {10, 20};
        UA_Variant_setArray(&attr.value, values, 2, &UA_TYPES[UA_TYPES_INT32]);
        attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
        attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
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

    // create Method 'Dummy' within Coordinates Object.
    {
        UA_MethodAttributes attr = UA_MethodAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Dummy method");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Dummy");
        attr.executable = true;
        attr.userExecutable = true;
        retval = UA_Client_addMethodNode(client, UA_NODEID_NULL,
                                         newObjectId,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                         UA_QUALIFIEDNAME(1, "Dummy"),
                                         attr, &newMethodId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // create View 'AllTopCoordinates' whithin Views Folder
    {
        UA_ViewAttributes attr = UA_ViewAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "List of all top coordinates");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "AllTopCoordinates");
        retval = UA_Client_addViewNode(client, UA_NODEID_NULL,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(1, "AllTopCoordinates"),
                                       attr, &newViewId);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
    target.nodeId = newObjectId;

    // Add 'Top' to view
    retval = UA_Client_addReference(client, newViewId, UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                    UA_TRUE, UA_STRING_NULL, target, UA_NODECLASS_VARIABLE);

    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // Delete 'Top' from view
    retval = UA_Client_deleteReference(client, newViewId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       true, target, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // Delete 'AllTopCoordinates' view
    retval = UA_Client_deleteNode(client, newViewId, UA_TRUE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

unsigned int iteratedNodeCount = 0;
UA_NodeId iteratedNodes[4];

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    if (isInverse || (referenceTypeId.identifierType == UA_NODEIDTYPE_NUMERIC &&
                      referenceTypeId.identifier.numeric == UA_NS0ID_HASTYPEDEFINITION))
        return UA_STATUSCODE_GOOD;

    if (iteratedNodeCount >= 4)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    UA_NodeId_copy(&childId, &iteratedNodes[iteratedNodeCount]);
    iteratedNodeCount++;
    return UA_STATUSCODE_GOOD;
}

#endif

START_TEST(Node_Browse) {
    // Browse node in server folder
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

    /* References might have a different order in generated nodesets */
    /* UA_ReferenceDescription *ref = &(bResp.results[0].references[0]); */
    /* ck_assert_uint_eq(ref->nodeId.nodeId.identifier.numeric, UA_NS0ID_SERVERTYPE); */

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

    /* ref = &(bNextResp.results[0].references[0]); */
    /* ck_assert_uint_eq(ref->nodeId.nodeId.identifier.numeric, UA_NS0ID_SERVER_NAMESPACEARRAY); */

    UA_BrowseNextResponse_clear(&bNextResp);

    bNextResp = UA_Client_Service_browseNext(client, bNextReq);
    ck_assert_uint_eq(bNextResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bNextResp.resultsSize, 1);
    ck_assert_uint_eq(bNextResp.results[0].statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bNextResp.results[0].referencesSize, 1);

    /* ref = &(bNextResp.results[0].references[0]); */
    /* ck_assert_uint_eq(ref->nodeId.nodeId.identifier.numeric, UA_NS0ID_SERVER_SERVERARRAY); */

    UA_BrowseNextResponse_clear(&bNextResp);

    // release continuation point. Result is then empty
    bNextReq.releaseContinuationPoints = UA_TRUE;
    bNextResp = UA_Client_Service_browseNext(client, bNextReq);
    UA_BrowseNextResponse_clear(&bNextResp);
    ck_assert_uint_eq(bNextResp.responseHeader.serviceResult, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(bNextResp.resultsSize, 0);

    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
    // already deleted by browse request
    bNextReq.continuationPoints = NULL;
    bNextReq.continuationPointsSize = 0;
    UA_BrowseNextRequest_clear(&bNextReq);
}
END_TEST

START_TEST(Node_Register) {
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

    UA_UnregisterNodesRequest_clear(&reqUn);
    UA_UnregisterNodesResponse_clear(&resUn);
    UA_RegisterNodesRequest_clear(&req);
    UA_RegisterNodesResponse_clear(&res);
}
END_TEST



// NodeIds for ReadWrite testing
UA_NodeId nodeReadWriteUnitTest;
UA_NodeId nodeReadWriteArray;
UA_NodeId nodeReadWriteInt;
UA_NodeId nodeReadWriteGeneric;
UA_NodeId nodeReadWriteTestObjectType;
UA_NodeId nodeReadWriteTestHasSubSubType;
UA_NodeId nodeReadWriteView;
UA_NodeId nodeReadWriteMethod;

#ifdef UA_ENABLE_NODEMANAGEMENT

START_TEST(Node_AddReadWriteNodes) {
    UA_StatusCode retval;
    // Create a folder with two variables for testing

    // create Coordinates Object within ObjectsFolder
    {
        UA_ObjectAttributes attr = UA_ObjectAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "UnitTest");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "UnitTest");
        retval = UA_Client_addObjectNode(client, UA_NODEID_NULL,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                         UA_QUALIFIEDNAME(1, "UnitTest"),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                                         attr, &nodeReadWriteUnitTest);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // create Variable 'Top' within UnitTest Object
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Array");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Array");
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.writeMask = UA_WRITEMASK_ARRRAYDIMENSIONS;

        UA_Int32 values[2];
        values[0] = 10;
        values[1] = 20;

        UA_Variant_setArray(&attr.value, values, 2, &UA_TYPES[UA_TYPES_INT32]);
        attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
        attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
        UA_UInt32 arrayDims[1] = {2};
        attr.arrayDimensions = arrayDims;
        attr.arrayDimensionsSize = 1;

        retval = UA_Client_addVariableNode(client, UA_NODEID_NULL,
                                           nodeReadWriteUnitTest,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                           UA_QUALIFIEDNAME(1, "Array"),
                                           UA_NODEID_NULL, attr, &nodeReadWriteArray);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // create Variable 'Int' within UnitTest Object
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Int");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Int");
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.writeMask = 0xFFFFFFFF;

        UA_Int32 int_value = 5678;
        UA_Variant_setScalar(&attr.value, &int_value, &UA_TYPES[UA_TYPES_INT32]);
        attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

        retval = UA_Client_addVariableNode(client, UA_NODEID_NULL,
                                           nodeReadWriteUnitTest,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                           UA_QUALIFIEDNAME(1, "Int"),
                                           UA_NODEID_NULL, attr, &nodeReadWriteInt);

        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // create Variable 'Generic' within UnitTest Object
    {
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Generic");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Generic");
        attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        attr.writeMask = 0xFFFFFFFF;

        // generic datatype
        attr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE);

        retval = UA_Client_addVariableNode(client, UA_NODEID_NULL,
                                           nodeReadWriteUnitTest,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                           UA_QUALIFIEDNAME(1, "Generic"),
                                           UA_NODEID_NULL, attr, &nodeReadWriteGeneric);

        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // create Method 'Dummy' within Coordinates Object.
    {
        UA_MethodAttributes attr = UA_MethodAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Dummy method");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "Dummy");
        attr.executable = true;
        attr.userExecutable = true;
        attr.writeMask = 0xFFFFFFFF;
        retval = UA_Client_addMethodNode(client, UA_NODEID_NULL,
                                         nodeReadWriteUnitTest,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                         UA_QUALIFIEDNAME(1, "Dummy"),
                                         attr, &nodeReadWriteMethod);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // Create TestObjectType SubType within BaseObjectType
    {
        UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Some TestObjectType");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "TestObjectType");
        attr.writeMask = 0xFFFFFFFF;
        retval = UA_Client_addObjectTypeNode(client, UA_NODEID_NULL,
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                             UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                             UA_QUALIFIEDNAME(1, "TestObjectType"), attr, &nodeReadWriteTestObjectType);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // Create custom reference type 'HasSubSubType' as child of HasSubtype
    {
        UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "Some HasSubSubType");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "HasSubSubType");
        attr.inverseName = UA_LOCALIZEDTEXT("en-US", "HasParentParentType");
        attr.writeMask = 0xFFFFFFFF;
        retval = UA_Client_addReferenceTypeNode(client, UA_NODEID_NULL,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                UA_QUALIFIEDNAME(1, "HasSubSubType"), attr,
                                                &nodeReadWriteTestHasSubSubType);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }


    // create View 'AllTopCoordinates' whithin Views Folder
    {
        UA_ViewAttributes attr = UA_ViewAttributes_default;
        attr.description = UA_LOCALIZEDTEXT("en-US", "List of all top coordinates");
        attr.displayName = UA_LOCALIZEDTEXT("en-US", "AllTopCoordinates");
        attr.writeMask = 0xFFFFFFFF;
        retval = UA_Client_addViewNode(client, UA_NODEID_NULL,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(1, "AllTopCoordinates"),
                                       attr, &nodeReadWriteView);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }

    // iterate over children
    retval = UA_Client_forEachChildNodeCall(client, nodeReadWriteUnitTest, nodeIter, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Boolean found = false;
    for(unsigned int i = 0; i < iteratedNodeCount; i++) {
        if(UA_NodeId_equal(&nodeReadWriteArray, &iteratedNodes[i]))
            found = true;
    }
    ck_assert(found == true);

    found = false;
    for(unsigned int i = 0; i < iteratedNodeCount; i++) {
        if(UA_NodeId_equal(&nodeReadWriteInt, &iteratedNodes[i]))
            found = true;
    }
    ck_assert(found == true);
}
END_TEST

START_TEST(Node_ReadWrite_Id) {
    UA_NodeId newNodeId;

    // read to check if node id was changed
    UA_StatusCode retval = UA_Client_readNodeIdAttribute(client, nodeReadWriteInt, &newNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_NodeId_equal(&newNodeId, &nodeReadWriteInt));

    newNodeId.identifier.numeric = 900;
    retval = UA_Client_writeNodeIdAttribute(client, nodeReadWriteInt, &newNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);

}
END_TEST

static void checkNodeClass(UA_Client *clientNc, const UA_NodeId nodeId, const UA_NodeClass expectedClass) {
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    UA_StatusCode retval = UA_Client_readNodeClassAttribute(clientNc, nodeId, &nodeClass);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nodeClass, expectedClass);
}

START_TEST(Node_ReadWrite_Class) {
    checkNodeClass(client, nodeReadWriteInt, UA_NODECLASS_VARIABLE);
    checkNodeClass(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), UA_NODECLASS_OBJECT);

    /* Minimal nodeset does not contain UA_NS0ID_SERVER_GETMONITOREDITEMS node */
#if defined(UA_ENABLE_METHODCALLS) && defined(UA_GENERATED_NAMESPACE_ZERO)
    checkNodeClass(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS), UA_NODECLASS_METHOD);
#endif

    UA_NodeClass newClass = UA_NODECLASS_METHOD;
    UA_StatusCode retval = UA_Client_writeNodeClassAttribute(client, nodeReadWriteInt, &newClass);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
}
END_TEST

START_TEST(Node_ReadWrite_BrowseName) {

    UA_QualifiedName browseName;
    UA_StatusCode retval = UA_Client_readBrowseNameAttribute(client, nodeReadWriteInt, &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_QualifiedName orig = UA_QUALIFIEDNAME(1, "Int");
    ck_assert_int_eq(browseName.namespaceIndex, orig.namespaceIndex);
    ck_assert(UA_String_equal(&browseName.name, &orig.name));

    UA_QualifiedName_clear(&browseName);

    browseName = UA_QUALIFIEDNAME(1,"Int-Changed");

    retval = UA_Client_writeBrowseNameAttribute(client, UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);

    retval = UA_Client_writeBrowseNameAttribute(client, nodeReadWriteInt, &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
}
END_TEST

START_TEST(Node_ReadWrite_DisplayName) {

    UA_LocalizedText displayName;
    UA_StatusCode retval = UA_Client_readDisplayNameAttribute(client, nodeReadWriteInt, &displayName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText orig = UA_LOCALIZEDTEXT("en-US", "Int");
    ck_assert(UA_String_equal(&displayName.locale, &orig.locale));
    ck_assert(UA_String_equal(&displayName.text, &orig.text));
    UA_LocalizedText_clear(&displayName);

    UA_LocalizedText newLocale = UA_LOCALIZEDTEXT("en-US", "Int-Changed");

    retval = UA_Client_writeDisplayNameAttribute(client, nodeReadWriteInt, &newLocale);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText displayNameChangedRead;
    retval = UA_Client_readDisplayNameAttribute(client, nodeReadWriteInt, &displayNameChangedRead);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_String_equal(&newLocale.locale, &displayNameChangedRead.locale));
    ck_assert(UA_String_equal(&newLocale.text, &displayNameChangedRead.text));
    UA_LocalizedText_clear(&displayNameChangedRead);

}
END_TEST

START_TEST(Node_ReadWrite_Description) {

    UA_LocalizedText description;
    UA_StatusCode retval = UA_Client_readDescriptionAttribute(client, nodeReadWriteInt, &description);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText orig = UA_LOCALIZEDTEXT("en-US", "Int");
    ck_assert(UA_String_equal(&description.locale, &orig.locale));
    ck_assert(UA_String_equal(&description.text, &orig.text));
    UA_LocalizedText_clear(&description);

    UA_LocalizedText newLocale = UA_LOCALIZEDTEXT("en-US", "Int-Changed");

    retval = UA_Client_writeDescriptionAttribute(client, nodeReadWriteInt, &newLocale);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText descriptionChangedRead;
    retval = UA_Client_readDescriptionAttribute(client, nodeReadWriteInt, &descriptionChangedRead);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_String_equal(&newLocale.locale, &descriptionChangedRead.locale));
    ck_assert(UA_String_equal(&newLocale.text, &descriptionChangedRead.text));
    UA_LocalizedText_clear(&descriptionChangedRead);

}
END_TEST

START_TEST(Node_ReadWrite_WriteMask) {

    UA_UInt32 writeMask;
    UA_StatusCode retval = UA_Client_readWriteMaskAttribute(client, nodeReadWriteInt, &writeMask);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(writeMask, 0xFFFFFFFF);

    // Disable a random write mask bit
    UA_UInt32 newMask = 0xFFFFFFFF & ~UA_WRITEMASK_BROWSENAME;

    retval = UA_Client_writeWriteMaskAttribute(client, nodeReadWriteInt, &newMask);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_UInt32 writeMaskChangedRead;
    retval = UA_Client_readWriteMaskAttribute(client, nodeReadWriteInt, &writeMaskChangedRead);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(writeMaskChangedRead, newMask);

}
END_TEST

START_TEST(Node_ReadWrite_UserWriteMask) {

    UA_UInt32 userWriteMask;
    UA_StatusCode retval = UA_Client_readUserWriteMaskAttribute(client, nodeReadWriteInt, &userWriteMask);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(userWriteMask, 0xFFFFFFFF & ~UA_WRITEMASK_BROWSENAME);

    // Disable a random write mask bit
    UA_UInt32 newMask = 0xFFFFFFFF & ~UA_WRITEMASK_DISPLAYNAME;

    retval = UA_Client_writeUserWriteMaskAttribute(client, nodeReadWriteInt, &newMask);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
}
END_TEST

START_TEST(Node_ReadWrite_IsAbstract) {

    UA_Boolean isAbstract;
    UA_StatusCode retval = UA_Client_readIsAbstractAttribute(client, nodeReadWriteInt, &isAbstract);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
    retval = UA_Client_readIsAbstractAttribute(client, nodeReadWriteTestObjectType, &isAbstract);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(isAbstract, UA_FALSE);

    UA_Boolean newIsAbstract = UA_TRUE;

    retval = UA_Client_writeIsAbstractAttribute(client, nodeReadWriteTestObjectType, &newIsAbstract);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readIsAbstractAttribute(client, nodeReadWriteTestObjectType, &isAbstract);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(isAbstract, newIsAbstract);
}
END_TEST

START_TEST(Node_ReadWrite_Symmetric) {

    UA_Boolean symmetric;
    UA_StatusCode retval = UA_Client_readSymmetricAttribute(client, nodeReadWriteInt, &symmetric);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
    retval = UA_Client_readSymmetricAttribute(client, nodeReadWriteTestHasSubSubType, &symmetric);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(symmetric, UA_FALSE);

    UA_Boolean newSymmetric = UA_TRUE;

    retval = UA_Client_writeSymmetricAttribute(client, nodeReadWriteTestHasSubSubType, &newSymmetric);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readSymmetricAttribute(client, nodeReadWriteTestHasSubSubType, &symmetric);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(symmetric, newSymmetric);

    /* reset */
    newSymmetric = false;
    retval = UA_Client_writeSymmetricAttribute(client, nodeReadWriteTestHasSubSubType, &newSymmetric);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST


START_TEST(Node_ReadWrite_InverseName) {

    UA_LocalizedText inverseName;
    UA_StatusCode retval = UA_Client_readInverseNameAttribute(client, nodeReadWriteTestHasSubSubType, &inverseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText orig = UA_LOCALIZEDTEXT("en-US", "HasParentParentType");
    ck_assert(UA_String_equal(&inverseName.locale, &orig.locale));
    ck_assert(UA_String_equal(&inverseName.text, &orig.text));
    UA_LocalizedText_clear(&inverseName);

    UA_LocalizedText newLocale = UA_LOCALIZEDTEXT("en-US", "HasParentParentType-Changed");

    retval = UA_Client_writeInverseNameAttribute(client, nodeReadWriteTestHasSubSubType, &newLocale);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText inverseNameChangedRead;
    retval = UA_Client_readInverseNameAttribute(client, nodeReadWriteTestHasSubSubType, &inverseNameChangedRead);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_String_equal(&newLocale.locale, &inverseNameChangedRead.locale));
    ck_assert(UA_String_equal(&newLocale.text, &inverseNameChangedRead.text));
    UA_LocalizedText_clear(&inverseNameChangedRead);

}
END_TEST

START_TEST(Node_ReadWrite_ContainsNoLoops) {

    UA_Boolean containsNoLoops;
    UA_StatusCode retval = UA_Client_readContainsNoLoopsAttribute(client, nodeReadWriteTestHasSubSubType, &containsNoLoops);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
    retval = UA_Client_readContainsNoLoopsAttribute(client, nodeReadWriteView, &containsNoLoops);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(containsNoLoops, UA_FALSE);

    UA_Boolean newContainsNoLoops = UA_TRUE;

    retval = UA_Client_writeContainsNoLoopsAttribute(client, nodeReadWriteView, &newContainsNoLoops);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readContainsNoLoopsAttribute(client, nodeReadWriteView, &containsNoLoops);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(containsNoLoops, newContainsNoLoops);
}
END_TEST

START_TEST(Node_ReadWrite_EventNotifier) {

    UA_Byte eventNotifier = 0;
    UA_StatusCode retval = UA_Client_readEventNotifierAttribute(client, nodeReadWriteTestHasSubSubType, &eventNotifier);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
    retval = UA_Client_readEventNotifierAttribute(client, nodeReadWriteView, &eventNotifier);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(eventNotifier, 0);

    UA_Byte newEventNotifier = 1;

    retval = UA_Client_writeEventNotifierAttribute(client, nodeReadWriteView, &newEventNotifier);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readEventNotifierAttribute(client, nodeReadWriteView, &eventNotifier);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(eventNotifier, newEventNotifier);
}
END_TEST


START_TEST(Node_ReadWrite_Value) {

    UA_Int32 value = 0;
    UA_Variant *val = UA_Variant_new();
    UA_StatusCode retval = UA_Client_readValueAttribute(client, nodeReadWriteInt, val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_Variant_isScalar(val) && val->type == &UA_TYPES[UA_TYPES_INT32]);
    value = *(UA_Int32*)val->data;
    ck_assert_int_eq(value, 5678);
    UA_Variant_delete(val);

    /* Write attribute */
    value++;
    val = UA_Variant_new();
    UA_Variant_setScalarCopy(val, &value, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Client_writeValueAttribute(client, nodeReadWriteInt, val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_delete(val);

    /* Read again to check value */
    val = UA_Variant_new();
    retval = UA_Client_readValueAttribute(client, nodeReadWriteInt, val);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_Variant_isScalar(val) && val->type == &UA_TYPES[UA_TYPES_INT32]);
    value = *(UA_Int32*)val->data;
    ck_assert_int_eq(value, 5679);
    UA_Variant_delete(val);
}
END_TEST

START_TEST(Node_ReadWrite_DataType) {
    UA_NodeId dataType;

    // read to check if node id was changed
    UA_StatusCode retval = UA_Client_readDataTypeAttribute(client, nodeReadWriteInt, &dataType);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_NodeId_equal(&dataType, &UA_TYPES[UA_TYPES_INT32].typeId));

    UA_NodeId newDataType = UA_TYPES[UA_TYPES_VARIANT].typeId;
    retval = UA_Client_writeDataTypeAttribute(client, nodeReadWriteGeneric, &newDataType);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readDataTypeAttribute(client, nodeReadWriteGeneric, &dataType);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&dataType, &newDataType));

}
END_TEST

START_TEST(Node_ReadWrite_ValueRank) {

    UA_Int32 valueRank = UA_VALUERANK_ONE_OR_MORE_DIMENSIONS;
    UA_StatusCode retval = UA_Client_readValueRankAttribute(client, nodeReadWriteGeneric, &valueRank);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(valueRank, UA_VALUERANK_ANY);

    // set the value to a scalar
    UA_Double val = 0.0;
    UA_Variant value;
    UA_Variant_setScalar(&value, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = UA_Client_writeValueAttribute(client, nodeReadWriteGeneric, &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // we want an array
    UA_Int32 newValueRank = UA_VALUERANK_ONE_DIMENSION;

    // shall fail when the value is not compatible
    retval = UA_Client_writeValueRankAttribute(client, nodeReadWriteGeneric, &newValueRank);
    ck_assert(retval != UA_STATUSCODE_GOOD);

    // set the value to an array
    UA_Double vec[3] = {0.0, 0.0, 0.0};
    UA_Variant_setArray(&value, vec, 3, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = UA_Client_writeValueAttribute(client, nodeReadWriteGeneric, &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // try again
    retval = UA_Client_writeValueRankAttribute(client, nodeReadWriteGeneric, &newValueRank);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readValueRankAttribute(client, nodeReadWriteGeneric, &valueRank);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(valueRank, newValueRank);


    // set the value to no array
    UA_Variant_init(&value);
    retval = UA_Client_writeValueAttribute(client, nodeReadWriteGeneric, &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST


START_TEST(Node_ReadWrite_ArrayDimensions) {
    UA_UInt32 *arrayDimsRead;
    size_t arrayDimsReadSize;
    UA_StatusCode retval = UA_Client_readArrayDimensionsAttribute(client, nodeReadWriteGeneric,
                                                                  &arrayDimsReadSize, &arrayDimsRead);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrayDimsReadSize, 0);

    // Set a vector of size 1 as the value
    UA_Double vec2[2] = {0.0, 0.0};
    UA_Variant value;
    UA_Variant_setArray(&value, vec2, 2, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = UA_Client_writeValueAttribute(client, nodeReadWriteGeneric, &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // writing the array dimensions shall fail at first
    // because the current value is not conformant
    UA_UInt32 arrayDimsNew[] = {1};
    retval = UA_Client_writeArrayDimensionsAttribute(client, nodeReadWriteGeneric, 1, arrayDimsNew);
    ck_assert(retval != UA_STATUSCODE_GOOD);

    // Set a vector of size 1 as the value
    UA_Double vec1[1] = {0.0};
    UA_Variant_setArray(&value, vec1, 1, &UA_TYPES[UA_TYPES_DOUBLE]);
    retval = UA_Client_writeValueAttribute(client, nodeReadWriteGeneric, &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    // Now we can set matching array-dimensions
    retval = UA_Client_writeArrayDimensionsAttribute(client, nodeReadWriteGeneric, 1, arrayDimsNew);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readArrayDimensionsAttribute(client, nodeReadWriteGeneric,
                                                    &arrayDimsReadSize, &arrayDimsRead);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrayDimsReadSize, 1);
    ck_assert_int_eq(arrayDimsRead[0], 1);
    UA_Array_delete(arrayDimsRead, arrayDimsReadSize, &UA_TYPES[UA_TYPES_UINT32]);
}
END_TEST

START_TEST(Node_ReadWrite_AccessLevel) {

    UA_Byte accessLevel;
    UA_StatusCode retval = UA_Client_readAccessLevelAttribute(client, nodeReadWriteInt, &accessLevel);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(accessLevel, UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);

    UA_Byte newMask = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;

    retval = UA_Client_writeAccessLevelAttribute(client, nodeReadWriteInt, &newMask);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_Byte accessLevelChangedRead;
    retval = UA_Client_readAccessLevelAttribute(client, nodeReadWriteInt, &accessLevelChangedRead);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(accessLevelChangedRead, newMask);

}
END_TEST

START_TEST(Node_ReadWrite_UserAccessLevel) {

    UA_Byte userAccessLevel;
    UA_StatusCode retval = UA_Client_readUserAccessLevelAttribute(client, nodeReadWriteInt, &userAccessLevel);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(userAccessLevel, UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD);

    UA_Byte newMask = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;

    retval = UA_Client_writeUserAccessLevelAttribute(client, nodeReadWriteInt, &newMask);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
}
END_TEST

START_TEST(Node_ReadWrite_MinimumSamplingInterval) {

    UA_Double minimumSamplingInterval = 0;
    UA_StatusCode retval = UA_Client_readMinimumSamplingIntervalAttribute(client, nodeReadWriteGeneric, &minimumSamplingInterval);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(minimumSamplingInterval == 0);

    // we want an array
    UA_Double newMinimumSamplingInterval = 1;

    retval = UA_Client_writeMinimumSamplingIntervalAttribute(client, nodeReadWriteGeneric, &newMinimumSamplingInterval);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readMinimumSamplingIntervalAttribute(client, nodeReadWriteGeneric, &minimumSamplingInterval);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(minimumSamplingInterval == newMinimumSamplingInterval);
}
END_TEST

START_TEST(Node_ReadWrite_Historizing) {

    UA_Boolean historizing;
    UA_StatusCode retval = UA_Client_readHistorizingAttribute(client, nodeReadWriteInt, &historizing);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(historizing, UA_FALSE);

    UA_Boolean newHistorizing = UA_TRUE;

    retval = UA_Client_writeHistorizingAttribute(client, nodeReadWriteInt, &newHistorizing);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readHistorizingAttribute(client, nodeReadWriteInt, &historizing);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(historizing, newHistorizing);
}
END_TEST

START_TEST(Node_ReadWrite_Executable) {

    UA_Boolean executable;
    UA_StatusCode retval = UA_Client_readExecutableAttribute(client, nodeReadWriteMethod, &executable);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(executable, UA_TRUE);

    UA_Boolean newExecutable = UA_FALSE;

    retval = UA_Client_writeExecutableAttribute(client, nodeReadWriteMethod, &newExecutable);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Client_readExecutableAttribute(client, nodeReadWriteMethod, &executable);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(executable, newExecutable);
}
END_TEST

START_TEST(Node_ReadWrite_UserExecutable) {

    UA_Boolean userExecutable;
    UA_StatusCode retval = UA_Client_readUserExecutableAttribute(client, nodeReadWriteMethod, &userExecutable);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(userExecutable, UA_FALSE);

    UA_Boolean newUserExecutable = UA_TRUE;

    retval = UA_Client_writeUserExecutableAttribute(client, nodeReadWriteMethod, &newUserExecutable);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);

}
END_TEST

#endif

static Suite *testSuite_Client(void) {
    Suite *s = suite_create("Client Highlevel");
    TCase *tc_misc = tcase_create("Client Highlevel Misc");
    tcase_add_checked_fixture(tc_misc, setup, teardown);
    tcase_add_test(tc_misc, Misc_State);
    tcase_add_test(tc_misc, Misc_NamespaceGetIndex);
    suite_add_tcase(s, tc_misc);

    TCase *tc_nodes = tcase_create("Client Highlevel Node Management");
    tcase_add_checked_fixture(tc_nodes, setup, teardown);
#ifdef UA_ENABLE_NODEMANAGEMENT
    tcase_add_test(tc_nodes, Node_Add);
#endif
    tcase_add_test(tc_nodes, Node_Browse);
    tcase_add_test(tc_nodes, Node_Register);
    suite_add_tcase(s, tc_nodes);

#ifdef UA_ENABLE_NODEMANAGEMENT
    TCase *tc_readwrite = tcase_create("Client Highlevel Read/Write");
    tcase_add_unchecked_fixture(tc_readwrite, setup, teardown);
    // first add some nodes where we test on
    tcase_add_test(tc_readwrite, Node_AddReadWriteNodes);
    // Now run all the read write tests
    tcase_add_test(tc_readwrite, Node_ReadWrite_Id);
    tcase_add_test(tc_readwrite, Node_ReadWrite_Class);
    tcase_add_test(tc_readwrite, Node_ReadWrite_BrowseName);
    tcase_add_test(tc_readwrite, Node_ReadWrite_DisplayName);
    tcase_add_test(tc_readwrite, Node_ReadWrite_Description);
    tcase_add_test(tc_readwrite, Node_ReadWrite_WriteMask);
    tcase_add_test(tc_readwrite, Node_ReadWrite_UserWriteMask);
    tcase_add_test(tc_readwrite, Node_ReadWrite_IsAbstract);
    tcase_add_test(tc_readwrite, Node_ReadWrite_Symmetric);
    tcase_add_test(tc_readwrite, Node_ReadWrite_InverseName);
    tcase_add_test(tc_readwrite, Node_ReadWrite_ContainsNoLoops);
    tcase_add_test(tc_readwrite, Node_ReadWrite_EventNotifier);
    tcase_add_test(tc_readwrite, Node_ReadWrite_Value);
    tcase_add_test(tc_readwrite, Node_ReadWrite_DataType);
    tcase_add_test(tc_readwrite, Node_ReadWrite_ValueRank);
    tcase_add_test(tc_readwrite, Node_ReadWrite_ArrayDimensions);
    tcase_add_test(tc_readwrite, Node_ReadWrite_AccessLevel);
    tcase_add_test(tc_readwrite, Node_ReadWrite_UserAccessLevel);
    tcase_add_test(tc_readwrite, Node_ReadWrite_MinimumSamplingInterval);
    tcase_add_test(tc_readwrite, Node_ReadWrite_Historizing);
    tcase_add_test(tc_readwrite, Node_ReadWrite_Executable);
    tcase_add_test(tc_readwrite, Node_ReadWrite_UserExecutable);
    suite_add_tcase(s, tc_readwrite);
#endif
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
