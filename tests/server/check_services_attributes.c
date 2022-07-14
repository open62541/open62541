/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server_config_default.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"
#include "testing_clock.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __clang__
//required for ck_assert_ptr_eq and const casting
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#endif

static UA_Server *server = NULL;

static UA_StatusCode
readCPUTemperature(UA_Server *server_,
                   const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *nodeId, void *nodeContext,
                   UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                   UA_DataValue *dataValue) {
    UA_Float temp = 20.5f;
    UA_Variant_setScalarCopy(&dataValue->value, &temp, &UA_TYPES[UA_TYPES_FLOAT]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static void teardown(void) {
    UA_Server_delete(server);
}

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* VariableNode */
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&vattr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    vattr.description = UA_LOCALIZEDTEXT("locale","the answer");
    vattr.displayName = UA_LOCALIZEDTEXT("locale","the answer");
    vattr.valueRank = UA_VALUERANK_ANY;
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    retval = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                       parentReferenceNodeId, myIntegerName,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       vattr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Enum VariableNode */
    UA_MessageSecurityMode m = UA_MESSAGESECURITYMODE_SIGN;
    UA_Variant_setScalar(&vattr.value, &m, &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE]);
    vattr.description = UA_LOCALIZEDTEXT("locale","the enum answer");
    vattr.displayName = UA_LOCALIZEDTEXT("locale","the enum answer");
    vattr.valueRank = UA_VALUERANK_ANY;
    retval = UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "the.enum.answer"),
                                       parentNodeId, parentReferenceNodeId,
                                       UA_QUALIFIEDNAME(1, "the enum answer"),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       vattr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* DataSource VariableNode */
    vattr = UA_VariableAttributes_default;
    UA_DataSource temperatureDataSource;
    temperatureDataSource.read = readCPUTemperature;
    temperatureDataSource.write = NULL;
    vattr.description = UA_LOCALIZEDTEXT("en-US","temperature");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US","temperature");
    retval = UA_Server_addDataSourceVariableNode(server, UA_NODEID_STRING(1, "cpu.temperature"),
                                                 UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                 UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                                 UA_QUALIFIEDNAME(1, "cpu temperature"),
                                                 UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                 vattr, temperatureDataSource,
                                                 NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* VariableNode with array */
    vattr = UA_VariableAttributes_default;
    UA_Int32 myIntegerArray[9] = {1,2,3,4,5,6,7,8,9};
    UA_Variant_setArray(&vattr.value, &myIntegerArray, 9, &UA_TYPES[UA_TYPES_INT32]);
    vattr.valueRank = UA_VALUERANK_ANY;
    UA_UInt32 myIntegerDimensions[2] = {3,3};
    vattr.value.arrayDimensions = myIntegerDimensions;
    vattr.value.arrayDimensionsSize = 2;
    vattr.displayName = UA_LOCALIZEDTEXT("locale","myarray");
    myIntegerName = UA_QUALIFIEDNAME(1, "myarray");
    myIntegerNodeId = UA_NODEID_STRING(1, "myarray");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    retval = UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                                       parentReferenceNodeId, myIntegerName,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       vattr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* ObjectNode */
    UA_ObjectAttributes obj_attr = UA_ObjectAttributes_default;
    obj_attr.description = UA_LOCALIZEDTEXT("en-US","Demo");
    obj_attr.displayName = UA_LOCALIZEDTEXT("en-US","Demo");
    retval = UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 50),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                     UA_QUALIFIEDNAME(1, "Demo"),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                                     obj_attr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* ViewNode */
    UA_ViewAttributes view_attr = UA_ViewAttributes_default;
    view_attr.description = UA_LOCALIZEDTEXT("en-US", "Viewtest");
    view_attr.displayName = UA_LOCALIZEDTEXT("en-US", "Viewtest");
    retval = UA_Server_addViewNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWNODE),
                                   UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
                                   UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                   UA_QUALIFIEDNAME(0, "Viewtest"), view_attr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* DataTypeNode */
    UA_DataTypeAttributes typeattr = UA_DataTypeAttributes_default;
    typeattr.displayName = UA_LOCALIZEDTEXT("en-US", "TestDataType");
    UA_Server_addDataTypeNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_ARGUMENT),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATATYPE),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                  UA_QUALIFIEDNAME(0, "Argument"), typeattr, NULL, NULL);

