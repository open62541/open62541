/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated_handling.h>
#include "test_helpers.h"
#include "open62541/util.h"

#include <stdlib.h>
#include <check.h>

START_TEST(base64) {
    UA_ByteString test1 = UA_BYTESTRING("abc123\nopen62541");
    UA_String test1base64;
    UA_StatusCode res = UA_ByteString_toBase64(&test1, &test1base64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_ByteString test1out;
    res = UA_ByteString_fromBase64(&test1out, &test1base64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(test1.length, test1out.length);
    for(size_t i = 0; i < test1.length; i++)
        ck_assert_int_eq(test1.data[i], test1out.data[i]);

    UA_String_clear(&test1base64);
    UA_ByteString_clear(&test1out);
    UA_ByteString_clear(&test1out);

    UA_ByteString test2 = UA_BYTESTRING("");
    UA_String test2base64;
    res = UA_ByteString_toBase64(&test2, &test2base64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_ByteString test2out;
    res = UA_ByteString_fromBase64(&test2out, &test2base64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(test2.length, test2out.length);
    for(size_t i = 0; i < test2.length; i++)
        ck_assert_int_eq(test2.data[i], test2out.data[i]);

    UA_ByteString_clear(&test2out);
} END_TEST

/* Example taken from Part 6, 5.2.2.6 */
START_TEST(parseGuid) {
    UA_Guid guid = UA_GUID("72962B91-FA75-4AE6-8D28-B404DC7DAF63");
    ck_assert_int_eq((UA_Byte)guid.data1, 0x91);
    ck_assert_int_eq((UA_Byte)guid.data2, 0x75);
    ck_assert_int_eq((UA_Byte)guid.data3, 0xe6);
    ck_assert_int_eq(guid.data4[0], 0x8d);
    ck_assert_int_eq(guid.data4[1], 0x28);
    ck_assert_int_eq(guid.data4[2], 0xb4);
    ck_assert_int_eq(guid.data4[3], 0x04);
    ck_assert_int_eq(guid.data4[4], 0xdc);
    ck_assert_int_eq(guid.data4[5], 0x7d);
    ck_assert_int_eq(guid.data4[6], 0xaf);
    ck_assert_int_eq(guid.data4[7], 0x63);

#ifdef UA_ENABLE_PARSING
    /* Encoding decoding roundtrip */
    UA_String encoded = UA_STRING_NULL;
    UA_Guid_print(&guid, &encoded);
    UA_Guid guid2;
    UA_Guid_parse(&guid2, encoded);
    ck_assert(UA_Guid_equal(&guid, &guid2));
    UA_String_clear(&encoded);
#endif
} END_TEST

START_TEST(parseNodeIdNumeric) {
    UA_NodeId id = UA_NODEID("i=13");
    ck_assert_int_eq(id.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(id.identifier.numeric, 13);
    ck_assert_int_eq(id.namespaceIndex, 0);
} END_TEST

START_TEST(parseNodeIdNumeric2) {
    UA_NodeId id = UA_NODEID("ns=10;i=1");
    ck_assert_int_eq(id.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(id.identifier.numeric, 1);
    ck_assert_int_eq(id.namespaceIndex, 10);
} END_TEST

START_TEST(parseNodeIdString) {
    UA_NodeId id = UA_NODEID("ns=10;s=Hello:World");
    ck_assert_int_eq(id.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_int_eq(id.namespaceIndex, 10);
    UA_String strid = UA_STRING("Hello:World");
    ck_assert(UA_String_equal(&id.identifier.string, &strid));
    UA_NodeId_clear(&id);
} END_TEST

START_TEST(parseNodeIdGuid) {
    UA_NodeId id = UA_NODEID("g=09087e75-8e5e-499b-954f-f2a9603db28a");
    ck_assert_int_eq(id.identifierType, UA_NODEIDTYPE_GUID);
    ck_assert_int_eq(id.namespaceIndex, 0);
    ck_assert_int_eq(id.identifier.guid.data1, 151551605);
} END_TEST

START_TEST(parseNodeIdGuidFail) {
    UA_NodeId id = UA_NODEID("g=09087e75=8e5e-499b-954f-f2a9603db28a");
    ck_assert_int_eq(id.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(id.identifier.numeric, 0);
    ck_assert_int_eq(id.namespaceIndex, 0);
} END_TEST

START_TEST(parseNodeIdByteString) {
    UA_NodeId id = UA_NODEID("ns=1;b=b3BlbjYyNTQxIQ==");
    ck_assert_int_eq(id.identifierType, UA_NODEIDTYPE_BYTESTRING);
    ck_assert_int_eq(id.namespaceIndex, 1);
    UA_ByteString bstrid = UA_BYTESTRING("open62541!");
    ck_assert(UA_ByteString_equal(&id.identifier.byteString, &bstrid));
    UA_NodeId_clear(&id);
} END_TEST

START_TEST(parseExpandedNodeIdInteger) {
    UA_ExpandedNodeId id = UA_EXPANDEDNODEID("ns=1;i=1337");
    ck_assert_int_eq(id.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(id.nodeId.identifier.numeric, 1337);
    ck_assert_int_eq(id.nodeId.namespaceIndex, 1);
} END_TEST

START_TEST(parseExpandedNodeIdInteger2) {
    UA_ExpandedNodeId id = UA_EXPANDEDNODEID("svr=5;ns=1;i=1337");
    ck_assert_int_eq(id.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(id.nodeId.identifier.numeric, 1337);
    ck_assert_int_eq(id.nodeId.namespaceIndex, 1);
    ck_assert_int_eq(id.serverIndex, 5);
} END_TEST

START_TEST(parseExpandedNodeIdIntegerNSU) {
    UA_ExpandedNodeId id = UA_EXPANDEDNODEID("svr=5;nsu=urn:test:1234;i=1337");
    ck_assert_int_eq(id.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(id.nodeId.identifier.numeric, 1337);
    UA_String nsu = UA_STRING("urn:test:1234");
    ck_assert(UA_String_equal(&id.namespaceUri, &nsu));
    ck_assert_int_eq(id.serverIndex, 5);
    UA_ExpandedNodeId_clear(&id);
} END_TEST

START_TEST(parseExpandedNodeIdIntegerFailNSU) {
    UA_ExpandedNodeId id = UA_EXPANDEDNODEID("svr=5;nsu=urn:test:1234;;i=1337");
    ck_assert_int_eq(id.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(id.nodeId.identifier.numeric, 0);
} END_TEST

START_TEST(parseExpandedNodeIdIntegerFailNSU2) {
    UA_ExpandedNodeId id = UA_EXPANDEDNODEID("svr=5;nsu=urn:test:1234;ns=1;i=1337");
    ck_assert_int_eq(id.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(id.nodeId.identifier.numeric, 0);
} END_TEST

START_TEST(parseRelativePath) {
    UA_RelativePath rp;
    UA_StatusCode res = UA_RelativePath_parse(&rp, UA_STRING(""));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 0);

    res = UA_RelativePath_parse(&rp, UA_STRING("/2:Block&.Output"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 1);
    UA_RelativePath_clear(&rp);

    /* Paths with no BrowseName */
    res = UA_RelativePath_parse(&rp, UA_STRING("//"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    UA_RelativePath_clear(&rp);

    res = UA_RelativePath_parse(&rp, UA_STRING("/."));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    UA_RelativePath_clear(&rp);

    res = UA_RelativePath_parse(&rp, UA_STRING("<0:HierachicalReferences>2:Wheel"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 1);
    ck_assert_int_eq(rp.elements[0].targetName.namespaceIndex, 2);
    UA_RelativePath_clear(&rp);

    res = UA_RelativePath_parse(&rp, UA_STRING("<0:HasComponent>1:Boiler/1:HeatSensor"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    ck_assert_int_eq(rp.elements[0].targetName.namespaceIndex, 1);
    ck_assert_int_eq(rp.elements[1].targetName.namespaceIndex, 1);
    UA_RelativePath_clear(&rp);

    res = UA_RelativePath_parse(&rp, UA_STRING(".1:Boiler/1:HeatSensor/"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 3);
    ck_assert_int_eq(rp.elements[0].targetName.namespaceIndex, 1);
    ck_assert_int_eq(rp.elements[1].targetName.namespaceIndex, 1);
    UA_String tmp = UA_STRING("HeatSensor");
    ck_assert(UA_String_equal(&tmp, &rp.elements[1].targetName.name));
    ck_assert_int_eq(rp.elements[2].targetName.namespaceIndex, 0);
    UA_RelativePath_clear(&rp);

    res = UA_RelativePath_parse(&rp, UA_STRING("<!HasChild>Truck"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 1);
    ck_assert_int_eq(rp.elements[0].isInverse, true);
    UA_RelativePath_clear(&rp);

    res = UA_RelativePath_parse(&rp, UA_STRING("<0:HasChild>"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 1);
    UA_RelativePath_clear(&rp);

    UA_NodeId testRef = UA_NODEID_NUMERIC(1, 345);
    res = UA_RelativePath_parse(&rp, UA_STRING("/1:Boiler<ns=1;i=345>1:HeatSensor"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    ck_assert(UA_NodeId_equal(&rp.elements[1].referenceTypeId, &testRef));
    UA_RelativePath_clear(&rp);
} END_TEST

START_TEST(parseRelativePathWithServer) {
    UA_Server *server = UA_Server_newForUnitTest();

    /* Add a custom non-hierarchical reference type */
	UA_NodeId refTypeId;
	UA_ReferenceTypeAttributes refattr = UA_ReferenceTypeAttributes_default;
	refattr.displayName = UA_LOCALIZEDTEXT(NULL, "MyRef");
	refattr.inverseName = UA_LOCALIZEDTEXT(NULL, "RefMy");
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "MyRef");
	UA_StatusCode res =
        UA_Server_addReferenceTypeNode(server, UA_NODEID_NULL,
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_NONHIERARCHICALREFERENCES),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                       browseName, refattr, NULL, &refTypeId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Use the browse name in the path string. Expect the NodeId of the reference type */
    UA_RelativePath rp;
    res = UA_RelativePath_parseWithServer(server, &rp,
                                          UA_STRING("/1:Boiler<1:MyRef>1:HeatSensor"));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    ck_assert(UA_NodeId_equal(&rp.elements[1].referenceTypeId, &refTypeId));
    UA_RelativePath_clear(&rp);

    UA_Server_delete(server);
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test Builtin Type Parsing");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, base64);
    tcase_add_test(tc, parseGuid);
    tcase_add_test(tc, parseNodeIdNumeric);
    tcase_add_test(tc, parseNodeIdNumeric2);
    tcase_add_test(tc, parseNodeIdString);
    tcase_add_test(tc, parseNodeIdGuid);
    tcase_add_test(tc, parseNodeIdGuidFail);
    tcase_add_test(tc, parseNodeIdByteString);
    tcase_add_test(tc, parseExpandedNodeIdInteger);
    tcase_add_test(tc, parseExpandedNodeIdInteger2);
    tcase_add_test(tc, parseExpandedNodeIdIntegerNSU);
    tcase_add_test(tc, parseExpandedNodeIdIntegerFailNSU);
    tcase_add_test(tc, parseExpandedNodeIdIntegerFailNSU2);
    tcase_add_test(tc, parseRelativePath);
    tcase_add_test(tc, parseRelativePathWithServer);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
