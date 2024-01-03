/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>

#include "util/ua_util_internal.h"

#include <stdlib.h>

#include "check.h"

START_TEST(EndpointUrl_split) {
    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_UInt16 port = 0;

    // check for too short url
    UA_String endPointUrl = UA_STRING("inv.ali:/");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path),
                      UA_STATUSCODE_BADTCPENDPOINTURLINVALID);

    // check for opc.tcp:// protocol
    endPointUrl = UA_STRING("inv.ali://");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path),
                      UA_STATUSCODE_BADTCPENDPOINTURLINVALID);

    // empty url
    endPointUrl = UA_STRING("opc.tcp://");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path),
                      UA_STATUSCODE_BADTCPENDPOINTURLINVALID);
    ck_assert(UA_String_equal(&hostname, &UA_STRING_NULL));
    ck_assert_uint_eq(port, 0);
    ck_assert(UA_String_equal(&path, &UA_STRING_NULL));

    // only hostname
    endPointUrl = UA_STRING("opc.tcp://hostname");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path), UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("hostname");
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 0);
    ck_assert(UA_String_equal(&path, &UA_STRING_NULL));

    // empty port
    endPointUrl = UA_STRING("opc.tcp://hostname:");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path),
                      UA_STATUSCODE_BADTCPENDPOINTURLINVALID);
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 0);
    ck_assert(UA_String_equal(&path, &UA_STRING_NULL));

    // specific port
    endPointUrl = UA_STRING("opc.tcp://hostname:1234");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 1234);
    ck_assert(UA_String_equal(&path, &UA_STRING_NULL));

    // IPv6
    endPointUrl = UA_STRING("opc.tcp://[2001:0db8:85a3::8a2e:0370:7334]:1234/path");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path), UA_STATUSCODE_GOOD);
    expected = UA_STRING("2001:0db8:85a3::8a2e:0370:7334");
    UA_String expectedPath = UA_STRING("path");
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 1234);
    ck_assert(UA_String_equal(&path, &expectedPath));

    // invalid IPv6: missing ]
    endPointUrl = UA_STRING("opc.tcp://[2001:0db8:85a3::8a2e:0370:7334");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path), UA_STATUSCODE_BADTCPENDPOINTURLINVALID);

    // empty hostname
    endPointUrl = UA_STRING("opc.tcp://:");
    port = 0;
    path = UA_STRING_NULL;
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path),
                      UA_STATUSCODE_BADTCPENDPOINTURLINVALID);
    ck_assert(UA_String_equal(&hostname, &UA_STRING_NULL));
    ck_assert_uint_eq(port, 0);
    ck_assert(UA_String_equal(&path, &UA_STRING_NULL));

    // empty hostname and no port
    endPointUrl = UA_STRING("opc.tcp:///");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path), UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&hostname, &UA_STRING_NULL));
    ck_assert_uint_eq(port, 0);
    ck_assert(UA_String_equal(&path, &UA_STRING_NULL));

    // overlength port
    endPointUrl = UA_STRING("opc.tcp://hostname:12345678");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path),
                      UA_STATUSCODE_BADTCPENDPOINTURLINVALID);

    // port not a number
    endPointUrl = UA_STRING("opc.tcp://hostname:6x6");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path),
                      UA_STATUSCODE_BADTCPENDPOINTURLINVALID);
    expected = UA_STRING("hostname");
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 0);
    ck_assert(UA_String_equal(&path, &UA_STRING_NULL));

    // no port but path
    endPointUrl = UA_STRING("opc.tcp://hostname/");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path), UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 0);
    ck_assert(UA_String_equal(&path, &UA_STRING_NULL));

    // port and path
    endPointUrl = UA_STRING("opc.tcp://hostname:1234/path");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path), UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 1234);
    ck_assert(UA_String_equal(&path, &expectedPath));

    // port and path with a slash
    endPointUrl = UA_STRING("opc.tcp://hostname:1234/path/");
    ck_assert_uint_eq(UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path), UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 1234);
    ck_assert(UA_String_equal(&path, &expectedPath));
}
END_TEST

