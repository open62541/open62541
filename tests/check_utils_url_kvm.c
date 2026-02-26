/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/common.h>
#include <open62541/util.h>
#include "util/ua_util_internal.h"

#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <math.h>

/* ========== parseEndpointUrl ========== */
START_TEST(parse_url_tcp) {
    UA_String url = UA_STRING("opc.tcp://localhost:4840/path");
    UA_String host, path;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &host, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(port, 4840);
} END_TEST

START_TEST(parse_url_tcp_noport) {
    UA_String url = UA_STRING("opc.tcp://localhost/path");
    UA_String host, path;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &host, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(port, 0);
} END_TEST

START_TEST(parse_url_tcp_nopath) {
    UA_String url = UA_STRING("opc.tcp://host:1234");
    UA_String host, path;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &host, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(port, 1234);
} END_TEST

START_TEST(parse_url_ipv6) {
    UA_String url = UA_STRING("opc.tcp://[::1]:4840/path");
    UA_String host, path;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &host, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(port, 4840);
} END_TEST

START_TEST(parse_url_ipv6_noport) {
    UA_String url = UA_STRING("opc.tcp://[::1]/path");
    UA_String host, path;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &host, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(parse_url_invalid) {
    UA_String url = UA_STRING("http://invalid");
    UA_String host, path;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &host, &port, &path);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(parse_url_https) {
    UA_String url = UA_STRING("opc.https://host:443/path");
    UA_String host, path;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &host, &port, &path);
    /* BUG: opc.https is a valid OPC UA endpoint scheme (Part 6) but
     * UA_parseEndpointUrl does not support it. See COVERAGE_BUGS.md #6. */
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(parse_url_wss) {
    UA_String url = UA_STRING("opc.wss://host:8080");
    UA_String host, path;
    UA_UInt16 port = 0;
    UA_StatusCode res = UA_parseEndpointUrl(&url, &host, &port, &path);
    /* BUG: opc.wss is a valid OPC UA endpoint scheme (Part 6) but
     * UA_parseEndpointUrl does not support it. See COVERAGE_BUGS.md #6. */
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* ========== parseEndpointUrlEthernet ========== */
START_TEST(parse_url_ethernet) {
    UA_String url = UA_STRING("opc.eth://01-00-5E-00-00-01");
    UA_String target;
    UA_UInt16 vid = 0;
    UA_Byte pcp = 0;
    UA_StatusCode res = UA_parseEndpointUrlEthernet(&url, &target, &vid, &pcp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(parse_url_ethernet_vlan) {
    UA_String url = UA_STRING("opc.eth://01-00-5E-00-00-01:5.3");
    UA_String target;
    UA_UInt16 vid = 0;
    UA_Byte pcp = 0;
    UA_StatusCode res = UA_parseEndpointUrlEthernet(&url, &target, &vid, &pcp);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(vid, 5);
    ck_assert_uint_eq(pcp, 3);
} END_TEST

START_TEST(parse_url_ethernet_invalid) {
    UA_String url = UA_STRING("http://not-ethernet");
    UA_String target;
    UA_UInt16 vid = 0;
    UA_Byte pcp = 0;
    UA_StatusCode res = UA_parseEndpointUrlEthernet(&url, &target, &vid, &pcp);
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* ========== Base64 ========== */
START_TEST(base64_roundtrip) {
    UA_ByteString src = UA_BYTESTRING("Hello, World!");
    UA_String b64;
    UA_String_init(&b64);
    UA_StatusCode res = UA_ByteString_toBase64(&src, &b64);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(b64.length > 0);

    UA_ByteString decoded;
    UA_ByteString_init(&decoded);
    res = UA_ByteString_fromBase64(&decoded, &b64);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&src, &decoded));
    UA_String_clear(&b64);
    UA_ByteString_clear(&decoded);
} END_TEST

START_TEST(base64_empty) {
    UA_ByteString src;
    UA_ByteString_init(&src);
    UA_String b64;
    UA_String_init(&b64);
    UA_StatusCode res = UA_ByteString_toBase64(&src, &b64);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(base64_from_empty_string) {
    UA_String b64 = UA_STRING("");
    UA_ByteString decoded;
    UA_ByteString_init(&decoded);
    UA_StatusCode res = UA_ByteString_fromBase64(&decoded, &b64);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* ========== AttributeId ========== */
START_TEST(attributeid_name) {
    const char *name;
    name = UA_AttributeId_name(UA_ATTRIBUTEID_NODEID);
    ck_assert_str_eq(name, "NodeId");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_NODECLASS);
    ck_assert_str_eq(name, "NodeClass");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_BROWSENAME);
    ck_assert_str_eq(name, "BrowseName");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_DISPLAYNAME);
    ck_assert_str_eq(name, "DisplayName");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_DESCRIPTION);
    ck_assert_str_eq(name, "Description");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_WRITEMASK);
    ck_assert_str_eq(name, "WriteMask");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_USERWRITEMASK);
    ck_assert_str_eq(name, "UserWriteMask");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_VALUE);
    ck_assert_str_eq(name, "Value");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_DATATYPE);
    ck_assert_str_eq(name, "DataType");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_VALUERANK);
    ck_assert_str_eq(name, "ValueRank");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_ACCESSLEVEL);
    ck_assert_str_eq(name, "AccessLevel");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL);
    ck_assert_str_eq(name, "MinimumSamplingInterval");
    name = UA_AttributeId_name(UA_ATTRIBUTEID_HISTORIZING);
    ck_assert_str_eq(name, "Historizing");
    /* Out of range */
    name = UA_AttributeId_name((UA_AttributeId)99);
    ck_assert_ptr_ne(name, NULL);
} END_TEST

