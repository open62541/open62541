/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nodeset_loader_test.h"
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <check.h>
#include <stdlib.h>

#include "test_helpers.h"
#include "testing_clock.h"

UA_Server *server = NULL;
char **nodesetPaths = NULL;
int nodesetsNum = 0;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}
static void
teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(Server_loadInputNodesets) {
    for(int cnt = 0; cnt < nodesetsNum; cnt++) {
        UA_StatusCode retVal = loadNodesetFile(server, nodesetPaths[cnt], NULL);
        ck_assert(UA_StatusCode_isGood(retVal));
    }
}
END_TEST

START_TEST(Server_loadXmlElement) {
    const UA_XmlElement xml =
        UA_STRING_STATIC("<UANodeSet><NamespaceUris><Uri>urn:open62541:loader:test</Uri>"
                         "</NamespaceUris><UAObject NodeId=\"ns=1;i=1\" "
                         "BrowseName=\"1:MemoryObject\"><DisplayName>MemoryObject</DisplayName>"
                         "<References><Reference ReferenceType=\"i=35\" IsForward=\"false\">"
                         "i=85</Reference><Reference ReferenceType=\"i=40\">i=61</Reference>"
                         "</References></UAObject></UANodeSet>");
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_GOOD);

    size_t nsIndex = 0;
    ck_assert_uint_eq(
        UA_Server_getNamespaceByName(server, UA_STRING("urn:open62541:loader:test"), &nsIndex),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_le(nsIndex, UA_UINT16_MAX);
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    ck_assert_uint_eq(
        UA_Server_readNodeClass(server, UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 1), &nodeClass),
        UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nodeClass, UA_NODECLASS_OBJECT);
}
END_TEST

START_TEST(Server_loadNamespaceMapping) {
    UA_UInt16 dependencyIndex = UA_Server_addNamespace(server, "urn:open62541:loader:dependency");
    ck_assert_uint_ne(dependencyIndex, 0);

    const UA_XmlElement xml = UA_STRING_STATIC(
        "<UANodeSet><NamespaceUris><Uri>urn:open62541:loader:dependency</Uri>"
        "<Uri>urn:open62541:loader:new</Uri></NamespaceUris>"
        "<UAObject NodeId=\"nsu=urn:open62541:loader:new;i=1\" BrowseName=\"2:MappedObject\">"
        "<DisplayName>MappedObject</DisplayName><References>"
        "<Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>"
        "<Reference ReferenceType=\"i=40\">i=61</Reference>"
        "</References></UAObject></UANodeSet>");
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_GOOD);

    size_t newIndex = 0;
    ck_assert_uint_eq(
        UA_Server_getNamespaceByName(server, UA_STRING("urn:open62541:loader:new"), &newIndex),
        UA_STATUSCODE_GOOD);
    ck_assert_uint_le(newIndex, UA_UINT16_MAX);
    ck_assert_uint_ne(newIndex, dependencyIndex);

    UA_QualifiedName browseName;
    UA_QualifiedName_init(&browseName);
    UA_NodeId nodeId = UA_NODEID_NUMERIC((UA_UInt16)newIndex, 1);
    ck_assert_uint_eq(UA_Server_readBrowseName(server, nodeId, &browseName), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(browseName.namespaceIndex, newIndex);
    UA_String expectedName = UA_STRING("MappedObject");
    ck_assert(UA_String_equal(&browseName.name, &expectedName));
    UA_QualifiedName_clear(&browseName);
}
END_TEST

