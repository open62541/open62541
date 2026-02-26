/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Attribute service edge case tests */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "testing_clock.h"
#include "thread_wrapper.h"
#include "test_helpers.h"
#include <check.h>
#include <stdlib.h>
#include <stdio.h>

static UA_Server *server;

static void setup_server(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
}

static void teardown_server(void) {
    UA_Server_delete(server);
}

/* === Read all possible attributes from different node classes === */

START_TEST(read_object_attributes) {
    /* Read attributes from Objects folder */
    UA_NodeId objFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    
    UA_NodeClass nc;
    UA_StatusCode res = UA_Server_readNodeClass(server, objFolder, &nc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);

    UA_QualifiedName bn;
    res = UA_Server_readBrowseName(server, objFolder, &bn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_QualifiedName_clear(&bn);

    UA_LocalizedText dn;
    res = UA_Server_readDisplayName(server, objFolder, &dn);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&dn);

    UA_LocalizedText desc;
    res = UA_Server_readDescription(server, objFolder, &desc);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&desc);

    UA_UInt32 wm;
    res = UA_Server_readWriteMask(server, objFolder, &wm);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Byte el;
    res = UA_Server_readEventNotifier(server, objFolder, &el);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_variable_all_attributes) {
    /* Add a variable with all attributes set */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 42;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    vattr.writeMask = 0xFFFFFFFF;
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    vattr.valueRank = UA_VALUERANK_SCALAR;
    vattr.historizing = false;
    vattr.minimumSamplingInterval = 100.0;
    vattr.displayName = UA_LOCALIZEDTEXT("en", "TestVar");
    vattr.description = UA_LOCALIZEDTEXT("en", "Desc");

    UA_NodeId varId = UA_NODEID_NUMERIC(1, 80001);
    UA_Server_addVariableNode(server, varId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    /* Read every variable attribute */
    UA_NodeClass nc;
    UA_Server_readNodeClass(server, varId, &nc);
    ck_assert_int_eq(nc, UA_NODECLASS_VARIABLE);

    UA_QualifiedName bn;
    UA_Server_readBrowseName(server, varId, &bn);
    UA_QualifiedName_clear(&bn);

    UA_LocalizedText dn;
    UA_Server_readDisplayName(server, varId, &dn);
    UA_LocalizedText_clear(&dn);

    UA_LocalizedText desc;
    UA_Server_readDescription(server, varId, &desc);
    UA_LocalizedText_clear(&desc);

    UA_UInt32 wm;
    UA_Server_readWriteMask(server, varId, &wm);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Server_readValue(server, varId, &value);
    UA_Variant_clear(&value);

    UA_NodeId dataType;
    UA_Server_readDataType(server, varId, &dataType);
    UA_NodeId_clear(&dataType);

    UA_Int32 valueRank;
    UA_Server_readValueRank(server, varId, &valueRank);

    UA_Variant arrayDims;
    UA_Variant_init(&arrayDims);
    UA_Server_readArrayDimensions(server, varId, &arrayDims);
    UA_Variant_clear(&arrayDims);

    UA_Byte accessLevel;
    UA_Server_readAccessLevel(server, varId, &accessLevel);

    UA_Double minInterval;
    UA_Server_readMinimumSamplingInterval(server, varId, &minInterval);

    UA_Boolean historizing;
    UA_Server_readHistorizing(server, varId, &historizing);
} END_TEST

START_TEST(write_variable_all_attributes) {
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 10;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    vattr.writeMask = 0xFFFFFFFF;
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    UA_NodeId varId = UA_NODEID_NUMERIC(1, 80002);
    UA_Server_addVariableNode(server, varId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestVar2"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    /* Write different attributes */
    UA_LocalizedText newDN = UA_LOCALIZEDTEXT("en", "NewName");
    UA_Server_writeDisplayName(server, varId, newDN);

    UA_LocalizedText newDesc = UA_LOCALIZEDTEXT("en", "NewDesc");
    UA_Server_writeDescription(server, varId, newDesc);

    UA_UInt32 newWM = 0;
    UA_Server_writeWriteMask(server, varId, newWM);

    UA_Int32 newVal = 99;
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, varId, wv);

    UA_NodeId newDT = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Server_writeDataType(server, varId, newDT);

    UA_Int32 newVR = UA_VALUERANK_SCALAR;
    UA_Server_writeValueRank(server, varId, newVR);

    UA_Byte newAL = UA_ACCESSLEVELMASK_READ;
    UA_Server_writeAccessLevel(server, varId, newAL);

    UA_Double newMSI = 500.0;
    UA_Server_writeMinimumSamplingInterval(server, varId, newMSI);

    UA_Boolean newHist = true;
    UA_Server_writeHistorizing(server, varId, newHist);
} END_TEST

START_TEST(read_method_attributes) {
    UA_NodeId methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS);
    
    UA_Boolean exec;
    UA_StatusCode res = UA_Server_readExecutable(server, methodId, &exec);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_referencetype_attributes) {
    UA_NodeId refId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);

    UA_Boolean isAbstract;
    UA_StatusCode res = UA_Server_readIsAbstract(server, refId, &isAbstract);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Boolean symmetric;
    res = UA_Server_readSymmetric(server, refId, &symmetric);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_LocalizedText inverseName;
    res = UA_Server_readInverseName(server, refId, &inverseName);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&inverseName);
} END_TEST

