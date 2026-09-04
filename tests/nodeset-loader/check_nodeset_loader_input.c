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
    suite_add_tcase(s, tc_server);
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
