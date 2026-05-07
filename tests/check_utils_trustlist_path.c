/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* TrustListDataType, KeyValueMap and utility function tests */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>
#include <string.h>

/* === TrustListDataType operations === */
START_TEST(trustlist_add_empty) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    /* Add empty to empty */
    UA_StatusCode res = UA_TrustListDataType_add(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_add_with_certs) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    /* Set up src with trustedCertificates */
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES |
                         UA_TRUSTLISTMASKS_TRUSTEDCRLS |
                         UA_TRUSTLISTMASKS_ISSUERCERTIFICATES |
                         UA_TRUSTLISTMASKS_ISSUERCRLS;

    src.trustedCertificatesSize = 2;
    src.trustedCertificates = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    src.trustedCertificates[1] = UA_BYTESTRING_ALLOC("cert2");

    src.trustedCrlsSize = 1;
    src.trustedCrls = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.trustedCrls[0] = UA_BYTESTRING_ALLOC("crl1");

    src.issuerCertificatesSize = 1;
    src.issuerCertificates = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.issuerCertificates[0] = UA_BYTESTRING_ALLOC("issuer1");

    src.issuerCrlsSize = 1;
    src.issuerCrls = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.issuerCrls[0] = UA_BYTESTRING_ALLOC("issuercrl1");

    UA_StatusCode res = UA_TrustListDataType_add(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 2);
    ck_assert_uint_eq(dst.trustedCrlsSize, 1);
    ck_assert_uint_eq(dst.issuerCertificatesSize, 1);
    ck_assert_uint_eq(dst.issuerCrlsSize, 1);

    /* Add more to existing */
    UA_TrustListDataType src2;
    UA_TrustListDataType_init(&src2);
    src2.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    src2.trustedCertificatesSize = 1;
    src2.trustedCertificates = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src2.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert3");

    res = UA_TrustListDataType_add(&src2, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 3);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&src2);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_set) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    /* Set up dst with some data first */
    dst.trustedCertificatesSize = 1;
    dst.trustedCertificates = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    dst.trustedCertificates[0] = UA_BYTESTRING_ALLOC("old_cert");

    /* Set up src */
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    src.trustedCertificatesSize = 2;
    src.trustedCertificates = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.trustedCertificates[0] = UA_BYTESTRING_ALLOC("new_cert1");
    src.trustedCertificates[1] = UA_BYTESTRING_ALLOC("new_cert2");

    UA_StatusCode res = UA_TrustListDataType_set(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 2);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_remove) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    /* Set up dst with 3 certs */
    dst.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES |
                         UA_TRUSTLISTMASKS_TRUSTEDCRLS;
    dst.trustedCertificatesSize = 3;
    dst.trustedCertificates = (UA_ByteString *)UA_Array_new(3, &UA_TYPES[UA_TYPES_BYTESTRING]);
    dst.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    dst.trustedCertificates[1] = UA_BYTESTRING_ALLOC("cert2");
    dst.trustedCertificates[2] = UA_BYTESTRING_ALLOC("cert3");

    dst.trustedCrlsSize = 2;
    dst.trustedCrls = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    dst.trustedCrls[0] = UA_BYTESTRING_ALLOC("crl1");
    dst.trustedCrls[1] = UA_BYTESTRING_ALLOC("crl2");

    /* Remove cert2 and crl1 */
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES |
                         UA_TRUSTLISTMASKS_TRUSTEDCRLS;
    src.trustedCertificatesSize = 1;
    src.trustedCertificates = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert2");

    src.trustedCrlsSize = 1;
    src.trustedCrls = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.trustedCrls[0] = UA_BYTESTRING_ALLOC("crl1");

    UA_StatusCode res = UA_TrustListDataType_remove(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 2);
    ck_assert_uint_eq(dst.trustedCrlsSize, 1);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_remove_issuer) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    /* Set up dst with issuer certs and CRLs */
    dst.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCERTIFICATES |
                         UA_TRUSTLISTMASKS_ISSUERCRLS;
    dst.issuerCertificatesSize = 2;
    dst.issuerCertificates = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    dst.issuerCertificates[0] = UA_BYTESTRING_ALLOC("issuer1");
    dst.issuerCertificates[1] = UA_BYTESTRING_ALLOC("issuer2");

    dst.issuerCrlsSize = 2;
    dst.issuerCrls = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    dst.issuerCrls[0] = UA_BYTESTRING_ALLOC("issuercrl1");
    dst.issuerCrls[1] = UA_BYTESTRING_ALLOC("issuercrl2");

    /* Remove issuer1 and issuercrl2 */
    src.specifiedLists = UA_TRUSTLISTMASKS_ISSUERCERTIFICATES |
                         UA_TRUSTLISTMASKS_ISSUERCRLS;
    src.issuerCertificatesSize = 1;
    src.issuerCertificates = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.issuerCertificates[0] = UA_BYTESTRING_ALLOC("issuer1");

    src.issuerCrlsSize = 1;
    src.issuerCrls = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.issuerCrls[0] = UA_BYTESTRING_ALLOC("issuercrl2");

    UA_StatusCode res = UA_TrustListDataType_remove(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.issuerCertificatesSize, 1);
    ck_assert_uint_eq(dst.issuerCrlsSize, 1);

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_remove_nonexistent) {
    UA_TrustListDataType src, dst;
    UA_TrustListDataType_init(&src);
    UA_TrustListDataType_init(&dst);

    /* Set up dst with 1 cert */
    dst.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    dst.trustedCertificatesSize = 1;
    dst.trustedCertificates = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    dst.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");

    /* Try removing something that doesn't exist */
    src.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    src.trustedCertificatesSize = 1;
    src.trustedCertificates = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
    src.trustedCertificates[0] = UA_BYTESTRING_ALLOC("nonexistent");

    UA_StatusCode res = UA_TrustListDataType_remove(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.trustedCertificatesSize, 1); /* Unchanged */

    UA_TrustListDataType_clear(&src);
    UA_TrustListDataType_clear(&dst);
} END_TEST

