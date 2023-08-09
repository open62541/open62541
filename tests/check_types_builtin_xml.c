/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>
#include <open62541/util.h>

#include "ua_types_encoding_xml.h"

#include <check.h>
#include <math.h>

#if defined(_MSC_VER)
# pragma warning(disable: 4146)
#endif

/* Boolean */
START_TEST(UA_Boolean_true_xml_encode) {
    UA_Boolean *src = UA_Boolean_new();
    UA_Boolean_init(src);
    *src = true;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BOOLEAN];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, 4);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "true";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Boolean_delete(src);
}
END_TEST

START_TEST(UA_Boolean_false_xml_encode) {
    UA_Boolean *src = UA_Boolean_new();
    UA_Boolean_init(src);
    *src = false;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BOOLEAN];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, 5);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "false";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Boolean_delete(src);
}
END_TEST

START_TEST(UA_Boolean_true_bufferTooSmall_xml_encode) {
    UA_Boolean *src = UA_Boolean_new();
    UA_Boolean_init(src);
    *src = false;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BOOLEAN];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_Boolean_delete(src);
}
END_TEST

/* SByte */
START_TEST(UA_SByte_Max_Number_xml_encode) {
    UA_SByte src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_SBYTE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "127";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_SByte_Min_Number_xml_encode) {
    UA_SByte src = -128;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_SBYTE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "-128";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_SByte_Zero_Number_xml_encode) {
    UA_SByte *src = UA_SByte_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_SBYTE];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_SByte_delete(src);
}
END_TEST

START_TEST(UA_SByte_smallbuf_Number_xml_encode) {
    UA_SByte *src = UA_SByte_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_SBYTE];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_SByte_delete(src);
}
END_TEST

/* Byte */
START_TEST(UA_Byte_Max_Number_xml_encode) {
    UA_Byte *src = UA_Byte_new();
    *src = 255;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTE];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "255";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Byte_delete(src);
}
END_TEST

START_TEST(UA_Byte_Min_Number_xml_encode) {
    UA_Byte src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Byte_smallbuf_Number_xml_encode) {
    UA_Byte src = 255;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTE];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
}
END_TEST

/* Int16 */
START_TEST(UA_Int16_Max_Number_xml_encode) {
    UA_Int16 *src = UA_Int16_new();
    *src = 32767;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT16];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "32767";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Int16_delete(src);
}
END_TEST

START_TEST(UA_Int16_Min_Number_xml_encode) {
    UA_Int16 *src = UA_Int16_new();
    *src = -32768;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT16];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "-32768";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Int16_delete(src);
}
END_TEST

START_TEST(UA_Int16_Zero_Number_xml_encode) {
    UA_Int16 *src = UA_Int16_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT16];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Int16_delete(src);
}
END_TEST

START_TEST(UA_Int16_smallbuf_Number_xml_encode) {
    UA_Int16 *src = UA_Int16_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT16];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_Int16_delete(src);
}
END_TEST

/* UInt16 */
START_TEST(UA_UInt16_Max_Number_xml_encode) {
    UA_UInt16 *src = UA_UInt16_new();
    *src = 65535;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT16];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "65535";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_UInt16_delete(src);
}
END_TEST

START_TEST(UA_UInt16_Min_Number_xml_encode) {
    UA_UInt16 *src = UA_UInt16_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT16];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_UInt16_delete(src);
}
END_TEST

START_TEST(UA_UInt16_smallbuf_Number_xml_encode) {
    UA_UInt16 *src = UA_UInt16_new();
    *src = 255;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT16];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_UInt16_delete(src);
}
END_TEST

/* Int32 */
START_TEST(UA_Int32_Max_Number_xml_encode) {
    UA_Int32 *src = UA_Int32_new();
    *src = 2147483647;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT32];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "2147483647";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Int32_delete(src);
}
END_TEST

START_TEST(UA_Int32_Min_Number_xml_encode) {
    UA_Int32 src = -2147483648;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT32];
    size_t size = UA_calcSizeXml((void *)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "-2147483648";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Int32_Zero_Number_xml_encode) {
    UA_Int32 *src = UA_Int32_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT32];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Int32_delete(src);
}
END_TEST

START_TEST(UA_Int32_smallbuf_Number_xml_encode) {
    UA_Int32 *src = UA_Int32_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT32];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_Int32_delete(src);
}
END_TEST

/* UInt32 */
START_TEST(UA_UInt32_Max_Number_xml_encode) {
    UA_UInt32 *src = UA_UInt32_new();
    *src = 4294967295;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT32];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "4294967295";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_UInt32_delete(src);
}
END_TEST

START_TEST(UA_UInt32_Min_Number_xml_encode) {
    UA_UInt32 *src = UA_UInt32_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT32];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_UInt32_delete(src);
}
END_TEST

START_TEST(UA_UInt32_smallbuf_Number_xml_encode) {
    UA_UInt32 *src = UA_UInt32_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT32];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_UInt32_delete(src);
}
END_TEST

/* Int64 */
START_TEST(UA_Int64_Max_Number_xml_encode) {
    UA_Int64 *src = UA_Int64_new();
    *src = 9223372036854775807LL;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT64];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "9223372036854775807";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Int64_delete(src);
}
END_TEST

START_TEST(UA_Int64_Min_Number_xml_encode) {
    UA_Int64 *src = UA_Int64_new();
    *src = -UA_INT64_MAX - 1LL;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT64];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "-9223372036854775808";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Int64_delete(src);
}
END_TEST

START_TEST(UA_Int64_Zero_Number_xml_encode) {
    UA_Int64 *src = UA_Int64_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT64];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Int64_delete(src);
}
END_TEST

START_TEST(UA_Int64_smallbuf_Number_xml_encode) {
    UA_Int64 *src = UA_Int64_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT64];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_Int64_delete(src);
}
END_TEST

/* UInt64 */
START_TEST(UA_UInt64_Max_Number_xml_encode) {
    UA_UInt64 *src = UA_UInt64_new();
    *src = 18446744073709551615ULL;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT64];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "18446744073709551615";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_UInt64_delete(src);
}
END_TEST

START_TEST(UA_UInt64_Min_Number_xml_encode) {
    UA_UInt64 *src = UA_UInt64_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT64];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_UInt64_delete(src);
}
END_TEST

