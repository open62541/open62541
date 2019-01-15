/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server.h"
#include "ua_config_default.h"

#include "test_types_generated.h"
#include "test_types_generated_handling.h"
#include "test_types_generated_encoding_binary.h"

#include "check.h"

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

START_TEST(Datatypes_check_bitfield_members) {
    // Ensure all bitfield members are accounted for, even padding members
    int memberCount = TEST_TYPES[TEST_TYPES_OBJECTWITHBITFIELD].membersSize;
    ck_assert_uint_eq(memberCount, 3); // bitfield, padding field and proper member

    memberCount = TEST_TYPES[TEST_TYPES_OBJECTWITHBITSOPTIONALFIELDS].membersSize;
    ck_assert_uint_eq(memberCount, 5); // Two bit fields, padding bitfield and two members
}
END_TEST

START_TEST(Datatypes_check_bitfield_calculated_size) {
    // Ensure that the calculated size accounts for all bit fields
    size_t itemSize = sizeof(UA_ObjectWithBitField);
    UA_ObjectWithBitField *item0 = UA_ObjectWithBitField_new();
    size_t encodedSize = UA_ObjectWithBitField_calcSizeBinary(item0);
    UA_ObjectWithBitField_delete(item0);
    ck_assert_uint_eq(encodedSize, itemSize); // bitfield including padding fits one byte, one byte payload, nothing optional
}
END_TEST


START_TEST(Datatypes_check_encoding_mask_size) {
    const size_t expectedSize = sizeof(UA_ObjectWithBitField);
    // The two StructuredTypes are declared the same way, except for a SwitchField
    UA_ObjectWithBitField *item0 = UA_ObjectWithBitField_new();
    size_t encodedSize = UA_ObjectWithBitField_calcSizeBinary(item0);
    UA_ObjectWithBitField_delete(item0);
    ck_assert_uint_eq(encodedSize, expectedSize);

    UA_ObjectWithBitFieldAndOption *item1 = UA_ObjectWithBitFieldAndOption_new();
    item1->bitField = 1; // Make sure the option is enabled
    encodedSize = UA_ObjectWithBitFieldAndOption_calcSizeBinary(item1);
    UA_ObjectWithBitFieldAndOption_delete(item1);
    ck_assert_uint_eq(encodedSize, expectedSize + 4);
}
END_TEST

START_TEST(Datatypes_check_optional_field_sizes) {
    // Ensure that disabled optional fields are accounted for in the size calculations
    UA_ObjectWithBitsOptionalFields item;
    UA_ObjectWithBitsOptionalFields_init(&item);

    item.optionOne = false;
    item.optionTwo = false;
    item.optionalByteOne = 0x55;
    item.optionalByteTwo = 0x42;

    size_t encodedSize = UA_ObjectWithBitsOptionalFields_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 5); // 4 bytes encoding mask, 1 byte bitflags

    item.optionOne = true;
    item.optionTwo = false;
    encodedSize = UA_ObjectWithBitsOptionalFields_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 6); // 4 bytes encoding mask, 1 byte bitflags, 1 optional byte

    item.optionOne = false;
    item.optionTwo = true;
    encodedSize = UA_ObjectWithBitsOptionalFields_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 6); // 4 bytes encoding mask, 1 byte bitflags, 1 optional byte

    item.optionOne = true;
    item.optionTwo = true;
    encodedSize = UA_ObjectWithBitsOptionalFields_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 7); // 4 bytes encoding mask, 1 byte bitflags, 2 optional bytes
}
END_TEST

