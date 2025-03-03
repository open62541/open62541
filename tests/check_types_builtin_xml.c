/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/util.h>

#include "ua_types_encoding_xml.h"

#include <check.h>
#include <math.h>
#include <stdlib.h>

#if defined(_MSC_VER)
# pragma warning(disable: 4146)
#endif

/* Boolean */
START_TEST(UA_Boolean_true_xml_encode) {
    UA_Boolean *src = UA_Boolean_new();
    UA_Boolean_init(src);
    *src = true;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BOOLEAN];
    const size_t booleanXmlSectLen = 19;
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, booleanXmlSectLen + 4);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Boolean>true</Boolean>";
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
    const size_t booleanXmlSectLen = 19;
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, booleanXmlSectLen + 5);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Boolean>false</Boolean>";
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

    char *result = "<SByte>127</SByte>";
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

    char *result = "<SByte>-128</SByte>";
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

    char *result = "<SByte>0</SByte>";
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

    char *result = "<Byte>255</Byte>";
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

    char *result = "<Byte>0</Byte>";
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

    char *result = "<Int16>32767</Int16>";
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

    char *result = "<Int16>-32768</Int16>";
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

    char *result = "<Int16>0</Int16>";
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

    char *result = "<UInt16>65535</UInt16>";
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

    char *result = "<UInt16>0</UInt16>";
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

    char *result = "<Int32>2147483647</Int32>";
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

    char *result = "<Int32>-2147483648</Int32>";
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

    char *result = "<Int32>0</Int32>";
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

    char *result = "<UInt32>4294967295</UInt32>";
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

    char *result = "<UInt32>0</UInt32>";
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

    char *result = "<Int64>9223372036854775807</Int64>";
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

    char *result = "<Int64>-9223372036854775808</Int64>";
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

    char *result = "<Int64>0</Int64>";
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

    char *result = "<UInt64>18446744073709551615</UInt64>";
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

    char *result = "<UInt64>0</UInt64>";
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

    char* result = "<Float>1.0</Float>";
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

    char* result = "<Double>1.1234</Double>";
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

    char* result = "<Double>0.0</Double>";
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

    char* result = "<Double>0.0</Double>";
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

    char *result = "<Double>INF</Double>";
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

    char *result = "<Double>-INF</Double>";
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

    char *result = "<Double>NaN</Double>";
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

    char* result = "<Double>1.0000000000000002</Double>";
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

    char *result = "<String>hello</String>";
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

    char *result = "<String></String>";
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

    char *result = "<String>null</String>";
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

    char *result = "<String>\b\th\"e\fl\nl\\o\r</String>";
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

    char *result = "<String>he\\zsdl\alo€ \x26\x3A asdasd</String>";
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

    char *result = "<String>𝄞𠂊𝕥🔍</String>";
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

    char *result = "<DateTime>1970-01-15T06:56:07Z</DateTime>";
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

    char *result = "<DateTime>1601-01-01T00:00:00Z</DateTime>";
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

    char *result = "<DateTime>1970-01-15T06:56:07.8901234Z</DateTime>";
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
    const size_t guidXmlSectLen = 30;
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);
    ck_assert_uint_eq(size, guidXmlSectLen + 36);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Guid><String>00000003-0009-000A-0807-060504030201</String></Guid>";
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

/* ByteString */
START_TEST(UA_ByteString_xml_encode) {
    UA_ByteString *src = UA_ByteString_new();
    UA_ByteString_init(src);
    *src = UA_BYTESTRING_ALLOC("asdfasdf");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTESTRING];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<ByteString>YXNkZmFzZGY=</ByteString>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ByteString_delete(src);
}
END_TEST

START_TEST(UA_ByteString2_xml_encode) {
    UA_ByteString *src = UA_ByteString_new();
    UA_ByteString_init(src);
    *src = UA_BYTESTRING_ALLOC("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTESTRING];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<ByteString>TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdC4=</ByteString>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ByteString_delete(src);
}
END_TEST

START_TEST(UA_ByteString3_xml_encode) {
    UA_ByteString *src = UA_ByteString_new();
    UA_ByteString_init(src);
    *src = UA_BYTESTRING_ALLOC("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
                               "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud "
                               "exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure "
                               "dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
                               "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
                               "mollit anim id est laborum.");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTESTRING];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status retval = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    buf.data[size] = 0; /* zero terminate */

    UA_ByteString out;
    UA_ByteString_init(&out);
    retval |= UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BYTESTRING], NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(UA_ByteString_equal(src, &out));

    UA_ByteString_clear(&buf);
    UA_ByteString_clear(&out);
    UA_ByteString_delete(src);
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

    char *result = "<NodeId><Identifier>i=5555</Identifier></NodeId>";
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

    char *result = "<NodeId><Identifier>ns=4;i=5555</Identifier></NodeId>";
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

    char *result = "<NodeId><Identifier>s=foobar</Identifier></NodeId>";
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

    char *result = "<NodeId><Identifier>ns=5;s=foobar</Identifier></NodeId>";
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

    char *result = "<NodeId><Identifier>g=00000003-0009-000a-0807-060504030201</Identifier></NodeId>";
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
    const size_t nodeIdXmlSectLen = 42;
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, nodeIdXmlSectLen + 43);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<NodeId><Identifier>ns=5;g=00000003-0009-000a-0807-060504030201</Identifier></NodeId>";
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
    const size_t nodeIdXmlSectLen = 42;
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, nodeIdXmlSectLen + 14);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<NodeId><Identifier>b=YXNkZmFzZGY=</Identifier></NodeId>";
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

    char *result = "<NodeId><Identifier>ns=5;b=YXNkZmFzZGY=</Identifier></NodeId>";
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

    char *result = "<ExpandedNodeId><Identifier>svr=1345;nsu=asdf;s=testtestTest</Identifier></ExpandedNodeId>";
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

    char *result = "<ExpandedNodeId><Identifier>svr=1345;ns=23;s=testtestTest</Identifier></ExpandedNodeId>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST

/* StatusCode */
START_TEST(UA_StatusCode_xml_encode) {
    UA_StatusCode *src = UA_StatusCode_new();
    *src = UA_STATUSCODE_BADAGGREGATECONFIGURATIONREJECTED;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STATUSCODE];
    const size_t statusCodeXmlSectLen = 38;
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, statusCodeXmlSectLen + 10);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<StatusCode><Code>2161770496</Code></StatusCode>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_StatusCode_delete(src);
}
END_TEST

START_TEST(UA_StatusCode_good_xml_encode) {
    UA_StatusCode *src = UA_StatusCode_new();
    *src = UA_STATUSCODE_GOOD;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STATUSCODE];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<StatusCode><Code>0</Code></StatusCode>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_StatusCode_delete(src);
}
END_TEST

START_TEST(UA_StatusCode_smallbuf_xml_encode) {
    UA_StatusCode *src = UA_StatusCode_new();
    *src = UA_STATUSCODE_BADAGGREGATECONFIGURATIONREJECTED;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STATUSCODE];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);

    UA_ByteString_clear(&buf);
    UA_StatusCode_delete(src);
}
END_TEST

/* QualifiedName */
START_TEST(UA_QualifiedName_xml_encode_1) {
    UA_QualifiedName *src = UA_QualifiedName_new();
    UA_QualifiedName_init(src);
    src->name = UA_STRING_ALLOC("derName");
    src->namespaceIndex = 1;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<QualifiedName><NamespaceIndex>1</NamespaceIndex><Name>derName</Name></QualifiedName>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_QualifiedName_delete(src);
}
END_TEST

START_TEST(UA_QualifiedName_xml_encode_2) {
    UA_QualifiedName *src = UA_QualifiedName_new();
    UA_QualifiedName_init(src);
    src->name = UA_STRING_ALLOC("derName");
    src->namespaceIndex = 6789;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<QualifiedName><NamespaceIndex>6789</NamespaceIndex><Name>derName</Name></QualifiedName>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_QualifiedName_delete(src);
}
END_TEST

/* LocalizedText */
START_TEST(UA_LocalizedText_xml_encode) {
    UA_LocalizedText src;
    UA_LocalizedText_init(&src);
    src.locale = UA_STRING_ALLOC("en");
    src.text = UA_STRING_ALLOC("enabled");;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<LocalizedText><Locale>en</Locale><Text>enabled</Text></LocalizedText>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_LocalizedText_clear(&src);
}
END_TEST

START_TEST(UA_LocalizedText_empty_text_xml_encode) {
    UA_LocalizedText src;
    UA_LocalizedText_init(&src);
    src.locale = UA_STRING_ALLOC("en");
    src.text = UA_STRING_ALLOC("");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<LocalizedText><Locale>en</Locale><Text></Text></LocalizedText>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_LocalizedText_clear(&src);
}
END_TEST

START_TEST(UA_LocalizedText_null_locale_xml_encode) {
    UA_LocalizedText src;
    UA_LocalizedText_init(&src);
    src.locale = UA_STRING_ALLOC("en");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<LocalizedText><Locale>en</Locale><Text>null</Text></LocalizedText>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_LocalizedText_clear(&src);
}
END_TEST