START_TEST(Server_loadForwardParentReferences) {
    const UA_XmlElement xml = UA_STRING_STATIC(
        "<UANodeSet><NamespaceUris><Uri>urn:open62541:loader:forward-parent</Uri>"
        "</NamespaceUris><UAObject NodeId=\"ns=1;i=1\" BrowseName=\"1:Parent\">"
        "<DisplayName>Parent</DisplayName><References>"
        "<Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>"
        "<Reference ReferenceType=\"i=40\">i=61</Reference>"
        "<Reference ReferenceType=\"i=47\">ns=1;i=2</Reference>"
        "<Reference ReferenceType=\"i=47\">ns=1;i=3</Reference>"
        "<Reference ReferenceType=\"i=47\">ns=1;i=4</Reference>"
        "<Reference ReferenceType=\"i=47\">ns=1;i=5</Reference>"
        "</References></UAObject>"
        "<UAObject NodeId=\"ns=1;i=2\" BrowseName=\"1:ChildObject\">"
        "<DisplayName>ChildObject</DisplayName><References>"
        "<Reference ReferenceType=\"i=40\">i=61</Reference>"
        "</References></UAObject>"
        "<UAVariable NodeId=\"ns=1;i=3\" BrowseName=\"1:ChildVariable\" DataType=\"i=6\">"
        "<DisplayName>ChildVariable</DisplayName><References>"
        "<Reference ReferenceType=\"i=40\">i=63</Reference>"
        "</References></UAVariable>"
        "<UAMethod NodeId=\"ns=1;i=4\" BrowseName=\"1:ChildMethod\">"
        "<DisplayName>ChildMethod</DisplayName></UAMethod>"
        "<UAView NodeId=\"ns=1;i=5\" BrowseName=\"1:ChildView\">"
        "<DisplayName>ChildView</DisplayName></UAView>"
        "</UANodeSet>");
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_GOOD);

    size_t nsIndex = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(
                          server, UA_STRING("urn:open62541:loader:forward-parent"), &nsIndex),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_le(nsIndex, UA_UINT16_MAX);

    const UA_NodeClass expected[] = {UA_NODECLASS_OBJECT, UA_NODECLASS_VARIABLE,
                                     UA_NODECLASS_METHOD, UA_NODECLASS_VIEW};
    for(size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
        UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, (UA_UInt32)i + 2);
        ck_assert_uint_eq(UA_Server_readNodeClass(server, id, &nodeClass), UA_STATUSCODE_GOOD);
        ck_assert_int_eq(nodeClass, expected[i]);
    }
}
END_TEST

START_TEST(Server_loadCustomHierarchicalParentReference) {
    const UA_XmlElement xml = UA_STRING_STATIC(
        "<UANodeSet><NamespaceUris><Uri>urn:open62541:loader:custom-parent</Uri>"
        "</NamespaceUris>"
        "<UAReferenceType NodeId=\"ns=1;i=1\" BrowseName=\"1:HasCustomChild\">"
        "<DisplayName>HasCustomChild</DisplayName><InverseName>CustomChildOf</InverseName>"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=33</Reference>"
        "</References></UAReferenceType>"
        "<UAObject NodeId=\"ns=1;i=2\" BrowseName=\"1:CustomChild\">"
        "<DisplayName>CustomChild</DisplayName><References>"
        "<Reference ReferenceType=\"ns=1;i=1\" IsForward=\"false\">i=85</Reference>"
        "<Reference ReferenceType=\"i=40\">i=58</Reference>"
        "</References></UAObject></UANodeSet>");
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_GOOD);

    size_t nsIndex = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(
                          server, UA_STRING("urn:open62541:loader:custom-parent"), &nsIndex),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_le(nsIndex, UA_UINT16_MAX);

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = UA_NS0ID(OBJECTSFOLDER);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.referenceTypeId = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 1);
    bd.includeSubtypes = false;
    bd.resultMask = UA_BROWSERESULTMASK_ALL;
    UA_BrowseResult result = UA_Server_browse(server, 0, &bd);
    ck_assert_uint_eq(result.statusCode, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesSize, 1);
    UA_NodeId expected = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 2);
    ck_assert(UA_NodeId_equal(&result.references[0].nodeId.nodeId, &expected));
    UA_BrowseResult_clear(&result);
}
END_TEST

START_TEST(Server_rejectInvalidXmlElement) {
    const UA_XmlElement xml = UA_STRING_STATIC("<UANodeSet>");
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_BADDECODINGERROR);
}
END_TEST

