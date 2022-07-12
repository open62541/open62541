/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/types.h>
#include <open62541/types_generated.h>
#include <open62541/types_generated_handling.h>
#include <open62541/util.h>

#include "ua_util_internal.h"

#include <check.h>
#include <float.h>
#include <math.h>

/* copied here from encoding_binary.c */
enum UA_VARIANT_ENCODINGMASKTYPE_enum {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,            // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6),     // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)      // bit 7
};

START_TEST(UA_Byte_decodeShallCopyAndAdvancePosition) {
    // given
    UA_Byte dst;
    UA_Byte data[] = { 0x08 };
    UA_ByteString src = { 1, data };
    size_t pos = 0;

    // when
    UA_StatusCode retval = UA_Byte_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 1);
    ck_assert_uint_eq(pos, UA_calcSizeBinary(&dst, &UA_TYPES[UA_TYPES_BYTE]));
    ck_assert_uint_eq(dst, 0x08);
}
END_TEST

START_TEST(UA_Byte_decodeShallModifyOnlyCurrentPosition) {
    // given
    UA_Byte dst[]  = { 0xFF, 0xFF, 0xFF };
    UA_Byte data[] = { 0x08 };
    UA_ByteString src = { 1, data };
    size_t pos = 0;
    // when
    UA_StatusCode retval = UA_Byte_decodeBinary(&src, &pos, &dst[1]);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 1);
    ck_assert_uint_eq(dst[0], 0xFF);
    ck_assert_uint_eq(dst[1], 0x08);
    ck_assert_uint_eq(dst[2], 0xFF);
}
END_TEST

START_TEST(UA_Int16_decodeShallAssumeLittleEndian) {
    // given
    size_t pos = 0;
    UA_Byte data[] = {
            0x01, 0x00,     // 1
            0x00, 0x01      // 256
    };
    UA_ByteString src = { 4, data };
    // when
    UA_Int16 val_01_00, val_00_01;
    UA_StatusCode retval = UA_Int16_decodeBinary(&src, &pos, &val_01_00);
    retval |= UA_Int16_decodeBinary(&src, &pos, &val_00_01);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(val_01_00, 1);
    ck_assert_int_eq(val_00_01, 256);
    ck_assert_uint_eq(pos, 4);
}
END_TEST

START_TEST(UA_Int16_decodeShallRespectSign) {
    // given
    size_t pos = 0;
    UA_Byte data[] = {
            0xFF, 0xFF,     // -1
            0x00, 0x80      // -32768
    };
    UA_ByteString src = { 4, data };
    // when
    UA_Int16 val_ff_ff, val_00_80;
    UA_StatusCode retval = UA_Int16_decodeBinary(&src, &pos, &val_ff_ff);
    retval |= UA_Int16_decodeBinary(&src, &pos, &val_00_80);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(val_ff_ff, -1);
    ck_assert_int_eq(val_00_80, -32768);
}
END_TEST

START_TEST(UA_UInt16_decodeShallNotRespectSign) {
    // given
    size_t pos = 0;
    UA_Byte data[] = {
            0xFF, 0xFF,     // (2^16)-1
            0x00, 0x80      // (2^15)
    };
    UA_ByteString src = { 4, data };
    // when
    UA_UInt16     val_ff_ff, val_00_80;
    UA_StatusCode retval = UA_UInt16_decodeBinary(&src, &pos, &val_ff_ff);
    retval |= UA_UInt16_decodeBinary(&src, &pos, &val_00_80);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 4);
    ck_assert_uint_eq(val_ff_ff, (0x01 << 16)-1);
    ck_assert_uint_eq(val_00_80, (0x01 << 15));
}
END_TEST

START_TEST(UA_Int32_decodeShallAssumeLittleEndian) {
    // given
    size_t pos = 0;
    UA_Byte data[] = {
            0x01, 0x00, 0x00, 0x00,     // 1
            0x00, 0x01, 0x00, 0x00      // 256
    };
    UA_ByteString src = { 8, data };

    // when
    UA_Int32 val_01_00, val_00_01;
    UA_StatusCode retval = UA_Int32_decodeBinary(&src, &pos, &val_01_00);
    retval |= UA_Int32_decodeBinary(&src, &pos, &val_00_01);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(val_01_00, 1);
    ck_assert_int_eq(val_00_01, 256);
    ck_assert_uint_eq(pos, 8);
}
END_TEST

START_TEST(UA_Int32_decodeShallRespectSign) {
    // given
    size_t pos = 0;
    UA_Byte data[] = {
            0xFF, 0xFF, 0xFF, 0xFF,     // -1
            0x00, 0x80, 0xFF, 0xFF      // -32768
    };
    UA_ByteString src = { 8, data };

    // when
    UA_Int32 val_ff_ff, val_00_80;
    UA_StatusCode retval = UA_Int32_decodeBinary(&src, &pos, &val_ff_ff);
    retval |= UA_Int32_decodeBinary(&src, &pos, &val_00_80);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(val_ff_ff, -1);
    ck_assert_int_eq(val_00_80, -32768);
}
END_TEST

START_TEST(UA_UInt32_decodeShallNotRespectSign) {
    // given
    size_t pos = 0;
    UA_Byte data[] = {
            0xFF, 0xFF, 0xFF, 0xFF,     // (2^32)-1
            0x00, 0x00, 0x00, 0x80      // (2^31)
    };
    UA_ByteString src = { 8, data };

    // when
    UA_UInt32 val_ff_ff, val_00_80;
    UA_StatusCode retval = UA_UInt32_decodeBinary(&src, &pos, &val_ff_ff);
    retval |= UA_UInt32_decodeBinary(&src, &pos, &val_00_80);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 8);
    ck_assert_uint_eq(val_ff_ff, (UA_UInt32)( (0x01LL << 32 ) - 1 ));
    ck_assert_uint_eq(val_00_80, (UA_UInt32)(0x01) << 31);
}
END_TEST

START_TEST(UA_UInt64_decodeShallNotRespectSign) {
    // given
    UA_ByteString rawMessage;
    UA_UInt64     expectedVal = 0xFF;
    expectedVal = expectedVal << 56;
    UA_Byte mem[8] = { 00, 00, 00, 00, 0x00, 0x00, 0x00, 0xFF };
    rawMessage.data   = mem;
    rawMessage.length = 8;
    size_t pos = 0;
    UA_UInt64 val;
    // when
    UA_UInt64_decodeBinary(&rawMessage, &pos, &val);
    // then
    ck_assert_uint_eq(val, expectedVal);
}
END_TEST

START_TEST(UA_Int64_decodeShallRespectSign) {
    // given
    UA_ByteString rawMessage;
    UA_Int64 expectedVal = ((UA_Int64)0xFF) << 56;
    UA_Byte  mem[8]      = { 00, 00, 00, 00, 0x00, 0x00, 0x00, 0xFF };
    rawMessage.data   = mem;
    rawMessage.length = 8;

    size_t pos = 0;
    UA_Int64 val;
    // when
    UA_Int64_decodeBinary(&rawMessage, &pos, &val);
    //then
    ck_assert_int_eq(val, expectedVal);
}
END_TEST

START_TEST(UA_Float_decodeShallWorkOnExample) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { 0x00, 0x00, 0xD0, 0xC0 }; // -6.5
    UA_ByteString src = { 4, data };
    UA_Float dst;
    // when
    UA_StatusCode retval = UA_Float_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 4);
    ck_assert(-6.5000001 < dst);
    ck_assert(dst < -6.49999999999);
}
END_TEST

START_TEST(UA_Double_decodeShallGiveOne) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F }; // 1
    UA_ByteString src = { 8, data }; // 1
    UA_Double dst;
    // when
    UA_StatusCode retval = UA_Double_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 8);
    ck_assert(0.9999999 < dst);
    ck_assert(dst < 1.00000000001);
}
END_TEST

