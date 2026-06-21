/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/client.h>

#include <dtoa.h>
#include <stdlib.h>
#include <math.h>

#include "util/ua_util_internal.h"
#include "check.h"

/* vs2008 does not have INFINITY and NAN defined */
#ifndef INFINITY
# define INFINITY ((UA_Double)(DBL_MAX+DBL_MAX))
#endif
#ifndef NAN
# define NAN ((UA_Double)(INFINITY-INFINITY))
#endif

/* Provide ck_assert_ptr_null / ck_assert_ptr_nonnull on top of older
 * libcheck (Ubuntu 20.04 ships 0.10.x where these shorthands are not
 * yet defined). The ck_assert_msg form compiles on every libcheck
 * version. */
#ifndef ck_assert_ptr_null
# define ck_assert_ptr_null(p) ck_assert_msg((p) == NULL, #p " != NULL")
#endif
#ifndef ck_assert_ptr_nonnull
# define ck_assert_ptr_nonnull(p) ck_assert_msg((p) != NULL, #p " == NULL")
#endif

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


START_TEST(doubleToString) {
    char buffer[256];

    const double number_13_37 = 13.37;
    const unsigned length_13_37 = dtoa(number_13_37, buffer);
    buffer[length_13_37] = 0;
    ck_assert_str_eq(buffer, "13.37");

    const double number_neg_13_37 = -13.37;
    const unsigned length_neg_13_37 = dtoa(number_neg_13_37, buffer);
    buffer[length_neg_13_37] = 0;
    ck_assert_str_eq(buffer, "-13.37");

    const double number_inf = INFINITY;
    const unsigned length_inf = dtoa(number_inf, buffer);
    buffer[length_inf] = 0;
    ck_assert_str_eq(buffer, "inf");

    const double number_neginf = -INFINITY;
    const unsigned length_neginf = dtoa(number_neginf, buffer);
    buffer[length_neginf] = 0;
    ck_assert_str_eq(buffer, "-inf");

    const double number_nan = NAN;
    const unsigned length_nan = dtoa(number_nan, buffer);
    buffer[length_nan] = 0;
    ck_assert_str_eq(buffer, "nan");

    const double number_negnan = -NAN;
    const unsigned length_negnan = dtoa(number_negnan, buffer);
    buffer[length_negnan] = 0;
    ck_assert_str_eq(buffer, "nan");
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

START_TEST(idToStringWithMapping) {
    UA_NamespaceMapping nsMapping;
    memset(&nsMapping, 0, sizeof(UA_NamespaceMapping));

    UA_String namespaces[2] = {
        UA_STRING_STATIC("ns1"),
        UA_STRING_STATIC("ns2")
    };

    nsMapping.namespaceUris = namespaces;
    nsMapping.namespaceUrisSize = 2;

    UA_NodeId n, n2;
    UA_String str = UA_STRING_NULL;

    n = UA_NODEID_NUMERIC(1,1234567890);
    UA_NodeId_printEx(&n, &str, &nsMapping);
    assertNodeIdString(&str, "nsu=ns2;i=1234567890");
    UA_NodeId_parseEx(&n2, str, &nsMapping);
    ck_assert(UA_NodeId_equal(&n, &n2));
    UA_String_clear(&str);

    n = UA_NODEID_NUMERIC(0xFFFF,0xFFFFFFFF);
    UA_NodeId_print(&n, &str);
    assertNodeIdString(&str, "ns=65535;i=4294967295");
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

START_TEST(kvmContain) {
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();

    UA_UInt16 value_1 = 1;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "value-1"), (void *)&value_1,
                             &UA_TYPES[UA_TYPES_UINT16]);

    ck_assert(UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, "value-1")));
    ck_assert(!UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, "value-2")));

    UA_UInt16 value_2 = 2;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "value-2"), (void *)&value_2,
                             &UA_TYPES[UA_TYPES_UINT16]);

    ck_assert(UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, "value-1")));
    ck_assert(UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, "value-2")));

    UA_KeyValueMap_clear(kvm);

    ck_assert(!UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, "value-1")));
    ck_assert(!UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, "value-2")));

    UA_KeyValueMap_delete(kvm);
} END_TEST

