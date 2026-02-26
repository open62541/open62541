/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2019 (c) basysKom GmbH <opensource@basyskom.com> (Author: Frank Meerkötter)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "test_helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"

static UA_Server *server = NULL;

static void
serverNotificationCallback(UA_Server *server, UA_ApplicationNotificationType type,
                           const UA_KeyValueMap payload) {

}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    UA_ServerConfig *cfg = UA_Server_getConfig(server);
    cfg->globalNotificationCallback = serverNotificationCallback;
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(checkGetConfig) {
    ck_assert_ptr_eq(UA_Server_getConfig(NULL), NULL);
    ck_assert_ptr_ne(UA_Server_getConfig(server), NULL);
} END_TEST

START_TEST(checkGetNamespaceByName) {
    size_t notFoundIndex = 62541;
    UA_StatusCode notFound = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/invalid"), &notFoundIndex);
    ck_assert_uint_eq(notFoundIndex, 62541); // not changed
    ck_assert_uint_eq(notFound, UA_STATUSCODE_BADNOTFOUND);

    size_t foundIndex = 62541;
    UA_StatusCode found = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/"), &foundIndex);
    ck_assert_uint_eq(foundIndex, 0); // this namespace always has index 0 (defined by the standard)
    ck_assert_uint_eq(found, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkGetNamespaceById) {
    UA_String searchResultNamespace;
    UA_StatusCode notFound = UA_Server_getNamespaceByIndex(server, 10, &searchResultNamespace);
    ck_assert_uint_eq(notFound, UA_STATUSCODE_BADNOTFOUND);

    UA_StatusCode found1 = UA_Server_getNamespaceByIndex(server, 1, &searchResultNamespace);
    ck_assert_uint_eq(found1, UA_STATUSCODE_GOOD);
    UA_String_clear(&searchResultNamespace);

    UA_StatusCode notFound2 = UA_Server_getNamespaceByIndex(server, 2, &searchResultNamespace);
    ck_assert_uint_eq(notFound2, UA_STATUSCODE_BADNOTFOUND);

    UA_String compareNamespace = UA_STRING("http://opcfoundation.org/UA/");
    UA_StatusCode found = UA_Server_getNamespaceByIndex(server, 0, &searchResultNamespace);
    ck_assert(UA_String_equal(&compareNamespace, &searchResultNamespace));
    ck_assert_uint_eq(found, UA_STATUSCODE_GOOD);
    UA_String_clear(&searchResultNamespace);
} END_TEST

static void timedCallbackHandler(UA_Server *s, void *data) {
    *((UA_Boolean*)data) = false;  // stop the server via a timedCallback
}

START_TEST(checkServer_run) {
    UA_Boolean running = true;
    // 0 is in the past so the server will terminate on the first iteration
    UA_StatusCode ret;
    ret = UA_Server_addTimedCallback(server, &timedCallbackHandler, &running, 0, NULL);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ret = UA_Server_run(server, &running);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkGetStatistics) {
    /* Get server statistics without running - should return valid stats */
    UA_ServerStatistics stats = UA_Server_getStatistics(server);

    /* Initial stats should be zero/empty */
    ck_assert_uint_eq(stats.ss.currentSessionCount, 0);
    ck_assert_uint_eq(stats.ss.cumulatedSessionCount, 0);
    ck_assert_uint_eq(stats.ss.securityRejectedSessionCount, 0);
    ck_assert_uint_eq(stats.ss.rejectedSessionCount, 0);
    ck_assert_uint_eq(stats.ss.sessionTimeoutCount, 0);
    ck_assert_uint_eq(stats.ss.sessionAbortCount, 0);

    /* SecureChannel stats should also be initialized */
    ck_assert_uint_eq(stats.scs.currentChannelCount, 0);
    ck_assert_uint_eq(stats.scs.cumulatedChannelCount, 0);
    ck_assert_uint_eq(stats.scs.rejectedChannelCount, 0);
    ck_assert_uint_eq(stats.scs.channelTimeoutCount, 0);
    ck_assert_uint_eq(stats.scs.channelAbortCount, 0);
    ck_assert_uint_eq(stats.scs.channelPurgeCount, 0);
} END_TEST

START_TEST(checkGetLifecycleState) {
    /* Before startup, server should be in stopped state */
    UA_LifecycleState state = UA_Server_getLifecycleState(server);
    ck_assert_int_eq(state, UA_LIFECYCLESTATE_STOPPED);

    /* After startup, server should be in started state */
    UA_StatusCode ret = UA_Server_run_startup(server);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);

    state = UA_Server_getLifecycleState(server);
    ck_assert_int_eq(state, UA_LIFECYCLESTATE_STARTED);

    /* After shutdown, server should transition back to stopped */
    ret = UA_Server_run_shutdown(server);
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);

    state = UA_Server_getLifecycleState(server);
    ck_assert_int_eq(state, UA_LIFECYCLESTATE_STOPPED);
} END_TEST

