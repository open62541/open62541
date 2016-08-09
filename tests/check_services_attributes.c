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

/* copied definition */
UA_StatusCode parse_numericrange(const UA_String *str, UA_NumericRange *range);

static UA_StatusCode
readCPUTemperature_broken(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
                          const UA_NumericRange *range, UA_DataValue *dataValue) {
  dataValue->hasValue = true;
  return UA_STATUSCODE_GOOD;
}

static UA_Server* makeTestSequence(void) {
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);

    /* VariableNode */
    UA_VariableAttributes vattr;
    UA_VariableAttributes_init(&vattr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&vattr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    vattr.description = UA_LOCALIZEDTEXT("locale","the answer");
    vattr.displayName = UA_LOCALIZEDTEXT("locale","the answer");
    vattr.valueRank = -2;
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NULL, vattr, NULL, NULL);

    /* DataSource VariableNode */
    UA_VariableAttributes_init(&vattr);
    UA_DataSource temperatureDataSource = (UA_DataSource) {
                                           .handle = NULL, .read = NULL, .write = NULL};
    vattr.description = UA_LOCALIZEDTEXT("en_US","temperature");
    vattr.displayName = UA_LOCALIZEDTEXT("en_US","temperature");
    UA_Server_addDataSourceVariableNode(server, UA_NODEID_STRING(1, "cpu.temperature"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        UA_QUALIFIEDNAME(1, "cpu temperature"),
                                        UA_NODEID_NULL, vattr, temperatureDataSource, NULL);

    /* DataSource Variable returning no value */
    UA_DataSource temperatureDataSource1 = (UA_DataSource) {
                                            .handle = NULL, .read = readCPUTemperature_broken, .write = NULL};
    vattr.description = UA_LOCALIZEDTEXT("en_US","temperature1");
    vattr.displayName = UA_LOCALIZEDTEXT("en_US","temperature1");
    UA_Server_addDataSourceVariableNode(server, UA_NODEID_STRING(1, "cpu.temperature1"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        UA_QUALIFIEDNAME(1, "cpu temperature bogus"),
                                        UA_NODEID_NULL, vattr, temperatureDataSource1, NULL);
    /* VariableNode with array */
    UA_VariableAttributes_init(&vattr);
    UA_Int32 myIntegerArray[9] = {1,2,3,4,5,6,7,8,9};
    UA_Variant_setArray(&vattr.value, &myIntegerArray, 9, &UA_TYPES[UA_TYPES_INT32]);
    UA_Int32 myIntegerDimensions[2] = {3,3};
    vattr.value.arrayDimensions = myIntegerDimensions;
    vattr.value.arrayDimensionsSize = 2;
    vattr.displayName = UA_LOCALIZEDTEXT("locale","myarray");
    myIntegerName = UA_QUALIFIEDNAME(1, "myarray");
    myIntegerNodeId = UA_NODEID_STRING(1, "myarray");
    parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NULL, vattr, NULL, NULL);

    /* ObjectNode */
    UA_ObjectAttributes obj_attr;
    UA_ObjectAttributes_init(&obj_attr);
    obj_attr.description = UA_LOCALIZEDTEXT("en_US","Demo");
    obj_attr.displayName = UA_LOCALIZEDTEXT("en_US","Demo");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 50),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Demo"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            obj_attr, NULL, NULL);

    /* ViewNode */
    UA_ViewAttributes view_attr;
    UA_ViewAttributes_init(&view_attr);
    view_attr.description = UA_LOCALIZEDTEXT("en_US", "Viewtest");
    view_attr.displayName = UA_LOCALIZEDTEXT("en_US", "Viewtest");
    UA_Server_addViewNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWNODE),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_VIEWSFOLDER),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                          UA_QUALIFIEDNAME(0, "Viewtest"), view_attr, NULL, NULL);