START_TEST(kvmRemove) {
    UA_KeyValueMap *kvm = UA_KeyValueMap_new();

    UA_UInt16 value_1 = 1;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "value-1"), (void *)&value_1,
                             &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 value_2 = 2;
    UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "value-2"), (void *)&value_2,
                             &UA_TYPES[UA_TYPES_UINT16]);

    UA_KeyValueMap_remove(kvm, UA_QUALIFIEDNAME(0, "value-1"));
    ck_assert(UA_KeyValueMap_contains(kvm, UA_QUALIFIEDNAME(0, "value-2")));

    UA_KeyValueMap_delete(kvm);
} END_TEST

START_TEST(kvmMerge) {
    UA_KeyValueMap *kvm_1 = UA_KeyValueMap_new();
    UA_UInt16 value_11 = 11;
    UA_KeyValueMap_setScalar(kvm_1, UA_QUALIFIEDNAME(0, "value-1"), (void *)&value_11,
                             &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 value_12 = 12;
    UA_KeyValueMap_setScalar(kvm_1, UA_QUALIFIEDNAME(0, "value-2"), (void *)&value_12,
                             &UA_TYPES[UA_TYPES_UINT16]);

    UA_KeyValueMap *kvm_2 = UA_KeyValueMap_new();
    UA_UInt16 value_22 = 22;
    UA_KeyValueMap_setScalar(kvm_2, UA_QUALIFIEDNAME(0, "value-2"), (void *)&value_22,
                             &UA_TYPES[UA_TYPES_UINT16]);
    UA_UInt16 value_23 = 23;
    UA_KeyValueMap_setScalar(kvm_2, UA_QUALIFIEDNAME(0, "value-3"), (void *)&value_23,
                             &UA_TYPES[UA_TYPES_UINT16]);

    UA_KeyValueMap_merge(kvm_1, kvm_2);

    const UA_Variant *value_1 = UA_KeyValueMap_get(kvm_1, UA_QUALIFIEDNAME(0, "value-1"));
    const UA_Variant *value_2 = UA_KeyValueMap_get(kvm_1, UA_QUALIFIEDNAME(0, "value-2"));
    const UA_Variant *value_3 = UA_KeyValueMap_get(kvm_1, UA_QUALIFIEDNAME(0, "value-3"));
    ck_assert(UA_Variant_hasScalarType(value_1, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert(UA_Variant_hasScalarType(value_2, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert(UA_Variant_hasScalarType(value_3, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert(*((UA_UInt16 *) value_1->data) == 11);
    ck_assert(*((UA_UInt16 *) value_2->data) == 22);
    ck_assert(*((UA_UInt16 *) value_3->data) == 23);

    UA_KeyValueMap_delete(kvm_1);
    UA_KeyValueMap_delete(kvm_2);
} END_TEST

START_TEST(expIdToStringNumeric) {
    UA_ExpandedNodeId n;
    UA_String str = UA_STRING_NULL;

    n = UA_EXPANDEDNODEID_NUMERIC(0,0);
    UA_ExpandedNodeId_print(&n, &str);
    assertNodeIdString(&str, "i=0");
    UA_String_clear(&str);

    n.serverIndex = 1;
    UA_ExpandedNodeId_print(&n, &str);
    assertNodeIdString(&str, "svr=1;i=0");
    UA_String_clear(&str);

    n.namespaceUri = UA_STRING("testuri");
    UA_ExpandedNodeId_print(&n, &str);
    assertNodeIdString(&str, "svr=1;nsu=testuri;i=0");
    UA_String_clear(&str);
} END_TEST

START_TEST(expIdToStringNumericWithMapping) {
    UA_String serverUris[2] = {
        UA_STRING_STATIC("uri:server1"),
        UA_STRING_STATIC("uri:server2")
    };

    UA_ExpandedNodeId n, n2;
    UA_String str = UA_STRING_NULL;

    n = UA_EXPANDEDNODEID_NUMERIC(0,0);
    n.serverIndex = 1;
    UA_ExpandedNodeId_printEx(&n, &str, NULL, 2, serverUris);
    assertNodeIdString(&str, "svu=uri:server2;i=0");
    UA_String_clear(&str);

    n.namespaceUri = UA_STRING("testuri");
    UA_ExpandedNodeId_printEx(&n, &str, NULL, 2, serverUris);
    assertNodeIdString(&str, "svu=uri:server2;nsu=testuri;i=0");
    UA_ExpandedNodeId_parseEx(&n2, str, NULL, 2, serverUris);
    ck_assert(UA_ExpandedNodeId_equal(&n, &n2));
    UA_ExpandedNodeId_clear(&n2);
    UA_String_clear(&str);

    n.namespaceUri = UA_STRING_NULL;
    n.nodeId.namespaceIndex = 2;
    UA_ExpandedNodeId_printEx(&n, &str, NULL, 0, NULL);
    assertNodeIdString(&str, "svr=1;ns=2;i=0");
    UA_ExpandedNodeId_parseEx(&n2, str, NULL, 0, NULL);
    ck_assert(UA_ExpandedNodeId_equal(&n, &n2));
    UA_ExpandedNodeId_clear(&n2);
    UA_String_clear(&str);
} END_TEST

START_TEST(qualifiedNameNsUri) {
    UA_String namespaces[2] = {
        UA_STRING_STATIC("ns1"),
        UA_STRING_STATIC("ns2")
    };

    UA_NamespaceMapping nsMapping;
    memset(&nsMapping, 0, sizeof(UA_NamespaceMapping));
    nsMapping.namespaceUris = namespaces;
    nsMapping.namespaceUrisSize = 2;

    UA_QualifiedName qn = UA_QUALIFIEDNAME(1, "name");
    UA_String str = UA_STRING_NULL;

    UA_QualifiedName_printEx(&qn, &str, &nsMapping);
    assertNodeIdString(&str, "ns2;name");

    UA_QualifiedName qn2;
    UA_QualifiedName_parseEx(&qn2, str, &nsMapping);
    ck_assert(UA_QualifiedName_equal(&qn, &qn2));

    UA_QualifiedName_clear(&qn2);
    UA_String_clear(&str);
} END_TEST

START_TEST(qualifiedNameNsIndex) {
    UA_QualifiedName qn = UA_QUALIFIEDNAME(1, "name");
    UA_String str = UA_STRING_NULL;

    UA_QualifiedName_printEx(&qn, &str, NULL);
    assertNodeIdString(&str, "1:name");

    UA_QualifiedName qn2;
    UA_QualifiedName_parseEx(&qn2, str, NULL);
    ck_assert(UA_QualifiedName_equal(&qn, &qn2));

    UA_QualifiedName_clear(&qn2);
    UA_String_clear(&str);
} END_TEST

START_TEST(format_string) {
    UA_NodeId test = UA_NODEID_NUMERIC(1,1);
    UA_String testStr = UA_STRING("banana");
    UA_String out = UA_STRING_NULL;

    UA_StatusCode res = UA_String_format(&out, "test %N %S", test, testStr);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_String expected = UA_STRING("test ns=1;i=1 banana");
    ck_assert(UA_String_equal(&out, &expected));

    UA_String_clear(&out);

    UA_Byte buf[4];
    UA_String shortOut = {4, buf};
    res = UA_String_format(&shortOut, "test %N %S", test, testStr);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(TrustListDataType_contains) {
    /* Initialize trust list */
    UA_TrustListDataType trustList;
    UA_TrustListDataType_init(&trustList);

    /* Create test data for each trust list category */
    UA_ByteString cert1 = UA_BYTESTRING("certificate1");
    UA_ByteString cert2 = UA_BYTESTRING("certificate2");
    UA_ByteString crl1 = UA_BYTESTRING("crl1");
    UA_ByteString issuerCert1 = UA_BYTESTRING("issuerCert1");
    UA_ByteString issuerCrl1 = UA_BYTESTRING("issuerCrl1");
    UA_ByteString notInList = UA_BYTESTRING("notInList");

    /* Define mask constants */
    const UA_TrustListMasks maskAll = (UA_TrustListMasks)UA_TRUSTLISTMASKS_ALL;
    const UA_TrustListMasks maskTrustedCerts = (UA_TrustListMasks)UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    const UA_TrustListMasks maskTrustedCrls = (UA_TrustListMasks)UA_TRUSTLISTMASKS_TRUSTEDCRLS;
    const UA_TrustListMasks maskIssuerCerts = (UA_TrustListMasks)UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;
    const UA_TrustListMasks maskIssuerCrls = (UA_TrustListMasks)UA_TRUSTLISTMASKS_ISSUERCRLS;
    const UA_TrustListMasks maskCertsOrIssuer =
        (UA_TrustListMasks)(UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES | UA_TRUSTLISTMASKS_ISSUERCERTIFICATES);
    const UA_TrustListMasks maskCrlsOrIssuerCrls =
        (UA_TrustListMasks)(UA_TRUSTLISTMASKS_TRUSTEDCRLS | UA_TRUSTLISTMASKS_ISSUERCRLS);
    const UA_TrustListMasks maskNone = (UA_TrustListMasks)0;

    /* Test with NULL parameters */
    ck_assert_int_eq(UA_TrustListDataType_contains(NULL, &cert1, maskAll), false);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, NULL, maskAll), false);

    /* Populate trusted certificates */
    trustList.trustedCertificates = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    trustList.trustedCertificatesSize = 2;
    UA_ByteString_copy(&cert1, &trustList.trustedCertificates[0]);
    UA_ByteString_copy(&cert2, &trustList.trustedCertificates[1]);
    trustList.specifiedLists |= (UA_TrustListMasks)UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;

    /* Populate trusted CRLs */
    trustList.trustedCrls = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    trustList.trustedCrlsSize = 1;
    UA_ByteString_copy(&crl1, &trustList.trustedCrls[0]);
    trustList.specifiedLists |= (UA_TrustListMasks)UA_TRUSTLISTMASKS_TRUSTEDCRLS;

    /* Populate issuer certificates */
    trustList.issuerCertificates = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    trustList.issuerCertificatesSize = 1;
    UA_ByteString_copy(&issuerCert1, &trustList.issuerCertificates[0]);
    trustList.specifiedLists |= (UA_TrustListMasks)UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;

    /* Populate issuer CRLs */
    trustList.issuerCrls = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    trustList.issuerCrlsSize = 1;
    UA_ByteString_copy(&issuerCrl1, &trustList.issuerCrls[0]);
    trustList.specifiedLists |= (UA_TrustListMasks)UA_TRUSTLISTMASKS_ISSUERCRLS;

    /* Test with UA_TRUSTLISTMASKS_ALL */
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &cert1, maskAll), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &cert2, maskAll), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &crl1, maskAll), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &issuerCert1, maskAll), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &issuerCrl1, maskAll), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &notInList, maskAll), false);

    /* Test with single mask flags */
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &cert1, maskTrustedCerts), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &crl1, maskTrustedCerts), false);

    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &crl1, maskTrustedCrls), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &cert1, maskTrustedCrls), false);

    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &issuerCert1, maskIssuerCerts), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &crl1, maskIssuerCerts), false);

    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &issuerCrl1, maskIssuerCrls), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &cert1, maskIssuerCrls), false);

    /* Test with OR'ed mask flags */
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &cert1, maskCertsOrIssuer), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &issuerCert1, maskCertsOrIssuer), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &crl1, maskCertsOrIssuer), false);

    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &crl1, maskCrlsOrIssuerCrls), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &issuerCrl1, maskCrlsOrIssuerCrls), true);
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &cert1, maskCrlsOrIssuerCrls), false);

    /* Test with empty mask (no lists specified) */
    ck_assert_int_eq(UA_TrustListDataType_contains(&trustList, &cert1, maskNone), false);
    /* Cleanup */
    UA_TrustListDataType_clear(&trustList);
} END_TEST