#ifdef UA_ENABLE_METHODCALLS
    /* MethodNode */
    UA_MethodAttributes ma = UA_MethodAttributes_default;
    ma.description = UA_LOCALIZEDTEXT("en-US", "Methodtest");
    ma.displayName = UA_LOCALIZEDTEXT("en-US", "Methodtest");
    retval = UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, UA_NS0ID_METHODNODE),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                     UA_QUALIFIEDNAME(0, "Methodtest"), ma,
                                     NULL, 0, NULL, 0, NULL, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
#endif

    /* Variable node with localized DisplayName and Description */
    UA_VariableAttributes lvattr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&vattr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    lvattr.description = UA_LOCALIZEDTEXT("en-US","MyDescription");
    lvattr.displayName = UA_LOCALIZEDTEXT("en-US","MyDisplayName");
    lvattr.valueRank = UA_VALUERANK_ANY;
    UA_QualifiedName myLocalizedVarName = UA_QUALIFIEDNAME(1, "LocalizedAttributes");
    UA_NodeId myLocalizedVarNodeId = UA_NODEID_STRING(1, "localized.attrs");
    retval = UA_Server_addVariableNode(server, myLocalizedVarNodeId, parentNodeId,
                                       parentReferenceNodeId, myLocalizedVarName,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                       lvattr, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}

static UA_VariableNode* makeCompareSequence(void) {
    UA_VariableNode *node = (UA_VariableNode*)
        UA_NODESTORE_NEW(server, UA_NODECLASS_VARIABLE);

    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&node->value.data.value.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    node->value.data.value.hasValue = true;

    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_QualifiedName_copy(&myIntegerName, &node->head.browseName);

    const UA_LocalizedText myIntegerDisplName = UA_LOCALIZEDTEXT("locale", "the answer");
    UA_Node_insertOrUpdateDescription(&node->head, &myIntegerDisplName);
    UA_Node_insertOrUpdateDisplayName(&node->head, &myIntegerDisplName);

    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId_copy(&myIntegerNodeId, &node->head.nodeId);

    return node;
}

START_TEST(ReadSingleAttributeValueWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_int_eq(resp.status, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_INT32] == resp.value.type);
    ck_assert_int_eq(42, *(UA_Int32* )resp.value.data);
    UA_DataValue_clear(&resp);
} END_TEST

/* Variables under the Server object return the current time for the timestamps */
START_TEST(ReadSingleServerAttribute) {
    UA_fakeSleep(5000);

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_BUILDINFO);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);

    ck_assert_int_eq(resp.status, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert_int_eq(resp.serverTimestamp, UA_DateTime_now());
    ck_assert_int_eq(resp.sourceTimestamp, UA_DateTime_now());
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleDataSourceAttributeValueEmptyWithoutTimestamp) {
    UA_Variant empty;
    UA_Variant_init(&empty);
    UA_StatusCode ret =
        UA_Server_writeValue(server, UA_NODEID_STRING(1, "the.answer"), empty);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, ret);

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    // read 1
    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    ck_assert_int_eq(true, resp.hasValue);
    UA_DataValue_clear(&resp);

    // read 2
    ret = UA_Server_readValue(server, rvi.nodeId, &empty);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, ret);
} END_TEST

