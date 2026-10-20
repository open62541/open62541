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

/* --- NodePointer tests --- */

START_TEST(nodePointerFromNumericNodeId) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, 85); /* ObjectsFolder */
    UA_NodePointer np = UA_NodePointer_fromNodeId(&id);
    UA_NodeId back = UA_NodePointer_toNodeId(np);
    ck_assert(UA_NodeId_equal(&id, &back));
    ck_assert(UA_NodePointer_isLocal(np));
    UA_NodePointer_clear(&np);
} END_TEST

START_TEST(nodePointerFromStringNodeId) {
    UA_NodeId id = UA_NODEID_STRING_ALLOC(1, "test.node.pointer");
    UA_NodePointer np = UA_NodePointer_fromNodeId(&id);
    UA_NodeId back = UA_NodePointer_toNodeId(np);
    ck_assert(UA_NodeId_equal(&id, &back));
    ck_assert(UA_NodePointer_isLocal(np));
    UA_NodeId_clear(&id);
    /* np references the original id memory, we've cleared it */
} END_TEST

START_TEST(nodePointerCopyNumeric) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, 100);
    UA_NodePointer np = UA_NodePointer_fromNodeId(&id);
    UA_NodePointer copy;
    UA_NodePointer_init(&copy);
    UA_StatusCode res = UA_NodePointer_copy(np, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NodeId nid1 = UA_NodePointer_toNodeId(np);
    UA_NodeId nid2 = UA_NodePointer_toNodeId(copy);
    ck_assert(UA_NodeId_equal(&nid1, &nid2));

    UA_NodePointer_clear(&copy);
} END_TEST

START_TEST(nodePointerCopyString) {
    UA_NodeId id = UA_NODEID_STRING_ALLOC(2, "copytest");
    UA_NodePointer np = UA_NodePointer_fromNodeId(&id);
    UA_NodePointer copy;
    UA_NodePointer_init(&copy);
    UA_StatusCode res = UA_NodePointer_copy(np, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    ck_assert(UA_NodePointer_isLocal(copy));

    UA_NodeId nid2 = UA_NodePointer_toNodeId(copy);
    ck_assert(UA_NodeId_equal(&id, &nid2));

    UA_NodePointer_clear(&copy);
    UA_NodeId_clear(&id);
} END_TEST

START_TEST(nodePointerOrder) {
    UA_NodeId id1 = UA_NODEID_NUMERIC(0, 10);
    UA_NodeId id2 = UA_NODEID_NUMERIC(0, 20);
    UA_NodePointer np1 = UA_NodePointer_fromNodeId(&id1);
    UA_NodePointer np2 = UA_NodePointer_fromNodeId(&id2);

    UA_Order order = UA_NodePointer_order(np1, np2);
    ck_assert_int_eq(order, UA_ORDER_LESS);

    order = UA_NodePointer_order(np2, np1);
    ck_assert_int_eq(order, UA_ORDER_MORE);

    order = UA_NodePointer_order(np1, np1);
    ck_assert_int_eq(order, UA_ORDER_EQ);
} END_TEST

START_TEST(nodePointerOrderString) {
    UA_NodeId id1 = UA_NODEID_STRING_ALLOC(1, "aaa");
    UA_NodeId id2 = UA_NODEID_STRING_ALLOC(1, "bbb");
    UA_NodePointer np1 = UA_NodePointer_fromNodeId(&id1);
    UA_NodePointer np2 = UA_NodePointer_fromNodeId(&id2);

    UA_Order order = UA_NodePointer_order(np1, np2);
    ck_assert_int_eq(order, UA_ORDER_LESS);

    UA_NodeId_clear(&id1);
    UA_NodeId_clear(&id2);
} END_TEST

/* --- Node read operations via UA_Server --- */

START_TEST(readObjectNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);
} END_TEST

START_TEST(readVariableNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_VARIABLE);
} END_TEST

START_TEST(readObjectTypeNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECTTYPE);

    /* Read IsAbstract */
    UA_Boolean isAbstract;
    retval = UA_Server_readIsAbstract(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), &isAbstract);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(readDataTypeNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_DATATYPE);
} END_TEST

START_TEST(readReferenceTypeNode) {
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_REFERENCETYPE);

    /* Read InverseName */
    UA_LocalizedText inverseName;
    retval = UA_Server_readInverseName(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), &inverseName);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_LocalizedText_clear(&inverseName);
} END_TEST