#ifdef UA_ENABLE_METHODCALLS
    /* MethodNode */
    UA_MethodAttributes ma;
    UA_MethodAttributes_init(&ma);
    ma.description = UA_LOCALIZEDTEXT("en_US", "Methodtest");
    ma.displayName = UA_LOCALIZEDTEXT("en_US", "Methodtest");
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(0, UA_NS0ID_METHODNODE),
                            UA_NODEID_NUMERIC(0, 3),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME_ALLOC(0, "Methodtest"), ma,
                            NULL, NULL, 0, NULL, 0, NULL, NULL);
#endif

    return server;
}

static UA_VariableNode* makeCompareSequence(void) {
    UA_VariableNode *node = UA_NodeStore_newVariableNode();

    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&node->value.variant.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);

    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_QualifiedName_copy(&myIntegerName,&node->browseName);

    const UA_LocalizedText myIntegerDisplName = UA_LOCALIZEDTEXT("locale", "the answer");
    UA_LocalizedText_copy(&myIntegerDisplName, &node->displayName);
    UA_LocalizedText_copy(&myIntegerDisplName, &node->description);

    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId_copy(&myIntegerNodeId,&node->nodeId);

    return node;
}

START_TEST(ReadSingleAttributeValueWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_INT32], resp.value.type);
    ck_assert_int_eq(42, *(UA_Int32* )resp.value.data);
    UA_Server_delete(server);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
} END_TEST

START_TEST(ReadSingleAttributeValueRangeWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "myarray");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;
    rReq.nodesToRead[0].indexRange = UA_STRING_ALLOC("1:2,0:1");
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(4, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_INT32], resp.value.type);
    UA_Server_delete(server);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
} END_TEST

START_TEST(ReadSingleAttributeNodeIdWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_NODEID;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_NODEID], resp.value.type);
    UA_NodeId* respval = (UA_NodeId*) resp.value.data;
    ck_assert_int_eq(1, respval->namespaceIndex);
    ck_assert(UA_String_equal(&myIntegerNodeId.identifier.string, &respval->identifier.string));
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeNodeClassWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_NODECLASS;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_NODECLASS],resp.value.type);
    ck_assert_int_eq(*(UA_Int32*)resp.value.data,UA_NODECLASS_VARIABLE);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeBrowseNameWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_BROWSENAME;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    UA_QualifiedName* respval = (UA_QualifiedName*) resp.value.data;
    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_QUALIFIEDNAME], resp.value.type);
    ck_assert_int_eq(1, respval->namespaceIndex);
    ck_assert(UA_String_equal(&myIntegerName.name, &respval->name));
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeDisplayNameWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    UA_LocalizedText* respval = (UA_LocalizedText*) resp.value.data;
    const UA_LocalizedText comp = UA_LOCALIZEDTEXT("locale", "the answer");
    UA_VariableNode* compNode = makeCompareSequence();
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_LOCALIZEDTEXT], resp.value.type);
    ck_assert(UA_String_equal(&comp.text, &respval->text));
    ck_assert(UA_String_equal(&compNode->displayName.locale, &respval->locale));
    UA_Server_delete(server);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_NodeStore_deleteNode((UA_Node*)compNode);
} END_TEST