START_TEST(UA_LocalizedText_null_xml_encode) {
    UA_LocalizedText src;
    UA_LocalizedText_init(&src);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<LocalizedText><Locale>null</Locale><Text>null</Text></LocalizedText>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_LocalizedText_clear(&src);
}
END_TEST

/* Print */
START_TEST(UA_UInt32_print_xml_encode) {
    UA_UInt32 *src = UA_UInt32_new();
    *src = 12345678;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT32];

    UA_EncodeXmlOptions options;
    memset(&options, 0, sizeof(UA_EncodeXmlOptions));
    size_t size = UA_calcSizeXml((void*)src, type, &options);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<UInt32>12345678</UInt32>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_UInt32_delete(src);
}
END_TEST

START_TEST(UA_ExpandedNodeId_print_xml_encode) {
    UA_ExpandedNodeId *src = UA_ExpandedNodeId_new();
    UA_ExpandedNodeId_init(src);
    *src = UA_EXPANDEDNODEID_STRING_ALLOC(23, "testtestTest");
    src->namespaceUri = UA_STRING_ALLOC("asdf");
    src->serverIndex = 1345;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXPANDEDNODEID];

    UA_EncodeXmlOptions options;
    memset(&options, 0, sizeof(UA_EncodeXmlOptions));
    size_t size = UA_calcSizeXml((void*)src, type, &options);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<ExpandedNodeId><Identifier>svr=1345;nsu=asdf;s=testtestTest</Identifier></ExpandedNodeId>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST

START_TEST(UA_QualifiedName_print_xml_encode) {
    UA_QualifiedName *src = UA_QualifiedName_new();
    UA_QualifiedName_init(src);
    src->name = UA_STRING_ALLOC("derName");
    src->namespaceIndex = 6789;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];

    UA_EncodeXmlOptions options;
    memset(&options, 0, sizeof(UA_EncodeXmlOptions));
    size_t size = UA_calcSizeXml((void*)src, type, &options);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<QualifiedName><NamespaceIndex>6789</NamespaceIndex><Name>derName</Name></QualifiedName>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_QualifiedName_delete(src);
}
END_TEST