START_TEST(EndpointUrl_ethernet) {
    UA_String target;
    UA_UInt16 vid = 0;
    UA_Byte pcp = 0;
    UA_String expected;

    // check for too short url
    UA_String endPointUrl = UA_STRING("inv.ali:/");
    ck_assert_uint_eq(UA_parseEndpointUrlEthernet(&endPointUrl, &target, &vid, &pcp),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(vid, 0);
    ck_assert_uint_eq(pcp, 0);

    // long enough, but malformed
    endPointUrl = UA_STRING("opc.eth.//target:");
    ck_assert_uint_eq(UA_parseEndpointUrlEthernet(&endPointUrl, &target, &vid, &pcp),
                      UA_STATUSCODE_BADINTERNALERROR);

    // valid without vid and pcp but leading ':'
    endPointUrl = UA_STRING("opc.eth://target:");
    ck_assert_uint_eq(UA_parseEndpointUrlEthernet(&endPointUrl, &target, &vid, &pcp),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(vid, 0);
    ck_assert_uint_eq(pcp, 0);

    // without pcp and vid as non decimal
    endPointUrl = UA_STRING("opc.eth://target:abc");
    ck_assert_uint_eq(UA_parseEndpointUrlEthernet(&endPointUrl, &target, &vid, &pcp),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(vid, 0);
    ck_assert_uint_eq(pcp, 0);

    // pcp as non decimal
    endPointUrl = UA_STRING("opc.eth://target:100.abc");
    ck_assert_uint_eq(UA_parseEndpointUrlEthernet(&endPointUrl, &target, &vid, &pcp),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(vid, 100);
    ck_assert_uint_eq(pcp, 0);

    // valid without pcp but leading '.'
    endPointUrl = UA_STRING("opc.eth://target:100.");
    ck_assert_uint_eq(UA_parseEndpointUrlEthernet(&endPointUrl, &target, &vid, &pcp),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(vid, 100);
    ck_assert_uint_eq(pcp, 0);

    // valid without pcp
    endPointUrl = UA_STRING("opc.eth://target:100");
    ck_assert_uint_eq(UA_parseEndpointUrlEthernet(&endPointUrl, &target, &vid, &pcp),
                      UA_STATUSCODE_GOOD);
    expected = UA_STRING("target");
    ck_assert(UA_String_equal(&target, &expected));
    ck_assert_uint_eq(vid, 100);
    ck_assert_uint_eq(pcp, 0);

    // valid
    endPointUrl = UA_STRING("opc.eth://target:100.7");
    ck_assert_uint_eq(UA_parseEndpointUrlEthernet(&endPointUrl, &target, &vid, &pcp),
                      UA_STATUSCODE_GOOD);
    expected = UA_STRING("target");
    ck_assert(UA_String_equal(&target, &expected));
    ck_assert_uint_eq(vid, 100);
    ck_assert_uint_eq(pcp, 7);
}
END_TEST

START_TEST(readNumber) {
    UA_UInt32 result;
    ck_assert_uint_eq(UA_readNumber((UA_Byte*)"x", 1, &result), 0);

    ck_assert_uint_eq(UA_readNumber((UA_Byte*)"1x", 2, &result), 1);
    ck_assert_uint_eq(result, 1);

    ck_assert_uint_eq(UA_readNumber((UA_Byte*)"123456789", 9, &result), 9);
    ck_assert_uint_eq(result, 123456789);
}
END_TEST


START_TEST(stringCompare) {

    UA_String sa1 = UA_String_fromChars("A");
    UA_String sa2 = UA_String_fromChars("a");

    UA_String s1 = UA_String_fromChars("SomeLongString");
    UA_String s2 = UA_String_fromChars("SomeLongString");
    UA_String s3 = UA_String_fromChars("somelongstring");
    UA_String s4 = UA_String_fromChars("somelongstring ");

    // same string
    ck_assert(UA_String_equal(&sa1, &sa1));
    ck_assert(UA_String_equal_ignorecase(&sa1, &sa1));

    // case sensitive
    ck_assert(!UA_String_equal(&sa1, &sa2));
    // case insensitive
    ck_assert(UA_String_equal_ignorecase(&sa1, &sa2));


    // same string
    ck_assert(UA_String_equal(&s1, &s2));
    ck_assert(UA_String_equal_ignorecase(&s1, &s2));

    // case sensitive
    ck_assert(!UA_String_equal(&s1, &s3));
    // case insensitive
    ck_assert(UA_String_equal_ignorecase(&s1, &s3));

    // different length
    ck_assert(!UA_String_equal(&s3, &s4));
    ck_assert(!UA_String_equal_ignorecase(&s3, &s4));

        UA_String_clear(&sa1);
        UA_String_clear(&sa2);
        UA_String_clear(&s1);
        UA_String_clear(&s2);
        UA_String_clear(&s3);
        UA_String_clear(&s4);
}
END_TEST

START_TEST(readNumberWithBase) {
    UA_UInt32 result;
    ck_assert_uint_eq(UA_readNumberWithBase((UA_Byte*)"g", 1, &result, 16), 0);

    ck_assert_uint_eq(UA_readNumberWithBase((UA_Byte*)"f", 1, &result, 16), 1);
    ck_assert_uint_eq(result, 15);

    ck_assert_uint_eq(UA_readNumberWithBase((UA_Byte*)"F", 1, &result, 16), 1);
    ck_assert_uint_eq(result, 15);

    ck_assert_uint_eq(UA_readNumberWithBase((UA_Byte*)"1x", 2, &result, 16), 1);
    ck_assert_uint_eq(result, 1);

    ck_assert_uint_eq(UA_readNumberWithBase((UA_Byte*)"12345678", 9, &result, 16), 8);
    ck_assert_uint_eq(result, 0x12345678);

    ck_assert_uint_eq(UA_readNumberWithBase((UA_Byte*)"123456789", 9, &result, 8), 7);
    ck_assert_uint_eq(result, 01234567);
}
END_TEST


START_TEST(StatusCode_msg) {
#ifndef UA_ENABLE_STATUSCODE_DESCRIPTIONS
    return;
#endif
        // first element in table
    ck_assert_str_eq(UA_StatusCode_name(UA_STATUSCODE_GOOD), "Good");

        // just some randomly picked status codes
    ck_assert_str_eq(UA_StatusCode_name(UA_STATUSCODE_BADNOCOMMUNICATION),
                     "BadNoCommunication");

    ck_assert_str_eq(UA_StatusCode_name(UA_STATUSCODE_GOODNODATA), "GoodNoData");

        // last element in table
    ck_assert_str_eq(UA_StatusCode_name(UA_STATUSCODE_BADMAXCONNECTIONSREACHED),
                     "BadMaxConnectionsReached");

        // an invalid status code
    ck_assert_str_eq(UA_StatusCode_name(0x70123456), "Unknown StatusCode");
}
END_TEST


static void assertNodeIdString(const UA_String *gotStr, const char* expectedStr) {
    size_t expectedStringLength = strlen(expectedStr);
    ck_assert_uint_eq(gotStr->length, expectedStringLength);
    char *gotChars = (char*)UA_malloc(gotStr->length+1);
    memcpy(gotChars, gotStr->data, gotStr->length);
    gotChars[gotStr->length] = 0;
    ck_assert_str_eq(gotChars, expectedStr);
    UA_free(gotChars);
}

START_TEST(idToStringNumeric) {
    UA_NodeId n;
    UA_String str = UA_STRING_NULL;

    n = UA_NODEID_NUMERIC(0,0);
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "i=0");
    UA_String_clear(&str);

    n = UA_NODEID_NUMERIC(12345,1234567890);
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=12345;i=1234567890");
    UA_String_clear(&str);

    n = UA_NODEID_NUMERIC(0xFFFF,0xFFFFFFFF);
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=65535;i=4294967295");
    UA_String_clear(&str);
} END_TEST

START_TEST(idToStringString) {
    UA_NodeId n;
    UA_String str = UA_STRING_NULL;

    n = UA_NODEID_STRING(0,"");
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "s=");
    UA_String_clear(&str);

    n = UA_NODEID_STRING(54321,"Some String");
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=54321;s=Some String");
    UA_String_clear(&str);

    n = UA_NODEID_STRING(0,"Some String");
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "s=Some String");
    UA_String_clear(&str);
} END_TEST

START_TEST(idToStringGuid) {
    UA_NodeId n;
    UA_String str = UA_STRING_NULL;

    UA_Guid g = UA_GUID_NULL;

    n = UA_NODEID_GUID(0,UA_GUID_NULL);
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "g=00000000-0000-0000-0000-000000000000");
    UA_String_clear(&str);

    g.data1 = 0xA123456C;
    g.data2 = 0x0ABC;
    g.data3 = 0x1A2B;
    g.data4[0] = 0x81;
    g.data4[1] = 0x5F;
    g.data4[2] = 0x68;
    g.data4[3] = 0x72;
    g.data4[4] = 0x12;
    g.data4[5] = 0xAA;
    g.data4[6] = 0xEE;
    g.data4[7] = 0x1B;

    n = UA_NODEID_GUID(65535,g);
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=65535;g=a123456c-0abc-1a2b-815f-687212aaee1b");
    UA_String_clear(&str);

    g.data1 = 0xFFFFFFFF;
    g.data2 = 0xFFFF;
    g.data3 = 0xFFFF;
    g.data4[0] = 0xFF;
    g.data4[1] = 0xFF;
    g.data4[2] = 0xFF;
    g.data4[3] = 0xFF;
    g.data4[4] = 0xFF;
    g.data4[5] = 0xFF;
    g.data4[6] = 0xFF;
    g.data4[7] = 0xFF;

    n = UA_NODEID_GUID(65535,g);
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=65535;g=ffffffff-ffff-ffff-ffff-ffffffffffff");
    UA_String_clear(&str);
} END_TEST

START_TEST(idToStringByte) {
    UA_NodeId n;
    UA_String str = UA_STRING_NULL;

    n.namespaceIndex = 0;
    n.identifierType = UA_NODEIDTYPE_BYTESTRING;
    n.identifier.byteString.data = NULL;
    n.identifier.byteString.length = 0;
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "b=");
    UA_String_clear(&str);

    UA_ByteString bs = UA_BYTESTRING_NULL;

    bs.length = 1;
    bs.data = (UA_Byte*)UA_malloc(bs.length);
    bs.data[0] = 0x00;
    n.identifier.byteString = bs;
    n.namespaceIndex = 123;
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=123;b=AA==");
    UA_free(bs.data);
    UA_String_clear(&str);

    bs.length = 1;
    bs.data = (UA_Byte*)UA_malloc(bs.length);
    bs.data[0] = 0x2C;
    n.identifier.byteString = bs;
    n.namespaceIndex = 123;
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=123;b=LA==");
    UA_free(bs.data);
    UA_String_clear(&str);

    bs.length = 5;
    bs.data = (UA_Byte*)UA_malloc(bs.length);
    bs.data[0] = 0x21;
    bs.data[1] = 0x83;
    bs.data[2] = 0xE0;
    bs.data[3] = 0x54;
    bs.data[4] = 0x78;
    n.identifier.byteString = bs;
    n.namespaceIndex = 599;
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=599;b=IYPgVHg=");
    UA_free(bs.data);
    UA_String_clear(&str);
} END_TEST