START_TEST(UA_Double_decodeShallGiveZero) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    UA_ByteString src = { 8, data }; // 1
    UA_Double dst;
    // when
    UA_StatusCode retval = UA_Double_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 8);
    ck_assert(-0.00000001 < dst);
    ck_assert(dst < 0.000000001);
}
END_TEST

START_TEST(UA_Double_decodeShallGiveMinusTwo) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0 }; // -2
    UA_ByteString src = { 8, data };
    UA_Double dst;
    // when
    UA_StatusCode retval = UA_Double_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 8);
    ck_assert(-1.9999999 > dst);
    ck_assert(dst > -2.00000000001);
}
END_TEST

START_TEST(UA_Double_decodeShallGive2147483648) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x41 }; //2147483648
    UA_ByteString src = { 8, data }; // 1
    UA_Double dst;
    // when
    UA_StatusCode retval = UA_Double_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 8);
    ck_assert(2147483647.9999999 <= dst);
    ck_assert(dst <= 2147483648.00000001);
}
END_TEST

START_TEST(UA_String_decodeShallAllocateMemoryAndCopyString) {
    // given
    size_t pos = 0;
    UA_Byte data[] =
    { 0x08, 0x00, 0x00, 0x00, 'A', 'C', 'P', 'L', 'T', ' ', 'U', 'A', 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    UA_ByteString src = { 16, data };
    UA_String dst;
    // when
    UA_StatusCode retval = UA_String_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.length, 8);
    ck_assert_int_eq(dst.data[3], 'L');
    ck_assert_uint_eq(pos, UA_calcSizeBinary(&dst, &UA_TYPES[UA_TYPES_STRING]));
    // finally
    UA_String_clear(&dst);
}
END_TEST

START_TEST(UA_String_decodeWithNegativeSizeShallNotAllocateMemoryAndNullPtr) {
    // given
    size_t pos = 0;
    UA_Byte data[] =
    { 0xFF, 0xFF, 0xFF, 0xFF, 'A', 'C', 'P', 'L', 'T', ' ', 'U', 'A', 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    UA_ByteString src = { 16, data };
    UA_String dst;
    // when
    UA_StatusCode retval = UA_String_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.length, 0);
    ck_assert_ptr_eq(dst.data, NULL);
}
END_TEST

START_TEST(UA_String_decodeWithZeroSizeShallNotAllocateMemoryAndNullPtr) {
    // given
    size_t pos = 0;
    UA_Byte data[] =
    { 0x00, 0x00, 0x00, 0x00, 'A', 'C', 'P', 'L', 'T', ' ', 'U', 'A', 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    UA_ByteString src = { 17, data };
    UA_String dst;
    // when
    UA_StatusCode retval = UA_String_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(dst.length, 0);
    ck_assert_ptr_eq(dst.data, UA_EMPTY_ARRAY_SENTINEL);
}
END_TEST

START_TEST(UA_NodeId_decodeTwoByteShallReadTwoBytesAndSetNamespaceToZero) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { 0 /* UA_NODEIDTYPE_TWOBYTE */, 0x10 };
    UA_ByteString src    = { 2, data };
    UA_NodeId dst;
    // when
    UA_StatusCode retval = UA_NodeId_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 2);
    ck_assert_uint_eq(pos, UA_calcSizeBinary(&dst, &UA_TYPES[UA_TYPES_NODEID]));
    ck_assert_int_eq(dst.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(dst.identifier.numeric, 16);
    ck_assert_int_eq(dst.namespaceIndex, 0);
}
END_TEST

START_TEST(UA_NodeId_decodeFourByteShallReadFourBytesAndRespectNamespace) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { 1 /* UA_NODEIDTYPE_FOURBYTE */, 0x01, 0x00, 0x01 };
    UA_ByteString src = { 4, data };
    UA_NodeId dst;
    // when
    UA_StatusCode retval = UA_NodeId_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 4);
    ck_assert_uint_eq(pos, UA_calcSizeBinary(&dst, &UA_TYPES[UA_TYPES_NODEID]));
    ck_assert_int_eq(dst.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_eq(dst.identifier.numeric, 256);
    ck_assert_int_eq(dst.namespaceIndex, 1);
}
END_TEST

START_TEST(UA_NodeId_decodeStringShallAllocateMemory) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { UA_NODEIDTYPE_STRING, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 'P', 'L', 'T' };
    UA_ByteString src = { 10, data };
    UA_NodeId dst;
    // when
    UA_StatusCode retval = UA_NodeId_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 10);
    ck_assert_uint_eq(pos, UA_calcSizeBinary(&dst, &UA_TYPES[UA_TYPES_NODEID]));
    ck_assert_int_eq(dst.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_int_eq(dst.namespaceIndex, 1);
    ck_assert_uint_eq(dst.identifier.string.length, 3);
    ck_assert_int_eq(dst.identifier.string.data[1], 'L');
    // finally
    UA_NodeId_clear(&dst);
}
END_TEST

START_TEST(UA_Variant_decodeWithOutArrayFlagSetShallSetVTAndAllocateMemoryForArray) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { (UA_Byte)UA_TYPES[UA_TYPES_INT32].typeId.identifier.numeric, 0xFF, 0x00, 0x00, 0x00 };
    UA_ByteString src = { 5, data };
    UA_Variant dst;
    // when
    UA_StatusCode retval = UA_Variant_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 5);
    ck_assert_uint_eq(pos, UA_calcSizeBinary(&dst, &UA_TYPES[UA_TYPES_VARIANT]));
    //ck_assert_ptr_eq((const void *)dst.type, (const void *)&UA_TYPES[UA_TYPES_INT32]); //does not compile in gcc 4.6
    ck_assert_uint_eq((uintptr_t)dst.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(dst.arrayLength, 0);
    ck_assert_uint_ne((uintptr_t)dst.data, 0);
    UA_assert(dst.data != NULL); /* repeat the previous argument so that clang-analyzer is happy */
    ck_assert_int_eq(*(UA_Int32 *)dst.data, 255);
    // finally
    UA_Variant_clear(&dst);
}
END_TEST

