/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server.h"
#include "ua_config_default.h"

#include "ua_types.h"

#include "test_types_generated.h"
#include "test_types_generated_handling.h"
#include "test_types_generated_encoding_binary.h"

#include "check.h"
#include "testing_clock.h"

#include "unistd.h"

UA_Server *server = NULL;
UA_ServerConfig *config = NULL;

static void setup(void) {
    config = UA_ServerConfig_new_default();
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

START_TEST(Datatypes_check_encoding_mask_members) {
    // Ensure all bitfield members are accounted for
    int memberCount = TEST_TYPES[TEST_TYPES_OBJECTWITHBITFIELD].membersSize;
    ck_assert_uint_eq(memberCount, 2);

    memberCount = TEST_TYPES[TEST_TYPES_OBJECTWITHBITSOPTIONALFIELDS].membersSize;
    ck_assert_uint_eq(memberCount, 5);
}
END_TEST

START_TEST(Datatypes_check_encoding_mask_size) {
    // Ensure that the calculated size accounts for all bit fields and pads properly
    UA_ObjectWithBitField *item0 = UA_ObjectWithBitField_new();
    size_t encodedSize = UA_ObjectWithBitField_calcSizeBinary(item0);
    UA_ObjectWithBitField_delete(item0);
    ck_assert_uint_eq(encodedSize, 2); // 1 byte data + 1 byte for the encoding mask

    UA_ObjectWithNineBits *item1 = UA_ObjectWithNineBits_new();
    encodedSize = UA_ObjectWithNineBits_calcSizeBinary(item1);
    UA_ObjectWithNineBits_delete(item1);
    ck_assert_uint_eq(encodedSize, 3); // 1 byte data + 2 bytes for the encoding mask
}
END_TEST

START_TEST(Datatypes_check_encoding_mask_serialize) {
    UA_ObjectWithNineBits *item = UA_ObjectWithNineBits_new();

    size_t buflen = UA_calcSizeBinary(item, &TEST_TYPES[TEST_TYPES_OBJECTWITHNINEBITS]);
    UA_ByteString buf;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    item->bitField0 = true;
    item->bitField2 = true;
    item->bitField4 = true;
    item->bitField6 = true;
    item->bitField8 = true;
    item->data = 42;

    UA_Byte *bufPos = buf.data;
    const UA_Byte *bufEnd = &buf.data[buf.length];

    retval = UA_ObjectWithNineBits_encodeBinary(item, &bufPos, bufEnd);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_ObjectWithNineBits_delete(item);

    ck_assert_uint_eq(buf.data[0], 0x55); // 0b10101010 first encoding mask byte
    ck_assert_uint_eq(buf.data[1], 0x01); // 0b00000001 second encoding mask byte
    ck_assert_uint_eq(buf.data[2], 42); // Byte payload as set
}
END_TEST

START_TEST(Datatypes_check_encoding_mask_deserialize) {
    UA_Byte data[] = {
                0x89, // 0b10001001
                0x00, // 0b00000000
                0x12, // Data 18
            };
    UA_ByteString src    = { 3, data };

    UA_ObjectWithNineBits item;
    UA_ObjectWithNineBits_init(&item);

    size_t posDecode = 0;
    UA_StatusCode retval = UA_ObjectWithNineBits_decodeBinary(&src, &posDecode, &item);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(item.bitField0);
    ck_assert(!item.bitField1);
    ck_assert(!item.bitField2);
    ck_assert(item.bitField3);
    ck_assert(!item.bitField4);
    ck_assert(!item.bitField5);
    ck_assert(!item.bitField6);
    ck_assert(item.bitField7);
    ck_assert(!item.bitField8);
    ck_assert_int_eq(item.data, 18);
}
END_TEST

// START_TEST(Datatypes_bitField_serialization) {
//     // TODO: create type
//     // serialize
//     // deserialize
//     // compare
//     ck_assert_uint_eq(0, 1);
// }
// END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Datatype generator test");
    TCase *tc_server = tcase_create("Optional field test");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_members);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_size);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_serialize);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_deserialize);
    suite_add_tcase(s, tc_server);
    return s;
}

int main(void) {
    Suite *s = testSuite_Client();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
