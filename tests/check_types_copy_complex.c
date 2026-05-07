/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Type operations and JSON encoding tests */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>
#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

/* === Copy and clear operations on complex types === */
START_TEST(copy_applicationdescription) {
    UA_ApplicationDescription src;
    UA_ApplicationDescription_init(&src);
    src.applicationUri = UA_STRING_ALLOC("urn:test");
    src.productUri = UA_STRING_ALLOC("urn:product");
    src.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", "TestApp");
    src.applicationType = UA_APPLICATIONTYPE_SERVER;
    src.gatewayServerUri = UA_STRING_ALLOC("urn:gateway");
    src.discoveryProfileUri = UA_STRING_ALLOC("urn:disc");

    src.discoveryUrlsSize = 2;
    src.discoveryUrls = (UA_String*)UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    src.discoveryUrls[0] = UA_STRING_ALLOC("opc.tcp://localhost:4840");
    src.discoveryUrls[1] = UA_STRING_ALLOC("opc.tcp://localhost:4841");

    UA_ApplicationDescription dst;
    UA_StatusCode res = UA_ApplicationDescription_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.applicationUri, &dst.applicationUri));
    ck_assert_uint_eq(dst.discoveryUrlsSize, 2);

    UA_ApplicationDescription_clear(&src);
    UA_ApplicationDescription_clear(&dst);
} END_TEST

START_TEST(copy_endpointdescription) {
    UA_EndpointDescription src;
    UA_EndpointDescription_init(&src);
    src.endpointUrl = UA_STRING_ALLOC("opc.tcp://localhost:4840");
    src.securityMode = UA_MESSAGESECURITYMODE_NONE;
    src.securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");
    src.securityLevel = 1;
    src.transportProfileUri = UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");
    src.server.applicationUri = UA_STRING_ALLOC("urn:test");
    src.server.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", "TestServer");

    /* UserIdentityTokens array */
    src.userIdentityTokensSize = 1;
    src.userIdentityTokens = (UA_UserTokenPolicy*)
        UA_Array_new(1, &UA_TYPES[UA_TYPES_USERTOKENPOLICY]);
    src.userIdentityTokens[0].tokenType = UA_USERTOKENTYPE_ANONYMOUS;
    src.userIdentityTokens[0].policyId = UA_STRING_ALLOC("anon");

    UA_EndpointDescription dst;
    UA_StatusCode res = UA_EndpointDescription_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(UA_String_equal(&src.endpointUrl, &dst.endpointUrl));

    UA_EndpointDescription_clear(&src);
    UA_EndpointDescription_clear(&dst);
} END_TEST

START_TEST(copy_diagnosticinfo) {
    UA_DiagnosticInfo src;
    UA_DiagnosticInfo_init(&src);
    src.hasSymbolicId = true;
    src.symbolicId = 42;
    src.hasNamespaceUri = true;
    src.namespaceUri = 1;
    src.hasLocalizedText = true;
    src.localizedText = 2;
    src.hasLocale = true;
    src.locale = 3;
    src.hasAdditionalInfo = true;
    src.additionalInfo = UA_STRING_ALLOC("extra info");
    src.hasInnerStatusCode = true;
    src.innerStatusCode = UA_STATUSCODE_BADNOTFOUND;

    /* Nested DiagnosticInfo */
    src.hasInnerDiagnosticInfo = true;
    src.innerDiagnosticInfo = UA_DiagnosticInfo_new();
    src.innerDiagnosticInfo->hasSymbolicId = true;
    src.innerDiagnosticInfo->symbolicId = 99;

    UA_DiagnosticInfo dst;
    UA_StatusCode res = UA_DiagnosticInfo_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst.symbolicId, 42);
    ck_assert(dst.hasInnerDiagnosticInfo);
    ck_assert_int_eq(dst.innerDiagnosticInfo->symbolicId, 99);

    UA_DiagnosticInfo_clear(&src);
    UA_DiagnosticInfo_clear(&dst);
} END_TEST