START_TEST(attributeid_from_name) {
    UA_AttributeId a;
    a = UA_AttributeId_fromName(UA_STRING("Value"));
    ck_assert_uint_eq(a, UA_ATTRIBUTEID_VALUE);
    a = UA_AttributeId_fromName(UA_STRING("BrowseName"));
    ck_assert_uint_eq(a, UA_ATTRIBUTEID_BROWSENAME);
    a = UA_AttributeId_fromName(UA_STRING("DisplayName"));
    ck_assert_uint_eq(a, UA_ATTRIBUTEID_DISPLAYNAME);
    a = UA_AttributeId_fromName(UA_STRING("NodeId"));
    ck_assert_uint_eq(a, UA_ATTRIBUTEID_NODEID);
    a = UA_AttributeId_fromName(UA_STRING("NodeClass"));
    ck_assert_uint_eq(a, UA_ATTRIBUTEID_NODECLASS);
    a = UA_AttributeId_fromName(UA_STRING("Description"));
    ck_assert_uint_eq(a, UA_ATTRIBUTEID_DESCRIPTION);
    a = UA_AttributeId_fromName(UA_STRING("DataType"));
    ck_assert_uint_eq(a, UA_ATTRIBUTEID_DATATYPE);
    /* Unknown name */
    a = UA_AttributeId_fromName(UA_STRING("NonExistent"));
    ck_assert_uint_eq(a, UA_ATTRIBUTEID_INVALID);
} END_TEST

/* ========== readNumberWithBase ========== */
START_TEST(read_number_decimal) {
    UA_UInt32 num = 0;
    const UA_Byte buf[] = "12345";
    size_t len = UA_readNumberWithBase(buf, 5, &num, 10);
    ck_assert_uint_eq(len, 5);
    ck_assert_uint_eq(num, 12345);
} END_TEST

START_TEST(read_number_hex) {
    UA_UInt32 num = 0;
    const UA_Byte buf[] = "FF";
    size_t len = UA_readNumberWithBase(buf, 2, &num, 16);
    ck_assert_uint_eq(len, 2);
    ck_assert_uint_eq(num, 255);
} END_TEST

START_TEST(read_number_hex_lower) {
    UA_UInt32 num = 0;
    const UA_Byte buf[] = "ff";
    size_t len = UA_readNumberWithBase(buf, 2, &num, 16);
    ck_assert_uint_eq(len, 2);
    ck_assert_uint_eq(num, 255);
} END_TEST

START_TEST(read_number_empty) {
    UA_UInt32 num = 0;
    size_t len = UA_readNumberWithBase((const UA_Byte *)"", 0, &num, 10);
    ck_assert_uint_eq(len, 0);
} END_TEST

