/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_encoding_json.h"
#include "ua_types_generated.h"
#include "ua_types_generated_handling.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_util.h"
#include "check.h"

/* Test Boolean */
START_TEST(UA_Boolean_true_json_encode) {
   
    UA_Boolean *src = UA_Boolean_new();
    UA_Boolean_init(src);
    *src = UA_TRUE;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BOOLEAN];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "true";
    ck_assert_str_eq(result, (char*)buf.data);
    
    UA_ByteString_deleteMembers(&buf);
    UA_Boolean_delete(src);
    
}
END_TEST

START_TEST(UA_Boolean_false_json_encode) {
   
    UA_Boolean *src = UA_Boolean_new();
    UA_Boolean_init(src);
    *src = UA_FALSE;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BOOLEAN];

    UA_ByteString buf;

    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    ck_assert_int_eq(size, 5);
    
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];
        
    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "false";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Boolean_delete(src);
}
END_TEST

START_TEST(UA_Boolean_null_json_encode) {
   
    UA_Boolean *src = NULL;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BOOLEAN];

    UA_ByteString buf;

    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    
    UA_ByteString_allocBuffer(&buf, size+1+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGERROR);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_Boolean_true_bufferTooSmall_json_encode) {
   
    UA_Boolean *src = UA_Boolean_new();
    UA_Boolean_init(src);
    *src = UA_FALSE;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BOOLEAN];

    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    char* result = "";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Boolean_delete(src);
}
END_TEST

/* Test String */
START_TEST(UA_String_json_encode) {
    // given
    UA_String src = UA_STRING("hello");
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    
    UA_ByteString buf;
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];
    // when
    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);
    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"hello\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_String_Empty_json_encode) {
    // given
    UA_String src = UA_STRING("");
    UA_ByteString buf;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];
    // when
    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);
    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_String_Null_json_encode) {
    // given
    UA_String src = UA_STRING_NULL;
    UA_ByteString buf;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString_allocBuffer(&buf, size+1);
 
    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];
    // when
    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);
    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "null";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_String_escapesimple_json_encode) {
    // given
    UA_String src = UA_STRING("\b\th\"e\fl\nl\\o\r");
    UA_ByteString buf;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];
    // when
    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);
    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"\\b\\th\\\"e\\fl\\nl\\\\o\\r\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_String_escapeutf_json_encode) {
    // given
    UA_String src = UA_STRING("he\\zsdl\alo€ \x26\x3A asdasd");
    UA_ByteString buf;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_STRING];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];
    // when
    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);
    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"he\\\\zsdl\\u0007lo€ &: asdasd\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

/* Byte */
START_TEST(UA_Byte_Max_Number_json_encode) {

    UA_Byte *src = UA_Byte_new();
    *src = 255;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTE];

    UA_ByteString buf;
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "255";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Byte_delete(src);
}
END_TEST

START_TEST(UA_Byte_Min_Number_json_encode) {

    UA_Byte *src = UA_Byte_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTE];

    UA_ByteString buf;
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Byte_delete(src);
}
END_TEST

START_TEST(UA_Byte_smallbuf_Number_json_encode) {

    UA_Byte *src = UA_Byte_new();
    *src = 255;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTE];
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf);
    UA_Byte_delete(src);
}
END_TEST

/* sByte */
START_TEST(UA_SByte_Max_Number_json_encode) {

    UA_SByte *src = UA_SByte_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_SBYTE];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "127";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_SByte_delete(src);
}
END_TEST

START_TEST(UA_SByte_Min_Number_json_encode) {

    UA_SByte *src = UA_SByte_new();
    *src = -128;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_SBYTE];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "-128";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_SByte_delete(src);
}
END_TEST

START_TEST(UA_SByte_Zero_Number_json_encode) {

    UA_SByte *src = UA_SByte_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_SBYTE];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_SByte_delete(src);
}
END_TEST

START_TEST(UA_SByte_smallbuf_Number_json_encode) {

    UA_SByte *src = UA_SByte_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_SBYTE];
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf); 
    UA_SByte_delete(src);
}
END_TEST


/* UInt16 */
START_TEST(UA_UInt16_Max_Number_json_encode) {

    UA_UInt16 *src = UA_UInt16_new();
    *src = 65535;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT16];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "65535";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_UInt16_delete(src);
}
END_TEST

START_TEST(UA_UInt16_Min_Number_json_encode) {

    UA_UInt16 *src = UA_UInt16_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT16];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_UInt16_delete(src);
}
END_TEST

START_TEST(UA_UInt16_smallbuf_Number_json_encode) {

    UA_UInt16 *src = UA_UInt16_new();
    *src = 255;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT16];
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf); 
    UA_UInt16_delete(src);
}
END_TEST

/* Int16 */
START_TEST(UA_Int16_Max_Number_json_encode) {

    UA_Int16 *src = UA_Int16_new();
    *src = 32767;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT16];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "32767";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Int16_delete(src);
}
END_TEST

START_TEST(UA_Int16_Min_Number_json_encode) {

    UA_Int16 *src = UA_Int16_new();
    *src = -32768;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT16];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "-32768";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Int16_delete(src);
}
END_TEST

START_TEST(UA_Int16_Zero_Number_json_encode) {

    UA_Int16 *src = UA_Int16_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT16];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Int16_delete(src);
}
END_TEST

START_TEST(UA_Int16_smallbuf_Number_json_encode) {

    UA_Int16 *src = UA_Int16_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT16];
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf);
    UA_Int16_delete(src);
}
END_TEST



/* UInt32 */
START_TEST(UA_UInt32_Max_Number_json_encode) {

    UA_UInt32 *src = UA_UInt32_new();
    *src = 4294967295;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT32];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "4294967295";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_UInt32_delete(src);
}
END_TEST

START_TEST(UA_UInt32_Min_Number_json_encode) {

    UA_UInt32 *src = UA_UInt32_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT32];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_UInt32_delete(src);
}
END_TEST

START_TEST(UA_UInt32_smallbuf_Number_json_encode) {

    UA_UInt32 *src = UA_UInt32_new();
    *src = 255;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT32];
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf);
    UA_UInt32_delete(src);
}
END_TEST



/* Int32 */
START_TEST(UA_Int32_Max_Number_json_encode) {

    UA_Int32 *src = UA_Int32_new();
    *src = 2147483647;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT32];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "2147483647";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Int32_delete(src);
}
END_TEST

START_TEST(UA_Int32_Min_Number_json_encode) {

    UA_Int32 *src = UA_Int32_new();
    *src = -2147483648;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT32];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "-2147483648";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Int32_delete(src);
}
END_TEST

START_TEST(UA_Int32_Zero_Number_json_encode) {

    UA_Int32 *src = UA_Int32_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT32];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Int32_delete(src);
}
END_TEST

START_TEST(UA_Int32_smallbuf_Number_json_encode) {

    UA_Int32 *src = UA_Int32_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT32];

    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf); 
    UA_Int32_delete(src);
}
END_TEST



/* UINT64*/
START_TEST(UA_UInt64_Max_Number_json_encode) {

    UA_UInt64 *src = UA_UInt64_new();
    //*src = 18446744073709551615;
    ((u8*)src)[0] = 0xFF;
    ((u8*)src)[1] = 0xFF;
    ((u8*)src)[2] = 0xFF;
    ((u8*)src)[3] = 0xFF;
    ((u8*)src)[4] = 0xFF;
    ((u8*)src)[5] = 0xFF;
    ((u8*)src)[6] = 0xFF;
    ((u8*)src)[7] = 0xFF;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT64];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "18446744073709551615";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_UInt64_delete(src);
}
END_TEST

START_TEST(UA_UInt64_Min_Number_json_encode) {

    UA_UInt64 *src = UA_UInt64_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT64];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_UInt64_delete(src);
}
END_TEST

START_TEST(UA_UInt64_smallbuf_Number_json_encode) {

    UA_UInt64 *src = UA_UInt64_new();
    //*src = -9223372036854775808;
    ((u8*)src)[0] = 0x00;
    ((u8*)src)[1] = 0x00;
    ((u8*)src)[2] = 0x00;
    ((u8*)src)[3] = 0x00;
    ((u8*)src)[4] = 0x00;
    ((u8*)src)[5] = 0x00;
    ((u8*)src)[6] = 0x00;
    ((u8*)src)[7] = 0x80;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_UINT64];

    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf); 
    UA_UInt64_delete(src);
}
END_TEST

/* Int64 */
START_TEST(UA_Int64_Max_Number_json_encode) {

    UA_Int64 *src = UA_Int64_new();
    //*src = 9223372036854775808;
    ((u8*)src)[0] = 0xFF;
    ((u8*)src)[1] = 0xFF;
    ((u8*)src)[2] = 0xFF;
    ((u8*)src)[3] = 0xFF;
    ((u8*)src)[4] = 0xFF;
    ((u8*)src)[5] = 0xFF;
    ((u8*)src)[6] = 0xFF;
    ((u8*)src)[7] = 0x7F;
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT64];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "9223372036854775807";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Int64_delete(src);
}
END_TEST

START_TEST(UA_Int64_Min_Number_json_encode) {

    UA_Int64 *src = UA_Int64_new();
    
    // TODO: compiler error: integer constant is so large that it is unsigned [-Werror]
    //*src = -9223372036854775808;
  
    ((u8*)src)[0] = 0x00;
    ((u8*)src)[1] = 0x00;
    ((u8*)src)[2] = 0x00;
    ((u8*)src)[3] = 0x00;
    ((u8*)src)[4] = 0x00;
    ((u8*)src)[5] = 0x00;
    ((u8*)src)[6] = 0x00;
    ((u8*)src)[7] = 0x80;
    
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT64];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "-9223372036854775808";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Int64_delete(src);
}
END_TEST

START_TEST(UA_Int64_Zero_Number_json_encode) {

    UA_Int64 *src = UA_Int64_new();
    *src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT64];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Int64_delete(src);
}
END_TEST

START_TEST(UA_Int64_smallbuf_Number_json_encode) {

    UA_Int64 *src = UA_Int64_new();
    *src = 127;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_INT64];

    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf); 
    UA_Int64_delete(src);
}
END_TEST


START_TEST(UA_Double_json_encode) {
    UA_Double src = 1.1234;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    
    /* TODO: floating point behavior: clang results in "1.1233999999999999" */
    char* result = "1.1234"; 
    char* result2 = "1.1233999999999999"; 
    if(memcmp(result, (char*)buf.data, 6) != 0 
            && memcmp(result2, (char*)buf.data, 18) != 0){
        UA_ByteString_deleteMembers(&buf);
        ck_abort_msg("%s", "");
    }else{
        UA_ByteString_deleteMembers(&buf);
    }
    
}
END_TEST

START_TEST(UA_Double_pluszero_json_encode) {
    UA_Double src = 0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_Double_minuszero_json_encode) {
    UA_Double src = -0;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "0";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_Double_plusInf_json_encode) {
    UA_Double src = INFINITY;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"Infinity\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST



START_TEST(UA_Double_minusInf_json_encode) {
    UA_Double src = -INFINITY;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"-Infinity\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_Double_nan_json_encode) {
    UA_Double src = NAN;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"NaN\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_Double_onesmallest_json_encode) {
    UA_Double src = 1.0000000000000002;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DOUBLE];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "1.0000000000000002";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_Float_json_encode) {
    UA_Float src = 1.0000000000F;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_FLOAT];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "1";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST




/* -------------------------LocalizedText------------------------- */
START_TEST(UA_LocText_json_encode) {

    UA_LocalizedText *src = UA_LocalizedText_new();
    UA_LocalizedText_init(src);
    src->locale = UA_STRING_ALLOC("theLocale");
    src->text = UA_STRING_ALLOC("theText");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Locale\":\"theLocale\",\"Text\":\"theText\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_LocalizedText_delete(src);
}
END_TEST


START_TEST(UA_LocText_NonReversible_json_encode) {

    UA_LocalizedText *src = UA_LocalizedText_new();
    UA_LocalizedText_init(src);
    src->locale = UA_STRING_ALLOC("theLocale");
    src->text = UA_STRING_ALLOC("theText");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"theText\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_LocalizedText_delete(src);
}
END_TEST


START_TEST(UA_LocText_smallBuffer_json_encode) {

    UA_LocalizedText *src = UA_LocalizedText_new();
    UA_LocalizedText_init(src);
    src->locale = UA_STRING_ALLOC("theLocale");
    src->text = UA_STRING_ALLOC("theText");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 4);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[4];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf);
    UA_LocalizedText_delete(src);
}
END_TEST

/* --------------------------------GUID----------------------------------- */
START_TEST(UA_Guid_json_encode) {
    UA_Guid src = {3, 9, 10, {8, 7, 6, 5, 4, 3, 2, 1}};
    const UA_DataType *type = &UA_TYPES[UA_TYPES_GUID];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) &src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);
    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"00000003-0009-000A-0807-060504030201\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
}
END_TEST

START_TEST(UA_Guid_smallbuf_json_encode) {
    UA_Guid *src = UA_Guid_new();
    *src = UA_Guid_random();
    const UA_DataType *type = &UA_TYPES[UA_TYPES_GUID];
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);
    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf);
    UA_Guid_delete(src);
}
END_TEST



/* -------------------------DateTime--------------------------------------*/
START_TEST(UA_DateTime_json_encode) {
    UA_DateTime *src = UA_DateTime_new();
    *src = UA_DateTime_fromUnixTime(1234567);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DATETIME];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"1970-01-15T06:56:07.000Z\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_DateTime_delete(src);
}
END_TEST


/* ------------------------Statuscode--------------------------------- */
START_TEST(UA_StatusCode_json_encode) {
    UA_StatusCode *src = UA_StatusCode_new();
    *src = UA_STATUSCODE_BADAGGREGATECONFIGURATIONREJECTED;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STATUSCODE];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

     *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "2161770496";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_StatusCode_delete(src);
}
END_TEST

START_TEST(UA_StatusCode_nonReversible_json_encode) {
    UA_StatusCode *src = UA_StatusCode_new();
    *src = UA_STATUSCODE_BADAGGREGATECONFIGURATIONREJECTED;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STATUSCODE];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

     *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Code\":2161770496,\"Symbol\":\"BadAggregateConfigurationRejected\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_StatusCode_delete(src);
}
END_TEST

START_TEST(UA_StatusCode_nonReversible_good_json_encode) {
    UA_StatusCode *src = UA_StatusCode_new();
    *src = UA_STATUSCODE_GOOD;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STATUSCODE];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

     *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "null";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_StatusCode_delete(src);
}
END_TEST


START_TEST(UA_StatusCode_smallbuf_json_encode) {
    UA_StatusCode *src = UA_StatusCode_new();
    *src = UA_STATUSCODE_BADAGGREGATECONFIGURATIONREJECTED;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_STATUSCODE];
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf); 
    UA_StatusCode_delete(src);
}
END_TEST