START_TEST(ReadSingleAttributeDescriptionWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_DESCRIPTION;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    UA_LocalizedText* respval = (UA_LocalizedText*) resp.value.data;
    UA_VariableNode* compNode = makeCompareSequence();
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_LOCALIZEDTEXT], resp.value.type);
    ck_assert(UA_String_equal(&compNode->description.locale, &respval->locale));
    ck_assert(UA_String_equal(&compNode->description.text, &respval->text));
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_NodeStore_deleteNode((UA_Node*)compNode);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeWriteMaskWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_WRITEMASK;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    UA_UInt32* respval = (UA_UInt32*) resp.value.data;
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_UINT32], resp.value.type);
    ck_assert_int_eq(0,*respval);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeUserWriteMaskWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_USERWRITEMASK;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    UA_UInt32* respval = (UA_UInt32*) resp.value.data;
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_UINT32], resp.value.type);
    ck_assert_int_eq(0,*respval);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeIsAbstractWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_ISABSTRACT;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BOOLEAN], resp.value.type);
    ck_assert(*(UA_Boolean* )resp.value.data==false);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeSymmetricWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_SYMMETRIC;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BOOLEAN], resp.value.type);
    ck_assert(*(UA_Boolean* )resp.value.data==false);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeInverseNameWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId.identifier.numeric = UA_NS0ID_ORGANIZES;
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_INVERSENAME;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    UA_LocalizedText* respval = (UA_LocalizedText*) resp.value.data;
    const UA_LocalizedText comp = UA_LOCALIZEDTEXT("en_US", "OrganizedBy");
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_LOCALIZEDTEXT],resp.value.type);
    ck_assert(UA_String_equal(&comp.text, &respval->text));
    ck_assert(UA_String_equal(&comp.locale, &respval->locale));
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeContainsNoLoopsWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId.identifier.numeric = UA_NS0ID_VIEWNODE;
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_CONTAINSNOLOOPS;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BOOLEAN], resp.value.type);
    ck_assert(*(UA_Boolean* )resp.value.data==false);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeEventNotifierWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_NUMERIC(1, 50);
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BYTE],resp.value.type);
    ck_assert_int_eq(*(UA_Byte*)resp.value.data, 0);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeDataTypeWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_DATATYPE;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    UA_NodeId* respval = (UA_NodeId*) resp.value.data;
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_NODEID], resp.value.type);
    ck_assert_int_eq(respval->namespaceIndex,0);
    ck_assert_int_eq(respval->identifier.numeric,UA_NS0ID_INT32);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeValueRankWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUERANK;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_INT32], resp.value.type);
    ck_assert_int_eq(-2, *(UA_Int32* )resp.value.data);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeArrayDimensionsWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;
    Service_Read_single(server, &adminSession,  UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_INT32], resp.value.type);
    ck_assert_ptr_eq((UA_Int32*)resp.value.data,0);
    UA_DataValue_deleteMembers(&resp);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeAccessLevelWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_ACCESSLEVEL;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BYTE], resp.value.type);
    ck_assert_int_eq(*(UA_Byte*)resp.value.data, 0);
    UA_DataValue_deleteMembers(&resp);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeUserAccessLevelWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_USERACCESSLEVEL;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    const UA_VariableNode* compNode =
        (const UA_VariableNode*)UA_NodeStore_get(server->nodestore, &rReq.nodesToRead[0].nodeId);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BYTE], resp.value.type);
    ck_assert_int_eq(*(UA_Byte*)resp.value.data, compNode->userAccessLevel);
    UA_Server_delete(server);
    UA_DataValue_deleteMembers(&resp);
    UA_ReadRequest_deleteMembers(&rReq);
} END_TEST

START_TEST(ReadSingleAttributeMinimumSamplingIntervalWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    UA_Double* respval = (UA_Double*) resp.value.data;
    UA_VariableNode *compNode = makeCompareSequence();
    UA_Double comp = (UA_Double) compNode->minimumSamplingInterval;
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_DOUBLE], resp.value.type);
    ck_assert(*respval == comp);
    UA_DataValue_deleteMembers(&resp);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_NodeStore_deleteNode((UA_Node*)compNode);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeHistorizingWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_HISTORIZING;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BOOLEAN], resp.value.type);
    ck_assert(*(UA_Boolean*)resp.value.data==false);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(ReadSingleAttributeExecutableWithoutTimestamp) {
#ifdef UA_ENABLE_METHODCALLS
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId.identifier.numeric = UA_NS0ID_METHODNODE;
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_EXECUTABLE;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BOOLEAN], resp.value.type);
    ck_assert(*(UA_Boolean*)resp.value.data==false);
    UA_DataValue_deleteMembers(&resp);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_Server_delete(server);
#endif
} END_TEST

START_TEST(ReadSingleAttributeUserExecutableWithoutTimestamp) {
#ifdef UA_ENABLE_METHODCALLS
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId.identifier.numeric = UA_NS0ID_METHODNODE;
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_USEREXECUTABLE;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(0, resp.value.arrayLength);
    ck_assert_ptr_eq(&UA_TYPES[UA_TYPES_BOOLEAN], resp.value.type);
    ck_assert(*(UA_Boolean*)resp.value.data==false);
    UA_DataValue_deleteMembers(&resp);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_Server_delete(server);
#endif
} END_TEST

