/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static UA_Server *server = NULL;
static void *sessionCalled = (void *)1;
static void *nodeCalled = (void *)2;
static UA_Int32 handleCalled = 0;

static UA_StatusCode
globalInstantiationMethod(UA_Server *server_,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *nodeId, void **nodeContext) {
    sessionCalled = sessionContext;
    nodeCalled = *nodeContext;
    handleCalled++;
    return UA_STATUSCODE_GOOD;
}

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    UA_GlobalNodeLifecycle lifecycle;
    lifecycle.constructor = globalInstantiationMethod;
    lifecycle.destructor = NULL;
    lifecycle.createOptionalChild = NULL;
    lifecycle.generateChildNodeId = NULL;
    config->nodeLifecycle = lifecycle;
    UA_Server_setAdminSessionContext(server, (void *)0x3);
}

static void teardown(void) {
    UA_Server_delete(server);
}

START_TEST(AddVariableNode) {
    /* Add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    ck_assert_ptr_eq(sessionCalled, (void *)1);
    ck_assert_ptr_eq(nodeCalled, (void *)2);
    UA_StatusCode res =
        UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                  parentReferenceNodeId, myIntegerName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, (void *)4, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    ck_assert_ptr_eq(sessionCalled, (void *)3);
    ck_assert_ptr_eq(nodeCalled, (void *)4);
} END_TEST

START_TEST(AddVariableNode_ValueRankZero) {
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Array ValueRank 0");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Set the variable value constraints */
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.valueRank = UA_VALUERANK_ONE_OR_MORE_DIMENSIONS;

    /* Set the value */
    UA_UInt32 arrayDims[1] = {2};
    UA_Double zero[2] = {0.0, 0.0};
    UA_Variant_setArray(&attr.value, zero, 2, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.value.arrayDimensions = arrayDims;
    attr.value.arrayDimensionsSize = 1;

    UA_NodeId myNodeId = UA_NODEID_STRING(1, "array0");
    UA_QualifiedName myName = UA_QUALIFIEDNAME(1, "array0");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res =
        UA_Server_addVariableNode(server, myNodeId, parentNodeId,
                                  parentReferenceNodeId, myName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
} END_TEST

START_TEST(AddVariableNode_EmptyValueWithNonZeroValueRank) {
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    /* VariableNode with zero (unlimited dimensions */
    vattr = UA_VariableAttributes_default;
    UA_Variant_clear(&vattr.value);
    vattr.valueRank = 2;
    UA_UInt32 myIntegerDimensions2[2] = {0, 2};
    vattr.arrayDimensions = myIntegerDimensions2;
    vattr.arrayDimensionsSize = 2;
    vattr.dataType = UA_NODEID_NUMERIC(0, UA_NS0ID_INT32);
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "myarraydims");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "myarraydims");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "myarraydims");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode retval = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                                     parentReferenceNodeId, myIntegerName,
                                                     UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                     vattr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddVariableNode_Matrix) {
    /* Add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Double Matrix");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.valueRank = UA_VALUERANK_TWO_DIMENSIONS;
    UA_UInt32 arrayDims[2] = {2, 2};
    attr.arrayDimensions = arrayDims;
    attr.arrayDimensionsSize = 2;
    UA_Double zero[4] = {0.0, 0.0, 0.0, 0.0};
    UA_Variant_setArray(&attr.value, zero, 4, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.value.arrayDimensions = arrayDims;
    attr.value.arrayDimensionsSize = 2;

    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "double.matrix");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "double matrix");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res =
        UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                  parentReferenceNodeId, myIntegerName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
} END_TEST

START_TEST(AddVariableNode_ExtensionObject) {
        /* Add a variable node to the address space */
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        attr.displayName = UA_LOCALIZEDTEXT("en-US","the extensionobject");

        /* Set an ExtensionObject with an unknown binary encoding */
        UA_ExtensionObject myExtensionObject;
        UA_ExtensionObject_init(&myExtensionObject);
        myExtensionObject.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
        myExtensionObject.content.encoded.typeId = UA_NODEID_NUMERIC(5, 1234);
        UA_ByteString byteString = UA_BYTESTRING("String Payload as a ByteString extension");
        myExtensionObject.content.encoded.body = byteString;
        UA_Variant_setScalar(&attr.value, &myExtensionObject, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);

        UA_NodeId myEONodeId = UA_NODEID_STRING(1, "the.extensionobject");
        UA_QualifiedName myEOName = UA_QUALIFIEDNAME(1, "the extensionobject");
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_StatusCode res =
            UA_Server_addVariableNode(server, myEONodeId, parentNodeId,
                                      parentReferenceNodeId, myEOName,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                      attr, NULL, NULL);
        ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    } END_TEST


