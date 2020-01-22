/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>

#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"

#include <check.h>

UA_ByteString *buffers;
size_t bufIndex;
size_t counter;
size_t dataCount;

static UA_StatusCode
sendChunkMockUp(void *_, UA_Byte **bufPos, const UA_Byte **bufEnd) {
    size_t offset = (uintptr_t)(*bufPos - buffers[bufIndex].data);
    bufIndex++;
    *bufPos = buffers[bufIndex].data;
    *bufEnd = &(*bufPos)[buffers[bufIndex].length];
    counter++;
    dataCount += offset;
    return UA_STATUSCODE_GOOD;
}

START_TEST(encodeArrayIntoFiveChunksShallWork) {
    size_t arraySize = 30; //number of elements within the array which should be encoded
    size_t chunkCount = 6; // maximum chunk count
    size_t chunkSize = 30; //size in bytes of each chunk
    bufIndex = 0;
    counter = 0;
    dataCount = 0;
    buffers = (UA_ByteString*)UA_Array_new(chunkCount, &UA_TYPES[UA_TYPES_BYTESTRING]);
    for(size_t i=0;i<chunkCount;i++){
        UA_ByteString_allocBuffer(&buffers[i],chunkSize);
    }

    UA_Int32 *ar = (UA_Int32*)UA_Array_new(arraySize,&UA_TYPES[UA_TYPES_INT32]);
    for(size_t i = 0; i < arraySize; i++)
        ar[i] = (UA_Int32)i;

    UA_Variant v;
    UA_Variant_setArrayCopy(&v, ar, arraySize, &UA_TYPES[UA_TYPES_INT32]);

    UA_ByteString workingBuffer = buffers[0];
    UA_Byte *pos = workingBuffer.data;
    const UA_Byte *end = &workingBuffer.data[workingBuffer.length];
    UA_StatusCode retval = UA_encodeBinary(&v,&UA_TYPES[UA_TYPES_VARIANT],
                                           &pos, &end, sendChunkMockUp, NULL);

    ck_assert_uint_eq(retval,UA_STATUSCODE_GOOD);
    ck_assert_int_eq(counter,4); //5 chunks allocated - callback called 4 times

    dataCount += (uintptr_t)(pos - buffers[bufIndex].data);
    ck_assert_int_eq(UA_calcSizeBinary(&v,&UA_TYPES[UA_TYPES_VARIANT]), dataCount);

    UA_Variant_deleteMembers(&v);
    UA_Array_delete(buffers, chunkCount, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Array_delete(ar, arraySize, &UA_TYPES[UA_TYPES_INT32]);
} END_TEST

START_TEST(encodeStringIntoFiveChunksShallWork) {
    size_t stringLength = 120; //number of elements within the array which should be encoded
    size_t chunkCount = 6; // maximum chunk count
    size_t chunkSize = 30; //size in bytes of each chunk

    UA_String string;
    bufIndex = 0;
    counter = 0;
    dataCount = 0;
    UA_String_init(&string);
    string.data = (UA_Byte*)UA_malloc(stringLength);
    string.length = stringLength;
    char tmpString[9] = {'o','p','e','n','6','2','5','4','1'};
    //char tmpString[9] = {'1','4','5','2','6','n','e','p','o'};
    buffers = (UA_ByteString*)UA_Array_new(chunkCount, &UA_TYPES[UA_TYPES_BYTESTRING]);
    for(size_t i=0;i<chunkCount;i++){
        UA_ByteString_allocBuffer(&buffers[i],chunkSize);
    }

    UA_ByteString workingBuffer=buffers[0];

    for(size_t i=0;i<stringLength;i++){
        size_t tmp = i % 9;
        string.data[i] =  tmpString[tmp];
    }
    UA_Variant v;
    UA_Variant_setScalarCopy(&v,&string,&UA_TYPES[UA_TYPES_STRING]);

    UA_Byte *pos = workingBuffer.data;
    const UA_Byte *end = &workingBuffer.data[workingBuffer.length];
    UA_StatusCode retval = UA_encodeBinary(&v, &UA_TYPES[UA_TYPES_VARIANT],
                                           &pos, &end, sendChunkMockUp, NULL);

    ck_assert_uint_eq(retval,UA_STATUSCODE_GOOD);
    ck_assert_int_eq(counter,4); //5 chunks allocated - callback called 4 times

    dataCount += (uintptr_t)(pos - buffers[bufIndex].data);
    ck_assert_int_eq(UA_calcSizeBinary(&v,&UA_TYPES[UA_TYPES_VARIANT]), dataCount);

    UA_Variant_deleteMembers(&v);
    UA_Array_delete(buffers, chunkCount, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_String_deleteMembers(&string);
} END_TEST

START_TEST(encodeTwoStringsIntoTenChunksShallWork) {
    size_t stringLength = 143; //number of elements within the array which should be encoded
    size_t chunkCount = 10; // maximum chunk count
    size_t chunkSize = 30; //size in bytes of each chunk

    UA_String string;
    bufIndex = 0;
    counter = 0;
    dataCount = 0;
    UA_String_init(&string);
    string.data = (UA_Byte*)UA_malloc(stringLength);
    string.length = stringLength;
    char tmpString[9] = {'o','p','e','n','6','2','5','4','1'};
    buffers = (UA_ByteString*)UA_Array_new(chunkCount, &UA_TYPES[UA_TYPES_BYTESTRING]);
    for(size_t i=0;i<chunkCount;i++){
        UA_ByteString_allocBuffer(&buffers[i],chunkSize);
    }

    UA_ByteString workingBuffer=buffers[0];

    for(size_t i=0;i<stringLength;i++){
        size_t tmp = i % 9;
        string.data[i] =  tmpString[tmp];
    }

    UA_Byte *pos = workingBuffer.data;
    const UA_Byte *end = &workingBuffer.data[workingBuffer.length];
    UA_StatusCode retval = UA_encodeBinary(&string, &UA_TYPES[UA_TYPES_STRING],
                                           &pos, &end, sendChunkMockUp, NULL);
    ck_assert_uint_eq(retval,UA_STATUSCODE_GOOD);
    ck_assert_int_eq(counter,4); //5 chunks allocated - callback called 4 times
    size_t offset = (uintptr_t)(pos - buffers[bufIndex].data);
    ck_assert_int_eq(UA_calcSizeBinary(&string,&UA_TYPES[UA_TYPES_STRING]), dataCount + offset);

    retval = UA_encodeBinary(&string,&UA_TYPES[UA_TYPES_STRING],
                             &pos, &end, sendChunkMockUp, NULL);
    dataCount += (uintptr_t)(pos - buffers[bufIndex].data);
    ck_assert_uint_eq(retval,UA_STATUSCODE_GOOD);
    ck_assert_int_eq(counter,9); //10 chunks allocated - callback called 4 times
    ck_assert_int_eq(2 * UA_calcSizeBinary(&string,&UA_TYPES[UA_TYPES_STRING]), dataCount);

    UA_Array_delete(buffers, chunkCount, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_String_deleteMembers(&string);
} END_TEST

int main(void) {
    Suite *s = suite_create("Chunked encoding");
    TCase *tc_message = tcase_create("encode chunking");
    tcase_add_test(tc_message,encodeArrayIntoFiveChunksShallWork);
    tcase_add_test(tc_message,encodeStringIntoFiveChunksShallWork);
    tcase_add_test(tc_message,encodeTwoStringsIntoTenChunksShallWork);
    suite_add_tcase(s, tc_message);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