START_TEST(ReadSingleDataSourceAttributeDataTypeWithoutTimestampFromBrokenSource) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "cpu.temperature1");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_DATATYPE;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(UA_STATUSCODE_GOOD, resp.status);
    UA_Server_delete(server);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
} END_TEST

START_TEST(ReadSingleDataSourceAttributeValueWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "cpu.temperature");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(UA_STATUSCODE_BADINTERNALERROR, resp.status);
    UA_Server_delete(server);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
} END_TEST

START_TEST(ReadSingleDataSourceAttributeDataTypeWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "cpu.temperature");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_DATATYPE;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(UA_STATUSCODE_BADINTERNALERROR, resp.status);
    UA_Server_delete(server);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
} END_TEST

START_TEST (ReadSingleDataSourceAttributeArrayDimensionsWithoutTimestamp) {
    UA_Server *server = makeTestSequence();
    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "cpu.temperature");
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &rReq.nodesToRead[0], &resp);
    ck_assert_int_eq(UA_STATUSCODE_BADINTERNALERROR, resp.status);
    UA_Server_delete(server);
    UA_ReadRequest_deleteMembers(&rReq);
    UA_DataValue_deleteMembers(&resp);
} END_TEST

/* Tests for writeValue method */

START_TEST(WriteSingleAttributeNodeId) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_NodeId id;
    UA_NodeId_init(&id);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_NODEID;
    wValue.value.hasValue = true;
    UA_Variant_setScalar(&wValue.value.value, &id, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeNodeclass) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeClass class;
    UA_NodeClass_init(&class);
    wValue.attributeId = UA_ATTRIBUTEID_NODECLASS;
    wValue.value.hasValue = true;
    UA_Variant_setScalar(&wValue.value.value, &class, &UA_TYPES[UA_TYPES_NODECLASS]);
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeBrowseName) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_QualifiedName testValue = UA_QUALIFIEDNAME(1, "the.answer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeDisplayName) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText testValue = UA_LOCALIZEDTEXT("en_EN", "the.answer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeDescription) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText testValue = UA_LOCALIZEDTEXT("en_EN", "the.answer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_DESCRIPTION;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeWriteMask) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_UINT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_WRITEMASK;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeUserWriteMask) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_UINT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_USERWRITEMASK;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeIsAbstract) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_ISABSTRACT;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeSymmetric) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_SYMMETRIC;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeInverseName) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText testValue = UA_LOCALIZEDTEXT("en_US", "not.the.answer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_INVERSENAME;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeContainsNoLoops) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_CONTAINSNOLOOPS;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeEventNotifier) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Byte testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BYTE]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeValue) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 myInteger = 20;
    UA_Variant_setScalar(&wValue.value.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);

    UA_DataValue resp;
    UA_DataValue_init(&resp);
    UA_ReadValueId id;
    UA_ReadValueId_init(&id);
    id.nodeId = UA_NODEID_STRING(1, "the.answer");
    id.attributeId = UA_ATTRIBUTEID_VALUE;
    Service_Read_single(server, &adminSession, UA_TIMESTAMPSTORETURN_NEITHER, &id, &resp);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(wValue.value.hasValue);
    ck_assert_int_eq(20, *(UA_Int32*)resp.value.data);
    UA_DataValue_deleteMembers(&resp);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeDataType) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_NodeId typeId;
    UA_NodeId_init(&typeId);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_DATATYPE;
    wValue.value.hasValue = true;
    UA_Variant_setScalar(&wValue.value.value, &typeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeValueRank) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue = -1;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_INT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_VALUERANK;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    // Returns attributeInvalid, since variant/value may be writable
    ck_assert_int_eq(retval, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeArrayDimensions) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue[] = {-1,-1,-1};
    UA_Variant_setArray(&wValue.value.value, &testValue, 3, &UA_TYPES[UA_TYPES_INT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_ARRAYDIMENSIONS;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    // Returns attributeInvalid, since variant/value may be writable
    ck_assert_int_eq(retval, UA_STATUSCODE_BADATTRIBUTEIDINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeAccessLevel) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Byte testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BYTE]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_ACCESSLEVEL;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeUserAccessLevel) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Byte testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BYTE]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_USERACCESSLEVEL;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeMinimumSamplingInterval) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Double testValue = 0.0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeHistorizing) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_HISTORIZING;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeExecutable) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_EXECUTABLE;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleAttributeUserExecutable) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Boolean testValue = true;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    wValue.nodeId = UA_NODEID_STRING(1, "the.answer");
    wValue.attributeId = UA_ATTRIBUTEID_USEREXECUTABLE;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADNODECLASSINVALID);
    UA_Server_delete(server);
} END_TEST