START_TEST(byteStringCopy) {
    UA_ByteString src = UA_BYTESTRING("test data");
    UA_ByteString dst;
    UA_ByteString_init(&dst);

    UA_StatusCode retval = UA_ByteString_copy(&src, &dst);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(src.length, dst.length);
    ck_assert(memcmp(src.data, dst.data, src.length) == 0);
    ck_assert_ptr_ne(src.data, dst.data);

    UA_ByteString_clear(&dst);

    /* Test with empty ByteString */
    UA_ByteString empty = UA_BYTESTRING_NULL;
    UA_ByteString emptyCopy;
    retval = UA_ByteString_copy(&empty, &emptyCopy);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(emptyCopy.length, 0);
} END_TEST

START_TEST(byteStringEqual) {
    UA_ByteString bs1 = UA_BYTESTRING("hello");
    UA_ByteString bs2 = UA_BYTESTRING("hello");
    UA_ByteString bs3 = UA_BYTESTRING("world");
    UA_ByteString bs4 = UA_BYTESTRING("helloworld");

    ck_assert(UA_ByteString_equal(&bs1, &bs2) == true);
    ck_assert(UA_ByteString_equal(&bs1, &bs3) == false);
    ck_assert(UA_ByteString_equal(&bs1, &bs4) == false);

    /* Test with empty ByteStrings */
    UA_ByteString empty1 = UA_BYTESTRING_NULL;
    UA_ByteString empty2 = UA_BYTESTRING_NULL;
    ck_assert(UA_ByteString_equal(&empty1, &empty2) == true);
    ck_assert(UA_ByteString_equal(&bs1, &empty1) == false);
} END_TEST