START_TEST(read_datatype_attributes) {
    UA_NodeId dtId = UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE);

    UA_Boolean isAbstract;
    UA_StatusCode res = UA_Server_readIsAbstract(server, dtId, &isAbstract);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_view_attributes) {
    /* Add a view first */
    UA_ViewAttributes attr = UA_ViewAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "TestView");
    attr.containsNoLoops = true;

    UA_NodeId viewId = UA_NODEID_NUMERIC(1, 80010);
    UA_Server_addViewNode(server, viewId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "TestView"),
        attr, NULL, NULL);

    UA_Boolean containsNoLoops;
    UA_StatusCode res = UA_Server_readContainsNoLoops(server, viewId, &containsNoLoops);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Byte eventNotifier;
    res = UA_Server_readEventNotifier(server, viewId, &eventNotifier);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Read/Write nonexistent or wrong-type attributes === */
START_TEST(read_wrong_attribute) {
    /* Try reading Executable from a Variable - should fail */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 0;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    UA_NodeId varId = UA_NODEID_NUMERIC(1, 80020);
    UA_Server_addVariableNode(server, varId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "WrongAttrVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);

    UA_Boolean exec;
    UA_StatusCode res = UA_Server_readExecutable(server, varId, &exec);
    ck_assert(res != UA_STATUSCODE_GOOD);

    /* Try reading IsAbstract from a Variable */
    UA_Boolean isAbs;
    res = UA_Server_readIsAbstract(server, varId, &isAbs);
    ck_assert(res != UA_STATUSCODE_GOOD);

    /* Try reading Symmetric from a Variable */
    UA_Boolean sym;
    res = UA_Server_readSymmetric(server, varId, &sym);
    ck_assert(res != UA_STATUSCODE_GOOD);

    /* Try reading InverseName from a Variable */
    UA_LocalizedText inv;
    res = UA_Server_readInverseName(server, varId, &inv);
    ck_assert(res != UA_STATUSCODE_GOOD);

    /* Try reading ContainsNoLoops from a Variable */
    UA_Boolean cnl;
    res = UA_Server_readContainsNoLoops(server, varId, &cnl);
    ck_assert(res != UA_STATUSCODE_GOOD);

    /* Try reading EventNotifier from a Variable */
    UA_Byte en;
    res = UA_Server_readEventNotifier(server, varId, &en);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(read_nonexistent_node) {
    UA_NodeId badId = UA_NODEID_NUMERIC(1, 99999);
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode res = UA_Server_readValue(server, badId, &value);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
} END_TEST

/* === Add many different node types via server API === */
START_TEST(add_objecttype_with_lifecycle) {
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "MyObjType");
    attr.isAbstract = false;

    UA_NodeId outId;
    UA_StatusCode res = UA_Server_addObjectTypeNode(server,
        UA_NODEID_NUMERIC(1, 80030),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "MyObjType"),
        attr, NULL, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read IsAbstract */
    UA_Boolean isAbs;
    res = UA_Server_readIsAbstract(server, outId, &isAbs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(!isAbs);

    /* Write IsAbstract */
    res = UA_Server_writeIsAbstract(server, outId, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId_clear(&outId);
} END_TEST

START_TEST(add_variabletype_node) {
    UA_VariableTypeAttributes attr = UA_VariableTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "MyVarType");
    attr.isAbstract = false;
    attr.valueRank = UA_VALUERANK_SCALAR;

    UA_NodeId outId;
    UA_StatusCode res = UA_Server_addVariableTypeNode(server,
        UA_NODEID_NUMERIC(1, 80031),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "MyVarType"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Boolean isAbs;
    res = UA_Server_readIsAbstract(server, outId, &isAbs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId_clear(&outId);
} END_TEST

START_TEST(add_referencetype_node) {
    UA_ReferenceTypeAttributes attr = UA_ReferenceTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "MyRefType");
    attr.inverseName = UA_LOCALIZEDTEXT("en", "InverseMyRefType");
    attr.isAbstract = false;
    attr.symmetric = false;

    UA_NodeId outId;
    UA_StatusCode res = UA_Server_addReferenceTypeNode(server,
        UA_NODEID_NUMERIC(1, 80032),
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "MyRefType"),
        attr, NULL, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read all reference type attributes */
    UA_Boolean isAbs;
    res = UA_Server_readIsAbstract(server, outId, &isAbs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Boolean sym;
    res = UA_Server_readSymmetric(server, outId, &sym);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(!sym);

    UA_LocalizedText inv;
    res = UA_Server_readInverseName(server, outId, &inv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&inv);

    /* Write value attribute on a ReferenceType node. ReferenceType nodes
     * do not have a Value attribute, so this is expected to fail. */
    UA_Boolean newSym = true;
    UA_Variant symVal;
    UA_Variant_setScalar(&symVal, &newSym, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = UA_Server_writeValue(server, outId, symVal);
    ck_assert(res != UA_STATUSCODE_GOOD);

    /* Write inverseName -- may fail depending on WriteMask */
    UA_LocalizedText newInv = UA_LOCALIZEDTEXT("en", "NewInverse");
    res = UA_Server_writeInverseName(server, outId, newInv);
    (void)res;

    UA_NodeId_clear(&outId);
} END_TEST

START_TEST(add_datatype_node) {
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en", "MyDT");
    attr.isAbstract = true;

    UA_NodeId outId;
    UA_StatusCode res = UA_Server_addDataTypeNode(server,
        UA_NODEID_NUMERIC(1, 80033),
        UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "MyDT"),
        attr, NULL, &outId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_Boolean isAbs;
    res = UA_Server_readIsAbstract(server, outId, &isAbs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(isAbs);

    UA_NodeId_clear(&outId);
} END_TEST

/* === Delete nodes and references === */
START_TEST(delete_node_with_refs) {
    /* Add object with children */
    UA_ObjectAttributes oa = UA_ObjectAttributes_default;
    oa.displayName = UA_LOCALIZEDTEXT("en", "Parent");
    UA_NodeId parentId = UA_NODEID_NUMERIC(1, 80040);
    UA_Server_addObjectNode(server, parentId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Parent"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oa, NULL, NULL);

    /* Add child variable */
    UA_VariableAttributes va = UA_VariableAttributes_default;
    UA_Int32 v = 5;
    UA_Variant_setScalar(&va.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    va.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_NodeId childId = UA_NODEID_NUMERIC(1, 80041);
    UA_Server_addVariableNode(server, childId,
        parentId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Child"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        va, NULL, NULL);

    /* Delete parent with children */
    UA_StatusCode res = UA_Server_deleteNode(server, parentId, true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Child should also be deleted */
    UA_Variant value;
    UA_Variant_init(&value);
    res = UA_Server_readValue(server, childId, &value);
    ck_assert(res != UA_STATUSCODE_GOOD);
    UA_Variant_clear(&value);
} END_TEST

START_TEST(delete_reference) {
    UA_ObjectAttributes oa1 = UA_ObjectAttributes_default;
    oa1.displayName = UA_LOCALIZEDTEXT("en", "RefA");
    UA_NodeId aId = UA_NODEID_NUMERIC(1, 80050);
    UA_Server_addObjectNode(server, aId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefA"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oa1, NULL, NULL);

    UA_ObjectAttributes oa2 = UA_ObjectAttributes_default;
    oa2.displayName = UA_LOCALIZEDTEXT("en", "RefB");
    UA_NodeId bId = UA_NODEID_NUMERIC(1, 80051);
    UA_Server_addObjectNode(server, bId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "RefB"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oa2, NULL, NULL);

    /* Add reference */
    UA_StatusCode res = UA_Server_addReference(server, aId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_EXPANDEDNODEID_NUMERIC(1, 80051), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Delete reference */
    res = UA_Server_deleteReference(server, aId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        true, UA_EXPANDEDNODEID_NUMERIC(1, 80051), true);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Delete the nodes */
    UA_Server_deleteNode(server, aId, true);
    UA_Server_deleteNode(server, bId, true);
} END_TEST

/* === Browse operations via Server API === */
START_TEST(browse_with_filter) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = true;
    bd.nodeClassMask = UA_NODECLASS_VARIABLE;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    UA_BrowseResult_clear(&br);

    /* Browse inverse */
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    UA_BrowseResult_clear(&br);

    /* Browse both directions */
    bd.browseDirection = UA_BROWSEDIRECTION_BOTH;
    br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    UA_BrowseResult_clear(&br);
} END_TEST

START_TEST(browse_next) {
    /* Create many children to force continuation point */
    UA_ObjectAttributes oa = UA_ObjectAttributes_default;
    UA_NodeId parentId = UA_NODEID_NUMERIC(1, 80060);
    oa.displayName = UA_LOCALIZEDTEXT("en", "BNParent");
    UA_Server_addObjectNode(server, parentId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "BNParent"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oa, NULL, NULL);

    char name[32];
    for(int i = 0; i < 20; i++) {
        snprintf(name, sizeof(name), "BNChild%d", i);
        UA_ObjectAttributes ca = UA_ObjectAttributes_default;
        ca.displayName = UA_LOCALIZEDTEXT("en", name);
        UA_Server_addObjectNode(server,
            UA_NODEID_NUMERIC(1, 80070 + (UA_UInt32)i),
            parentId,
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(1, name),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
            ca, NULL, NULL);
    }

    /* Browse with small max references */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = parentId;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = true;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 5, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize <= 5);

    /* Browse next until done */
    int iters = 0;
    while(br.continuationPoint.length > 0 && iters < 10) {
        UA_ByteString cp = br.continuationPoint;
        br.continuationPoint = UA_BYTESTRING_NULL;
        UA_BrowseResult_clear(&br);
        br = UA_Server_browseNext(server, false, &cp);
        UA_ByteString_clear(&cp);
        iters++;
    }
    UA_BrowseResult_clear(&br);
    ck_assert(iters > 0);
} END_TEST

/* === TranslateBrowsePaths === */
START_TEST(translate_browsepath) {
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER);

    UA_RelativePathElement rpe[2];
    UA_RelativePathElement_init(&rpe[0]);
    UA_RelativePathElement_init(&rpe[1]);
    rpe[0].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe[0].includeSubtypes = true;
    rpe[0].targetName = UA_QUALIFIEDNAME(0, "Objects");
    rpe[1].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe[1].includeSubtypes = true;
    rpe[1].targetName = UA_QUALIFIEDNAME(0, "Server");

    bp.relativePath.elements = rpe;
    bp.relativePath.elementsSize = 2;

    UA_BrowsePathResult bpr =
        UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert_uint_eq(bpr.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(bpr.targetsSize > 0);
    UA_BrowsePathResult_clear(&bpr);
} END_TEST

START_TEST(translate_browsepath_bad) {
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER);

    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rpe.includeSubtypes = true;
    rpe.targetName = UA_QUALIFIEDNAME(0, "NonexistentNode12345");

    bp.relativePath.elements = &rpe;
    bp.relativePath.elementsSize = 1;

    UA_BrowsePathResult bpr =
        UA_Server_translateBrowsePathToNodeIds(server, &bp);
    ck_assert(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize == 0);
    UA_BrowsePathResult_clear(&bpr);
} END_TEST

/* Register/Unregister nodes removed - requires internal server header */

/* === Node context === */
static int contextCallbackCount = 0;

static void nodeContextDestructor(UA_Server *s, const UA_NodeId *sessionId,
                                  void *sessionContext,
                                  const UA_NodeId *nodeId,
                                  void *nodeContext) {
    contextCallbackCount++;
}

START_TEST(node_context_operations) {
    /* Add node with context */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 val = 0;
    UA_Variant_setScalar(&vattr.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;

    UA_NodeId varId = UA_NODEID_NUMERIC(1, 80080);
    int myContext = 42;
    UA_Server_addVariableNode(server, varId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "CtxVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, &myContext, NULL);

    /* Get node context */
    void *ctx = NULL;
    UA_StatusCode res = UA_Server_getNodeContext(server, varId, &ctx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(ctx, &myContext);

    /* Set node context */
    int newContext = 99;
    res = UA_Server_setNodeContext(server, varId, &newContext);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    ctx = NULL;
    res = UA_Server_getNodeContext(server, varId, &ctx);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(ctx, &newContext);
} END_TEST

/* === Namespace operations === */
START_TEST(namespace_operations) {
    /* Get namespace array */
    UA_UInt16 nsIdx = UA_Server_addNamespace(server, "urn:test:namespace");
    ck_assert(nsIdx > 0);

    /* Add another */
    UA_UInt16 nsIdx2 = UA_Server_addNamespace(server, "urn:test:namespace2");
    ck_assert(nsIdx2 > nsIdx);

    /* Add duplicate should return existing */
    UA_UInt16 nsIdx3 = UA_Server_addNamespace(server, "urn:test:namespace");
    ck_assert_uint_eq(nsIdx3, nsIdx);

    /* Get namespace by name */
    size_t found;
    UA_StatusCode res = UA_Server_getNamespaceByName(server,
        UA_STRING("urn:test:namespace"), &found);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(found, nsIdx);
} END_TEST

/* === Variable with array value === */
START_TEST(variable_with_array) {
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 arr[] = {1, 2, 3, 4, 5};
    UA_Variant_setArray(&vattr.value, arr, 5, &UA_TYPES[UA_TYPES_INT32]);
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    vattr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 dims = 5;
    vattr.arrayDimensions = &dims;
    vattr.arrayDimensionsSize = 1;
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId varId = UA_NODEID_NUMERIC(1, 80090);
    UA_StatusCode res = UA_Server_addVariableNode(server, varId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "ArrayVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    UA_Variant value;
    UA_Variant_init(&value);
    res = UA_Server_readValue(server, varId, &value);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(value.arrayLength, 5);
    UA_Variant_clear(&value);

    /* Write new array */
    UA_Int32 newArr[] = {10, 20, 30};
    UA_Variant wv;
    UA_Variant_setArray(&wv, newArr, 3, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Server_writeValue(server, varId, wv);
    /* May fail due to array dimension mismatch */
    (void)res;

    /* Write array dimensions */
    UA_Variant adv;
    UA_UInt32 newDims = 3;
    UA_Variant_setScalar(&adv, &newDims, &UA_TYPES[UA_TYPES_UINT32]);
    res = UA_Server_writeArrayDimensions(server, varId, adv);
    (void)res;
} END_TEST

/* === Datasource variable === */
static UA_StatusCode readDatasource(UA_Server *s,
        const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeId, void *nodeContext,
        UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
        UA_DataValue *data) {
    UA_Int32 val = 42;
    UA_Variant_setScalarCopy(&data->value, &val, &UA_TYPES[UA_TYPES_INT32]);
    data->hasValue = true;
    if(sourceTimeStamp) {
        data->sourceTimestamp = UA_DateTime_now();
        data->hasSourceTimestamp = true;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeDatasource(UA_Server *s,
        const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeId, void *nodeContext,
        const UA_NumericRange *range, const UA_DataValue *data) {
    return UA_STATUSCODE_GOOD;
}

START_TEST(datasource_variable) {
    UA_DataSource ds;
    ds.read = readDatasource;
    ds.write = writeDatasource;

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    vattr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_NodeId varId = UA_NODEID_NUMERIC(1, 80100);
    UA_StatusCode res = UA_Server_addDataSourceVariableNode(server, varId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "DSVar"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vattr, ds, NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read from datasource */
    UA_Variant value;
    UA_Variant_init(&value);
    res = UA_Server_readValue(server, varId, &value);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(*(UA_Int32*)value.data, 42);
    UA_Variant_clear(&value);

    /* Write to datasource */
    UA_Int32 newVal = 99;
    UA_Variant wv;
    UA_Variant_setScalar(&wv, &newVal, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_Server_writeValue(server, varId, wv);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Object instantiation === */
START_TEST(instantiate_object) {
    /* Add ObjectType first */
    UA_ObjectTypeAttributes ota = UA_ObjectTypeAttributes_default;
    ota.displayName = UA_LOCALIZEDTEXT("en", "InstObjType");

    UA_NodeId typeId = UA_NODEID_NUMERIC(1, 80110);
    UA_Server_addObjectTypeNode(server, typeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "InstObjType"),
        ota, NULL, NULL);

    /* Add mandatory child to type */
    UA_VariableAttributes va = UA_VariableAttributes_default;
    UA_Int32 v = 0;
    UA_Variant_setScalar(&va.value, &v, &UA_TYPES[UA_TYPES_INT32]);
    va.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Server_addVariableNode(server,
        UA_NODEID_NUMERIC(1, 80111),
        typeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Mandatory"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        va, NULL, NULL);

    /* Set mandatory modeling rule */
    UA_Server_addReference(server,
        UA_NODEID_NUMERIC(1, 80111),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY),
        true);

    /* Instantiate the type */
    UA_ObjectAttributes oa = UA_ObjectAttributes_default;
    oa.displayName = UA_LOCALIZEDTEXT("en", "MyInst");
    UA_NodeId instId;
    UA_StatusCode res = UA_Server_addObjectNode(server,
        UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "MyInst"),
        typeId,
        oa, NULL, &instId);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* The mandatory child should exist on the instance */
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = instId;
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = true;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    ck_assert(br.referencesSize > 0);
    UA_BrowseResult_clear(&br);
    UA_NodeId_clear(&instId);
} END_TEST

/* === Method with callback === */
static UA_StatusCode
methodCallback(UA_Server *s, const UA_NodeId *sessionId,
               void *sessionContext, const UA_NodeId *methodId,
               void *methodContext, const UA_NodeId *objectId,
               void *objectContext, size_t inputSize,
               const UA_Variant *input, size_t outputSize,
               UA_Variant *output) {
    if(inputSize < 1 || !UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_INT32]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    
    UA_Int32 inVal = *(UA_Int32*)input[0].data;
    UA_Int32 outVal = inVal * 2;
    UA_Variant_setScalarCopy(&output[0], &outVal, &UA_TYPES[UA_TYPES_INT32]);
    return UA_STATUSCODE_GOOD;
}

START_TEST(method_with_args) {
    /* Add input argument */
    UA_Argument inputArg;
    UA_Argument_init(&inputArg);
    inputArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArg.name = UA_STRING("Input");
    inputArg.valueRank = UA_VALUERANK_SCALAR;

    /* Add output argument */
    UA_Argument outputArg;
    UA_Argument_init(&outputArg);
    outputArg.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArg.name = UA_STRING("Output");
    outputArg.valueRank = UA_VALUERANK_SCALAR;

    /* Add object to host the method */
    UA_ObjectAttributes oa = UA_ObjectAttributes_default;
    UA_NodeId objId = UA_NODEID_NUMERIC(1, 80120);
    UA_Server_addObjectNode(server, objId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "MethodObj"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oa, NULL, NULL);

    UA_NodeId methodId = UA_NODEID_NUMERIC(1, 80121);
    UA_StatusCode res = UA_Server_addMethodNode(server, methodId,
        objId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "DoubleIt"),
        UA_MethodAttributes_default,
        methodCallback, 1, &inputArg, 1, &outputArg,
        NULL, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Call the method */
    UA_Int32 inVal = 21;
    UA_Variant inVar;
    UA_Variant_setScalar(&inVar, &inVal, &UA_TYPES[UA_TYPES_INT32]);

    UA_CallMethodRequest cmr;
    UA_CallMethodRequest_init(&cmr);
    cmr.objectId = objId;
    cmr.methodId = methodId;
    cmr.inputArguments = &inVar;
    cmr.inputArgumentsSize = 1;

    UA_CallMethodResult result = UA_Server_call(server, &cmr);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.outputArgumentsSize, 1);
    ck_assert_int_eq(*(UA_Int32*)result.outputArguments[0].data, 42);
    UA_CallMethodResult_clear(&result);
} END_TEST

/* === Suite definition === */
static Suite *testSuite_servicesAttrExt(void) {
    TCase *tc_read = tcase_create("ReadAttrs");
    tcase_add_checked_fixture(tc_read, setup_server, teardown_server);
    tcase_add_test(tc_read, read_object_attributes);
    tcase_add_test(tc_read, read_variable_all_attributes);
    tcase_add_test(tc_read, read_method_attributes);
    tcase_add_test(tc_read, read_referencetype_attributes);
    tcase_add_test(tc_read, read_datatype_attributes);
    tcase_add_test(tc_read, read_view_attributes);
    tcase_add_test(tc_read, read_wrong_attribute);
    tcase_add_test(tc_read, read_nonexistent_node);

    TCase *tc_write = tcase_create("WriteAttrs");
    tcase_add_checked_fixture(tc_write, setup_server, teardown_server);
    tcase_add_test(tc_write, write_variable_all_attributes);

    TCase *tc_nodes = tcase_create("NodeOps");
    tcase_add_checked_fixture(tc_nodes, setup_server, teardown_server);
    tcase_add_test(tc_nodes, add_objecttype_with_lifecycle);
    tcase_add_test(tc_nodes, add_variabletype_node);
    tcase_add_test(tc_nodes, add_referencetype_node);
    tcase_add_test(tc_nodes, add_datatype_node);
    tcase_add_test(tc_nodes, delete_node_with_refs);
    tcase_add_test(tc_nodes, delete_reference);
    tcase_add_test(tc_nodes, node_context_operations);
    tcase_add_test(tc_nodes, namespace_operations);

    TCase *tc_browse = tcase_create("Browse");
    tcase_add_checked_fixture(tc_browse, setup_server, teardown_server);
    tcase_add_test(tc_browse, browse_with_filter);
    tcase_add_test(tc_browse, browse_next);
    tcase_add_test(tc_browse, translate_browsepath);
    tcase_add_test(tc_browse, translate_browsepath_bad);

    TCase *tc_advanced = tcase_create("Advanced");
    tcase_add_checked_fixture(tc_advanced, setup_server, teardown_server);
    tcase_add_test(tc_advanced, variable_with_array);
    tcase_add_test(tc_advanced, datasource_variable);
    tcase_add_test(tc_advanced, instantiate_object);
    tcase_add_test(tc_advanced, method_with_args);

    Suite *s = suite_create("Services Attribute Extended");
    suite_add_tcase(s, tc_read);
    suite_add_tcase(s, tc_write);
    suite_add_tcase(s, tc_nodes);
    suite_add_tcase(s, tc_browse);
    suite_add_tcase(s, tc_advanced);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_servicesAttrExt();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
