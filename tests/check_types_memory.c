/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define _XOPEN_SOURCE 500
#include <open62541/server.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>
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

START_TEST(encodeShallYieldDecode) {
    // given
    UA_ByteString msg1, msg2;
    void *obj1 = UA_new(&UA_TYPES[_i]);
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg1, 65000); // fixed buf size
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Byte *pos = msg1.data;
    const UA_Byte *end = &msg1.data[msg1.length];
    retval = UA_encodeBinaryInternal(obj1, &UA_TYPES[_i], &pos, &end, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_delete(obj1, &UA_TYPES[_i]);
        UA_ByteString_clear(&msg1);
        return;
    }

    // when
    void *obj2 = UA_new(&UA_TYPES[_i]);
    size_t offset = 0;
    retval = UA_decodeBinaryInternal(&msg1, &offset, obj2, &UA_TYPES[_i], NULL);
    ck_assert_msg(retval == UA_STATUSCODE_GOOD, "could not decode idx=%d,nodeid=%i",
                  _i, UA_TYPES[_i].typeId.identifier.numeric);
    ck_assert(!memcmp(obj1, obj2, UA_TYPES[_i].memSize)); // bit identical decoding
    retval = UA_ByteString_allocBuffer(&msg2, 65000);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    pos = msg2.data;
    end = &msg2.data[msg2.length];
    retval = UA_encodeBinaryInternal(obj2, &UA_TYPES[_i], &pos, &end, NULL, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // then
    msg1.length = offset;
    msg2.length = offset;
    ck_assert_msg(UA_ByteString_equal(&msg1, &msg2) == true,
                  "messages differ idx=%d,nodeid=%i", _i,
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
    retval |= UA_encodeBinaryInternal(obj1, &UA_TYPES[_i], &pos, &end, NULL, NULL);
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
#ifdef _WIN32
    srand(42);
#else
    srandom(42);
#endif
    for(int n = 0;n < RANDOM_TESTS;n++) {
        for(UA_UInt32 i = 0;i < buflen;i++) {
#ifdef _WIN32
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
                      "Decoding %d from random buffer",
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
#ifdef _WIN32
    srand(42);
#else
    srandom(42);
#endif
    // when
    for(int n = 0; n < RANDOM_TESTS; n++) {
        for(UA_UInt32 i = 0; i < buflen; i++) {
#ifdef _WIN32
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
    size_t predicted_size = UA_calcSizeBinary(obj, &UA_TYPES[_i]);
    ck_assert_uint_ne(predicted_size, 0);
    UA_ByteString msg;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg, predicted_size);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    UA_Byte *pos = msg.data;
    const UA_Byte *end = &msg.data[msg.length];
    retval = UA_encodeBinaryInternal(obj, &UA_TYPES[_i], &pos, &end, NULL, NULL);
    if(retval)
        printf("%i\n",_i);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq((uintptr_t)(pos - msg.data), predicted_size);
    UA_delete(obj, &UA_TYPES[_i]);
    UA_ByteString_clear(&msg);
}
END_TEST

int main(void) {
    int number_failed = 0;
    SRunner *sr;

    Suite *s  = suite_create("testMemoryHandling");
    TCase *tc = tcase_create("Empty Objects");
    tcase_add_loop_test(tc, newAndEmptyObjectShallBeDeleted, UA_TYPES_BOOLEAN, UA_TYPES_COUNT - 1);
    tcase_add_test(tc, arrayCopyShallMakeADeepCopy);
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

    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