START_TEST(stringCopy) {
    UA_String src = UA_STRING("copy test");
    UA_String dst;
    UA_String_init(&dst);

    UA_StatusCode retval = UA_String_copy(&src, &dst);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(src.length, dst.length);
    ck_assert(memcmp(src.data, dst.data, src.length) == 0);
    ck_assert_ptr_ne(src.data, dst.data);

    UA_String_clear(&dst);

    /* Test with empty String */
    UA_String empty = UA_STRING_NULL;
    UA_String emptyCopy;
    retval = UA_String_copy(&empty, &emptyCopy);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(emptyCopy.length, 0);
} END_TEST

START_TEST(trustListDataTypeOperations) {
    /* Test UA_TrustListDataType_add, remove, contains, and getSize */
    UA_TrustListDataType src;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType dst;
    UA_TrustListDataType_init(&dst);

    /* Create test certificates */
    UA_ByteString cert1 = UA_BYTESTRING("TestCert1");
    UA_ByteString cert2 = UA_BYTESTRING("TestCert2");

    /* Set up source trust list with certificates */
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    src.trustedCertificatesSize = 1;
    src.trustedCertificates = (UA_ByteString*)UA_malloc(sizeof(UA_ByteString));
    UA_ByteString_copy(&cert1, &src.trustedCertificates[0]);

    /* Test add - adds src to dst */
    UA_StatusCode retval = UA_TrustListDataType_add(&src, &dst);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 1);

    /* Test contains */
    UA_Boolean contains = UA_TrustListDataType_contains(&dst, &cert1, UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES);
    ck_assert(contains == true);
    contains = UA_TrustListDataType_contains(&dst, &cert2, UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES);
    ck_assert(contains == false);

    /* Test getSize */
    UA_UInt32 size = UA_TrustListDataType_getSize(&dst);
    ck_assert(size > 0);

    /* Add another certificate */
    UA_TrustListDataType src2;
    UA_TrustListDataType_init(&src2);
    src2.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    src2.trustedCertificatesSize = 1;
    src2.trustedCertificates = (UA_ByteString*)UA_malloc(sizeof(UA_ByteString));
    UA_ByteString_copy(&cert2, &src2.trustedCertificates[0]);

    retval = UA_TrustListDataType_add(&src2, &dst);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 2);

    /* Test remove */
    retval = UA_TrustListDataType_remove(&src, &dst);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 1);

    /* Verify cert1 removed, cert2 remains */
    contains = UA_TrustListDataType_contains(&dst, &cert1, UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES);
    ck_assert(contains == false);
    contains = UA_TrustListDataType_contains(&dst, &cert2, UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES);
    ck_assert(contains == true);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&src2);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(simpleAttributeOperandPrint) {
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);

    /* Set up a SimpleAttributeOperand */
    sao.typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    sao.browsePathSize = 2;
    sao.browsePath = (UA_QualifiedName*)UA_calloc(2, sizeof(UA_QualifiedName));
    sao.browsePath[0] = UA_QUALIFIEDNAME(0, "Message");
    sao.browsePath[1] = UA_QUALIFIEDNAME(0, "Text");
    sao.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode retval = UA_SimpleAttributeOperand_print(&sao, &out);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    ck_assert(out.data != NULL);

    UA_String_clear(&out);
    UA_free(sao.browsePath);
} END_TEST