START_TEST(UA_Variant_decodeWithArrayFlagSetShallSetVTAndAllocateMemoryForArray) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { (UA_Byte)(UA_TYPES[UA_TYPES_INT32].typeId.identifier.numeric |
                                 UA_VARIANT_ENCODINGMASKTYPE_ARRAY),
                       0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF,
                       0xFF, 0xFF };
    UA_ByteString src = { 13, data };
    UA_Variant dst;
    // when
    UA_StatusCode retval = UA_Variant_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 1+4+2*4);
    ck_assert_uint_eq(pos, UA_calcSizeBinary(&dst, &UA_TYPES[UA_TYPES_VARIANT]));
    //ck_assert_ptr_eq((const (void*))dst.type, (const void*)&UA_TYPES[UA_TYPES_INT32]); //does not compile in gcc 4.6
    ck_assert_uint_eq((uintptr_t)dst.type,(uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_uint_eq(dst.arrayLength, 2);
    ck_assert_int_eq(((UA_Int32 *)dst.data)[0], 255);
    ck_assert_int_eq(((UA_Int32 *)dst.data)[1], -1);
    // finally
    UA_Variant_clear(&dst);
}
END_TEST

START_TEST(UA_Variant_decodeSingleExtensionObjectShallSetVTAndAllocateMemory){
    /* // given */
    /* size_t pos = 0; */
    /* UA_Variant dst; */
    /* UA_NodeId tmpNodeId; */

    /* UA_NodeId_init(&tmpNodeId); */
    /* tmpNodeId.identifier.numeric = 22; */
    /* tmpNodeId.namespaceIndex = 2; */
    /* tmpNodeId.identifierType = UA_NODEIDTYPE_NUMERIC; */

    /* UA_ExtensionObject tmpExtensionObject; */
    /* UA_ExtensionObject_init(&tmpExtensionObject); */
    /* tmpExtensionObject.encoding = UA_EXTENSIONOBJECT_ENCODED_BYTESTRING; */
    /* tmpExtensionObject.content.encoded.body = UA_ByteString_withSize(3); */
    /* tmpExtensionObject.content.encoded.body.data[0]= 10; */
    /* tmpExtensionObject.content.encoded.body.data[1]= 20; */
    /* tmpExtensionObject.content.encoded.body.data[2]= 30; */
    /* tmpExtensionObject.content.encoded.typeId = tmpNodeId; */

    /* UA_Variant tmpVariant; */
    /* UA_Variant_init(&tmpVariant); */
    /* tmpVariant.arrayDimensions = NULL; */
    /* tmpVariant.arrayDimensionsSize = -1; */
    /* tmpVariant.arrayLength = -1; */
    /* tmpVariant.storageType = UA_VARIANT_DATA_NODELETE; */
    /* tmpVariant.type = &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]; */
    /* tmpVariant.data = &tmpExtensionObject; */

    /* UA_ByteString srcByteString = UA_ByteString_withSize(200); */
    /* pos = 0; */
    /* UA_Variant_encodeBinary(&tmpVariant,&srcByteString,pos); */

    /* // when */
    /* pos = 0; */
    /* UA_StatusCode retval = UA_Variant_decodeBinary(&srcByteString, &pos, &dst); */
    /* // then */
    /* ck_assert_int_eq(retval, UA_STATUSCODE_GOOD); */
    /* // TODO!! */
    /* /\* ck_assert_int_eq(dst.encoding, UA_EXTENSIONOBJECT_DECODED); *\/ */
    /* /\* ck_assert_uint_eq((uintptr_t)dst.content.decoded.type, (uintptr_t)&UA_TYPES[UA_TYPES_EXTENSIONOBJECT]); *\/ */
    /* /\* ck_assert_uint_eq(dst.arrayLength, -1); *\/ */
    /* /\* ck_assert_int_eq(((UA_ExtensionObject *)dst.data)->body.data[0], 10); *\/ */
    /* /\* ck_assert_int_eq(((UA_ExtensionObject *)dst.data)->body.data[1], 20); *\/ */
    /* /\* ck_assert_int_eq(((UA_ExtensionObject *)dst.data)->body.data[2], 30); *\/ */
    /* /\* ck_assert_uint_eq(((UA_ExtensionObject *)dst.data)->body.length, 3); *\/ */


    /* // finally */
    /* UA_Variant_clear(&dst); */
    /* UA_ByteString_clear(&srcByteString); */
    /* UA_ExtensionObject_clear(&tmpExtensionObject); */

}
END_TEST

START_TEST(UA_Variant_decodeWithOutDeleteMembersShallFailInCheckMem) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { (UA_Byte)(UA_TYPES[UA_TYPES_INT32].typeId.identifier.numeric |
                                 UA_VARIANT_ENCODINGMASKTYPE_ARRAY),
                       0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF };
    UA_ByteString src = { 13, data };
    UA_Variant dst;
    // when
    UA_StatusCode retval = UA_Variant_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    // finally
    UA_Variant_clear(&dst);
}
END_TEST

START_TEST(UA_Variant_decodeWithTooSmallSourceShallReturnWithError) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { (UA_Byte)(UA_TYPES[UA_TYPES_INT32].typeId.identifier.numeric |
                                 UA_VARIANT_ENCODINGMASKTYPE_ARRAY),
                       0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF };
    UA_ByteString src = { 4, data };

    UA_Variant dst;
    // when
    UA_StatusCode retval = UA_Variant_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
    // finally
    UA_Variant_clear(&dst);
}
END_TEST

START_TEST(UA_Byte_encode_test) {
    // given
    UA_Byte src       = 8;
    UA_Byte data[]    = { 0x00, 0xFF };
    UA_ByteString dst = { 2, data };
    ck_assert_uint_eq(dst.data[1], 0xFF);

    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];
    UA_StatusCode retval = UA_Byte_encodeBinary(&src, &pos, end);

    ck_assert_uint_eq(dst.data[0], 0x08);
    ck_assert_uint_eq(dst.data[1], 0xFF);
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 1);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // Test2
    // given
    src = 0xFF;
    dst.data[1] = 0x00;
    pos = dst.data;
    retval      = UA_Byte_encodeBinary(&src, &pos, end);

    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0x00);
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 1);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

}
END_TEST