START_TEST(ReadSingleAttributeValueRangeWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "myarray");
    rvi.indexRange = UA_STRING("1:2,0:1");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(4, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_INT32] == resp.value.type);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeNodeIdWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_NODEID;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_NODEID] == resp.value.type);
    UA_NodeId* respval = (UA_NodeId*) resp.value.data;
    ck_assert_int_eq(1, respval->namespaceIndex);
    ck_assert(UA_String_equal(&myIntegerNodeId.identifier.string, &respval->identifier.string));
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeNodeClassWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_NODECLASS;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_NODECLASS] == resp.value.type);
    ck_assert_int_eq(*(UA_Int32*)resp.value.data,UA_NODECLASS_VARIABLE);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeBrowseNameWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_BROWSENAME;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    UA_QualifiedName* respval = (UA_QualifiedName*) resp.value.data;
    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_QUALIFIEDNAME] == resp.value.type);
    ck_assert_int_eq(1, respval->namespaceIndex);
    ck_assert(UA_String_equal(&myIntegerName.name, &respval->name));
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeDisplayNameWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    UA_LocalizedText* respval = (UA_LocalizedText*) resp.value.data;
    const UA_LocalizedText comp = UA_LOCALIZEDTEXT("locale", "the answer");
    UA_VariableNode* compNode = makeCompareSequence();
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_LOCALIZEDTEXT] == resp.value.type);
    ck_assert(UA_String_equal(&comp.text, &respval->text));
    UA_LocalizedText displayName =
        UA_Session_getNodeDisplayName(NULL, &compNode->head);
    ck_assert(UA_String_equal(&displayName.locale, &respval->locale));
    UA_DataValue_clear(&resp);
    UA_NODESTORE_DELETE(server, (UA_Node*)compNode);
} END_TEST

START_TEST(ReadSingleAttributeDescriptionWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_DESCRIPTION;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    UA_LocalizedText* respval = (UA_LocalizedText*) resp.value.data;
    UA_VariableNode* compNode = makeCompareSequence();
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_LOCALIZEDTEXT] == resp.value.type);
    UA_LocalizedText description =
        UA_Session_getNodeDescription(NULL, &compNode->head);
    ck_assert(UA_String_equal(&description.locale, &respval->locale));
    ck_assert(UA_String_equal(&description.text, &respval->text));
    UA_DataValue_clear(&resp);
    UA_NODESTORE_DELETE(server, (UA_Node*)compNode);
} END_TEST

