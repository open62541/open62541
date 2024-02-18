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
    UA_String ex = UA_STRING("");
    UA_String exout = UA_STRING_NULL;
    UA_StatusCode res = UA_RelativePath_parse(&rp, ex);
    res |= UA_RelativePath_print(&rp, &exout);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 0);
    ck_assert(UA_String_equal(&ex, &exout));
    UA_String_clear(&exout);

    UA_String ex1 = UA_STRING("/2:Block&.Output");
    UA_String ex1out = UA_STRING_NULL;
    res = UA_RelativePath_parse(&rp, ex1);
    res |= UA_RelativePath_print(&rp, &ex1out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 1);
    UA_RelativePath_clear(&rp);
    ck_assert(UA_String_equal(&ex1, &ex1out));
    UA_String_clear(&ex1out);

    /* Paths with no BrowseName */
    UA_String ex2 = UA_STRING("//");
    UA_String ex2out = UA_STRING_NULL;
    res = UA_RelativePath_parse(&rp, ex2);
    res |= UA_RelativePath_print(&rp, &ex2out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    UA_RelativePath_clear(&rp);
    ck_assert(UA_String_equal(&ex2, &ex2out));
    UA_String_clear(&ex2out);

    UA_String ex3 = UA_STRING("/.");
    UA_String ex3out = UA_STRING_NULL;
    res = UA_RelativePath_parse(&rp, ex3);
    res |= UA_RelativePath_print(&rp, &ex3out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    UA_RelativePath_clear(&rp);
    ck_assert(UA_String_equal(&ex3, &ex3out));
    UA_String_clear(&ex3out);

    UA_String ex4 = UA_STRING("<HierachicalReferences>2:Wheel");
    UA_String ex4out = UA_STRING_NULL;
    res = UA_RelativePath_parse(&rp, ex4);
    res |= UA_RelativePath_print(&rp, &ex4out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 1);
    ck_assert_int_eq(rp.elements[0].targetName.namespaceIndex, 2);
    UA_RelativePath_clear(&rp);
    //ck_assert(UA_String_equal(&ex4, &ex4out)); // <HierachicalReferences> -> /
    UA_String_clear(&ex4out);

    UA_String ex5 = UA_STRING("<HasComponent>1:Boiler/1:HeatSensor");
    UA_String ex5out = UA_STRING_NULL;
    res = UA_RelativePath_parse(&rp, ex5);
    res |= UA_RelativePath_print(&rp, &ex5out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    ck_assert_int_eq(rp.elements[0].targetName.namespaceIndex, 1);
    ck_assert_int_eq(rp.elements[1].targetName.namespaceIndex, 1);
    UA_RelativePath_clear(&rp);
    ck_assert(UA_String_equal(&ex5, &ex5out));
    UA_String_clear(&ex5out);

    UA_String ex6 = UA_STRING(".1:Boiler/1:HeatSensor/");
    UA_String ex6out = UA_STRING_NULL;
    res = UA_RelativePath_parse(&rp, ex6);
    res |= UA_RelativePath_print(&rp, &ex6out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 3);
    ck_assert_int_eq(rp.elements[0].targetName.namespaceIndex, 1);
    ck_assert_int_eq(rp.elements[1].targetName.namespaceIndex, 1);
    UA_String tmp = UA_STRING("HeatSensor");
    ck_assert(UA_String_equal(&tmp, &rp.elements[1].targetName.name));
    ck_assert_int_eq(rp.elements[2].targetName.namespaceIndex, 0);
    ck_assert(UA_String_equal(&ex6, &ex6out));
    UA_RelativePath_clear(&rp);
    UA_String_clear(&ex6out);

    UA_String ex7 = UA_STRING("<!HasChild>Truck");
    UA_String ex7out = UA_STRING_NULL;
    res = UA_RelativePath_parse(&rp, ex7);
    res |= UA_RelativePath_print(&rp, &ex7out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 1);
    ck_assert_int_eq(rp.elements[0].isInverse, true);
    UA_RelativePath_clear(&rp);
    ck_assert(UA_String_equal(&ex7, &ex7out));
    UA_String_clear(&ex7out);

    UA_String ex8 = UA_STRING("<HasChild>");
    UA_String ex8out = UA_STRING_NULL;
    res = UA_RelativePath_parse(&rp, ex8);
    res |= UA_RelativePath_print(&rp, &ex8out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 1);
    UA_RelativePath_clear(&rp);
    ck_assert(UA_String_equal(&ex8, &ex8out));
    UA_String_clear(&ex8out);

    UA_String ex9 = UA_STRING("/1:Boiler<ns=1;i=345>1:HeatSensor");
    UA_String ex9out = UA_STRING_NULL;
    UA_NodeId testRef = UA_NODEID_NUMERIC(1, 345);
    res = UA_RelativePath_parse(&rp, ex9);
    res |= UA_RelativePath_print(&rp, &ex9out);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(rp.elementsSize, 2);
    ck_assert(UA_NodeId_equal(&rp.elements[1].referenceTypeId, &testRef));
    UA_RelativePath_clear(&rp);
    ck_assert(UA_String_equal(&ex9, &ex9out));
    UA_String_clear(&ex9out);
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

START_TEST(parseSimpleAttributeOperand) {
    UA_String sao1_str = UA_STRING("");
    UA_SimpleAttributeOperand sao1;
    UA_StatusCode res = UA_SimpleAttributeOperand_parse(&sao1, sao1_str);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_String sao2_str = UA_STRING("#Value");
    UA_SimpleAttributeOperand sao2;
    res = UA_SimpleAttributeOperand_parse(&sao2, sao2_str);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_String sao3_str = UA_STRING("[1:2]");
    UA_SimpleAttributeOperand sao3;
    res = UA_SimpleAttributeOperand_parse(&sao3, sao3_str);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_SimpleAttributeOperand_clear(&sao3);

    UA_String sao4_str = UA_STRING("ns=1;s=1&&23/1:& Boiler/Temperature#BrowseName[0:5]");
    UA_SimpleAttributeOperand sao4;
    UA_String cmp1 = UA_STRING("1&23");
    UA_String cmp2 = UA_STRING(" Boiler");
    res = UA_SimpleAttributeOperand_parse(&sao4, sao4_str);
    ck_assert(UA_String_equal(&sao4.typeDefinitionId.identifier.string, &cmp1));
    ck_assert(UA_String_equal(&sao4.browsePath[0].name, &cmp2));
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_SimpleAttributeOperand_clear(&sao4);

    UA_String sao5_str = UA_STRING("///");
    UA_SimpleAttributeOperand sao5;
    res = UA_SimpleAttributeOperand_parse(&sao5, sao5_str);
    ck_assert(sao5.browsePathSize == 3);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_SimpleAttributeOperand_clear(&sao5);
} END_TEST

START_TEST(printSimpleAttributeOperand) {
    UA_QualifiedName browsePath[2];
    browsePath[0] = UA_QUALIFIEDNAME(1, "Boiler");
    browsePath[1] = UA_QUALIFIEDNAME(0, "Temperature");

    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    sao.typeDefinitionId = UA_NODEID("ns=1;i=123");
    sao.browsePath = browsePath;
    sao.browsePathSize = 2;
    sao.attributeId = UA_ATTRIBUTEID_BROWSENAME;
    sao.indexRange = UA_STRING("0:5");

    UA_String encoding = UA_STRING_NULL;
    UA_SimpleAttributeOperand_print(&sao, &encoding);
    UA_String expected = UA_STRING("ns=1;i=123/1:Boiler/Temperature#BrowseName[0:5]");
    ck_assert(UA_String_equal(&encoding, &expected));
    UA_String_clear(&encoding);

    UA_SimpleAttributeOperand sao2;
    UA_StatusCode res = UA_SimpleAttributeOperand_parse(&sao2, expected);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_SimpleAttributeOperand_equal(&sao, &sao2));
    UA_SimpleAttributeOperand_clear(&sao2);
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
    tcase_add_test(tc, parseSimpleAttributeOperand);
    tcase_add_test(tc, printSimpleAttributeOperand);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
