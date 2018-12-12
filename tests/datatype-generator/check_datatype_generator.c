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

// Note: the check for builtin types verifies, that calculated sizes do not change arbitrarily

START_TEST(Datatypes_check_encoding_mask_members) {
    // Ensure all bitfield members are accounted for
    int memberCount = TEST_TYPES[TEST_TYPES_OBJECTWITHBITFIELD].membersSize;
    ck_assert_uint_eq(memberCount, 2);

    memberCount = TEST_TYPES[TEST_TYPES_OBJECTWITHBITSOPTIONALFIELDS].membersSize;
    ck_assert_uint_eq(memberCount, 4);
}
END_TEST

START_TEST(Datatypes_check_encoding_mask_size) {
    // Ensure that the calculated size accounts for all bit fields and pads properly
    UA_ObjectWithBitField *item0 = UA_ObjectWithBitField_new();
    size_t encodedSize = UA_ObjectWithBitField_calcSizeBinary(item0);
    UA_ObjectWithBitField_delete(item0);
    ck_assert_uint_eq(encodedSize, 2); // 1 byte data + 1 byte for the encoding mask

    UA_ObjectWithNineBits *item1 = UA_ObjectWithNineBits_new();
    item1->bitField8 = true;
    encodedSize = UA_ObjectWithNineBits_calcSizeBinary(item1);
    UA_ObjectWithNineBits_delete(item1);
    ck_assert_uint_eq(encodedSize, 3); // 1 byte data + 2 bytes for the encoding mask
}
END_TEST

START_TEST(Datatypes_check_encoding_mask_size_with_option) {
    // Ensure that disabled optional fields are accounted for in the size calculations
    UA_ObjectWithBitsOptionalFields item;
    UA_ObjectWithBitsOptionalFields_init(&item);

    item.optionOne = false;
    item.optionTwo = false;
    item.optionalByteOne = 0x55;
    item.optionalByteTwo = 0x42;

    size_t encodedSize = UA_ObjectWithBitsOptionalFields_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 1); // 1 byte encoding mask

    item.optionOne = true;
    item.optionTwo = false;
    encodedSize = UA_ObjectWithBitsOptionalFields_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 2); // 1 byte encoding mask, one optional byte

    item.optionOne = false;
    item.optionTwo = true;
    encodedSize = UA_ObjectWithBitsOptionalFields_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 2); // 1 byte encoding mask + one optional byte

    item.optionOne = true;
    item.optionTwo = true;
    encodedSize = UA_ObjectWithBitsOptionalFields_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 3); // 1 byte encoding mask + two optional bytes
}
END_TEST

START_TEST(Datatypes_check_encoding_mask_serialize) {
    UA_ObjectWithNineBits *item = UA_ObjectWithNineBits_new();

    item->bitField0 = true;
    item->bitField2 = true;
    item->bitField4 = true;
    item->bitField6 = true;
    item->bitField8 = true; // Optional field switch, influences binary size calculation
    item->data = 42;

    size_t buflen = UA_calcSizeBinary(item, &TEST_TYPES[TEST_TYPES_OBJECTWITHNINEBITS]);
    UA_ByteString buf;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

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
                0x01, // 0b00000001
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
    ck_assert(item.bitField8);
    ck_assert_int_eq(item.data, 18);
}
END_TEST

START_TEST(Datatypes_check_optional_fields_encoding) {
    // given an object with enabled and disabled options
    UA_ObjectWithBitsOptionalFields option_item;
    UA_ObjectWithBitsOptionalFields_init(&option_item);

    option_item.optionOne = false;
    option_item.optionTwo = true;
    option_item.optionalByteOne = 0x12;
    option_item.optionalByteTwo = 0x23;

    // if serialized into a buffer to hold the encoding mask and optional fields only
    UA_Byte data[] = { 0x55, 0x55 };
    UA_ByteString dst = { 2, data };

    UA_Byte *bufPos = dst.data;
    const UA_Byte *bufEnd = &dst.data[dst.length];

    UA_StatusCode retval = UA_ObjectWithBitsOptionalFields_encodeBinary(&option_item, &bufPos, bufEnd);
    // encoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(dst.data[0], 0x02); // Encoding mask indicates optionTwo
    ck_assert_int_eq(dst.data[1], 0x23); // Value is optionalByteTwo
}
END_TEST