START_TEST(Datatypes_check_multi_switch_size) {
    // Ensure that disabled optional fields are accounted for in the size calculations
    UA_DoubleSwitch item;
    UA_DoubleSwitch_init(&item);

    item.option = false;
    item.optionalByteOne = 0x55;
    item.optionalByteTwo = 0x42;

    size_t encodedSize = UA_DoubleSwitch_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 5); // 4 bytes encoding mask, 1 byte bitflags

    item.option = true;
    encodedSize = UA_DoubleSwitch_calcSizeBinary(&item);
    ck_assert_uint_eq(encodedSize, 7); // 4 bytes encoding mask, 1 byte bitflags, 2 optional byte
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
    ck_assert_uint_eq(buflen, 7); // 4 bytes encoding mask, 2 bytes bitflags, 1 byte data
    UA_ByteString buf;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buf.data;
    const UA_Byte *bufEnd = &buf.data[buf.length];

    retval = UA_ObjectWithNineBits_encodeBinary(item, &bufPos, bufEnd);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_ObjectWithNineBits_delete(item);

    ck_assert_uint_eq(*((const UA_UInt32*)buf.data), 1); // encoding mask
    ck_assert_uint_eq(buf.data[4], 0x55); // 0b10101010 bitflag byte 1
    ck_assert_uint_eq(buf.data[5], 0x01); // 0b00000001 bitflag byte 2
    ck_assert_uint_eq(buf.data[6], 42);   // Byte payload as set
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(Datatypes_check_encoding_mask_full_range) {
    UA_MaximumOptional *item = UA_MaximumOptional_new();

    item->optionOne = true;
    item->optionTwo = true;
    item->optionOther = false;

    size_t buflen = UA_calcSizeBinary(item, &TEST_TYPES[TEST_TYPES_MAXIMUMOPTIONAL]);
    ck_assert_uint_eq(buflen, 7); // 4 bytes encoding mask, 1 byte bitflags, 2 bytes data
    UA_ByteString buf;
    UA_StatusCode retval = UA_ByteString_allocBuffer(&buf, buflen);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_Byte *bufPos = buf.data;
    const UA_Byte *bufEnd = &buf.data[buf.length];

    retval = UA_MaximumOptional_encodeBinary(item, &bufPos, bufEnd);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_MaximumOptional_delete(item);

    ck_assert_uint_eq(*((const UA_UInt32*)buf.data), 0x20000002); // encoding mask 0b00100000000000000000000000000010
    ck_assert_uint_eq(buf.data[4], 0x03); // 0b00000101 bitflag byte 1
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(Datatypes_check_encoding_mask_deserialize) {
    UA_Byte data[] = {
                0x00, 0x00, 0x00, 0x00, // 4 byte encoding mask, will set later
                0x89, 0x01, // bitfields
                0x12, // Data 18
            };
    *((UA_UInt32*)data) = 1; // Set the correct flag in the encoding mask
    UA_ByteString src    = { 7, data };

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
    UA_Byte data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 7, data };

    UA_Byte *bufPos = dst.data;
    const UA_Byte *bufEnd = &dst.data[dst.length];

    UA_StatusCode retval = UA_ObjectWithBitsOptionalFields_encodeBinary(&option_item, &bufPos, bufEnd);
    // encoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(*((const UA_UInt32*)dst.data), 2); // encoding mask 0b00000000000000000000000000000010
    ck_assert_int_eq(dst.data[5], 0x23); // Value is optionalByteTwo
}
END_TEST

START_TEST(Datatypes_check_optional_fields_decoding) {
    // given an object with enabled and disabled options
    UA_ObjectWithBitsOptionalFields option_item;
    UA_ObjectWithBitsOptionalFields_init(&option_item);

    // if serialized into a buffer to hold the encoding mask and optional fields only
    UA_Byte singleData[] = { 0x55, 0x55, 0x55, 0x55, 0x01, 0x54 };
    UA_ByteString singleSrc = { 6, singleData };
    *((UA_UInt32*)singleData) = 1;

    size_t offset = 0;
    UA_StatusCode retval = UA_ObjectWithBitsOptionalFields_decodeBinary(&singleSrc, &offset, &option_item);
    // decoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert(option_item.optionOne);
    ck_assert(!option_item.optionTwo);
    ck_assert_int_eq(option_item.optionalByteOne, 0x54);
    ck_assert_int_eq(option_item.optionalByteTwo, 0);

    // switch fields
    singleSrc.data[4] = 2; // Option two enabled
    *((UA_UInt32*)singleData) = 2;
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
    UA_Byte dualData[] = { 0x55, 0x55, 0x55, 0x55, 0x03, 0x12, 0x34 };
    UA_ByteString doubleSrc = { 7, dualData };
    *((UA_UInt32*)dualData) = 3; // Both optional bits enabled

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
    ck_assert_int_eq(buflen, 8);

    UA_Byte data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 8, data };

    UA_Byte *bufPos = dst.data;
    const UA_Byte *bufEnd = &dst.data[dst.length];

    UA_StatusCode retval = UA_SwitchValueObject_encodeBinary(&option_item, &bufPos, bufEnd);
    // encoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    *((UA_UInt32*)data) = 11;
    ck_assert_int_eq(dst.data[4], 0x00);
    ck_assert_int_eq(dst.data[5], 0x12);
    ck_assert_int_eq(dst.data[6], 0x23);
    ck_assert_int_eq(dst.data[7], 0x67);

    // Alter the options and encode again
    option_item.optionOne = true;
    option_item.optionTwo = true;

    buflen = UA_SwitchValueObject_calcSizeBinary(&option_item);
    ck_assert_int_eq(buflen, 6);

    UA_Byte data2[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst2 = { 6, data2 };

    UA_Byte *bufPos2 = dst2.data;
    const UA_Byte *bufEnd2 = &dst2.data[dst2.length];

    retval = UA_SwitchValueObject_encodeBinary(&option_item, &bufPos2, bufEnd2);
    // encoding should succeed
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    *((UA_UInt32*)data2) = 4;
    ck_assert_int_eq(dst2.data[4], 3); // The option bitfields
    ck_assert_int_eq(dst2.data[5], 0x45);
}
END_TEST

