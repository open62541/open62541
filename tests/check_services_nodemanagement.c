/* This Source Code Form is subject to the terms of the Mozilla Public
*  License, v. 2.0. If a copy of the MPL was not distributed with this 
*  file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"
#include "server/ua_nodestore.h"
#include "server/ua_services.h"
#include "ua_client.h"
#include "ua_nodeids.h"
#include "ua_types.h"
#include "ua_config_standard.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_MULTITHREADING
#include <pthread.h>
#include <urcu.h>
#endif

static UA_StatusCode
instantiationMethod(UA_NodeId newNodeId, UA_NodeId templateId, void *handle ) {
  *((UA_Int32 *) handle) += 1;
  return UA_STATUSCODE_GOOD;
}
START_TEST(AddVariableNode) {
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);

    /* add a variable node to the address space */
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                                                  myIntegerName, UA_NODEID_NULL, attr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    UA_Server_delete(server);
} END_TEST

START_TEST(AddComplexTypeWithInheritance) {
  UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
  
  /* add a variable node to the address space */
  UA_ObjectAttributes attr;
  UA_ObjectAttributes_init(&attr);
  attr.description = UA_LOCALIZEDTEXT("en_US","fakeServerStruct");
  attr.displayName = UA_LOCALIZEDTEXT("en_US","fakeServerStruct");
  
  UA_NodeId myObjectNodeId = UA_NODEID_STRING(1, "the.fake.Server.Struct");
  UA_QualifiedName myObjectName = UA_QUALIFIEDNAME(1, "the.fake.Server.Struct");
  UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
  UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
  UA_Int32 handleCalled = 0;
  UA_InstantiationCallback iCallback = {.method=instantiationMethod, .handle = (void *) &handleCalled};
    
  UA_StatusCode res = UA_Server_addObjectNode(server, myObjectNodeId, parentNodeId, parentReferenceNodeId,
                                                myObjectName, UA_NODEID_NUMERIC(0, 2004), attr, &iCallback, NULL);
  ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
  ck_assert_int_gt(handleCalled, 0); // Should be 58, but may depend on NS0 XML detail
  UA_Server_delete(server);
} END_TEST

START_TEST(AddNodeTwiceGivesError) {
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);

    /* add a variable node to the address space */
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                                                  myIntegerName, UA_NODEID_NULL, attr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    res = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                                    myIntegerName, UA_NODEID_NULL, attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNODEIDEXISTS);
    UA_Server_delete(server);
} END_TEST

static UA_Boolean constructorCalled = false;

static void * objectConstructor(const UA_NodeId instance) {
    constructorCalled = true;
    return NULL;
}

START_TEST(AddObjectWithConstructor) {
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);

    /* Add an object type */
    UA_NodeId objecttypeid = UA_NODEID_NUMERIC(0, 13371337);
    UA_ObjectTypeAttributes attr;
    UA_ObjectTypeAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US","my objecttype");
    UA_StatusCode res = UA_Server_addObjectTypeNode(server, objecttypeid,
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                    UA_QUALIFIEDNAME(0, "myobjecttype"), attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Add a constructor to the object type */
    UA_ObjectLifecycleManagement olm = {objectConstructor, NULL};
    res = UA_Server_setObjectTypeNode_lifecycleManagement(server, objecttypeid, olm);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Add an object of the type */
    UA_ObjectAttributes attr2;
    UA_ObjectAttributes_init(&attr2);
    attr2.displayName = UA_LOCALIZEDTEXT("en_US","my object");
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(0, ""),
                                  objecttypeid, attr2, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Verify that the constructor was called */
    ck_assert_int_eq(constructorCalled, true);

    UA_Server_delete(server);
} END_TEST

static UA_Boolean destructorCalled = false;

static void objectDestructor(const UA_NodeId instance, void *handle) {
    destructorCalled = true;
}