/* -------------------------------NodeId--------------------------------*/

/* Numeric */
START_TEST(UA_NodeId_Numeric_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_NUMERIC(0, 5555);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Id\":5555}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_Numeric_Namespace_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_NUMERIC(4, 5555);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Id\":5555,\"Namespace\":4}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_NodeId_delete(src);
}
END_TEST

/* String */
START_TEST(UA_NodeId_String_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_STRING_ALLOC(0, "foobar");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":1,\"Id\":\"foobar\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_String_Namespace_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_STRING_ALLOC(5, "foobar");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":1,\"Id\":\"foobar\",\"Namespace\":5}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_NodeId_delete(src);
}
END_TEST

/* Guid */
START_TEST(UA_NodeId_Guid_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    UA_NodeId_init(src);
    UA_Guid g = {3, 9, 10, {8, 7, 6, 5, 4, 3, 2, 1}};
    *src = UA_NODEID_GUID(0, g);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":2,\"Id\":\"00000003-0009-000A-0807-060504030201\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_Guid_Namespace_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    UA_Guid g = {3, 9, 10, {8, 7, 6, 5, 4, 3, 2, 1}};
    *src = UA_NODEID_GUID(5, g);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    // {"IdType":2,"Id":"00000003-0009-000A-0807-060504030201","Namespace":5}
    ck_assert_int_eq(size, 70);
    
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":2,\"Id\":\"00000003-0009-000A-0807-060504030201\",\"Namespace\":5}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_NodeId_delete(src);
}
END_TEST

/* ByteString */
START_TEST(UA_NodeId_ByteString_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_BYTESTRING_ALLOC(0, "asdfasdf");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    //{"IdType":3,"Id":"YXNkZmFzZGY="}
    ck_assert_int_eq(size, 32);
    
    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":3,\"Id\":\"YXNkZmFzZGY=\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_NodeId_delete(src);
}
END_TEST

START_TEST(UA_NodeId_ByteString_Namespace_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_BYTESTRING_ALLOC(5, "asdfasdf");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":3,\"Id\":\"YXNkZmFzZGY=\",\"Namespace\":5}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_NodeId_delete(src);
}
END_TEST

/* Non-reversible, Namespace */
START_TEST(UA_NodeId_NonReversible_Numeric_Namespace_json_encode) {
    UA_NodeId *src = UA_NodeId_new();
    *src = UA_NODEID_NUMERIC(2, 5555);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_NODEID];
    
    UA_String namespaces[3] = {UA_STRING("ns0"),UA_STRING("ns1"),UA_STRING("ns2")};
    size_t size = UA_calcSizeJson((void *) src, type, namespaces, 3, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];


    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, namespaces, 3, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Id\":5555,\"Namespace\":\"ns2\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_NodeId_delete(src);
}
END_TEST

/* --------------------Diagnostic Info------------------------- */
START_TEST(UA_DiagInfo_json_encode) {
    UA_DiagnosticInfo *src = UA_DiagnosticInfo_new();
    UA_DiagnosticInfo_init(src);
    src->hasAdditionalInfo = UA_TRUE;
    src->hasInnerDiagnosticInfo = UA_FALSE;
    src->hasInnerStatusCode = UA_TRUE;
    src->hasLocale = UA_TRUE;
    src->hasSymbolicId = UA_TRUE;
    src->hasLocalizedText = UA_TRUE;
    src->hasNamespaceUri = UA_TRUE;

    UA_StatusCode statusCode = UA_STATUSCODE_BADARGUMENTSMISSING;
    src->additionalInfo = UA_STRING_ALLOC("additionalInfo");
    src->innerStatusCode = statusCode;
    src->locale = 12;
    src->symbolicId = 13;
    src->localizedText = 14;
    src->namespaceUri = 15;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_DIAGNOSTICINFO];

    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"SymbolicId\":13,\"NamespaceUri\":15,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_DiagnosticInfo_delete(src);
}
END_TEST



START_TEST(UA_DiagInfo_withInner_json_encode) {
    UA_DiagnosticInfo *innerDiag = UA_DiagnosticInfo_new();
    UA_DiagnosticInfo_init(innerDiag);
    innerDiag->hasAdditionalInfo = UA_TRUE;
    innerDiag->additionalInfo = UA_STRING_ALLOC("INNER ADDITION INFO");
    innerDiag->hasInnerDiagnosticInfo = UA_FALSE;
    innerDiag->hasInnerStatusCode = UA_FALSE;
    innerDiag->hasLocale = UA_FALSE;
    innerDiag->hasSymbolicId = UA_FALSE;
    innerDiag->hasLocalizedText = UA_FALSE;
    innerDiag->hasNamespaceUri = UA_FALSE;

    UA_DiagnosticInfo *src = UA_DiagnosticInfo_new();
    UA_DiagnosticInfo_init(src);
    src->hasAdditionalInfo = UA_TRUE;
    src->hasInnerDiagnosticInfo = UA_TRUE;
    src->hasInnerStatusCode = UA_TRUE;
    src->hasLocale = UA_TRUE;
    src->hasSymbolicId = UA_TRUE;
    src->hasLocalizedText = UA_TRUE;
    src->hasNamespaceUri = UA_FALSE;

    UA_StatusCode statusCode = UA_STATUSCODE_BADARGUMENTSMISSING;
    src->additionalInfo = UA_STRING_ALLOC("additionalInfo");
    src->innerDiagnosticInfo = innerDiag;
    src->innerStatusCode = statusCode;
    src->locale = 12;
    src->symbolicId = 13;
    src->localizedText = 14;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_DIAGNOSTICINFO];


    UA_ByteString buf;
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    //{"SymbolicId":13,"LocalizedText":14,"Locale":12,"AdditionalInfo":"additionalInfo","InnerStatusCode":2155216896,"InnerDiagnosticInfo":{"AdditionalInfo":"INNER ADDITION INFO"}}
    ck_assert_int_eq(size, 174);
    
    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896,\"InnerDiagnosticInfo\":{\"AdditionalInfo\":\"INNER ADDITION INFO\"}}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_DiagnosticInfo_delete(src);
}
END_TEST

START_TEST(UA_DiagInfo_withTwoInner_json_encode) {
    
    UA_DiagnosticInfo *innerDiag2 = UA_DiagnosticInfo_new();
    UA_DiagnosticInfo_init(innerDiag2);
    innerDiag2->hasAdditionalInfo = UA_TRUE;
    innerDiag2->additionalInfo = UA_STRING_ALLOC("INNER ADDITION INFO2");
    innerDiag2->hasInnerDiagnosticInfo = UA_FALSE;
    innerDiag2->hasInnerStatusCode = UA_FALSE;
    innerDiag2->hasLocale = UA_FALSE;
    innerDiag2->hasSymbolicId = UA_FALSE;
    innerDiag2->hasLocalizedText = UA_FALSE;
    innerDiag2->hasNamespaceUri = UA_FALSE;
    
    UA_DiagnosticInfo *innerDiag = UA_DiagnosticInfo_new();
    UA_DiagnosticInfo_init(innerDiag);
    innerDiag->hasAdditionalInfo = UA_TRUE;
    innerDiag->additionalInfo = UA_STRING_ALLOC("INNER ADDITION INFO");
    innerDiag->hasInnerDiagnosticInfo = UA_TRUE;
    innerDiag->innerDiagnosticInfo = innerDiag2;
    innerDiag->hasInnerStatusCode = UA_FALSE;
    innerDiag->hasLocale = UA_FALSE;
    innerDiag->hasSymbolicId = UA_FALSE;
    innerDiag->hasLocalizedText = UA_FALSE;
    innerDiag->hasNamespaceUri = UA_FALSE;

    UA_DiagnosticInfo *src = UA_DiagnosticInfo_new();
    UA_DiagnosticInfo_init(src);
    
    src->hasAdditionalInfo = UA_TRUE;
    src->hasInnerDiagnosticInfo = UA_TRUE;
    src->hasInnerStatusCode = UA_TRUE;
    src->hasLocale = UA_TRUE;
    src->hasSymbolicId = UA_TRUE;
    src->hasLocalizedText = UA_TRUE;
    src->hasNamespaceUri = UA_FALSE;

    UA_StatusCode statusCode = UA_STATUSCODE_BADARGUMENTSMISSING;
    src->additionalInfo = UA_STRING_ALLOC("additionalInfo");
    src->innerDiagnosticInfo = innerDiag;
    src->innerStatusCode = statusCode;
    src->locale = 12;
    src->symbolicId = 13;
    src->localizedText = 14;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_DIAGNOSTICINFO];

    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896,\"InnerDiagnosticInfo\":{\"AdditionalInfo\":\"INNER ADDITION INFO\",\"InnerDiagnosticInfo\":{\"AdditionalInfo\":\"INNER ADDITION INFO2\"}}}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_DiagnosticInfo_delete(src);
}
END_TEST

START_TEST(UA_DiagInfo_noFields_json_encode) {
    UA_DiagnosticInfo *src = UA_DiagnosticInfo_new();
    UA_DiagnosticInfo_init(src);
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DIAGNOSTICINFO];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "null";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_DiagnosticInfo_delete(src);
}
END_TEST

START_TEST(UA_DiagInfo_smallBuffer_json_encode) {
    UA_DiagnosticInfo *src = UA_DiagnosticInfo_new();
    UA_DiagnosticInfo_init(src);
    src->hasAdditionalInfo = UA_TRUE;
    src->hasInnerDiagnosticInfo = UA_FALSE;
    src->hasInnerStatusCode = UA_TRUE;
    src->hasLocale = UA_TRUE;
    src->hasSymbolicId = UA_TRUE;
    src->hasLocalizedText = UA_TRUE;
    src->hasNamespaceUri = UA_FALSE;

    UA_StatusCode statusCode = UA_STATUSCODE_BADARGUMENTSMISSING;
    src->additionalInfo = UA_STRING_ALLOC("additionalInfo");
    src->innerStatusCode = statusCode;
    src->locale = 12;
    src->symbolicId = 13;
    src->localizedText = 14;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_DIAGNOSTICINFO];

    
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, 2);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[2];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_ByteString_deleteMembers(&buf); 
    UA_DiagnosticInfo_delete(src);
}
END_TEST

/* ---------------ByteString----------------- */
START_TEST(UA_ByteString_json_encode) {
    UA_ByteString *src = UA_ByteString_new();
    UA_ByteString_init(src);
    *src = UA_BYTESTRING_ALLOC("asdfasdf");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTESTRING];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"YXNkZmFzZGY=\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ByteString_delete(src);
}
END_TEST

START_TEST(UA_ByteString_Long_json_encode) {
    UA_ByteString *src = UA_ByteString_new();
    UA_ByteString_init(src);
    *src = UA_BYTESTRING_ALLOC("Lorem ipsum dolor sit amet, consectetur adipiscing elit.");
    const UA_DataType *type = &UA_TYPES[UA_TYPES_BYTESTRING];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    
    //s = UA_decodeJson()
    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "\"TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdC4=\"";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_ByteString_delete(src);
}
END_TEST

/* ------------------QualifiedName---------------------------- */
START_TEST(UA_QualName_json_encode) {
    UA_QualifiedName *src = UA_QualifiedName_new();
    UA_QualifiedName_init(src);
    src->name = UA_STRING_ALLOC("derName");
    src->namespaceIndex = 1;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Name\":\"derName\",\"Uri\":1}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_QualifiedName_delete(src);
}
END_TEST



START_TEST(UA_QualName_NonReversible_json_encode) {
    UA_QualifiedName *src = UA_QualifiedName_new();
    UA_QualifiedName_init(src);
    src->name = UA_STRING_ALLOC("derName");
    src->namespaceIndex = 1;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Name\":\"derName\",\"Uri\":1}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_QualifiedName_delete(src);
}
END_TEST

START_TEST(UA_QualName_NonReversible_Namespace_json_encode) {
    UA_QualifiedName *src = UA_QualifiedName_new();
    UA_QualifiedName_init(src);
    src->name = UA_STRING_ALLOC("derName");
    src->namespaceIndex = 2;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];
    UA_String namespaces[3] = {UA_STRING("ns0"),UA_STRING("ns1"),UA_STRING("ns2")};
    size_t size = UA_calcSizeJson((void *) src, type, namespaces, 3, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];


    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, namespaces, 3, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Name\":\"derName\",\"Uri\":\"ns2\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_QualifiedName_delete(src);
}
END_TEST

START_TEST(UA_QualName_NonReversible_NoNamespaceAsNumber_json_encode) {
    UA_QualifiedName *src = UA_QualifiedName_new();
    UA_QualifiedName_init(src);
    src->name = UA_STRING_ALLOC("derName");
    src->namespaceIndex = 6789;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Name\":\"derName\",\"Uri\":6789}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_QualifiedName_delete(src);
}
END_TEST

/* ----------------------------Variant------------------------ */