START_TEST(idOrderNs) {
    UA_NodeId id_ns1 = UA_NODEID_NUMERIC(1, 12345);
    UA_NodeId id_ns3 = UA_NODEID_NUMERIC(3, 12345);

    ck_assert(UA_NodeId_order(&id_ns1, &id_ns1) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_ns1, &id_ns3) == UA_ORDER_LESS);
    ck_assert(UA_NodeId_order(&id_ns3, &id_ns1) == UA_ORDER_MORE);
} END_TEST

START_TEST(idOrderIdentifier) {
    UA_NodeId id_num = UA_NODEID_NUMERIC(1, 12345);
    UA_NodeId id_str = UA_NODEID_STRING(1, "asdf");

    ck_assert(UA_NodeId_order(&id_num, &id_num) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_num, &id_str) == UA_ORDER_LESS);
    ck_assert(UA_NodeId_order(&id_str, &id_num) == UA_ORDER_MORE);
} END_TEST

START_TEST(idOrderNumeric) {
    UA_NodeId id_num_12345 = UA_NODEID_NUMERIC(1, 12345);
    UA_NodeId id_num_23456 = UA_NODEID_NUMERIC(1, 23456);

    ck_assert(UA_NodeId_order(&id_num_12345, &id_num_12345) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_num_12345, &id_num_23456) == UA_ORDER_LESS);
    ck_assert(UA_NodeId_order(&id_num_23456, &id_num_12345) == UA_ORDER_MORE);
} END_TEST