/* ========== KeyValueMap ========== */
START_TEST(kvm_lifecycle) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    ck_assert_ptr_ne(map, NULL);
    ck_assert(UA_KeyValueMap_isEmpty(map));

    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "testKey");
    UA_Int32 val = 42;
    UA_StatusCode res = UA_KeyValueMap_setScalar(map, key,
                                                  &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(!UA_KeyValueMap_isEmpty(map));

    ck_assert(UA_KeyValueMap_contains(map, key));

    const UA_Variant *v = UA_KeyValueMap_get(map, key);
    ck_assert_ptr_ne(v, NULL);
    ck_assert(UA_Variant_hasScalarType(v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert_int_eq(*(UA_Int32 *)v->data, 42);

    /* Overwrite with new value */
    UA_Int32 val2 = 99;
    res = UA_KeyValueMap_setScalar(map, key, &val2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    v = UA_KeyValueMap_get(map, key);
    ck_assert_int_eq(*(UA_Int32 *)v->data, 99);

    /* Remove */
    res = UA_KeyValueMap_remove(map, key);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_KeyValueMap_isEmpty(map));

    /* Remove non-existent */
    res = UA_KeyValueMap_remove(map, key);
    ck_assert(res != UA_STATUSCODE_GOOD);

    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_set_shallow) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();

    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "shallowKey");

    /* Use heap-allocated data so cleanup is safe */
    UA_Int32 *val = UA_Int32_new();
    *val = 77;
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Variant_setScalar(&v, val, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_KeyValueMap_setShallow(map, key, &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Read back */
    const UA_Variant *got = UA_KeyValueMap_get(map, key);
    ck_assert_ptr_ne(got, NULL);

    /* Overwrite with new heap value - old one will be freed via clear */
    UA_Int32 *val2 = UA_Int32_new();
    *val2 = 88;
    UA_Variant v2;
    UA_Variant_init(&v2);
    UA_Variant_setScalar(&v2, val2, &UA_TYPES[UA_TYPES_INT32]);
    res = UA_KeyValueMap_setShallow(map, key, &v2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Delete map (will clear the remaining entry) */
    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_copy) {
    UA_KeyValueMap *src = UA_KeyValueMap_new();
    UA_KeyValueMap *dst = UA_KeyValueMap_new();

    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "copyKey");
    UA_Int32 val = 55;
    UA_KeyValueMap_setScalar(src, key, &val, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_KeyValueMap_copy(src, dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_KeyValueMap_contains(dst, key));

    UA_KeyValueMap_delete(src);
    UA_KeyValueMap_delete(dst);
} END_TEST

START_TEST(kvm_merge) {
    UA_KeyValueMap *map1 = UA_KeyValueMap_new();
    UA_KeyValueMap *map2 = UA_KeyValueMap_new();

    UA_QualifiedName k1 = UA_QUALIFIEDNAME(0, "key1");
    UA_QualifiedName k2 = UA_QUALIFIEDNAME(0, "key2");
    UA_Int32 v1 = 10, v2 = 20;
    UA_KeyValueMap_setScalar(map1, k1, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_KeyValueMap_setScalar(map2, k2, &v2, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_KeyValueMap_merge(map1, map2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_KeyValueMap_contains(map1, k1));
    ck_assert(UA_KeyValueMap_contains(map1, k2));

    UA_KeyValueMap_delete(map1);
    UA_KeyValueMap_delete(map2);
} END_TEST

START_TEST(kvm_merge_overlapping) {
    UA_KeyValueMap *map1 = UA_KeyValueMap_new();
    UA_KeyValueMap *map2 = UA_KeyValueMap_new();

    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "sameKey");
    UA_Int32 v1 = 10, v2 = 20;
    UA_KeyValueMap_setScalar(map1, key, &v1, &UA_TYPES[UA_TYPES_INT32]);
    UA_KeyValueMap_setScalar(map2, key, &v2, &UA_TYPES[UA_TYPES_INT32]);

    UA_StatusCode res = UA_KeyValueMap_merge(map1, map2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Should have the merged value */
    const UA_Variant *v = UA_KeyValueMap_get(map1, key);
    ck_assert_ptr_ne(v, NULL);

    UA_KeyValueMap_delete(map1);
    UA_KeyValueMap_delete(map2);
} END_TEST

START_TEST(kvm_null_checks) {
    ck_assert(UA_KeyValueMap_isEmpty(NULL));
    ck_assert(!UA_KeyValueMap_contains(NULL, UA_QUALIFIEDNAME(0, "key")));
    UA_KeyValueMap_clear(NULL); /* Should not crash */
    const UA_Variant *v = UA_KeyValueMap_get(NULL, UA_QUALIFIEDNAME(0, "key"));
    ck_assert_ptr_eq(v, NULL);
} END_TEST

/* ========== TrustListDataType ========== */
START_TEST(trustlist_getsize) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);

    UA_UInt32 size = UA_TrustListDataType_getSize(&tl);
    ck_assert_uint_eq(size, 0);

    /* Add a trusted certificate */
    tl.trustedCertificatesSize = 1;
    tl.trustedCertificates = UA_ByteString_new();
    tl.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    tl.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;

    size = UA_TrustListDataType_getSize(&tl);
    ck_assert_uint_ge(size, 1);

    UA_TrustListDataType_clear(&tl);
} END_TEST

START_TEST(trustlist_contains) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);
    tl.trustedCertificatesSize = 1;
    tl.trustedCertificates = UA_ByteString_new();
    tl.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    tl.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;

    UA_ByteString cert = UA_BYTESTRING("cert1");
    UA_Boolean found = UA_TrustListDataType_contains(&tl, &cert,
                                                       UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES);
    ck_assert(found);

    UA_ByteString notFound = UA_BYTESTRING("cert2");
    found = UA_TrustListDataType_contains(&tl, &notFound,
                                            UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES);
    ck_assert(!found);

    UA_TrustListDataType_clear(&tl);
} END_TEST

START_TEST(trustlist_add) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    src.trustedCertificatesSize = 1;
    src.trustedCertificates = UA_ByteString_new();
    src.trustedCertificates[0] = UA_BYTESTRING_ALLOC("newcert");
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;

    UA_StatusCode res = UA_TrustListDataType_add(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 1);

    /* Add another */
    UA_TrustListDataType src2;
    UA_TrustListDataType_init(&src2);
    src2.trustedCertificatesSize = 1;
    src2.trustedCertificates = UA_ByteString_new();
    src2.trustedCertificates[0] = UA_BYTESTRING_ALLOC("anothercert");
    src2.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    res = UA_TrustListDataType_add(&src2, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 2);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&src2);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_remove) {
    UA_TrustListDataType dst;
    UA_TrustListDataType_init(&dst);

    /* Add two certs */
    UA_TrustListDataType add;
    UA_TrustListDataType_init(&add);
    add.trustedCertificatesSize = 2;
    add.trustedCertificates = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    add.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    add.trustedCertificates[1] = UA_BYTESTRING_ALLOC("cert2");
    add.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    UA_TrustListDataType_add(&add, &dst);

    /* Remove one */
    UA_TrustListDataType rem;
    UA_TrustListDataType_init(&rem);
    rem.trustedCertificatesSize = 1;
    rem.trustedCertificates = UA_ByteString_new();
    rem.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    rem.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    UA_StatusCode res = UA_TrustListDataType_remove(&rem, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 1);

    UA_TrustListDataType_clear(&add);
    UA_TrustListDataType_clear(&rem);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_set) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    /* Set initial */
    src.trustedCertificatesSize = 1;
    src.trustedCertificates = UA_ByteString_new();
    src.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    UA_StatusCode res = UA_TrustListDataType_set(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Replace with new */
    UA_TrustListDataType src2;
    UA_TrustListDataType_init(&src2);
    src2.trustedCertificatesSize = 2;
    src2.trustedCertificates = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src2.trustedCertificates[0] = UA_BYTESTRING_ALLOC("newcert1");
    src2.trustedCertificates[1] = UA_BYTESTRING_ALLOC("newcert2");
    src2.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    res = UA_TrustListDataType_set(&src2, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 2);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&src2);
    UA_TrustListDataType_clear(&dst);
} END_TEST