START_TEST(trustlist_contains) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);

    tl.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    tl.trustedCertificatesSize = 2;
    tl.trustedCertificates = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    tl.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    tl.trustedCertificates[1] = UA_BYTESTRING_ALLOC("cert2");

    UA_ByteString cert = UA_BYTESTRING("cert1");
    ck_assert(UA_TrustListDataType_contains(&tl, &cert, UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES));

    UA_ByteString notCert = UA_BYTESTRING("nope");
    ck_assert(!UA_TrustListDataType_contains(&tl, &notCert, UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES));

    UA_TrustListDataType_clear(&tl);
} END_TEST

START_TEST(trustlist_getSize) {
    UA_TrustListDataType tl;
    UA_TrustListDataType_init(&tl);
    UA_UInt32 size0 = UA_TrustListDataType_getSize(&tl);
    ck_assert_uint_eq(size0, 0);

    /* Add real data to get a non-zero size */
    tl.specifiedLists = UA_TRUSTLISTMASKS_TRUSTEDCERTIFICATES;
    tl.trustedCertificatesSize = 2;
    tl.trustedCertificates = (UA_ByteString *)UA_Array_new(2, &UA_TYPES[UA_TYPES_BYTESTRING]);
    tl.trustedCertificates[0] = UA_BYTESTRING_ALLOC("cert1");
    tl.trustedCertificates[1] = UA_BYTESTRING_ALLOC("cert2");

    UA_UInt32 size1 = UA_TrustListDataType_getSize(&tl);
    ck_assert(size1 > 0);

    UA_TrustListDataType_clear(&tl);
} END_TEST