START_TEST(idOrderGuid) {

    // See also https://github.com/open62541/open62541/pull/2904#issuecomment-514111395

    // 00000000-FFFF-FFFF-FFFFFFFFFFFF,
    UA_Guid guid1 = {
            0,
            0xffff,
            0xffff,
            { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
    };
    // 00000001-0000-0000-000000000000
    UA_Guid guid2 = {
            0x1,
            0,
            0,
            { 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    // 00000000-0000-FFFF-FFFFFFFFFFFF
    UA_Guid guid3 = {
            0,
            0,
            0xffff,
            { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
    };
    // 00000000-0001-0000-000000000000
    UA_Guid guid4 = {
            0,
            0x1,
            0,
            { 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    // 00000000-0000-0000-FFFFFFFFFFFF
    UA_Guid guid5 = {
            0,
            0,
            0,
            { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
    };
    // 00000000-0000-0001-000000000000
    UA_Guid guid6 = {
            0,
            0,
            0x1,
            { 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    // 00000000-0000-0000-000000000000
    UA_Guid guid7 = {
            0,
            0,
            0,
            { 0, 0, 0, 0, 0, 0, 0, 0 }
    };
    // 00000000-0000-0000-000000000001
    UA_Guid guid8 = {
            0,
            0,
            0,
            { 0, 0, 0, 0, 0, 0, 0, 0x1 }
    };


    UA_NodeId id_guid_1 = UA_NODEID_GUID(1, guid1);
    UA_NodeId id_guid_2 = UA_NODEID_GUID(1, guid2);
    ck_assert(UA_NodeId_order(&id_guid_1, &id_guid_1) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_guid_1, &id_guid_2) == UA_ORDER_LESS);
    ck_assert(UA_NodeId_order(&id_guid_2, &id_guid_1) == UA_ORDER_MORE);

    UA_NodeId id_guid_3 = UA_NODEID_GUID(1, guid3);
    UA_NodeId id_guid_4 = UA_NODEID_GUID(1, guid4);
    ck_assert(UA_NodeId_order(&id_guid_3, &id_guid_3) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_guid_3, &id_guid_4) == UA_ORDER_LESS);
    ck_assert(UA_NodeId_order(&id_guid_4, &id_guid_3) == UA_ORDER_MORE);

    UA_NodeId id_guid_5 = UA_NODEID_GUID(1, guid5);
    UA_NodeId id_guid_6 = UA_NODEID_GUID(1, guid6);
    ck_assert(UA_NodeId_order(&id_guid_5, &id_guid_5) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_guid_5, &id_guid_2) == UA_ORDER_LESS);
    ck_assert(UA_NodeId_order(&id_guid_6, &id_guid_5) == UA_ORDER_MORE);

    UA_NodeId id_guid_7 = UA_NODEID_GUID(1, guid7);
    UA_NodeId id_guid_8 = UA_NODEID_GUID(1, guid8);
    ck_assert(UA_NodeId_order(&id_guid_7, &id_guid_7) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_guid_7, &id_guid_8) == UA_ORDER_LESS);
    ck_assert(UA_NodeId_order(&id_guid_8, &id_guid_7) == UA_ORDER_MORE);

} END_TEST

START_TEST(idOrderString) {
    UA_NodeId id_str_a = UA_NODEID_STRING(1, "aaaaa");
    UA_NodeId id_str_b = UA_NODEID_STRING(1, "baa");

    ck_assert(UA_NodeId_order(&id_str_a, &id_str_a) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_str_a, &id_str_b) == UA_ORDER_MORE);
    ck_assert(UA_NodeId_order(&id_str_b, &id_str_a) == UA_ORDER_LESS);

    UA_NodeId id_str_c = UA_NODEID_STRING(1, "cddd");
    UA_NodeId id_str_d = UA_NODEID_STRING(1, "dddd");

    ck_assert(UA_NodeId_order(&id_str_c, &id_str_c) == UA_ORDER_EQ);
    ck_assert(UA_NodeId_order(&id_str_c, &id_str_d) == UA_ORDER_LESS);
    ck_assert(UA_NodeId_order(&id_str_d, &id_str_c) == UA_ORDER_MORE);
} END_TEST

static Suite* testSuite_Utils(void) {
    Suite *s = suite_create("Utils");
    TCase *tc_endpointUrl_split = tcase_create("EndpointUrl_split");
    tcase_add_test(tc_endpointUrl_split, EndpointUrl_split);
    suite_add_tcase(s,tc_endpointUrl_split);
    TCase *tc_endpointUrl_ethernet = tcase_create("EndpointUrl_ethernet");
    tcase_add_test(tc_endpointUrl_ethernet, EndpointUrl_ethernet);
    suite_add_tcase(s,tc_endpointUrl_ethernet);
    TCase *tc_utils = tcase_create("Utils");
    tcase_add_test(tc_utils, readNumber);
    tcase_add_test(tc_utils, readNumberWithBase);
    tcase_add_test(tc_utils, StatusCode_msg);
    tcase_add_test(tc_utils, stringCompare);
    suite_add_tcase(s,tc_utils);


    TCase *tc1 = tcase_create("test nodeid string");
    tcase_add_test(tc1, idToStringNumeric);
    tcase_add_test(tc1, idToStringString);
    tcase_add_test(tc1, idToStringGuid);
    tcase_add_test(tc1, idToStringByte);
    suite_add_tcase(s, tc1);

    TCase *tc2 = tcase_create("test nodeid order");
    tcase_add_test(tc1, idOrderNs);
    tcase_add_test(tc1, idOrderIdentifier);
    tcase_add_test(tc1, idOrderNumeric);
    tcase_add_test(tc1, idOrderGuid);
    tcase_add_test(tc1, idOrderString);
    suite_add_tcase(s, tc2);

    return s;
}

int main(void) {
    Suite *s = testSuite_Utils();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