/* ExtensionObject */
START_TEST(UA_ExtensionObject_xml_xml_encode) {
    UA_ExtensionObject *src = UA_ExtensionObject_new();
    UA_ExtensionObject_init(src);
    src->encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    src->content.encoded.typeId = UA_NODEID_NUMERIC(2, 1234);

    UA_ByteString b = UA_BYTESTRING_ALLOC("<Elemement></Element>");
    src->content.encoded.body = b;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeXml((void *) src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<ExtensionObject><TypeId><Identifier>ns=2;i=1234</Identifier></TypeId><Body><Elemement></Element></Body></ExtensionObject>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExtensionObject_delete(src);
}
END_TEST


START_TEST(UA_ExtensionObject_byteString_xml_encode) {
    UA_ExtensionObject *src = UA_ExtensionObject_new();
    UA_ExtensionObject_init(src);
    src->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    src->content.encoded.typeId = UA_NODEID_NUMERIC(2, 1234);

    UA_ByteString b = UA_BYTESTRING_ALLOC("123456789012345678901234567890");
    src->content.encoded.body = b;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<ExtensionObject>"
                      "<TypeId>"
                        "<Identifier>ns=2;i=1234</Identifier>"
                      "</TypeId>"
                      "<Body>"
                        "<ByteString>MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkw</ByteString>"
                      "</Body>"
                    "</ExtensionObject>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExtensionObject_delete(src);
}
END_TEST

START_TEST(UA_ExtensionObject_xml_body_encode) {
    UA_ExtensionObject src;
    UA_NodeId content = UA_NODEID_NUMERIC(0, 5555);
    UA_ExtensionObject_setValue(&src, &content, &UA_TYPES[UA_TYPES_NODEID]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeXml((void*)&src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)&src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    buf.data[size] = 0; /* zero terminate */

    char *result = "<ExtensionObject>"
                      "<TypeId>"
                        "<Identifier>i=17</Identifier>"
                      "</TypeId>"
                      "<Body>"
                        "<NodeId><Identifier>i=5555</Identifier></NodeId>"
                      "</Body>"
                    "</ExtensionObject>";
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
}
END_TEST

START_TEST(UA_ExtensionObject_null_xml_encode) {
    UA_ExtensionObject *src = UA_ExtensionObject_new();
    UA_ExtensionObject_init(src);
    src->encoding = UA_EXTENSIONOBJECT_ENCODED_NOBODY;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<ExtensionObject></ExtensionObject>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExtensionObject_delete(src);
}
END_TEST

START_TEST(UA_ExtensionObject_print_xml_encode) {
    UA_ExtensionObject *src = UA_ExtensionObject_new();
    UA_ExtensionObject_init(src);
    src->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    src->content.encoded.typeId = UA_NODEID_NUMERIC(2, 1234);

    UA_ByteString b = UA_BYTESTRING_ALLOC("123456789012345678901234567890");
    src->content.encoded.body = b;

    UA_EncodeXmlOptions options;
    memset(&options, 0, sizeof(UA_EncodeXmlOptions));
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeXml((void*)src, type, &options);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<ExtensionObject>"
                      "<TypeId>"
                        "<Identifier>ns=2;i=1234</Identifier>"
                      "</TypeId>"
                      "<Body>"
                        "<ByteString>MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkw</ByteString>"
                      "</Body>"
                    "</ExtensionObject>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExtensionObject_delete(src);
}
END_TEST

/* Array */
START_TEST(UA_Array_Variant_Byte_xml_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_Byte zero[2] = {42, 43};
    UA_Variant_setArrayCopy(src, zero, 2, &UA_TYPES[UA_TYPES_BYTE]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    const size_t variantXmlSectLen = 34;
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, variantXmlSectLen + 55);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Variant>"
                      "<Value>"
                        "<ListOfByte>"
                          "<Byte>42</Byte>"
                          "<Byte>43</Byte>"
                        "</ListOfByte>"
                      "</Value>"
                    "</Variant>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Array_Variant_UInt16_xml_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_UInt16 zero[2] = {42, 43};
    UA_Variant_setArrayCopy(src, zero, 2, &UA_TYPES[UA_TYPES_UINT16]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    const size_t variantXmlSectLen = 34;
    size_t size = UA_calcSizeXml((void*)src, type, NULL);
    ck_assert_uint_eq(size, variantXmlSectLen + 67);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Variant>"
                      "<Value>"
                        "<ListOfUInt16>"
                          "<UInt16>42</UInt16>"
                          "<UInt16>43</UInt16>"
                        "</ListOfUInt16>"
                      "</Value>"
                    "</Variant>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Array_Variant_Null_xml_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_Variant_setArray(src, NULL, 0, &UA_TYPES[UA_TYPES_UINT16]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Variant>"
                      "<Value>"
                        "<ListOfUInt16>"
                        "</ListOfUInt16>"
                      "</Value>"
                    "</Variant>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Array_Variant_String_xml_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_String zero[2] = {UA_STRING("eins"),UA_STRING("zwei")};
    UA_Variant_setArrayCopy(src, zero, 2, &UA_TYPES[UA_TYPES_STRING]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Variant>"
                      "<Value>"
                        "<ListOfString>"
                          "<String>eins</String>"
                          "<String>zwei</String>"
                        "</ListOfString>"
                      "</Value>"
                    "</Variant>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_clear(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Array_Variant_ExtensionObject_ByteString_xml_encode) {
    UA_ExtensionObject eo1;
    UA_ExtensionObject_init(&eo1);
    eo1.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo1.content.encoded.typeId = UA_NODEID_NUMERIC(2, 1234);
    eo1.content.encoded.body = UA_BYTESTRING_ALLOC("123456789012345678901234567890");

    UA_ExtensionObject eo2;
    UA_ExtensionObject_init(&eo2);
    eo2.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo2.content.encoded.typeId = UA_NODEID_NUMERIC(3, 5678);
    eo2.content.encoded.body = UA_BYTESTRING_ALLOC("98765432109876543210987654321");

    UA_ExtensionObject eo3;
    UA_ExtensionObject_init(&eo3);
    eo3.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo3.content.encoded.typeId = UA_NODEID_NUMERIC(4, 9999);
    eo3.content.encoded.body = UA_BYTESTRING_ALLOC("1357911131517192123252729");

    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_ExtensionObject zero[3] = {eo1, eo2, eo3};
    UA_Variant_setArrayCopy(src, zero, 3, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Variant>"
                      "<Value>"
                        "<ListOfExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>ns=2;i=1234</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<ByteString>MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkw</ByteString>"
                            "</Body>"
                          "</ExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>ns=3;i=5678</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<ByteString>OTg3NjU0MzIxMDk4NzY1NDMyMTA5ODc2NTQzMjE=</ByteString>"
                            "</Body>"
                          "</ExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>ns=4;i=9999</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<ByteString>MTM1NzkxMTEzMTUxNzE5MjEyMzI1MjcyOQ==</ByteString>"
                            "</Body>"
                          "</ExtensionObject>"
                        "</ListOfExtensionObject>"
                      "</Value>"
                    "</Variant>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExtensionObject_clear(&eo1);
    UA_ExtensionObject_clear(&eo2);
    UA_ExtensionObject_clear(&eo3);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Array_Variant_ExtensionObject_Xml_xml_encode) {
    UA_ExtensionObject eo1;
    UA_ExtensionObject_init(&eo1);
    eo1.encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    eo1.content.encoded.typeId = UA_NODEID_NUMERIC(0, 297);
    eo1.content.encoded.body = UA_BYTESTRING_ALLOC("<Argument>"
                                                     "<Name>ObjectToMoveOrCopy</Name>"
                                                     "<DataType>"
                                                       "<Identifier>i=17</Identifier>"
                                                     "</DataType>"
                                                     "<ValueRank>-1</ValueRank>"
                                                     "<ArrayDimensions />"
                                                   "</Argument>");

    UA_ExtensionObject eo2;
    UA_ExtensionObject_init(&eo2);
    eo2.encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    eo2.content.encoded.typeId = UA_NODEID_NUMERIC(0, 297);
    eo2.content.encoded.body = UA_BYTESTRING_ALLOC("<Argument>"
                                                     "<Name>TargetDirectory</Name>"
                                                     "<DataType>"
                                                       "<Identifier>i=17</Identifier>"
                                                     "</DataType>"
                                                     "<ValueRank>-1</ValueRank>"
                                                     "<ArrayDimensions />"
                                                   "</Argument>");

    UA_ExtensionObject eo3;
    UA_ExtensionObject_init(&eo3);
    eo3.encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    eo3.content.encoded.typeId = UA_NODEID_NUMERIC(0, 297);
    eo3.content.encoded.body = UA_BYTESTRING_ALLOC("<Argument>"
                                                     "<Name>CreateCopy</Name>"
                                                     "<DataType>"
                                                       "<Identifier>i=1</Identifier>"
                                                     "</DataType>"
                                                     "<ValueRank>-1</ValueRank>"
                                                     "<ArrayDimensions />"
                                                   "</Argument>");

    UA_ExtensionObject eo4;
    UA_ExtensionObject_init(&eo4);
    eo4.encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    eo4.content.encoded.typeId = UA_NODEID_NUMERIC(0, 297);
    eo4.content.encoded.body = UA_BYTESTRING_ALLOC("<Argument>"
                                                     "<Name>NewName</Name>"
                                                     "<DataType>"
                                                       "<Identifier>i=12</Identifier>"
                                                     "</DataType>"
                                                     "<ValueRank>-1</ValueRank>"
                                                     "<ArrayDimensions />"
                                                   "</Argument>");

    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_ExtensionObject zero[4] = {eo1, eo2, eo3, eo4};
    UA_Variant_setArrayCopy(src, zero, 4, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeXml((void*)src, type, NULL);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Variant>"
                      "<Value>"
                        "<ListOfExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>i=297</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<Argument>"
                                "<Name>ObjectToMoveOrCopy</Name>"
                                "<DataType>"
                                  "<Identifier>i=17</Identifier>"
                                "</DataType>"
                                "<ValueRank>-1</ValueRank>"
                                "<ArrayDimensions />"
                              "</Argument>"
                            "</Body>"
                          "</ExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>i=297</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<Argument>"
                                "<Name>TargetDirectory</Name>"
                                "<DataType>"
                                  "<Identifier>i=17</Identifier>"
                                "</DataType>"
                                "<ValueRank>-1</ValueRank>"
                                "<ArrayDimensions />"
                              "</Argument>"
                            "</Body>"
                          "</ExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>i=297</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<Argument>"
                                "<Name>CreateCopy</Name>"
                                "<DataType>"
                                  "<Identifier>i=1</Identifier>"
                                "</DataType>"
                                "<ValueRank>-1</ValueRank>"
                                "<ArrayDimensions />"
                              "</Argument>"
                            "</Body>"
                          "</ExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>i=297</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<Argument>"
                                "<Name>NewName</Name>"
                                "<DataType>"
                                  "<Identifier>i=12</Identifier>"
                                "</DataType>"
                                "<ValueRank>-1</ValueRank>"
                                "<ArrayDimensions />"
                              "</Argument>"
                            "</Body>"
                          "</ExtensionObject>"
                        "</ListOfExtensionObject>"
                      "</Value>"
                    "</Variant>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExtensionObject_clear(&eo1);
    UA_ExtensionObject_clear(&eo2);
    UA_ExtensionObject_clear(&eo3);
    UA_ExtensionObject_clear(&eo4);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Array_Variant_ExtensionObject_print_xml_encode) {
    UA_ExtensionObject eo1;
    UA_ExtensionObject_init(&eo1);
    eo1.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo1.content.encoded.typeId = UA_NODEID_NUMERIC(2, 1234);
    eo1.content.encoded.body = UA_BYTESTRING_ALLOC("123456789012345678901234567890");

    UA_ExtensionObject eo2;
    UA_ExtensionObject_init(&eo2);
    eo2.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    eo2.content.encoded.typeId = UA_NODEID_NUMERIC(3, 5678);
    eo2.content.encoded.body = UA_BYTESTRING_ALLOC("98765432109876543210987654321");

    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_ExtensionObject zero[2] = {eo1, eo2};
    UA_Variant_setArrayCopy(src, zero, 2, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);

    UA_EncodeXmlOptions options;
    memset(&options, 0, sizeof(UA_EncodeXmlOptions));
    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeXml((void*)src, type, &options);

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, size + 1);

    status s = UA_encodeXml((void*)src, type, &buf, NULL);
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

    char *result = "<Variant>"
                      "<Value>"
                        "<ListOfExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>ns=2;i=1234</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<ByteString>MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkw</ByteString>"
                            "</Body>"
                          "</ExtensionObject>"
                          "<ExtensionObject>"
                            "<TypeId>"
                              "<Identifier>ns=3;i=5678</Identifier>"
                            "</TypeId>"
                            "<Body>"
                              "<ByteString>OTg3NjU0MzIxMDk4NzY1NDMyMTA5ODc2NTQzMjE=</ByteString>"
                            "</Body>"
                          "</ExtensionObject>"
                        "</ListOfExtensionObject>"
                      "</Value>"
                    "</Variant>";
    buf.data[size] = 0; /* zero terminate */
    ck_assert_str_eq(result, (char*)buf.data);

    UA_ByteString_clear(&buf);
    UA_ExtensionObject_clear(&eo1);
    UA_ExtensionObject_clear(&eo2);
    UA_Variant_delete(src);
}
END_TEST


// ---------------------------DECODE-------------------------------------


/* Boolean */
START_TEST(UA_Boolean_true_xml_decode) {
    UA_Boolean out;
    UA_Boolean_init(&out);
    UA_ByteString buf = UA_STRING("<Boolean>true</Boolean>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BOOLEAN], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, true);

    UA_Boolean_clear(&out);
}
END_TEST

START_TEST(UA_Boolean_false_xml_decode) {
    UA_Boolean out;
    UA_Boolean_init(&out);
    UA_ByteString buf = UA_STRING("<Boolean>false</Boolean>");

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
    UA_ByteString buf = UA_STRING("<SByte>-128</SByte>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_SBYTE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, -128);

    UA_SByte_clear(&out);
}
END_TEST

START_TEST(UA_SByte_Max_xml_decode) {
    UA_SByte out;
    UA_SByte_init(&out);
    UA_ByteString buf = UA_STRING("<SByte>127</SByte>");

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
    UA_ByteString buf = UA_STRING("<Byte>0</Byte>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BYTE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 0);

    UA_Byte_clear(&out);
}
END_TEST

START_TEST(UA_Byte_Max_xml_decode) {
    UA_Byte out;
    UA_Byte_init(&out);
    UA_ByteString buf = UA_STRING("<Byte>255</Byte>");

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
    UA_ByteString buf = UA_STRING("<Int16>-32768</Int16>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT16], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out, -32768);

    UA_Int16_clear(&out);
}
END_TEST

START_TEST(UA_Int16_Max_xml_decode) {
    UA_Int16 out;
    UA_Int16_init(&out);
    UA_ByteString buf = UA_STRING("<Int16>32767</Int16>");

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
    UA_ByteString buf = UA_STRING("<UInt16>0</UInt16>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT16], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 0);

    UA_UInt16_clear(&out);
}
END_TEST

START_TEST(UA_UInt16_Max_xml_decode) {
    UA_UInt16 out;
    UA_UInt16_init(&out);
    UA_ByteString buf = UA_STRING("<UInt16>65535</UInt16>");

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
    UA_ByteString buf = UA_STRING("<Int32>-2147483648</Int32>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT32], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert(out == -2147483648);

    UA_Int32_clear(&out);
}
END_TEST

START_TEST(UA_Int32_Max_xml_decode) {
    UA_Int32 out;
    UA_Int32_init(&out);
    UA_ByteString buf = UA_STRING("<Int32>2147483647</Int32>");

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
    UA_ByteString buf = UA_STRING("<UInt32>0</UInt32>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT32], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 0);

    UA_UInt32_clear(&out);
}
END_TEST

START_TEST(UA_UInt32_Max_xml_decode) {
    UA_UInt32 out;
    UA_UInt32_init(&out);
    UA_ByteString buf = UA_STRING("<UInt32>4294967295</UInt32>");

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
    UA_ByteString buf = UA_STRING("<Int64>-9223372036854775808</Int64>");

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
    UA_ByteString buf = UA_STRING("<Int64>9223372036854775807</Int64>");

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
    UA_ByteString buf = UA_STRING("<Int64>9223372036854775808</Int64>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

START_TEST(UA_Int64_TooBig_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("<Int64>111111111111111111111111111111</Int64>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

START_TEST(UA_Int64_NoDigit_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("<Int64>a</Int64>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

/* UInt64 */
START_TEST(UA_UInt64_Min_xml_decode) {
    UA_UInt64 out;
    UA_UInt64_init(&out);
    UA_ByteString buf = UA_STRING("<UInt64>0</UInt64>");

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
    UA_ByteString buf = UA_STRING("<UInt64>18446744073709551615</UInt64>");

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
    UA_ByteString buf = UA_STRING("<UInt64>18446744073709551616</UInt64>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_UInt64_clear(&out);
}
END_TEST

START_TEST(UA_UInt64_TooBig_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("<UInt64>111111111111111111111111111111</UInt64>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

START_TEST(UA_UInt64_NoDigit_xml_decode) {
    UA_Int64 out;
    UA_Int64_init(&out);
    UA_ByteString buf = UA_STRING("<UInt64>a</UInt64>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_INT64], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Int64_clear(&out);
}
END_TEST

/* Float */
START_TEST(UA_Float_xml_decode) {
    UA_Float out;
    UA_Float_init(&out);
    UA_ByteString buf = UA_STRING("<Float>3.1415927410</Float>");

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
    UA_ByteString buf = UA_STRING("<Float>1</Float>");

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
    UA_ByteString buf = UA_STRING("<Float>INF</Float>");

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
    UA_ByteString buf = UA_STRING("<Float>-INF</Float>");

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
    UA_ByteString buf = UA_STRING("<Float>NaN</Float>");

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
    UA_ByteString buf = UA_STRING("<Float>-NaN</Float>");

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
    UA_ByteString buf = UA_STRING("<Double>1.1234</Double>");

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
    UA_ByteString buf = UA_STRING("<Double>1.12.34</Double>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DOUBLE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Double_clear(&out);
}
END_TEST

START_TEST(UA_Double_one_xml_decode) {
    UA_Double out;
    UA_Double_init(&out);
    UA_ByteString buf = UA_STRING("<Double>1</Double>");

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
    UA_ByteString buf = UA_STRING("<Double>1.0000000000000002</Double>");

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
    UA_ByteString buf = UA_STRING("<Double>NaN</Double>");

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
    UA_ByteString buf = UA_STRING("<Double>-NaN</Double>");

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
    UA_ByteString buf = UA_STRING("<Double>INF</Double>");

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
    UA_ByteString buf = UA_STRING("<Double>-INF</Double>");

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
    UA_ByteString buf = UA_STRING("<Double>0</Double>");

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
    UA_ByteString buf = UA_STRING("<Double>-0</Double>");

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
    UA_String *out = UA_String_new();
    UA_String_init(out);
    UA_ByteString buf = UA_STRING("<String>abcdef</String>");

    UA_StatusCode retval = UA_decodeXml(&buf, out, &UA_TYPES[UA_TYPES_STRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out->length, 6);
    ck_assert_int_eq(out->data[0], 'a');
    ck_assert_int_eq(out->data[1], 'b');
    ck_assert_int_eq(out->data[2], 'c');
    ck_assert_int_eq(out->data[3], 'd');
    ck_assert_int_eq(out->data[4], 'e');
    ck_assert_int_eq(out->data[5], 'f');

    UA_String_delete(out);
}
END_TEST

START_TEST(UA_String_empty_xml_decode) {
    UA_String *out = UA_String_new();
    UA_String_init(out);
    UA_ByteString buf = UA_STRING("<String />");

    UA_StatusCode retval = UA_decodeXml(&buf, out, &UA_TYPES[UA_TYPES_STRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out->length, 0);
    ck_assert_ptr_eq(out->data, UA_EMPTY_ARRAY_SENTINEL);

    UA_String_delete(out);
}
END_TEST

START_TEST(UA_String_unescapeBS_xml_decode) {
    UA_String *out = UA_String_new();
    UA_String_init(out);
    UA_ByteString buf = UA_STRING("<String>ab\tcdef</String>");

    UA_StatusCode retval = UA_decodeXml(&buf, out, &UA_TYPES[UA_TYPES_STRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out->length, 7);
    ck_assert_int_eq(out->data[0], 'a');
    ck_assert_int_eq(out->data[1], 'b');
    ck_assert_int_eq(out->data[2], '\t');
    ck_assert_int_eq(out->data[3], 'c');
    ck_assert_int_eq(out->data[4], 'd');
    ck_assert_int_eq(out->data[5], 'e');
    ck_assert_int_eq(out->data[6], 'f');

    UA_String_delete(out);
}
END_TEST

/* TODO: Add option for escaping special chars. */
// START_TEST(UA_String_escape2_xml_decode) {
//     UA_String *out = UA_String_new();
//     UA_String_init(out);
//     UA_ByteString buf = UA_STRING("<String>\b\th\"e\fl\nl\\o\r</String>");

//     UA_StatusCode retval = UA_decodeXml(&buf, out, &UA_TYPES[UA_TYPES_STRING], NULL);

//     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
//     ck_assert_uint_eq(out->length, 12);
//     ck_assert_int_eq(out->data[0], '\b');
//     ck_assert_int_eq(out->data[1], '\t');
//     ck_assert_int_eq(out->data[2], 'h');
//     ck_assert_int_eq(out->data[3], '\"');
//     ck_assert_int_eq(out->data[4], 'e');
//     ck_assert_int_eq(out->data[5], '\f');
//     ck_assert_int_eq(out->data[6], 'l');
//     ck_assert_int_eq(out->data[7], '\n');
//     ck_assert_int_eq(out->data[8], 'l');
//     ck_assert_int_eq(out->data[9], '\\');
//     ck_assert_int_eq(out->data[10], 'o');
//     ck_assert_int_eq(out->data[11], '\r');

//     UA_String_delete(out);
// }
// END_TEST

/* DateTime */
START_TEST(UA_DateTime_xml_decode) {
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_ByteString buf = UA_STRING("<DateTime>1970-01-02T01:02:03.005Z</DateTime>");

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
    UA_ByteString buf = UA_STRING("<DateTime>10970-01-02T01:02:03.005Z</DateTime>");

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

// START_TEST(UA_DateTime_xml_decode_negative) {
//     UA_DateTime out;
//     UA_DateTime_init(&out);
//     UA_ByteString buf = UA_STRING("<DateTime>-0050-01-02T01:02:03.005Z<DateTime>");

//     UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);

//     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
//     UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
//     ck_assert_int_eq(dts.year, -50);
//     ck_assert_int_eq(dts.month, 1);
//     ck_assert_int_eq(dts.day, 2);
//     ck_assert_int_eq(dts.hour, 1);
//     ck_assert_int_eq(dts.min, 2);
//     ck_assert_int_eq(dts.sec, 3);
//     ck_assert_int_eq(dts.milliSec, 5);
//     ck_assert_int_eq(dts.microSec, 0);
//     ck_assert_int_eq(dts.nanoSec, 0);

//     UA_DateTime_clear(&out);
// }
// END_TEST

// START_TEST(UA_DateTime_xml_decode_min) {
//     UA_DateTime dt_min = (UA_DateTime)UA_INT64_MIN;
//     const UA_DataType *type = &UA_TYPES[UA_TYPES_DATETIME];

//     UA_Byte data[128];
//     UA_ByteString buf;
//     buf.data = data;
//     buf.length = 128;

//     status s = UA_encodeXml((void*)&dt_min, type, &buf, NULL);
//     ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

//     UA_DateTime out;
//     UA_ByteString outBuf1 = UA_STRING("<DateTime>");
//     UA_ByteString outBuf2 = UA_STRING("</DateTime>");
//     UA_Byte outData[128] = {0};
//     UA_ByteString outBuf;
//     outBuf.data = outData;
//     size_t currentPos = 0;
//     memcpy(outBuf.data + currentPos, outBuf1.data, outBuf1.length);
//     currentPos += outBuf1.length;
//     memcpy(outBuf.data + currentPos, buf.data, buf.length);
//     currentPos += buf.length;
//     memcpy(outBuf.data + currentPos, outBuf2.data, outBuf2.length);
//     outBuf.length = outBuf1.length + buf.length + outBuf2.length;

//     s = UA_decodeXml(&outBuf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);
//     ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

//     ck_assert_int_eq(dt_min, out);
//     UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
//     ck_assert_int_eq(dts.year, -27627);
//     ck_assert_int_eq(dts.month, 4);
//     ck_assert_int_eq(dts.day, 19);
//     ck_assert_int_eq(dts.hour, 21);
//     ck_assert_int_eq(dts.min, 11);
//     ck_assert_int_eq(dts.sec, 54);
//     ck_assert_int_eq(dts.milliSec, 522);
//     ck_assert_int_eq(dts.microSec, 419);
//     ck_assert_int_eq(dts.nanoSec, 200);
// }
// END_TEST

// START_TEST(UA_DateTime_xml_decode_max) {
//     UA_DateTime dt_max = (UA_DateTime)UA_INT64_MAX;
//     const UA_DataType *type = &UA_TYPES[UA_TYPES_DATETIME];

//     UA_Byte data[128];
//     UA_ByteString buf;
//     buf.data = data;
//     buf.length = 128;

//     status s = UA_encodeXml((void*)&dt_max, type, &buf, NULL);
//     ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

//     UA_DateTime out;
//     UA_ByteString outBuf1 = UA_STRING("<DateTime>");
//     UA_ByteString outBuf2 = UA_STRING("</DateTime>");
//     UA_Byte outData[128] = {0};
//     UA_ByteString outBuf;
//     outBuf.data = outData;
//     size_t currentPos = 0;
//     memcpy(outBuf.data + currentPos, outBuf1.data, outBuf1.length);
//     currentPos += outBuf1.length;
//     memcpy(outBuf.data + currentPos, buf.data, buf.length);
//     currentPos += buf.length;
//     memcpy(outBuf.data + currentPos, outBuf2.data, outBuf2.length);
//     outBuf.length = outBuf1.length + buf.length + outBuf2.length;

//     s = UA_decodeXml(&outBuf, &out, &UA_TYPES[UA_TYPES_DATETIME], NULL);
//     ck_assert_int_eq(s, UA_STATUSCODE_GOOD);

//     ck_assert_int_eq(dt_max, out);
//     UA_DateTimeStruct dts = UA_DateTime_toStruct(out);
//     ck_assert_int_eq(dts.year, 30828);
//     ck_assert_int_eq(dts.month, 9);
//     ck_assert_int_eq(dts.day, 14);
//     ck_assert_int_eq(dts.hour, 2);
//     ck_assert_int_eq(dts.min, 48);
//     ck_assert_int_eq(dts.sec, 5);
//     ck_assert_int_eq(dts.milliSec, 477);
//     ck_assert_int_eq(dts.microSec, 580);
//     ck_assert_int_eq(dts.nanoSec, 700);
// }
// END_TEST

START_TEST(UA_DateTime_micro_xml_decode) {
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_ByteString buf = UA_STRING("<DateTime>1970-01-02T01:02:03.042Z</DateTime>");

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
    UA_ByteString buf = UA_STRING("<Guid>"
                                    "<String>00000001-0002-0003-0405-060708090A0B</String>"
                                  "</Guid>");

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
    UA_ByteString buf = UA_STRING("<Guid>"
                                    "<String>00000001-0002-0003-0405-060708090a0b</String>"
                                  "</Guid>");

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
    UA_ByteString buf = UA_STRING("<Guid>"
                                    "<String>00000001-00</String>"
                                  "</Guid>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_GUID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Guid_clear(&out);
}
END_TEST

START_TEST(UA_Guid_tooLong_xml_decode) {
    UA_Guid out;
    UA_Guid_init(&out);
    UA_ByteString buf = UA_STRING("<Guid>"
                                    "<String>00000001-0002-0003-0405-060708090A0B00000001</String>"
                                  "</Guid>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_GUID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Guid_clear(&out);
}
END_TEST

START_TEST(UA_Guid_wrong_xml_decode) {
    UA_Guid out;
    UA_Guid_init(&out);
    UA_ByteString buf = UA_STRING("<Guid>"
                                    "<String>00000=01-0002-0003-0405-060708090A0B</String>"
                                  "</Guid>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_GUID], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Guid_clear(&out);
}
END_TEST

/* ByteString */
START_TEST(UA_ByteString_xml_decode) {
    UA_ByteString out;
    UA_ByteString_init(&out);
    UA_ByteString buf = UA_STRING("<ByteString>YXNkZmFzZGY=</ByteString>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BYTESTRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.length, 8);
    ck_assert_int_eq(out.data[0], 'a');
    ck_assert_int_eq(out.data[1], 's');
    ck_assert_int_eq(out.data[2], 'd');
    ck_assert_int_eq(out.data[3], 'f');
    ck_assert_int_eq(out.data[4], 'a');
    ck_assert_int_eq(out.data[5], 's');
    ck_assert_int_eq(out.data[6], 'd');
    ck_assert_int_eq(out.data[7], 'f');

    UA_ByteString_clear(&out);
}
END_TEST

/* TODO: Add option for escaping special chars. */
// START_TEST(UA_ByteString_bad_xml_decode) {
//     UA_ByteString out;
//     UA_ByteString_init(&out);
//     UA_ByteString buf = UA_STRING("<ByteString>\x90!\xc5 c{</ByteString>");

//     UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BYTESTRING], NULL);

//     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

//     UA_ByteString_clear(&out);
// }
// END_TEST

START_TEST(UA_ByteString_null_xml_decode) {
    UA_ByteString out;
    UA_ByteString_init(&out);
    UA_ByteString buf = UA_STRING("<ByteString>null</ByteString>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BYTESTRING], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_ByteString_clear(&out);
}
END_TEST

/* NodeId */
START_TEST(UA_NodeId_Nummeric_xml_decode) {
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("<NodeId>"
                                    "<Identifier>i=42</Identifier>"
                                  "</NodeId>");

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
    UA_ByteString buf = UA_STRING("<NodeId>"
                                    "<Identifier>ns=123;i=42</Identifier>"
                                  "</NodeId>");

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
    UA_ByteString buf = UA_STRING("<NodeId>"
                                    "<Identifier>s=test123</Identifier>"
                                  "</NodeId>");

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
    UA_ByteString buf = UA_STRING("<NodeId>"
                                    "<Identifier>g=00000001-0002-0003-0405-060708090A0B</Identifier>"
                                  "</NodeId>");

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
    UA_ByteString buf = UA_STRING("<NodeId>"
                                    "<Identifier>b=YXNkZmFzZGY=</Identifier>"
                                  "</NodeId>");

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
    UA_ByteString buf = UA_STRING("<ExpandedNodeId>"
                                    "<Identifier>i=42</Identifier>"
                                  "</ExpandedNodeId>");

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
    UA_ByteString buf = UA_STRING("<ExpandedNodeId>"
                                    "<Identifier>s=test</Identifier>"
                                  "</ExpandedNodeId>");

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
    UA_ByteString buf = UA_STRING("<ExpandedNodeId>"
                                    "<Identifier>nsu=abcdef;s=test</Identifier>"
                                  "</ExpandedNodeId>");

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
    UA_ByteString buf = UA_STRING("<ExpandedNodeId>"
                                    "<Identifier>ns=42;s=test</Identifier>"
                                  "</ExpandedNodeId>");

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
    UA_ByteString buf = UA_STRING("<ExpandedNodeId>"
                                    "<Identifier>svr=13;nsu=test;s=test</Identifier>"
                                  "</ExpandedNodeId>");

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
    UA_ByteString buf = UA_STRING("<ExpandedNodeId>"
                                    "<Identifier>svr=13;nsu=test;b=YXNkZmFzZGY=</Identifier>"
                                  "</ExpandedNodeId>");

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

/* StatusCode */
START_TEST(UA_StatusCode_0_xml_decode) {
    UA_StatusCode out;
    UA_StatusCode_init(&out);
    UA_ByteString buf = UA_STRING("<StatusCode>"
                                    "<Code>0</Code>"
                                  "</StatusCode>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_STATUSCODE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 0);

    UA_StatusCode_clear(&out);
}
END_TEST

START_TEST(UA_StatusCode_2_xml_decode) {
    UA_StatusCode out;
    UA_StatusCode_init(&out);
    UA_ByteString buf = UA_STRING("<StatusCode>"
                                    "<Code>2</Code>"
                                  "</StatusCode>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_STATUSCODE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out, 2);

    UA_StatusCode_clear(&out);
}
END_TEST

START_TEST(UA_StatusCode_3_xml_decode) {
    UA_StatusCode out;
    UA_StatusCode_init(&out);
    UA_ByteString buf = UA_STRING("<StatusCode>"
                                    "<Code>222222222222222222222222222222222222</Code>"
                                  "</StatusCode>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_STATUSCODE], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_StatusCode_clear(&out);
}
END_TEST

/* QualifiedName */
START_TEST(UA_QualifiedName_1_xml_decode) {
    UA_QualifiedName out;
    UA_QualifiedName_init(&out);
    UA_ByteString buf = UA_STRING("<QualifiedName>"
                                    "<NamespaceIndex>1</NamespaceIndex>"
                                    "<Name>derName</Name>"
                                  "</QualifiedName>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.name.length, 7);
    ck_assert_int_eq(out.name.data[1], 'e');
    ck_assert_int_eq(out.name.data[6], 'e');
    ck_assert_int_eq(out.namespaceIndex, 1);

    UA_QualifiedName_clear(&out);
}
END_TEST

START_TEST(UA_QualifiedName_2_xml_decode) {
    UA_QualifiedName out;
    UA_QualifiedName_init(&out);
    UA_ByteString buf = UA_STRING("<QualifiedName>"
                                    "<NamespaceIndex>6789</NamespaceIndex>"
                                    "<Name>derName</Name>"
                                  "</QualifiedName>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_QUALIFIEDNAME], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.name.length, 7);
    ck_assert_int_eq(out.name.data[1], 'e');
    ck_assert_int_eq(out.name.data[6], 'e');
    ck_assert_int_eq(out.namespaceIndex, 6789);

    UA_QualifiedName_clear(&out);
}
END_TEST

/* LocalizedText */
START_TEST(UA_LocalizedText_xml_decode) {
    UA_LocalizedText out;
    UA_LocalizedText_init(&out);
    UA_ByteString buf = UA_STRING("<LocalizedText>"
                                    "<Locale>t1</Locale>"
                                    "<Text>t2</Text>"
                                  "</LocalizedText>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.locale.data[0], 't');
    ck_assert_int_eq(out.text.data[0], 't');
    ck_assert_int_eq(out.locale.data[1], '1');
    ck_assert_int_eq(out.text.data[1], '2');

    UA_LocalizedText_clear(&out);
}
END_TEST

START_TEST(UA_LocalizedText_missing_text_xml_decode) {
    UA_LocalizedText out;
    UA_LocalizedText_init(&out);
    UA_ByteString buf = UA_STRING("<LocalizedText>"
                                    "<Locale>t1</Locale>"
                                  "</LocalizedText>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(out.locale.length, 2);
    ck_assert_int_eq(out.locale.data[0], 't');
    ck_assert_int_eq(out.locale.data[1], '1');
    ck_assert_ptr_eq(out.text.data, NULL);
    ck_assert_uint_eq(out.text.length, 0);

    UA_LocalizedText_clear(&out);
}
END_TEST

START_TEST(UA_LocalizedText_empty_locale_xml_decode) {
    UA_LocalizedText out;
    UA_LocalizedText_init(&out);
    UA_ByteString buf = UA_STRING("<LocalizedText>"
                                    "<Locale />"
                                    "<Text>t2</Text>"
                                  "</LocalizedText>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(out.locale.data, (UA_Byte*)UA_EMPTY_ARRAY_SENTINEL);
    ck_assert_uint_eq(out.locale.length, 0);
    ck_assert_int_eq(out.text.data[0], 't');
    ck_assert_int_eq(out.text.data[1], '2');
    ck_assert_uint_eq(out.text.length, 2);

    UA_LocalizedText_clear(&out);
}
END_TEST

START_TEST(UA_LocalizedText_empty_xml_decode) {
    UA_LocalizedText out;
    UA_LocalizedText_init(&out);
    UA_ByteString buf = UA_STRING("<LocalizedText />");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(out.locale.data, NULL);
    ck_assert_uint_eq(out.locale.length, 0);
    ck_assert_ptr_eq(out.text.data, NULL);
    ck_assert_uint_eq(out.text.length, 0);

    UA_LocalizedText_clear(&out);
}
END_TEST

/* ExtensionObject */
START_TEST(UA_ExtensionObject_EncodedByteString_xml_decode) {
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("<ExtensionObject>"
                                    "<TypeId>"
                                      "<Identifier>i=42</Identifier>"
                                    "</TypeId>"
                                    "<Body>"
                                      "<ByteString>YXNkZmFzZGY=</ByteString>"
                                    "</Body>"
                                  "</ExtensionObject>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_ENCODED_BYTESTRING);
    ck_assert_int_eq(out.content.encoded.body.data[0], 'a');
    ck_assert_int_eq(out.content.encoded.body.data[1], 's');
    ck_assert_int_eq(out.content.encoded.body.data[2], 'd');
    ck_assert_int_eq(out.content.encoded.body.data[3], 'f');
    ck_assert_int_eq(out.content.encoded.body.data[4], 'a');
    ck_assert_int_eq(out.content.encoded.body.data[5], 's');
    ck_assert_int_eq(out.content.encoded.body.data[6], 'd');
    ck_assert_int_eq(out.content.encoded.body.data[7], 'f');
    ck_assert_int_eq(out.content.encoded.typeId.identifier.numeric, 42);

    UA_ExtensionObject_clear(&out);
}
END_TEST

START_TEST(UA_ExtensionObject_EncodedXml_1_xml_decode) {
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("<ExtensionObject>"
                                    "<TypeId>"
                                      "<Identifier>i=42</Identifier>"
                                    "</TypeId>"
                                    "<Body>"
                                      "<Element></Element>"
                                    "</Body>"
                                  "</ExtensionObject>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_ENCODED_XML);
    UA_ByteString testBufData = UA_STRING("<Element></Element>");
    ck_assert(UA_ByteString_equal(&out.content.encoded.body, &testBufData));
    ck_assert_int_eq(out.content.encoded.typeId.identifier.numeric, 42);

    UA_ExtensionObject_clear(&out);
}
END_TEST

START_TEST(UA_ExtensionObject_EncodedXml_2_xml_decode) {
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("<ExtensionObject>"
                                    "<TypeId>"
                                      "<Identifier>ns=1;i=5006</Identifier>"
                                    "</TypeId>"
                                    "<Body>"
                                      "<PointWithOffset>"
                                        "<offset>"
                                            "<x>1</x>"
                                            "<y>2</y>"
                                        "</offset>"
                                        "<x>20</x>"
                                        "<y>30</y>"
                                      "</PointWithOffset>"
                                    "</Body>"
                                  "</ExtensionObject>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_ENCODED_XML);
    UA_ByteString testBufData = UA_STRING("<PointWithOffset>"
                                            "<offset>"
                                              "<x>1</x>"
                                              "<y>2</y>"
                                            "</offset>"
                                            "<x>20</x>"
                                            "<y>30</y>"
                                          "</PointWithOffset>");
    ck_assert(UA_ByteString_equal(&out.content.encoded.body, &testBufData));
    ck_assert_int_eq(out.content.encoded.typeId.namespaceIndex, 1);
    ck_assert_int_eq(out.content.encoded.typeId.identifier.numeric, 5006);

    UA_ExtensionObject_clear(&out);
}
END_TEST

START_TEST(UA_ExtensionObject_EncodedXml_3_xml_decode) {
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("<ExtensionObject>"
                                    "<TypeId>"
                                      "<Identifier>i=863</Identifier>"
                                    "</TypeId>"
                                    "<Body>"
                                      "<ServerStatusDataType xmlns=\"http://opcfoundation.org/UA/2008/02/Types.xsd\">"
                                        "<StartTime>2000-01-01T00:00:00Z</StartTime>"
                                        "<CurrentTime>2000-01-01T00:00:00Z</CurrentTime>"
                                        "<State>5</State>"
                                        "<BuildInfo>"
                                          "<ProductUri>open62541</ProductUri>"
                                          "<ManufacturerName>oss</ManufacturerName>"
                                          "<ProductName>open62541</ProductName>"
                                          "<SoftwareVersion>master</SoftwareVersion>"
                                          "<BuildNumber>latest</BuildNumber>"
                                          "<BuildDate>2000-01-01T00:00:00Z</BuildDate>"
                                        "</BuildInfo>"
                                        "<SecondsTillShutdown>0</SecondsTillShutdown>"
                                        "<ShutdownReason>"
                                          "<Locale>en</Locale>"
                                          "<Text></Text>"
                                        "</ShutdownReason>"
                                      "</ServerStatusDataType>"
                                    "</Body>"
                                  "</ExtensionObject>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_ENCODED_XML);
    UA_ByteString testBufData = UA_STRING("<ServerStatusDataType xmlns=\"http://opcfoundation.org/UA/2008/02/Types.xsd\">"
                                            "<StartTime>2000-01-01T00:00:00Z</StartTime>"
                                            "<CurrentTime>2000-01-01T00:00:00Z</CurrentTime>"
                                            "<State>5</State>"
                                            "<BuildInfo>"
                                              "<ProductUri>open62541</ProductUri>"
                                              "<ManufacturerName>oss</ManufacturerName>"
                                              "<ProductName>open62541</ProductName>"
                                              "<SoftwareVersion>master</SoftwareVersion>"
                                              "<BuildNumber>latest</BuildNumber>"
                                              "<BuildDate>2000-01-01T00:00:00Z</BuildDate>"
                                            "</BuildInfo>"
                                            "<SecondsTillShutdown>0</SecondsTillShutdown>"
                                            "<ShutdownReason>"
                                              "<Locale>en</Locale>"
                                              "<Text></Text>"
                                            "</ShutdownReason>"
                                          "</ServerStatusDataType>");
    ck_assert(UA_ByteString_equal(&out.content.encoded.body, &testBufData));
    ck_assert_int_eq(out.content.encoded.typeId.identifier.numeric, 863);

    UA_ExtensionObject_clear(&out);
}
END_TEST

START_TEST(UA_ExtensionObject_EncodedXml_4_xml_decode) {
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("<ExtensionObject>"
                                    "<TypeId>"
                                        "<Identifier>i=297</Identifier>"
                                    "</TypeId>"
                                    "<Body>"
                                      "<Argument>"
                                        "<Name>TransferID</Name>"
                                        "<DataType>"
                                          "<Identifier>i=6</Identifier>"
                                        "</DataType>"
                                        "<ValueRank>-1</ValueRank>"
                                        "<ArrayDimensions />"
                                        "<Description>"
                                          "<Locale>en</Locale>"
                                          "<Text>Enabled</Text>"
                                        "</Description>"
                                      "</Argument>"
                                    "</Body>"
                                  "</ExtensionObject>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);

    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_ENCODED_XML);
    UA_ByteString testBufData = UA_STRING("<Argument>"
                                            "<Name>TransferID</Name>"
                                            "<DataType>"
                                              "<Identifier>i=6</Identifier>"
                                            "</DataType>"
                                            "<ValueRank>-1</ValueRank>"
                                            "<ArrayDimensions />"
                                            "<Description>"
                                              "<Locale>en</Locale>"
                                              "<Text>Enabled</Text>"
                                            "</Description>"
                                          "</Argument>");
    ck_assert(UA_ByteString_equal(&out.content.encoded.body, &testBufData));
    ck_assert_int_eq(out.content.encoded.typeId.identifier.numeric, 297);

    UA_ExtensionObject_clear(&out);
}
END_TEST

START_TEST(UA_ExtensionObject_InvalidBody_xml_decode) {
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("<ExtensionObject>"
                                    "<TypeId>"
                                        "<Identifier>i=297</Identifier>"
                                    "</TypeId>"
                                    "<Body>"
                                      "SomeString"
                                    "</Body>"
                                  "</ExtensionObject>");

    UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_ExtensionObject_clear(&out);
}
END_TEST

/* Array */
/* START_TEST(UA_Array_Boolean_xml_decode) { */
/*     UA_Boolean *out; */
/*     UA_ByteString buf = UA_STRING("<ListOfBoolean>" */
/*                                     "<Boolean>true</Boolean>" */
/*                                     "<Boolean>false</Boolean>" */
/*                                   "</ListOfBoolean>"); */

/*     UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_BOOLEAN], NULL); */

/*     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD); */
/*     ck_assert_int_eq(out[0], true); */
/*     ck_assert_int_eq(out[1], false); */

/*     UA_Array_delete(out, 2, &UA_TYPES[UA_TYPES_BOOLEAN]); */
/* } */
/* END_TEST */

/* START_TEST(UA_Array_Number_xml_decode) { */
/*     UA_UInt32 *out; */
/*     UA_ByteString buf = UA_STRING("<ListOfUInt32>" */
/*                                     "<UInt32>0</UInt32>" */
/*                                     "<UInt32>1</UInt32>" */
/*                                     "<UInt32>2</UInt32>" */
/*                                     "<UInt32>3</UInt32>" */
/*                                     "<UInt32>4</UInt32>" */
/*                                     "<UInt32>5</UInt32>" */
/*                                     "<UInt32>6</UInt32>" */
/*                                     "<UInt32>7</UInt32>" */
/*                                     "<UInt32>8</UInt32>" */
/*                                     "<UInt32>9</UInt32>" */
/*                                   "</ListOfUInt32>"); */

/*     UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT32], NULL); */

/*     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD); */
/*     for(size_t i = 0; i < 10; ++i) */
/*         ck_assert_int_eq(out[i], (UA_UInt32)i); */

/*     UA_Array_delete(out, 10, &UA_TYPES[UA_TYPES_UINT32]); */
/* } */
/* END_TEST */

/* START_TEST(UA_Array_Null_xml_decode) { */
/*     UA_UInt32 *out; */
/*     UA_ByteString buf = UA_STRING("<ListOfUInt32>" */
/*                                   "</ListOfUInt32>"); */

/*     UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_UINT32], NULL); */

/*     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD); */
/*     ck_assert_ptr_eq(out, (UA_UInt32*)UA_EMPTY_ARRAY_SENTINEL); */
/* } */
/* END_TEST */

/* START_TEST(UA_Array_String_xml_decode) { */
/*     UA_String *out; */
/*     UA_ByteString buf = UA_STRING("<ListOfString>" */
/*                                     "<String>eins</String>" */
/*                                     "<String>zwei</String>" */
/*                                     "<String>drei</String>" */
/*                                     "<String>vier</String>" */
/*                                   "</ListOfString>"); */

/*     UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_STRING], NULL); */

/*     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD); */
/*     UA_String testData1 = UA_STRING("eins"); */
/*     UA_String testData2 = UA_STRING("zwei"); */
/*     UA_String testData3 = UA_STRING("drei"); */
/*     UA_String testData4 = UA_STRING("vier"); */
/*     ck_assert(UA_String_equal(&out[0], &testData1)); */
/*     ck_assert(UA_String_equal(&out[1], &testData2)); */
/*     ck_assert(UA_String_equal(&out[2], &testData3)); */
/*     ck_assert(UA_String_equal(&out[3], &testData4)); */

/*     UA_Array_delete(out, 4, &UA_TYPES[UA_TYPES_STRING]); */
/* } */
/* END_TEST */

/* START_TEST(UA_Array_ExtensionObject_ByteString_xml_decode) { */
/*     UA_ExtensionObject *out; */
/*     UA_ByteString buf = UA_STRING("<ListOfExtensionObject>" */
/*                                     "<ExtensionObject>" */
/*                                       "<TypeId>" */
/*                                         "<Identifier>ns=2;i=1234</Identifier>" */
/*                                       "</TypeId>" */
/*                                       "<Body>" */
/*                                         "<ByteString>123456789012345678901234567890</ByteString>" */
/*                                       "</Body>" */
/*                                     "</ExtensionObject>" */
/*                                     "<ExtensionObject>" */
/*                                       "<TypeId>" */
/*                                         "<Identifier>ns=3;i=5678</Identifier>" */
/*                                       "</TypeId>" */
/*                                       "<Body>" */
/*                                         "<ByteString>98765432109876543210987654321</ByteString>" */
/*                                       "</Body>" */
/*                                     "</ExtensionObject>" */
/*                                     "<ExtensionObject>" */
/*                                       "<TypeId>" */
/*                                         "<Identifier>ns=4;i=9999</Identifier>" */
/*                                       "</TypeId>" */
/*                                       "<Body>" */
/*                                         "<ByteString>1357911131517192123252729</ByteString>" */
/*                                       "</Body>" */
/*                                     "</ExtensionObject>" */
/*                                   "</ListOfExtensionObject>"); */

/*     UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL); */

/*     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD); */

/*     ck_assert_int_eq(out[0].encoding, UA_EXTENSIONOBJECT_ENCODED_BYTESTRING); */
/*     UA_String testData1 = UA_STRING("123456789012345678901234567890"); */
/*     ck_assert(UA_String_equal(&out[0].content.encoded.body, &testData1)); */
/*     ck_assert_int_eq(out[0].content.encoded.typeId.namespaceIndex, 2); */
/*     ck_assert_int_eq(out[0].content.encoded.typeId.identifier.numeric, 1234); */
/*     ck_assert_int_eq(out[1].encoding, UA_EXTENSIONOBJECT_ENCODED_BYTESTRING); */
/*     UA_String testData2 = UA_STRING("98765432109876543210987654321"); */
/*     ck_assert(UA_String_equal(&out[1].content.encoded.body, &testData2)); */
/*     ck_assert_int_eq(out[1].content.encoded.typeId.namespaceIndex, 3); */
/*     ck_assert_int_eq(out[1].content.encoded.typeId.identifier.numeric, 5678); */
/*     ck_assert_int_eq(out[2].encoding, UA_EXTENSIONOBJECT_ENCODED_BYTESTRING); */
/*     UA_String testData3 = UA_STRING("1357911131517192123252729"); */
/*     ck_assert(UA_String_equal(&out[2].content.encoded.body, &testData3)); */
/*     ck_assert_int_eq(out[2].content.encoded.typeId.namespaceIndex, 4); */
/*     ck_assert_int_eq(out[2].content.encoded.typeId.identifier.numeric, 9999); */

/*     UA_Array_delete(out, 3, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]); */
/* } */
/* END_TEST */

/* START_TEST(UA_Array_ExtensionObject_Xml_ByteString_xml_decode) { */
/*     UA_ExtensionObject *out; */
/*     UA_ByteString buf = UA_STRING("<ListOfExtensionObject>" */
/*                                     "<ExtensionObject>" */
/*                                       "<TypeId>" */
/*                                         "<Identifier>i=297</Identifier>" */
/*                                       "</TypeId>" */
/*                                       "<Body>" */
/*                                         "<Argument>" */
/*                                           "<Name>ObjectToMoveOrCopy</Name>" */
/*                                           "<DataType>" */
/*                                             "<Identifier>i=17</Identifier>" */
/*                                           "</DataType>" */
/*                                           "<ValueRank>-1</ValueRank>" */
/*                                           "<ArrayDimensions />" */
/*                                         "</Argument>" */
/*                                       "</Body>" */
/*                                     "</ExtensionObject>" */
/*                                     "<ExtensionObject>" */
/*                                       "<TypeId>" */
/*                                         "<Identifier>i=297</Identifier>" */
/*                                       "</TypeId>" */
/*                                       "<Body>" */
/*                                         "<Argument>" */
/*                                           "<Name>TargetDirectory</Name>" */
/*                                           "<DataType>" */
/*                                             "<Identifier>i=17</Identifier>" */
/*                                           "</DataType>" */
/*                                           "<ValueRank>-1</ValueRank>" */
/*                                           "<ArrayDimensions />" */
/*                                         "</Argument>" */
/*                                       "</Body>" */
/*                                     "</ExtensionObject>" */
/*                                     "<ExtensionObject>" */
/*                                       "<TypeId>" */
/*                                         "<Identifier>i=297</Identifier>" */
/*                                       "</TypeId>" */
/*                                       "<Body>" */
/*                                         "<Argument>" */
/*                                           "<Name>CreateCopy</Name>" */
/*                                           "<DataType>" */
/*                                             "<Identifier>i=1</Identifier>" */
/*                                           "</DataType>" */
/*                                           "<ValueRank>-1</ValueRank>" */
/*                                           "<ArrayDimensions />" */
/*                                         "</Argument>" */
/*                                       "</Body>" */
/*                                     "</ExtensionObject>" */
/*                                     "<ExtensionObject>" */
/*                                       "<TypeId>" */
/*                                         "<Identifier>ns=4;i=9999</Identifier>" */
/*                                       "</TypeId>" */
/*                                       "<Body>" */
/*                                         "<ByteString>1357911131517192123252729</ByteString>" */
/*                                       "</Body>" */
/*                                     "</ExtensionObject>" */
/*                                   "</ListOfExtensionObject>"); */

/*     UA_StatusCode retval = UA_decodeXml(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL); */

/*     ck_assert_int_eq(retval, UA_STATUSCODE_GOOD); */

/*     ck_assert_int_eq(out[0].encoding, UA_EXTENSIONOBJECT_ENCODED_XML); */
/*     UA_String testData1 = UA_STRING("<Argument>" */
/*                                       "<Name>ObjectToMoveOrCopy</Name>" */
/*                                       "<DataType>" */
/*                                         "<Identifier>i=17</Identifier>" */
/*                                       "</DataType>" */
/*                                       "<ValueRank>-1</ValueRank>" */
/*                                       "<ArrayDimensions />" */
/*                                     "</Argument>"); */
/*     ck_assert(UA_String_equal(&out[0].content.encoded.body, &testData1)); */
/*     ck_assert_int_eq(out[0].content.encoded.typeId.namespaceIndex, 0); */
/*     ck_assert_int_eq(out[0].content.encoded.typeId.identifier.numeric, 297); */
/*     ck_assert_int_eq(out[1].encoding, UA_EXTENSIONOBJECT_ENCODED_XML); */
/*     UA_String testData2 = UA_STRING("<Argument>" */
/*                                       "<Name>TargetDirectory</Name>" */
/*                                       "<DataType>" */
/*                                         "<Identifier>i=17</Identifier>" */
/*                                       "</DataType>" */
/*                                       "<ValueRank>-1</ValueRank>" */
/*                                       "<ArrayDimensions />" */
/*                                     "</Argument>"); */
/*     ck_assert(UA_String_equal(&out[1].content.encoded.body, &testData2)); */
/*     ck_assert_int_eq(out[1].content.encoded.typeId.namespaceIndex, 0); */
/*     ck_assert_int_eq(out[1].content.encoded.typeId.identifier.numeric, 297); */
/*     ck_assert_int_eq(out[2].encoding, UA_EXTENSIONOBJECT_ENCODED_XML); */
/*     UA_String testData3 = UA_STRING("<Argument>" */
/*                                       "<Name>CreateCopy</Name>" */
/*                                       "<DataType>" */
/*                                         "<Identifier>i=1</Identifier>" */
/*                                       "</DataType>" */
/*                                       "<ValueRank>-1</ValueRank>" */
/*                                       "<ArrayDimensions />" */
/*                                     "</Argument>"); */
/*     ck_assert(UA_String_equal(&out[2].content.encoded.body, &testData3)); */
/*     ck_assert_int_eq(out[2].content.encoded.typeId.namespaceIndex, 0); */
/*     ck_assert_int_eq(out[2].content.encoded.typeId.identifier.numeric, 297); */
/*     ck_assert_int_eq(out[3].encoding, UA_EXTENSIONOBJECT_ENCODED_BYTESTRING); */
/*     UA_String testData4 = UA_STRING("1357911131517192123252729"); */
/*     ck_assert(UA_String_equal(&out[3].content.encoded.body, &testData4)); */
/*     ck_assert_int_eq(out[3].content.encoded.typeId.namespaceIndex, 4); */
/*     ck_assert_int_eq(out[3].content.encoded.typeId.identifier.numeric, 9999); */

/*     UA_Array_delete(out, 4, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]); */
/* } */
/* END_TEST */


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

    tcase_add_test(tc_xml_encode, UA_ByteString_xml_encode);
    tcase_add_test(tc_xml_encode, UA_ByteString2_xml_encode);
    tcase_add_test(tc_xml_encode, UA_ByteString3_xml_encode);

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

    tcase_add_test(tc_xml_encode, UA_StatusCode_xml_encode);
    tcase_add_test(tc_xml_encode, UA_StatusCode_good_xml_encode);
    tcase_add_test(tc_xml_encode, UA_StatusCode_smallbuf_xml_encode);

    tcase_add_test(tc_xml_encode, UA_QualifiedName_xml_encode_1);
    tcase_add_test(tc_xml_encode, UA_QualifiedName_xml_encode_2);

    tcase_add_test(tc_xml_encode, UA_LocalizedText_xml_encode);
    tcase_add_test(tc_xml_encode, UA_LocalizedText_empty_text_xml_encode);
    tcase_add_test(tc_xml_encode, UA_LocalizedText_null_locale_xml_encode);
    tcase_add_test(tc_xml_encode, UA_LocalizedText_null_xml_encode);

    tcase_add_test(tc_xml_encode, UA_UInt32_print_xml_encode);
    tcase_add_test(tc_xml_encode, UA_ExpandedNodeId_print_xml_encode);
    tcase_add_test(tc_xml_encode, UA_QualifiedName_print_xml_encode);

    tcase_add_test(tc_xml_encode, UA_ExtensionObject_xml_xml_encode);
    tcase_add_test(tc_xml_encode, UA_ExtensionObject_byteString_xml_encode);
    tcase_add_test(tc_xml_encode, UA_ExtensionObject_xml_body_encode);
    tcase_add_test(tc_xml_encode, UA_ExtensionObject_null_xml_encode);
    tcase_add_test(tc_xml_encode, UA_ExtensionObject_print_xml_encode);

    tcase_add_test(tc_xml_encode, UA_Array_Variant_Byte_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Array_Variant_UInt16_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Array_Variant_Null_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Array_Variant_String_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Array_Variant_ExtensionObject_ByteString_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Array_Variant_ExtensionObject_Xml_xml_encode);
    tcase_add_test(tc_xml_encode, UA_Array_Variant_ExtensionObject_print_xml_encode);

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
    // tcase_add_test(tc_xml_decode, UA_String_escape2_xml_decode);

    tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode);
    tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode_large);
    // tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode_negative);
    // tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode_min);
    // tcase_add_test(tc_xml_decode, UA_DateTime_xml_decode_max);
    tcase_add_test(tc_xml_decode, UA_DateTime_micro_xml_decode);

    tcase_add_test(tc_xml_decode, UA_Guid_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Guid_lower_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Guid_tooShort_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Guid_tooLong_xml_decode);
    tcase_add_test(tc_xml_decode, UA_Guid_wrong_xml_decode);

    tcase_add_test(tc_xml_decode, UA_ByteString_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ByteString_null_xml_decode);

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

    tcase_add_test(tc_xml_decode, UA_StatusCode_0_xml_decode);
    tcase_add_test(tc_xml_decode, UA_StatusCode_2_xml_decode);
    tcase_add_test(tc_xml_decode, UA_StatusCode_3_xml_decode);

    tcase_add_test(tc_xml_decode, UA_QualifiedName_1_xml_decode);
    tcase_add_test(tc_xml_decode, UA_QualifiedName_2_xml_decode);

    tcase_add_test(tc_xml_decode, UA_LocalizedText_xml_decode);
    tcase_add_test(tc_xml_decode, UA_LocalizedText_missing_text_xml_decode);
    tcase_add_test(tc_xml_decode, UA_LocalizedText_empty_locale_xml_decode);
    tcase_add_test(tc_xml_decode, UA_LocalizedText_empty_xml_decode);

    tcase_add_test(tc_xml_decode, UA_ExtensionObject_EncodedByteString_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExtensionObject_EncodedXml_1_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExtensionObject_EncodedXml_2_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExtensionObject_EncodedXml_3_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExtensionObject_EncodedXml_4_xml_decode);
    tcase_add_test(tc_xml_decode, UA_ExtensionObject_InvalidBody_xml_decode);

    /* tcase_add_test(tc_xml_decode, UA_Array_Boolean_xml_decode); */
    /* tcase_add_test(tc_xml_decode, UA_Array_Number_xml_decode); */
    /* tcase_add_test(tc_xml_decode, UA_Array_Null_xml_decode); */
    /* tcase_add_test(tc_xml_decode, UA_Array_String_xml_decode); */
    /* tcase_add_test(tc_xml_decode, UA_Array_ExtensionObject_ByteString_xml_decode); */
    /* tcase_add_test(tc_xml_decode, UA_Array_ExtensionObject_Xml_ByteString_xml_decode); */

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