/* ========== constantTimeEqual ========== */
START_TEST(constant_time_equal) {
    UA_Byte a[] = {1, 2, 3, 4};
    UA_Byte b[] = {1, 2, 3, 4};
    UA_Byte c[] = {1, 2, 3, 5};

    ck_assert(UA_constantTimeEqual(a, b, 4));
    ck_assert(!UA_constantTimeEqual(a, c, 4));
    ck_assert(UA_constantTimeEqual(a, b, 0));
} END_TEST

/* ========== ByteString_memZero ========== */
START_TEST(bytestring_memzero) {
    UA_ByteString bs = UA_BYTESTRING_ALLOC("secret data");
    UA_ByteString_memZero(&bs);
    /* All bytes should be zero */
    for(size_t i = 0; i < bs.length; i++)
        ck_assert_uint_eq(bs.data[i], 0);
    UA_ByteString_clear(&bs);
} END_TEST

/* ========== RelativePath_print ========== */
START_TEST(relativepath_print) {
    UA_RelativePath rp;
    UA_RelativePath_init(&rp);

    UA_RelativePathElement elem;
    UA_RelativePathElement_init(&elem);
    elem.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    elem.isInverse = false;
    elem.includeSubtypes = true;
    elem.targetName = UA_QUALIFIEDNAME(0, "Objects");

    rp.elementsSize = 1;
    rp.elements = &elem;

    UA_String out;
    UA_String_init(&out);
    UA_StatusCode res = UA_RelativePath_print(&rp, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);

    rp.elements = NULL;
    rp.elementsSize = 0;
} END_TEST