START_TEST(Datatypes_check_clear_object_with_option) {
    UA_ObjectWithNineBits o;
    UA_ObjectWithNineBits_init(&o);

    o.bitField0 = true;
    o.bitField1 = true;
    o.bitField2 = true;
    o.bitField3 = true;
    o.bitField4 = true;
    o.bitField5 = true;
    o.bitField6 = true;
    o.bitField7 = true;
    o.bitField8 = true;
    o.data = 0x55;

    UA_ObjectWithNineBits_clear(&o);

    ck_assert(!o.bitField0);
    ck_assert(!o.bitField1);
    ck_assert(!o.bitField2);
    ck_assert(!o.bitField3);
    ck_assert(!o.bitField4);
    ck_assert(!o.bitField5);
    ck_assert(!o.bitField6);
    ck_assert(!o.bitField7);
    ck_assert(!o.bitField8);
    ck_assert_int_eq(o.data, 0x00);

    UA_ObjectWithNineBits_deleteMembers(&o);
}
END_TEST

START_TEST(Datatypes_check_copy_object_with_option) {
    UA_ObjectWithNineBits source;
    UA_ObjectWithNineBits_init(&source);

    source.bitField0 = true;
    source.bitField1 = true;
    source.bitField5 = true;
    source.bitField7 = true;
    source.bitField8 = true;

    source.data = 0x55;

    UA_ObjectWithNineBits dest;
    UA_ObjectWithNineBits_init(&dest);

    ck_assert_int_eq(dest.data, 0x00);

    UA_ObjectWithNineBits_copy(&source, &dest);

    ck_assert(dest.bitField0);
    ck_assert(dest.bitField1);
    ck_assert(!dest.bitField2);
    ck_assert(!dest.bitField3);
    ck_assert(!dest.bitField4);
    ck_assert(dest.bitField5);
    ck_assert(!dest.bitField6);
    ck_assert(dest.bitField7);
    ck_assert(dest.bitField8);
    ck_assert_int_eq(dest.data, 0x55);

    UA_ObjectWithNineBits_deleteMembers(&source);
    UA_ObjectWithNineBits_deleteMembers(&dest);
}
END_TEST

static Suite* testSuite_Client(void) {
    Suite *s = suite_create("Datatype generator test");
    TCase *tc_server = tcase_create("Optional field test");
    tcase_add_unchecked_fixture(tc_server, setup, teardown);
    tcase_add_test(tc_server, Datatypes_check_bitfield_members);
    tcase_add_test(tc_server, Datatypes_check_bitfield_calculated_size);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_size);
    tcase_add_test(tc_server, Datatypes_check_optional_field_sizes);
    tcase_add_test(tc_server, Datatypes_check_multi_switch_size);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_serialize);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_full_range);
    tcase_add_test(tc_server, Datatypes_check_encoding_mask_deserialize);
    tcase_add_test(tc_server, Datatypes_check_optional_fields_encoding);
    tcase_add_test(tc_server, Datatypes_check_optional_fields_decoding);
    tcase_add_test(tc_server, Datatypes_check_optional_fields_fieldValue);
    tcase_add_test(tc_server, Datatypes_check_copy_object_with_option);
    tcase_add_test(tc_server, Datatypes_check_clear_object_with_option);
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