/* -----Builtin scalar----- */
START_TEST(UA_Variant_Bool_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    
    UA_Boolean *variantContent = UA_Boolean_new();
    *variantContent = true;
    UA_Variant_setScalar(src, variantContent, &UA_TYPES[UA_TYPES_BOOLEAN]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":1,\"Body\":true}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Variant_Number_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_UInt64 *variantContent = UA_UInt64_new();
    *variantContent = 345634563456;
    UA_Variant_setScalar(src, variantContent, &UA_TYPES[UA_TYPES_UINT64]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":9,\"Body\":345634563456}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Variant_delete(src);
}
END_TEST


START_TEST(UA_Variant_NodeId_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_NodeId variantContent = UA_NODEID_STRING(1, "theID");
    UA_Variant_setScalarCopy(src, &variantContent, &UA_TYPES[UA_TYPES_NODEID]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":17,\"Body\":{\"IdType\":1,\"Id\":\"theID\",\"Namespace\":1}}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Variant_LocText_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_LocalizedText variantContent;
    variantContent.locale = UA_STRING("localeString");
    variantContent.text = UA_STRING("textString");
    UA_Variant_setScalarCopy(src, &variantContent, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":21,\"Body\":{\"Locale\":\"localeString\",\"Text\":\"textString\"}}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Variant_QualName_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_QualifiedName variantContent;
    UA_QualifiedName_init(&variantContent);
    variantContent.name = UA_STRING("derName");
    variantContent.namespaceIndex = 1;
    
    UA_Variant_setScalarCopy(src, &variantContent, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":20,\"Body\":{\"Name\":\"derName\",\"Uri\":1}}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Variant_delete(src);
}
END_TEST


/* ---Reversible Variant Array---- */
START_TEST(UA_Variant_Array_UInt16_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_UInt16 zero[2] = {42,43};
    UA_Variant_setArrayCopy(src, zero, 2, &UA_TYPES[UA_TYPES_UINT16]);
   

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    ck_assert_int_eq(size, 25);
    
    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":5,\"Body\":[42,43]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Variant_Array_Byte_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_Byte zero[2] = {42,43};
    UA_Variant_setArrayCopy(src, zero, 2, &UA_TYPES[UA_TYPES_BYTE]);
   

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":3,\"Body\":[42,43]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Variant_Array_String_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_String zero[2] = {UA_STRING("eins"),UA_STRING("zwei")};
    UA_Variant_setArrayCopy(src, zero, 2, &UA_TYPES[UA_TYPES_STRING]);
   

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":12,\"Body\":[\"eins\",\"zwei\"]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Variant_Matrix_UInt16_json_encode) {

     // Set an array value
    UA_Variant src;
    UA_Variant_init(&src);
    UA_UInt16 d[9] = {1, 2, 3,
                      4, 5, 6,
                      7, 8, 9};
    UA_Variant_setArrayCopy(&src, d, 9, &UA_TYPES[UA_TYPES_UINT16]);

    //Set array dimensions
    src.arrayDimensions = (UA_UInt32 *)UA_Array_new(2, &UA_TYPES[UA_TYPES_UINT32]);
    src.arrayDimensionsSize = 2;
    src.arrayDimensions[0] = 3;
    src.arrayDimensions[1] = 3;
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    //{"Type":5,"Body":[1,2,3,4,5,6,7,8,9],"Dimension":[3,3]}
    size_t sizeOfBytes = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    ck_assert_int_eq(sizeOfBytes, 55);
    
    status s = UA_encodeJson((void *) &src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":5,\"Body\":[1,2,3,4,5,6,7,8,9],\"Dimension\":[3,3]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_deleteMembers(&src);
}
END_TEST


/* NON-Reversible builtin simple */
START_TEST(UA_Variant_StatusCode_NonReversible_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_StatusCode variantContent = UA_STATUSCODE_BADAGGREGATECONFIGURATIONREJECTED;
    UA_Variant_setScalarCopy(src, &variantContent, &UA_TYPES[UA_TYPES_STATUSCODE]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Body\":{\"Code\":2161770496,\"Symbol\":\"BadAggregateConfigurationRejected\"}}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf); 
    UA_Variant_delete(src);
}
END_TEST


/* NON-Reversible Array */
START_TEST(UA_Variant_Array_String_NonReversible_json_encode) {
    UA_Variant src;
    UA_Variant_init(&src);
    UA_String d[] = {UA_STRING("1"), UA_STRING("2"), UA_STRING("3"),
                      UA_STRING("4"), UA_STRING("5"), UA_STRING("6"),
                      UA_STRING("7"), UA_STRING("8")};
    UA_Variant_setArrayCopy(&src, d, 8, &UA_TYPES[UA_TYPES_STRING]);

    src.arrayDimensions = (UA_UInt32 *)UA_Array_new(4, &UA_TYPES[UA_TYPES_UINT32]);
    src.arrayDimensionsSize = 1;
    src.arrayDimensions[0] = 8;
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) &src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Body\":[\"1\",\"2\",\"3\",\"4\",\"5\",\"6\",\"7\",\"8\"]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_deleteMembers(&src);
}
END_TEST

/* NON-Reversible Matrix */
START_TEST(UA_Variant_Matrix_String_NonReversible_json_encode) {
    UA_Variant src;
    UA_Variant_init(&src);
    UA_String d[] = {UA_STRING("1"), UA_STRING("2"), UA_STRING("3"),
                      UA_STRING("4"), UA_STRING("5"), UA_STRING("6"),
                      UA_STRING("7"), UA_STRING("8")};
    UA_Variant_setArrayCopy(&src, d, 8, &UA_TYPES[UA_TYPES_STRING]);

    src.arrayDimensions = (UA_UInt32 *)UA_Array_new(4, &UA_TYPES[UA_TYPES_UINT32]);
    src.arrayDimensionsSize = 4;
    src.arrayDimensions[0] = 2;
    src.arrayDimensions[1] = 2;
    src.arrayDimensions[2] = 2;
    src.arrayDimensions[3] = 1;
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    //{"Body":[[[["1"],["2"]],[["3"],["4"]]],[[["5"],["6"]],[["7"],["8"]]]]}
    size_t sizeOfBytes = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_FALSE);
    ck_assert_int_eq(sizeOfBytes, 70);
    
    status s = UA_encodeJson((void *) &src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Body\":[[[[\"1\"],[\"2\"]],[[\"3\"],[\"4\"]]],[[[\"5\"],[\"6\"]],[[\"7\"],[\"8\"]]]]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_deleteMembers(&src);
}
END_TEST

START_TEST(UA_Variant_Matrix_NodeId_NonReversible_json_encode) {
    UA_Variant src;
    UA_Variant_init(&src);
    UA_NodeId d[] = {UA_NODEID_NUMERIC(1,1),UA_NODEID_NUMERIC(1,2),UA_NODEID_NUMERIC(1,3),UA_NODEID_NUMERIC(1,4),UA_NODEID_NUMERIC(1,5),UA_NODEID_NUMERIC(1,6),UA_NODEID_NUMERIC(1,7),UA_NODEID_NUMERIC(1,8)};
    UA_Variant_setArrayCopy(&src, d, 8, &UA_TYPES[UA_TYPES_NODEID]);

    src.arrayDimensions = (UA_UInt32 *)UA_Array_new(4, &UA_TYPES[UA_TYPES_UINT32]);
    src.arrayDimensionsSize = 4;
    src.arrayDimensions[0] = 2;
    src.arrayDimensions[1] = 2;
    src.arrayDimensions[2] = 2;
    src.arrayDimensions[3] = 1;
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) &src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Body\":[[[[{\"Id\":1,\"Namespace\":1}],[{\"Id\":2,\"Namespace\":1}]],[[{\"Id\":3,\"Namespace\":1}],[{\"Id\":4,\"Namespace\":1}]]],[[[{\"Id\":5,\"Namespace\":1}],[{\"Id\":6,\"Namespace\":1}]],[[{\"Id\":7,\"Namespace\":1}],[{\"Id\":8,\"Namespace\":1}]]]]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_deleteMembers(&src);
}
END_TEST



START_TEST(UA_Variant_Wrap_json_encode) {
    UA_Variant *src = UA_Variant_new();
    UA_Variant_init(src);
    UA_ViewDescription variantContent;
    UA_DateTime srvts = UA_DateTime_fromUnixTime(1234567);
    variantContent.timestamp = srvts;
    variantContent.viewVersion = 1236;
    variantContent.viewId = UA_NODEID_NUMERIC(0,99999);
    
    UA_Variant_setScalarCopy(src, &variantContent, &UA_TYPES[UA_TYPES_VIEWDESCRIPTION]);

    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":22,\"Body\":{\"TypeId\":{\"Id\":511},\"Body\":{\"ViewId\":{\"Id\":99999},\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"ViewVersion\":1236}}}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_delete(src);
}
END_TEST

START_TEST(UA_Variant_Wrap_Array_json_encode) {
    UA_Variant src;
    UA_Variant_init(&src);
    //src.arrayDimensions = (UA_UInt32 *)UA_Array_new(1, &UA_TYPES[UA_TYPES_UINT32]);
    //src.arrayDimensionsSize = 1;
    //src.arrayDimensions[0] = 2;
    
    UA_ViewDescription variantContent1;
    UA_DateTime srvts1 = UA_DateTime_fromUnixTime(1234567);
    variantContent1.timestamp = srvts1;
    variantContent1.viewVersion = 1;
    variantContent1.viewId = UA_NODEID_NUMERIC(0,1);
    
    UA_ViewDescription variantContent2;
    UA_DateTime srvts2 = UA_DateTime_fromUnixTime(1234567);
    variantContent2.timestamp = srvts2;
    variantContent2.viewVersion = 2;
    variantContent2.viewId = UA_NODEID_NUMERIC(0,2);
    
    
    UA_ViewDescription d[2] = {variantContent1, variantContent2};
    UA_Variant_setArrayCopy(&src, d, 2, &UA_TYPES[UA_TYPES_VIEWDESCRIPTION]);


    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Type\":22,\"Body\":[{\"TypeId\":{\"Id\":511},\"Body\":{\"ViewId\":{\"Id\":1},\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"ViewVersion\":1}},{\"TypeId\":{\"Id\":511},\"Body\":{\"ViewId\":{\"Id\":2},\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"ViewVersion\":2}}]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_deleteMembers(&src);
}
END_TEST

START_TEST(UA_Variant_Wrap_Array_NonReversible_json_encode) {
    UA_Variant src;
    UA_Variant_init(&src);
    //src.arrayDimensions = (UA_UInt32 *)UA_Array_new(1, &UA_TYPES[UA_TYPES_UINT32]);
    //src.arrayDimensionsSize = 1;
    //src.arrayDimensions[0] = 2;
    
    UA_ViewDescription variantContent1;
    UA_DateTime srvts1 = UA_DateTime_fromUnixTime(1234567);
    variantContent1.timestamp = srvts1;
    variantContent1.viewVersion = 1;
    variantContent1.viewId = UA_NODEID_NUMERIC(1,1);
    
    UA_ViewDescription variantContent2;
    UA_DateTime srvts2 = UA_DateTime_fromUnixTime(1234567);
    variantContent2.timestamp = srvts2;
    variantContent2.viewVersion = 2;
    variantContent2.viewId = UA_NODEID_NUMERIC(1,2);
    
    
    UA_ViewDescription d[2] = {variantContent1, variantContent2};
    UA_Variant_setArrayCopy(&src, d, 2, &UA_TYPES[UA_TYPES_VIEWDESCRIPTION]);


    const UA_DataType *type = &UA_TYPES[UA_TYPES_VARIANT];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson(&src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Body\":[{\"Body\":{\"ViewId\":{\"Id\":1,\"Namespace\":1},\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"ViewVersion\":1}},{\"Body\":{\"ViewId\":{\"Id\":2,\"Namespace\":1},\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"ViewVersion\":2}}]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_Variant_deleteMembers(&src);
}
END_TEST

/* -----------ExtensionObject------------------*/
START_TEST(UA_ExtensionObject_json_encode) {
    UA_ExtensionObject *src = UA_ExtensionObject_new();
    UA_ExtensionObject_init(src);
    src->encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    src->content.decoded.type = &UA_TYPES[UA_TYPES_BOOLEAN];

    UA_Boolean b = UA_FALSE;
    src->content.decoded.data = &b;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"TypeId\":{\"Id\":1},\"Body\":false}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExtensionObject_delete(src);
}
END_TEST

START_TEST(UA_ExtensionObject_xml_json_encode) {
    UA_ExtensionObject *src = UA_ExtensionObject_new();
    UA_ExtensionObject_init(src);
    src->encoding = UA_EXTENSIONOBJECT_ENCODED_XML;
    src->content.encoded.typeId = UA_NODEID_NUMERIC(2,1234);

    UA_ByteString b = UA_BYTESTRING_ALLOC("<Elemement></Element>");
    src->content.encoded.body = b;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"TypeId\":{\"Id\":1234,\"Namespace\":2},\"Encoding\":2,\"Body\":\"<Elemement></Element>\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExtensionObject_delete(src);
}
END_TEST


START_TEST(UA_ExtensionObject_byteString_json_encode) {
    UA_ExtensionObject *src = UA_ExtensionObject_new();
    UA_ExtensionObject_init(src);
    src->encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING;
    src->content.encoded.typeId = UA_NODEID_NUMERIC(2,1234);

    UA_ByteString b = UA_BYTESTRING_ALLOC("123456789012345678901234567890");
    src->content.encoded.body = b;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"TypeId\":{\"Id\":1234,\"Namespace\":2},\"Encoding\":1,\"Body\":\"123456789012345678901234567890\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExtensionObject_delete(src);
}
END_TEST


START_TEST(UA_ExtensionObject_NonReversible_StatusCode_json_encode) {
    UA_ExtensionObject *src = UA_ExtensionObject_new();
    UA_ExtensionObject_init(src);
    src->encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    src->content.decoded.type = &UA_TYPES[UA_TYPES_STATUSCODE];

    UA_StatusCode b = UA_STATUSCODE_BADENCODINGERROR;
    src->content.decoded.data = &b;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Body\":{\"Code\":2147876864,\"Symbol\":\"BadEncodingError\"}}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExtensionObject_delete(src);
}
END_TEST



/* --------------ExpandedNodeId-------------------------- */
START_TEST(UA_ExpandedNodeId_json_encode) {
    UA_ExpandedNodeId *src = UA_ExpandedNodeId_new();
    UA_ExpandedNodeId_init(src);
    *src = UA_EXPANDEDNODEID_STRING_ALLOC(23, "testtestTest");
    src->namespaceUri = UA_STRING_ALLOC("asdf");
    src->serverIndex = 1345;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXPANDEDNODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":1,\"Id\":\"testtestTest\",\"Namespace\":\"asdf\",\"ServerUri\":1345}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST


START_TEST(UA_ExpandedNodeId_MissingNamespaceUri_json_encode) {
    UA_ExpandedNodeId *src = UA_ExpandedNodeId_new();
    UA_ExpandedNodeId_init(src);
    *src = UA_EXPANDEDNODEID_STRING_ALLOC(23, "testtestTest");
    src->namespaceUri = UA_STRING_NULL;
    src->serverIndex = 1345;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXPANDEDNODEID];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":1,\"Id\":\"testtestTest\",\"Namespace\":23,\"ServerUri\":1345}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST

START_TEST(UA_ExpandedNodeId_NonReversible_Ns1_json_encode) {
    UA_ExpandedNodeId *src = UA_ExpandedNodeId_new();
    UA_ExpandedNodeId_init(src);
    *src = UA_EXPANDEDNODEID_STRING_ALLOC(1, "testtestTest");
    src->namespaceUri = UA_STRING_NULL;
    src->serverIndex = 1;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXPANDEDNODEID];
    
    UA_String serverUris[3] = {UA_STRING("uri0"),UA_STRING("uri1"),UA_STRING("uri2")};
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, serverUris, 3, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    
    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, serverUris, 3, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":1,\"Id\":\"testtestTest\",\"Namespace\":1,\"ServerUri\":\"uri1\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST

START_TEST(UA_ExpandedNodeId_NonReversible_Namespace_json_encode) {
    UA_ExpandedNodeId *src = UA_ExpandedNodeId_new();
    UA_ExpandedNodeId_init(src);
    *src = UA_EXPANDEDNODEID_STRING_ALLOC(2, "testtestTest");
    src->namespaceUri = UA_STRING_NULL;
    src->serverIndex = 1;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXPANDEDNODEID];
    
    UA_String namespaces[3] = {UA_STRING("ns0"),UA_STRING("ns1"),UA_STRING("ns2")};
    UA_String serverUris[3] = {UA_STRING("uri0"),UA_STRING("uri1"),UA_STRING("uri2")};
    
    size_t size = UA_calcSizeJson((void *) src, type, namespaces, 3, serverUris, 3, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];


    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, namespaces, 3, serverUris, 3, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":1,\"Id\":\"testtestTest\",\"Namespace\":\"ns2\",\"ServerUri\":\"uri1\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST

START_TEST(UA_ExpandedNodeId_NonReversible_NamespaceUriGiven_json_encode) {
    UA_ExpandedNodeId *src = UA_ExpandedNodeId_new();
    UA_ExpandedNodeId_init(src);
    *src = UA_EXPANDEDNODEID_STRING_ALLOC(2, "testtestTest");
    src->namespaceUri = UA_STRING_ALLOC("NamespaceUri");
    src->serverIndex = 1;
    
    UA_String namespaces[3] = {UA_STRING("ns0"),UA_STRING("ns1"),UA_STRING("ns2")};
    UA_String serverUris[3] = {UA_STRING("uri0"),UA_STRING("uri1"),UA_STRING("uri2")};
    
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_EXPANDEDNODEID];
    size_t size = UA_calcSizeJson((void *) src, type, namespaces, 3, serverUris, 3, UA_FALSE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, namespaces, 3, serverUris, 3, UA_FALSE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"IdType\":1,\"Id\":\"testtestTest\",\"Namespace\":\"NamespaceUri\",\"ServerUri\":\"uri1\"}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ExpandedNodeId_delete(src);
}
END_TEST

/* -------------------------DataValue------------------------ */
START_TEST(UA_DataValue_json_encode) {
    UA_DataValue *src = UA_DataValue_new();
    UA_DataValue_init(src);
    src->hasServerPicoseconds = UA_TRUE;
    src->hasServerTimestamp = UA_TRUE;
    src->hasSourcePicoseconds = UA_TRUE;
    src->hasSourceTimestamp = UA_TRUE;
    src->hasStatus = UA_TRUE;
    src->hasValue = UA_TRUE;

    UA_DateTime srcts = UA_DateTime_fromUnixTime(1234567);
    UA_DateTime srvts = UA_DateTime_fromUnixTime(1234567);

    src->sourceTimestamp = srcts;
    src->serverTimestamp = srvts;
    src->sourcePicoseconds = 0;
    src->serverPicoseconds = 0;
    
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Boolean variantContent = true;
    UA_Variant_setScalarCopy(&variant, &variantContent, &UA_TYPES[UA_TYPES_BOOLEAN]);
    src->value = variant;

    src->status = UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DATAVALUE];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"Value\":{\"Type\":1,\"Body\":true},\"Status\":2153250816,\"SourceTimestamp\":\"1970-01-15T06:56:07.000Z\",\"SourcePicoseconds\":0,\"ServerTimestamp\":\"1970-01-15T06:56:07.000Z\",\"ServerPicoseconds\":0}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_DataValue_delete(src);
}
END_TEST

START_TEST(UA_DataValue_null_json_encode) {
    UA_DataValue *src = UA_DataValue_new();
    UA_DataValue_init(src);
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_DATAVALUE];
    size_t size = UA_calcSizeJson((void *) src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "null";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_DataValue_delete(src);
}
END_TEST

START_TEST(UA_MessageReadResponse_json_encode) {
    UA_ReadResponse src;
    UA_ReadResponse_init(&src);
    UA_DiagnosticInfo innerDiag;
    innerDiag.hasAdditionalInfo = UA_TRUE;
    innerDiag.additionalInfo = UA_STRING_ALLOC("INNER ADDITION INFO");
    innerDiag.hasInnerDiagnosticInfo = UA_FALSE;
    innerDiag.hasInnerStatusCode = UA_FALSE;
    innerDiag.hasLocale = UA_FALSE;
    innerDiag.hasSymbolicId = UA_FALSE;
    innerDiag.hasLocalizedText = UA_FALSE;
    innerDiag.hasNamespaceUri = UA_FALSE;
    
    UA_DiagnosticInfo *info = (UA_DiagnosticInfo*)UA_calloc(1, sizeof(UA_DiagnosticInfo));
    info[0] = innerDiag;
    src.diagnosticInfos = info;
    src.diagnosticInfosSize = 1;
    
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasServerPicoseconds = UA_TRUE;
    dv.hasServerTimestamp = UA_TRUE;
    dv.hasSourcePicoseconds = UA_TRUE;
    dv.hasSourceTimestamp = UA_TRUE;
    dv.hasStatus = UA_TRUE;
    dv.hasValue = UA_TRUE;

    UA_DateTime srcts = UA_DateTime_fromUnixTime(1234567);
    UA_DateTime srvts = UA_DateTime_fromUnixTime(1234567);

    dv.sourceTimestamp = srcts;
    dv.serverTimestamp = srvts;
    dv.sourcePicoseconds = 0;
    dv.serverPicoseconds = 0;
    
    
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Boolean variantContent = true;
    UA_Variant_setScalarCopy(&variant, &variantContent, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dv.value = variant;

    dv.status = UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;
    
    UA_DataValue *values = (UA_DataValue*)UA_calloc(1, sizeof(UA_DataValue));
    values[0] = dv;
    src.results = values;
    src.resultsSize = 1;
    
    
    UA_ResponseHeader rh;
    UA_ResponseHeader_init(&rh);
    rh.stringTableSize = 0;
    rh.requestHandle = 123123;
    rh.serviceResult = UA_STATUSCODE_GOOD;
    rh.timestamp = UA_DateTime_fromUnixTime(1234567);
    
    
    UA_DiagnosticInfo serverDiag;
    UA_DiagnosticInfo_init(&serverDiag);
    serverDiag.hasAdditionalInfo = UA_TRUE;
    serverDiag.additionalInfo = UA_STRING_ALLOC("serverDiag");
    serverDiag.hasInnerDiagnosticInfo = UA_FALSE;
    serverDiag.hasInnerStatusCode = UA_FALSE;
    serverDiag.hasLocale = UA_FALSE;
    serverDiag.hasSymbolicId = UA_FALSE;
    serverDiag.hasLocalizedText = UA_FALSE;
    serverDiag.hasNamespaceUri = UA_FALSE;
    rh.serviceDiagnostics = serverDiag;
    
    
    UA_ExtensionObject e;
    UA_ExtensionObject_init(&e);
    e.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    e.content.decoded.type = &UA_TYPES[UA_TYPES_BOOLEAN];

    UA_Boolean b = UA_FALSE;
    e.content.decoded.data = &b;
    
    rh.additionalHeader = e;
            
    src.responseHeader = rh;
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_READRESPONSE];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) &src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"ResponseHeader\":{\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"RequestHandle\":123123,\"ServiceResult\":0,\"ServiceDiagnostics\":{\"AdditionalInfo\":\"serverDiag\"},\"StringTable\":[],\"AdditionalHeader\":{\"TypeId\":{\"Id\":1},\"Body\":false}},\"Results\":[{\"Value\":{\"Type\":1,\"Body\":true},\"Status\":2153250816,\"SourceTimestamp\":\"1970-01-15T06:56:07.000Z\",\"SourcePicoseconds\":0,\"ServerTimestamp\":\"1970-01-15T06:56:07.000Z\",\"ServerPicoseconds\":0}],\"DiagnosticInfos\":[{\"AdditionalInfo\":\"INNER ADDITION INFO\"}]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ReadResponse_deleteMembers(&src); //TODO
}
END_TEST

START_TEST(UA_ViewDescription_json_encode) {
    UA_ViewDescription src;
    UA_ViewDescription_init(&src);
    UA_DateTime srvts = UA_DateTime_fromUnixTime(1234567);
    src.timestamp = srvts;
    src.viewVersion = 1236;
    src.viewId = UA_NODEID_NUMERIC(0,99999);
    
    const UA_DataType *type = &UA_TYPES[UA_TYPES_VIEWDESCRIPTION];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) &src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"ViewId\":{\"Id\":99999},\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"ViewVersion\":1236}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_ViewDescription_deleteMembers(&src);
}
END_TEST

START_TEST(UA_WriteRequest_json_encode) {
    UA_WriteRequest src;
    UA_WriteRequest_init(&src);
    
    UA_RequestHeader rh;
    rh.returnDiagnostics = 1;
    rh.auditEntryId = UA_STRING_ALLOC("Auditentryid");
    rh.requestHandle = 123123;
    rh.authenticationToken = UA_NODEID_STRING_ALLOC(0,"authToken");
    rh.timestamp = UA_DateTime_fromUnixTime(1234567);
    rh.timeoutHint = 120;
    
    UA_ExtensionObject e;
    UA_ExtensionObject_init(&e);
    e.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    e.content.decoded.type = &UA_TYPES[UA_TYPES_BOOLEAN];

    UA_Boolean b = UA_FALSE;
    e.content.decoded.data = &b;
    
    rh.additionalHeader = e;
   
    
    
    UA_DataValue dv;
    UA_DataValue_init(&dv);
    dv.hasServerPicoseconds = UA_TRUE;
    dv.hasServerTimestamp = UA_TRUE;
    dv.hasSourcePicoseconds = UA_TRUE;
    dv.hasSourceTimestamp = UA_TRUE;
    dv.hasStatus = UA_TRUE;
    dv.hasValue = UA_TRUE;

    UA_DateTime srcts = UA_DateTime_fromUnixTime(1234567);
    UA_DateTime srvts = UA_DateTime_fromUnixTime(1234567);

    dv.sourceTimestamp = srcts;
    dv.serverTimestamp = srvts;
    dv.sourcePicoseconds = 0;
    dv.serverPicoseconds = 0;
    
    UA_Variant variant;
    UA_Variant_init(&variant);
    UA_Boolean variantContent = true;
    UA_Variant_setScalarCopy(&variant, &variantContent, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dv.value = variant;

    dv.status = UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;
    
    UA_DataValue dv2;
    UA_DataValue_init(&dv2);
    dv2.hasServerPicoseconds = UA_TRUE;
    dv2.hasServerTimestamp = UA_TRUE;
    dv2.hasSourcePicoseconds = UA_TRUE;
    dv2.hasSourceTimestamp = UA_TRUE;
    dv2.hasStatus = UA_TRUE;
    dv2.hasValue = UA_TRUE;

    UA_DateTime srcts2 = UA_DateTime_fromUnixTime(1234567);
    UA_DateTime srvts2 = UA_DateTime_fromUnixTime(1234567);

    dv2.sourceTimestamp = srcts2;
    dv2.serverTimestamp = srvts2;
    dv2.sourcePicoseconds = 0;
    dv2.serverPicoseconds = 0;
    
    UA_Variant variant2;
    UA_Variant_init(&variant2);
    UA_Boolean variantContent2 = true;
    UA_Variant_setScalarCopy(&variant2, &variantContent2, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dv2.value = variant2;

    dv2.status = UA_STATUSCODE_BADAPPLICATIONSIGNATUREINVALID;
    
    UA_WriteValue value;
    UA_WriteValue_init(&value);
    value.value = dv;
    value.attributeId = 12;
    value.indexRange = UA_STRING_ALLOC("BLOAB");
    value.nodeId = UA_NODEID_STRING_ALLOC(0, "a1111");
    
    UA_WriteValue value2;
    UA_WriteValue_init(&value2);
    value2.value = dv2;
    value2.attributeId = 12;
    value2.indexRange = UA_STRING_ALLOC("BLOAB");
    value2.nodeId = UA_NODEID_STRING_ALLOC(0, "a2222");
    
    UA_WriteValue *values = (UA_WriteValue*)UA_calloc(2,sizeof(UA_WriteValue));
    values[0] = value;
    values[1] = value2;
    
    src.nodesToWrite = values;
    src.nodesToWriteSize = 2;
    src.requestHeader = rh;

    const UA_DataType *type = &UA_TYPES[UA_TYPES_WRITEREQUEST];
    size_t size = UA_calcSizeJson((void *) &src, type, NULL, 0, NULL, 0, UA_TRUE);
    UA_ByteString buf;

    UA_ByteString_allocBuffer(&buf, size+1);

    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[size+1];

    status s = UA_encodeJson((void *) &src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);

    *bufPos = 0;
    // then
    ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
    char* result = "{\"RequestHeader\":{\"AuthenticationToken\":{\"IdType\":1,\"Id\":\"authToken\"},\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"RequestHandle\":123123,\"ReturnDiagnostics\":1,\"AuditEntryId\":\"Auditentryid\",\"TimeoutHint\":120,\"AdditionalHeader\":{\"TypeId\":{\"Id\":1},\"Body\":false}},\"NodesToWrite\":[{\"NodeId\":{\"IdType\":1,\"Id\":\"a1111\"},\"AttributeId\":12,\"IndexRange\":\"BLOAB\",\"Value\":{\"Value\":{\"Type\":1,\"Body\":true},\"Status\":2153250816,\"SourceTimestamp\":\"1970-01-15T06:56:07.000Z\",\"SourcePicoseconds\":0,\"ServerTimestamp\":\"1970-01-15T06:56:07.000Z\",\"ServerPicoseconds\":0}},{\"NodeId\":{\"IdType\":1,\"Id\":\"a2222\"},\"AttributeId\":12,\"IndexRange\":\"BLOAB\",\"Value\":{\"Value\":{\"Type\":1,\"Body\":true},\"Status\":2153250816,\"SourceTimestamp\":\"1970-01-15T06:56:07.000Z\",\"SourcePicoseconds\":0,\"ServerTimestamp\":\"1970-01-15T06:56:07.000Z\",\"ServerPicoseconds\":0}}]}";
    ck_assert_str_eq(result, (char*)buf.data);
    UA_ByteString_deleteMembers(&buf);
    UA_WriteRequest_deleteMembers(&src);
}
END_TEST

START_TEST(UA_null_json_encode) {
    UA_ByteString buf;
    UA_ByteString_allocBuffer(&buf, 100);
    UA_Byte *bufPos = &buf.data[0];
    const UA_Byte *bufEnd = &buf.data[100];

    void* src = NULL;
    char* result = "null";
    
    const UA_DataType *type;
    status s;

    int i;
    for (i = 1; i < 25; i++) {
        bufPos = &buf.data[0];
        src = NULL;
        type = &UA_TYPES[i];
        s = UA_encodeJson((void *) src, type, &bufPos, &bufEnd, NULL, 0, NULL, 0, UA_TRUE);
        *bufPos = 0;
        ck_assert_int_eq(s, UA_STATUSCODE_GOOD);
        ck_assert_str_eq(result, (char*)buf.data);
    }

    UA_ByteString_deleteMembers(&buf);
}
END_TEST




// ---------------------------DECODE-------------------------------------

START_TEST(UA_Byte_Min_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":3,\"Body\":0}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_BYTE);
    ck_assert_uint_eq(*((UA_Byte*)out.data), 0);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Byte_Max_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":3,\"Body\":255}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_BYTE);
    ck_assert_uint_eq(*((UA_Byte*)out.data), 255);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


/* ----UInt16---- */
START_TEST(UA_UInt16_Min_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":5,\"Body\":0}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_UINT16);
    ck_assert_uint_eq(*((UA_UInt16*)out.data), 0);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_UInt16_Max_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":5,\"Body\":65535}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_UINT16);
    ck_assert_uint_eq(*((UA_UInt16*)out.data), 65535);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_UInt32_Min_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":7,\"Body\":0}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_UINT32);
    ck_assert_uint_eq(*((UA_UInt32*)out.data), 0);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_UInt32_Max_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":7,\"Body\":4294967295}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_UINT32);
    ck_assert_uint_eq(*((UA_UInt32*)out.data), 4294967295);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_UInt64_Min_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":9,\"Body\":0}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_UINT64);    
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x00);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_UInt64_Max_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":9,\"Body\":18446744073709551615}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_UINT64);
    ck_assert_int_eq(((u8*)(out.data))[0], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[1], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[2], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[3], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[4], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[5], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[6], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[7], 0xFF);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_SByte_Min_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":2,\"Body\":-128}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_SBYTE);
    ck_assert_uint_eq(*((UA_SByte*)out.data), -128);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_SByte_Max_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":2,\"Body\":127}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_SBYTE);
    ck_assert_uint_eq(*((UA_SByte*)out.data), 127);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_Int16_Min_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":4,\"Body\":-32768}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_INT16);
    ck_assert_uint_eq(*((UA_Int16*)out.data), -32768);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Int16_Max_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":4,\"Body\":32767}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_INT16);
    ck_assert_uint_eq(*((UA_Int16*)out.data), 32767);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_Int32_Min_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":6,\"Body\":-2147483648}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_INT32);
    ck_assert_uint_eq(*((UA_Int32*)out.data), -2147483648);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Int32_Max_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":6,\"Body\":2147483647}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_INT32);
    ck_assert_uint_eq(*((UA_Int32*)out.data), 2147483647);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_Int64_Min_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":8,\"Body\":-9223372036854775808}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_INT64);    
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x80);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_Int64_Max_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":8,\"Body\":9223372036854775807}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_INT64);    
    ck_assert_int_eq(((u8*)(out.data))[0], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[1], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[2], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[3], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[4], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[5], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[6], 0xFF);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x7F);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_Float_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":10,\"Body\":3.1415927410}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    //0 10000000 10010010000111111011011 = 40 49 0f db
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)(out.data))[0], 0xdb);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x0f);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x49);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x40);
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Float_json_one_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":10,\"Body\":1}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    //0 01111111 00000000000000000000000 = 3f80 0000
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x80);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x3f);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Double_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":1.1234}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);   
    ck_assert_int_eq(((u8*)(out.data))[0], 0xef);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x38);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x45);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x47);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x72);
    ck_assert_int_eq(((u8*)(out.data))[5], 0xf9);
    ck_assert_int_eq(((u8*)(out.data))[6], 0xf1);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x3f);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Double_one_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":1}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    // 0 01111111111 0000000000000000000000000000000000000000000000000000
    // 3FF0000000000000
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);       
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0xF0);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x3F);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_Double_onepointsmallest_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":1.0000000000000002}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    // 0 01111111111 0000000000000000000000000000000000000000000000000001
    // 3FF0000000000001
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);       
    ck_assert_int_eq(((u8*)(out.data))[0], 0x01);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0xF0);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x3F);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Double_nan_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":\"NaN\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    // 0 11111111111 1000000000000000000000000000000000000000000000000000
    // 7FF8000000000000
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);       
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0xF8);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x7F);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Double_inf_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":\"Infinity\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    // 0 111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);       
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0xF0);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x7F);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Double_neginf_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":\"-Infinity\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    // 1 111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);       
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0xF0);
    ck_assert_int_eq(((u8*)(out.data))[7], 0xFF);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Double_zero_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":0}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    // 0 000 0000 0000 0000 0000 0000 00000000 00000000 00000000 00000000 00000000
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);       
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x00);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Double_negzero_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":-0}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    // 1 000 0000 0000 0000 00000000 00000000 00000000 00000000 00000000 00000000
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);       
    ck_assert_int_eq(((u8*)(out.data))[0], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[1], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[2], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[3], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[4], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[5], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[6], 0x00);
    ck_assert_int_eq(((u8*)(out.data))[7], 0x80);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_String_json_decode) { 
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":\"abcdef\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_STRING);
    ck_assert_int_eq(  ((UA_String*)out.data)->length, 6);
    ck_assert_int_eq( ((UA_String*)out.data)->data[0], 'a');
    ck_assert_int_eq(((UA_String*)out.data)->data[1], 'b');
    ck_assert_int_eq(((UA_String*)out.data)->data[2], 'c');
    ck_assert_int_eq(((UA_String*)out.data)->data[3], 'd');
    ck_assert_int_eq(((UA_String*)out.data)->data[4], 'e');
    ck_assert_int_eq(((UA_String*)out.data)->data[5], 'f');
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_String_unescapeBS_json_decode) { 
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":\"ab\\tcdef\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_STRING);
    ck_assert_int_eq(  ((UA_String*)out.data)->length, 7);
    ck_assert_int_eq( ((UA_String*)out.data)->data[0], 'a');
    ck_assert_int_eq(((UA_String*)out.data)->data[1], 'b');
    ck_assert_int_eq(((UA_String*)out.data)->data[2], '\t');
    ck_assert_int_eq(((UA_String*)out.data)->data[3], 'c');
    ck_assert_int_eq(((UA_String*)out.data)->data[4], 'd');
    ck_assert_int_eq(((UA_String*)out.data)->data[5], 'e');
    ck_assert_int_eq(((UA_String*)out.data)->data[6], 'f');
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_String_escape_unicode_json_decode) { 
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":\"\\u002c\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_STRING);
    ck_assert_int_eq(  ((UA_String*)out.data)->length, 1);
    ck_assert_int_eq( ((UA_String*)out.data)->data[0], ',');
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_String_escape2_json_decode) { 
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":\"\\b\\th\\\"e\\fl\\nl\\\\o\\r\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_STRING);
    ck_assert_int_eq(  ((UA_String*)out.data)->length, 12);//  \b\th\"e\fl\nl\\o\r
    ck_assert_int_eq( ((UA_String*)out.data)->data[0], '\b');
    ck_assert_int_eq( ((UA_String*)out.data)->data[1], '\t');
    ck_assert_int_eq( ((UA_String*)out.data)->data[2], 'h');
    ck_assert_int_eq( ((UA_String*)out.data)->data[3], '\"');
    ck_assert_int_eq( ((UA_String*)out.data)->data[4], 'e');
    ck_assert_int_eq( ((UA_String*)out.data)->data[5], '\f');
    ck_assert_int_eq( ((UA_String*)out.data)->data[6], 'l');
    ck_assert_int_eq( ((UA_String*)out.data)->data[7], '\n');
    ck_assert_int_eq( ((UA_String*)out.data)->data[8], 'l');
    ck_assert_int_eq( ((UA_String*)out.data)->data[9], '\\');
    ck_assert_int_eq( ((UA_String*)out.data)->data[10], 'o');
    ck_assert_int_eq( ((UA_String*)out.data)->data[11], '\r');
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_String_surrogatePair_json_decode) { 
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":\"\\uD800\\uDC00\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_STRING);
    ck_assert_int_eq(  ((UA_String*)out.data)->length, 4);//U+10000  => 0xF0 0x90 0x80 0x80
    ck_assert_uint_eq( ((UA_String*)out.data)->data[0], 0xF0);
    ck_assert_uint_eq( ((UA_String*)out.data)->data[1], 0x90);
    ck_assert_uint_eq( ((UA_String*)out.data)->data[2], 0x80);
    ck_assert_uint_eq( ((UA_String*)out.data)->data[3], 0x80);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

/* ---------------ByteString---------------------------*/
START_TEST(UA_ByteString_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":15,\"Body\":\"YXNkZmFzZGY=\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_BYTESTRING);
    ck_assert_int_eq(((UA_ByteString*)out.data)->length, 8);
    ck_assert_int_eq(((UA_ByteString*)out.data)->data[0], 'a');
    ck_assert_int_eq(((UA_ByteString*)out.data)->data[1], 's');
    ck_assert_int_eq(((UA_ByteString*)out.data)->data[2], 'd');
    ck_assert_int_eq(((UA_ByteString*)out.data)->data[3], 'f');
    ck_assert_int_eq(((UA_ByteString*)out.data)->data[4], 'a');
    ck_assert_int_eq(((UA_ByteString*)out.data)->data[5], 's');
    ck_assert_int_eq(((UA_ByteString*)out.data)->data[6], 'd');
    ck_assert_int_eq(((UA_ByteString*)out.data)->data[7], 'f');
    
    UA_Variant_deleteMembers(&out);
}
END_TEST



START_TEST(UA_ByteString_bad_json_decode) {
    UA_ByteString out;
    UA_ByteString_init(&out);
    UA_ByteString buf = UA_STRING("\"\x90!\xc5 c{\",");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_BYTESTRING]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_ByteString_deleteMembers(&out);
}
END_TEST