/* ========== ReadValueId_print ========== */
START_TEST(readvalueid_print) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_String out;
    UA_String_init(&out);
    UA_StatusCode res = UA_ReadValueId_print(&rvi, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(readvalueid_print_browsename) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_STRING(1, "myNode");
    rvi.attributeId = UA_ATTRIBUTEID_BROWSENAME;

    UA_String out;
    UA_String_init(&out);
    UA_StatusCode res = UA_ReadValueId_print(&rvi, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(readvalueid_print_with_indexrange) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, 85);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.indexRange = UA_STRING("0:2");

    UA_String out;
    UA_String_init(&out);
    UA_StatusCode res = UA_ReadValueId_print(&rvi, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

/* ========== SimpleAttributeOperand_print ========== */
START_TEST(simpleattroperand_print) {
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    sao.typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    sao.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_QualifiedName path[] = {UA_QUALIFIEDNAME(0, "Severity")};
    sao.browsePathSize = 1;
    sao.browsePath = path;

    UA_String out;
    UA_String_init(&out);
    UA_StatusCode res = UA_SimpleAttributeOperand_print(&sao, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);

    sao.browsePath = NULL;
    sao.browsePathSize = 0;
} END_TEST

/* ========== AttributeOperand_print ========== */
START_TEST(attroperand_print) {
    UA_AttributeOperand ao;
    UA_AttributeOperand_init(&ao);
    ao.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    ao.attributeId = UA_ATTRIBUTEID_BROWSENAME;

    UA_String out;
    UA_String_init(&out);
    UA_StatusCode res = UA_AttributeOperand_print(&ao, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

/* ========== TrustList with CRLs and IssuerCerts ========== */
START_TEST(trustlist_with_crls) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);

    tl.trustedCrlsSize = 1;
    tl.trustedCrls = UA_ByteString_new();
    tl.trustedCrls[0] = UA_BYTESTRING_ALLOC("crl1");
    tl.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCRLS;

    UA_UInt32 size = UA_TrustListDataType_getSize(&tl);
    ck_assert_uint_ge(size, 1);

    UA_ByteString crl = UA_BYTESTRING("crl1");
    UA_Boolean found = UA_TrustListDataType_contains(&tl, &crl,
                                                       UA_TRUSTLISTMASKS_TRUSTEDCRLS);
    ck_assert(found);

    UA_TrustListDataType_clear(&tl);
} END_TEST

START_TEST(trustlist_with_issuer_certs) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);

    tl.issuerCertificatesSize = 1;
    tl.issuerCertificates = UA_ByteString_new();
    tl.issuerCertificates[0] = UA_BYTESTRING_ALLOC("issuercert1");
    tl.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;

    UA_UInt32 size = UA_TrustListDataType_getSize(&tl);
    ck_assert_uint_ge(size, 1);

    UA_ByteString cert = UA_BYTESTRING("issuercert1");
    UA_Boolean found = UA_TrustListDataType_contains(&tl, &cert,
                                                       UA_TRUSTLISTMASKS_ISSUERCERTIFICATES);
    ck_assert(found);

    UA_TrustListDataType_clear(&tl);
} END_TEST

START_TEST(trustlist_with_issuer_crls) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);

    tl.issuerCrlsSize = 1;
    tl.issuerCrls = UA_ByteString_new();
    tl.issuerCrls[0] = UA_BYTESTRING_ALLOC("issuercrl1");
    tl.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCRLS;

    UA_UInt32 size = UA_TrustListDataType_getSize(&tl);
    ck_assert_uint_ge(size, 1);

    UA_ByteString crl = UA_BYTESTRING("issuercrl1");
    UA_Boolean found = UA_TrustListDataType_contains(&tl, &crl,
                                                       UA_TRUSTLISTMASKS_ISSUERCRLS);
    ck_assert(found);

    UA_TrustListDataType_clear(&tl);
} END_TEST