START_TEST(UA_UInt16_encodeNegativeShallEncodeLittleEndian) {
    // given
    UA_UInt16     src    = (UA_UInt16)-1;
    UA_Byte       data[] = { 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst    = { 4, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_UInt16_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 2);
    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = (UA_UInt16)-32768;
    retval = UA_UInt16_encodeBinary(&src, &pos, end);
    // then test 2
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 4);
    ck_assert_int_eq(dst.data[2], 0x00);
    ck_assert_int_eq(dst.data[3], 0x80);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_UInt16_encodeShallEncodeLittleEndian) {
    // given
    UA_UInt16     src    = 0;
    UA_Byte       data[] = {  0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst    = { 4, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_UInt16_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 2);
    ck_assert_int_eq(dst.data[0], 0x00);
    ck_assert_int_eq(dst.data[1], 0x00);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = 32767;
    retval = UA_UInt16_encodeBinary(&src, &pos, end);
    // then test 2
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 4);
    ck_assert_int_eq(dst.data[2], 0xFF);
    ck_assert_int_eq(dst.data[3], 0x7F);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_UInt32_encodeShallEncodeLittleEndian) {
    // given
    UA_UInt32     src    = (UA_UInt32)(-1);
    UA_Byte       data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst    = { 8, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_UInt32_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 4);
    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(dst.data[2], 0xFF);
    ck_assert_int_eq(dst.data[3], 0xFF);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = 0x0101FF00;
    retval = UA_UInt32_encodeBinary(&src, &pos, end);
    // then test 2
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 8);
    ck_assert_int_eq(dst.data[4], 0x00);
    ck_assert_int_eq(dst.data[5], 0xFF);
    ck_assert_int_eq(dst.data[6], 0x01);
    ck_assert_int_eq(dst.data[7], 0x01);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_Int32_encodeShallEncodeLittleEndian) {
    // given
    UA_Int32 src    = 1;
    UA_Byte  data[]   = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 8, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_Int32_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 4);
    ck_assert_int_eq(dst.data[0], 0x01);
    ck_assert_int_eq(dst.data[1], 0x00);
    ck_assert_int_eq(dst.data[2], 0x00);
    ck_assert_int_eq(dst.data[3], 0x00);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = 0x7FFFFFFF;
    retval = UA_Int32_encodeBinary(&src, &pos, end);
    // then test 2
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 8);
    ck_assert_int_eq(dst.data[4], 0xFF);
    ck_assert_int_eq(dst.data[5], 0xFF);
    ck_assert_int_eq(dst.data[6], 0xFF);
    ck_assert_int_eq(dst.data[7], 0x7F);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_Int32_encodeNegativeShallEncodeLittleEndian) {
    // given
    UA_Int32 src    = -1;
    UA_Byte  data[]   = {  0x55, 0x55,    0x55,  0x55, 0x55,  0x55,    0x55,  0x55 };
    UA_ByteString dst = { 8, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_Int32_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 4);
    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(dst.data[2], 0xFF);
    ck_assert_int_eq(dst.data[3], 0xFF);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_UInt64_encodeShallWorkOnExample) {
    // given
    UA_UInt64     src    = (UA_UInt64)(-1LL);
    UA_Byte       data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                             0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst    = { 16, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_UInt64_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 8);
    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(dst.data[2], 0xFF);
    ck_assert_int_eq(dst.data[3], 0xFF);
    ck_assert_int_eq(dst.data[4], 0xFF);
    ck_assert_int_eq(dst.data[5], 0xFF);
    ck_assert_int_eq(dst.data[6], 0xFF);
    ck_assert_int_eq(dst.data[7], 0xFF);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = 0x7F0033AA44EE6611;
    retval = UA_UInt64_encodeBinary(&src, &pos, end);
    // then test 2
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 16);
    ck_assert_int_eq(dst.data[8], 0x11);
    ck_assert_int_eq(dst.data[9], 0x66);
    ck_assert_int_eq(dst.data[10], 0xEE);
    ck_assert_int_eq(dst.data[11], 0x44);
    ck_assert_int_eq(dst.data[12], 0xAA);
    ck_assert_int_eq(dst.data[13], 0x33);
    ck_assert_int_eq(dst.data[14], 0x00);
    ck_assert_int_eq(dst.data[15], 0x7F);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_Int64_encodeShallEncodeLittleEndian) {
    // given
    UA_Int64 src    = 0x7F0033AA44EE6611;
    UA_Byte  data[]   = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                          0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 16, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_Int64_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 8);
    ck_assert_int_eq(dst.data[0], 0x11);
    ck_assert_int_eq(dst.data[1], 0x66);
    ck_assert_int_eq(dst.data[2], 0xEE);
    ck_assert_int_eq(dst.data[3], 0x44);
    ck_assert_int_eq(dst.data[4], 0xAA);
    ck_assert_int_eq(dst.data[5], 0x33);
    ck_assert_int_eq(dst.data[6], 0x00);
    ck_assert_int_eq(dst.data[7], 0x7F);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_Int64_encodeNegativeShallEncodeLittleEndian) {
    // given
    UA_Int64 src    = -1;
    UA_Byte  data[]   = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                          0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 16, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_Int64_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 8);
    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(dst.data[2], 0xFF);
    ck_assert_int_eq(dst.data[3], 0xFF);
    ck_assert_int_eq(dst.data[4], 0xFF);
    ck_assert_int_eq(dst.data[5], 0xFF);
    ck_assert_int_eq(dst.data[6], 0xFF);
    ck_assert_int_eq(dst.data[7], 0xFF);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_Float_encodeShallWorkOnExample) {
#define UA_FLOAT_TESTS 9
    /* use -NAN since the UA standard expected specific values for NAN with the
       negative bit set */
    UA_Float src[UA_FLOAT_TESTS] = {27.5f, -6.5f, 0.0f, -0.0f, -NAN, FLT_MAX, FLT_MIN, INFINITY, -INFINITY};
    UA_Byte result[UA_FLOAT_TESTS][4] = {
        {0x00, 0x00, 0xDC, 0x41}, // 27.5
        {0x00, 0x00, 0xD0, 0xC0}, // -6.5
        {0x00, 0x00, 0x00, 0x00}, // 0.0
        {0x00, 0x00, 0x00, 0x80}, // -0.0
        {0x00, 0x00, 0xC0, 0xFF}, // -NAN
        {0xFF, 0xFF, 0x7F, 0x7F}, // FLT_MAX
        {0x00, 0x00, 0x80, 0x00}, // FLT_MIN
        {0x00, 0x00, 0x80, 0x7F}, // INF
        {0x00, 0x00, 0x80, 0xFF} // -INF
    };
#if defined(_MSC_VER)
    /* On Visual Studio, -NAN is encoded differently */
    result[4][3] = 127;
#endif

    UA_Byte data[] = {0x55, 0x55, 0x55,  0x55};
    UA_ByteString dst = {4, data};
    const UA_Byte *end = &dst.data[dst.length];

    for(size_t i = 0; i < 7; i++) {
        UA_Byte *pos = dst.data;
        UA_UInt32 retval = UA_Float_encodeBinary(&src[i], &pos, end);
        ck_assert_uint_eq((uintptr_t)(pos - dst.data), 4);
        ck_assert_uint_eq(dst.data[0], result[i][0]);
        ck_assert_uint_eq(dst.data[1], result[i][1]);
        ck_assert_uint_eq(dst.data[2], result[i][2]);
        ck_assert_uint_eq(dst.data[3], result[i][3]);
        ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    }
}
END_TEST

START_TEST(UA_Double_encodeShallWorkOnExample) {
    // given
    UA_Double src = -6.5;
    UA_Byte data[] = { 0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
                       0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55 };
    UA_ByteString dst = {16,data};
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when test 1
    UA_StatusCode retval = UA_Double_encodeBinary(&src, &pos, end);
    // then test 1
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 8);
    ck_assert_int_eq(dst.data[6], 0x1A);
    ck_assert_int_eq(dst.data[7], 0xC0);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_String_encodeShallWorkOnExample) {
    // given
    UA_String src;
    src.length = 11;
    UA_Byte mem[12] = "ACPLT OPCUA";
    src.data = mem;

    UA_Byte data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                       0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                       0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 24, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when
    UA_StatusCode retval = UA_String_encodeBinary(&src, &pos, end);
    // then
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), sizeof(UA_Int32)+11);
    ck_assert_uint_eq(sizeof(UA_Int32)+11, UA_calcSizeBinary(&src, &UA_TYPES[UA_TYPES_STRING]));
    ck_assert_int_eq(dst.data[0], 11);
    ck_assert_int_eq(dst.data[sizeof(UA_Int32)+0], 'A');
    ck_assert_int_eq(dst.data[sizeof(UA_Int32)+1], 'C');
    ck_assert_int_eq(dst.data[sizeof(UA_Int32)+2], 'P');
    ck_assert_int_eq(dst.data[sizeof(UA_Int32)+3], 'L');
    ck_assert_int_eq(dst.data[sizeof(UA_Int32)+4], 'T');
    ck_assert_int_eq(dst.data[sizeof(UA_Int32)+5], 0x20); //Space
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_ExpandedNodeId_encodeShallWorkOnExample) {
    // given
    UA_ExpandedNodeId src = UA_EXPANDEDNODEID_NUMERIC(0, 15);
    src.namespaceUri = UA_STRING("testUri");

    UA_Byte data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                       0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                       0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                       0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 32, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when
    UA_StatusCode retval = UA_ExpandedNodeId_encodeBinary(&src, &pos, end);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 13);
    ck_assert_uint_eq(13, UA_calcSizeBinary(&src, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]));
    ck_assert_int_eq(dst.data[0], 0x80); // namespaceuri flag
}
END_TEST