START_TEST(copy_browseresult) {
    UA_BrowseResult src;
    UA_BrowseResult_init(&src);
    src.statusCode = UA_STATUSCODE_GOOD;
    src.continuationPoint = UA_BYTESTRING_ALLOC("cp");

    src.referencesSize = 2;
    src.references = (UA_ReferenceDescription*)
        UA_Array_new(2, &UA_TYPES[UA_TYPES_REFERENCEDESCRIPTION]);
    src.references[0].nodeId.nodeId = UA_NODEID_NUMERIC(0, 1000);
    src.references[0].browseName = UA_QUALIFIEDNAME_ALLOC(0, "Ref1");
    src.references[0].displayName = UA_LOCALIZEDTEXT_ALLOC("en", "Ref1");
    src.references[0].nodeClass = UA_NODECLASS_OBJECT;
    src.references[0].isForward = true;
    src.references[1].nodeId.nodeId = UA_NODEID_NUMERIC(0, 1001);
    src.references[1].browseName = UA_QUALIFIEDNAME_ALLOC(0, "Ref2");
    src.references[1].displayName = UA_LOCALIZEDTEXT_ALLOC("en", "Ref2");
    src.references[1].nodeClass = UA_NODECLASS_VARIABLE;
    src.references[1].isForward = false;

    UA_BrowseResult dst;
    UA_StatusCode res = UA_BrowseResult_copy(&src, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.referencesSize, 2);

    UA_BrowseResult_clear(&src);
    UA_BrowseResult_clear(&dst);
} END_TEST

/* === Variant operations === */
START_TEST(variant_copy_string_array) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_String strs[] = {
        UA_STRING_STATIC("hello"),
        UA_STRING_STATIC("world"),
        UA_STRING_STATIC("foo")
    };
    UA_Variant_setArray(&v, strs, 3, &UA_TYPES[UA_TYPES_STRING]);

    UA_Variant dst;
    UA_StatusCode res = UA_Variant_copy(&v, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 3);
    ck_assert(UA_String_equal(&((UA_String*)dst.data)[0], &strs[0]));
    UA_Variant_clear(&dst);
} END_TEST