/* === KeyValueMap merge === */
START_TEST(kvm_merge) {
    UA_KeyValueMap *lhs = UA_KeyValueMap_new();
    UA_KeyValueMap *rhs = UA_KeyValueMap_new();

    /* Add entries to both */
    UA_Int32 val1 = 10;
    UA_KeyValueMap_setScalar(lhs, UA_QUALIFIEDNAME(0, "key1"),
                             &val1, &UA_TYPES[UA_TYPES_INT32]);

    UA_Int32 val2 = 20;
    UA_KeyValueMap_setScalar(rhs, UA_QUALIFIEDNAME(0, "key2"),
                             &val2, &UA_TYPES[UA_TYPES_INT32]);

    /* Merge rhs into lhs */
    UA_StatusCode res = UA_KeyValueMap_merge(lhs, rhs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* lhs should have both keys */
    ck_assert(UA_KeyValueMap_contains(lhs, UA_QUALIFIEDNAME(0, "key1")));
    ck_assert(UA_KeyValueMap_contains(lhs, UA_QUALIFIEDNAME(0, "key2")));

    UA_KeyValueMap_delete(lhs);
    UA_KeyValueMap_delete(rhs);
} END_TEST

START_TEST(kvm_merge_overlapping) {
    UA_KeyValueMap *lhs = UA_KeyValueMap_new();
    UA_KeyValueMap *rhs = UA_KeyValueMap_new();

    UA_Int32 val1 = 10, val2 = 20;
    UA_KeyValueMap_setScalar(lhs, UA_QUALIFIEDNAME(0, "key"),
                             &val1, &UA_TYPES[UA_TYPES_INT32]);
    UA_KeyValueMap_setScalar(rhs, UA_QUALIFIEDNAME(0, "key"),
                             &val2, &UA_TYPES[UA_TYPES_INT32]);

    /* Merge - same key, rhs should overwrite */
    UA_StatusCode res = UA_KeyValueMap_merge(lhs, rhs);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_KeyValueMap_delete(lhs);
    UA_KeyValueMap_delete(rhs);
} END_TEST

/* === RelativePath print === */
START_TEST(relativepath_print_simple) {
    UA_RelativePath rp;
    UA_RelativePath_init(&rp);

    UA_RelativePathElement elem;
    UA_RelativePathElement_init(&elem);
    elem.targetName = UA_QUALIFIEDNAME(0, "TestNode");
    elem.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HIERARCHICALREFERENCES);
    elem.includeSubtypes = true;

    rp.elements = &elem;
    rp.elementsSize = 1;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_RelativePath_print(&rp, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(relativepath_print_multi) {
    UA_RelativePath rp;
    UA_RelativePath_init(&rp);

    UA_RelativePathElement elems[3];
    UA_RelativePathElement_init(&elems[0]);
    UA_RelativePathElement_init(&elems[1]);
    UA_RelativePathElement_init(&elems[2]);

    elems[0].targetName = UA_QUALIFIEDNAME(0, "Objects");
    elems[0].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    elems[0].includeSubtypes = true;

    elems[1].targetName = UA_QUALIFIEDNAME(0, "Server");
    elems[1].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    elems[1].includeSubtypes = false;

    elems[2].targetName = UA_QUALIFIEDNAME(1, "CustomObj");
    elems[2].referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    elems[2].includeSubtypes = true;

    rp.elements = elems;
    rp.elementsSize = 3;

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_RelativePath_print(&rp, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
} END_TEST

START_TEST(relativepath_print_empty) {
    UA_RelativePath rp;
    UA_RelativePath_init(&rp);

    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_RelativePath_print(&rp, &out);
    /* Empty path - should succeed or fail gracefully */
    (void)res;
    UA_String_clear(&out);
} END_TEST

/* === BrowsePath parse === */
START_TEST(browsepath_parse) {
    UA_QualifiedName qn;
    UA_QualifiedName_init(&qn);
    UA_StatusCode res = UA_QualifiedName_parse(&qn, UA_STRING("1:TestName"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(qn.namespaceIndex, 1);
    UA_QualifiedName_clear(&qn);
} END_TEST

START_TEST(browsepath_parse_default_ns) {
    UA_QualifiedName qn;
    UA_QualifiedName_init(&qn);
    UA_StatusCode res = UA_QualifiedName_parse(&qn, UA_STRING("TestName"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(qn.namespaceIndex, 0);
    UA_QualifiedName_clear(&qn);
} END_TEST

/* === NodeId parse/print roundtrip === */
START_TEST(nodeid_parse_print_numeric) {
    UA_NodeId id;
    UA_StatusCode res = UA_NodeId_parse(&id, UA_STRING("ns=2;i=1234"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(id.namespaceIndex, 2);
    ck_assert_uint_eq(id.identifier.numeric, 1234);

    UA_String out = UA_STRING_NULL;
    res = UA_NodeId_print(&id, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(out.length > 0);
    UA_String_clear(&out);
    UA_NodeId_clear(&id);
} END_TEST

START_TEST(nodeid_parse_print_string) {
    UA_NodeId id;
    UA_StatusCode res = UA_NodeId_parse(&id, UA_STRING("ns=3;s=MyNode.Name"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(id.namespaceIndex, 3);

    UA_String out = UA_STRING_NULL;
    res = UA_NodeId_print(&id, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
    UA_NodeId_clear(&id);
} END_TEST

START_TEST(nodeid_parse_print_guid) {
    UA_NodeId id;
    UA_StatusCode res = UA_NodeId_parse(&id,
        UA_STRING("ns=1;g=09087e75-8e5e-499b-954f-f2a9603db28a"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(id.identifierType, UA_NODEIDTYPE_GUID);

    UA_String out = UA_STRING_NULL;
    res = UA_NodeId_print(&id, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
    UA_NodeId_clear(&id);
} END_TEST

START_TEST(nodeid_parse_print_bytestring) {
    UA_NodeId id;
    UA_StatusCode res = UA_NodeId_parse(&id, UA_STRING("ns=1;b=YWJj"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(id.identifierType, UA_NODEIDTYPE_BYTESTRING);

    UA_String out = UA_STRING_NULL;
    res = UA_NodeId_print(&id, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
    UA_NodeId_clear(&id);
} END_TEST

/* === ExpandedNodeId parse/print === */
START_TEST(expandednodeid_parse_print) {
    UA_ExpandedNodeId eid;
    UA_StatusCode res = UA_ExpandedNodeId_parse(&eid,
        UA_STRING("ns=2;i=5555"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_String out = UA_STRING_NULL;
    res = UA_ExpandedNodeId_print(&eid, &out);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_String_clear(&out);
    UA_ExpandedNodeId_clear(&eid);
} END_TEST

/* === Suite definition === */
static Suite *testSuite_utilExt(void) {
    TCase *tc_tl = tcase_create("TrustList");
    tcase_add_test(tc_tl, trustlist_add_empty);
    tcase_add_test(tc_tl, trustlist_add_with_certs);
    tcase_add_test(tc_tl, trustlist_set);
    tcase_add_test(tc_tl, trustlist_remove);
    tcase_add_test(tc_tl, trustlist_remove_issuer);
    tcase_add_test(tc_tl, trustlist_remove_nonexistent);
    tcase_add_test(tc_tl, trustlist_contains);
    tcase_add_test(tc_tl, trustlist_getSize);

    TCase *tc_kvm = tcase_create("KVMerge");
    tcase_add_test(tc_kvm, kvm_merge);
    tcase_add_test(tc_kvm, kvm_merge_overlapping);

    TCase *tc_path = tcase_create("PathOps");
    tcase_add_test(tc_path, relativepath_print_simple);
    tcase_add_test(tc_path, relativepath_print_multi);
    tcase_add_test(tc_path, relativepath_print_empty);
    tcase_add_test(tc_path, browsepath_parse);
    tcase_add_test(tc_path, browsepath_parse_default_ns);
    tcase_add_test(tc_path, nodeid_parse_print_numeric);
    tcase_add_test(tc_path, nodeid_parse_print_string);
    tcase_add_test(tc_path, nodeid_parse_print_guid);
    tcase_add_test(tc_path, nodeid_parse_print_bytestring);
    tcase_add_test(tc_path, expandednodeid_parse_print);

    Suite *s = suite_create("Util Extended");
    suite_add_tcase(s, tc_tl);
    suite_add_tcase(s, tc_kvm);
    suite_add_tcase(s, tc_path);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_utilExt();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