START_TEST(ReadSingleAttributeWriteMaskWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_WRITEMASK;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    UA_UInt32* respval = (UA_UInt32*) resp.value.data;
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_UINT32] == resp.value.type);
    ck_assert_int_eq(0,*respval);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeUserWriteMaskWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_USERWRITEMASK;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    /* Uncommented since the userwritemask is always 0xffffffff for the local admin user */
    /* UA_UInt32* respval = (UA_UInt32*) resp.value.data; */
    /* ck_assert_uint_eq(0, resp.value.arrayLength); */
    /* ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_UINT32], resp.value.type); */
    /* ck_assert_int_eq(0,*respval); */
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeIsAbstractWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rvi.attributeId = UA_ATTRIBUTEID_ISABSTRACT;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_BOOLEAN] == resp.value.type);
    ck_assert(*(UA_Boolean* )resp.value.data==false);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeSymmetricWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rvi.attributeId = UA_ATTRIBUTEID_SYMMETRIC;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_BOOLEAN] == resp.value.type);
    ck_assert(*(UA_Boolean* )resp.value.data==false);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeInverseNameWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    rvi.attributeId = UA_ATTRIBUTEID_INVERSENAME;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    UA_LocalizedText* respval = (UA_LocalizedText*) resp.value.data;
    const UA_LocalizedText comp = UA_LOCALIZEDTEXT("", "OrganizedBy");
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_LOCALIZEDTEXT] == resp.value.type);
    ck_assert(UA_String_equal(&comp.text, &respval->text));
    ck_assert(UA_String_equal(&comp.locale, &respval->locale));
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeContainsNoLoopsWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWNODE);
    rvi.attributeId = UA_ATTRIBUTEID_CONTAINSNOLOOPS;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_BOOLEAN] == resp.value.type);
    ck_assert(*(UA_Boolean* )resp.value.data==false);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeEventNotifierWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(1, 50);
    rvi.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_BYTE] == resp.value.type);
    ck_assert_int_eq(*(UA_Byte*)resp.value.data, 0);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeDataTypeWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_DATATYPE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    ck_assert_int_eq(true, resp.hasValue);
    ck_assert(&UA_TYPES[UA_TYPES_NODEID] == resp.value.type);
    UA_NodeId* respval = (UA_NodeId*)resp.value.data;
    ck_assert_int_eq(respval->namespaceIndex,0);
    ck_assert_int_eq(respval->identifier.numeric, UA_NS0ID_BASEDATATYPE);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeValueRankWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_VALUERANK;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_INT32] == resp.value.type);
    ck_assert_int_eq(-2, *(UA_Int32* )resp.value.data);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeArrayDimensionsWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_UINT32] == resp.value.type);
    ck_assert_ptr_eq((UA_Int32*)resp.value.data,0);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeAccessLevelWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_ACCESSLEVEL;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_BYTE] == resp.value.type);
    ck_assert_int_eq(*(UA_Byte*)resp.value.data, UA_ACCESSLEVELMASK_READ); // set by default
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeUserAccessLevelWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_USERACCESSLEVEL;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    /* Uncommented since the accesslevel is always 0xff for the local admin user */
    /* const UA_VariableNode* compNode = */
    /*     (const UA_VariableNode*)UA_NodeStore_getNode(server->nsCtx, &rvi.nodeId); */
    /* ck_assert_uint_eq(0, resp.value.arrayLength); */
    /* ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BYTE], resp.value.type); */
    /* ck_assert_int_eq(*(UA_Byte*)resp.value.data, compNode->accessLevel & 0xFF); // 0xFF is the default userAccessLevel */
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeMinimumSamplingIntervalWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    UA_Double* respval = (UA_Double*) resp.value.data;
    UA_VariableNode *compNode = makeCompareSequence();
    UA_Double comp = (UA_Double) compNode->minimumSamplingInterval;
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_DOUBLE] == resp.value.type);
    ck_assert(*respval == comp);
    UA_DataValue_clear(&resp);
    UA_NODESTORE_DELETE(server, (UA_Node*)compNode);
} END_TEST

START_TEST(ReadSingleAttributeHistorizingWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_HISTORIZING;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_BOOLEAN] == resp.value.type);
    ck_assert(*(UA_Boolean*)resp.value.data==false);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeExecutableWithoutTimestamp) {
#ifdef UA_ENABLE_METHODCALLS
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(1, UA_NS0ID_METHODNODE);
    rvi.attributeId = UA_ATTRIBUTEID_EXECUTABLE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_int_eq(true, resp.hasValue);
    ck_assert_uint_eq(0, resp.value.arrayLength);
    ck_assert(&UA_TYPES[UA_TYPES_BOOLEAN] == resp.value.type);
    ck_assert(*(UA_Boolean*)resp.value.data==true);
    UA_DataValue_clear(&resp);
#endif
} END_TEST

START_TEST(ReadSingleAttributeUserExecutableWithoutTimestamp) {
#ifdef UA_ENABLE_METHODCALLS
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(1, UA_NS0ID_METHODNODE);
    rvi.attributeId = UA_ATTRIBUTEID_USEREXECUTABLE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    /* Uncommented since userexecutable is always true for the local admin user */
    /* ck_assert_uint_eq(0, resp.value.arrayLength); */
    /* ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BOOLEAN], resp.value.type); */
    /* ck_assert(*(UA_Boolean*)resp.value.data==false); */
    UA_DataValue_clear(&resp);
#endif
} END_TEST

START_TEST(ReadSingleDataSourceAttributeValueWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "cpu.temperature");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleDataSourceAttributeDataTypeWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "cpu.temperature");
    rvi.attributeId = UA_ATTRIBUTEID_DATATYPE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    ck_assert_int_eq(resp.hasServerTimestamp, false);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleDataSourceAttributeArrayDimensionsWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "cpu.temperature");
    rvi.attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(ReadSingleAttributeDataTypeDefinitionWithoutTimestamp) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ARGUMENT);
    rvi.attributeId = UA_ATTRIBUTEID_DATATYPEDEFINITION;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);