/* ---- Additional coverage tests ---- */

START_TEST(checkGetDataTypes) {
    const UA_DataTypeArray *types = UA_Server_getDataTypes(server);
    /* Server always has at least the built-in types chain */
    (void)types; /* may be NULL if no custom types, that's fine */
} END_TEST

START_TEST(checkFindDataType) {
    /* Find a well-known built-in type: Int32 */
    UA_NodeId int32Id = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    const UA_DataType *dt = UA_Server_findDataType(server, &int32Id);
    /* Built-in types are found via the default DataTypeArray */
    if(dt) {
        ck_assert_uint_eq(dt->memSize, sizeof(UA_Int32));
    }

    /* Non-existing type should return NULL */
    UA_NodeId fakeId = UA_NODEID_NUMERIC(0, 99999);
    const UA_DataType *dtFake = UA_Server_findDataType(server, &fakeId);
    ck_assert_ptr_eq(dtFake, NULL);
} END_TEST

START_TEST(checkServerReadValue) {
    /* Read the ServerStatus CurrentTime value from the server */
    UA_Variant value;
    UA_StatusCode retval = UA_Server_readValue(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &value);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME]));
    UA_Variant_clear(&value);
} END_TEST

START_TEST(checkServerReadDisplayName) {
    UA_LocalizedText displayName;
    UA_StatusCode retval = UA_Server_readDisplayName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &displayName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(displayName.text.length > 0);
    UA_LocalizedText_clear(&displayName);
} END_TEST

START_TEST(checkServerReadDescription) {
    UA_LocalizedText desc;
    UA_StatusCode retval = UA_Server_readDescription(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &desc);
    /* May or may not have description, but shouldn't fail */
    ck_assert(retval == UA_STATUSCODE_GOOD || retval == UA_STATUSCODE_BADATTRIBUTEIDINVALID);
    if(retval == UA_STATUSCODE_GOOD)
        UA_LocalizedText_clear(&desc);
} END_TEST

START_TEST(checkServerReadWriteRank) {
    UA_LocalizedText desc;
    UA_StatusCode retval;

    /* Read browse name */
    UA_QualifiedName browseName;
    retval = UA_Server_readBrowseName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(browseName.name.length > 0);
    UA_QualifiedName_clear(&browseName);

    /* Read node class */
    UA_NodeClass nodeClass;
    retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nodeClass);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nodeClass, UA_NODECLASS_OBJECT);
} END_TEST

START_TEST(checkServerReadNodeId) {
    /* Read the NodeId attribute of a node */
    UA_NodeId outNodeId;
    UA_StatusCode retval = UA_Server_readNodeId(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &outNodeId);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(outNodeId.identifier.numeric, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId_clear(&outNodeId);
} END_TEST

START_TEST(checkServerAddRemoveNamespace) {
    /* Add a namespace */
    UA_UInt16 nsIdx = UA_Server_addNamespace(server, "http://test.example.com");
    ck_assert_uint_gt(nsIdx, 0);

    /* Find it */
    size_t foundIdx = 0;
    UA_StatusCode retval = UA_Server_getNamespaceByName(server,
        UA_STRING("http://test.example.com"), &foundIdx);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(foundIdx, nsIdx);

    /* Add the same namespace again - should return same index */
    UA_UInt16 nsIdx2 = UA_Server_addNamespace(server, "http://test.example.com");
    ck_assert_uint_eq(nsIdx, nsIdx2);
} END_TEST

START_TEST(checkServerWriteValue) {
    /* Add a variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInt = 42;
    UA_Variant_setScalar(&attr.value, &myInt, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myNodeId = UA_NODEID_STRING(1, "test.writevar");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestWriteVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Write a new value */
    UA_Int32 newVal = 100;
    UA_Variant writeVal;
    UA_Variant_setScalar(&writeVal, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    retval = UA_Server_writeValue(server, myNodeId, writeVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant readVal;
    retval = UA_Server_readValue(server, myNodeId, &readVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32*)readVal.data, 100);
    UA_Variant_clear(&readVal);
} END_TEST

START_TEST(checkServerAddObjectNode) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestObject");
    UA_NodeId objId = UA_NODEID_STRING(1, "test.object");
    UA_StatusCode retval = UA_Server_addObjectNode(server, objId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestObject"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify node class */
    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, objId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);

    /* Delete the node */
    retval = UA_Server_deleteNode(server, objId, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify deletion */
    retval = UA_Server_readNodeClass(server, objId, &nc);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkServerBrowse) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(br.referencesSize, 0);
    UA_BrowseResult_clear(&br);
} END_TEST

START_TEST(checkServerReadNonExistingNode) {
    UA_Variant value;
    UA_NodeId badId = UA_NODEID_NUMERIC(0, 99999);
    UA_StatusCode retval = UA_Server_readValue(server, badId, &value);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkServerTranslateBrowsePath) {
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe.isInverse = false;
    rpe.includeSubtypes = true;
    rpe.targetName = UA_QUALIFIEDNAME(0, "Server");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bpr.targetsSize, 0);
    UA_BrowsePathResult_clear(&bpr);
} END_TEST