START_TEST(Server_ignoreMalformedArrayDimensions) {
    const UA_XmlElement xml = UA_STRING_STATIC(
        "<UANodeSet><NamespaceUris><Uri>urn:open62541:loader:dimensions</Uri>"
        "</NamespaceUris><UAVariable NodeId=\"ns=1;i=1\" BrowseName=\"1:Dimensions\" "
        "DataType=\"i=6\" ValueRank=\"2\" ArrayDimensions=\"1,\">"
        "<DisplayName>Dimensions</DisplayName><References>"
        "<Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>"
        "<Reference ReferenceType=\"i=40\">i=63</Reference>"
        "</References></UAVariable></UANodeSet>");
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_GOOD);

    size_t nsIndex = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(
                          server, UA_STRING("urn:open62541:loader:dimensions"), &nsIndex),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_le(nsIndex, UA_UINT16_MAX);

    UA_Variant dimensions;
    UA_Variant_init(&dimensions);
    UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 1);
    ck_assert_uint_eq(UA_Server_readArrayDimensions(server, id, &dimensions), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&dimensions, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(dimensions.arrayLength, 2);
    UA_UInt32 *values = (UA_UInt32 *)dimensions.data;
    ck_assert_uint_eq(values[0], 0);
    ck_assert_uint_eq(values[1], 0);
    UA_Variant_clear(&dimensions);
}
END_TEST

START_TEST(Server_loadMissingNodeAttributes) {
    const UA_XmlElement xml = UA_STRING_STATIC(
        "<UANodeSet><NamespaceUris><Uri>urn:open62541:loader:attributes</Uri>"
        "</NamespaceUris>"
        "<UAReferenceType NodeId=\"ns=1;i=1\" BrowseName=\"1:AbstractReference\" "
        "IsAbstract=\"true\" WriteMask=\"4\" UserWriteMask=\"8\">"
        "<DisplayName>AbstractReference</DisplayName><InverseName>ReferencedBy</InverseName>"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=33</Reference>"
        "</References></UAReferenceType>"
        "<UAVariableType NodeId=\"ns=1;i=2\" BrowseName=\"1:ValuedVariableType\" "
        "DataType=\"i=6\" WriteMask=\"12\" UserWriteMask=\"16\">"
        "<DisplayName>ValuedVariableType</DisplayName>"
        "<Value><Int32>42</Int32></Value>"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=63</Reference>"
        "</References></UAVariableType></UANodeSet>");
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_GOOD);

    size_t nsIndex = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(
                          server, UA_STRING("urn:open62541:loader:attributes"), &nsIndex),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_le(nsIndex, UA_UINT16_MAX);

    UA_NodeId referenceTypeId = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 1);
    UA_Boolean isAbstract = false;
    ck_assert_uint_eq(UA_Server_readIsAbstract(server, referenceTypeId, &isAbstract),
                      UA_STATUSCODE_GOOD);
    ck_assert(isAbstract);
    UA_UInt32 writeMask = 0;
    ck_assert_uint_eq(UA_Server_readWriteMask(server, referenceTypeId, &writeMask),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(writeMask, 4);

    UA_NodeId variableTypeId = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 2);
    ck_assert_uint_eq(UA_Server_readWriteMask(server, variableTypeId, &writeMask),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(writeMask, 12);
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(server, variableTypeId, &value), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_int_eq(*(UA_Int32 *)value.data, 42);
    UA_Variant_clear(&value);
}
END_TEST