#ifdef UA_ENABLE_TYPEDESCRIPTION
    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    ck_assert(resp.value.type == &UA_TYPES[UA_TYPES_STRUCTUREDEFINITION]);
    UA_StructureDefinition *def = (UA_StructureDefinition*)resp.value.data;
    ck_assert_uint_eq(def->fieldsSize, 5);
#else
    ck_assert_int_eq(UA_STATUSCODE_BADATTRIBUTEIDINVALID, resp.status);
#endif
    UA_DataValue_clear(&resp);
} END_TEST

/* Tests for writeValue method */

START_TEST(WriteSingleAttributeNodeId) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_NodeId id;
    UA_NodeId_init(&id);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_NODEID;
    wValue.value.hasValue = true;
    UA_Variant_setScalar(&wValue.value.value, &id, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
} END_TEST

START_TEST(WriteSingleAttributeNodeclass) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeClass nc;
    UA_NodeClass_init(&nc);
    wValue.attributeId = UA_ATTRIBUTEID_NODECLASS;
    wValue.value.hasValue = true;
    UA_Variant_setScalar(&wValue.value.value, &nc, &UA_TYPES[UA_TYPES_NODECLASS]);
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
} END_TEST

START_TEST(WriteSingleAttributeBrowseName) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_QualifiedName testValue = UA_QUALIFIEDNAME(1, "the.answer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
} END_TEST

START_TEST(WriteSingleAttributeDisplayName) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText testValue = UA_LOCALIZEDTEXT("en-EN", "the.answer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeDescription) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText testValue = UA_LOCALIZEDTEXT("en-EN", "the.answer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_DESCRIPTION;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeWriteMask) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_UINT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_WRITEMASK;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeIsAbstract) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_ISABSTRACT;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
} END_TEST

START_TEST(WriteSingleAttributeSymmetric) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_SYMMETRIC;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
} END_TEST

START_TEST(WriteSingleAttributeInverseName) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText testValue = UA_LOCALIZEDTEXT("en-US", "not.the.answer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_INVERSENAME;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
} END_TEST

START_TEST(WriteSingleAttributeContainsNoLoops) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_CONTAINSNOLOOPS;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
} END_TEST

START_TEST(WriteSingleAttributeEventNotifier) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Byte testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BYTE]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
} END_TEST

START_TEST(WriteSingleAttributeValue) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 myInteger = 20;
    UA_Variant_setScalar(&wValue.value.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    wValue.value.hasValue = true;
    wValue.value.hasSourceTimestamp = true;
    wValue.value.sourceTimestamp = 1337;
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(resp.hasValue);
    ck_assert(resp.hasSourceTimestamp);
    ck_assert_int_eq(resp.sourceTimestamp, 1337);
    ck_assert_int_eq(20, *(UA_Int32*)resp.value.data);
    UA_DataValue_clear(&resp);
} END_TEST

/* The ServerTimestamp during a Write Request shall be ignored. Instead the
 * server uses its own current time. */
START_TEST(WriteSingleAttributeValueWithServerTimestamp) {
    UA_fakeSleep(5000);

    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 myInteger = 20;
    UA_Variant_setScalar(&wValue.value.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    wValue.value.hasServerTimestamp = true;
    wValue.value.serverTimestamp = 1337;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.answer");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_SERVER);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(resp.hasValue);
    ck_assert_int_eq(20, *(UA_Int32*)resp.value.data);
    ck_assert(resp.hasServerTimestamp);
    ck_assert_int_eq(resp.serverTimestamp, UA_DateTime_now());
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(WriteSingleAttributeValueEnum) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 myInteger = 4;
    UA_Variant_setScalar(&wValue.value.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "the.enum.answer");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "the.enum.answer");
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(resp.hasValue);
    ck_assert_int_eq(4, *(UA_Int32*)resp.value.data);
    UA_DataValue_clear(&resp);
} END_TEST