START_TEST(UA_UInt64_smallbuf_Number_xml_encode) {
    UA_UInt64 *src = UA_UInt64_new();
    *src = -9223372036854775808ULL;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT64];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_UInt64_delete(src);
}
END_TEST

/* Float */
START_TEST(UA_Float_xml_encode) {
    UA_Float src = 1.0000000000F;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_FLOAT];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "1.0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

/* Double */
START_TEST(UA_Double_xml_encode) {
    UA_Double src = 1.1234;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "1.1234";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Double_pluszero_xml_encode) {
    UA_Double src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0.0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Double_minuszero_xml_encode) {
    UA_Double src = -0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "0.0";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Double_plusInf_xml_encode) {
    UA_Double src = INFINITY;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "INF";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Double_minusInf_xml_encode) {
    UA_Double src = -INFINITY;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "-INF";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Double_nan_xml_encode) {
    UA_Double src = NAN;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "NaN";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Double_onesmallest_xml_encode) {
    UA_Double src = 1.0000000000000002220446049250313080847263336181640625;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "1.0000000000000002";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

/* String */
START_TEST(UA_String_xml_encode) {
    UA_String src = UA_STRING("hello");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "hello";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_String_Empty_xml_encode) {
    UA_String src = UA_STRING("");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_String_Null_xml_encode) {
    UA_String src = UA_STRING_NULL;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "null";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_String_escapesimple_xml_encode) {
    UA_String src = UA_STRING("\b\th\"e\fl\nl\\o\r");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "\b\th\"e\fl\nl\\o\r";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_String_escapeutf_xml_encode) {
    UA_String src = UA_STRING("he\\zsdl\alo€ \x26\x3A asdasd");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "he\\zsdl\alo€ \x26\x3A asdasd";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_String_special_xml_encode) {
    UA_String src = UA_STRING("𝄞𠂊𝕥🔍");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml(&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "𝄞𠂊𝕥🔍";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_clear(&buf);
}
END_TEST

/* DateTime */
START_TEST(UA_DateTime_xml_encode) {
    UA_DateTime *src = UA_DateTime_new();
    *src = UA_DateTime_fromUnixTime(1234567);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DATETIME];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "1970-01-15T06:56:07Z";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_DateTime_delete(src);
}
END_TEST

START_TEST(UA_DateTime_xml_encode_null) {
    UA_DateTime src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DATETIME];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "1601-01-01T00:00:00Z";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_DateTime_with_nanoseconds_xml_encode) {
    UA_DateTime *src = UA_DateTime_new();
    *src = UA_DateTime_fromUnixTime(1234567) + 8901234;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DATETIME];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "1970-01-15T06:56:07.8901234Z";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_DateTime_delete(src);
}
END_TEST

/* Guid */
START_TEST(UA_Guid_xml_encode) {
    UA_Guid src = {3, 9, 10, {8, 7, 6, 5, 4, 3, 2, 1}};
    const UA_DataType *type = &UA_TYPES[UA_TYPES_GUID];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "00000003-0009-000A-0807-060504030201";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_Guid_smallbuf_xml_encode) {
    UA_Guid *src = UA_Guid_new();
    *src = UA_Guid_random();
    const UA_DataType *type = &UA_TYPES[UA_TYPES_GUID];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_Guid_delete(src);
}
END_TEST

/* NodeId */
START_TEST(UA_NodeId_Numeric_xml_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_NUMERIC(0, 5555);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "i=5555";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_Numeric_Namespace_xml_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_NUMERIC(4, 5555);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "ns=4;i=5555";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_String_xml_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_STRING_ALLOC(0, "foobar");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "s=foobar";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_String_Namespace_xml_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_STRING_ALLOC(5, "foobar");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "ns=5;s=foobar";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_Guid_xml_encode) {
    UA_NodeId *src = UA_NodeId_new();
    UA_NodeId_init(src);
    UA_Guid g = {3, 9, 10, {8, 7, 6, 5, 4, 3, 2, 1}};
    *src = UA_NODEID_GUID(0, g);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "g=00000003-0009-000a-0807-060504030201";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_Guid_Namespace_xml_encode) {
    UA_NodeId *src = UA_NodeId_new();
    UA_Guid g = {3, 9, 10, {8, 7, 6, 5, 4, 3, 2, 1}};
    *src = UA_NODEID_GUID(5, g);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, 43);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "ns=5;g=00000003-0009-000a-0807-060504030201";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_ByteString_xml_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_BYTESTRING_ALLOC(0, "asdfasdf");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, 14);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "b=YXNkZmFzZGY=";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_ByteString_Namespace_xml_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_BYTESTRING_ALLOC(5, "asdfasdf");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "ns=5;b=YXNkZmFzZGY=";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_NodeId_delete(src);
}
END_TEST

/* ExpandedNodeId */
START_TEST(UA_ExpandedNodeId_xml_encode) {
    UA_ExpandedNodeId *src = UA_ExpandedNodeId_new();
    UA_ExpandedNodeId_init(src);
    *src = UA_EXPANDEDNODEID_STRING_ALLOC(23, "testtestTest");
    src->namespaceUri = UA_STRING_ALLOC("asdf");
    src->serverIndex = 1345;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXPANDEDNODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "svr=1345;nsu=asdf;s=testtestTest";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST

START_TEST(UA_ExpandedNodeId_MissingNamespaceUri_xml_encode) {
    UA_ExpandedNodeId *src = UA_ExpandedNodeId_new();
    UA_ExpandedNodeId_init(src);
    *src = UA_EXPANDEDNODEID_STRING_ALLOC(23, "testtestTest");
    src->namespaceUri = UA_STRING_NULL;
    src->serverIndex = 1345;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXPANDEDNODEID];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char* result = "svr=1345;ns=23;s=testtestTest";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST


// ---------------------------DECODE-------------------------------------


/* Boolean */
START_TEST(UA_Boolean_true_xml_decode) {
    UA_Boolean out;
    UA_Boolean_init(&out);
    UA_ByteString buf = UA_STRING("true");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BOOLEAN], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, true);

    UA_Boolean_clear(&out);
}
END_TEST

START_TEST(UA_Boolean_false_xml_decode) {
    UA_Boolean out;
    UA_Boolean_init(&out);
    UA_ByteString buf = UA_STRING("false");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BOOLEAN], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, false);

    UA_Boolean_clear(&out);
}
END_TEST