static UA_NodeId pointTypeId;

static void
addVariableTypeNode(void) {
    UA_VariableTypeAttributes vtAttr = UA_VariableTypeAttributes_default;
    vtAttr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    vtAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    UA_UInt32 arrayDims[1] = {2};
    vtAttr.arrayDimensions = arrayDims;
    vtAttr.arrayDimensionsSize = 1;
    vtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "2DPoint Type");

    /* a matching default value is required */
    UA_Double zero[2] = {0.0, 0.0};
    UA_Variant_setArray(&vtAttr.value, zero, 2, &UA_TYPES[UA_TYPES_DOUBLE]);

    UA_StatusCode res =
        UA_Server_addVariableTypeNode(server, UA_NODEID_NULL,
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                      UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                      UA_QUALIFIEDNAME(1, "2DPoint Type"), UA_NODEID_NULL,
                                      vtAttr, NULL, &pointTypeId);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
}

START_TEST(InstantiateVariableTypeNode) {
    addVariableTypeNode();

    /* Prepare the node attributes */
    UA_UInt32 arrayDims[1] = {2};
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    vAttr.arrayDimensions = arrayDims;
    vAttr.arrayDimensionsSize = 1;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "2DPoint Variable");
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    /* vAttr.value is left empty, the server instantiates with the default value */

    /* Add the node */
    UA_NodeId pointVariableId;
    UA_StatusCode res =
        UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "2DPoint Type"), pointTypeId,
                                  vAttr, NULL, &pointVariableId);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);

    /* Was the value instantiated? */
    UA_Variant val;
    UA_Server_readValue(server, pointVariableId, &val);
    ck_assert(val.type != NULL);

    UA_Variant_clear(&val);
} END_TEST

START_TEST(InstantiateVariableTypeNodeWrongDims) {
    addVariableTypeNode();

    /* Prepare the node attributes */
    UA_UInt32 arrayDims[1] = {3}; /* This will fail as the dimensions are too big */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    vAttr.arrayDimensions = arrayDims;
    vAttr.arrayDimensionsSize = 1;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "2DPoint Variable");
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    /* vAttr.value is left empty, the server instantiates with the default value */

    /* Add the node */
    UA_StatusCode res =
        UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "2DPoint Type"), pointTypeId,
                                  vAttr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_BADTYPEMISMATCH, res);
} END_TEST

START_TEST(InstantiateVariableTypeNodeLessDims) {
    addVariableTypeNode();

    /* Prepare the node attributes */
    UA_UInt32 arrayDims[1] = {1}; /* This will match as the dimension
                                   * constraints are an upper bound */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    vAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    vAttr.arrayDimensions = arrayDims;
    vAttr.arrayDimensionsSize = 1;
    vAttr.displayName = UA_LOCALIZEDTEXT("en-US", "2DPoint Variable");
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* vAttr.value is left empty, the server tries to instantiate with the
     * default value from the VariableType. This will fail. Then the server
     * tries to auto-generate a matching zero-value of the correct
     * dimensions. */

    /* Add the node */
    UA_StatusCode res =
        UA_Server_addVariableNode(server, UA_NODEID_NULL,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "2DPoint Type"), pointTypeId,
                                  vAttr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
} END_TEST