START_TEST(UA_ByteString_null_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":15,\"Body\":null}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_BYTESTRING);
    ck_assert_ptr_eq(out.data, NULL);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


/* ---------GUID---------------------------  */
START_TEST(UA_Guid_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":14,\"Body\":\"00000001-0002-0003-0405-060708090A0B\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_GUID);
    ck_assert_int_eq(((UA_Guid*)out.data)->data1, 1);
    ck_assert_int_eq(((UA_Guid*)out.data)->data2, 2);
    ck_assert_int_eq(((UA_Guid*)out.data)->data3, 3);
    ck_assert_int_eq(((UA_Guid*)out.data)->data4[0], 4);
    ck_assert_int_eq(((UA_Guid*)out.data)->data4[1], 5);
    ck_assert_int_eq(((UA_Guid*)out.data)->data4[2], 6);
    ck_assert_int_eq(((UA_Guid*)out.data)->data4[3], 7);
    ck_assert_int_eq(((UA_Guid*)out.data)->data4[4], 8);
    ck_assert_int_eq(((UA_Guid*)out.data)->data4[5], 9);
    ck_assert_int_eq(((UA_Guid*)out.data)->data4[6], 10);
    ck_assert_int_eq(((UA_Guid*)out.data)->data4[7], 11);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_Guid_tooShort_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":14,\"Body\":\"00000001-00\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Guid_tooLong_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":14,\"Body\":\"00000001-0002-0003-0405-060708090A0B00000001\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Guid_wrong_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":14,\"Body\":\"00000=01-0002-0003-0405-060708090A0B\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);

    UA_Variant_deleteMembers(&out);
}
END_TEST

/* ------Statuscode----------- */
START_TEST(UA_StatusCode_2_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":19,\"Body\":2}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_STATUSCODE);
    ck_assert_uint_eq(*((UA_StatusCode*)out.data), 2);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_StatusCode_0_json_decode) {
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":19,\"Body\":0}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_STATUSCODE);
    ck_assert_uint_eq(*((UA_StatusCode*)out.data), 0);
    UA_Variant_deleteMembers(&out);
}
END_TEST


/* ----------DateTime---------------- */
START_TEST(UA_DateTime_json_decode) {
    // given
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_ByteString buf = UA_STRING("\"1970-01-02T01:02:03.005Z\"");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME]);
    // then
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
    
    UA_DateTime_deleteMembers(&out);
}
END_TEST

