/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define _XOPEN_SOURCE 500
#include <open62541/server.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>

#include "ua_types_encoding_binary.h"

#include <stdio.h>
#include <stdlib.h>

#include "check.h"

START_TEST(newAndEmptyObjectShallBeDeleted) {
    // given
    void *obj = UA_new(&UA_TYPES[_i]);
    // then
    ck_assert_ptr_ne(obj, NULL);
    ck_assert(UA_order(obj, obj, &UA_TYPES[_i]) == UA_ORDER_EQ);
    // finally
    UA_delete(obj, &UA_TYPES[_i]);
}
END_TEST

START_TEST(arrayCopyShallMakeADeepCopy) {
    // given
    UA_String a1[3];
    a1[0] = (UA_String){1, (UA_Byte*)"a"};
    a1[1] = (UA_String){2, (UA_Byte*)"bb"};
    a1[2] = (UA_String){3, (UA_Byte*)"ccc"};
    // when
    UA_String *a2;
    UA_UInt32 retval = UA_Array_copy((const void *)a1, 3, (void **)&a2, &UA_TYPES[UA_TYPES_STRING]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(a1[0].length, 1);
    ck_assert_uint_eq(a1[1].length, 2);
    ck_assert_uint_eq(a1[2].length, 3);
    ck_assert_uint_eq(a1[0].length, a2[0].length);
    ck_assert_uint_eq(a1[1].length, a2[1].length);
    ck_assert_uint_eq(a1[2].length, a2[2].length);
    ck_assert_ptr_ne(a1[0].data, a2[0].data);
    ck_assert_ptr_ne(a1[1].data, a2[1].data);
    ck_assert_ptr_ne(a1[2].data, a2[2].data);
    ck_assert_int_eq(a1[0].data[0], a2[0].data[0]);
    ck_assert_int_eq(a1[1].data[0], a2[1].data[0]);
    ck_assert_int_eq(a1[2].data[0], a2[2].data[0]);
    // finally
    UA_Array_delete((void *)a2, 3, &UA_TYPES[UA_TYPES_STRING]);
}
END_TEST

START_TEST(arrayAppendCopyShallWorkOnExample) {
    UA_UInt32 *arr = NULL;
    size_t arrSize = 0;

    UA_UInt32 val1 = 10;
    UA_StatusCode retval = UA_Array_appendCopy((void**)&arr, &arrSize, &val1,
                                               &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 1);
    ck_assert_uint_eq(arr[0], 10);

    UA_UInt32 val2 = 20;
    retval = UA_Array_appendCopy((void**)&arr, &arrSize, &val2,
                                 &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 2);
    ck_assert_uint_eq(arr[0], 10);
    ck_assert_uint_eq(arr[1], 20);

    UA_UInt32 val3 = 30;
    retval = UA_Array_appendCopy((void**)&arr, &arrSize, &val3,
                                 &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 3);
    ck_assert_uint_eq(arr[2], 30);

    UA_Array_delete(arr, arrSize, &UA_TYPES[UA_TYPES_UINT32]);
}
END_TEST

START_TEST(arrayResizeShallWorkOnExample) {
    UA_UInt32 *arr = NULL;
    size_t arrSize = 0;

    UA_StatusCode retval = UA_Array_resize((void**)&arr, &arrSize, 5,
                                           &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 5);
    for(size_t i = 0; i < 5; i++)
        arr[i] = (UA_UInt32)i;

    retval = UA_Array_resize((void**)&arr, &arrSize, 10,
                             &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 10);
    for(size_t i = 0; i < 5; i++)
        ck_assert_uint_eq(arr[i], i);

    retval = UA_Array_resize((void**)&arr, &arrSize, 3,
                             &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 3);
    for(size_t i = 0; i < 3; i++)
        ck_assert_uint_eq(arr[i], i);

    retval = UA_Array_resize((void**)&arr, &arrSize, 0,
                             &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 0);

    UA_Array_delete(arr, arrSize, &UA_TYPES[UA_TYPES_UINT32]);
}
END_TEST

START_TEST(arrayAppendShallWorkOnExample) {
    UA_UInt32 *arr = NULL;
    size_t arrSize = 0;

    UA_UInt32 val1 = 100;
    UA_StatusCode retval = UA_Array_append((void**)&arr, &arrSize, &val1,
                                           &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 1);
    ck_assert_uint_eq(arr[0], 100);

    UA_UInt32 val2 = 200;
    retval = UA_Array_append((void**)&arr, &arrSize, &val2,
                             &UA_TYPES[UA_TYPES_UINT32]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(arrSize, 2);
    ck_assert_uint_eq(arr[1], 200);

    UA_Array_delete(arr, arrSize, &UA_TYPES[UA_TYPES_UINT32]);
}
END_TEST

START_TEST(equalShallWorkOnScalarTypes) {
    UA_UInt32 a = 42;
    UA_UInt32 b = 42;
    UA_UInt32 c = 43;

    ck_assert(UA_equal(&a, &b, &UA_TYPES[UA_TYPES_UINT32]) == true);
    ck_assert(UA_equal(&a, &c, &UA_TYPES[UA_TYPES_UINT32]) == false);

    UA_String s1 = UA_STRING("test");
    UA_String s2 = UA_STRING("test");
    UA_String s3 = UA_STRING("other");

    ck_assert(UA_equal(&s1, &s2, &UA_TYPES[UA_TYPES_STRING]) == true);
    ck_assert(UA_equal(&s1, &s3, &UA_TYPES[UA_TYPES_STRING]) == false);

    UA_NodeId n1 = UA_NODEID_NUMERIC(1, 100);
    UA_NodeId n2 = UA_NODEID_NUMERIC(1, 100);
    UA_NodeId n3 = UA_NODEID_NUMERIC(2, 100);

    ck_assert(UA_equal(&n1, &n2, &UA_TYPES[UA_TYPES_NODEID]) == true);
    ck_assert(UA_equal(&n1, &n3, &UA_TYPES[UA_TYPES_NODEID]) == false);
}
END_TEST

START_TEST(encodeShallYieldDecode) {
    // given
    UA_ByteString msg1, msg2;
    void *obj1 = UA_new(&UA_TYPES[_i]);
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg1, 65000); // fixed buf size
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Byte *pos = msg1.data;
    const UA_Byte *end = &msg1.data[msg1.length];
    retval = UA_encodeBinaryInternal(obj1, &UA_TYPES[_i], &pos, &end, NULL, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_delete(obj1, &UA_TYPES[_i]);
        UA_ByteString_clear(&msg1);
        return;
    }

    // when
    void *obj2 = UA_new(&UA_TYPES[_i]);
    size_t offset = 0;
    retval = UA_decodeBinaryInternal(&msg1, &offset, obj2, &UA_TYPES[_i], NULL);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "could not decode idx=%d,nodeid=%u",
                  _i, UA_TYPES[_i].typeId.identifier.numeric);
    ck_assert(!memcmp(obj1, obj2, UA_TYPES[_i].memSize)); // bit identical decoding
    retval = UA_ByteString_allocBuffer(&msg2, 65000);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    pos = msg2.data;
    end = &msg2.data[msg2.length];
    retval = UA_encodeBinaryInternal(obj2, &UA_TYPES[_i], &pos, &end, NULL, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // then
    msg1.length = offset;
    msg2.length = offset;
    ck_assert_msg(UA_ByteString_equal(&msg1, &msg2) == true,
                  "messages differ idx=%d,nodeid=%u", _i,
                  UA_TYPES[_i].typeId.identifier.numeric);
    ck_assert(UA_order(obj1, obj2, &UA_TYPES[_i]) == UA_ORDER_EQ);

    // pretty-print the value
#ifdef UA_ENABLE_JSON_ENCODING
    UA_Byte staticBuf[4096];
    UA_String buf;
    buf.data = staticBuf;
    buf.length = 4096;
    retval = UA_print(obj2, &UA_TYPES[_i], &buf);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
#endif

    // finally
    UA_delete(obj1, &UA_TYPES[_i]);
    UA_delete(obj2, &UA_TYPES[_i]);
    UA_ByteString_clear(&msg1);
    UA_ByteString_clear(&msg2);
}
END_TEST

START_TEST(decodeShallFailWithTruncatedBufferButSurvive) {
    // given
    UA_ByteString msg1;
    void *obj1 = UA_new(&UA_TYPES[_i]);
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg1, 65000); // fixed buf size
    UA_Byte *pos = msg1.data;
    const UA_Byte *end = &msg1.data[msg1.length];
    retval |= UA_encodeBinaryInternal(obj1, &UA_TYPES[_i], &pos, &end, NULL, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_delete(obj1, &UA_TYPES[_i]);

    size_t half = (uintptr_t)(pos - msg1.data) / 2;
    msg1.length = half;

    // when
    void *obj2 = UA_new(&UA_TYPES[_i]);
    size_t offset = 0;
    retval = UA_decodeBinaryInternal(&msg1, &offset, obj2, &UA_TYPES[_i], NULL);
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
    UA_delete(obj2, &UA_TYPES[_i]);
    msg1.length = 65000;
    UA_ByteString_clear(&msg1);
}
END_TEST

#define RANDOM_TESTS 1000

START_TEST(decodeScalarBasicTypeFromRandomBufferShallSucceed) {
    // given
    void *obj1 = NULL;
    UA_ByteString msg1;
    UA_UInt32 buflen = 256;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg1, buflen); // fixed size
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
#ifdef UA_ARCHITECTURE_WIN32
    srand(42);
#else
    srandom(42);
#endif
    for(int n = 0;n < RANDOM_TESTS;n++) {
        for(UA_UInt32 i = 0;i < buflen;i++) {
#ifdef UA_ARCHITECTURE_WIN32
            UA_UInt32 rnd;
            rnd = rand();
            msg1.data[i] = rnd;
#else
            msg1.data[i] = (UA_Byte)random();  // when
#endif
        }
        size_t pos = 0;
        obj1 = UA_new(&UA_TYPES[_i]);
        retval = UA_decodeBinaryInternal(&msg1, &pos, obj1, &UA_TYPES[_i], NULL);
        (void)retval;
        //then
        ck_assert_msg(retval == UA_STATUSCODE_GOOD,
                      "Decoding %u from random buffer",
                      UA_TYPES[_i].typeId.identifier.numeric);
        // finally
        UA_delete(obj1, &UA_TYPES[_i]);
    }
    UA_ByteString_clear(&msg1);
}
END_TEST