START_TEST(readViewNode) {
    /* The default namespace has Views folder but might not have actual views.
     * Just read the ViewsFolder node class. */
    UA_NodeClass nc;
    UA_StatusCode retval = UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER), &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECT);
} END_TEST

/* --- Node add/copy operations via server API --- */

START_TEST(addVariableNodeCheckAttributes) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Double val = 3.14;
    UA_Variant_setScalar(&attr.value, &val, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "MyDouble");
    attr.description = UA_LOCALIZEDTEXT("en-US", "A double value");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    attr.historizing = false;

    UA_NodeId myId = UA_NODEID_STRING(1, "nodes.test.double");
    UA_StatusCode retval = UA_Server_addVariableNode(server, myId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "MyDouble"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        attr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read back all attributes */
    UA_Variant readVal;
    retval = UA_Server_readValue(server, myId, &readVal);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&readVal, &UA_TYPES[UA_TYPES_DOUBLE]));
    ck_assert((*(UA_Double*)readVal.data) - 3.14 < 1e-10 &&
              (*(UA_Double*)readVal.data) - 3.14 > -1e-10);
    UA_Variant_clear(&readVal);

    UA_LocalizedText dn;
    retval = UA_Server_readDisplayName(server, myId, &dn);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_String expectedDN = UA_STRING("MyDouble");
    ck_assert(UA_String_equal(&dn.text, &expectedDN));
    UA_LocalizedText_clear(&dn);

    UA_Byte accessLevel;
    retval = UA_Server_readAccessLevel(server, myId, &accessLevel);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(accessLevel & UA_ACCESSLEVELMASK_READ);
    ck_assert(accessLevel & UA_ACCESSLEVELMASK_WRITE);

    UA_Boolean historizing;
    retval = UA_Server_readHistorizing(server, myId, &historizing);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(historizing == false);
} END_TEST

START_TEST(addObjectTypeNode) {
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestObjectType");
    otAttr.isAbstract = false;

    UA_NodeId otId = UA_NODEID_STRING(1, "nodes.test.ot");
    UA_StatusCode retval = UA_Server_addObjectTypeNode(server, otId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestObjectType"),
        otAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    /* Read NodeClass */
    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, otId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_OBJECTTYPE);
} END_TEST

START_TEST(addVariableTypeNode) {
    UA_VariableTypeAttributes vtAttr = UA_VariableTypeAttributes_default;
    vtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestVariableType");
    vtAttr.isAbstract = false;
    vtAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    vtAttr.valueRank = UA_VALUERANK_SCALAR;

    UA_NodeId vtId = UA_NODEID_STRING(1, "nodes.test.vt");
    UA_StatusCode retval = UA_Server_addVariableTypeNode(server, vtId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestVariableType"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        vtAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, vtId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_VARIABLETYPE);
} END_TEST

START_TEST(addReferenceTypeNode) {
    UA_ReferenceTypeAttributes rtAttr = UA_ReferenceTypeAttributes_default;
    rtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "TestRefType");
    rtAttr.isAbstract = false;
    rtAttr.symmetric = false;
    rtAttr.inverseName = UA_LOCALIZEDTEXT("en-US", "IsTestedBy");

    UA_NodeId rtId = UA_NODEID_STRING(1, "nodes.test.rt");
    UA_StatusCode retval = UA_Server_addReferenceTypeNode(server, rtId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
        UA_QUALIFIEDNAME(1, "TestRefType"),
        rtAttr, NULL, NULL);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);

    UA_NodeClass nc;
    retval = UA_Server_readNodeClass(server, rtId, &nc);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nc, UA_NODECLASS_REFERENCETYPE);
} END_TEST

/* --- NS0 ReferenceTypes and their ReferenceTypeIndex --- */

/* Fetch a bootstrapped ReferenceTypeNode from the nodestore. The caller has to
 * release the node again. */
static const UA_ReferenceTypeNode *
getReferenceType(UA_UInt32 identifier) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, identifier);
    const UA_Node *node = UA_NODESTORE_GET(server, &id);
    ck_assert(node != NULL);
    ck_assert_uint_eq(node->head.nodeClass, UA_NODECLASS_REFERENCETYPE);
    return &node->referenceTypeNode;
}

static UA_Byte
referenceTypeIndexOf(UA_UInt32 identifier) {
    const UA_ReferenceTypeNode *rt = getReferenceType(identifier);
    UA_Byte index = rt->referenceTypeIndex;
    UA_NODESTORE_RELEASE(server, (const UA_Node*)rt);
    return index;
}