/* SByte */
START_TEST(UA_SByte_Min_xml_decode) {
    UA_SByte out;
    UA_SByte_init(&out);
    UA_ByteString buf = UA_STRING("-128");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_SBYTE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, -128);

    UA_SByte_clear(&out);
}
END_TEST

START_TEST(UA_SByte_Max_xml_decode) {
    UA_SByte out;
    UA_SByte_init(&out);
    UA_ByteString buf = UA_STRING("127");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_SBYTE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, 127);

    UA_SByte_clear(&out);
}
END_TEST

/* Byte */
START_TEST(UA_Byte_Min_xml_decode) {
    UA_Byte out;
    UA_Byte_init(&out);
    UA_ByteString buf = UA_STRING("0");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BYTE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 0);

    UA_Byte_clear(&out);
}
END_TEST

START_TEST(UA_Byte_Max_xml_decode) {
    UA_Byte out;
    UA_Byte_init(&out);
    UA_ByteString buf = UA_STRING("255");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BYTE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 255);

    UA_Byte_clear(&out);
}
END_TEST

/* Int16 */
START_TEST(UA_Int16_Min_xml_decode) {
    UA_Int16 out;
    UA_Int16_init(&out);
    UA_ByteString buf = UA_STRING("-32768");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT16], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, -32768);

    UA_Int16_clear(&out);
}
END_TEST

START_TEST(UA_Int16_Max_xml_decode) {
    UA_Int16 out;
    UA_Int16_init(&out);
    UA_ByteString buf = UA_STRING("32767");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT16], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, 32767);

    UA_Int16_clear(&out);
}
END_TEST

/* UInt16 */
START_TEST(UA_UInt16_Min_xml_decode) {
    UA_UInt16 out;
    UA_UInt16_init(&out);
    UA_ByteString buf = UA_STRING("0");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT16], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 0);

    UA_UInt16_clear(&out);
}
END_TEST

START_TEST(UA_UInt16_Max_xml_decode) {
    UA_UInt16 out;
    UA_UInt16_init(&out);
    UA_ByteString buf = UA_STRING("65535");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT16], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 65535);

    UA_UInt16_clear(&out);
}
END_TEST

/* Int32 */
START_TEST(UA_Int32_Min_xml_decode) {
    UA_Int32 out;
    UA_Int32_init(&out);
    UA_ByteString buf = UA_STRING("-2147483648");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT32], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(out == -2147483648);

    UA_Int32_clear(&out);
}
END_TEST

START_TEST(UA_Int32_Max_xml_decode) {
    UA_Int32 out;
    UA_Int32_init(&out);
    UA_ByteString buf = UA_STRING("2147483647");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT32], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, 2147483647);

    UA_Int32_clear(&out);
}
END_TEST

/* UInt32 */
START_TEST(UA_UInt32_Min_xml_decode) {
    UA_UInt32 out;
    UA_UInt32_init(&out);
    UA_ByteString buf = UA_STRING("0");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT32], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 0);

    UA_UInt32_clear(&out);
}
END_TEST

START_TEST(UA_UInt32_Max_xml_decode) {
    UA_UInt32 out;
    UA_UInt32_init(&out);
    UA_ByteString buf = UA_STRING("4294967295");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT32], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 4294967295);

    UA_UInt32_clear(&out);
}
END_TEST

/* Int64 */
START_TEST(UA_Int64_Min_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("-9223372036854775808");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0x00);
    ck_assert_int_eq(((u8*)&out)[7], 0x80);

    UA_Int64_clear(&out);
}
END_TEST

START_TEST(UA_Int64_Max_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("9223372036854775807");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0xFF);
    ck_assert_int_eq(((u8*)&out)[1], 0xFF);
    ck_assert_int_eq(((u8*)&out)[2], 0xFF);
    ck_assert_int_eq(((u8*)&out)[3], 0xFF);
    ck_assert_int_eq(((u8*)&out)[4], 0xFF);
    ck_assert_int_eq(((u8*)&out)[5], 0xFF);
    ck_assert_int_eq(((u8*)&out)[6], 0xFF);
    ck_assert_int_eq(((u8*)&out)[7], 0x7F);

    UA_Int64_clear(&out);
}
END_TEST

START_TEST(UA_Int64_Overflow_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("9223372036854775808");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

START_TEST(UA_Int64_TooBig_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("111111111111111111111111111111");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

START_TEST(UA_Int64_NoDigit_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("a");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

/* UInt64 */
START_TEST(UA_UInt64_Min_xml_decode) {
    UA_UInt64 out;
    UA_UInt64_init(&out);
    UA_ByteString buf = UA_STRING("0");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0x00);
    ck_assert_int_eq(((u8*)&out)[7], 0x00);

    UA_UInt64_clear(&out);
}
END_TEST

START_TEST(UA_UInt64_Max_xml_decode) {
    UA_UInt64 out;
    UA_UInt64_init(&out);
    UA_ByteString buf = UA_STRING("18446744073709551615");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0xFF);
    ck_assert_int_eq(((u8*)&out)[1], 0xFF);
    ck_assert_int_eq(((u8*)&out)[2], 0xFF);
    ck_assert_int_eq(((u8*)&out)[3], 0xFF);
    ck_assert_int_eq(((u8*)&out)[4], 0xFF);
    ck_assert_int_eq(((u8*)&out)[5], 0xFF);
    ck_assert_int_eq(((u8*)&out)[6], 0xFF);
    ck_assert_int_eq(((u8*)&out)[7], 0xFF);

    UA_UInt64_clear(&out);
}
END_TEST

START_TEST(UA_UInt64_Overflow_xml_decode) {
    UA_UInt64 out;
    UA_UInt64_init(&out);
    UA_ByteString buf = UA_STRING("18446744073709551616");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_UInt64_clear(&out);
}
END_TEST

START_TEST(UA_UInt64_TooBig_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("111111111111111111111111111111");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

START_TEST(UA_UInt64_NoDigit_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("a");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

/* Float */
START_TEST(UA_Float_xml_decode) {
    UA_Float out;
    UA_Float_init(&out);
    UA_ByteString buf = UA_STRING("3.1415927410");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_FLOAT], NULL);

    // 0 10000000 10010010000111111011011
    // 40 49 0f db
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0xdb);
    ck_assert_int_eq(((u8*)&out)[1], 0x0f);
    ck_assert_int_eq(((u8*)&out)[2], 0x49);
    ck_assert_int_eq(((u8*)&out)[3], 0x40);
    UA_Float_clear(&out);
}
END_TEST