START_TEST(DeleteObjectWithDestructor) {
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);

    /* Add an object type */
    UA_NodeId objecttypeid = UA_NODEID_NUMERIC(0, 13371337);
    UA_ObjectTypeAttributes attr;
    UA_ObjectTypeAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US","my objecttype");
    UA_StatusCode res = UA_Server_addObjectTypeNode(server, objecttypeid,
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                                    UA_QUALIFIEDNAME(0, "myobjecttype"), attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Add a constructor to the object type */
    UA_ObjectLifecycleManagement olm = {NULL, objectDestructor};
    res = UA_Server_setObjectTypeNode_lifecycleManagement(server, objecttypeid, olm);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Add an object of the type */
    UA_NodeId objectid = UA_NODEID_NUMERIC(0, 23372337);
    UA_ObjectAttributes attr2;
    UA_ObjectAttributes_init(&attr2);
    attr2.displayName = UA_LOCALIZEDTEXT("en_US","my object");
    res = UA_Server_addObjectNode(server, objectid, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(0, ""),
                                  objecttypeid, attr2, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Delete the object */
    UA_Server_deleteNode(server, objectid, true);

    /* Verify that the destructor was called */
    ck_assert_int_eq(destructorCalled, true);

    UA_Server_delete(server);
} END_TEST

START_TEST(DeleteObjectAndReferences) {
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);

    /* Add an object of the type */
    UA_ObjectAttributes attr;
    UA_ObjectAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US","my object");
    UA_NodeId objectid = UA_NODEID_NUMERIC(0, 23372337);
    UA_StatusCode res;
    res = UA_Server_addObjectNode(server, objectid, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(0, ""),
                                  UA_NODEID_NULL, attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Verify that we have a reference to the node from the objects folder */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    size_t refCount = 0;
    for(size_t i = 0; i < br.referencesSize; ++i) {
        if(UA_NodeId_equal(&br.references[i].nodeId.nodeId, &objectid))
            refCount++;
    }
    ck_assert_int_eq(refCount, 1);
    UA_BrowseResult_deleteMembers(&br);

    /* Delete the object */
    UA_Server_deleteNode(server, objectid, true);

    /* Browse again, this time we expect that no reference is found */
    br = UA_Server_browse(server, 0, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    refCount = 0;
    for(size_t i = 0; i < br.referencesSize; ++i) {
        if(UA_NodeId_equal(&br.references[i].nodeId.nodeId, &objectid))
            refCount++;
    }
    ck_assert_int_eq(refCount, 0);
    UA_BrowseResult_deleteMembers(&br);

    /* Add an object the second time */
    UA_ObjectAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US","my object");
    objectid = UA_NODEID_NUMERIC(0, 23372337);
    res = UA_Server_addObjectNode(server, objectid, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), UA_QUALIFIEDNAME(0, ""),
                                  UA_NODEID_NULL, attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Browse again, this time we expect that a single reference to the node is found */
    refCount = 0;
    br = UA_Server_browse(server, 0, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < br.referencesSize; ++i) {
        if(UA_NodeId_equal(&br.references[i].nodeId.nodeId, &objectid))
            refCount++;
    }
    ck_assert_int_eq(refCount, 1);
    UA_BrowseResult_deleteMembers(&br);

    UA_Server_delete(server);
} END_TEST

static Suite * testSuite_services_nodemanagement(void) {
    Suite *s = suite_create("services_nodemanagement");

    TCase *tc_addnodes = tcase_create("addnodes");
    tcase_add_test(tc_addnodes, AddVariableNode);
    tcase_add_test(tc_addnodes, AddComplexTypeWithInheritance);
    tcase_add_test(tc_addnodes, AddNodeTwiceGivesError);
    tcase_add_test(tc_addnodes, AddObjectWithConstructor);

    TCase *tc_deletenodes = tcase_create("deletenodes");
    tcase_add_test(tc_addnodes, DeleteObjectWithDestructor);
    tcase_add_test(tc_addnodes, DeleteObjectAndReferences);

    suite_add_tcase(s, tc_addnodes);
    suite_add_tcase(s, tc_deletenodes);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s;
    s = testSuite_services_nodemanagement();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