/* createNS0_base assigns the ReferenceTypeIndex in order of creation. The
 * UA_REFERENCETYPEINDEX_* defines hardcode that order, so the two have to stay
 * in sync. Note that the NodeIds are not in ascending order -- only the
 * bootstrap sequence matters. */
START_TEST(fixedReferenceTypeIndices) {
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_REFERENCES),
                      UA_REFERENCETYPEINDEX_REFERENCES);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASSUBTYPE),
                      UA_REFERENCETYPEINDEX_HASSUBTYPE);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_AGGREGATES),
                      UA_REFERENCETYPEINDEX_AGGREGATES);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HIERARCHICALREFERENCES),
                      UA_REFERENCETYPEINDEX_HIERARCHICALREFERENCES);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_NONHIERARCHICALREFERENCES),
                      UA_REFERENCETYPEINDEX_NONHIERARCHICALREFERENCES);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASCHILD),
                      UA_REFERENCETYPEINDEX_HASCHILD);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_ORGANIZES),
                      UA_REFERENCETYPEINDEX_ORGANIZES);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASEVENTSOURCE),
                      UA_REFERENCETYPEINDEX_HASEVENTSOURCE);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASMODELLINGRULE),
                      UA_REFERENCETYPEINDEX_HASMODELLINGRULE);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASENCODING),
                      UA_REFERENCETYPEINDEX_HASENCODING);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASDESCRIPTION),
                      UA_REFERENCETYPEINDEX_HASDESCRIPTION);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASTYPEDEFINITION),
                      UA_REFERENCETYPEINDEX_HASTYPEDEFINITION);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_GENERATESEVENT),
                      UA_REFERENCETYPEINDEX_GENERATESEVENT);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASPROPERTY),
                      UA_REFERENCETYPEINDEX_HASPROPERTY);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASCOMPONENT),
                      UA_REFERENCETYPEINDEX_HASCOMPONENT);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASNOTIFIER),
                      UA_REFERENCETYPEINDEX_HASNOTIFIER);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASORDEREDCOMPONENT),
                      UA_REFERENCETYPEINDEX_HASORDEREDCOMPONENT);
    ck_assert_uint_eq(referenceTypeIndexOf(UA_NS0ID_HASINTERFACE),
                      UA_REFERENCETYPEINDEX_HASINTERFACE);
} END_TEST

#ifdef UA_GENERATED_NAMESPACE_ZERO

/* ReferenceTypes created after the bootstrap must not take an index that a
 * UA_REFERENCETYPEINDEX_* define claims. */
START_TEST(additionalReferenceTypeIndices) {
    ck_assert_uint_gt(referenceTypeIndexOf(UA_NS0ID_HASARGUMENTDESCRIPTION),
                      UA_REFERENCETYPEINDEX_HASINTERFACE);
    ck_assert_uint_gt(referenceTypeIndexOf(UA_NS0ID_HASOPTIONALINPUTARGUMENTDESCRIPTION),
                      UA_REFERENCETYPEINDEX_HASINTERFACE);
    ck_assert_uint_ne(referenceTypeIndexOf(UA_NS0ID_HASARGUMENTDESCRIPTION),
                      referenceTypeIndexOf(UA_NS0ID_HASOPTIONALINPUTARGUMENTDESCRIPTION));
} END_TEST

/* HasArgumentDescription (i=129) and its subtype HasOptionalInputArgumentDescription
 * (i=131) are defined in OPC 10000-3, 5.7.2 - 5.7.3. They come from the generated
 * nodeset, so they are not available for UA_NAMESPACE_ZERO=MINIMAL. */