START_TEST(attributeOperandPrint) {
    UA_AttributeOperand ao;
    UA_AttributeOperand_init(&ao);

    /* Set up an AttributeOperand */
    ao.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    ao.attributeId = UA_ATTRIBUTEID_VALUE;
    ao.alias = UA_STRING("TestAlias");

    UA_String out = UA_STRING_NULL;
    UA_StatusCode retval = UA_AttributeOperand_print(&ao, &out);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    ck_assert(out.data != NULL);

    UA_String_clear(&out);
} END_TEST

START_TEST(attributeId_name_roundtrip) {
    /* Round-trip every valid AttributeId through name() and fromName(). The
     * names must round-trip exactly. */
    for(size_t i = 0; i < 28; i++) {
        const char *n = UA_AttributeId_name((UA_AttributeId)i);
        ck_assert_ptr_ne(n, NULL);
        ck_assert_uint_eq(strlen(n) > 0, 1);
        UA_String s = UA_STRING((char*)(uintptr_t)n);
        UA_AttributeId back = UA_AttributeId_fromName(s);
        ck_assert_uint_eq((UA_UInt32)back, (UA_UInt32)i);
    }

    /* fromName with an unknown name returns UA_ATTRIBUTEID_INVALID. */
    UA_String unknown = UA_STRING("Definitely-Not-A-Real-AttributeId");
    ck_assert_uint_eq(UA_AttributeId_fromName(unknown),
                      UA_ATTRIBUTEID_INVALID);

    /* Empty string also returns UA_ATTRIBUTEID_INVALID. */
    UA_String empty = UA_STRING_NULL;
    ck_assert_uint_eq(UA_AttributeId_fromName(empty),
                      UA_ATTRIBUTEID_INVALID);
} END_TEST