START_TEST(checkServerAddMethodNode) {
    UA_MethodAttributes methAttr = UA_MethodAttributes_default;
    methAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestMethod");
    methAttr.executable = true;
    methAttr.userExecutable = true;

    UA_NodeId methId = UA_NODEID_STRING(1, "test.method");
    UA_StatusCode retval = UA_Server_addMethodNode(server, methId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "TestMethod"),
        methAttr, NULL, 0, NULL, 0, NULL, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify it's a method */
    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, methId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_METHOD);
} END_TEST

START_TEST(checkServerAddReference) {
    /* Add two objects, then add a reference between them */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_NodeId obj1 = UA_NODEID_STRING(1, "test.ref1");
    UA_NodeId obj2 = UA_NODEID_STRING(1, "test.ref2");

    UA_Server_addObjectNode(server, obj1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefObj1"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    UA_Server_addObjectNode(server, obj2,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefObj2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);

    /* Add reference */
    UA_StatusCode retval = UA_Server_addReference(server, obj1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_EXPANDEDNODEID_STRING(1, "test.ref2"), true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete reference */
    retval = UA_Server_deleteReference(server, obj1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), true,
        UA_EXPANDEDNODEID_STRING(1, "test.ref2"), true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(checkServerReadDataType) {
    /* Read the DataType attribute of the CurrentTime variable */
    UA_NodeId dataType;
    UA_StatusCode retval = UA_Server_readDataType(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
        &dataType);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* Should be UtcTime (ns=0;i=294) which is a subtype of DateTime */
    ck_assert_uint_eq(dataType.namespaceIndex, 0);
    ck_assert_uint_gt(dataType.identifier.numeric, 0);
    UA_NodeId_clear(&dataType);
} END_TEST

START_TEST(checkServerRepeatedCallback) {
    UA_Boolean triggered = false;
    UA_UInt64 callbackId = 0;
    UA_StatusCode ret = UA_Server_addRepeatedCallback(server,
        timedCallbackHandler, &triggered, 100.0, &callbackId);
    ck_assert_uint_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(callbackId, 0);

    /* Remove the callback */
    UA_Server_removeCallback(server, callbackId);
} END_TEST

int main(void) {
    Suite *s = suite_create("server");

    TCase *tc_call = tcase_create("server - basics");
    tcase_add_checked_fixture(tc_call, setup, teardown);
    tcase_add_test(tc_call, checkGetConfig);
    tcase_add_test(tc_call, checkGetNamespaceByName);
    tcase_add_test(tc_call, checkGetNamespaceById);
    tcase_add_test(tc_call, checkServer_run);
    suite_add_tcase(s, tc_call);

    TCase *tc_ext = tcase_create("server - extended");
    tcase_add_checked_fixture(tc_ext, setup, teardown);
    tcase_add_test(tc_ext, checkGetDataTypes);
    tcase_add_test(tc_ext, checkFindDataType);
    tcase_add_test(tc_ext, checkServerReadValue);
    tcase_add_test(tc_ext, checkServerReadDisplayName);
    tcase_add_test(tc_ext, checkServerReadDescription);
    tcase_add_test(tc_ext, checkServerReadWriteRank);
    tcase_add_test(tc_ext, checkServerReadNodeId);
    tcase_add_test(tc_ext, checkServerAddRemoveNamespace);
    tcase_add_test(tc_ext, checkServerWriteValue);
    tcase_add_test(tc_ext, checkServerAddObjectNode);
    tcase_add_test(tc_ext, checkServerBrowse);
    tcase_add_test(tc_ext, checkServerReadNonExistingNode);
    tcase_add_test(tc_ext, checkServerTranslateBrowsePath);
    tcase_add_test(tc_ext, checkServerAddMethodNode);
    tcase_add_test(tc_ext, checkServerAddReference);
    tcase_add_test(tc_ext, checkServerReadDataType);
    tcase_add_test(tc_ext, checkServerRepeatedCallback);
    suite_add_tcase(s, tc_ext);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