START_TEST(UA_Float_xml_one_decode) {
    UA_Float out;
    UA_Float_init(&out);
    UA_ByteString buf = UA_STRING("1");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_FLOAT], NULL);

    // 0 01111111 00000000000000000000000
    // 3f 80 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x80);
    ck_assert_int_eq(((u8*)&out)[3], 0x3f);

    UA_Float_clear(&out);
}
END_TEST

START_TEST(UA_Float_xml_inf_decode) {
    UA_Float out;
    UA_Float_init(&out);
    UA_ByteString buf = UA_STRING("INF");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_FLOAT], NULL);

    // 0 11111111 00000000000000000000000
    // 7f 80 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x80);
    ck_assert_int_eq(((u8*)&out)[3], 0x7f);

    UA_Float_clear(&out);
}
END_TEST

START_TEST(UA_Float_xml_neginf_decode) {
    UA_Float out;
    UA_Float_init(&out);
    UA_ByteString buf = UA_STRING("-INF");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_FLOAT], NULL);

    // 1 11111111 00000000000000000000000
    // ff 80 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x80);
    ck_assert_int_eq(((u8*)&out)[3], 0xff);

    UA_Float_clear(&out);
}
END_TEST

START_TEST(UA_Float_xml_nan_decode) {
    UA_Float out;
    UA_Float_init(&out);
    UA_ByteString buf = UA_STRING("NaN");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_FLOAT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
#if !defined(__TINYC__) && (defined(__clang__) || ((!defined(__aarch64__) && !defined(__amd64__)) && ((defined(__GNUC__) && __GNUC__ < 11))))
    // gcc 32-bit and linux clang specific
    // 0 11111111 10000000000000000000000
    // 7f c0 00 00
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0xc0);
    ck_assert_int_eq(((u8*)&out)[3], 0x7f);
#else
    // 1 11111111 10000000000000000000000
    // ff c0 00 00
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0xc0);
    ck_assert_int_eq(((u8*)&out)[3], 0xff);
#endif

    UA_Float val = out;
    ck_assert(val != val); /* Check if not a number */

    UA_Float_clear(&out);
}
END_TEST

START_TEST(UA_Float_xml_negnan_decode) {
    UA_Float out;
    UA_Float_init(&out);
    UA_ByteString buf = UA_STRING("-NaN");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_FLOAT], NULL);

    // 1 11111111 10000000000000000000000
    // ff c0 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0xc0);
    ck_assert_int_eq(((u8*)&out)[3], 0xff);

    UA_Float val = out;
    ck_assert(val != val); /* Check if not a number */

    UA_Float_clear(&out);
}
END_TEST

/* Double */
START_TEST(UA_Double_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("1.1234");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    // 0 01111111111 0001111110010111001001000111010001010011100011101111
    // 3f f1 f9 72 47 45 38 ef
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0xef);
    ck_assert_int_eq(((u8*)&out)[1], 0x38);
    ck_assert_int_eq(((u8*)&out)[2], 0x45);
    ck_assert_int_eq(((u8*)&out)[3], 0x47);
    ck_assert_int_eq(((u8*)&out)[4], 0x72);
    ck_assert_int_eq(((u8*)&out)[5], 0xf9);
    ck_assert_int_eq(((u8*)&out)[6], 0xf1);
    ck_assert_int_eq(((u8*)&out)[7], 0x3f);

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_corrupt_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("1.12.34");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_one_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("1");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    // 0 01111111111 0000000000000000000000000000000000000000000000000000
    // 3f f0 00 00 00 00 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0xf0);
    ck_assert_int_eq(((u8*)&out)[7], 0x3f);

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_onepointsmallest_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("1.0000000000000002");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    // 0 01111111111 0000000000000000000000000000000000000000000000000001
    // 3f f0 00 00 00 00 00 01
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x01);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0xf0);
    ck_assert_int_eq(((u8*)&out)[7], 0x3f);

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_nan_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("NaN");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
#if !defined(__TINYC__) && (defined(__clang__) || ((!defined(__aarch64__) && !defined(__amd64__)) && ((defined(__GNUC__) && __GNUC__ < 11))))
    // gcc 32-bit and linux clang specific
    // 0 11111111111 1000000000000000000000000000000000000000000000000000
    // 7f f8 00 00 00 00 00 00
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0xf8);
    ck_assert_int_eq(((u8*)&out)[7], 0x7f);
#else
    // 1 11111111111 1000000000000000000000000000000000000000000000000000
    // ff f8 00 00 00 00 00 00
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0xf8);
    ck_assert_int_eq(((u8*)&out)[7], 0xff);
#endif

    UA_Double val = out;
    ck_assert(val != val); /* Check if not a number */

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_negnan_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("-NaN");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    // 1 11111111111 1000000000000000000000000000000000000000000000000000
    // ff f8 00 00 00 00 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0xf8);
    ck_assert_int_eq(((u8*)&out)[7], 0xff);

    UA_Double val = out;
    ck_assert(val != val); /* Check if not a number */

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_inf_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("INF");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    // 0 11111111111 0000000000000000000000000000000000000000000000000000
    // 7f f0 00 00 00 00 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0xf0);
    ck_assert_int_eq(((u8*)&out)[7], 0x7f);

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_neginf_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("-INF");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    // 1 11111111111 0000000000000000000000000000000000000000000000000000
    // ff f0 00 00 00 00 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0xf0);
    ck_assert_int_eq(((u8*)&out)[7], 0xff);

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_zero_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("0");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    // 0 00000000000 0000000000000000000000000000000000000000000000000000
    // 00 00 00 00 00 00 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0x00);
    ck_assert_int_eq(((u8*)&out)[7], 0x00);

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_negzero_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("-0");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    // 1 00000000000 0000000000000000000000000000000000000000000000000000
    // 80 00 00 00 00 00 00 00
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)&out)[0], 0x00);
    ck_assert_int_eq(((u8*)&out)[1], 0x00);
    ck_assert_int_eq(((u8*)&out)[2], 0x00);
    ck_assert_int_eq(((u8*)&out)[3], 0x00);
    ck_assert_int_eq(((u8*)&out)[4], 0x00);
    ck_assert_int_eq(((u8*)&out)[5], 0x00);
    ck_assert_int_eq(((u8*)&out)[6], 0x00);
    ck_assert_int_eq(((u8*)&out)[7], 0x80);

    UA_Double_clear(&out);
}
END_TEST