START_TEST(trustlist_add_issuer_certs) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    src.issuerCertificatesSize = 1;
    src.issuerCertificates = UA_ByteString_new();
    src.issuerCertificates[0] = UA_BYTESTRING_ALLOC("issuercert");
    src.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCERTIFICATES;

    UA_StatusCode res = UA_TrustListDataType_add(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.issuerCertificatesSize, 1);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_add_crls) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    src.trustedCrlsSize = 1;
    src.trustedCrls = UA_ByteString_new();
    src.trustedCrls[0] = UA_BYTESTRING_ALLOC("crl");
    src.issuerCrlsSize = 1;
    src.issuerCrls = UA_ByteString_new();
    src.issuerCrls[0] = UA_BYTESTRING_ALLOC("issuercrl");
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCRLS | UA_TRUSTLISTMASKS_ISSUERCRLS;

    UA_StatusCode res = UA_TrustListDataType_add(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCrlsSize, 1);
    ck_assert_uint_eq(dst.issuerCrlsSize, 1);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&dst);
} END_TEST

int main(void) {
    Suite *s = suite_create("Util Ext2");

    TCase *tc_url = tcase_create("ParseURL");
    tcase_add_test(tc_url, parse_url_tcp);
    tcase_add_test(tc_url, parse_url_tcp_noport);
    tcase_add_test(tc_url, parse_url_tcp_nopath);
    tcase_add_test(tc_url, parse_url_ipv6);
    tcase_add_test(tc_url, parse_url_ipv6_noport);
    tcase_add_test(tc_url, parse_url_invalid);
    tcase_add_test(tc_url, parse_url_https);
    tcase_add_test(tc_url, parse_url_wss);
    tcase_add_test(tc_url, parse_url_ethernet);
    tcase_add_test(tc_url, parse_url_ethernet_vlan);
    tcase_add_test(tc_url, parse_url_ethernet_invalid);
    suite_add_tcase(s, tc_url);

    TCase *tc_base64 = tcase_create("Base64");
    tcase_add_test(tc_base64, base64_roundtrip);
    tcase_add_test(tc_base64, base64_empty);
    tcase_add_test(tc_base64, base64_from_empty_string);
    suite_add_tcase(s, tc_base64);

    TCase *tc_attr = tcase_create("AttributeId");
    tcase_add_test(tc_attr, attributeid_name);
    tcase_add_test(tc_attr, attributeid_from_name);
    suite_add_tcase(s, tc_attr);

    TCase *tc_num = tcase_create("ReadNumber");
    tcase_add_test(tc_num, read_number_decimal);
    tcase_add_test(tc_num, read_number_hex);
    tcase_add_test(tc_num, read_number_hex_lower);
    tcase_add_test(tc_num, read_number_empty);
    suite_add_tcase(s, tc_num);

    TCase *tc_kvm = tcase_create("KeyValueMap");
    tcase_add_test(tc_kvm, kvm_lifecycle);
    tcase_add_test(tc_kvm, kvm_set_shallow);
    tcase_add_test(tc_kvm, kvm_copy);
    tcase_add_test(tc_kvm, kvm_merge);
    tcase_add_test(tc_kvm, kvm_merge_overlapping);
    tcase_add_test(tc_kvm, kvm_null_checks);
    suite_add_tcase(s, tc_kvm);

    TCase *tc_tl = tcase_create("TrustList");
    tcase_add_test(tc_tl, trustlist_getsize);
    tcase_add_test(tc_tl, trustlist_contains);
    tcase_add_test(tc_tl, trustlist_add);
    tcase_add_test(tc_tl, trustlist_remove);
    tcase_add_test(tc_tl, trustlist_set);
    tcase_add_test(tc_tl, trustlist_with_crls);
    tcase_add_test(tc_tl, trustlist_with_issuer_certs);
    tcase_add_test(tc_tl, trustlist_with_issuer_crls);
    tcase_add_test(tc_tl, trustlist_add_issuer_certs);
    tcase_add_test(tc_tl, trustlist_add_crls);
    suite_add_tcase(s, tc_tl);

    TCase *tc_misc = tcase_create("Misc");
    tcase_add_test(tc_misc, constant_time_equal);
    tcase_add_test(tc_misc, bytestring_memzero);
    tcase_add_test(tc_misc, relativepath_print);
    tcase_add_test(tc_misc, readvalueid_print);
    tcase_add_test(tc_misc, readvalueid_print_browsename);
    tcase_add_test(tc_misc, readvalueid_print_with_indexrange);
    tcase_add_test(tc_misc, simpleattroperand_print);
    tcase_add_test(tc_misc, attroperand_print);
    suite_add_tcase(s, tc_misc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int fails = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (fails == 0) ? 0 : 1;
}