START_TEST(WriteSingleDataSourceAttributeValue) {
    UA_Server *server = makeTestSequence();
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_Int32 testValue = 0;
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_INT32]);
    wValue.nodeId = UA_NODEID_STRING(1, "cpu.temperature");
    wValue.attributeId = UA_ATTRIBUTEID_VALUE;
    wValue.value.hasValue = true;
    UA_StatusCode retval = Service_Write_single(server, &adminSession, &wValue);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADWRITENOTSUPPORTED);
    UA_Server_delete(server);
} END_TEST

START_TEST(numericRange) {
    UA_NumericRange range;
    const UA_String str = (UA_String){9, (UA_Byte*)"1:2,0:3,5"};
    UA_StatusCode retval = parse_numericrange(&str, &range);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(range.dimensionsSize,3);
    ck_assert_int_eq(range.dimensions[0].min,1);
    ck_assert_int_eq(range.dimensions[0].max,2);
    ck_assert_int_eq(range.dimensions[1].min,0);
    ck_assert_int_eq(range.dimensions[1].max,3);
    ck_assert_int_eq(range.dimensions[2].min,5);
    ck_assert_int_eq(range.dimensions[2].max,5);
    UA_free(range.dimensions);
} END_TEST

static Suite * testSuite_services_attributes(void) {
	Suite *s = suite_create("services_attributes_read");

	TCase *tc_readSingleAttributes = tcase_create("readSingleAttributes");
	tcase_add_test(tc_readSingleAttributes, ReadSingleAttributeValueWithoutTimestamp);
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
        tcase_add_test(tc_readSingleAttributes, ReadSingleDataSourceAttributeDataTypeWithoutTimestampFromBrokenSource);
        tcase_add_test(tc_readSingleAttributes, ReadSingleDataSourceAttributeValueWithoutTimestamp);
	tcase_add_test(tc_readSingleAttributes, ReadSingleDataSourceAttributeDataTypeWithoutTimestamp);
	tcase_add_test(tc_readSingleAttributes, ReadSingleDataSourceAttributeArrayDimensionsWithoutTimestamp);

	suite_add_tcase(s, tc_readSingleAttributes);

	TCase *tc_writeSingleAttributes = tcase_create("writeSingleAttributes");
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeNodeId);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeNodeclass);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeBrowseName);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeDisplayName);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeDescription);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeWriteMask);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeUserWriteMask);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeIsAbstract);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeSymmetric);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeInverseName);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeContainsNoLoops);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeEventNotifier);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeValue);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeDataType);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeValueRank);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeArrayDimensions);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeAccessLevel);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeUserAccessLevel);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeMinimumSamplingInterval);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeHistorizing);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeExecutable);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleAttributeUserExecutable);
	tcase_add_test(tc_writeSingleAttributes, WriteSingleDataSourceAttributeValue);

	suite_add_tcase(s, tc_writeSingleAttributes);

	TCase *tc_parseNumericRange = tcase_create("parseNumericRange");
	tcase_add_test(tc_parseNumericRange, numericRange);
	suite_add_tcase(s, tc_parseNumericRange);

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