START_TEST(UA_DataValue_encodeShallWorkOnExampleWithoutVariant) {
    // given
    UA_DataValue src;
    UA_DataValue_init(&src);
    src.serverTimestamp = 80;
    src.hasServerTimestamp = true;

    UA_Byte data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                       0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                       0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 24, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when
    UA_StatusCode retval = UA_DataValue_encodeBinary(&src, &pos, end);
    // then
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 9);
    ck_assert_uint_eq(9, UA_calcSizeBinary(&src, &UA_TYPES[UA_TYPES_DATAVALUE]));
    ck_assert_int_eq(dst.data[0], 0x08); // encodingMask
    ck_assert_int_eq(dst.data[1], 80);   // 8 Byte serverTimestamp
    ck_assert_int_eq(dst.data[2], 0);
    ck_assert_int_eq(dst.data[3], 0);
    ck_assert_int_eq(dst.data[4], 0);
    ck_assert_int_eq(dst.data[5], 0);
    ck_assert_int_eq(dst.data[6], 0);
    ck_assert_int_eq(dst.data[7], 0);
    ck_assert_int_eq(dst.data[8], 0);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_DataValue_encodeShallWorkOnExampleWithVariant) {
    // given
    UA_DataValue src;
    UA_DataValue_init(&src);
    src.serverTimestamp    = 80;
    src.hasValue = true;
    src.hasServerTimestamp = true;
    src.value.type = &UA_TYPES[UA_TYPES_INT32];
    src.value.arrayLength  = 0; // one element (encoded as not an array)
    UA_Int32 vdata = 45;
    src.value.data = (void *)&vdata;

    UA_Byte data[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    UA_ByteString dst = { 24, data };
    UA_Byte *pos = dst.data;
    const UA_Byte *end = &dst.data[dst.length];

    // when
    UA_StatusCode retval = UA_DataValue_encodeBinary(&src, &pos, end);
    // then
    ck_assert_uint_eq((uintptr_t)(pos - dst.data), 1+(1+4)+8);           // represents the length
    ck_assert_uint_eq(1+(1+4)+8, UA_calcSizeBinary(&src, &UA_TYPES[UA_TYPES_DATAVALUE]));
    ck_assert_int_eq(dst.data[0], 0x08 | 0x01); // encodingMask
    ck_assert_int_eq(dst.data[1], 0x06);        // Variant's Encoding Mask - INT32
    ck_assert_int_eq(dst.data[2], 45);          // the single value
    ck_assert_int_eq(dst.data[3], 0);
    ck_assert_int_eq(dst.data[4], 0);
    ck_assert_int_eq(dst.data[5], 0);
    ck_assert_int_eq(dst.data[6], 80);  // the server timestamp
    ck_assert_int_eq(dst.data[7], 0);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_DateTime_toStructShallWorkOnExample) {
    // given
    UA_DateTime src = 13974671891234567 + (11644473600 * 10000000); // ua counts since 1601, unix since 1970
    //1397467189... is Mon, 14 Apr 2014 09:19:49 GMT
    //...1234567 are the milli-, micro- and nanoseconds
    UA_DateTimeStruct dst;

    // when
    dst = UA_DateTime_toStruct(src);
    // then
    ck_assert_int_eq(dst.nanoSec, 700);
    ck_assert_int_eq(dst.microSec, 456);
    ck_assert_int_eq(dst.milliSec, 123);

    ck_assert_int_eq(dst.sec, 49);
    ck_assert_int_eq(dst.min, 19);
    ck_assert_int_eq(dst.hour, 9);

    ck_assert_int_eq(dst.day, 14);
    ck_assert_int_eq(dst.month, 4);
    ck_assert_int_eq(dst.year, 2014);
}
END_TEST

START_TEST(UA_DateTime_toStructAndBack) {
    UA_DateTime src = 13974671891234567 + (11644473600 * 10000000);
    UA_DateTime dst = UA_DateTime_fromStruct(UA_DateTime_toStruct(src));
    ck_assert_int_eq(src, dst);

    src = 0;
    dst = UA_DateTime_fromStruct(UA_DateTime_toStruct(src));
    ck_assert_int_eq(src, dst);

    src = UA_DATETIME_UNIX_EPOCH;
    dst = UA_DateTime_fromStruct(UA_DateTime_toStruct(src));
    ck_assert_int_eq(src, dst);

    src = -UA_DATETIME_UNIX_EPOCH;
    dst = UA_DateTime_fromStruct(UA_DateTime_toStruct(src));
    ck_assert_int_eq(src, dst);

    /* Conversion to DateTimeStruct is currently broken for negative DateTimes.
     * So dates before 1601! */

    /* src = -UA_DATETIME_UNIX_EPOCH - UA_DATETIME_SEC - UA_DATETIME_MSEC - UA_DATETIME_USEC; */
    /* dst = UA_DateTime_fromStruct(UA_DateTime_toStruct(src)); */
    /* ck_assert_int_eq(src, dst); */

    /* src = LLONG_MIN; */
    /* dst = UA_DateTime_fromStruct(UA_DateTime_toStruct(src)); */
    /* ck_assert_int_eq(src, dst); */

    src = UA_INT64_MAX;
    dst = UA_DateTime_fromStruct(UA_DateTime_toStruct(src));
    ck_assert_int_eq(src, dst);
}
END_TEST

START_TEST(UA_QualifiedName_equalShallWorkOnExample) {
    // given
    UA_QualifiedName qn1 = UA_QUALIFIEDNAME(5, "tEsT123!");
    UA_QualifiedName qn2 = UA_QUALIFIEDNAME(3, "tEsT123!");
    UA_QualifiedName qn3 = UA_QUALIFIEDNAME(5, "tEsT1");
    UA_QualifiedName qn4 = UA_QUALIFIEDNAME(5, "tEsT123!");

    ck_assert(UA_QualifiedName_equal(&qn1, &qn2) == UA_FALSE);
    ck_assert(UA_QualifiedName_equal(&qn1, &qn3) == UA_FALSE);
    ck_assert(UA_QualifiedName_equal(&qn1, &qn4) == UA_TRUE);
}
END_TEST

START_TEST(UA_ExpandedNodeId_hashIdentical) {
    // given
    UA_NodeId n = UA_NODEID_NUMERIC(1, 1234);
    UA_ExpandedNodeId en = UA_EXPANDEDNODEID_NUMERIC(1, 1234);

    ck_assert(UA_ExpandedNodeId_hash(&en) == UA_NodeId_hash(&n));
}
END_TEST

START_TEST(UA_ExtensionObject_copyShallWorkOnExample) {
    // given
    /* UA_Byte data[3] = { 1, 2, 3 }; */

    /* UA_ExtensionObject value, valueCopied; */
    /* UA_ExtensionObject_init(&value); */
    /* UA_ExtensionObject_init(&valueCopied); */

    //Todo!!
    /* value.typeId = UA_TYPES[UA_TYPES_BYTE].typeId; */
    /* value.encoding    = UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED; */
    /* value.encoding    = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING; */
    /* value.body.data   = data; */
    /* value.body.length = 3; */

    /* //when */
    /* UA_ExtensionObject_copy(&value, &valueCopied); */

    /* for(UA_Int32 i = 0;i < 3;i++) */
    /*     ck_assert_int_eq(valueCopied.body.data[i], value.body.data[i]); */

    /* ck_assert_int_eq(valueCopied.encoding, value.encoding); */
    /* ck_assert_int_eq(valueCopied.typeId.identifierType, value.typeId.identifierType); */
    /* ck_assert_int_eq(valueCopied.typeId.identifier.numeric, value.typeId.identifier.numeric); */

    /* //finally */
    /* value.body.data = NULL; // we cannot free the static string */
    /* UA_ExtensionObject_clear(&value); */
    /* UA_ExtensionObject_clear(&valueCopied); */
}
END_TEST

START_TEST(UA_Array_copyByteArrayShallWorkOnExample) {
    //given
    UA_String testString;
    UA_Byte  *dstArray;
    UA_UInt32  size = 5;
    UA_UInt32  i    = 0;
    testString.data = (UA_Byte*)UA_malloc(size);
    testString.data[0] = 'O';
    testString.data[1] = 'P';
    testString.data[2] = 'C';
    testString.data[3] = 'U';
    testString.data[4] = 'A';
    testString.length  = 5;

    //when
    UA_StatusCode retval;
    retval = UA_Array_copy((const void *)testString.data, 5, (void **)&dstArray, &UA_TYPES[UA_TYPES_BYTE]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    //then
    for(i = 0;i < size;i++)
        ck_assert_int_eq(testString.data[i], dstArray[i]);

    //finally
    UA_String_clear(&testString);
    UA_free((void *)dstArray);

}
END_TEST

START_TEST(UA_Array_copyUA_StringShallWorkOnExample) {
    // given
    UA_Int32   i, j;
    UA_String *srcArray = (UA_String*)UA_Array_new(3, &UA_TYPES[UA_TYPES_STRING]);
    UA_String *dstArray;

    srcArray[0] = UA_STRING_ALLOC("open");
    srcArray[1] = UA_STRING_ALLOC("62541");
    srcArray[2] = UA_STRING_ALLOC("opc ua");
    //when
    UA_StatusCode retval;
    retval = UA_Array_copy((const void *)srcArray, 3, (void **)&dstArray, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    //then
    for(i = 0;i < 3;i++) {
        for(j = 0;j < 3;j++)
            ck_assert_int_eq(srcArray[i].data[j], dstArray[i].data[j]);
        ck_assert_uint_eq(srcArray[i].length, dstArray[i].length);
    }
    //finally
    UA_Array_delete(srcArray, 3, &UA_TYPES[UA_TYPES_STRING]);
    UA_Array_delete(dstArray, 3, &UA_TYPES[UA_TYPES_STRING]);
}
END_TEST

START_TEST(UA_DiagnosticInfo_copyShallWorkOnExample) {
    //given
    UA_DiagnosticInfo value, innerValue, copiedValue;
    UA_String testString = (UA_String){5, (UA_Byte*)"OPCUA"};

    UA_DiagnosticInfo_init(&value);
    UA_DiagnosticInfo_init(&innerValue);
    value.hasInnerDiagnosticInfo = true;
    value.innerDiagnosticInfo = &innerValue;
    value.hasAdditionalInfo = true;
    value.additionalInfo = testString;

    //when
    UA_DiagnosticInfo_copy(&value, &copiedValue);

    //then
    for(size_t i = 0;i < testString.length;i++)
        ck_assert_int_eq(copiedValue.additionalInfo.data[i], value.additionalInfo.data[i]);
    ck_assert_uint_eq(copiedValue.additionalInfo.length, value.additionalInfo.length);

    ck_assert_int_eq(copiedValue.hasInnerDiagnosticInfo, value.hasInnerDiagnosticInfo);
    ck_assert_int_eq(copiedValue.innerDiagnosticInfo->locale, value.innerDiagnosticInfo->locale);
    ck_assert_int_eq(copiedValue.innerStatusCode, value.innerStatusCode);
    ck_assert_int_eq(copiedValue.locale, value.locale);
    ck_assert_int_eq(copiedValue.localizedText, value.localizedText);
    ck_assert_int_eq(copiedValue.namespaceUri, value.namespaceUri);
    ck_assert_int_eq(copiedValue.symbolicId, value.symbolicId);

    //finally
    value.additionalInfo.data = NULL; // do not delete the static string
    value.innerDiagnosticInfo = NULL; // do not delete the static innerdiagnosticinfo
    UA_DiagnosticInfo_clear(&value);
    UA_DiagnosticInfo_clear(&copiedValue);

}
END_TEST

START_TEST(UA_ApplicationDescription_copyShallWorkOnExample) {
    //given

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_String appString = (UA_String){3, (UA_Byte*)"APP"};
    UA_String discString = (UA_String){4, (UA_Byte*)"DISC"};
    UA_String gateWayString = (UA_String){7, (UA_Byte*)"GATEWAY"};

    UA_String srcArray[3];
    srcArray[0] = (UA_String){ 6, (UA_Byte*)"__open" };
    srcArray[1] = (UA_String){ 6, (UA_Byte*)"_62541" };
    srcArray[2] = (UA_String){ 6, (UA_Byte*)"opc ua" };

    UA_ApplicationDescription value, copiedValue;
    UA_ApplicationDescription_init(&value);
    value.applicationUri = appString;
    value.discoveryProfileUri = discString;
    value.gatewayServerUri = gateWayString;
    value.discoveryUrlsSize = 3;
    value.discoveryUrls     = srcArray;

    //when
    retval = UA_ApplicationDescription_copy(&value, &copiedValue);

    //then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    for(size_t i = 0; i < appString.length; i++)
        ck_assert_int_eq(copiedValue.applicationUri.data[i], value.applicationUri.data[i]);
    ck_assert_uint_eq(copiedValue.applicationUri.length, value.applicationUri.length);

    for(size_t i = 0; i < discString.length; i++)
        ck_assert_int_eq(copiedValue.discoveryProfileUri.data[i], value.discoveryProfileUri.data[i]);
    ck_assert_uint_eq(copiedValue.discoveryProfileUri.length, value.discoveryProfileUri.length);

    for(size_t i = 0; i < gateWayString.length; i++)
        ck_assert_int_eq(copiedValue.gatewayServerUri.data[i], value.gatewayServerUri.data[i]);
    ck_assert_uint_eq(copiedValue.gatewayServerUri.length, value.gatewayServerUri.length);

    //String Array Test
    for(UA_Int32 i = 0;i < 3;i++) {
        for(UA_Int32 j = 0;j < 6;j++)
            ck_assert_int_eq(value.discoveryUrls[i].data[j], copiedValue.discoveryUrls[i].data[j]);
        ck_assert_uint_eq(value.discoveryUrls[i].length, copiedValue.discoveryUrls[i].length);
    }
    ck_assert_int_eq(copiedValue.discoveryUrls[0].data[2], 'o');
    ck_assert_int_eq(copiedValue.discoveryUrls[0].data[3], 'p');
    ck_assert_uint_eq(copiedValue.discoveryUrlsSize, value.discoveryUrlsSize);

    //finally
    // UA_ApplicationDescription_clear(&value); // do not free the members as they are statically allocated
    UA_ApplicationDescription_clear(&copiedValue);
}
END_TEST

START_TEST(UA_QualifiedName_copyShallWorkOnInputExample) {
    // given
    UA_QualifiedName src = UA_QUALIFIEDNAME(5, "tEsT123!");
    UA_QualifiedName dst;

    // when
    UA_StatusCode ret = UA_QualifiedName_copy(&src, &dst);
    // then
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_int_eq('E', dst.name.data[1]);
    ck_assert_int_eq('!', dst.name.data[7]);
    ck_assert_uint_eq(8, dst.name.length);
    ck_assert_int_eq(5, dst.namespaceIndex);
    // finally
    UA_QualifiedName_clear(&dst);
}
END_TEST

START_TEST(UA_Guid_copyShallWorkOnInputExample) {
    //given
    const UA_Guid src = {3, 45, 1222, {8, 7, 6, 5, 4, 3, 2, 1}};
    UA_Guid dst;

    //when
    UA_StatusCode ret = UA_Guid_copy(&src, &dst);

    //then
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(src.data1, dst.data1);
    ck_assert_int_eq(src.data3, dst.data3);
    ck_assert_int_eq(src.data4[4], dst.data4[4]);
    //finally
}
END_TEST

START_TEST(UA_LocalizedText_copycstringShallWorkOnInputExample) {
    // given
    char src[8] = {'t', 'e', 'X', 't', '1', '2', '3', (char)0};
    const UA_LocalizedText dst = UA_LOCALIZEDTEXT("", src);

    // then
    ck_assert_int_eq('1', dst.text.data[4]);
    ck_assert_uint_eq(0, dst.locale.length);
    ck_assert_uint_eq(7, dst.text.length);
}
END_TEST

START_TEST(UA_DataValue_copyShallWorkOnInputExample) {
    // given
    UA_Variant srcVariant;
    UA_Variant_init(&srcVariant);
    UA_DataValue src;
    UA_DataValue_init(&src);
    src.hasSourceTimestamp = true;
    src.sourceTimestamp = 4;
    src.hasSourcePicoseconds = true;
    src.sourcePicoseconds = 77;
    src.hasServerPicoseconds = true;
    src.serverPicoseconds = 8;
    UA_DataValue dst;

    // when
    UA_StatusCode ret = UA_DataValue_copy(&src, &dst);
    // then
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(4, dst.sourceTimestamp);
    ck_assert_int_eq(77, dst.sourcePicoseconds);
    ck_assert_int_eq(8, dst.serverPicoseconds);
}
END_TEST

START_TEST(UA_Variant_copyShallWorkOnSingleValueExample) {
    //given
    UA_String testString = (UA_String){5, (UA_Byte*)"OPCUA"};
    UA_Variant value, copiedValue;
    UA_Variant_init(&value);
    UA_Variant_init(&copiedValue);
    value.data = UA_malloc(sizeof(UA_String));
    *((UA_String*)value.data) = testString;
    value.type = &UA_TYPES[UA_TYPES_STRING];
    value.arrayLength = 1;

    //when
    UA_Variant_copy(&value, &copiedValue);

    //then
    UA_String copiedString = *(UA_String*)(copiedValue.data);
    for(UA_Int32 i = 0;i < 5;i++)
        ck_assert_int_eq(copiedString.data[i], testString.data[i]);
    ck_assert_uint_eq(copiedString.length, testString.length);

    ck_assert_uint_eq(value.arrayDimensionsSize, copiedValue.arrayDimensionsSize);
    ck_assert_uint_eq(value.arrayLength, copiedValue.arrayLength);

    //finally
    ((UA_String*)value.data)->data = NULL; // the string is statically allocated. do not free it.
    UA_Variant_clear(&value);
    UA_Variant_clear(&copiedValue);
}
END_TEST

START_TEST(UA_Variant_copyShallWorkOnByteStringIndexRange) {
    UA_ByteString text = UA_BYTESTRING("My xml");
    UA_Variant src;
    UA_Variant_setScalar(&src, &text, &UA_TYPES[UA_TYPES_BYTESTRING]);

    UA_NumericRangeDimension d1 = {0, 8388607};
    UA_NumericRange nr;
    nr.dimensionsSize = 1;
    nr.dimensions = &d1;

    UA_Variant dst;
    UA_StatusCode retval = UA_Variant_copyRange(&src, &dst, nr);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    UA_Variant_clear(&dst);
}
END_TEST

START_TEST(UA_Variant_copyShallWorkOn1DArrayExample) {
    // given
    UA_String *srcArray = (UA_String*)UA_Array_new(3, &UA_TYPES[UA_TYPES_STRING]);
    srcArray[0] = UA_STRING_ALLOC("__open");
    srcArray[1] = UA_STRING_ALLOC("_62541");
    srcArray[2] = UA_STRING_ALLOC("opc ua");

    UA_UInt32 *dimensions;
    dimensions = (UA_UInt32*)UA_malloc(sizeof(UA_UInt32));
    dimensions[0] = 3;

    UA_Variant value, copiedValue;
    UA_Variant_init(&value);
    UA_Variant_init(&copiedValue);

    value.arrayLength = 3;
    value.data = (void *)srcArray;
    value.arrayDimensionsSize = 1;
    value.arrayDimensions = dimensions;
    value.type = &UA_TYPES[UA_TYPES_STRING];

    //when
    UA_Variant_copy(&value, &copiedValue);

    //then
    UA_UInt32 i1 = value.arrayDimensions[0];
    UA_UInt32 i2 = copiedValue.arrayDimensions[0];
    ck_assert_uint_eq(i1, i2);

    for(UA_Int32 i = 0;i < 3;i++) {
        for(UA_Int32 j = 0;j < 6;j++) {
            ck_assert_int_eq(((UA_String *)value.data)[i].data[j],
                    ((UA_String *)copiedValue.data)[i].data[j]);
        }
        ck_assert_uint_eq(((UA_String *)value.data)[i].length,
                ((UA_String *)copiedValue.data)[i].length);
    }
    ck_assert_int_eq(((UA_String *)copiedValue.data)[0].data[2], 'o');
    ck_assert_int_eq(((UA_String *)copiedValue.data)[0].data[3], 'p');
    ck_assert_uint_eq(value.arrayDimensionsSize, copiedValue.arrayDimensionsSize);
    ck_assert_uint_eq(value.arrayLength, copiedValue.arrayLength);

    //finally
    UA_Variant_clear(&value);
    UA_Variant_clear(&copiedValue);
}
END_TEST

START_TEST(UA_Variant_copyShallWorkOn2DArrayExample) {
    // given
    UA_Int32 *srcArray = (UA_Int32*)UA_Array_new(6, &UA_TYPES[UA_TYPES_INT32]);
    srcArray[0] = 0;
    srcArray[1] = 1;
    srcArray[2] = 2;
    srcArray[3] = 3;
    srcArray[4] = 4;
    srcArray[5] = 5;

    UA_UInt32 *dimensions = (UA_UInt32*)UA_Array_new(2, &UA_TYPES[UA_TYPES_INT32]);
    UA_UInt32 dim1 = 3;
    UA_UInt32 dim2 = 2;
    dimensions[0] = dim1;
    dimensions[1] = dim2;

    UA_Variant value, copiedValue;
    UA_Variant_init(&value);
    UA_Variant_init(&copiedValue);

    value.arrayLength = 6;
    value.data     = srcArray;
    value.arrayDimensionsSize = 2;
    value.arrayDimensions     = dimensions;
    value.type = &UA_TYPES[UA_TYPES_INT32];

    //when
    UA_Variant_copy(&value, &copiedValue);

    //then
    //1st dimension
    UA_UInt32 i1 = value.arrayDimensions[0];
    UA_UInt32 i2 = copiedValue.arrayDimensions[0];
    ck_assert_uint_eq(i1, i2);
    ck_assert_uint_eq(i1, dim1);


    //2nd dimension
    i1 = value.arrayDimensions[1];
    i2 = copiedValue.arrayDimensions[1];
    ck_assert_int_eq(i1, i2);
    ck_assert_int_eq(i1, dim2);


    for(UA_Int32 i = 0;i < 6;i++) {
        i1 = ((UA_UInt32 *)value.data)[i];
        i2 = ((UA_UInt32 *)copiedValue.data)[i];
        ck_assert_int_eq(i1, i2);
        ck_assert_int_eq(i2, i);
    }

    ck_assert_uint_eq(value.arrayDimensionsSize, copiedValue.arrayDimensionsSize);
    ck_assert_uint_eq(value.arrayLength, copiedValue.arrayLength);

    //finally
    UA_Variant_clear(&value);
    UA_Variant_clear(&copiedValue);
}
END_TEST

START_TEST(UA_ExtensionObject_encodeDecodeShallWorkOnExtensionObject) {
    /* UA_Int32 val = 42; */
    /* UA_VariableAttributes varAttr; */
    /* UA_VariableAttributes_init(&varAttr); */
    /* varAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId; */
    /* UA_Variant_init(&varAttr.value); */
    /* varAttr.value.type = &UA_TYPES[UA_TYPES_INT32]; */
    /* varAttr.value.data = &val; */
    /* varAttr.value.arrayLength = -1; */
    /* varAttr.userWriteMask = 41; */
    /* varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DATATYPE; */
    /* varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_VALUE; */
    /* varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_USERWRITEMASK; */

    /* /\* wrap it into an extension object attributes *\/ */
    /* UA_ExtensionObject extensionObject; */
    /* UA_ExtensionObject_init(&extensionObject); */
    /* extensionObject.typeId = UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES].typeId; */
    /* UA_Byte extensionData[50]; */
    /* extensionObject.body = (UA_ByteString){.data = extensionData, .length=50}; */
    /* size_t posEncode = 0; */
    /* UA_VariableAttributes_encodeBinary(&varAttr, &extensionObject.body, posEncode); */
    /* extensionObject.body.length = posEncode; */
    /* extensionObject.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING; */

    /* UA_Byte data[50]; */
    /* UA_ByteString dst = {.data = data, .length=50}; */

    /* posEncode = 0; */
    /* UA_ExtensionObject_encodeBinary(&extensionObject, &dst, posEncode); */

    /* UA_ExtensionObject extensionObjectDecoded; */
    /* size_t posDecode = 0; */
    /* UA_ExtensionObject_decodeBinary(&dst, &posDecode, &extensionObjectDecoded); */

    /* ck_assert_uint_eq(posEncode, posDecode); */
    /* ck_assert_uint_eq(extensionObjectDecoded.body.length, extensionObject.body.length); */

    /* UA_VariableAttributes varAttrDecoded; */
    /* UA_VariableAttributes_init(&varAttrDecoded); */
    /* posDecode = 0; */
    /* UA_VariableAttributes_decodeBinary(&extensionObjectDecoded.body, &posDecode, &varAttrDecoded); */
    /* ck_assert_uint_eq(41, varAttrDecoded.userWriteMask); */
    /* ck_assert_uint_eq(-1, varAttrDecoded.value.arrayLength); */

    /* // finally */
    /* UA_ExtensionObject_clear(&extensionObjectDecoded); */
    /* UA_Variant_clear(&varAttrDecoded.value); */
}
END_TEST

START_TEST(UA_StatusCode_utils) {

    ck_assert(UA_TRUE == UA_StatusCode_isBad(UA_STATUSCODE_BADINTERNALERROR));
    ck_assert(UA_TRUE == UA_StatusCode_isBad(UA_STATUSCODE_BADOUTOFMEMORY));
    ck_assert(UA_TRUE == UA_StatusCode_isBad(UA_STATUSCODE_BADTIMEOUT));

    ck_assert(UA_FALSE == UA_StatusCode_isBad(UA_STATUSCODE_GOOD));
    ck_assert(UA_FALSE == UA_StatusCode_isBad(UA_STATUSCODE_GOODNODATA));
    ck_assert(UA_FALSE == UA_StatusCode_isBad(UA_STATUSCODE_GOODOVERLOAD));

    ck_assert(UA_TRUE == UA_StatusCode_isBad((UA_StatusCode) -1));
    ck_assert(UA_FALSE == UA_StatusCode_isBad((UA_StatusCode) 1));

} END_TEST

static Suite *testSuite_builtin(void) {
    Suite *s = suite_create("Built-in Data Types 62541-6 Table 1");

    TCase *tc_decode = tcase_create("decode");
    tcase_add_test(tc_decode, UA_Byte_decodeShallCopyAndAdvancePosition);
    tcase_add_test(tc_decode, UA_Byte_decodeShallModifyOnlyCurrentPosition);
    tcase_add_test(tc_decode, UA_Int16_decodeShallAssumeLittleEndian);
    tcase_add_test(tc_decode, UA_Int16_decodeShallRespectSign);
    tcase_add_test(tc_decode, UA_UInt16_decodeShallNotRespectSign);
    tcase_add_test(tc_decode, UA_Int32_decodeShallAssumeLittleEndian);
    tcase_add_test(tc_decode, UA_Int32_decodeShallRespectSign);
    tcase_add_test(tc_decode, UA_UInt32_decodeShallNotRespectSign);
    tcase_add_test(tc_decode, UA_UInt64_decodeShallNotRespectSign);
    tcase_add_test(tc_decode, UA_Int64_decodeShallRespectSign);
    tcase_add_test(tc_decode, UA_Float_decodeShallWorkOnExample);
    tcase_add_test(tc_decode, UA_Double_decodeShallGiveOne);
    tcase_add_test(tc_decode, UA_Double_decodeShallGiveZero);
    tcase_add_test(tc_decode, UA_Double_decodeShallGiveMinusTwo);
    tcase_add_test(tc_decode, UA_Double_decodeShallGive2147483648);
    tcase_add_test(tc_decode, UA_Byte_encode_test);
    tcase_add_test(tc_decode, UA_String_decodeShallAllocateMemoryAndCopyString);
    tcase_add_test(tc_decode, UA_String_decodeWithNegativeSizeShallNotAllocateMemoryAndNullPtr);
    tcase_add_test(tc_decode, UA_String_decodeWithZeroSizeShallNotAllocateMemoryAndNullPtr);
    tcase_add_test(tc_decode, UA_NodeId_decodeTwoByteShallReadTwoBytesAndSetNamespaceToZero);
    tcase_add_test(tc_decode, UA_NodeId_decodeFourByteShallReadFourBytesAndRespectNamespace);
    tcase_add_test(tc_decode, UA_NodeId_decodeStringShallAllocateMemory);
    tcase_add_test(tc_decode, UA_Variant_decodeSingleExtensionObjectShallSetVTAndAllocateMemory);
    tcase_add_test(tc_decode, UA_Variant_decodeWithOutArrayFlagSetShallSetVTAndAllocateMemoryForArray);
    tcase_add_test(tc_decode, UA_Variant_decodeWithArrayFlagSetShallSetVTAndAllocateMemoryForArray);
    tcase_add_test(tc_decode, UA_Variant_decodeWithOutDeleteMembersShallFailInCheckMem);
    tcase_add_test(tc_decode, UA_Variant_decodeWithTooSmallSourceShallReturnWithError);
    suite_add_tcase(s, tc_decode);

    TCase *tc_encode = tcase_create("encode");
    tcase_add_test(tc_encode, UA_Byte_encode_test);
    tcase_add_test(tc_encode, UA_UInt16_encodeNegativeShallEncodeLittleEndian);
    tcase_add_test(tc_encode, UA_UInt16_encodeShallEncodeLittleEndian);
    tcase_add_test(tc_encode, UA_UInt32_encodeShallEncodeLittleEndian);
    tcase_add_test(tc_encode, UA_Int32_encodeShallEncodeLittleEndian);
    tcase_add_test(tc_encode, UA_Int32_encodeNegativeShallEncodeLittleEndian);
    tcase_add_test(tc_encode, UA_UInt64_encodeShallWorkOnExample);
    tcase_add_test(tc_encode, UA_Int64_encodeNegativeShallEncodeLittleEndian);
    tcase_add_test(tc_encode, UA_Int64_encodeShallEncodeLittleEndian);
    tcase_add_test(tc_encode, UA_Float_encodeShallWorkOnExample);
    tcase_add_test(tc_encode, UA_Double_encodeShallWorkOnExample);
    tcase_add_test(tc_encode, UA_String_encodeShallWorkOnExample);
    tcase_add_test(tc_encode, UA_ExpandedNodeId_encodeShallWorkOnExample);
    tcase_add_test(tc_encode, UA_DataValue_encodeShallWorkOnExampleWithoutVariant);
    tcase_add_test(tc_encode, UA_DataValue_encodeShallWorkOnExampleWithVariant);
    tcase_add_test(tc_encode, UA_ExtensionObject_encodeDecodeShallWorkOnExtensionObject);
    suite_add_tcase(s, tc_encode);

    TCase *tc_convert = tcase_create("convert");
    tcase_add_test(tc_convert, UA_DateTime_toStructShallWorkOnExample);
    tcase_add_test(tc_convert, UA_DateTime_toStructAndBack);
    suite_add_tcase(s, tc_convert);

    TCase *tc_equal = tcase_create("equal");
    tcase_add_test(tc_equal, UA_QualifiedName_equalShallWorkOnExample);
    suite_add_tcase(s, tc_equal);

    TCase *tc_hash = tcase_create("hash");
    tcase_add_test(tc_hash, UA_ExpandedNodeId_hashIdentical);
    suite_add_tcase(s, tc_hash);

    TCase *tc_copy = tcase_create("copy");
    tcase_add_test(tc_copy, UA_Array_copyByteArrayShallWorkOnExample);
    tcase_add_test(tc_copy, UA_Array_copyUA_StringShallWorkOnExample);
    tcase_add_test(tc_copy, UA_ExtensionObject_copyShallWorkOnExample);
    tcase_add_test(tc_copy, UA_Variant_copyShallWorkOnSingleValueExample);
    tcase_add_test(tc_copy, UA_Variant_copyShallWorkOn1DArrayExample);
    tcase_add_test(tc_copy, UA_Variant_copyShallWorkOn2DArrayExample);
    tcase_add_test(tc_copy, UA_Variant_copyShallWorkOnByteStringIndexRange);

    tcase_add_test(tc_copy, UA_DiagnosticInfo_copyShallWorkOnExample);
    tcase_add_test(tc_copy, UA_ApplicationDescription_copyShallWorkOnExample);
    tcase_add_test(tc_copy, UA_QualifiedName_copyShallWorkOnInputExample);
    tcase_add_test(tc_copy, UA_Guid_copyShallWorkOnInputExample);
    tcase_add_test(tc_copy, UA_LocalizedText_copycstringShallWorkOnInputExample);
    tcase_add_test(tc_copy, UA_DataValue_copyShallWorkOnInputExample);
    suite_add_tcase(s, tc_copy);

    TCase *tc_utils = tcase_create("utils");
    tcase_add_test(tc_utils, UA_StatusCode_utils);
    suite_add_tcase(s, tc_utils);

    return s;
}

int main(void) {
    int      number_failed = 0;
    Suite   *s;
    SRunner *sr;

    s  = testSuite_builtin();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