START_TEST(Datatypes_check_optional_fields_decoding) {
    // given an object with enabled and disabled options
    UA_ObjectWithBitsOptionalFields option_item;
    UA_ObjectWithBitsOptionalFields_init(&option_item);

    // if serialized into a buffer to hold the encoding mask and optional fields only
    UA_Byte singleData[] = { 0x01, 0x54 }; // Option one enabled
    UA_ByteString singleSrc = { 2, singleData };

    size_t offset = 0;
    UA_StatusCode retval = UA_ObjectWithBitsOptionalFields_decodeBinary(&singleSrc, &offset, &option_item);
    // decoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(option_item.optionOne);
    ck_assert(!option_item.optionTwo);
    ck_assert_int_eq(option_item.optionalByteOne, 0x54);
    ck_assert_int_eq(option_item.optionalByteTwo, 0);

    // switch fields
    singleSrc.data[0] = 2; // Option two enabled
    offset = 0;
    UA_ObjectWithBitsOptionalFields_init(&option_item); // clear the item
    retval = UA_ObjectWithBitsOptionalFields_decodeBinary(&singleSrc, &offset, &option_item);
    // decoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(!option_item.optionOne);
    ck_assert(option_item.optionTwo);
    ck_assert_int_eq(option_item.optionalByteOne, 0);
    ck_assert_int_eq(option_item.optionalByteTwo, 0x54);

    // Test decoding of two optional fields
    UA_Byte dualData[] = { 0x03, 0x12, 0x34 };
    UA_ByteString doubleSrc = { 3, dualData };

    offset = 0;
    UA_ObjectWithBitsOptionalFields_init(&option_item); // clear the item
    retval = UA_ObjectWithBitsOptionalFields_decodeBinary(&doubleSrc, &offset, &option_item);
    // decoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(option_item.optionOne);
    ck_assert(option_item.optionTwo);
    ck_assert_int_eq(option_item.optionalByteOne, 0x12);
    ck_assert_int_eq(option_item.optionalByteTwo, 0x34);
}
END_TEST

START_TEST(Datatypes_check_optional_fields_fieldValue) {
    // given an object with enabled and disabled options and fieldValue 0
    UA_SwitchValueObject option_item;
    UA_SwitchValueObject_init(&option_item);

    option_item.optionOne = false;
    option_item.optionTwo = false;
    option_item.notOne = 0x12;
    option_item.notTwo = 0x23;
    option_item.yesTwo = 0x45;
    option_item.alsoNotOne = 0x67;

    size_t buflen = UA_SwitchValueObject_calcSizeBinary(&option_item);
    ck_assert_int_eq(buflen, 4);

    UA_Byte data[] = { 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 4, data };

    UA_Byte *bufPos = dst.data;
    const UA_Byte *bufEnd = &dst.data[dst.length];

    UA_StatusCode retval = UA_SwitchValueObject_encodeBinary(&option_item, &bufPos, bufEnd);
    // encoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(dst.data[0], 0x00); // Encoding mask is empty
    ck_assert_int_eq(dst.data[1], 0x12); // notOne
    ck_assert_int_eq(dst.data[2], 0x23); // notTwo
    ck_assert_int_eq(dst.data[3], 0x67); // alsoNotOne

    // Alter the options and encode again
    option_item.optionOne = true;
    option_item.optionTwo = true;

    buflen = UA_SwitchValueObject_calcSizeBinary(&option_item);
    ck_assert_int_eq(buflen, 2); // encoding mask and yesTwo

    UA_Byte data2[] = { 0x55, 0x55 };
    UA_ByteString dst2 = { 2, data2 };

    UA_Byte *bufPos2 = dst2.data;
    const UA_Byte *bufEnd2 = &dst2.data[dst2.length];

    retval = UA_SwitchValueObject_encodeBinary(&option_item, &bufPos2, bufEnd2);
    // encoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(dst2.data[0], 0x03); // Encoding mask
    ck_assert_int_eq(dst2.data[1], 0x45); // yesTwo
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Datatype generator test");
    TCase *tc_server = tcase_create("Optional field test");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_members);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_size);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_size_with_option);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_serialize);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_deserialize);
    tcase_add_test(tc_server, Datatypes_check_optional_fields_encoding);
    tcase_add_test(tc_server, Datatypes_check_optional_fields_decoding);
    tcase_add_test(tc_server, Datatypes_check_optional_fields_fieldValue);
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