/* String */
START_TEST(UA_String_xml_decode) {
    UA_String out;
    UA_String_init(&out);
    UA_ByteString buf = UA_STRING("abcdef");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_STRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 6);
    ck_assert_int_eq(out.data[0], 'a');
    ck_assert_int_eq(out.data[1], 'b');
    ck_assert_int_eq(out.data[2], 'c');
    ck_assert_int_eq(out.data[3], 'd');
    ck_assert_int_eq(out.data[4], 'e');
    ck_assert_int_eq(out.data[5], 'f');
}
END_TEST

START_TEST(UA_String_empty_xml_decode) {
    UA_String out;
    UA_String_init(&out);
    UA_ByteString buf = UA_STRING("");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_STRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 0);
    ck_assert_ptr_eq(out.data, UA_EMPTY_ARRAY_SENTINEL);
}
END_TEST

START_TEST(UA_String_unescapeBS_xml_decode) {
    UA_String out;
    UA_String_init(&out);
    UA_ByteString buf = UA_STRING("ab\tcdef");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_STRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 7);
    ck_assert_int_eq(out.data[0], 'a');
    ck_assert_int_eq(out.data[1], 'b');
    ck_assert_int_eq(out.data[2], '\t');
    ck_assert_int_eq(out.data[3], 'c');
    ck_assert_int_eq(out.data[4], 'd');
    ck_assert_int_eq(out.data[5], 'e');
    ck_assert_int_eq(out.data[6], 'f');
}
END_TEST

START_TEST(UA_String_escape2_xml_decode) {
    UA_String out;
    UA_String_init(&out);
    UA_ByteString buf = UA_STRING("\b\th\"e\fl\nl\\o\r");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_STRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 12);
    ck_assert_int_eq(out.data[0], '\b');
    ck_assert_int_eq(out.data[1], '\t');
    ck_assert_int_eq(out.data[2], 'h');
    ck_assert_int_eq(out.data[3], '\"');
    ck_assert_int_eq(out.data[4], 'e');
    ck_assert_int_eq(out.data[5], '\f');
    ck_assert_int_eq(out.data[6], 'l');
    ck_assert_int_eq(out.data[7], '\n');
    ck_assert_int_eq(out.data[8], 'l');
    ck_assert_int_eq(out.data[9], '\\');
    ck_assert_int_eq(out.data[10], 'o');
    ck_assert_int_eq(out.data[11], '\r');
}
END_TEST

/* DateTime */
START_TEST(UA_DateTime_xml_decode) {
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_ByteString buf = UA_STRING("1970-01-02T01:02:03.005Z");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
    ck_assert_int_eq(dts.year, 1970);
    ck_assert_int_eq(dts.month, 1);
    ck_assert_int_eq(dts.day, 2);
    ck_assert_int_eq(dts.hour, 1);
    ck_assert_int_eq(dts.min, 2);
    ck_assert_int_eq(dts.sec, 3);
    ck_assert_int_eq(dts.milliSec, 5);
    ck_assert_int_eq(dts.microSec, 0);
    ck_assert_int_eq(dts.nanoSec, 0);

    UA_DateTime_clear(&out);
}
END_TEST

START_TEST(UA_DateTime_xml_decode_large) {
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_ByteString buf = UA_STRING("10970-01-02T01:02:03.005Z");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
    ck_assert_int_eq(dts.year, 10970);
    ck_assert_int_eq(dts.month, 1);
    ck_assert_int_eq(dts.day, 2);
    ck_assert_int_eq(dts.hour, 1);
    ck_assert_int_eq(dts.min, 2);
    ck_assert_int_eq(dts.sec, 3);
    ck_assert_int_eq(dts.milliSec, 5);
    ck_assert_int_eq(dts.microSec, 0);
    ck_assert_int_eq(dts.nanoSec, 0);
    UA_DateTime_clear(&out);
}
END_TEST

START_TEST(UA_DateTime_xml_decode_negative) {
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_ByteString buf = UA_STRING("-0050-01-02T01:02:03.005Z");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
    ck_assert_int_eq(dts.year, -50);
    ck_assert_int_eq(dts.month, 1);
    ck_assert_int_eq(dts.day, 2);
    ck_assert_int_eq(dts.hour, 1);
    ck_assert_int_eq(dts.min, 2);
    ck_assert_int_eq(dts.sec, 3);
    ck_assert_int_eq(dts.milliSec, 5);
    ck_assert_int_eq(dts.microSec, 0);
    ck_assert_int_eq(dts.nanoSec, 0);

    UA_DateTime_clear(&out);
}
END_TEST

START_TEST(UA_DateTime_xml_decode_min) {
    UA_DateTime dt_min = (UA_DateTime)UA_INT64_MIN;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DATETIME];

    UA_Byte data[128];
    UA_ByteString buf;
    buf.data = data;
    buf.length = 128;

    status s = UA_encodeXml((void*)&dt_min, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    UA_DateTime out;
    s = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(dt_min, out);
    UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
    ck_assert_int_eq(dts.year, -27627);
    ck_assert_int_eq(dts.month, 4);
    ck_assert_int_eq(dts.day, 19);
    ck_assert_int_eq(dts.hour, 21);
    ck_assert_int_eq(dts.min, 11);
    ck_assert_int_eq(dts.sec, 54);
    ck_assert_int_eq(dts.milliSec, 522);
    ck_assert_int_eq(dts.microSec, 419);
    ck_assert_int_eq(dts.nanoSec, 200);
}
END_TEST

START_TEST(UA_DateTime_xml_decode_max) {
    UA_DateTime dt_max = (UA_DateTime)UA_INT64_MAX;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DATETIME];

    UA_Byte data[128];
    UA_ByteString buf;
    buf.data = data;
    buf.length = 128;

    status s = UA_encodeXml((void*)&dt_max, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    UA_DateTime out;
    s = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(dt_max, out);

    UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
    ck_assert_int_eq(dts.year, 30828);
    ck_assert_int_eq(dts.month, 9);
    ck_assert_int_eq(dts.day, 14);
    ck_assert_int_eq(dts.hour, 2);
    ck_assert_int_eq(dts.min, 48);
    ck_assert_int_eq(dts.sec, 5);
    ck_assert_int_eq(dts.milliSec, 477);
    ck_assert_int_eq(dts.microSec, 580);
    ck_assert_int_eq(dts.nanoSec, 700);
}
END_TEST