START_TEST(decodeComplexTypeFromRandomBufferShallSurvive) {
    // given
    UA_ByteString msg1;
    UA_UInt32 buflen = 256;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg1, buflen); // fixed size
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
#ifdef UA_ARCHITECTURE_WIN32
    srand(42);
#else
    srandom(42);
#endif
    // when
    for(int n = 0; n < RANDOM_TESTS; n++) {
        for(UA_UInt32 i = 0; i < buflen; i++) {
#ifdef UA_ARCHITECTURE_WIN32
            UA_UInt32 rnd;
            rnd = rand();
            msg1.data[i] = rnd;
#else
            msg1.data[i] = (UA_Byte)random();  // when
#endif
        }
        size_t pos = 0;
        void *obj1 = UA_new(&UA_TYPES[_i]);
        retval = UA_decodeBinaryInternal(&msg1, &pos, obj1, &UA_TYPES[_i], NULL);
        (void)retval;
        UA_delete(obj1, &UA_TYPES[_i]);
    }

    // finally
    UA_ByteString_clear(&msg1);
}
END_TEST

START_TEST(calcSizeBinaryShallBeCorrect) {
    void *obj = UA_new(&UA_TYPES[_i]);
    size_t predicted_size = UA_calcSizeBinary(obj, &UA_TYPES[_i], NULL);
    ck_assert_uint_ne(predicted_size, 0);
    UA_ByteString msg;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg, predicted_size);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Byte *pos = msg.data;
    const UA_Byte *end = &msg.data[msg.length];
    retval = UA_encodeBinaryInternal(obj, &UA_TYPES[_i], &pos, &end, NULL, NULL, NULL);
    if(retval)
        printf("%i\n",_i);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq((uintptr_t)(pos - msg.data), predicted_size);
    UA_delete(obj, &UA_TYPES[_i]);
    UA_ByteString_clear(&msg);
}
END_TEST