START_TEST(WriteSingleAttributeValueRangeFromScalar) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 myInteger = 20;
    UA_Variant_setScalar(&wValue.value.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "myarray");
    wValue.indexRange = UA_STRING("0,0");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeValueRangeFromArray) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 myInteger = 20;
    UA_Variant_setArray(&wValue.value.value, &myInteger, 1, &UA_TYPES[UA_TYPES_INT32]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "myarray");
    wValue.indexRange = UA_STRING("0,0");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeDataType) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_DATATYPE;
    wValue.value.hasValue = true;
    UA_Variant_setScalar(&wValue.value.value, &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADTYPEMISMATCH);
} END_TEST

START_TEST(WriteSingleAttributeValueRank) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue = -1;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_INT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_VALUERANK;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    // Returns attributeInvalid, since variant/value may be writable
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeArrayDimensions) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_UInt32 testValue[] = {1,1,1};
    UA_Variant_setArray(&wValue.value.value, &testValue, 3, &UA_TYPES[UA_TYPES_UINT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    // Returns attributeInvalid, since variant/value may be writable
    ck_assert_int_eq(retval, UA_STATUSCODE_BADTYPEMISMATCH);
} END_TEST

START_TEST(WriteSingleAttributeAccessLevel) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Byte testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BYTE]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_ACCESSLEVEL;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeMinimumSamplingInterval) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Double testValue = 0.0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeHistorizing) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_HISTORIZING;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(WriteSingleAttributeExecutable) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_EXECUTABLE;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
} END_TEST

START_TEST(WriteSingleDataSourceAttributeValue) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_INT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "cpu.temperature");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
} END_TEST

START_TEST(CheckDisplayNameLocalization) {
    /* Add a german localization for the DisplayName attribute */
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText germanDisplayName = UA_LOCALIZEDTEXT("de-DE", "MeinAnzeigeName");
    UA_Variant_setScalar(&wValue.value.value, &germanDisplayName, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.nodeId = UA_NODEID_STRING(1, "localized.attrs");
    wValue.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Check the original english value fallback */
    UA_LocalizedText lt;
    UA_LocalizedText_init(&lt);
    retval = UA_Server_readDisplayName(server, wValue.nodeId, &lt);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText expectedEnglishValue = UA_LOCALIZEDTEXT("en-US", "MyDisplayName");
    ck_assert(UA_String_equal(&lt.locale, &expectedEnglishValue.locale));
    ck_assert(UA_String_equal(&lt.text, &expectedEnglishValue.text));
    UA_LocalizedText_clear(&lt);

    /* Check the new german value */
    server->adminSession.localeIdsSize = 1;
    server->adminSession.localeIds = UA_LocaleId_new();
    *server->adminSession.localeIds = UA_STRING_ALLOC("de-DE");

    retval = UA_Server_readDisplayName(server, wValue.nodeId, &lt);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_String_equal(&lt.locale, &germanDisplayName.locale));
    ck_assert(UA_String_equal(&lt.text, &germanDisplayName.text));
    UA_LocalizedText_clear(&lt);

    /* Requesting de-CH should return de-DE if only de-DE is available */
    UA_LocaleId_clear(server->adminSession.localeIds);
    *server->adminSession.localeIds = UA_STRING_ALLOC("de-CH");

    retval = UA_Server_readDisplayName(server, wValue.nodeId, &lt);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_String_equal(&lt.locale, &germanDisplayName.locale));
    ck_assert(UA_String_equal(&lt.text, &germanDisplayName.text));
    UA_LocalizedText_clear(&lt);
} END_TEST