START_TEST(AddComplexTypeWithInheritance) {
    /* add a variable node to the address space */

    /* Node UA_NS0ID_SERVERTYPE is not available in the minimal NS0 */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US","fakeServerStruct");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","fakeServerStruct");

    UA_NodeId myObjectNodeId = UA_NODEID_STRING(1, "the.fake.Server.Struct");
    UA_QualifiedName myObjectName = UA_QUALIFIEDNAME(1, "the.fake.Server.Struct");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res =
        UA_Server_addObjectNode(server, myObjectNodeId, parentNodeId,
                                parentReferenceNodeId, myObjectName,
                                UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERTYPE), attr,
                                &handleCalled, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    ck_assert_int_gt(handleCalled, 0); // Should be 58, but may depend on NS0 XML detail
#endif
} END_TEST

START_TEST(AddNodeTwiceGivesError) {
    /* add a variable node to the address space */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_StatusCode res =
        UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                  parentReferenceNodeId, myIntegerName,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  attr, NULL, NULL);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, res);
    res = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                    parentReferenceNodeId, myIntegerName,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNODEIDEXISTS);
} END_TEST

static UA_Boolean constructorCalled = false;

static UA_StatusCode
objectConstructor(UA_Server *server_,
                  const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *typeId, void *typeContext,
                  const UA_NodeId *nodeId, void **nodeContext) {
    constructorCalled = true;
    return UA_STATUSCODE_GOOD;
}