START_TEST(Server_loadLegacyChildAttributes) {
    const UA_XmlElement xml = UA_STRING_STATIC(
        "<UANodeSet><NamespaceUris><Uri>urn:open62541:loader:child-attributes</Uri>"
        "</NamespaceUris><UAVariable NodeId=\"ns=1;i=1\">"
        "<BrowseName>1:ChildAttributes</BrowseName><DisplayName>ChildAttributes</DisplayName>"
        "<WriteMask>4</WriteMask><UserWriteMask>8</UserWriteMask><DataType>i=6</DataType>"
        "<ValueRank>1</ValueRank><ArrayDimensions><ListOfUInt32><UInt32>2</UInt32>"
        "</ListOfUInt32></ArrayDimensions><AccessLevel>3</AccessLevel><UserAccessLevel>1"
        "</UserAccessLevel><MinimumSamplingInterval>2.5</MinimumSamplingInterval>"
        "<Historizing>true</Historizing><Value><ListOfInt32><Int32>1</Int32><Int32>2</Int32>"
        "</ListOfInt32></Value><References>"
        "<Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>"
        "<Reference ReferenceType=\"i=40\">i=63</Reference>"
        "</References></UAVariable></UANodeSet>");
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_GOOD);

    size_t nsIndex = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(
                          server, UA_STRING("urn:open62541:loader:child-attributes"), &nsIndex),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_le(nsIndex, UA_UINT16_MAX);
    UA_NodeId id = UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 1);

    UA_QualifiedName browseName;
    UA_QualifiedName_init(&browseName);
    ck_assert_uint_eq(UA_Server_readBrowseName(server, id, &browseName), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(browseName.namespaceIndex, nsIndex);
    const UA_String expectedBrowseName = UA_STRING_STATIC("ChildAttributes");
    ck_assert(UA_String_equal(&browseName.name, &expectedBrowseName));
    UA_QualifiedName_clear(&browseName);

    UA_UInt32 writeMask = 0;
    ck_assert_uint_eq(UA_Server_readWriteMask(server, id, &writeMask), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(writeMask, 4);
    UA_Int32 valueRank = 0;
    ck_assert_uint_eq(UA_Server_readValueRank(server, id, &valueRank), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(valueRank, 1);
    UA_Variant dimensions;
    UA_Variant_init(&dimensions);
    ck_assert_uint_eq(UA_Server_readArrayDimensions(server, id, &dimensions), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&dimensions, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(dimensions.arrayLength, 1);
    ck_assert_uint_eq(*(UA_UInt32 *)dimensions.data, 2);
    UA_Variant_clear(&dimensions);

    UA_Byte accessLevel = 0;
    ck_assert_uint_eq(UA_Server_readAccessLevel(server, id, &accessLevel), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(accessLevel, 3);
    UA_Double minimumSamplingInterval = 0.0;
    ck_assert_uint_eq(UA_Server_readMinimumSamplingInterval(server, id, &minimumSamplingInterval),
                      UA_STATUSCODE_GOOD);
    ck_assert_double_eq_tol(minimumSamplingInterval, 2.5, 0.0001);
    UA_Boolean historizing = false;
    ck_assert_uint_eq(UA_Server_readHistorizing(server, id, &historizing), UA_STATUSCODE_GOOD);
    ck_assert(historizing);
}
END_TEST

static void
setupImportValidation(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_nonnull(server);
}

static void
teardownImportValidation(void) {
    UA_Server_delete(server);
}

/* Every import also contains an independent object, which must survive a
 * rejected node or datatype description elsewhere in the document. */
static UA_UInt16
loadValidationNodes(const char *nodes) {
    UA_XmlElement xml = UA_STRING_NULL;
    ck_assert_uint_eq(UA_String_format(&xml,
        "<UANodeSet><NamespaceUris><Uri>urn:open62541:loader:validation</Uri>"
        "</NamespaceUris>%s<UAObject NodeId=\"ns=1;i=99\" BrowseName=\"1:Kept\">"
        "<References><Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85"
        "</Reference></References></UAObject></UANodeSet>", nodes), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Server_loadNodeset(server, xml), UA_STATUSCODE_GOOD);
    UA_String_clear(&xml);
    size_t nsIndex = 0;
    ck_assert_uint_eq(UA_Server_getNamespaceByName(server,
        UA_STRING("urn:open62541:loader:validation"), &nsIndex), UA_STATUSCODE_GOOD);
    ck_assert_uint_le(nsIndex, UA_UINT16_MAX);
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    ck_assert_uint_eq(UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC((UA_UInt16)nsIndex, 99), &nodeClass), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nodeClass, UA_NODECLASS_OBJECT);
    return (UA_UInt16)nsIndex;
}

START_TEST(Server_rejectWrongTypeDefinitionClasses) {
    UA_UInt16 nsIndex = loadValidationNodes(
        "<UAObject NodeId=\"ns=1;i=1\" BrowseName=\"1:WrongExternalType\">"
        "<References><Reference ReferenceType=\"i=40\">i=63</Reference>"
        "</References></UAObject>"
        "<UAObject NodeId=\"ns=1;i=2\" BrowseName=\"1:WrongLocalType\">"
        "<References><Reference ReferenceType=\"i=40\">ns=1;i=3</Reference>"
        "</References></UAObject>"
        "<UAVariableType NodeId=\"ns=1;i=3\" BrowseName=\"1:VariableType\">"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=63"
        "</Reference></References></UAVariableType>"
        "<UAVariable NodeId=\"ns=1;i=4\" BrowseName=\"1:DefaultType\">"
        "<References><Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85"
        "</Reference></References></UAVariable>");
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    for(UA_UInt32 id = 1; id <= 2; id++)
        ck_assert_uint_eq(UA_Server_readNodeClass(server,
            UA_NODEID_NUMERIC(nsIndex, id), &nodeClass), UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert_uint_eq(UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(nsIndex, 3), &nodeClass), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(nodeClass, UA_NODECLASS_VARIABLETYPE);
    UA_NodeId dataType;
    ck_assert_uint_eq(UA_Server_readDataType(server,
        UA_NODEID_NUMERIC(nsIndex, 4), &dataType), UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&dataType, &UA_TYPES[UA_TYPES_VARIANT].typeId));
    UA_NodeId_clear(&dataType);
}
END_TEST

START_TEST(Server_ignoreMalformedVariableTypeDimensions) {
    UA_UInt16 nsIndex = loadValidationNodes(
        "<UAVariableType NodeId=\"ns=1;i=1\" BrowseName=\"1:MatrixType\" "
        "DataType=\"i=6\" ValueRank=\"2\" ArrayDimensions=\"3,\">"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=63"
        "</Reference></References></UAVariableType>");
    UA_Variant dimensions;
    UA_Variant_init(&dimensions);
    ck_assert_uint_eq(UA_Server_readArrayDimensions(server,
        UA_NODEID_NUMERIC(nsIndex, 1), &dimensions), UA_STATUSCODE_GOOD);
    ck_assert(UA_Variant_hasArrayType(&dimensions, &UA_TYPES[UA_TYPES_UINT32]));
    ck_assert_uint_eq(dimensions.arrayLength, 2);
    ck_assert_uint_eq(((UA_UInt32*)dimensions.data)[0], 0);
    ck_assert_uint_eq(((UA_UInt32*)dimensions.data)[1], 0);
    UA_Variant_clear(&dimensions);
}
END_TEST

START_TEST(Server_rejectUnresolvableDataTypeDefinitions) {
    UA_UInt16 nsIndex = loadValidationNodes(
        "<UADataType NodeId=\"ns=1;i=1\" BrowseName=\"1:MissingFieldType\">"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=22"
        "</Reference></References><Definition Name=\"MissingFieldType\">"
        "<Field Name=\"Missing\" DataType=\"ns=1;i=404\"/></Definition></UADataType>"
        "<UADataType NodeId=\"ns=1;i=2\" BrowseName=\"1:ObjectFieldType\">"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=22"
        "</Reference></References><Definition Name=\"ObjectFieldType\">"
        "<Field Name=\"Object\" DataType=\"ns=1;i=99\"/></Definition></UADataType>"
        "<UADataType NodeId=\"ns=1;i=3\" BrowseName=\"1:RejectedFieldType\">"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=22"
        "</Reference></References><Definition Name=\"RejectedFieldType\">"
        "<Field Name=\"Rejected\" DataType=\"ns=1;i=1\"/></Definition></UADataType>"
        "<UADataType NodeId=\"ns=1;i=4\" BrowseName=\"1:CycleA\">"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=22"
        "</Reference></References><Definition Name=\"CycleA\">"
        "<Field Name=\"B\" DataType=\"ns=1;i=5\"/></Definition></UADataType>"
        "<UADataType NodeId=\"ns=1;i=5\" BrowseName=\"1:CycleB\">"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=22"
        "</Reference></References><Definition Name=\"CycleB\">"
        "<Field Name=\"A\" DataType=\"ns=1;i=4\"/></Definition></UADataType>");
    for(UA_UInt32 i = 1; i <= 5; i++) {
        UA_NodeId id = UA_NODEID_NUMERIC(nsIndex, i);
        ck_assert_ptr_null(UA_Server_findDataType(server, &id));
    }
}
END_TEST

START_TEST(Server_validateDataTypeFieldLimits) {
    const UA_Boolean isEnum = (_i < 2);
    const size_t fieldsSize = UA_DATATYPE_MEMBERS_MAX + (size_t)(_i % 2);
    char nodes[64 * (UA_DATATYPE_MEMBERS_MAX + 1) + 512];
    int written = snprintf(nodes, sizeof(nodes),
        "<UADataType NodeId=\"ns=1;i=1\" BrowseName=\"1:FieldLimit\">"
        "<References><Reference ReferenceType=\"i=45\" IsForward=\"false\">i=%u"
        "</Reference></References><Definition Name=\"FieldLimit\">",
        isEnum ? 29u : 22u);
    if(written <= 0 || (size_t)written >= sizeof(nodes)) {
        ck_abort_msg("Could not format the DataType XML header");
        return;
    }
    size_t offset = (size_t)written;
    for(size_t i = 0; i < fieldsSize; i++) {
        if(isEnum)
            written = snprintf(nodes + offset, sizeof(nodes) - offset,
                               "<Field Name=\"F%u\" Value=\"%u\"/>",
                               (unsigned)i, (unsigned)i);
        else
            written = snprintf(nodes + offset, sizeof(nodes) - offset,
                               "<Field Name=\"F%u\" DataType=\"i=6\"/>", (unsigned)i);
        if(written <= 0 || (size_t)written >= sizeof(nodes) - offset) {
            ck_abort_msg("Could not append a DataType XML field");
            return;
        }
        offset += (size_t)written;
    }
    written = snprintf(nodes + offset, sizeof(nodes) - offset,
                       "</Definition></UADataType>");
    ck_assert_int_gt(written, 0);
    ck_assert_uint_lt((size_t)written, sizeof(nodes) - offset);
    UA_UInt16 nsIndex = loadValidationNodes(nodes);
    UA_NodeId id = UA_NODEID_NUMERIC(nsIndex, 1);
    const UA_DataType *type = UA_Server_findDataType(server, &id);
    if(fieldsSize > UA_DATATYPE_MEMBERS_MAX) {
        ck_assert_ptr_null(type);
    } else {
        ck_assert_ptr_nonnull(type);
        ck_assert_uint_eq(type->typeKind,
                          isEnum ? UA_DATATYPEKIND_ENUM : UA_DATATYPEKIND_STRUCTURE);
        ck_assert_uint_eq(type->membersSize, fieldsSize);
    }
}
END_TEST

START_TEST(Server_rejectNegativeUnsignedInitialValue) {
    const char *negative = _i ? "<ListOfInt32><Int32>42</Int32><Int32>-1</Int32></ListOfInt32>"
                             : "<Int32>-1</Int32>";
    const char *positive = _i ? "<ListOfInt32><Int32>42</Int32><Int32>7</Int32></ListOfInt32>"
                             : "<Int32>42</Int32>";
    char nodes[2048];
    int written = snprintf(nodes, sizeof(nodes),
        "<UAVariable NodeId=\"ns=1;i=1\" BrowseName=\"1:Negative\" "
        "DataType=\"i=7\" ValueRank=\"%d\"><Value>%s</Value><References>"
        "<Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>"
        "<Reference ReferenceType=\"i=40\">i=63</Reference></References></UAVariable>"
        "<UAVariable NodeId=\"ns=1;i=2\" BrowseName=\"1:Positive\" "
        "DataType=\"i=7\" ValueRank=\"%d\"><Value>%s</Value><References>"
        "<Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>"
        "<Reference ReferenceType=\"i=40\">i=63</Reference></References></UAVariable>",
        _i ? 1 : -1, negative, _i ? 1 : -1, positive);
    ck_assert_int_gt(written, 0);
    ck_assert_uint_lt((size_t)written, sizeof(nodes));
    UA_UInt16 nsIndex = loadValidationNodes(nodes);
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    ck_assert_uint_eq(UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(nsIndex, 1), &nodeClass), UA_STATUSCODE_BADNODEIDUNKNOWN);
    UA_Variant value;
    UA_Variant_init(&value);
    ck_assert_uint_eq(UA_Server_readValue(server,
        UA_NODEID_NUMERIC(nsIndex, 2), &value), UA_STATUSCODE_GOOD);
    if(_i) {
        ck_assert(UA_Variant_hasArrayType(&value, &UA_TYPES[UA_TYPES_UINT32]));
        ck_assert_uint_eq(value.arrayLength, 2);
        ck_assert_uint_eq(((UA_UInt32*)value.data)[1], 7);
    } else {
        ck_assert(UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32]));
    }
    ck_assert_uint_eq(*(UA_UInt32*)value.data, 42);
    UA_Variant_clear(&value);
}
END_TEST