START_TEST(UA_DateTime_micro_xml_decode) {
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_ByteString buf = UA_STRING("1970-01-02T01:02:03.042Z");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
    ck_assert_int_eq(dts.year, 1970);
    ck_assert_int_eq(dts.month, 1);
    ck_assert_int_eq(dts.day, 2);
    ck_assert_int_eq(dts.hour, 1);
    ck_assert_int_eq(dts.min, 2);
    ck_assert_int_eq(dts.sec, 3);
    ck_assert_int_eq(dts.milliSec, 42);
    ck_assert_int_eq(dts.microSec, 0);
    ck_assert_int_eq(dts.nanoSec, 0);

    UA_DateTime_clear(&out);
}
END_TEST

/* Guid */
START_TEST(UA_Guid_xml_decode) {
    UA_Guid out;
    UA_Guid_init(&out);
    UA_ByteString buf = UA_STRING("00000001-0002-0003-0405-060708090A0B");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_GUID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.data1, 1);
    ck_assert_int_eq(out.data2, 2);
    ck_assert_int_eq(out.data3, 3);
    ck_assert_int_eq(out.data4[0], 4);
    ck_assert_int_eq(out.data4[1], 5);
    ck_assert_int_eq(out.data4[2], 6);
    ck_assert_int_eq(out.data4[3], 7);
    ck_assert_int_eq(out.data4[4], 8);
    ck_assert_int_eq(out.data4[5], 9);
    ck_assert_int_eq(out.data4[6], 10);
    ck_assert_int_eq(out.data4[7], 11);

    UA_Guid_clear(&out);
}
END_TEST

START_TEST(UA_Guid_lower_xml_decode) {
    UA_Guid out;
    UA_Guid_init(&out);
    UA_ByteString buf = UA_STRING("00000001-0002-0003-0405-060708090a0b");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_GUID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.data1, 1);
    ck_assert_int_eq(out.data2, 2);
    ck_assert_int_eq(out.data3, 3);
    ck_assert_int_eq(out.data4[0], 4);
    ck_assert_int_eq(out.data4[1], 5);
    ck_assert_int_eq(out.data4[2], 6);
    ck_assert_int_eq(out.data4[3], 7);
    ck_assert_int_eq(out.data4[4], 8);
    ck_assert_int_eq(out.data4[5], 9);
    ck_assert_int_eq(out.data4[6], 10);
    ck_assert_int_eq(out.data4[7], 11);

    UA_Guid_clear(&out);
}
END_TEST

START_TEST(UA_Guid_tooShort_xml_decode) {
    UA_Guid out;
    UA_Guid_init(&out);
    UA_ByteString buf = UA_STRING("00000001-00");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_GUID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Guid_clear(&out);
}
END_TEST

START_TEST(UA_Guid_tooLong_xml_decode) {
    UA_Guid out;
    UA_Guid_init(&out);
    UA_ByteString buf = UA_STRING("00000001-0002-0003-0405-060708090A0B00000001");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_GUID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Guid_clear(&out);
}
END_TEST

START_TEST(UA_Guid_wrong_xml_decode) {
    UA_Guid out;
    UA_Guid_init(&out);
    UA_ByteString buf = UA_STRING("00000=01-0002-0003-0405-060708090A0B");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_GUID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Guid_clear(&out);
}
END_TEST

/* NodeId */
START_TEST(UA_NodeId_Nummeric_xml_decode) {
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("i=42");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_NODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.identifier.numeric, 42);
    ck_assert_uint_eq(out.namespaceIndex, 0);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_NUMERIC);

    UA_NodeId_clear(&out);
}
END_TEST

START_TEST(UA_NodeId_Nummeric_Namespace_xml_decode) {
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("ns=123;i=42");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_NODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.identifier.numeric, 42);
    ck_assert_uint_eq(out.namespaceIndex, 123);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_NUMERIC);

    UA_NodeId_clear(&out);
}
END_TEST


START_TEST(UA_NodeId_String_xml_decode) {
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("s=test123");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_NODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.identifier.string.length, 7);
    ck_assert_int_eq(out.identifier.string.data[0], 't');
    ck_assert_int_eq(out.identifier.string.data[1], 'e');
    ck_assert_int_eq(out.identifier.string.data[2], 's');
    ck_assert_int_eq(out.identifier.string.data[3], 't');
    ck_assert_int_eq(out.identifier.string.data[4], '1');
    ck_assert_int_eq(out.identifier.string.data[5], '2');
    ck_assert_int_eq(out.identifier.string.data[6], '3');
    ck_assert_int_eq(out.namespaceIndex, 0);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_STRING);

    UA_NodeId_clear(&out);
}
END_TEST


START_TEST(UA_NodeId_Guid_xml_decode) {
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("g=00000001-0002-0003-0405-060708090A0B");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_NODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.namespaceIndex, 0);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_GUID);
    ck_assert_int_eq(out.identifier.guid.data1, 1);
    ck_assert_int_eq(out.identifier.guid.data2, 2);
    ck_assert_int_eq(out.identifier.guid.data3, 3);
    ck_assert_int_eq(out.identifier.guid.data4[0], 4);
    ck_assert_int_eq(out.identifier.guid.data4[1], 5);
    ck_assert_int_eq(out.identifier.guid.data4[2], 6);
    ck_assert_int_eq(out.identifier.guid.data4[3], 7);
    ck_assert_int_eq(out.identifier.guid.data4[4], 8);
    ck_assert_int_eq(out.identifier.guid.data4[5], 9);
    ck_assert_int_eq(out.identifier.guid.data4[6], 10);
    ck_assert_int_eq(out.identifier.guid.data4[7], 11);

    UA_NodeId_clear(&out);
}
END_TEST