START_TEST(argumentDescriptionAttributes) {
    UA_QualifiedName bn = UA_QUALIFIEDNAME(0, "HasArgumentDescription");
    UA_String inverse = UA_STRING("ArgumentDescriptionOf");
    const UA_ReferenceTypeNode *rt = getReferenceType(UA_NS0ID_HASARGUMENTDESCRIPTION);
    ck_assert(UA_QualifiedName_equal(&rt->head.browseName, &bn));
    ck_assert(UA_String_equal(&rt->inverseName.text, &inverse));
    ck_assert_uint_eq(rt->isAbstract, false);
    ck_assert_uint_eq(rt->symmetric, false);
    UA_NODESTORE_RELEASE(server, (const UA_Node*)rt);

    bn = UA_QUALIFIEDNAME(0, "HasOptionalInputArgumentDescription");
    inverse = UA_STRING("OptionalInputArgumentDescriptionOf");
    rt = getReferenceType(UA_NS0ID_HASOPTIONALINPUTARGUMENTDESCRIPTION);
    ck_assert(UA_QualifiedName_equal(&rt->head.browseName, &bn));
    ck_assert(UA_String_equal(&rt->inverseName.text, &inverse));
    ck_assert_uint_eq(rt->isAbstract, false);
    ck_assert_uint_eq(rt->symmetric, false);
    UA_NODESTORE_RELEASE(server, (const UA_Node*)rt);
} END_TEST

/* The subtype hierarchy from OPC 10000-3: HasArgumentDescription is a subtype of
 * HasComponent, HasOptionalInputArgumentDescription a subtype of
 * HasArgumentDescription. The subtype sets are what the Browse service uses. */
START_TEST(argumentDescriptionSubtypeHierarchy) {
    UA_Byte argIdx = referenceTypeIndexOf(UA_NS0ID_HASARGUMENTDESCRIPTION);
    UA_Byte optIdx = referenceTypeIndexOf(UA_NS0ID_HASOPTIONALINPUTARGUMENTDESCRIPTION);

    const UA_ReferenceTypeNode *hasComponent = getReferenceType(UA_NS0ID_HASCOMPONENT);
    ck_assert(UA_ReferenceTypeSet_contains(&hasComponent->subTypes, argIdx));
    ck_assert(UA_ReferenceTypeSet_contains(&hasComponent->subTypes, optIdx));
    UA_NODESTORE_RELEASE(server, (const UA_Node*)hasComponent);

    const UA_ReferenceTypeNode *hasArgument =
        getReferenceType(UA_NS0ID_HASARGUMENTDESCRIPTION);
    ck_assert(UA_ReferenceTypeSet_contains(&hasArgument->subTypes, optIdx));
    UA_NODESTORE_RELEASE(server, (const UA_Node*)hasArgument);

    /* Unrelated branch of the hierarchy */
    const UA_ReferenceTypeNode *hasInterface = getReferenceType(UA_NS0ID_HASINTERFACE);
    ck_assert(!UA_ReferenceTypeSet_contains(&hasInterface->subTypes, argIdx));
    ck_assert(!UA_ReferenceTypeSet_contains(&hasInterface->subTypes, optIdx));
    UA_NODESTORE_RELEASE(server, (const UA_Node*)hasInterface);
} END_TEST

#endif /* UA_GENERATED_NAMESPACE_ZERO */

#if defined(UA_GENERATED_NAMESPACE_ZERO) && defined(UA_ENABLE_LOGOBJECT)

/* --- OPC UA Part 26 LogObject nodes --- */

static void
assertNode(UA_UInt32 identifier, UA_NodeClass expectedClass, char *expectedName) {
    UA_NodeId id = UA_NODEID_NUMERIC(0, identifier);
    UA_NodeClass nc = UA_NODECLASS_UNSPECIFIED;
    UA_StatusCode retval = UA_Server_readNodeClass(server, id, &nc);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "Node i=%u is missing",
                  (unsigned)identifier);
    ck_assert_uint_eq(nc, expectedClass);
    UA_QualifiedName bn;
    retval = UA_Server_readBrowseName(server, id, &bn);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_QualifiedName expected = UA_QUALIFIEDNAME(0, expectedName);
    ck_assert(UA_QualifiedName_equal(&bn, &expected));
    UA_QualifiedName_clear(&bn);
}

/* Browse the references of one type and direction and expect the target */
static void
assertReference(UA_UInt32 source, UA_UInt32 referenceType,
                UA_BrowseDirection direction, UA_UInt32 expectedTarget) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NODEID_NUMERIC(0, source);
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, referenceType);
    bd.includeSubtypes = false;
    bd.browseDirection = direction;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(br.statusCode, UA_STATUSCODE_GOOD);
    UA_NodeId expected = UA_NODEID_NUMERIC(0, expectedTarget);
    UA_Boolean found = false;
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(UA_NodeId_equal(&br.references[i].nodeId.nodeId, &expected))
            found = true;
    }
    ck_assert_msg(found, "Reference i=%u -> i=%u is missing",
                  (unsigned)source, (unsigned)expectedTarget);
    UA_BrowseResult_clear(&br);
}