START_TEST(Server_rejectMalformedInitialValue) {
    /* A malformed supplied value must not become a default value, even when
     * defaults are allowed for variables that omit their initial value. */
    UA_Server_getConfig(server)->allowEmptyVariables = UA_RULEHANDLING_DEFAULT;
    static const char *const nodes[] = {
        ("<UAVariable NodeId=\"ns=1;i=1\" BrowseName=\"1:InvalidValue\" DataType=\"i=6\">"
         "<Value><Int32>12x</Int32></Value><References>"
         "<Reference ReferenceType=\"i=35\" IsForward=\"false\">i=85</Reference>"
         "<Reference ReferenceType=\"i=40\">i=63</Reference>"
         "</References></UAVariable>"),
        ("<UAVariableType NodeId=\"ns=1;i=1\" BrowseName=\"1:InvalidValue\" DataType=\"i=6\">"
         "<Value><Int32>12x</Int32></Value><References>"
         "<Reference ReferenceType=\"i=45\" IsForward=\"false\">i=63</Reference>"
         "</References></UAVariableType>")
    };
    UA_UInt16 nsIndex = loadValidationNodes(nodes[_i]);
    UA_NodeClass nodeClass = UA_NODECLASS_UNSPECIFIED;
    ck_assert_uint_eq(UA_Server_readNodeClass(server,
        UA_NODEID_NUMERIC(nsIndex, 1), &nodeClass), UA_STATUSCODE_BADNODEIDUNKNOWN);
}
END_TEST