START_TEST(UA_NodeId_ByteString_xml_decode) {
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("b=YXNkZmFzZGY=");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_NODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.namespaceIndex, 0);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_BYTESTRING);
    ck_assert_uint_eq(out.identifier.byteString.length, 8);
    ck_assert_int_eq(out.identifier.byteString.data[0], 'a');
    ck_assert_int_eq(out.identifier.byteString.data[1], 's');
    ck_assert_int_eq(out.identifier.byteString.data[2], 'd');
    ck_assert_int_eq(out.identifier.byteString.data[3], 'f');
    ck_assert_int_eq(out.identifier.byteString.data[4], 'a');
    ck_assert_int_eq(out.identifier.byteString.data[5], 's');
    ck_assert_int_eq(out.identifier.byteString.data[6], 'd');
    ck_assert_int_eq(out.identifier.byteString.data[7], 'f');

    UA_NodeId_clear(&out);
}
END_TEST

/* ExpandedNodeId */
START_TEST(UA_ExpandedNodeId_Nummeric_xml_decode) {
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("i=42");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.nodeId.identifier.numeric, 42);
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_ptr_eq(out.namespaceUri.data, NULL);
    ck_assert_uint_eq(out.namespaceUri.length, 0);
    ck_assert_int_eq(out.serverIndex, 0);

    UA_ExpandedNodeId_clear(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_String_xml_decode) {
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("s=test");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.nodeId.identifier.string.length, 4);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 't');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_ptr_eq(out.namespaceUri.data, NULL);
    ck_assert_uint_eq(out.namespaceUri.length, 0);
    ck_assert_int_eq(out.serverIndex, 0);

    UA_ExpandedNodeId_clear(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_String_Namespace_xml_decode) {
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("nsu=abcdef;s=test");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.nodeId.identifier.string.length, 4);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 't');
    ck_assert_int_eq(out.nodeId.identifier.string.data[1], 'e');
    ck_assert_int_eq(out.nodeId.identifier.string.data[2], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[3], 't');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_uint_eq(out.namespaceUri.length, 6);
    ck_assert_int_eq(out.namespaceUri.data[0], 'a');
    ck_assert_int_eq(out.namespaceUri.data[1], 'b');
    ck_assert_int_eq(out.namespaceUri.data[2], 'c');
    ck_assert_int_eq(out.namespaceUri.data[3], 'd');
    ck_assert_int_eq(out.namespaceUri.data[4], 'e');
    ck_assert_int_eq(out.namespaceUri.data[5], 'f');
    ck_assert_int_eq(out.serverIndex, 0);

    UA_ExpandedNodeId_clear(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_String_NamespaceAsIndex_xml_decode) {
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("ns=42;s=test");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.nodeId.identifier.string.length, 4);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 't');
    ck_assert_int_eq(out.nodeId.identifier.string.data[1], 'e');
    ck_assert_int_eq(out.nodeId.identifier.string.data[2], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[3], 't');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_uint_eq(out.namespaceUri.length, 0);
    ck_assert_ptr_eq(out.namespaceUri.data, NULL);
    ck_assert_uint_eq(out.nodeId.namespaceIndex, 42);
    ck_assert_uint_eq(out.serverIndex, 0);

    UA_ExpandedNodeId_clear(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_String_Namespace_ServerUri_xml_decode) {
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("svr=13;nsu=test;s=test");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.nodeId.identifier.string.length, 4);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 't');
    ck_assert_int_eq(out.nodeId.identifier.string.data[1], 'e');
    ck_assert_int_eq(out.nodeId.identifier.string.data[2], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[3], 't');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_uint_eq(out.serverIndex, 13);
    ck_assert_int_eq(out.namespaceUri.data[0], 't');
    ck_assert_int_eq(out.namespaceUri.data[1], 'e');
    ck_assert_int_eq(out.namespaceUri.data[2], 's');
    ck_assert_int_eq(out.namespaceUri.data[3], 't');

    UA_ExpandedNodeId_clear(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_ByteString_xml_decode) {
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("svr=13;nsu=test;b=YXNkZmFzZGY=");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.nodeId.identifier.string.length, 8);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 'a');
    ck_assert_int_eq(out.nodeId.identifier.string.data[1], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[2], 'd');
    ck_assert_int_eq(out.nodeId.identifier.string.data[3], 'f');
    ck_assert_int_eq(out.nodeId.identifier.string.data[4], 'a');
    ck_assert_int_eq(out.nodeId.identifier.string.data[5], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[6], 'd');
    ck_assert_int_eq(out.nodeId.identifier.string.data[7], 'f');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_BYTESTRING);
    ck_assert_uint_eq(out.serverIndex, 13);
    ck_assert_int_eq(out.namespaceUri.data[0], 't');
    ck_assert_int_eq(out.namespaceUri.data[1], 'e');
    ck_assert_int_eq(out.namespaceUri.data[2], 's');
    ck_assert_int_eq(out.namespaceUri.data[3], 't');

    UA_ExpandedNodeId_clear(&out);
}
END_TEST