/* LogObjectType (i=19352), its well-known ServerLog instance (i=19372) and the
 * Logs Folder (i=19378) below Server/Resources are defined in OPC 10000-26,
 * 5.2, 7.2 and 7.3. */
START_TEST(logObjectNodes) {
    assertNode(UA_NS0ID_LOGOBJECTTYPE, UA_NODECLASS_OBJECTTYPE, "LogObjectType");
    assertNode(UA_NS0ID_LOGOBJECTTYPE_GETRECORDS, UA_NODECLASS_METHOD, "GetRecords");
    assertNode(UA_NS0ID_LOGOBJECTTYPE_RELEASECONTINUATIONPOINT, UA_NODECLASS_METHOD,
               "ReleaseContinuationPoint");
    assertNode(UA_NS0ID_LOGOBJECTTYPE_MAXRECORDS, UA_NODECLASS_VARIABLE, "MaxRecords");
    assertNode(UA_NS0ID_LOGOBJECTTYPE_MAXSTORAGEDURATION, UA_NODECLASS_VARIABLE,
               "MaxStorageDuration");
    assertNode(UA_NS0ID_LOGOBJECTTYPE_MINIMUMSEVERITY, UA_NODECLASS_VARIABLE,
               "MinimumSeverity");
    assertNode(UA_NS0ID_SERVERLOG, UA_NODECLASS_OBJECT, "ServerLog");
    assertNode(UA_NS0ID_SERVERLOG_GETRECORDS, UA_NODECLASS_METHOD, "GetRecords");
    assertNode(UA_NS0ID_RESOURCES, UA_NODECLASS_OBJECT, "Resources");
    assertNode(UA_NS0ID_LOGS, UA_NODECLASS_OBJECT, "Logs");
    assertNode(UA_NS0ID_SERVERCAPABILITIESTYPE_MAXLOGOBJECTCONTINUATIONPOINTS,
               UA_NODECLASS_VARIABLE, "MaxLogObjectContinuationPoints");

    /* The type hierarchy and the well-known instances (OPC 10000-26, Figure 7) */
    assertReference(UA_NS0ID_LOGOBJECTTYPE, UA_NS0ID_HASSUBTYPE,
                    UA_BROWSEDIRECTION_INVERSE, UA_NS0ID_BASEOBJECTTYPE);
    assertReference(UA_NS0ID_SERVERLOG, UA_NS0ID_HASTYPEDEFINITION,
                    UA_BROWSEDIRECTION_FORWARD, UA_NS0ID_LOGOBJECTTYPE);
    assertReference(UA_NS0ID_SERVER, UA_NS0ID_HASCOMPONENT,
                    UA_BROWSEDIRECTION_FORWARD, UA_NS0ID_SERVERLOG);
    assertReference(UA_NS0ID_SERVER, UA_NS0ID_HASCOMPONENT,
                    UA_BROWSEDIRECTION_FORWARD, UA_NS0ID_RESOURCES);
    assertReference(UA_NS0ID_RESOURCES, UA_NS0ID_ORGANIZES,
                    UA_BROWSEDIRECTION_FORWARD, UA_NS0ID_LOGS);
    assertReference(UA_NS0ID_LOGS, UA_NS0ID_HASTYPEDEFINITION,
                    UA_BROWSEDIRECTION_FORWARD, UA_NS0ID_FOLDERTYPE);
} END_TEST

/* The DataTypes of OPC 10000-26, 5.5 - 5.10 and the matching entries of the
 * generated type array */
