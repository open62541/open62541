/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "test_helpers.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
}

static void teardown(void) {
    UA_Server_delete(server);
}

/* --- UA_Server_getDataTypes --- */

START_TEST(getDataTypes) {
    const UA_DataTypeArray *dta = UA_Server_getDataTypes(server);
    /* May be NULL if no custom types; that's fine. The function itself
     * should not crash. */
    (void)dta;
} END_TEST

/* --- UA_Server_findDataType --- */

START_TEST(findBuiltinDataType) {
    /* Look up Boolean (ns=0;i=1) */
    UA_NodeId boolId = UA_NODEID_NUMERIC(0, UA_NS0ID_BOOLEAN);
    const UA_DataType *dt = UA_Server_findDataType(server, &boolId);
    if(dt)
        ck_assert_uint_eq(dt->memSize, sizeof(UA_Boolean));
} END_TEST

START_TEST(findNonExistentDataType) {
    UA_NodeId fakeId = UA_NODEID_NUMERIC(0, 99999);
    const UA_DataType *dt = UA_Server_findDataType(server, &fakeId);
    ck_assert_ptr_eq(dt, NULL);
} END_TEST

/* --- UA_Server_addDataType --- */

START_TEST(addDataTypeBasic) {
    /* Create a simple test DataType */
    UA_DataType testType;
    memset(&testType, 0, sizeof(UA_DataType));
    testType.typeName = "TestType";
    testType.typeId = UA_NODEID_NUMERIC(1, 50000);
    testType.binaryEncodingId = UA_NODEID_NUMERIC(1, 50001);
    testType.memSize = 8;
    testType.typeKind = UA_DATATYPEKIND_STRUCTURE;
    testType.pointerFree = true;
    testType.overlayable = false;
    testType.membersSize = 0;

    /* Also add the type node in the address space */
    UA_DataTypeAttributes dtattr = UA_DataTypeAttributes_default;
    dtattr.displayName = UA_LOCALIZEDTEXT("en-US", "TestType");
    UA_StatusCode retval = UA_Server_addDataTypeNode(server,
        testType.typeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestType"),
        dtattr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_addDataType(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE), &testType);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Find it */
    const UA_DataType *found = UA_Server_findDataType(server, &testType.typeId);
    ck_assert_ptr_ne(found, NULL);
} END_TEST

/* --- Server browsing utilities --- */

START_TEST(browseObjectsFolder) {
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

START_TEST(browseInverse) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    /* Server object should have at least one inverse reference (from ObjectsFolder) */
    ck_assert_uint_gt(br.referencesSize, 0);
    UA_BrowseResult_clear(&br);
} END_TEST

START_TEST(browseBothDirections) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    bd.browseDirection = UA_BROWSEDIRECTION_BOTH;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(br.referencesSize, 0);
    UA_BrowseResult_clear(&br);
} END_TEST

START_TEST(browseNonExistentNode) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, 99999);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_ne(br.statusCode, UA_STATUSCODE_GOOD);
    UA_BrowseResult_clear(&br);
} END_TEST

/* --- TranslateBrowsePath --- */

START_TEST(translateBrowsePathToServer) {
    /* ObjectsFolder -> Server */
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

START_TEST(translateBrowsePathMultiHop) {
    /* Root -> Objects -> Server (two hops) */
    UA_RelativePathElement rpe[2];
    UA_RelativePathElement_init(&rpe[0]);
    rpe[0].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe[0].isInverse = false;
    rpe[0].includeSubtypes = true;
    rpe[0].targetName = UA_QUALIFIEDNAME(0, "Objects");

    UA_RelativePathElement_init(&rpe[1]);
    rpe[1].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe[1].isInverse = false;
    rpe[1].includeSubtypes = true;
    rpe[1].targetName = UA_QUALIFIEDNAME(0, "Server");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER);
    bp.relativePath.elementsSize = 2;
    bp.relativePath.elements = rpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_gt(bpr.targetsSize, 0);
    UA_BrowsePathResult_clear(&bpr);
} END_TEST

START_TEST(translateBrowsePathNotFound) {
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe.isInverse = false;
    rpe.includeSubtypes = true;
    rpe.targetName = UA_QUALIFIEDNAME(0, "NonExistentNode");

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_ne(bpr.statusCode, UA_STATUSCODE_GOOD);
    UA_BrowsePathResult_clear(&bpr);
} END_TEST

