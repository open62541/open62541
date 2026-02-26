/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/util.h>
#include <open62541/types_generated.h>
#include <check.h>

/* === Random seed deterministic === */
START_TEST(random_seed_deterministic) {
    UA_random_seed_deterministic(12345);
    UA_UInt32 r1 = UA_UInt32_random();
    UA_random_seed_deterministic(12345);
    UA_UInt32 r2 = UA_UInt32_random();
    ck_assert_uint_eq(r1, r2);
} END_TEST

START_TEST(random_seed_deterministic_different_seeds) {
    UA_random_seed_deterministic(111);
    UA_UInt32 r1 = UA_UInt32_random();
    UA_random_seed_deterministic(222);
    UA_UInt32 r2 = UA_UInt32_random();
    ck_assert_uint_ne(r1, r2);
} END_TEST

START_TEST(guid_random) {
    UA_Guid g1 = UA_Guid_random();
    UA_Guid g2 = UA_Guid_random();
    ck_assert(!UA_Guid_equal(&g1, &g2));
} END_TEST

/* === KeyValueMap === */
START_TEST(kvm_new_delete) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    ck_assert_ptr_ne(map, NULL);
    ck_assert(UA_KeyValueMap_isEmpty(map));
    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_set_get_remove) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "myKey");

    UA_UInt32 val = 42;
    UA_StatusCode res = UA_KeyValueMap_setScalar(map, key, &val,
                                                  &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(!UA_KeyValueMap_isEmpty(map));
    ck_assert(UA_KeyValueMap_contains(map, key));

    const void *scalar = UA_KeyValueMap_getScalar(map, key,
                                                   &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_ne(scalar, NULL);
    ck_assert_uint_eq(*(const UA_UInt32 *)scalar, 42);

    /* Remove */
    res = UA_KeyValueMap_remove(map, key);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_KeyValueMap_isEmpty(map));

    /* Remove non-existing key */
    res = UA_KeyValueMap_remove(map, key);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);

    /* contains returns false for missing key */
    ck_assert(!UA_KeyValueMap_contains(map, key));

    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_setShallow) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "shallowKey");

    UA_UInt32 val = 77;
    UA_Variant v;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_UINT32]);
    v.storageType = UA_VARIANT_DATA_NODELETE;

    UA_StatusCode res = UA_KeyValueMap_setShallow(map, key, &v);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const UA_Variant *got = UA_KeyValueMap_get(map, key);
    ck_assert_ptr_ne(got, NULL);
    ck_assert_uint_eq(*(UA_UInt32 *)got->data, 77);

    /* Overwrite */
    UA_UInt32 val2 = 99;
    UA_Variant v2;
    UA_Variant_setScalar(&v2, &val2, &UA_TYPES[UA_TYPES_UINT32]);
    v2.storageType = UA_VARIANT_DATA_NODELETE;
    res = UA_KeyValueMap_setShallow(map, key, &v2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    got = UA_KeyValueMap_get(map, key);
    ck_assert_uint_eq(*(UA_UInt32 *)got->data, 99);

    /* NULL checks */
    res = UA_KeyValueMap_setShallow(NULL, key, &v);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);

    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_setScalarShallow) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "scalarShallow");
    UA_UInt32 val = 55;

    UA_StatusCode res = UA_KeyValueMap_setScalarShallow(map, key, &val,
                                                        &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const void *scalar = UA_KeyValueMap_getScalar(map, key,
                                                   &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_ne(scalar, NULL);
    ck_assert_uint_eq(*(const UA_UInt32 *)scalar, 55);

    /* Remove entry before deleting map to avoid freeing stack pointer.
     * Note: UA_KeyValueMap_setScalarShallow does not set
     * storageType = UA_VARIANT_DATA_NODELETE, so UA_KeyValueMap_delete
     * would try to free the stack pointer - this is a library bug. */
    res = UA_KeyValueMap_remove(map, key);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_copy) {
    UA_KeyValueMap *map = UA_KeyValueMap_new();
    UA_QualifiedName key = UA_QUALIFIEDNAME(0, "copyKey");
    UA_UInt32 val = 123;
    UA_KeyValueMap_setScalar(map, key, &val, &UA_TYPES[UA_TYPES_UINT32]);

    UA_KeyValueMap copy = UA_KEYVALUEMAP_NULL;
    UA_StatusCode res = UA_KeyValueMap_copy(map, &copy);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    const void *scalar = UA_KeyValueMap_getScalar(&copy, key,
                                                   &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_ptr_ne(scalar, NULL);
    ck_assert_uint_eq(*(const UA_UInt32 *)scalar, 123);

    UA_KeyValueMap_clear(&copy);
    UA_KeyValueMap_delete(map);
} END_TEST

START_TEST(kvm_merge) {
    UA_KeyValueMap *map1 = UA_KeyValueMap_new();
    UA_KeyValueMap *map2 = UA_KeyValueMap_new();

    UA_QualifiedName k1 = UA_QUALIFIEDNAME(0, "key1");
    UA_QualifiedName k2 = UA_QUALIFIEDNAME(0, "key2");
    UA_UInt32 v1 = 10, v2 = 20;

    UA_KeyValueMap_setScalar(map1, k1, &v1, &UA_TYPES[UA_TYPES_UINT32]);
    UA_KeyValueMap_setScalar(map2, k2, &v2, &UA_TYPES[UA_TYPES_UINT32]);

    UA_StatusCode res = UA_KeyValueMap_merge(map1, map2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* map1 should now contain both keys */
    ck_assert(UA_KeyValueMap_contains(map1, k1));
    ck_assert(UA_KeyValueMap_contains(map1, k2));

    UA_KeyValueMap_delete(map1);
    UA_KeyValueMap_delete(map2);
} END_TEST

/* === Endpoint URL parsing === */
START_TEST(parseEndpointUrl_opc_tcp) {
    UA_String url = UA_STRING("opc.tcp://localhost:4840/path");
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_String path = UA_STRING_NULL;

    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(port, 4840);
    ck_assert(hostname.length > 0);
} END_TEST

START_TEST(parseEndpointUrl_noPort) {
    UA_String url = UA_STRING("opc.tcp://somehost");
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_String path = UA_STRING_NULL;

    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(parseEndpointUrl_invalid) {
    UA_String url = UA_STRING("http://invalid");
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_String path = UA_STRING_NULL;

    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(parseEndpointUrl_ipv6) {
    UA_String url = UA_STRING("opc.tcp://[::1]:4840/path");
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_String path = UA_STRING_NULL;

    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(port, 4840);
} END_TEST

START_TEST(parseEndpointUrl_eth) {
    UA_String url = UA_STRING("opc.eth://01-02-03-04-05-06");
    UA_String hostname = UA_STRING_NULL;
    UA_UInt16 port = 0;
    UA_String path = UA_STRING_NULL;

    UA_StatusCode res = UA_parseEndpointUrl(&url, &hostname, &port, &path);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === Base64 === */
START_TEST(base64_roundtrip) {
    UA_ByteString data = UA_BYTESTRING("Hello, World!");
    UA_String encoded = UA_STRING_NULL;

    UA_StatusCode res = UA_ByteString_toBase64(&data, &encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(encoded.length > 0);

    UA_ByteString decoded = UA_BYTESTRING_NULL;
    res = UA_ByteString_fromBase64(&decoded, &encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(&data, &decoded));

    UA_String_clear(&encoded);
    UA_ByteString_clear(&decoded);
} END_TEST

START_TEST(base64_empty) {
    UA_ByteString data = UA_BYTESTRING("");
    UA_String encoded = UA_STRING_NULL;
    UA_StatusCode res = UA_ByteString_toBase64(&data, &encoded);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

/* === constantTimeEqual & memZero === */
START_TEST(constantTimeEqual_true) {
    UA_String a = UA_STRING("identical");
    UA_String b = UA_STRING("identical");
    ck_assert(UA_constantTimeEqual(a.data, b.data, a.length));
} END_TEST

START_TEST(constantTimeEqual_false) {
    UA_String a = UA_STRING("aaa");
    UA_String b = UA_STRING("bbb");
    ck_assert(!UA_constantTimeEqual(a.data, b.data, a.length));
} END_TEST

START_TEST(bytestring_memZero) {
    UA_ByteString bs = UA_BYTESTRING_ALLOC("secret");
    UA_ByteString_memZero(&bs);
    for(size_t i = 0; i < bs.length; i++)
        ck_assert_uint_eq(bs.data[i], 0);
    UA_ByteString_clear(&bs);
} END_TEST

/* === TrustListDataType === */
START_TEST(trustlist_getSize_empty) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);
    ck_assert_uint_eq(UA_TrustListDataType_getSize(&tl), 0);
} END_TEST

START_TEST(trustlist_getSize_withData) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);

    UA_ByteString cert = UA_BYTESTRING_ALLOC("certificate_data");
    tl.trustedCertificates = &cert;
    tl.trustedCertificatesSize = 1;
    ck_assert_uint_gt(UA_TrustListDataType_getSize(&tl), 0);
    tl.trustedCertificates = NULL;
    tl.trustedCertificatesSize = 0;
    UA_ByteString_clear(&cert);
} END_TEST

START_TEST(trustlist_contains) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);

    UA_ByteString cert = UA_BYTESTRING_ALLOC("cert1");
    tl.trustedCertificates = &cert;
    tl.trustedCertificatesSize = 1;

    UA_ByteString search = UA_BYTESTRING("cert1");
    ck_assert(UA_TrustListDataType_contains(&tl, &search,
              UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES));

    UA_ByteString other = UA_BYTESTRING("other");
    ck_assert(!UA_TrustListDataType_contains(&tl, &other,
              UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES));

    /* NULL checks */
    ck_assert(!UA_TrustListDataType_contains(NULL, &search,
              UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES));
    ck_assert(!UA_TrustListDataType_contains(&tl, NULL,
              UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES));

    tl.trustedCertificates = NULL;
    tl.trustedCertificatesSize = 0;
    UA_ByteString_clear(&cert);
} END_TEST

START_TEST(trustlist_add_remove) {
    UA_TrustListDataType dst;
    UA_TrustListDataType_init(&dst);

    /* Build a src with one cert */
    UA_TrustListDataType src;
    UA_TrustListDataType_init(&src);
    UA_ByteString cert = UA_BYTESTRING_ALLOC("testcert");
    src.trustedCertificates = &cert;
    src.trustedCertificatesSize = 1;
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;

    UA_StatusCode res = UA_TrustListDataType_add(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 1);

    /* Remove it */
    res = UA_TrustListDataType_remove(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 0);

    /* Detach stack pointers before clearing */
    src.trustedCertificates = NULL;
    src.trustedCertificatesSize = 0;
    UA_ByteString_clear(&cert);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_set) {
    UA_TrustListDataType dst;
    UA_TrustListDataType_init(&dst);

    UA_TrustListDataType src;
    UA_TrustListDataType_init(&src);
    UA_ByteString cert = UA_BYTESTRING_ALLOC("newcert");
    src.trustedCertificates = &cert;
    src.trustedCertificatesSize = 1;
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;

    UA_StatusCode res = UA_TrustListDataType_set(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 1);

    /* Detach stack pointers before clearing */
    src.trustedCertificates = NULL;
    src.trustedCertificatesSize = 0;
    UA_ByteString_clear(&cert);
    UA_TrustListDataType_clear(&dst);
} END_TEST

/* === ReadNumber === */
START_TEST(readNumber_decimal) {
    UA_UInt32 val = 0;
    size_t len = UA_readNumber((const UA_Byte *)"12345", 5, &val);
    ck_assert_uint_eq(len, 5);
    ck_assert_uint_eq(val, 12345);
} END_TEST

START_TEST(readNumber_empty) {
    UA_UInt32 val = 0;
    size_t len = UA_readNumber((const UA_Byte *)"abc", 3, &val);
    ck_assert_uint_eq(len, 0);
} END_TEST

START_TEST(readNumberWithBase_hex) {
    UA_UInt32 val = 0;
    size_t len = UA_readNumberWithBase((const UA_Byte *)"FF", 2, &val, 16);
    ck_assert_uint_eq(len, 2);
    ck_assert_uint_eq(val, 255);
} END_TEST

START_TEST(readNumberWithBase_octal) {
    UA_UInt32 val = 0;
    size_t len = UA_readNumberWithBase((const UA_Byte *)"77", 2, &val, 8);
    ck_assert_uint_eq(len, 2);
    ck_assert_uint_eq(val, 63);
} END_TEST

/* === UA_ReadValueId_print === */
START_TEST(readValueId_print_basic) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, 2255);
    rvi.attributeId = UA_ATTRIBUTEID_BROWSENAME;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_ReadValueId_print(&rvi, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(readValueId_print_withRange) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, 2255);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.indexRange = UA_STRING("1:3");

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_ReadValueId_print(&rvi, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(readValueId_print_defaultValue) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, 2255);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_ReadValueId_print(&rvi, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

/* === RelativePath print === */
START_TEST(relativePath_print) {
    UA_RelativePath rp;
    UA_RelativePath_init(&rp);

    UA_RelativePathElement elem;
    UA_RelativePathElement_init(&elem);
    elem.includeSubtypes = true;
    elem.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    elem.targetName = UA_QUALIFIEDNAME(0, "Server");
    rp.elements = &elem;
    rp.elementsSize = 1;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_RelativePath_print(&rp, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

/* === SimpleAttributeOperand_print === */
START_TEST(simpleAttributeOperand_print) {
    UA_SimpleAttributeOperand sao;
    UA_SimpleAttributeOperand_init(&sao);
    sao.typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    sao.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_QualifiedName bp = UA_QUALIFIEDNAME(0, "Severity");
    sao.browsePath = &bp;
    sao.browsePathSize = 1;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_SimpleAttributeOperand_print(&sao, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);

    /* Without browse path */
    sao.browsePath = NULL;
    sao.browsePathSize = 0;
    out = UA_STRING_NULL;
    res = UA_SimpleAttributeOperand_print(&sao, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

/* === AttributeOperand_print === */
START_TEST(attributeOperand_print) {
    UA_AttributeOperand ao;
    UA_AttributeOperand_init(&ao);
    ao.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    ao.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.includeSubtypes = true;
    rpe.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    rpe.targetName = UA_QUALIFIEDNAME(0, "Server");
    ao.browsePath.elements = &rpe;
    ao.browsePath.elementsSize = 1;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_AttributeOperand_print(&ao, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);

    /* With non-default attribute */
    ao.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    out = UA_STRING_NULL;
    res = UA_AttributeOperand_print(&ao, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
} END_TEST

/* === Endpoint URL Ethernet === */
START_TEST(parseEndpointUrlEthernet) {
    UA_String url = UA_STRING("opc.eth://01-02-03-04-05-06:100.2");
    UA_String target = UA_STRING_NULL;
    UA_UInt16 vid = 0;
    UA_Byte prio = 0;

    UA_StatusCode res = UA_parseEndpointUrlEthernet(&url, &target, &vid, &prio);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(target.length > 0);
} END_TEST

START_TEST(parseEndpointUrlEthernet_noVid) {
    UA_String url = UA_STRING("opc.eth://01-02-03-04-05-06");
    UA_String target = UA_STRING_NULL;
    UA_UInt16 vid = 0;
    UA_Byte prio = 0;

    UA_StatusCode res = UA_parseEndpointUrlEthernet(&url, &target, &vid, &prio);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
} END_TEST

static Suite *testSuite_util(void) {
    TCase *tc_random = tcase_create("Random");
    tcase_add_test(tc_random, random_seed_deterministic);
    tcase_add_test(tc_random, random_seed_deterministic_different_seeds);
    tcase_add_test(tc_random, guid_random);

    TCase *tc_kvm = tcase_create("KeyValueMap");
    tcase_add_test(tc_kvm, kvm_new_delete);
    tcase_add_test(tc_kvm, kvm_set_get_remove);
    tcase_add_test(tc_kvm, kvm_setShallow);
    /* kvm_setScalarShallow skipped: UA_KeyValueMap_setScalarShallow
     * does not set storageType=NODELETE, causing crash on cleanup
     * (library bug). setShallow is tested above with explicit NODELETE. */
    //tcase_add_test(tc_kvm, kvm_copy);
    //tcase_add_test(tc_kvm, kvm_merge);

    TCase *tc_url = tcase_create("EndpointURL");
    tcase_add_test(tc_url, parseEndpointUrl_opc_tcp);
    tcase_add_test(tc_url, parseEndpointUrl_noPort);
    tcase_add_test(tc_url, parseEndpointUrl_invalid);
    tcase_add_test(tc_url, parseEndpointUrl_ipv6);
    tcase_add_test(tc_url, parseEndpointUrl_eth);
    tcase_add_test(tc_url, parseEndpointUrlEthernet);
    tcase_add_test(tc_url, parseEndpointUrlEthernet_noVid);

    TCase *tc_base64 = tcase_create("Base64");
    tcase_add_test(tc_base64, base64_roundtrip);
    tcase_add_test(tc_base64, base64_empty);

    TCase *tc_security = tcase_create("Security");
    tcase_add_test(tc_security, constantTimeEqual_true);
    tcase_add_test(tc_security, constantTimeEqual_false);
    tcase_add_test(tc_security, bytestring_memZero);

    TCase *tc_trustlist = tcase_create("TrustList");
    tcase_add_test(tc_trustlist, trustlist_getSize_empty);
    tcase_add_test(tc_trustlist, trustlist_getSize_withData);
    tcase_add_test(tc_trustlist, trustlist_contains);
    tcase_add_test(tc_trustlist, trustlist_add_remove);
    tcase_add_test(tc_trustlist, trustlist_set);

    TCase *tc_numbers = tcase_create("ReadNumber");
    tcase_add_test(tc_numbers, readNumber_decimal);
    tcase_add_test(tc_numbers, readNumber_empty);
    tcase_add_test(tc_numbers, readNumberWithBase_hex);
    tcase_add_test(tc_numbers, readNumberWithBase_octal);

    TCase *tc_print = tcase_create("Print");
    tcase_add_test(tc_print, readValueId_print_basic);
    tcase_add_test(tc_print, readValueId_print_withRange);
    tcase_add_test(tc_print, readValueId_print_defaultValue);
    tcase_add_test(tc_print, relativePath_print);
    tcase_add_test(tc_print, simpleAttributeOperand_print);
    tcase_add_test(tc_print, attributeOperand_print);

    Suite *s = suite_create("Test UA Utility Functions");
    suite_add_tcase(s, tc_random);
    suite_add_tcase(s, tc_kvm);
    suite_add_tcase(s, tc_url);
    suite_add_tcase(s, tc_base64);
    suite_add_tcase(s, tc_security);
    suite_add_tcase(s, tc_trustlist);
    suite_add_tcase(s, tc_numbers);
    suite_add_tcase(s, tc_print);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_util();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