START_TEST(logObjectDataTypeNodes) {
    assertNode(UA_NS0ID_LOGRECORD, UA_NODECLASS_DATATYPE, "LogRecord");
    assertNode(UA_NS0ID_LOGRECORDSDATATYPE, UA_NODECLASS_DATATYPE, "LogRecordsDataType");
    assertNode(UA_NS0ID_SPANCONTEXTDATATYPE, UA_NODECLASS_DATATYPE, "SpanContextDataType");
    assertNode(UA_NS0ID_TRACECONTEXTDATATYPE, UA_NODECLASS_DATATYPE, "TraceContextDataType");
    assertNode(UA_NS0ID_NAMEVALUEPAIR, UA_NODECLASS_DATATYPE, "NameValuePair");
    assertNode(UA_NS0ID_LOGRECORDMASK, UA_NODECLASS_DATATYPE, "LogRecordMask");
    assertNode(UA_NS0ID_LOGRECORDMASK_OPTIONSETVALUES, UA_NODECLASS_VARIABLE,
               "OptionSetValues");

    assertReference(UA_NS0ID_LOGRECORD, UA_NS0ID_HASSUBTYPE,
                    UA_BROWSEDIRECTION_INVERSE, UA_NS0ID_STRUCTURE);
    assertReference(UA_NS0ID_TRACECONTEXTDATATYPE, UA_NS0ID_HASSUBTYPE,
                    UA_BROWSEDIRECTION_INVERSE, UA_NS0ID_SPANCONTEXTDATATYPE);
    assertReference(UA_NS0ID_LOGRECORDMASK, UA_NS0ID_HASSUBTYPE,
                    UA_BROWSEDIRECTION_INVERSE, UA_NS0ID_UINT32);

    UA_NodeId logRecordId = UA_NS0ID(LOGRECORD);
    UA_NodeId binaryEncodingId = UA_NS0ID(LOGRECORD_ENCODING_DEFAULTBINARY);
    ck_assert(UA_NodeId_equal(&UA_TYPES[UA_TYPES_LOGRECORD].typeId, &logRecordId));
    ck_assert(UA_NodeId_equal(&UA_TYPES[UA_TYPES_LOGRECORD].binaryEncodingId,
                              &binaryEncodingId));
    ck_assert_uint_eq(UA_TYPES[UA_TYPES_LOGRECORD].typeKind, UA_DATATYPEKIND_OPTSTRUCT);
} END_TEST

#ifdef UA_ENABLE_TYPEDESCRIPTION
/* The DataTypeDefinition attribute of LogRecord (i=19361) reflects the five
 * optional fields of OPC 10000-26, Table 8 */
START_TEST(logRecordDataTypeDefinition) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NS0ID(LOGRECORD);
    rvi.attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;
    UA_DataValue dv = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_uint_eq(dv.status, UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&dv.value, &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]));
    UA_StructureDefinition *sd = (UA_StructureDefinition*)dv.value.data;
    ck_assert_uint_eq(sd->structureType, UA_STRUCTURETYPE_STRUCTUREWITHOPTIONALFIELDS);
    ck_assert_uint_eq(sd->fieldsSize, 8);
    const UA_Boolean optional[8] = {false, false, true, true, true, false, true, true};
    for(size_t i = 0; i < 8; i++)
        ck_assert_uint_eq(sd->fields[i].isOptional, optional[i]);
    UA_NodeId encodingId = UA_NS0ID(LOGRECORD_ENCODING_DEFAULTBINARY);
    ck_assert(UA_NodeId_equal(&sd->defaultEncodingId, &encodingId));
    UA_DataValue_clear(&dv);
} END_TEST
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
/* The EventTypes and the ConditionClass of OPC 10000-26, 6.3 - 6.5 */
START_TEST(logObjectEventTypeNodes) {
    assertNode(UA_NS0ID_BASECONDITIONCLASSTYPE, UA_NODECLASS_OBJECTTYPE,
               "BaseConditionClassType");
    assertNode(UA_NS0ID_LOGENTRYCONDITIONCLASSTYPE, UA_NODECLASS_OBJECTTYPE,
               "LogEntryConditionClassType");
    assertNode(UA_NS0ID_BASELOGEVENTTYPE, UA_NODECLASS_OBJECTTYPE, "BaseLogEventType");
    assertNode(UA_NS0ID_BASELOGEVENTTYPE_CONDITIONCLASSID, UA_NODECLASS_VARIABLE,
               "ConditionClassId");
    assertNode(UA_NS0ID_BASELOGEVENTTYPE_CONDITIONCLASSNAME, UA_NODECLASS_VARIABLE,
               "ConditionClassName");
    assertNode(UA_NS0ID_BASELOGEVENTTYPE_ERRORCODE, UA_NODECLASS_VARIABLE, "ErrorCode");
    assertNode(UA_NS0ID_BASELOGEVENTTYPE_ERRORCODENODE, UA_NODECLASS_VARIABLE,
               "ErrorCodeNode");
    assertNode(UA_NS0ID_BASELOGEVENTTYPE_TRACECONTEXT, UA_NODECLASS_VARIABLE,
               "TraceContext");
    assertNode(UA_NS0ID_LOGOVERFLOWEVENTTYPE, UA_NODECLASS_OBJECTTYPE,
               "LogOverflowEventType");

    assertReference(UA_NS0ID_BASELOGEVENTTYPE, UA_NS0ID_HASSUBTYPE,
                    UA_BROWSEDIRECTION_INVERSE, UA_NS0ID_BASEEVENTTYPE);
    assertReference(UA_NS0ID_LOGOVERFLOWEVENTTYPE, UA_NS0ID_HASSUBTYPE,
                    UA_BROWSEDIRECTION_INVERSE, UA_NS0ID_BASEEVENTTYPE);
    assertReference(UA_NS0ID_LOGENTRYCONDITIONCLASSTYPE, UA_NS0ID_HASSUBTYPE,
                    UA_BROWSEDIRECTION_INVERSE, UA_NS0ID_BASECONDITIONCLASSTYPE);

    /* All three types are abstract in the standard nodeset */
    const UA_UInt32 abstractTypes[3] = {UA_NS0ID_BASELOGEVENTTYPE,
                                        UA_NS0ID_LOGOVERFLOWEVENTTYPE,
                                        UA_NS0ID_LOGENTRYCONDITIONCLASSTYPE};
    for(size_t i = 0; i < 3; i++) {
        UA_Boolean isAbstract = false;
        UA_StatusCode retval =
            UA_Server_readIsAbstract(server, UA_NODEID_NUMERIC(0, abstractTypes[i]),
                                     &isAbstract);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
        ck_assert(isAbstract);
    }
} END_TEST
#endif