static Suite *
testSuite_Client(void) {
    Suite *s = suite_create("Server Nodeset Loader");
    TCase *tc_server = tcase_create("Server load input nodesets");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Server_loadInputNodesets);
    tcase_add_test(tc_server, Server_loadXmlElement);
    tcase_add_test(tc_server, Server_loadNamespaceMapping);
    tcase_add_test(tc_server, Server_loadForwardParentReferences);
    tcase_add_test(tc_server, Server_loadCustomHierarchicalParentReference);
    tcase_add_test(tc_server, Server_rejectInvalidXmlElement);
    tcase_add_test(tc_server, Server_ignoreMalformedArrayDimensions);
    tcase_add_test(tc_server, Server_loadMissingNodeAttributes);
    tcase_add_test(tc_server, Server_loadLegacyChildAttributes);
    suite_add_tcase(s, tc_server);
    TCase *tc_validation = tcase_create("Import validation");
    tcase_add_checked_fixture(tc_validation, setupImportValidation, teardownImportValidation);
    tcase_add_test(tc_validation, Server_rejectWrongTypeDefinitionClasses);
    tcase_add_test(tc_validation, Server_ignoreMalformedVariableTypeDimensions);
    tcase_add_test(tc_validation, Server_rejectUnresolvableDataTypeDefinitions);
    tcase_add_loop_test(tc_validation, Server_validateDataTypeFieldLimits, 0, 4);
    tcase_add_loop_test(tc_validation, Server_rejectNegativeUnsignedInitialValue, 0, 2);
    tcase_add_loop_test(tc_validation, Server_rejectMalformedInitialValue, 0, 2);
    suite_add_tcase(s, tc_validation);
    return s;
}

int
main(int argc, char *argv[]) {
    if(argc < 2) {
        nodesetPaths = (char **)malloc(sizeof(char *));
        nodesetPaths[0] = OPEN62541_NODESET_DIR "DI/Opc.Ua.Di.NodeSet2.xml";
        nodesetsNum = 1;
    } else {
        nodesetPaths = &argv[1];
        nodesetsNum = argc - 1;
    }
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