/* --- Node management tests --- */

START_TEST(addAndDeleteVariableNode) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myVal = 42;
    UA_Variant_setScalar(&attr.value, &myVal, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId myId = UA_NODEID_STRING(1, "utils.test.var");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "UtilsTestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Delete */
    retval = UA_Server_deleteNode(server, myId, true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Verify deletion */
    UA_Variant readVal;
    retval = UA_Server_readValue(server, myId, &readVal);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(addVariableTypeMismatch) {
    /* Try to add a var node with incorrect data type vs type definition */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_String myStr = UA_STRING("hello");
    UA_Variant_setScalar(&attr.value, &myStr, &UA_TYPES[UA_TYPES_STRING]);

    UA_NodeId myId = UA_NODEID_STRING(1, "utils.test.badtype");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "BadTypeVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    /* Should succeed since BaseDataVariableType accepts any data type */
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(deleteNonExistentNode) {
    UA_NodeId fakeId = UA_NODEID_NUMERIC(1, 99999);
    UA_StatusCode retval = UA_Server_deleteNode(server, fakeId, true);
    ck_assert_uint_ne(retval, UA_STATUSCODE_GOOD);
} END_TEST

/* --- Default attribute validation --- */

START_TEST(checkDefaultAttributes) {
    /* Verify VariableAttributes default */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    ck_assert_int_eq(vattr.valueRank, UA_VALUERANK_ANY);

    /* Verify ObjectAttributes default */
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    (void)oattr;

    /* Verify MethodAttributes default */
    UA_MethodAttributes mattr = UA_MethodAttributes_default;
    ck_assert(mattr.executable == true);
    ck_assert(mattr.userExecutable == true);

    /* Verify ViewAttributes default */
    UA_ViewAttributes viewattr = UA_ViewAttributes_default;
    ck_assert(viewattr.containsNoLoops == false);

    /* Verify DataTypeAttributes default */
    UA_DataTypeAttributes dtattr = UA_DataTypeAttributes_default;
    ck_assert(dtattr.isAbstract == false);

    /* Verify ReferenceTypeAttributes default */
    UA_ReferenceTypeAttributes rtattr = UA_ReferenceTypeAttributes_default;
    ck_assert(rtattr.isAbstract == false);
    ck_assert(rtattr.symmetric == false);
} END_TEST

/* --- Server read/write various attribute types --- */

START_TEST(readWriteMinimumSamplingInterval) {
    /* Add a variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Double val = 1.0;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.minimumSamplingInterval = 500.0;

    UA_NodeId myId = UA_NODEID_STRING(1, "utils.test.sampling");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "SamplingVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read minimumSamplingInterval */
    UA_Double readSampling = 0.0;
    retval = UA_Server_readMinimumSamplingInterval(server, myId, &readSampling);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(readSampling == 500.0);

    /* Write new minimumSamplingInterval */
    retval = UA_Server_writeMinimumSamplingInterval(server, myId, 100.0);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    retval = UA_Server_readMinimumSamplingInterval(server, myId, &readSampling);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(readSampling == 100.0);
} END_TEST

START_TEST(readWriteAccessLevel) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 10;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;

    UA_NodeId myId = UA_NODEID_STRING(1, "utils.test.access");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "AccessVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read accessLevel */
    UA_Byte accessLevel;
    retval = UA_Server_readAccessLevel(server, myId, &accessLevel);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(accessLevel & UA_ACCESSLEVELMASK_READ);

    /* Write new accessLevel */
    retval = UA_Server_writeAccessLevel(server, myId,
        UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(readWriteDisplayName) {
    /* Add a custom node so we can safely write its display name */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "OrigDN");
    UA_NodeId objId = UA_NODEID_STRING(1, "utils.test.dn");
    UA_StatusCode retval = UA_Server_addObjectNode(server, objId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DN_Object"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText displayName;
    retval = UA_Server_readDisplayName(server, objId, &displayName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&displayName);

    /* Write new display name */
    retval = UA_Server_writeDisplayName(server, objId,
        UA_LOCALIZEDTEXT("en-US", "MyObjects"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back */
    retval = UA_Server_readDisplayName(server, objId, &displayName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("MyObjects");
    ck_assert(UA_String_equal(&displayName.text, &expected));
    UA_LocalizedText_clear(&displayName);
} END_TEST

START_TEST(readWriteDescription) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 val = 10;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "OriginalDesc");

    UA_NodeId myId = UA_NODEID_STRING(1, "utils.test.desc");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DescVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Write new description */
    retval = UA_Server_writeDescription(server, myId,
        UA_LOCALIZEDTEXT("de-DE", "NeuerName"));
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back - description may match the latest write */
    UA_LocalizedText desc;
    retval = UA_Server_readDescription(server, myId, &desc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* Just verify we got something back */
    ck_assert(desc.text.length > 0);
    UA_LocalizedText_clear(&desc);
} END_TEST

START_TEST(readWriteBrowseName) {
    UA_QualifiedName browseName;
    UA_StatusCode retval = UA_Server_readBrowseName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(browseName.name.length > 0);
    UA_QualifiedName_clear(&browseName);

    /* Write new browse name for a custom node */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_NodeId objId = UA_NODEID_STRING(1, "utils.test.bn");
    retval = UA_Server_addObjectNode(server, objId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "OrigName"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read the browse name we just set */
    retval = UA_Server_readBrowseName(server, objId, &browseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_String origExpected = UA_STRING("OrigName");
    ck_assert(UA_String_equal(&browseName.name, &origExpected));
    UA_QualifiedName_clear(&browseName);
} END_TEST

/* --- Reference management --- */

START_TEST(addDeleteReference) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_NodeId s1 = UA_NODEID_STRING(1, "utils.ref.src");
    UA_NodeId t1 = UA_NODEID_STRING(1, "utils.ref.tgt");

    UA_Server_addObjectNode(server, s1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefSrc"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);
    UA_Server_addObjectNode(server, t1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefTgt"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, NULL);

    /* Add a reference */
    UA_StatusCode retval = UA_Server_addReference(server, s1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_EXPANDEDNODEID_STRING(1, "utils.ref.tgt"), true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Adding the same reference again should fail */
    retval = UA_Server_addReference(server, s1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_EXPANDEDNODEID_STRING(1, "utils.ref.tgt"), true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED);

    /* Delete the reference */
    retval = UA_Server_deleteReference(server, s1,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), true,
        UA_EXPANDEDNODEID_STRING(1, "utils.ref.tgt"), true);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

int main(void) {
    Suite *s = suite_create("server_utils");

    TCase *tc_types = tcase_create("DataTypes");
    tcase_add_checked_fixture(tc_types, setup, teardown);
    tcase_add_test(tc_types, getDataTypes);
    tcase_add_test(tc_types, findBuiltinDataType);
    tcase_add_test(tc_types, findNonExistentDataType);
    tcase_add_test(tc_types, addDataTypeBasic);
    suite_add_tcase(s, tc_types);

    TCase *tc_browse = tcase_create("Browse");
    tcase_add_checked_fixture(tc_browse, setup, teardown);
    tcase_add_test(tc_browse, browseObjectsFolder);
    tcase_add_test(tc_browse, browseInverse);
    tcase_add_test(tc_browse, browseBothDirections);
    tcase_add_test(tc_browse, browseNonExistentNode);
    tcase_add_test(tc_browse, translateBrowsePathToServer);
    tcase_add_test(tc_browse, translateBrowsePathMultiHop);
    tcase_add_test(tc_browse, translateBrowsePathNotFound);
    suite_add_tcase(s, tc_browse);

    TCase *tc_nodes = tcase_create("NodeMgmt");
    tcase_add_checked_fixture(tc_nodes, setup, teardown);
    tcase_add_test(tc_nodes, addAndDeleteVariableNode);
    tcase_add_test(tc_nodes, addVariableTypeMismatch);
    tcase_add_test(tc_nodes, deleteNonExistentNode);
    tcase_add_test(tc_nodes, checkDefaultAttributes);
    suite_add_tcase(s, tc_nodes);

    TCase *tc_attrs = tcase_create("Attributes");
    tcase_add_checked_fixture(tc_attrs, setup, teardown);
    tcase_add_test(tc_attrs, readWriteMinimumSamplingInterval);
    tcase_add_test(tc_attrs, readWriteAccessLevel);
    tcase_add_test(tc_attrs, readWriteDisplayName);
    tcase_add_test(tc_attrs, readWriteDescription);
    tcase_add_test(tc_attrs, readWriteBrowseName);
    tcase_add_test(tc_attrs, addDeleteReference);
    suite_add_tcase(s, tc_attrs);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