#endif /* UA_GENERATED_NAMESPACE_ZERO && UA_ENABLE_LOGOBJECT */

int main(void) {
    Suite *s = suite_create("nodes");

    TCase *tc_np = tcase_create("NodePointer");
    tcase_add_test(tc_np, nodePointerFromNumericNodeId);
    tcase_add_test(tc_np, nodePointerFromStringNodeId);
    tcase_add_test(tc_np, nodePointerCopyNumeric);
    tcase_add_test(tc_np, nodePointerCopyString);
    tcase_add_test(tc_np, nodePointerOrder);
    tcase_add_test(tc_np, nodePointerOrderString);
    suite_add_tcase(s, tc_np);

    TCase *tc_read = tcase_create("ReadNodes");
    tcase_add_checked_fixture(tc_read, setup, teardown);
    tcase_add_test(tc_read, readObjectNode);
    tcase_add_test(tc_read, readVariableNode);
    tcase_add_test(tc_read, readObjectTypeNode);
    tcase_add_test(tc_read, readDataTypeNode);
    tcase_add_test(tc_read, readReferenceTypeNode);
    tcase_add_test(tc_read, readViewNode);
    suite_add_tcase(s, tc_read);

    TCase *tc_add = tcase_create("AddNodes");
    tcase_add_checked_fixture(tc_add, setup, teardown);
    tcase_add_test(tc_add, addVariableNodeCheckAttributes);
    tcase_add_test(tc_add, addObjectTypeNode);
    tcase_add_test(tc_add, addVariableTypeNode);
    tcase_add_test(tc_add, addReferenceTypeNode);
    suite_add_tcase(s, tc_add);

    TCase *tc_reftypes = tcase_create("ReferenceTypes");
    tcase_add_checked_fixture(tc_reftypes, setup, teardown);
    tcase_add_test(tc_reftypes, fixedReferenceTypeIndices);
#ifdef UA_GENERATED_NAMESPACE_ZERO
    tcase_add_test(tc_reftypes, additionalReferenceTypeIndices);
    tcase_add_test(tc_reftypes, argumentDescriptionAttributes);
    tcase_add_test(tc_reftypes, argumentDescriptionSubtypeHierarchy);
#endif
    suite_add_tcase(s, tc_reftypes);

#if defined(UA_GENERATED_NAMESPACE_ZERO) && defined(UA_ENABLE_LOGOBJECT)
    TCase *tc_logobject = tcase_create("LogObjectNodes");
    tcase_add_checked_fixture(tc_logobject, setup, teardown);
    tcase_add_test(tc_logobject, logObjectNodes);
    tcase_add_test(tc_logobject, logObjectDataTypeNodes);
#ifdef UA_ENABLE_TYPEDESCRIPTION
    tcase_add_test(tc_logobject, logRecordDataTypeDefinition);
#endif
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    tcase_add_test(tc_logobject, logObjectEventTypeNodes);
#endif
    suite_add_tcase(s, tc_logobject);
#endif

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