START_TEST(UA_DateTime_micro_json_decode) {
    // given
    UA_DateTime out;
    UA_DateTime_init(&out);
    UA_ByteString buf = UA_STRING("\"1970-01-02T01:02:03.042Z\"");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_DATETIME]);
    // then
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
    
    UA_DateTime_deleteMembers(&out);
}
END_TEST


/* ---------------QualifiedName----------------------- */
START_TEST(UA_QualifiedName_json_decode) {
    // given
    UA_QualifiedName out;
    UA_QualifiedName_init(&out);
    UA_ByteString buf = UA_STRING("{\"Name\":\"derName\",\"Uri\":1}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.name.length, 7);
    ck_assert_int_eq(out.name.data[1], 'e');
    ck_assert_int_eq(out.name.data[6], 'e');
    ck_assert_int_eq(out.namespaceIndex, 1);
    
    UA_QualifiedName_deleteMembers(&out);
}
END_TEST


START_TEST(UA_QualifiedName_null_json_decode) {
    
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":20,\"Body\":null}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_QUALIFIEDNAME);
    ck_assert_ptr_eq(out.data, NULL);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


/* --------LocalizedText------------ */
START_TEST(UA_LocalizedText_json_decode) {
    // given
    UA_LocalizedText out;
    UA_LocalizedText_init(&out);
    UA_ByteString buf = UA_STRING("{\"Locale\":\"t1\",\"Text\":\"t2\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.locale.data[0], 't');
    ck_assert_int_eq(out.text.data[0], 't');
    ck_assert_int_eq(out.locale.data[1], '1');
    ck_assert_int_eq(out.text.data[1], '2');
    
    UA_LocalizedText_deleteMembers(&out);
}
END_TEST

START_TEST(UA_LocalizedText_missing_json_decode) {
    // given
    UA_LocalizedText out;
    UA_LocalizedText_init(&out);
    UA_ByteString buf = UA_STRING("{\"Locale\":\"t1\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.locale.length, 2);
    ck_assert_int_eq(out.locale.data[0], 't');
    ck_assert_int_eq(out.locale.data[1], '1');
    ck_assert_ptr_eq(out.text.data, NULL);
    ck_assert_int_eq(out.text.length, 0);
    
    UA_LocalizedText_deleteMembers(&out);
}
END_TEST

START_TEST(UA_LocalizedText_null_json_decode) {
    
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":21,\"Body\":null}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_LOCALIZEDTEXT);
    ck_assert_ptr_eq(out.data, NULL);
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_ViewDescription_json_decode) {
    // given
    UA_ViewDescription out;
    UA_ViewDescription_init(&out);
    UA_ByteString buf = UA_STRING("{\"Timestamp\":\"1970-01-15T06:56:07Z\",\"ViewVersion\":1236,\"ViewId\":{\"Id\":\"00000009-0002-027C-F3BF-BB7BEEFEEFBE\",\"IdType\":2}}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VIEWDESCRIPTION]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.viewVersion, 1236);
    ck_assert_int_eq(out.viewId.identifierType, UA_NODEIDTYPE_GUID);
    UA_DateTimeStruct dts = UA_DateTime_toStruct(out.timestamp);
    ck_assert_int_eq(dts.year, 1970);
    ck_assert_int_eq(dts.month, 1);
    ck_assert_int_eq(dts.day, 15);
    ck_assert_int_eq(dts.hour, 6);
    ck_assert_int_eq(dts.min, 56);
    ck_assert_int_eq(dts.sec, 7);
    ck_assert_int_eq(dts.milliSec, 0);
    ck_assert_int_eq(dts.microSec, 0);
    ck_assert_int_eq(dts.nanoSec, 0);
    ck_assert_int_eq(out.viewId.identifier.guid.data1, 9);
    ck_assert_int_eq(out.viewId.identifier.guid.data2, 2);
    
    UA_ViewDescription_deleteMembers(&out);
}
END_TEST



/* -----------------NodeId----------------------------- */
START_TEST(UA_NodeId_Nummeric_json_decode) {
    // given
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"Id\":42}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.identifier.numeric, 42);
    ck_assert_int_eq(out.namespaceIndex, 0);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_NUMERIC);
    
    UA_NodeId_deleteMembers(&out);
}
END_TEST

START_TEST(UA_NodeId_Nummeric_Namespace_json_decode) {
    // given
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"Id\":42,\"Namespace\":123}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.identifier.numeric, 42);
    ck_assert_int_eq(out.namespaceIndex, 123);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_NUMERIC);
    
    UA_NodeId_deleteMembers(&out);
}
END_TEST


START_TEST(UA_NodeId_String_json_decode) {
    // given
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"IdType\":1,\"Id\":\"test123\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.identifier.string.length, 7);
    ck_assert_int_eq(out.identifier.string.data[0], 't');
    ck_assert_int_eq(out.identifier.string.data[1], 'e');
    ck_assert_int_eq(out.identifier.string.data[2], 's');
    ck_assert_int_eq(out.identifier.string.data[3], 't');
    ck_assert_int_eq(out.identifier.string.data[4], '1');
    ck_assert_int_eq(out.identifier.string.data[5], '2');
    ck_assert_int_eq(out.identifier.string.data[6], '3');
    ck_assert_int_eq(out.namespaceIndex, 0);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_STRING);
    
    UA_NodeId_deleteMembers(&out);
}
END_TEST


START_TEST(UA_NodeId_Guid_json_decode) {
    // given
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"IdType\":2,\"Id\":\"00000001-0002-0003-0405-060708090A0B\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_NODEID]);
    // then
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
    
    UA_NodeId_deleteMembers(&out);
}
END_TEST

START_TEST(UA_NodeId_ByteString_json_decode) {
    // given
    UA_NodeId out;
    UA_NodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"IdType\":3,\"Id\":\"YXNkZmFzZGY=\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    ck_assert_int_eq(out.namespaceIndex, 0);
    ck_assert_int_eq(out.identifierType, UA_NODEIDTYPE_BYTESTRING);
    ck_assert_int_eq(out.identifier.byteString.length, 8);
    ck_assert_int_eq(out.identifier.byteString.data[0], 'a');
    ck_assert_int_eq(out.identifier.byteString.data[1], 's');
    ck_assert_int_eq(out.identifier.byteString.data[2], 'd');
    ck_assert_int_eq(out.identifier.byteString.data[3], 'f');
    ck_assert_int_eq(out.identifier.byteString.data[4], 'a');
    ck_assert_int_eq(out.identifier.byteString.data[5], 's');
    ck_assert_int_eq(out.identifier.byteString.data[6], 'd');
    ck_assert_int_eq(out.identifier.byteString.data[7], 'f');
    
    UA_NodeId_deleteMembers(&out);
}
END_TEST


/* -----------------------ExpandedNodeId---------------------------*/
START_TEST(UA_ExpandedNodeId_Nummeric_json_decode) {
    // given
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"Id\":42}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.nodeId.identifier.numeric, 42);
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_ptr_eq(out.namespaceUri.data, NULL);
    ck_assert_int_eq(out.namespaceUri.length, 0);
    ck_assert_int_eq(out.serverIndex, 0);
    
    UA_ExpandedNodeId_deleteMembers(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_String_json_decode) {
    // given
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"IdType\":1,\"Id\":\"test\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.nodeId.identifier.string.length, 4);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 't');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_ptr_eq(out.namespaceUri.data, NULL);
    ck_assert_int_eq(out.namespaceUri.length, 0);
    ck_assert_int_eq(out.serverIndex, 0);
    
    UA_ExpandedNodeId_deleteMembers(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_String_Namespace_json_decode) {
    // given
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"IdType\":1,\"Id\":\"test\",\"Namespace\":\"abcdef\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.nodeId.identifier.string.length, 4);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 't');
    ck_assert_int_eq(out.nodeId.identifier.string.data[1], 'e');
    ck_assert_int_eq(out.nodeId.identifier.string.data[2], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[3], 't');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_int_eq(out.namespaceUri.length, 6);
    ck_assert_int_eq(out.namespaceUri.data[0], 'a');
    ck_assert_int_eq(out.namespaceUri.data[1], 'b');
    ck_assert_int_eq(out.namespaceUri.data[2], 'c');
    ck_assert_int_eq(out.namespaceUri.data[3], 'd');
    ck_assert_int_eq(out.namespaceUri.data[4], 'e');
    ck_assert_int_eq(out.namespaceUri.data[5], 'f');
    ck_assert_int_eq(out.serverIndex, 0);
    
    UA_ExpandedNodeId_deleteMembers(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_String_NamespaceAsIndex_json_decode) {
    // given
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"IdType\":1,\"Id\":\"test\",\"Namespace\":42}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.nodeId.identifier.string.length, 4);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 't');
    ck_assert_int_eq(out.nodeId.identifier.string.data[1], 'e');
    ck_assert_int_eq(out.nodeId.identifier.string.data[2], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[3], 't');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_int_eq(out.namespaceUri.length, 0);
    ck_assert_ptr_eq(out.namespaceUri.data, NULL);
    ck_assert_int_eq(out.nodeId.namespaceIndex, 42);
    ck_assert_int_eq(out.serverIndex, 0);
    
    UA_ExpandedNodeId_deleteMembers(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_String_Namespace_ServerUri_json_decode) {
    // given
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"IdType\":1,\"Id\":\"test\",\"Namespace\":\"test\",\"ServerUri\":13}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.nodeId.identifier.string.length, 4);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 't');
    ck_assert_int_eq(out.nodeId.identifier.string.data[1], 'e');
    ck_assert_int_eq(out.nodeId.identifier.string.data[2], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[3], 't');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_int_eq(out.serverIndex, 13);
    ck_assert_int_eq(out.namespaceUri.data[0], 't');
    ck_assert_int_eq(out.namespaceUri.data[1], 'e');
    ck_assert_int_eq(out.namespaceUri.data[2], 's');
    ck_assert_int_eq(out.namespaceUri.data[3], 't');
    
    UA_ExpandedNodeId_deleteMembers(&out);
}
END_TEST

START_TEST(UA_ExpandedNodeId_ByteString_json_decode) {
    // given
    UA_ExpandedNodeId out;
    UA_ExpandedNodeId_init(&out);
    UA_ByteString buf = UA_STRING("{\"IdType\":3,\"Id\":\"YXNkZmFzZGY=\",\"Namespace\":\"test\",\"ServerUri\":13}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.nodeId.identifier.string.length, 8);
    ck_assert_int_eq(out.nodeId.identifier.string.data[0], 'a');
    ck_assert_int_eq(out.nodeId.identifier.string.data[1], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[2], 'd');
    ck_assert_int_eq(out.nodeId.identifier.string.data[3], 'f');
    ck_assert_int_eq(out.nodeId.identifier.string.data[4], 'a');
    ck_assert_int_eq(out.nodeId.identifier.string.data[5], 's');
    ck_assert_int_eq(out.nodeId.identifier.string.data[6], 'd');
    ck_assert_int_eq(out.nodeId.identifier.string.data[7], 'f');
    ck_assert_int_eq(out.nodeId.identifierType, UA_NODEIDTYPE_BYTESTRING);
    ck_assert_int_eq(out.serverIndex, 13);
    ck_assert_int_eq(out.namespaceUri.data[0], 't');
    ck_assert_int_eq(out.namespaceUri.data[1], 'e');
    ck_assert_int_eq(out.namespaceUri.data[2], 's');
    ck_assert_int_eq(out.namespaceUri.data[3], 't');
    
    UA_ExpandedNodeId_deleteMembers(&out);
}
END_TEST


START_TEST(UA_DiagnosticInfo_json_decode) {
    // given
    
    UA_DiagnosticInfo out;
    UA_DiagnosticInfo_init(&out);
    out.innerDiagnosticInfo = NULL;
    UA_ByteString buf = UA_STRING("{\"SymbolicId\":13,"
            "\"LocalizedText\":14,"
            "\"Locale\":12,"
            "\"AdditionalInfo\":\"additionalInfo\","
            "\"InnerStatusCode\":2155216896,"
            "\"InnerDiagnosticInfo\":{\"AdditionalInfo\":\"INNER ADDITION INFO\"}}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.locale, 12);
    ck_assert_int_eq(out.symbolicId, 13);
    ck_assert_int_eq(out.localizedText, 14);
    ck_assert_int_eq(out.innerStatusCode, 2155216896);
    ck_assert_int_eq(out.additionalInfo.length, 14);
    ck_assert_int_eq(out.innerDiagnosticInfo->additionalInfo.length, 19);
    UA_DiagnosticInfo_deleteMembers(&out);
}
END_TEST

START_TEST(UA_DiagnosticInfo_null_json_decode) {
    
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":25,\"Body\":null}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_DIAGNOSTICINFO);
    ck_assert_ptr_eq(out.data, NULL);
    
    //ck_assert_uint_eq(((UA_DiagnosticInfo*)out.data)->hasAdditionalInfo, 0);
    //ck_assert_uint_eq(((UA_DiagnosticInfo*)out.data)->hasInnerDiagnosticInfo, 0);
    //ck_assert_uint_eq(((UA_DiagnosticInfo*)out.data)->hasInnerStatusCode, 0);
    //ck_assert_uint_eq(((UA_DiagnosticInfo*)out.data)->hasLocale, 0);
    //ck_assert_uint_eq(((UA_DiagnosticInfo*)out.data)->hasLocalizedText, 0);
    //ck_assert_uint_eq(((UA_DiagnosticInfo*)out.data)->hasNamespaceUri, 0);
    //ck_assert_uint_eq(((UA_DiagnosticInfo*)out.data)->hasSymbolicId, 0);
    
    UA_Variant_deleteMembers(&out);
}
END_TEST


/* --------------------DataValue--------------------------- */
START_TEST(UA_DataValue_json_decode) {
    // given
    
    UA_DataValue out;
    UA_DataValue_init(&out);
    UA_ByteString buf = UA_STRING("{\"Value\":{\"Type\":1,\"Body\":true},\"Status\":2153250816,\"SourceTimestamp\":\"1970-01-15T06:56:07Z\",\"SourcePicoseconds\":0,\"ServerTimestamp\":\"1970-01-15T06:56:07Z\",\"ServerPicoseconds\":0}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_DATAVALUE]);
    //UA_DiagnosticInfo inner = *out.innerDiagnosticInfo;

    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.hasStatus, 1);
    ck_assert_int_eq(out.hasServerPicoseconds, 1);
    ck_assert_int_eq(out.hasServerTimestamp, 1);
    ck_assert_int_eq(out.hasSourcePicoseconds, 1);
    ck_assert_int_eq(out.hasSourceTimestamp, 1);
    ck_assert_int_eq(out.hasValue, 1);
    ck_assert_int_eq(out.status, 2153250816);
    ck_assert_int_eq(out.value.type->typeIndex, 0);
    ck_assert_int_eq((*((UA_Boolean*)out.value.data)), 1);
    UA_DataValue_deleteMembers(&out);
}
END_TEST

START_TEST(UA_DataValueMissingFields_json_decode) {
    // given
    
    UA_DataValue out;
    UA_DataValue_init(&out);
    UA_ByteString buf = UA_STRING("{\"Value\":{\"Type\":1,\"Body\":true}}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_DATAVALUE]);
    //UA_DiagnosticInfo inner = *out.innerDiagnosticInfo;

    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.hasStatus, 0);
    ck_assert_int_eq(out.hasServerPicoseconds, 0);
    ck_assert_int_eq(out.hasServerTimestamp, 0);
    ck_assert_int_eq(out.hasSourcePicoseconds, 0);
    ck_assert_int_eq(out.hasSourceTimestamp, 0);
    ck_assert_int_eq(out.hasValue, 1);
    UA_DataValue_deleteMembers(&out);
}
END_TEST

START_TEST(UA_DataValue_null_json_decode) {
    // given
    
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":23,\"Body\":null}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    //UA_DiagnosticInfo inner = *out.innerDiagnosticInfo;

    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(out.data, NULL);
    UA_Variant_deleteMembers(&out);
}
END_TEST


/*----------------------ExtensionObject------------------------*/
START_TEST(UA_ExtensionObject_json_decode) {
    // given
    
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("{\"TypeId\":{\"Id\":1},\"Body\":true}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_DECODED);
    ck_assert_int_eq(*((UA_Boolean*)out.content.decoded.data), UA_TRUE);
    ck_assert_int_eq(out.content.decoded.type->typeIndex, UA_TYPES_BOOLEAN);
    UA_ExtensionObject_deleteMembers(&out);
}
END_TEST

START_TEST(UA_ExtensionObject_EncodedByteString_json_decode) {
    // given
    
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("{\"Encoding\":1,\"TypeId\":{\"Id\":42},\"Body\":\"YXNkZmFzZGY=\"}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_ENCODED_BYTESTRING);
    //TODO: Not base64 decoded, correct?
    ck_assert_int_eq(out.content.encoded.body.data[0], 'Y');
    ck_assert_int_eq(out.content.encoded.body.data[0], 'Y');
    ck_assert_int_eq(out.content.encoded.body.data[1], 'X');
    ck_assert_int_eq(out.content.encoded.body.data[2], 'N');
    ck_assert_int_eq(out.content.encoded.body.data[3], 'k');
    ck_assert_int_eq(out.content.encoded.body.data[4], 'Z');
    ck_assert_int_eq(out.content.encoded.body.data[5], 'm');
    ck_assert_int_eq(out.content.encoded.body.data[6], 'F');
    ck_assert_int_eq(out.content.encoded.typeId.identifier.numeric, 42);
    
    UA_ExtensionObject_deleteMembers(&out);
}
END_TEST


START_TEST(UA_ExtensionObject_EncodedXml_json_decode) {
    // given
    
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("{\"Encoding\":2,\"TypeId\":{\"Id\":42},\"Body\":\"<Element></Element>\"}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_ENCODED_XML);
    ck_assert_int_eq(out.content.encoded.body.data[0], '<');
    ck_assert_int_eq(out.content.encoded.body.data[1], 'E');
    ck_assert_int_eq(out.content.encoded.body.data[2], 'l');
    ck_assert_int_eq(out.content.encoded.body.data[3], 'e');
    ck_assert_int_eq(out.content.encoded.typeId.identifier.numeric, 42);
    UA_ExtensionObject_deleteMembers(&out);
}
END_TEST


START_TEST(UA_ExtensionObject_Unkown_json_decode) {
    // given
    
    UA_ExtensionObject out;
    UA_ExtensionObject_init(&out);
    UA_ByteString buf = UA_STRING("{\"TypeId\":{\"Id\":4711},\"Body\":{\"unknown\":\"body\",\"saveas\":\"Bytestring\"}}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.encoding, UA_EXTENSIONOBJECT_ENCODED_BYTESTRING);
    ck_assert_int_eq(out.content.encoded.typeId.identifier.numeric, 4711);
    
    //{"unknown":"body","saveas":"Bytestring"}Q
    ck_assert_int_eq(out.content.encoded.body.length, 40);
    ck_assert_int_eq(out.content.encoded.body.data[2], 'u');
    ck_assert_int_eq(out.content.encoded.body.data[3], 'n');
    ck_assert_int_eq(out.content.encoded.body.data[4], 'k');
    ck_assert_int_eq(out.content.encoded.body.data[5], 'n');
    ck_assert_int_eq(out.content.encoded.body.data[6], 'o');
    ck_assert_int_eq(out.content.encoded.body.data[7], 'w');
    UA_ExtensionObject_deleteMembers(&out);
}
END_TEST

/* ----------------- Variant ---------------------*/
START_TEST(UA_VariantBool_json_decode) {
    // given
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":1,\"Body\":false}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, 0);
    ck_assert_uint_eq(*((UA_Boolean*)out.data), 0);
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_VariantStringArray_json_decode) {
    // given
    
    UA_Variant *out = UA_Variant_new();
    UA_Variant_init(out);
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":[\"1\",\"2\",\"3\",\"4\",\"5\",\"6\",\"7\",\"8\"],\"Dimension\":[2,4]}");
    //UA_ByteString buf = UA_STRING("{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, out, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_String *testArray;
    testArray = (UA_String*)(out->data);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq((char)testArray[0].data[0], '1');
    ck_assert_int_eq((char)testArray[1].data[0], '2');
    ck_assert_int_eq((char)testArray[2].data[0], '3');
    ck_assert_int_eq((char)testArray[3].data[0], '4');
    ck_assert_int_eq((char)testArray[4].data[0], '5');
    ck_assert_int_eq((char)testArray[5].data[0], '6');
    ck_assert_int_eq((char)testArray[6].data[0], '7');
    ck_assert_int_eq((char)testArray[7].data[0], '8');
    ck_assert_int_eq(out->arrayDimensionsSize, 2);
    ck_assert_int_eq(out->arrayDimensions[0], 2);
    ck_assert_int_eq(out->arrayDimensions[1], 4);
    ck_assert_int_eq(out->arrayLength, 8);
    ck_assert_int_eq(out->type->typeIndex, 11);
    UA_Variant_delete(out);
}
END_TEST

START_TEST(UA_VariantStringArrayNull_json_decode) {
    // given
    
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":[null, null, null, null, null, null, null, null],\"Dimension\":[2,4]}");
    //UA_ByteString buf = UA_STRING("{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_String *testArray;
    testArray = (UA_String*)(out.data);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(testArray[0].data, NULL);
    ck_assert_ptr_eq(testArray[1].data, NULL);
    ck_assert_ptr_eq(testArray[2].data, NULL);
    ck_assert_ptr_eq(testArray[3].data, NULL);
    ck_assert_ptr_eq(testArray[4].data, NULL);
    ck_assert_ptr_eq(testArray[5].data, NULL);
    ck_assert_ptr_eq(testArray[6].data, NULL);
    ck_assert_ptr_eq(testArray[7].data, NULL);
    ck_assert_int_eq(out.arrayDimensionsSize, 2);
    ck_assert_int_eq(out.arrayDimensions[0], 2);
    ck_assert_int_eq(out.arrayDimensions[1], 4);
    ck_assert_int_eq(out.arrayLength, 8);
    ck_assert_int_eq(out.type->typeIndex, 11);
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_VariantLocalizedTextArrayNull_json_decode) {
    // given
    
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":21,\"Body\":[null, null, null, null, null, null, null, null],\"Dimension\":[2,4]}");
    //UA_ByteString buf = UA_STRING("{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_LocalizedText *testArray;
    testArray = (UA_LocalizedText*)(out.data);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(testArray[0].locale.data, NULL);
    ck_assert_ptr_eq(testArray[1].locale.data, NULL);
    ck_assert_ptr_eq(testArray[2].locale.data, NULL);
    ck_assert_ptr_eq(testArray[3].locale.data, NULL);
    ck_assert_ptr_eq(testArray[4].locale.data, NULL);
    ck_assert_ptr_eq(testArray[5].locale.data, NULL);
    ck_assert_ptr_eq(testArray[6].locale.data, NULL);
    ck_assert_ptr_eq(testArray[7].locale.data, NULL);
    ck_assert_int_eq(out.arrayDimensionsSize, 2);
    ck_assert_int_eq(out.arrayDimensions[0], 2);
    ck_assert_int_eq(out.arrayDimensions[1], 4);
    ck_assert_int_eq(out.arrayLength, 8);
    ck_assert_int_eq(out.type->typeIndex, 20);
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_VariantVariantArrayNull_json_decode) {
    // given
    
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":22,\"Body\":[null, null, null, null, null, null, null, null],\"Dimension\":[2,4]}");
    //UA_ByteString buf = UA_STRING("{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_Variant *testArray;
    testArray = (UA_Variant*)(out.data);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_ptr_eq(testArray[0].type, NULL);
    ck_assert_ptr_eq(testArray[1].type, NULL);
    ck_assert_ptr_eq(testArray[2].type, NULL);
    ck_assert_ptr_eq(testArray[3].type, NULL);
    ck_assert_ptr_eq(testArray[4].type, NULL);
    ck_assert_ptr_eq(testArray[5].type, NULL);
    ck_assert_ptr_eq(testArray[6].type, NULL);
    ck_assert_ptr_eq(testArray[7].type, NULL);
    ck_assert_int_eq(out.arrayDimensionsSize, 2);
    ck_assert_int_eq(out.arrayDimensions[0], 2);
    ck_assert_int_eq(out.arrayDimensions[1], 4);
    ck_assert_int_eq(out.arrayLength, 8);
    ck_assert_int_eq(out.type->typeIndex, 21);
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_VariantStringArray_WithoutDimension_json_decode) {
    // given
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":[\"1\",\"2\",\"3\",\"4\",\"5\",\"6\",\"7\",\"8\"]}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_String *testArray;
    testArray = (UA_String*)(out.data);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq((char)testArray[0].data[0], '1');
    ck_assert_int_eq((char)testArray[1].data[0], '2');
    ck_assert_int_eq((char)testArray[2].data[0], '3');
    ck_assert_int_eq((char)testArray[3].data[0], '4');
    ck_assert_int_eq((char)testArray[4].data[0], '5');
    ck_assert_int_eq((char)testArray[5].data[0], '6');
    ck_assert_int_eq((char)testArray[6].data[0], '7');
    ck_assert_int_eq((char)testArray[7].data[0], '8');
    ck_assert_int_eq(out.arrayDimensionsSize, 0);
    ck_assert_int_eq(out.arrayLength, 8);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_STRING);
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_Variant_BooleanNull_json_decode) {
    // given
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":1,\"Body\":[null, null, null]}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);

    UA_Boolean *testArray;
    testArray = (UA_Boolean*)(out.data);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    //decoded as False
    ck_assert_int_eq(testArray[0], 0);
    ck_assert_int_eq(testArray[1], 0);
    ck_assert_int_eq(testArray[2], 0);
    ck_assert_int_eq(out.arrayDimensionsSize, 0);
    ck_assert_int_eq(out.arrayLength, 3);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_BOOLEAN);
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_Variant_bad1_json_decode) {
    // given
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":1,\"Body\":\"\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    UA_Variant_deleteMembers(&out);
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);
}
END_TEST

START_TEST(UA_Variant_ExtensionObjectWrap_json_decode) {
    // given
    
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":22,\"Body\":{\"TypeId\":{\"Id\":511},\"Body\":{\"ViewId\":{\"Id\":99999},\"Timestamp\":\"1970-01-15T06:56:07.000Z\",\"ViewVersion\":1236}}}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.type->typeIndex, UA_TYPES_VIEWDESCRIPTION);
    UA_ViewDescription *vd = (UA_ViewDescription*)out.data;
    ck_assert_int_eq(vd->viewId.identifier.numeric, 99999);
    ck_assert_int_eq(vd->viewVersion, 1236);
    UA_Variant_deleteMembers(&out);
}
END_TEST





START_TEST(UA_duplicate_json_decode) {
    // given
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":1, \"Body\":false, \"Type\":1}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);
    UA_Variant_deleteMembers(&out);
}
END_TEST

START_TEST(UA_wrongBoolean_json_decode) {
    // given
    UA_Variant out;
    UA_Variant_init(&out);
    UA_ByteString buf = UA_STRING("{\"Type\":1, \"Body\":\"asdfaaaaaaaaaaaaaaaaaaaa\"}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);
    UA_Variant_deleteMembers(&out);
}
END_TEST



START_TEST(UA_DataTypeAttributes_json_decode) {
    // given
    UA_DataTypeAttributes out;
    UA_DataTypeAttributes_init(&out);
    UA_ByteString buf = UA_STRING("{\"SpecifiedAttributes\":1,"
            "\"DisplayName\":{\"Locale\":\"t1\",\"Text\":\"t2\"},"
            "\"Description\":{\"Locale\":\"t3\",\"Text\":\"t4\"},"
            "\"WriteMask\":53,"
            "\"UserWriteMask\":63,"
            "\"IsAbstract\":false}");
    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_DATATYPEATTRIBUTES]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(out.isAbstract, 0);
    ck_assert_int_eq(out.writeMask, 53);
    ck_assert_int_eq(out.userWriteMask, 63);
    ck_assert_int_eq(out.specifiedAttributes, 1);
    ck_assert_int_eq(out.displayName.locale.data[1], '1');
    ck_assert_int_eq(out.displayName.text.data[1], '2');
    ck_assert_int_eq(out.description.locale.data[1], '3');
    ck_assert_int_eq(out.description.text.data[1], '4');
    UA_DataTypeAttributes_deleteMembers(&out);
}
END_TEST

//-------------------MISC heap free test cases--------------------------
START_TEST(UA_VariantStringArrayBad_shouldFreeArray_json_decode) {
    // given
    
    UA_Variant out;
    UA_Variant_init(&out);                         //not a string     V
    UA_ByteString buf = UA_STRING("{\"Type\":12,\"Body\":[\"1\",\"2\",true,\"4\",\"5\",\"6\",\"7\",\"8\"],\"Dimension\":[2,4]}");
    //UA_ByteString buf = UA_STRING("{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);

    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_VariantFuzzer1_json_decode) {
    // given
    
    UA_Variant out;
    UA_Variant_init(&out);         
    UA_ByteString buf = UA_STRING("\\x0a{\"Type\",\"Bode\",\"Body\":{\"se\":}}");
    //UA_ByteString buf = UA_STRING("{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);

    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_BADDECODINGERROR);
    UA_Variant_deleteMembers(&out);
}
END_TEST



//This test succeeds: Double will be parsed to zero if unparsable
START_TEST(UA_VariantFuzzer2_json_decode) {
    // given
    
    UA_Variant out;
    UA_Variant_init(&out);         
    UA_ByteString buf = UA_STRING("{\"Type\":11,\"Body\":2E+}");
    //UA_ByteString buf = UA_STRING("{\"SymbolicId\":13,\"LocalizedText\":14,\"Locale\":12,\"AdditionalInfo\":\"additionalInfo\",\"InnerStatusCode\":2155216896}");

    // when
    
    UA_StatusCode retval = UA_decodeJson(&buf, &out, &UA_TYPES[UA_TYPES_VARIANT]);

    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_deleteMembers(&out);
}
END_TEST


START_TEST(UA_JsonHelper) {
    // given
    
    CtxJson ctx;
    memset(&ctx, 0, sizeof(ctx));
    ck_assert_int_eq(encodingJsonStartArray(&ctx), UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    ck_assert_int_eq(encodingJsonStartObject(&ctx), UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    ck_assert_int_eq(encodingJsonEndObject(&ctx), UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    ck_assert_int_eq(encodingJsonEndArray(&ctx), UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    ck_assert_int_eq(writeNull(&ctx), UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
    UA_String s = UA_STRING("Test");
    ck_assert_int_eq(writeKey_UA_String(&ctx, &s, UA_FALSE), UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED);
}   
END_TEST


static Suite *testSuite_builtin_json(void) {
    Suite *s = suite_create("Built-in Data Types 62541-6 Json");
    
    TCase *tc_json_encode = tcase_create("json_encode");
    tcase_add_test(tc_json_encode, UA_Boolean_true_json_encode);
    tcase_add_test(tc_json_encode, UA_Boolean_false_json_encode);
    tcase_add_test(tc_json_encode, UA_Boolean_null_json_encode);
    tcase_add_test(tc_json_encode, UA_Boolean_true_bufferTooSmall_json_encode);
    
    tcase_add_test(tc_json_encode, UA_String_json_encode);
    tcase_add_test(tc_json_encode, UA_String_Empty_json_encode);
    tcase_add_test(tc_json_encode, UA_String_Null_json_encode);
    tcase_add_test(tc_json_encode, UA_String_escapesimple_json_encode);
    tcase_add_test(tc_json_encode, UA_String_escapeutf_json_encode);
    
    tcase_add_test(tc_json_encode, UA_Byte_Max_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Byte_Min_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Byte_smallbuf_Number_json_encode);
    
    tcase_add_test(tc_json_encode, UA_SByte_Max_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_SByte_Min_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_SByte_Zero_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_SByte_smallbuf_Number_json_encode);
    
 
    tcase_add_test(tc_json_encode, UA_UInt16_Max_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_UInt16_Min_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_UInt16_smallbuf_Number_json_encode);
    
    tcase_add_test(tc_json_encode, UA_Int16_Max_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int16_Min_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int16_Zero_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int16_smallbuf_Number_json_encode);
    
    
    tcase_add_test(tc_json_encode, UA_UInt32_Max_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_UInt32_Min_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_UInt32_smallbuf_Number_json_encode);
    
    tcase_add_test(tc_json_encode, UA_Int32_Max_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int32_Min_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int32_Zero_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int32_smallbuf_Number_json_encode);
    
    
    tcase_add_test(tc_json_encode, UA_UInt64_Max_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_UInt64_Min_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_UInt64_smallbuf_Number_json_encode);
    
    tcase_add_test(tc_json_encode, UA_Int64_Max_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int64_Min_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int64_Zero_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Int64_smallbuf_Number_json_encode);
    
    //Double Float
    tcase_add_test(tc_json_encode, UA_Double_json_encode);
    tcase_add_test(tc_json_encode, UA_Double_onesmallest_json_encode);
    tcase_add_test(tc_json_encode, UA_Double_pluszero_json_encode);
    tcase_add_test(tc_json_encode, UA_Double_minuszero_json_encode);
    tcase_add_test(tc_json_encode, UA_Double_plusInf_json_encode);
    tcase_add_test(tc_json_encode, UA_Double_minusInf_json_encode);
    tcase_add_test(tc_json_encode, UA_Double_nan_json_encode);
    tcase_add_test(tc_json_encode, UA_Float_json_encode);
    
    //LocalizedText
    tcase_add_test(tc_json_encode, UA_LocText_json_encode);
    tcase_add_test(tc_json_encode, UA_LocText_NonReversible_json_encode);
    tcase_add_test(tc_json_encode, UA_LocText_smallBuffer_json_encode);
    
    //Guid
    tcase_add_test(tc_json_encode, UA_Guid_json_encode);
    tcase_add_test(tc_json_encode, UA_Guid_smallbuf_json_encode);
    
    //DateTime
    tcase_add_test(tc_json_encode, UA_DateTime_json_encode);
    
    
    //StatusCode
    tcase_add_test(tc_json_encode, UA_StatusCode_json_encode);
    tcase_add_test(tc_json_encode, UA_StatusCode_nonReversible_json_encode);
    tcase_add_test(tc_json_encode, UA_StatusCode_nonReversible_good_json_encode);
    tcase_add_test(tc_json_encode, UA_StatusCode_smallbuf_json_encode);
    
    
    //NodeId
    tcase_add_test(tc_json_encode, UA_NodeId_Numeric_json_encode);
    tcase_add_test(tc_json_encode, UA_NodeId_Numeric_Namespace_json_encode);
    
    tcase_add_test(tc_json_encode, UA_NodeId_String_json_encode);
    tcase_add_test(tc_json_encode, UA_NodeId_String_Namespace_json_encode);
    
    tcase_add_test(tc_json_encode, UA_NodeId_Guid_json_encode);
    tcase_add_test(tc_json_encode, UA_NodeId_Guid_Namespace_json_encode);
    
    tcase_add_test(tc_json_encode, UA_NodeId_ByteString_json_encode);
    tcase_add_test(tc_json_encode, UA_NodeId_ByteString_Namespace_json_encode);
    
    tcase_add_test(tc_json_encode, UA_NodeId_NonReversible_Numeric_Namespace_json_encode);
    
    //ExpandedNodeId
    tcase_add_test(tc_json_encode, UA_ExpandedNodeId_json_encode);
    tcase_add_test(tc_json_encode, UA_ExpandedNodeId_MissingNamespaceUri_json_encode);
    tcase_add_test(tc_json_encode, UA_ExpandedNodeId_NonReversible_Ns1_json_encode);
    tcase_add_test(tc_json_encode, UA_ExpandedNodeId_NonReversible_Namespace_json_encode);
    tcase_add_test(tc_json_encode, UA_ExpandedNodeId_NonReversible_NamespaceUriGiven_json_encode);
    
    
    //DiagnosticInfo
    tcase_add_test(tc_json_encode, UA_DiagInfo_json_encode);
    tcase_add_test(tc_json_encode, UA_DiagInfo_withInner_json_encode);
    tcase_add_test(tc_json_encode, UA_DiagInfo_withTwoInner_json_encode);
    tcase_add_test(tc_json_encode, UA_DiagInfo_noFields_json_encode);
    tcase_add_test(tc_json_encode, UA_DiagInfo_smallBuffer_json_encode);
    
    
    //ByteString
    tcase_add_test(tc_json_encode, UA_ByteString_json_encode);
    tcase_add_test(tc_json_encode, UA_ByteString_Long_json_encode);
    
    //QualifiedName
    tcase_add_test(tc_json_encode, UA_QualName_json_encode);
    tcase_add_test(tc_json_encode, UA_QualName_NonReversible_json_encode);
    tcase_add_test(tc_json_encode, UA_QualName_NonReversible_Namespace_json_encode);
    tcase_add_test(tc_json_encode, UA_QualName_NonReversible_NoNamespaceAsNumber_json_encode);
           
    //Variant -REVERSIBLE-
    tcase_add_test(tc_json_encode, UA_Variant_Bool_json_encode);
    tcase_add_test(tc_json_encode, UA_Variant_Number_json_encode);
    tcase_add_test(tc_json_encode, UA_Variant_NodeId_json_encode);
    tcase_add_test(tc_json_encode, UA_Variant_LocText_json_encode);
    tcase_add_test(tc_json_encode, UA_Variant_QualName_json_encode);
    
    //Array
    tcase_add_test(tc_json_encode, UA_Variant_Array_UInt16_json_encode);
    tcase_add_test(tc_json_encode, UA_Variant_Array_Byte_json_encode);
    tcase_add_test(tc_json_encode, UA_Variant_Array_String_json_encode);
    
    //Matrix
    tcase_add_test(tc_json_encode, UA_Variant_Matrix_UInt16_json_encode);
    
    //Wrap
    tcase_add_test(tc_json_encode, UA_Variant_Wrap_json_encode);
    tcase_add_test(tc_json_encode, UA_Variant_Wrap_Array_json_encode);
    
    //Variant -NON-REVERSIBLE-
    tcase_add_test(tc_json_encode, UA_Variant_StatusCode_NonReversible_json_encode);
    
    //Array
    tcase_add_test(tc_json_encode, UA_Variant_Array_String_NonReversible_json_encode);
    
    //Matrix
    tcase_add_test(tc_json_encode, UA_Variant_Matrix_String_NonReversible_json_encode);
    tcase_add_test(tc_json_encode, UA_Variant_Matrix_NodeId_NonReversible_json_encode);
    
    //Wrap non reversible
    tcase_add_test(tc_json_encode, UA_Variant_Wrap_Array_NonReversible_json_encode);
    
    
    //ExtensionObject
    tcase_add_test(tc_json_encode, UA_ExtensionObject_json_encode);
    tcase_add_test(tc_json_encode, UA_ExtensionObject_xml_json_encode);
    tcase_add_test(tc_json_encode, UA_ExtensionObject_byteString_json_encode);
    
    tcase_add_test(tc_json_encode, UA_ExtensionObject_NonReversible_StatusCode_json_encode);
    
    //DataValue
    tcase_add_test(tc_json_encode, UA_DataValue_json_encode);
    tcase_add_test(tc_json_encode, UA_DataValue_null_json_encode);

    tcase_add_test(tc_json_encode, UA_MessageReadResponse_json_encode);
    tcase_add_test(tc_json_encode, UA_ViewDescription_json_encode);
    tcase_add_test(tc_json_encode, UA_WriteRequest_json_encode);

    
    tcase_add_test(tc_json_encode, UA_null_json_encode);
    
    suite_add_tcase(s, tc_json_encode);
    
    TCase *tc_json_decode = tcase_create("json_decode");
    
    
    tcase_add_test(tc_json_decode, UA_SByte_Min_json_decode);
    tcase_add_test(tc_json_decode, UA_SByte_Max_json_decode);
    
    tcase_add_test(tc_json_decode, UA_Byte_Min_json_decode);
    tcase_add_test(tc_json_decode, UA_Byte_Max_json_decode);
    
    tcase_add_test(tc_json_decode, UA_Int16_Min_json_decode);
    tcase_add_test(tc_json_decode, UA_Int16_Max_json_decode);
    
    tcase_add_test(tc_json_decode, UA_UInt16_Min_json_decode);
    tcase_add_test(tc_json_decode, UA_UInt16_Max_json_decode);
    
    tcase_add_test(tc_json_decode, UA_UInt32_Min_json_decode);
    tcase_add_test(tc_json_decode, UA_UInt32_Max_json_decode);
    
    tcase_add_test(tc_json_decode, UA_Int32_Min_json_decode);
    tcase_add_test(tc_json_decode, UA_Int32_Max_json_decode);
    
    tcase_add_test(tc_json_decode, UA_Int64_Min_json_decode);
    tcase_add_test(tc_json_decode, UA_Int64_Max_json_decode);
    
    tcase_add_test(tc_json_decode, UA_UInt64_Min_json_decode);
    tcase_add_test(tc_json_decode, UA_UInt64_Max_json_decode);
    
    
    tcase_add_test(tc_json_decode, UA_Float_json_decode);
    tcase_add_test(tc_json_decode, UA_Float_json_one_decode);
    
    tcase_add_test(tc_json_decode, UA_Double_json_decode);
    tcase_add_test(tc_json_decode, UA_Double_one_json_decode);
    tcase_add_test(tc_json_decode, UA_Double_onepointsmallest_json_decode);
    tcase_add_test(tc_json_decode, UA_Double_nan_json_decode);
    tcase_add_test(tc_json_decode, UA_Double_negzero_json_decode);
    tcase_add_test(tc_json_decode, UA_Double_zero_json_decode);
    tcase_add_test(tc_json_decode, UA_Double_inf_json_decode);
    tcase_add_test(tc_json_decode, UA_Double_neginf_json_decode);
    
    
    //String
    tcase_add_test(tc_json_decode, UA_String_json_decode);
    tcase_add_test(tc_json_decode, UA_String_unescapeBS_json_decode);
    
    tcase_add_test(tc_json_decode, UA_String_escape_unicode_json_decode);
    
    tcase_add_test(tc_json_decode, UA_String_escape2_json_decode);
    tcase_add_test(tc_json_decode, UA_String_surrogatePair_json_decode);
    
    //ByteString
    tcase_add_test(tc_json_decode, UA_ByteString_json_decode);
    tcase_add_test(tc_json_decode, UA_ByteString_bad_json_decode);
    tcase_add_test(tc_json_decode, UA_ByteString_null_json_decode);
    
    
    //DateTime
    tcase_add_test(tc_json_decode, UA_DateTime_json_decode);
    tcase_add_test(tc_json_decode, UA_DateTime_micro_json_decode);
    
    
    //Guid
    tcase_add_test(tc_json_decode, UA_Guid_json_decode);
    tcase_add_test(tc_json_decode, UA_Guid_tooShort_json_decode);
    tcase_add_test(tc_json_decode, UA_Guid_tooLong_json_decode);
    tcase_add_test(tc_json_decode, UA_Guid_wrong_json_decode);
    
    
    //StatusCode
    tcase_add_test(tc_json_decode, UA_StatusCode_2_json_decode);
    tcase_add_test(tc_json_decode, UA_StatusCode_0_json_decode);
    
    
    //QualName
    tcase_add_test(tc_json_decode, UA_QualifiedName_json_decode);
    tcase_add_test(tc_json_decode, UA_QualifiedName_null_json_decode);
    
    
    //LocalizedText
    tcase_add_test(tc_json_decode, UA_LocalizedText_json_decode);
    tcase_add_test(tc_json_decode, UA_LocalizedText_missing_json_decode);
    tcase_add_test(tc_json_decode, UA_LocalizedText_null_json_decode);
    

    //-NodeId-
    tcase_add_test(tc_json_decode, UA_NodeId_Nummeric_json_decode);
    tcase_add_test(tc_json_decode, UA_NodeId_Nummeric_Namespace_json_decode);

    tcase_add_test(tc_json_decode, UA_NodeId_String_json_decode);
    
    tcase_add_test(tc_json_decode, UA_NodeId_Guid_json_decode);
    
    tcase_add_test(tc_json_decode, UA_NodeId_ByteString_json_decode);
    
    
    //expandednodeid
    tcase_add_test(tc_json_decode, UA_ExpandedNodeId_Nummeric_json_decode);
    tcase_add_test(tc_json_decode, UA_ExpandedNodeId_String_json_decode);
    tcase_add_test(tc_json_decode, UA_ExpandedNodeId_String_Namespace_json_decode);
    tcase_add_test(tc_json_decode, UA_ExpandedNodeId_String_NamespaceAsIndex_json_decode);
    tcase_add_test(tc_json_decode, UA_ExpandedNodeId_String_Namespace_ServerUri_json_decode);
    tcase_add_test(tc_json_decode, UA_ExpandedNodeId_ByteString_json_decode);
    
    //Diaginfo
    tcase_add_test(tc_json_decode, UA_DiagnosticInfo_json_decode);
    tcase_add_test(tc_json_decode, UA_DiagnosticInfo_null_json_decode);
    
    
    //Variant
    tcase_add_test(tc_json_decode, UA_VariantBool_json_decode);
    tcase_add_test(tc_json_decode, UA_VariantStringArray_json_decode);
    tcase_add_test(tc_json_decode, UA_VariantStringArrayNull_json_decode);
    tcase_add_test(tc_json_decode, UA_VariantLocalizedTextArrayNull_json_decode);
    tcase_add_test(tc_json_decode, UA_VariantVariantArrayNull_json_decode);
    tcase_add_test(tc_json_decode, UA_VariantStringArray_WithoutDimension_json_decode);
    tcase_add_test(tc_json_decode, UA_Variant_BooleanNull_json_decode);
    tcase_add_test(tc_json_decode, UA_Variant_bad1_json_decode);
    tcase_add_test(tc_json_decode, UA_Variant_ExtensionObjectWrap_json_decode);
    
    //DataValue
    tcase_add_test(tc_json_decode, UA_DataValue_json_decode);
    tcase_add_test(tc_json_decode, UA_DataValueMissingFields_json_decode);
    tcase_add_test(tc_json_decode, UA_DataValue_null_json_decode);
            
    //extensionobject
    tcase_add_test(tc_json_decode, UA_ExtensionObject_json_decode);
    tcase_add_test(tc_json_decode, UA_ExtensionObject_EncodedByteString_json_decode);
    tcase_add_test(tc_json_decode, UA_ExtensionObject_EncodedXml_json_decode);
    tcase_add_test(tc_json_decode, UA_ExtensionObject_Unkown_json_decode);
    
    
    //Others
    tcase_add_test(tc_json_decode, UA_duplicate_json_decode);
    tcase_add_test(tc_json_decode, UA_wrongBoolean_json_decode);
    
    tcase_add_test(tc_json_decode, UA_ViewDescription_json_decode);
    tcase_add_test(tc_json_decode, UA_DataTypeAttributes_json_decode);
    
    
    tcase_add_test(tc_json_decode, UA_VariantStringArrayBad_shouldFreeArray_json_decode);
    tcase_add_test(tc_json_decode, UA_VariantFuzzer1_json_decode);
    tcase_add_test(tc_json_decode, UA_VariantFuzzer2_json_decode);
     
    suite_add_tcase(s, tc_json_decode);
    
    TCase *tc_json_helper = tcase_create("json_helper");
    tcase_add_test(tc_json_decode, UA_JsonHelper);
    suite_add_tcase(s, tc_json_helper);
    return s;
}

int main(void) {
    int      number_failed = 0;
    Suite   *s;
    SRunner *sr;

    s  = testSuite_builtin_json();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