START_TEST(AddObjectWithConstructor) {
    /* Add an object type */
    UA_NodeId objecttypeid = UA_NODEID_NUMERIC(0, 13371337);
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US","my objecttype");
    UA_StatusCode res =
        UA_Server_addObjectTypeNode(server, objecttypeid,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                    UA_QUALIFIEDNAME(0, "myobjecttype"), attr,
                                    NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Add a constructor to the object type */
    UA_NodeTypeLifecycle lifecycle;
    lifecycle.constructor = objectConstructor;
    lifecycle.destructor = NULL;
    res = UA_Server_setNodeTypeLifecycle(server, objecttypeid, lifecycle);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Add an object of the type */
    UA_ObjectAttributes attr2 = UA_ObjectAttributes_default;
    attr2.displayName = UA_LOCALIZEDTEXT("en-US","my object");
    res = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(0, "MyObjectNode"), objecttypeid,
                                  attr2, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Verify that the constructor was called */
    ck_assert_int_eq(constructorCalled, true);
} END_TEST

static UA_Boolean destructorCalled = false;

static void
objectDestructor(UA_Server *server_,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *typeId, void *typeContext,
                 const UA_NodeId *nodeId, void **nodeContext) {
    destructorCalled = true;
}

START_TEST(DeleteObjectWithDestructor) {
    /* Add an object type */
    UA_NodeId objecttypeid = UA_NODEID_NUMERIC(0, 13371337);
    UA_ObjectTypeAttributes attr = UA_ObjectTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US","my objecttype");
    UA_StatusCode res =
        UA_Server_addObjectTypeNode(server, objecttypeid,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                    UA_QUALIFIEDNAME(0, "myobjecttype"), attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Add a constructor to the object type */
    UA_NodeTypeLifecycle lifecycle;
    lifecycle.constructor = NULL;
    lifecycle.destructor = objectDestructor;
    res = UA_Server_setNodeTypeLifecycle(server, objecttypeid, lifecycle);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Add an object of the type */
    UA_NodeId objectid = UA_NODEID_NUMERIC(0, 23372337);
    UA_ObjectAttributes attr2 = UA_ObjectAttributes_default;
    attr2.displayName = UA_LOCALIZEDTEXT("en-US","my object");
    res = UA_Server_addObjectNode(server, objectid,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(0, "MyObject"), objecttypeid,
                                  attr2, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Delete the object */
    UA_Server_deleteNode(server, objectid, true);

    /* Verify that the destructor was called */
    ck_assert_int_eq(destructorCalled, true);
} END_TEST

START_TEST(DeleteObjectAndReferences) {
    /* Add an object of the type */
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US","my object");
    UA_NodeId objectid = UA_NODEID_NUMERIC(0, 23372337);
    UA_StatusCode res;
    res = UA_Server_addObjectNode(server, objectid,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(0, "MyObject"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                  attr, NULL, NULL);
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
    ck_assert_uint_eq(refCount, 1);
    UA_BrowseResult_clear(&br);

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
    ck_assert_uint_eq(refCount, 0);
    UA_BrowseResult_clear(&br);

    /* Add an object the second time */
    attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US","my object");
    objectid = UA_NODEID_NUMERIC(0, 23372337);
    res = UA_Server_addObjectNode(server, objectid,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(0, "MyObject"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                  attr, NULL, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Browse again, this time we expect that a single reference to the node is found */
    refCount = 0;
    br = UA_Server_browse(server, 0, &bd);
    ck_assert_int_eq(br.statusCode, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < br.referencesSize; ++i) {
        if(UA_NodeId_equal(&br.references[i].nodeId.nodeId, &objectid))
            refCount++;
    }
    ck_assert_uint_eq(refCount, 1);
    UA_BrowseResult_clear(&br);
} END_TEST


/* Example taken from tutorial_server_object.c */
START_TEST(InstantiateObjectType) {
    /* Define the object type */
    UA_NodeId pumpTypeId = {1, UA_NODEIDTYPE_NUMERIC, {1001}};

    UA_StatusCode retval;

    /* Define the object type for "Device" */
    UA_NodeId deviceTypeId; /* get the nodeid assigned by the server */
    UA_ObjectTypeAttributes dtAttr = UA_ObjectTypeAttributes_default;
    dtAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DeviceType");
    retval = UA_Server_addObjectTypeNode(server, UA_NODEID_NULL,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "DeviceType"), dtAttr,
                                         NULL, &deviceTypeId);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_VariableAttributes mnAttr = UA_VariableAttributes_default;
    mnAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ManufacturerName");
    UA_NodeId manufacturerNameId;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "ManufacturerName"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       mnAttr, NULL, &manufacturerNameId);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* UA_NS0ID_MODELLINGRULE_MANDATORY is not available in Minimal Nodeset */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* Make the manufacturer name mandatory */
    retval = UA_Server_addReference(server, manufacturerNameId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
#endif

    UA_VariableAttributes modelAttr = UA_VariableAttributes_default;
    modelAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ModelName");
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, deviceTypeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "ModelName"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       modelAttr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Define the object type for "Pump" */
    UA_ObjectTypeAttributes ptAttr = UA_ObjectTypeAttributes_default;
    ptAttr.displayName = UA_LOCALIZEDTEXT("en-US", "PumpType");
    retval = UA_Server_addObjectTypeNode(server, pumpTypeId, deviceTypeId,
                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                         UA_QUALIFIEDNAME(1, "PumpType"), ptAttr,
                                         NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_VariableAttributes statusAttr = UA_VariableAttributes_default;
    statusAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
    statusAttr.valueRank = UA_VALUERANK_SCALAR;
    UA_NodeId statusId;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpTypeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "Status"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       statusAttr, NULL, &statusId);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

/* UA_NS0ID_MODELLINGRULE_MANDATORY is not available in Minimal Nodeset */
#ifdef UA_GENERATED_NAMESPACE_ZERO
    /* Make the status variable mandatory */
    retval = UA_Server_addReference(server, statusId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASMODELLINGRULE),
                                    UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_MODELLINGRULE_MANDATORY), true);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
#endif

    UA_VariableAttributes rpmAttr = UA_VariableAttributes_default;
    rpmAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MotorRPM");
    rpmAttr.valueRank = UA_VALUERANK_SCALAR;
    retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, pumpTypeId,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                       UA_QUALIFIEDNAME(1, "MotorRPMs"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       rpmAttr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Instantiate the variable */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "MyPump");
    retval = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                     UA_QUALIFIEDNAME(1, "MyPump"),
                                     pumpTypeId, /* this refers to the object type
                                                    identifier */
                                     oAttr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(ObjectWithDynamicVariableChild) {
    /* Add a ServerRedundancyType object */
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US","my object with variable child");

    UA_NodeId newObjectId;
    UA_NodeId_init(&newObjectId);

    UA_StatusCode res = UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(0, "MyObjectWithVariableChild"), UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERREDUNDANCYTYPE),
                                  attr, NULL, &newObjectId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = newObjectId;
    bp.relativePath.elementsSize = 1;

    UA_RelativePathElement bpe;
    UA_RelativePathElement_init(&bpe);
    bpe.targetName = UA_QUALIFIEDNAME(0, "RedundancySupport");
    bpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    bp.relativePath.elements = &bpe;

    UA_BrowsePathResult bpr = UA_Server_translateBrowsePathToNodeIds(server, &bp);

    ck_assert_uint_eq(bpr.targetsSize, 1);

    UA_WriteValue wv;
    UA_WriteValue_init(&wv);
    wv.nodeId = bpr.targets->targetId.nodeId;
    wv.attributeId = UA_ATTRIBUTEID_VALUE;
    wv.value.hasValue = UA_TRUE;
    UA_Int32 rt = 1;
    UA_Variant_setScalar(&wv.value.value, &rt, &UA_TYPES[UA_TYPES_INT32]);
    wv.value.hasSourceTimestamp = UA_TRUE;
    wv.value.sourceTimestamp = 12345;

    res = UA_Server_write(server, &wv);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = bpr.targets->targetId.nodeId;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue dv = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);
    ck_assert(dv.hasSourceTimestamp == UA_TRUE);
    ck_assert_int_eq(dv.sourceTimestamp, 12345);

    UA_BrowsePathResult_clear(&bpr);
    UA_DataValue_clear(&dv);
} END_TEST

static UA_NodeId
findReference(const UA_NodeId sourceId, const UA_NodeId refTypeId) {
	UA_BrowseDescription * bDesc = UA_BrowseDescription_new();
	UA_NodeId_copy(&sourceId, &bDesc->nodeId);
	bDesc->browseDirection = UA_BROWSEDIRECTION_FORWARD;
	bDesc->includeSubtypes = true;
	bDesc->resultMask = UA_BROWSERESULTMASK_REFERENCETYPEID;
	UA_BrowseResult bRes = UA_Server_browse(server, 0, bDesc);
	ck_assert(bRes.statusCode == UA_STATUSCODE_GOOD);

	UA_NodeId outNodeId = UA_NODEID_NULL;
    for(size_t i = 0; i < bRes.referencesSize; i++) {
        UA_ReferenceDescription rDesc = bRes.references[i];
        if(UA_NodeId_equal(&rDesc.referenceTypeId, &refTypeId)) {
            UA_NodeId_copy(&rDesc.nodeId.nodeId, &outNodeId);
            break;
        }
    }

	UA_BrowseDescription_clear(bDesc);
	UA_BrowseDescription_delete(bDesc);
	UA_BrowseResult_clear(&bRes);
	return outNodeId;
}

static UA_NodeId
registerRefType(char *forwName, char *invName) {
	UA_NodeId outNodeId;
	UA_ReferenceTypeAttributes refattr = UA_ReferenceTypeAttributes_default;
	refattr.displayName = UA_LOCALIZEDTEXT(NULL, forwName);
	refattr.inverseName = UA_LOCALIZEDTEXT(NULL, invName );
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, forwName);
	UA_StatusCode st =
        UA_Server_addReferenceTypeNode(server, UA_NODEID_NULL,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                       browseName, refattr, NULL, &outNodeId);
	ck_assert(st == UA_STATUSCODE_GOOD);
	return outNodeId;
}

static UA_NodeId
addObjInstance(const UA_NodeId parentNodeId, char *dispName) {
	UA_NodeId outNodeId;
	UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
	oAttr.displayName = UA_LOCALIZEDTEXT(NULL, dispName);
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, dispName);
	UA_StatusCode st =
        UA_Server_addObjectNode(server, UA_NODEID_NULL,
                                parentNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                browseName, UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                oAttr, NULL, &outNodeId);
	ck_assert(st == UA_STATUSCODE_GOOD);
	return outNodeId;
}