/* === Memory edge case tests === */

START_TEST(clear_localizedText_with_content) {
    UA_LocalizedText *lt = (UA_LocalizedText*)UA_new(&UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    lt->locale = UA_STRING_ALLOC("en-US");
    lt->text = UA_STRING_ALLOC("Hello World");
    UA_clear(lt, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_free(lt);
} END_TEST

START_TEST(clear_qualifiedName_with_content) {
    UA_QualifiedName *qn = (UA_QualifiedName*)UA_new(&UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    qn->namespaceIndex = 5;
    qn->name = UA_STRING_ALLOC("TestName");
    UA_clear(qn, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    UA_free(qn);
} END_TEST

START_TEST(clear_nodeId_string) {
    UA_NodeId *nodeId = (UA_NodeId*)UA_new(&UA_TYPES[UA_TYPES_NODEID]);
    nodeId->identifierType = UA_NODEIDTYPE_STRING;
    nodeId->namespaceIndex = 1;
    nodeId->identifier.string = UA_STRING_ALLOC("TestNode");
    UA_clear(nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_free(nodeId);
} END_TEST

START_TEST(clear_nodeId_bytestring) {
    UA_NodeId *nodeId = (UA_NodeId*)UA_new(&UA_TYPES[UA_TYPES_NODEID]);
    nodeId->identifierType = UA_NODEIDTYPE_BYTESTRING;
    nodeId->namespaceIndex = 2;
    nodeId->identifier.byteString = UA_BYTESTRING_ALLOC("binaryid");
    UA_clear(nodeId, &UA_TYPES[UA_TYPES_NODEID]);
    UA_free(nodeId);
} END_TEST

START_TEST(dataValue_copy_with_value) {
    /* Copy a DataValue that has a scalar value attached. */
    UA_DataValue src;
    UA_DataValue_init(&src);
    UA_Int32 val = 42;
    UA_Variant_setScalarCopy(&src.value, &val, &UA_TYPES[UA_TYPES_INT32]);
    src.hasValue = true;
    UA_DateTime now = UA_DateTime_now();
    src.sourceTimestamp = now;
    src.hasSourceTimestamp = true;
    src.status = UA_STATUSCODE_GOOD;

    UA_DataValue dst;
    UA_DataValue_init(&dst);
    UA_StatusCode rv = UA_copy(&src, &dst, &UA_TYPES[UA_TYPES_DATAVALUE]);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.hasValue, true);
    ck_assert(dst.value.type == &UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(*(UA_Int32 *)dst.value.data, 42);
    ck_assert_uint_eq(dst.hasSourceTimestamp, true);
    ck_assert_uint_eq(dst.sourceTimestamp, now);
    ck_assert_uint_eq(dst.status, UA_STATUSCODE_GOOD);

    /* The destination is an independent deep copy. */
    ck_assert_ptr_ne(src.value.data, dst.value.data);

    UA_clear(&src, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_clear(&dst, &UA_TYPES[UA_TYPES_DATAVALUE]);
} END_TEST

START_TEST(dataValue_clear_empty) {
    /* Clearing a freshly-init'd DataValue must not crash. */
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    UA_clear(&dv, &UA_TYPES[UA_TYPES_DATAVALUE]);
} END_TEST

START_TEST(diagnosticInfo_copy) {
    /* DiagnosticInfo has bitfield flags plus several optional fields. */
    UA_DiagnosticInfo src;
    UA_DiagnosticInfo_init(&src);
    src.hasSymbolicId = true;
    src.symbolicId = 7;
    src.hasNamespaceUri = true;
    src.namespaceUri = 42;
    src.hasLocalizedText = true;
    src.localizedText = 1234;
    src.hasLocale = true;
    src.locale = 9;
    src.hasAdditionalInfo = true;
    src.additionalInfo = UA_BYTESTRING_ALLOC("extra");

    UA_DiagnosticInfo dst;
    UA_DiagnosticInfo_init(&dst);
    UA_StatusCode rv = UA_copy(&src, &dst, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    ck_assert_uint_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.hasSymbolicId, true);
    ck_assert_uint_eq(dst.symbolicId, 7);
    ck_assert_uint_eq(dst.hasNamespaceUri, true);
    ck_assert_uint_eq(dst.namespaceUri, 42);
    ck_assert_uint_eq(dst.hasLocalizedText, true);
    ck_assert_uint_eq(dst.localizedText, 1234);
    ck_assert_uint_eq(dst.hasLocale, true);
    ck_assert_uint_eq(dst.locale, 9);
    ck_assert_uint_eq(dst.hasAdditionalInfo, true);
    ck_assert(UA_ByteString_equal(&dst.additionalInfo, &src.additionalInfo));

    UA_clear(&src, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    UA_clear(&dst, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
} END_TEST

int main(void) {
    int number_failed = 0;
    SRunner *sr;

    Suite *s  = suite_create("testMemoryHandling");
    TCase *tc = tcase_create("Empty Objects");
    tcase_add_loop_test(tc, newAndEmptyObjectShallBeDeleted, UA_TYPES_BOOLEAN, UA_TYPES_COUNT - 1);
    tcase_add_test(tc, arrayCopyShallMakeADeepCopy);
    tcase_add_test(tc, arrayAppendCopyShallWorkOnExample);
    tcase_add_test(tc, arrayResizeShallWorkOnExample);
    tcase_add_test(tc, arrayAppendShallWorkOnExample);
    tcase_add_test(tc, equalShallWorkOnScalarTypes);
    tcase_add_loop_test(tc, encodeShallYieldDecode, UA_TYPES_BOOLEAN, UA_TYPES_COUNT - 1);
    suite_add_tcase(s, tc);
    tc = tcase_create("Truncated Buffers");
    tcase_add_loop_test(tc, decodeShallFailWithTruncatedBufferButSurvive,
                        UA_TYPES_BOOLEAN, UA_TYPES_COUNT - 1);
    suite_add_tcase(s, tc);

    tc = tcase_create("Fuzzing with Random Buffers");
    tcase_add_loop_test(tc, decodeScalarBasicTypeFromRandomBufferShallSucceed,
                        UA_TYPES_BOOLEAN, UA_TYPES_DOUBLE);
    tcase_add_loop_test(tc, decodeComplexTypeFromRandomBufferShallSurvive,
                        UA_TYPES_NODEID, UA_TYPES_COUNT - 1);
    suite_add_tcase(s, tc);

    tc = tcase_create("Test calcSizeBinary");
    tcase_add_loop_test(tc, calcSizeBinaryShallBeCorrect, UA_TYPES_BOOLEAN, UA_TYPES_COUNT - 1);
    suite_add_tcase(s, tc);

    tc = tcase_create("Clear edge cases");
    tcase_add_test(tc, clear_localizedText_with_content);
    tcase_add_test(tc, clear_qualifiedName_with_content);
    tcase_add_test(tc, clear_nodeId_string);
    tcase_add_test(tc, clear_nodeId_bytestring);
    tcase_add_test(tc, dataValue_clear_empty);
    suite_add_tcase(s, tc);

    tc = tcase_create("Copy edge cases");
    tcase_add_test(tc, dataValue_copy_with_value);
    tcase_add_test(tc, diagnosticInfo_copy);
    suite_add_tcase(s, tc);

    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