START_TEST(variant_setcopy_scalar) {
    UA_Double d = 3.14159;
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = UA_Variant_setScalarCopy(&v, &d, &UA_TYPES[UA_TYPES_DOUBLE]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(fabs(*(UA_Double*)v.data - 3.14159) < 0.0001);
    UA_Variant_clear(&v);
} END_TEST

START_TEST(variant_setcopy_array) {
    UA_Int32 arr[] = {1, 2, 3, 4, 5};
    UA_Variant v;
    UA_Variant_init(&v);
    UA_StatusCode res = UA_Variant_setArrayCopy(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(v.arrayLength, 5);
    ck_assert_int_eq(((UA_Int32*)v.data)[2], 3);
    UA_Variant_clear(&v);
} END_TEST

START_TEST(variant_has_type) {
    UA_Variant v;
    UA_Variant_init(&v);
    ck_assert(!UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(!UA_Variant_hasArrayType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(UA_Variant_isEmpty(&v));
    ck_assert(!UA_Variant_isScalar(&v));

    UA_Int32 val = 42;
    UA_Variant_setScalar(&v, &val, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert(UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_INT32]));
    ck_assert(!UA_Variant_hasScalarType(&v, &UA_TYPES[UA_TYPES_STRING]));
    ck_assert(UA_Variant_isScalar(&v));
    ck_assert(!UA_Variant_isEmpty(&v));
    ck_assert(!UA_Variant_hasArrayType(&v, &UA_TYPES[UA_TYPES_INT32]));
} END_TEST

START_TEST(variant_setrange) {
    /* Create array variant */
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 arr[] = {10, 20, 30, 40, 50};
    UA_StatusCode res = UA_Variant_setArrayCopy(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Set a range */
    UA_Int32 newArr[] = {99, 88};
    UA_Variant src;
    UA_Variant_setArray(&src, newArr, 2, &UA_TYPES[UA_TYPES_INT32]);

    UA_NumericRange range;
    range.dimensionsSize = 1;
    UA_NumericRangeDimension dim;
    dim.min = 1;
    dim.max = 2;
    range.dimensions = &dim;

    res = UA_Variant_setRange(&v, src.data, src.arrayLength, range);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((UA_Int32*)v.data)[1], 99);
    ck_assert_int_eq(((UA_Int32*)v.data)[2], 88);

    UA_Variant_clear(&v);
} END_TEST

START_TEST(variant_copyrange) {
    UA_Variant v;
    UA_Variant_init(&v);
    UA_Int32 arr[] = {10, 20, 30, 40, 50};
    UA_StatusCode res = UA_Variant_setArrayCopy(&v, arr, 5, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_NumericRange range;
    range.dimensionsSize = 1;
    UA_NumericRangeDimension dim;
    dim.min = 1;
    dim.max = 3;
    range.dimensions = &dim;

    UA_Variant dst;
    res = UA_Variant_copyRange(&v, &dst, range);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.arrayLength, 3);
    ck_assert_int_eq(((UA_Int32*)dst.data)[0], 20);
    ck_assert_int_eq(((UA_Int32*)dst.data)[2], 40);
    UA_Variant_clear(&dst);

    UA_Variant_clear(&v);
} END_TEST

/* === DataValue operations === */
START_TEST(datavalue_copy_all) {
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasValue = true;
    UA_Int32 val = 42;
    UA_Variant_setScalarCopy(&dv.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    dv.hasStatus = true;
    dv.status = UA_STATUSCODE_GOOD;
    dv.hasSourceTimestamp = true;
    dv.sourceTimestamp = UA_DateTime_now();
    dv.hasSourcePicoseconds = true;
    dv.sourcePicoseconds = 100;
    dv.hasServerTimestamp = true;
    dv.serverTimestamp = UA_DateTime_now();
    dv.hasServerPicoseconds = true;
    dv.serverPicoseconds = 200;

    UA_DataValue dst;
    UA_StatusCode res = UA_DataValue_copy(&dv, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(dst.hasValue);
    ck_assert(dst.hasStatus);
    ck_assert(dst.hasSourceTimestamp);
    ck_assert(dst.hasSourcePicoseconds);
    ck_assert(dst.hasServerTimestamp);
    ck_assert(dst.hasServerPicoseconds);
    ck_assert_int_eq(*(UA_Int32*)dst.value.data, 42);

    UA_DataValue_clear(&dv);
    UA_DataValue_clear(&dst);
} END_TEST

/* === ExtensionObject operations === */
START_TEST(extensionobject_decoded) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);

    /* Create a decoded extension object */
    UA_ReadValueId *rvid = UA_ReadValueId_new();
    rvid->nodeId = UA_NODEID_NUMERIC(0, 2255);
    rvid->attributeId = UA_ATTRIBUTEID_VALUE;

    eo.encoding = UA_EXTENSIONOBJECT_DECODED;
    eo.content.decoded.type = &UA_TYPES[UA_TYPES_READVALUEID];
    eo.content.decoded.data = rvid;

    UA_ExtensionObject dst;
    UA_StatusCode res = UA_ExtensionObject_copy(&eo, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dst.encoding, UA_EXTENSIONOBJECT_DECODED);

    UA_ExtensionObject_clear(&eo);
    UA_ExtensionObject_clear(&dst);
} END_TEST

START_TEST(extensionobject_bytestring) {
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);

    eo.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo.content.encoded.typeId = UA_NODEID_NUMERIC(0, 777);
    eo.content.encoded.body = UA_BYTESTRING_ALLOC("testdata");

    UA_ExtensionObject dst;
    UA_StatusCode res = UA_ExtensionObject_copy(&eo, &dst);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    UA_ExtensionObject_clear(&eo);
    UA_ExtensionObject_clear(&dst);
} END_TEST

/* === Array operations === */
START_TEST(array_copy_empty) {
    void *dst = NULL;
    size_t dstSize = 0;
    UA_StatusCode res = UA_Array_copy(NULL, 0, &dst, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    /* Empty array copy returns NULL or sentinel */
    (void)dstSize;
} END_TEST

START_TEST(array_append) {
    void *arr = NULL;
    size_t size = 0;

    UA_Int32 val1 = 10;
    UA_StatusCode res = UA_Array_appendCopy(&arr, &size, &val1, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 1);
    ck_assert_int_eq(((UA_Int32*)arr)[0], 10);

    UA_Int32 val2 = 20;
    res = UA_Array_appendCopy(&arr, &size, &val2, &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(size, 2);
    ck_assert_int_eq(((UA_Int32*)arr)[1], 20);

    UA_Array_delete(arr, size, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

/* === NumericRange parsing === */
START_TEST(numericrange_parse) {
    UA_NumericRange range;
    UA_StatusCode res = UA_NumericRange_parse(&range, UA_STRING("2:5"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensionsSize, 1);
    ck_assert_uint_eq(range.dimensions[0].min, 2);
    ck_assert_uint_eq(range.dimensions[0].max, 5);
    UA_free(range.dimensions);
} END_TEST

START_TEST(numericrange_parse_multi) {
    UA_NumericRange range;
    UA_StatusCode res = UA_NumericRange_parse(&range, UA_STRING("1:3,0:2"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensionsSize, 2);
    UA_free(range.dimensions);
} END_TEST

START_TEST(numericrange_parse_single) {
    UA_NumericRange range;
    UA_StatusCode res = UA_NumericRange_parse(&range, UA_STRING("3"));
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(range.dimensionsSize, 1);
    ck_assert_uint_eq(range.dimensions[0].min, 3);
    ck_assert_uint_eq(range.dimensions[0].max, 3);
    UA_free(range.dimensions);
} END_TEST

START_TEST(numericrange_parse_invalid) {
    UA_NumericRange range;
    UA_StatusCode res = UA_NumericRange_parse(&range, UA_STRING("abc"));
    ck_assert(res != UA_STATUSCODE_GOOD);

    res = UA_NumericRange_parse(&range, UA_STRING("5:2")); /* min > max */
    ck_assert(res != UA_STATUSCODE_GOOD);

    res = UA_NumericRange_parse(&range, UA_STRING("")); /* empty */
    ck_assert(res != UA_STATUSCODE_GOOD);
} END_TEST

/* === Type new/delete === */
START_TEST(type_new_delete_all) {
    /* Test allocating and freeing various types */
    for(size_t i = 0; i < UA_TYPES_COUNT; i++) {
        /* Skip union types and types with function pointers */
        if(UA_TYPES[i].membersSize == 0 && UA_TYPES[i].memSize > sizeof(void*))
            continue;
        void *p = UA_new(&UA_TYPES[i]);
        if(p) {
            UA_clear(p, &UA_TYPES[i]);
            UA_delete(p, &UA_TYPES[i]);
        }
    }
} END_TEST

/* === StatusCode name === */
START_TEST(statuscode_name) {
    const char *name = UA_StatusCode_name(UA_STATUSCODE_GOOD);
    ck_assert_str_eq(name, "Good");

    name = UA_StatusCode_name(UA_STATUSCODE_BADNOTFOUND);
    ck_assert(name != NULL);
    ck_assert(strlen(name) > 0);

    name = UA_StatusCode_name(UA_STATUSCODE_BADNODEIDUNKNOWN);
    ck_assert(name != NULL);

    /* Unknown status code */
    name = UA_StatusCode_name(0x12345678);
    ck_assert(name != NULL);
} END_TEST

/* === DateTime structure === */
START_TEST(datetime_toStruct) {
    UA_DateTime now = UA_DateTime_now();
    UA_DateTimeStruct dts = UA_DateTime_toStruct(now);
    ck_assert(dts.year >= 2020);
    ck_assert(dts.month >= 1 && dts.month <= 12);
    ck_assert(dts.day >= 1 && dts.day <= 31);
    ck_assert(dts.hour <= 23);
    ck_assert(dts.min <= 59);
    ck_assert(dts.sec <= 59);

    /* Convert from struct back to DateTime */
    UA_DateTime fromEpoch = UA_DateTime_fromStruct(dts);
    /* Should be close (within 1 second) */
    UA_DateTime diff = (now > fromEpoch) ? now - fromEpoch : fromEpoch - now;
    ck_assert(diff < UA_DATETIME_SEC);
} END_TEST

START_TEST(datetime_now_monotonic) {
    UA_Int64 t1 = UA_DateTime_nowMonotonic();
    UA_Int64 t2 = UA_DateTime_nowMonotonic();
    ck_assert(t2 >= t1);
} END_TEST

/* === Guid operations === */
START_TEST(guid_operations) {
    UA_Guid g1 = UA_GUID("09087e75-8e5e-499b-954f-f2a9603db28a");
    UA_Guid g2 = UA_GUID("09087e75-8e5e-499b-954f-f2a9603db28a");
    ck_assert(UA_Guid_equal(&g1, &g2));

    UA_Guid g3 = UA_GUID("19087e75-8e5e-499b-954f-f2a9603db28a");
    ck_assert(!UA_Guid_equal(&g1, &g3));

    /* Random Guid */
    UA_Guid random = UA_Guid_random();
    (void)random;
} END_TEST

/* === Suite definition === */
static Suite *testSuite_typesOps(void) {
    TCase *tc_copy = tcase_create("TypeCopy");
    tcase_add_test(tc_copy, copy_applicationdescription);
    tcase_add_test(tc_copy, copy_endpointdescription);
    tcase_add_test(tc_copy, copy_diagnosticinfo);
    tcase_add_test(tc_copy, copy_browseresult);

    TCase *tc_variant = tcase_create("VariantOps");
    tcase_add_test(tc_variant, variant_copy_string_array);
    tcase_add_test(tc_variant, variant_setcopy_scalar);
    tcase_add_test(tc_variant, variant_setcopy_array);
    tcase_add_test(tc_variant, variant_has_type);
    tcase_add_test(tc_variant, variant_setrange);
    tcase_add_test(tc_variant, variant_copyrange);

    TCase *tc_dv = tcase_create("DataValueOps");
    tcase_add_test(tc_dv, datavalue_copy_all);
    tcase_add_test(tc_dv, extensionobject_decoded);
    tcase_add_test(tc_dv, extensionobject_bytestring);

    TCase *tc_array = tcase_create("ArrayOps");
    tcase_add_test(tc_array, array_copy_empty);
    tcase_add_test(tc_array, array_append);

    TCase *tc_range = tcase_create("NumericRange");
    tcase_add_test(tc_range, numericrange_parse);
    tcase_add_test(tc_range, numericrange_parse_multi);
    tcase_add_test(tc_range, numericrange_parse_single);
    tcase_add_test(tc_range, numericrange_parse_invalid);

    TCase *tc_misc = tcase_create("TypeMisc");
    tcase_add_test(tc_misc, type_new_delete_all);
    tcase_add_test(tc_misc, statuscode_name);
    tcase_add_test(tc_misc, datetime_toStruct);
    tcase_add_test(tc_misc, datetime_now_monotonic);
    tcase_add_test(tc_misc, guid_operations);

    Suite *s = suite_create("Types Operations Extended");
    suite_add_tcase(s, tc_copy);
    suite_add_tcase(s, tc_variant);
    suite_add_tcase(s, tc_dv);
    suite_add_tcase(s, tc_array);
    suite_add_tcase(s, tc_range);
    suite_add_tcase(s, tc_misc);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_typesOps();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