START_TEST(AddDoubleReference) {
	// create two different reference types
	UA_NodeId ref1TypeId = registerRefType("HasRef1", "IsRefOf1");
	UA_NodeId ref2TypeId = registerRefType("HasRef2", "IsRefOf2");

	// create two different object instances
    UA_NodeId objectsNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId sourceId = addObjInstance(objectsNodeId, "obj1");
	UA_NodeId targetId = addObjInstance(objectsNodeId, "obj2");

	// connect them twice, one time per reference type
	UA_ExpandedNodeId targetExpId;
	targetExpId.nodeId       = targetId;
	targetExpId.namespaceUri = UA_STRING_NULL;
	targetExpId.serverIndex  = 0;
	UA_StatusCode st;
	st = UA_Server_addReference(server, sourceId, ref1TypeId, targetExpId, true);
	ck_assert(st == UA_STATUSCODE_GOOD);
	st = UA_Server_addReference(server, sourceId, ref2TypeId, targetExpId, true);
	ck_assert(st == UA_STATUSCODE_GOOD);
    /* repetition fails */
	st = UA_Server_addReference(server, sourceId, ref2TypeId, targetExpId, true);
	ck_assert(st != UA_STATUSCODE_GOOD);

	// check references where added
	UA_NodeId targetCheckId;
	targetCheckId = findReference(sourceId, ref1TypeId);
	ck_assert(UA_NodeId_equal(&targetCheckId, &targetId));
	targetCheckId = findReference(sourceId, ref2TypeId);
	ck_assert(UA_NodeId_equal(&targetCheckId, &targetId));


} END_TEST