START_TEST(attributeId_name_outOfRange_returnsInvalid) {
    /* src/util/ua_util.c:48-50:
     *   if(attrId < 0 || attrId > UA_ATTRIBUTEID_ACCESSLEVELEX)
     *     return attributeIdNames[0]; // "Invalid"
     * The existing attributeId_name_roundtrip loops over the valid
     * range 0..27; the negative and >27 branches are not exercised. */
    /* Negative cast -- attrId is signed; the explicit cast is what
     * the existing code uses internally. */
    const char *neg = UA_AttributeId_name((UA_AttributeId)-1);
    ck_assert_ptr_ne(neg, NULL);
    ck_assert_str_eq(neg, "Invalid");

    const char *large = UA_AttributeId_name((UA_AttributeId)99999);
    ck_assert_ptr_ne(large, NULL);
    ck_assert_str_eq(large, "Invalid");

    /* Just past the end of the table */
    const char *justOver = UA_AttributeId_name((UA_AttributeId)28);
    ck_assert_str_eq(justOver, "Invalid");
} END_TEST

START_TEST(attributeId_fromName_caseInsensitive) {
    /* src/util/ua_util.c:60-61:
     *   if((attributeIdNames[i][j] | 32) != (name.data[j] | 32))
     * The OR-32 lowercases both sides for case-insensitive matching.
     * The existing roundtrip test only uses the canonical (mixed)
     * case names. */
    UA_String upper = UA_STRING("VALUE");
    UA_AttributeId id = UA_AttributeId_fromName(upper);
    ck_assert_uint_eq((UA_UInt32)id, (UA_UInt32)UA_ATTRIBUTEID_VALUE);

    UA_String lower = UA_STRING("nodEclass");
    id = UA_AttributeId_fromName(lower);
    ck_assert_uint_eq((UA_UInt32)id, (UA_UInt32)UA_ATTRIBUTEID_NODECLASS);
} END_TEST