START_TEST(CheckDescriptionLocalization) {
    /* Add a german localization for the Description attribute */
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText germanDescription = UA_LOCALIZEDTEXT("de-DE", "MeineBeschreibung");
    UA_Variant_setScalar(&wValue.value.value, &germanDescription, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.nodeId = UA_NODEID_STRING(1, "localized.attrs");
    wValue.attributeId = UA_ATTRIBUTEID_DESCRIPTION;
    wValue.value.hasValue = true;
    UA_StatusCode retval = UA_Server_write(server, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    /* Check the original english value */
    UA_LocalizedText lt;
    UA_LocalizedText_init(&lt);
    retval = UA_Server_readDescription(server, wValue.nodeId, &lt);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_LocalizedText expectedEnglishValue = UA_LOCALIZEDTEXT("en-US", "MyDescription");
    ck_assert(UA_String_equal(&lt.locale, &expectedEnglishValue.locale));
    ck_assert(UA_String_equal(&lt.text, &expectedEnglishValue.text));
    UA_LocalizedText_clear(&lt);

    /* Check the new german value */
    server->adminSession.localeIdsSize = 1;
    server->adminSession.localeIds = UA_LocaleId_new();
    *server->adminSession.localeIds = UA_STRING_ALLOC("de-DE");

    retval = UA_Server_readDescription(server, wValue.nodeId, &lt);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_String_equal(&lt.locale, &germanDescription.locale));
    ck_assert(UA_String_equal(&lt.text, &germanDescription.text));
    UA_LocalizedText_clear(&lt);

    /* Requesting de-CH should return de-DE if only de-DE is available */
    UA_LocaleId_clear(server->adminSession.localeIds);
    *server->adminSession.localeIds = UA_STRING_ALLOC("de-CH");

    retval = UA_Server_readDescription(server, wValue.nodeId, &lt);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(UA_String_equal(&lt.locale, &germanDescription.locale));
    ck_assert(UA_String_equal(&lt.text, &germanDescription.text));
    UA_LocalizedText_clear(&lt);
} END_TEST

static Suite * testSuite_services_attributes(void) {
    Suite *s = suite_create("services_attributes_read");

    TCase *tc_readSingleAttributes = tcase_create("readSingleAttributes");
    tcase_add_checked_fixture(tc_readSingleAttributes, setup, teardown);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeValueWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleServerAttribute);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeValueRangeWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeNodeIdWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeNodeClassWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeBrowseNameWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeDisplayNameWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeDescriptionWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeWriteMaskWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeUserWriteMaskWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeIsAbstractWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeSymmetricWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeInverseNameWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeContainsNoLoopsWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeEventNotifierWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeDataTypeWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeValueRankWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeArrayDimensionsWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeAccessLevelWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeUserAccessLevelWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeMinimumSamplingIntervalWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeHistorizingWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeExecutableWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeUserExecutableWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleDataSourceAttributeValueWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleDataSourceAttributeValueEmptyWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleDataSourceAttributeDataTypeWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleDataSourceAttributeArrayDimensionsWithoutTimestamp);
    tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeDataTypeDefinitionWithoutTimestamp);

    suite_add_tcase(s, tc_readSingleAttributes);

    TCase *tc_writeSingleAttributes = tcase_create("writeSingleAttributes");
    tcase_add_checked_fixture(tc_writeSingleAttributes, setup, teardown);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeNodeId);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeNodeclass);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeBrowseName);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeDisplayName);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeDescription);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeWriteMask);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeIsAbstract);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeSymmetric);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeInverseName);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeContainsNoLoops);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeEventNotifier);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeValue);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeValueWithServerTimestamp);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeValueEnum);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeDataType);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeValueRangeFromScalar);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeValueRangeFromArray);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeValueRank);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeArrayDimensions);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeAccessLevel);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeMinimumSamplingInterval);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeHistorizing);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeExecutable);
    tcase_add_test(tc_writeSingleAttributes, WriteSingleDataSourceAttributeValue);

    suite_add_tcase(s, tc_writeSingleAttributes);

    TCase *tc_localization = tcase_create("localization");
    tcase_add_checked_fixture(tc_localization, setup, teardown);
    tcase_add_test(tc_localization, CheckDisplayNameLocalization);
    tcase_add_test(tc_localization, CheckDescriptionLocalization);
    suite_add_tcase(s, tc_localization);

    return s;
}

int main(void) {

    int number_failed = 0;
    Suite *s;
    s = testSuite_services_attributes();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);

    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


#ifdef __clang__
#pragma clang diagnostic pop
#endif