static Suite *testSuite_builtin_xml(void) {
    Suite *s = suite_create("Built-in Data Types 62541-5 Xml");

    TCase *tc_xml_encode = tcase_create("xml_encode");
    tcase_add_test(tc_xml_encode, UA_Boolean_true_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Boolean_false_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Boolean_true_bufferTooSmall_xml_encode);

    tcase_add_test(tc_xml_encode, UA_SByte_Max_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_SByte_Min_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_SByte_Zero_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_SByte_smallbuf_Number_xml_encode);

    tcase_add_test(tc_xml_encode, UA_Byte_Max_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Byte_Min_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Byte_smallbuf_Number_xml_encode);

    tcase_add_test(tc_xml_encode, UA_Int16_Max_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int16_Min_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int16_Zero_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int16_smallbuf_Number_xml_encode);

    tcase_add_test(tc_xml_encode, UA_UInt16_Max_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_UInt16_Min_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_UInt16_smallbuf_Number_xml_encode);

    tcase_add_test(tc_xml_encode, UA_Int32_Max_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int32_Min_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int32_Zero_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int32_smallbuf_Number_xml_encode);

    tcase_add_test(tc_xml_encode, UA_UInt32_Max_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_UInt32_Min_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_UInt32_smallbuf_Number_xml_encode);

    tcase_add_test(tc_xml_encode, UA_Int64_Max_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int64_Min_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int64_Zero_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Int64_smallbuf_Number_xml_encode);

    tcase_add_test(tc_xml_encode, UA_UInt64_Max_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_UInt64_Min_Number_xml_encode);
    tcase_add_test(tc_xml_encode, UA_UInt64_smallbuf_Number_xml_encode);

    tcase_add_test(tc_xml_encode, UA_Float_xml_encode);

    tcase_add_test(tc_xml_encode, UA_Double_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Double_onesmallest_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Double_pluszero_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Double_minuszero_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Double_plusInf_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Double_minusInf_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Double_nan_xml_encode);

    tcase_add_test(tc_xml_encode, UA_String_xml_encode);
    tcase_add_test(tc_xml_encode, UA_String_Empty_xml_encode);
    tcase_add_test(tc_xml_encode, UA_String_Null_xml_encode);
    tcase_add_test(tc_xml_encode, UA_String_escapesimple_xml_encode);
    tcase_add_test(tc_xml_encode, UA_String_escapeutf_xml_encode);
    tcase_add_test(tc_xml_encode, UA_String_special_xml_encode);

    tcase_add_test(tc_xml_encode, UA_DateTime_xml_encode);
    tcase_add_test(tc_xml_encode, UA_DateTime_xml_encode_null);
    tcase_add_test(tc_xml_encode, UA_DateTime_with_nanoseconds_xml_encode);

    tcase_add_test(tc_xml_encode, UA_Guid_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Guid_smallbuf_xml_encode);

    tcase_add_test(tc_xml_encode, UA_NodeId_Numeric_xml_encode);
    tcase_add_test(tc_xml_encode, UA_NodeId_Numeric_Namespace_xml_encode);
    tcase_add_test(tc_xml_encode, UA_NodeId_String_xml_encode);
    tcase_add_test(tc_xml_encode, UA_NodeId_String_Namespace_xml_encode);
    tcase_add_test(tc_xml_encode, UA_NodeId_Guid_xml_encode);
    tcase_add_test(tc_xml_encode, UA_NodeId_Guid_Namespace_xml_encode);
    tcase_add_test(tc_xml_encode, UA_NodeId_ByteString_xml_encode);
    tcase_add_test(tc_xml_encode, UA_NodeId_ByteString_Namespace_xml_encode);

    tcase_add_test(tc_xml_encode, UA_ExpandedNodeId_xml_encode);
    tcase_add_test(tc_xml_encode, UA_ExpandedNodeId_MissingNamespaceUri_xml_encode);

    suite_add_tcase(s, tc_xml_encode);

    TCase *tc_xml_decode = tcase_create("xml_decode");

    tcase_add_test(tc_xml_decode, UA_Boolean_true_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Boolean_false_xml_decode);

    tcase_add_test(tc_xml_decode, UA_SByte_Min_xml_decode);
    tcase_add_test(tc_xml_decode, UA_SByte_Max_xml_decode);

    tcase_add_test(tc_xml_decode, UA_Byte_Min_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Byte_Max_xml_decode);

    tcase_add_test(tc_xml_decode, UA_Int16_Min_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Int16_Max_xml_decode);

    tcase_add_test(tc_xml_decode, UA_UInt16_Min_xml_decode);
    tcase_add_test(tc_xml_decode, UA_UInt16_Max_xml_decode);

    tcase_add_test(tc_xml_decode, UA_UInt32_Min_xml_decode);
    tcase_add_test(tc_xml_decode, UA_UInt32_Max_xml_decode);

    tcase_add_test(tc_xml_decode, UA_Int32_Min_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Int32_Max_xml_decode);

    tcase_add_test(tc_xml_decode, UA_Int64_Min_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Int64_Max_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Int64_Overflow_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Int64_TooBig_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Int64_NoDigit_xml_decode);

    tcase_add_test(tc_xml_decode, UA_UInt64_Min_xml_decode);
    tcase_add_test(tc_xml_decode, UA_UInt64_Max_xml_decode);
    tcase_add_test(tc_xml_decode, UA_UInt64_Overflow_xml_decode);
    tcase_add_test(tc_xml_decode, UA_UInt64_TooBig_xml_decode);
    tcase_add_test(tc_xml_decode, UA_UInt64_NoDigit_xml_decode);

    tcase_add_test(tc_xml_decode, UA_Float_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Float_xml_one_decode);
    tcase_add_test(tc_xml_decode, UA_Float_xml_inf_decode);
    tcase_add_test(tc_xml_decode, UA_Float_xml_neginf_decode);
    tcase_add_test(tc_xml_decode, UA_Float_xml_nan_decode);
    tcase_add_test(tc_xml_decode, UA_Float_xml_negnan_decode);

    tcase_add_test(tc_xml_decode, UA_Double_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_one_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_corrupt_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_onepointsmallest_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_nan_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_negnan_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_negzero_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_zero_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_inf_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Double_neginf_xml_decode);

    tcase_add_test(tc_xml_decode, UA_String_xml_decode);
    tcase_add_test(tc_xml_decode, UA_String_empty_xml_decode);
    tcase_add_test(tc_xml_decode, UA_String_unescapeBS_xml_decode);
    tcase_add_test(tc_xml_decode, UA_String_escape2_xml_decode);

    tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode);
    tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode_large);
    tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode_negative);
    tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode_min);
    tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode_max);
    tcase_add_test(tc_xml_decode, UA_DateTime_micro_xml_decode);

    tcase_add_test(tc_xml_decode, UA_Guid_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Guid_lower_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Guid_tooShort_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Guid_tooLong_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Guid_wrong_xml_decode);

    tcase_add_test(tc_xml_decode, UA_NodeId_Nummeric_xml_decode);
    tcase_add_test(tc_xml_decode, UA_NodeId_Nummeric_Namespace_xml_decode);
    tcase_add_test(tc_xml_decode, UA_NodeId_String_xml_decode);
    tcase_add_test(tc_xml_decode, UA_NodeId_Guid_xml_decode);
    tcase_add_test(tc_xml_decode, UA_NodeId_ByteString_xml_decode);

    tcase_add_test(tc_xml_decode, UA_ExpandedNodeId_Nummeric_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExpandedNodeId_String_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExpandedNodeId_String_Namespace_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExpandedNodeId_String_NamespaceAsIndex_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExpandedNodeId_String_Namespace_ServerUri_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExpandedNodeId_ByteString_xml_decode);

    suite_add_tcase(s, tc_xml_decode);

    return s;
}

int main(void) {
    int      number_failed = 0;
    Suite   *s;
    SRunner *sr;

    s  = testSuite_builtin_xml();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