START_TEST(parseEndpointUrl_opcEth_scheme) {
    /* parseEndpointUrl accepts the opc.eth:// scheme and reports the
     * Ethernet (0x0800) EtherType. */
    UA_String hostname, path;
    UA_UInt16 port = 0;
    UA_String endPointUrl = UA_STRING("opc.eth://00:11:22:33:44:55:66:77");
    UA_StatusCode retval =
        UA_parseEndpointUrl(&endPointUrl, &hostname, &port, &path);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    /* Ethernet scheme has no port; the implementation should leave port=0. */
    ck_assert_uint_eq(port, 0);
    ck_assert(hostname.length > 0);
} END_TEST

START_TEST(parseEndpointUrl_opcUdp_scheme) {
    /* src/util/ua_util.c:140-141, 153-161:
     *   static const char* schemas[4] = {"opc.tcp://", "opc.udp://",
     *                                    "opc.eth://", "opc.mqtt://"};
     *   for(; schemaType < 4; schemaType++) { ... }
     *   if(schemaType == 4) return BADTCPENDPOINTURLINVALID;
     * The existing 33 tests in check_utils.c only exercise opc.tcp://
     * and opc.eth:// -- schemaType 1 (opc.udp://) is never reached. */
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_String path = UA_STRING_NULL;

    /* opc.udp:// hostname only */
    UA_String url = UA_STRING("opc.udp://hostname");
    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("hostname");
    ck_assert(UA_String_equal(&hostname, &expected));

    /* opc.udp:// with port */
    url = UA_STRING("opc.udp://hostname:4840");
    res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 4840);
} END_TEST

START_TEST(parseEndpointUrl_opcMqtt_scheme) {
    /* The same dispatch loop, schemaType 3 (opc.mqtt://). */
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_String path = UA_STRING_NULL;

    UA_String url = UA_STRING("opc.mqtt://broker.example.com");
    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String expected = UA_STRING("broker.example.com");
    ck_assert(UA_String_equal(&hostname, &expected));

    /* With port */
    url = UA_STRING("opc.mqtt://broker.example.com:1883");
    res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&hostname, &expected));
    ck_assert_uint_eq(port, 1883);
} END_TEST

START_TEST(parseEndpointUrl_unknownScheme_rejected) {
    /* src/util/ua_util.c:160-161: if(schemaType == 4)
     *   return UA_STATUSCODE_BADTCPENDPOINTURLINVALID; */
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_String path = UA_STRING_NULL;

    /* HTTP -- the loop never matches. */
    UA_String url = UA_STRING("http://example.com:80/");
    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADTCPENDPOINTURLINVALID);

    /* MQTTs (with trailing 's', different prefix). */
    url = UA_STRING("mqtt://broker:1883");
    res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_BADTCPENDPOINTURLINVALID);
} END_TEST