int main(void) {
    Suite *s = suite_create("services_nodemanagement");

    TCase *tc_addnodes = tcase_create("addnodes");
    tcase_add_checked_fixture(tc_addnodes, setup, teardown);
    tcase_add_test(tc_addnodes, AddVariableNode);
    tcase_add_test(tc_addnodes, AddVariableNode_ValueRankZero);
    tcase_add_test(tc_addnodes, AddVariableNode_EmptyValueWithNonZeroValueRank);
    tcase_add_test(tc_addnodes, AddVariableNode_Matrix);
    tcase_add_test(tc_addnodes, AddVariableNode_ExtensionObject);
    tcase_add_test(tc_addnodes, InstantiateVariableTypeNode);
    tcase_add_test(tc_addnodes, InstantiateVariableTypeNodeWrongDims);
    tcase_add_test(tc_addnodes, InstantiateVariableTypeNodeLessDims);
    tcase_add_test(tc_addnodes, AddComplexTypeWithInheritance);
    tcase_add_test(tc_addnodes, AddNodeTwiceGivesError);
    tcase_add_test(tc_addnodes, AddObjectWithConstructor);
    tcase_add_test(tc_addnodes, InstantiateObjectType);
    tcase_add_test(tc_addnodes, ObjectWithDynamicVariableChild);
    suite_add_tcase(s, tc_addnodes);

    TCase *tc_deletenodes = tcase_create("deletenodes");
    tcase_add_checked_fixture(tc_deletenodes, setup, teardown);
    tcase_add_test(tc_deletenodes, DeleteObjectWithDestructor);
    tcase_add_test(tc_deletenodes, DeleteObjectAndReferences);
    suite_add_tcase(s, tc_deletenodes);

    TCase *tc_addreferences = tcase_create("addreferences");
    tcase_add_checked_fixture(tc_addreferences, setup, teardown);
    tcase_add_test(tc_addreferences, AddDoubleReference);
    suite_add_tcase(s, tc_addreferences);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