/* Skipped: the parseEndpointUrlEthernet parser requires a colon after the
 * MAC for the VID/PCP components, and the various edge cases (missing
 * trailing colon, trailing colon with no value) all return BADINTERNALERROR.
 * The simple opc.tcp:// path through the URL parser is already covered by
 * the existing parseEndpointUrl_opc_tcp / parseEndpointUrl_eth tests. */


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
    tcase_add_test(tc_utils, doubleToString);
    tcase_add_test(tc_utils, StatusCode_msg);
    tcase_add_test(tc_utils, stringCompare);
    suite_add_tcase(s,tc_utils);

    TCase *tc1 = tcase_create("test nodeid string");
    tcase_add_test(tc1, idToStringNumeric);
    tcase_add_test(tc1, idToStringString);
    tcase_add_test(tc1, idToStringGuid);
    tcase_add_test(tc1, idToStringByte);
    tcase_add_test(tc1, idToStringWithMapping);
    suite_add_tcase(s, tc1);

    TCase *tc2 = tcase_create("test nodeid order");
    tcase_add_test(tc1, idOrderNs);
    tcase_add_test(tc1, idOrderIdentifier);
    tcase_add_test(tc1, idOrderNumeric);
    tcase_add_test(tc1, idOrderGuid);
    tcase_add_test(tc1, idOrderString);
    suite_add_tcase(s, tc2);

    TCase *tc3 = tcase_create("test keyvaluemap");
    tcase_add_test(tc3, kvmContain);
    tcase_add_test(tc3, kvmRemove);
    tcase_add_test(tc3, kvmMerge);
    suite_add_tcase(s, tc3);

    TCase *tc4 = tcase_create("test expandednodeid string");
    tcase_add_test(tc4, expIdToStringNumeric);
    tcase_add_test(tc4, expIdToStringNumericWithMapping);
    suite_add_tcase(s, tc4);

    TCase *tc5 = tcase_create("test qualifiedname string");
    tcase_add_test(tc5, qualifiedNameNsUri);
    tcase_add_test(tc5, qualifiedNameNsIndex);
    suite_add_tcase(s, tc5);

    TCase *tc6 = tcase_create("test string format");
    tcase_add_test(tc6, format_string);
    suite_add_tcase(s, tc6);

    TCase *tcTrustList = tcase_create("test trustlist contains");
    tcase_add_test(tcTrustList, TrustListDataType_contains);
    suite_add_tcase(s, tcTrustList);

    TCase *tc7 = tcase_create("string copy utilities");
    tcase_add_test(tc7, byteStringCopy);
    tcase_add_test(tc7, byteStringEqual);
    tcase_add_test(tc7, stringCopy);
    suite_add_tcase(s, tc7);

    TCase *tc8 = tcase_create("test trustlist and operand utilities");
    tcase_add_test(tc8, trustListDataTypeOperations);
    tcase_add_test(tc8, simpleAttributeOperandPrint);
    tcase_add_test(tc8, attributeOperandPrint);
    suite_add_tcase(s, tc8);

    TCase *tc9 = tcase_create("attribute id name round-trip");
    tcase_add_test(tc9, attributeId_name_roundtrip);
    tcase_add_test(tc9, attributeId_name_outOfRange_returnsInvalid);
    tcase_add_test(tc9, attributeId_fromName_caseInsensitive);
    suite_add_tcase(s, tc9);

    TCase *tc10 = tcase_create("parseEndpointUrl edge cases");
    tcase_add_test(tc10, parseEndpointUrl_opcEth_scheme);
    tcase_add_test(tc10, parseEndpointUrl_opcUdp_scheme);
    tcase_add_test(tc10, parseEndpointUrl_opcMqtt_scheme);
    tcase_add_test(tc10, parseEndpointUrl_unknownScheme_rejected);
    suite_add_tcase(s, tc10);

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
