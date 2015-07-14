#include <stdio.h>
#include <stdlib.h>
#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated.h"
//#include "ua_transport.h"
#include "ua_util.h"
#include "check.h"

/* copied here from encoding_binary.c */
enum UA_VARIANT_ENCODINGMASKTYPE_enum {
    UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK = 0x3F,            // bits 0:5
    UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS  = (0x01 << 6),     // bit 6
    UA_VARIANT_ENCODINGMASKTYPE_ARRAY       = (0x01 << 7)      // bit 7
};

START_TEST(UA_ExtensionObject_calcSizeShallWorkOnExample) {
    // given
    UA_Byte data[3] = { 1, 2, 3 };
    UA_ExtensionObject extensionObject;

    // empty ExtensionObject, handcoded
    // when
    UA_ExtensionObject_init(&extensionObject);
    extensionObject.typeId.identifierType = UA_NODEIDTYPE_NUMERIC;
    extensionObject.typeId.identifier.numeric = 0;
    extensionObject.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED;
    // then
    ck_assert_int_eq(UA_calcSizeBinary(&extensionObject, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), 1 + 1 + 1);

    // ExtensionObject with ByteString-Body
    // when
    extensionObject.encoding    = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
    extensionObject.body.data   = data;
    extensionObject.body.length = 3;
    // then
    ck_assert_int_eq(UA_calcSizeBinary(&extensionObject, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]), 3 + 4 + 3);
}
END_TEST

START_TEST(UA_DataValue_calcSizeShallWorkOnExample) {
    // given
    UA_DataValue dataValue;
    UA_DataValue_init(&dataValue);
    dataValue.status       = 12;
    dataValue.hasStatus = UA_TRUE;
    dataValue.sourceTimestamp = 80;
    dataValue.hasSourceTimestamp = UA_TRUE;
    dataValue.sourcePicoseconds = 214;
    dataValue.hasSourcePicoseconds = UA_TRUE;
    int size = 0;
    // when
    size = UA_calcSizeBinary(&dataValue, &UA_TYPES[UA_TYPES_DATAVALUE]);
    // then
    // 1 (bitfield) + 4 (status) + 8 (timestamp) + 2 (picoseconds)
    ck_assert_int_eq(size, 15);
}
END_TEST

START_TEST(UA_DiagnosticInfo_calcSizeShallWorkOnExample) {
    // given
    UA_DiagnosticInfo diagnosticInfo;
    UA_DiagnosticInfo_init(&diagnosticInfo);
    diagnosticInfo.symbolicId    = 30;
    diagnosticInfo.hasSymbolicId = UA_TRUE;
    diagnosticInfo.namespaceUri  = 25;
    diagnosticInfo.hasNamespaceUri = UA_TRUE;
    diagnosticInfo.localizedText = 22;
    diagnosticInfo.hasLocalizedText = UA_TRUE;
    UA_Byte additionalInfoData = 'd';
    diagnosticInfo.additionalInfo.data = &additionalInfoData; //"OPCUA";
    diagnosticInfo.additionalInfo.length = 1;
    diagnosticInfo.hasAdditionalInfo = UA_TRUE;
    // when & then
    // 1 (bitfield) + 4 (symbolic id) + 4 (namespaceuri) + 4 (localizedtext) + 5 (additionalinfo)
    ck_assert_int_eq(UA_calcSizeBinary(&diagnosticInfo, &UA_TYPES[UA_TYPES_DIAGNOSTICINFO]), 18);
}
END_TEST

START_TEST(UA_String_calcSizeWithNegativLengthShallReturnEncodingSize) {
    // given
    UA_String arg = { -1, UA_NULL };
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_STRING]);
    // then
    ck_assert_int_eq(encodingSize, 4);
}
END_TEST

START_TEST(UA_String_calcSizeWithNegativLengthAndValidPointerShallReturnEncodingSize) {
    // given
    UA_String arg = { -1, (UA_Byte *)"OPC" };
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_STRING]);
    // then
    ck_assert_int_eq(encodingSize, 4);
}
END_TEST

START_TEST(UA_String_calcSizeWithZeroLengthShallReturnEncodingSize) {
    // given
    UA_String arg = { 0, UA_NULL };
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_STRING]);
    // then
    ck_assert_int_eq(encodingSize, 4);
}
END_TEST

START_TEST(UA_String_calcSizeWithZeroLengthAndValidPointerShallReturnEncodingSize) {
    // given
    UA_String arg = { 0, (UA_Byte *)"OPC" };
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_STRING]);
    // then
    ck_assert_int_eq(encodingSize, 4);
}
END_TEST

START_TEST(UA_String_calcSizeShallReturnEncodingSize) {
    // given
    UA_String arg = { 3, (UA_Byte *)"OPC" };
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_STRING]);
    // then
    ck_assert_int_eq(encodingSize, 4+3);
}
END_TEST

START_TEST(UA_NodeId_calcSizeEncodingTwoByteShallReturnEncodingSize) {
    // given
    UA_NodeId arg;
    arg.identifierType = UA_NODEIDTYPE_NUMERIC;
    arg.namespaceIndex = 0;
    arg.identifier.numeric = 1;
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(encodingSize, 2);
}
END_TEST

START_TEST(UA_NodeId_calcSizeEncodingFourByteShallReturnEncodingSize) {
    // given
    UA_NodeId arg;
    arg.identifierType = UA_NODEIDTYPE_NUMERIC;
    arg.namespaceIndex = 1;
    arg.identifier.numeric = 1;
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(encodingSize, 4);
}
END_TEST

START_TEST(UA_NodeId_calcSizeEncodingStringShallReturnEncodingSize) {
    // given
    UA_NodeId arg;
    arg.identifierType = UA_NODEIDTYPE_STRING;
    arg.identifier.string.length = 3;
    arg.identifier.string.data   = (UA_Byte *)"PLT";
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(encodingSize, 1+2+4+3);
}
END_TEST

START_TEST(UA_NodeId_calcSizeEncodingStringNegativLengthShallReturnEncodingSize) {
    // given
    UA_NodeId arg;
    arg.identifierType = UA_NODEIDTYPE_STRING;
    arg.identifier.string.length = -1;
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(encodingSize, 1+2+4+0);
}
END_TEST

START_TEST(UA_NodeId_calcSizeEncodingStringZeroLengthShallReturnEncodingSize) {
    // given
    UA_NodeId arg;
    arg.identifierType = UA_NODEIDTYPE_STRING;
    arg.identifier.string.length = 0;
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_NODEID]);
    // then
    ck_assert_int_eq(encodingSize, 1+2+4+0);
}
END_TEST

START_TEST(UA_ExpandedNodeId_calcSizeEncodingStringAndServerIndexShallReturnEncodingSize) {
    // given
    UA_ExpandedNodeId arg;
    UA_ExpandedNodeId_init(&arg);
    arg.nodeId.identifierType = UA_NODEIDTYPE_STRING;
    arg.serverIndex = 1;
    arg.nodeId.identifier.string.length = 3;
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    // then
    ck_assert_int_eq(encodingSize, 1+2+4+3+4);
}
END_TEST

START_TEST(UA_ExpandedNodeId_calcSizeEncodingStringAndNamespaceUriShallReturnEncodingSize) {
    // given
    UA_ExpandedNodeId arg;
    UA_ExpandedNodeId_init(&arg);
    arg.nodeId.identifierType = UA_NODEIDTYPE_STRING;
    arg.nodeId.identifier.string.length = 3;
    arg.namespaceUri.length = 7;
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    // then
    ck_assert_int_eq(encodingSize, 1+2+4+3+4+7);
}
END_TEST

START_TEST(UA_Guid_calcSizeShallReturnEncodingSize) {
    // given
    UA_Guid   arg;
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_GUID]);
    // then
    ck_assert_int_eq(encodingSize, 16);
}
END_TEST

START_TEST(UA_LocalizedText_calcSizeTextOnlyShallReturnEncodingSize) {
    // given
    UA_LocalizedText arg;
    UA_LocalizedText_init(&arg);
    arg.text = (UA_String) {8, (UA_Byte *)"12345678"};
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    // then
    ck_assert_int_eq(encodingSize, 1+4+8);
    // finally
    UA_LocalizedText_init(&arg); // do not delete text
    UA_LocalizedText_deleteMembers(&arg);
}
END_TEST

START_TEST(UA_LocalizedText_calcSizeLocaleOnlyShallReturnEncodingSize) {
    // given
    UA_LocalizedText arg;
    UA_LocalizedText_init(&arg);
    arg.locale = (UA_String) {8, (UA_Byte *)"12345678"};
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    // then
    ck_assert_int_eq(encodingSize, 1+4+8);
    UA_LocalizedText_init(&arg); // do not delete locale
    UA_LocalizedText_deleteMembers(&arg);
}
END_TEST

START_TEST(UA_LocalizedText_calcSizeTextAndLocaleShallReturnEncodingSize) {
    // given
    UA_LocalizedText arg;
    UA_LocalizedText_init(&arg);
    arg.locale = (UA_String) {8, (UA_Byte *)"12345678"};
    arg.text = (UA_String) {8, (UA_Byte *)"12345678"};
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    // then
    ck_assert_int_eq(encodingSize, 1+4+8+4+8);
    UA_LocalizedText_init(&arg); // do not delete locale and text
    UA_LocalizedText_deleteMembers(&arg);
}
END_TEST

START_TEST(UA_Variant_calcSizeFixedSizeArrayShallReturnEncodingSize) {
    // given
    UA_Variant arg;
    UA_Variant_init(&arg);
    arg.type = &UA_TYPES[UA_TYPES_INT32];
#define ARRAY_LEN 8
    arg.arrayLength = ARRAY_LEN;
    UA_Int32 *data[ARRAY_LEN];
    arg.data = (void *)data;

    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_VARIANT]);

    // then
    ck_assert_int_eq(encodingSize, 1+4+ARRAY_LEN*4);
#undef ARRAY_LEN
}
END_TEST

START_TEST(UA_Variant_calcSizeVariableSizeArrayShallReturnEncodingSize) {
    // given
    UA_Variant arg;
    UA_Variant_init(&arg);
    arg.type = &UA_TYPES[UA_TYPES_STRING];
#define ARRAY_LEN 3
    arg.arrayLength = ARRAY_LEN;
    UA_String strings[3];
    strings[0] = (UA_String) {-1, UA_NULL };
    strings[1] = (UA_String) {3, (UA_Byte *)"PLT" };
    strings[2] = (UA_String) {47, UA_NULL };
    arg.data   = (void *)strings;
    // when
    UA_UInt32 encodingSize = UA_calcSizeBinary(&arg, &UA_TYPES[UA_TYPES_VARIANT]);
    // then
    ck_assert_int_eq(encodingSize, 1+4+(4+0)+(4+3)+(4+47));
#undef ARRAY_LEN
}
END_TEST

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
    ck_assert_int_eq(pos, 1);
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
    ck_assert_int_eq(pos, 4);
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
    ck_assert_int_eq(pos, 4);
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
    ck_assert_int_eq(pos, 8);
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
    ck_assert_int_eq(pos, 8);
    ck_assert_uint_eq(val_ff_ff, (UA_UInt32)( (0x01LL << 32 ) - 1 ));
    ck_assert_uint_eq(val_00_80, (UA_UInt32)(0x01 << 31));
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
    UA_Int64 expectedVal = 0xFF;
    expectedVal = expectedVal << 56;
    UA_Byte  mem[8]      = { 00, 00, 00, 00, 0x00, 0x00, 0x00, 0xFF };
    rawMessage.data   = mem;
    rawMessage.length = 8;

    size_t pos = 0;
    UA_Int64 val;
    // when
    UA_Int64_decodeBinary(&rawMessage, &pos, &val);
    //then
    ck_assert_uint_eq(val, expectedVal);
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
    ck_assert_int_eq(pos, 4);
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
    ck_assert_int_eq(pos, 8);
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
    ck_assert_int_eq(pos, 8);
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
    ck_assert_int_eq(pos, 8);
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
    ck_assert_int_eq(pos, 8);
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
    ck_assert_int_eq(dst.length, 8);
    ck_assert_int_eq(dst.data[3], 'L');
    // finally
    UA_String_deleteMembers(&dst);
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
    ck_assert_int_eq(dst.length, -1);
    ck_assert_ptr_eq(dst.data, UA_NULL);
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
    ck_assert_int_eq(dst.length, 0);
    ck_assert_ptr_eq(dst.data, UA_NULL);
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
    ck_assert_int_eq(pos, 2);
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
    ck_assert_int_eq(pos, 4);
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
    ck_assert_int_eq(pos, 10);
    ck_assert_int_eq(dst.identifierType, UA_NODEIDTYPE_STRING);
    ck_assert_int_eq(dst.namespaceIndex, 1);
    ck_assert_int_eq(dst.identifier.string.length, 3);
    ck_assert_int_eq(dst.identifier.string.data[1], 'L');
    // finally
    UA_NodeId_deleteMembers(&dst);
}
END_TEST

START_TEST(UA_Variant_decodeWithOutArrayFlagSetShallSetVTAndAllocateMemoryForArray) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { UA_TYPES[UA_TYPES_INT32].typeId.identifier.numeric, 0xFF, 0x00, 0x00, 0x00 };
    UA_ByteString src = { 5, data };
    UA_Variant dst;
    // when
    UA_StatusCode retval = UA_Variant_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(pos, 5);
    //ck_assert_ptr_eq((const void *)dst.type, (const void *)&UA_TYPES[UA_TYPES_INT32]); //does not compile in gcc 4.6
    ck_assert_int_eq((uintptr_t)dst.type, (uintptr_t)&UA_TYPES[UA_TYPES_INT32]); 
    ck_assert_int_eq(dst.arrayLength, -1);
    ck_assert_int_eq(*(UA_Int32 *)dst.data, 255);
    // finally
    UA_Variant_deleteMembers(&dst);
}
END_TEST

START_TEST(UA_Variant_decodeWithArrayFlagSetShallSetVTAndAllocateMemoryForArray) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { UA_TYPES[UA_TYPES_INT32].typeId.identifier.numeric | UA_VARIANT_ENCODINGMASKTYPE_ARRAY,
                       0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF,
                       0xFF, 0xFF };
    UA_ByteString src = { 13, data };
    UA_Variant dst;
    // when
    UA_StatusCode retval = UA_Variant_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(pos, 1+4+2*4);
    //ck_assert_ptr_eq((const (void*))dst.type, (const void*)&UA_TYPES[UA_TYPES_INT32]); //does not compile in gcc 4.6
    ck_assert_int_eq((uintptr_t)dst.type,(uintptr_t)&UA_TYPES[UA_TYPES_INT32]);
    ck_assert_int_eq(dst.arrayLength, 2);
    ck_assert_int_eq(((UA_Int32 *)dst.data)[0], 255);
    ck_assert_int_eq(((UA_Int32 *)dst.data)[1], -1);
    // finally
    UA_Variant_deleteMembers(&dst);
}
END_TEST

START_TEST(UA_Variant_decodeWithOutDeleteMembersShallFailInCheckMem) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { UA_TYPES[UA_TYPES_INT32].typeId.identifier.numeric | UA_VARIANT_ENCODINGMASKTYPE_ARRAY,
                       0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF };
    UA_ByteString src = { 13, data };
    UA_Variant dst;
    // when
    UA_StatusCode retval = UA_Variant_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    // finally
    UA_Variant_deleteMembers(&dst);
}
END_TEST

START_TEST(UA_Variant_decodeWithTooSmallSourceShallReturnWithError) {
    // given
    size_t pos = 0;
    UA_Byte data[] = { UA_TYPES[UA_TYPES_INT32].typeId.identifier.numeric | UA_VARIANT_ENCODINGMASKTYPE_ARRAY,
                       0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF };
    UA_ByteString src = { 4, data };

    UA_Variant dst;
    // when
    UA_StatusCode retval = UA_Variant_decodeBinary(&src, &pos, &dst);
    // then
    ck_assert_int_ne(retval, UA_STATUSCODE_GOOD);
    // finally
    UA_Variant_deleteMembers(&dst);
}
END_TEST

START_TEST(UA_Byte_encode_test) {
    // given
    UA_Byte src;
    UA_Byte data[]    = { 0x00, 0xFF };
    UA_ByteString dst = { 2, data };

    UA_Int32 retval = 0;
    size_t pos = 0;

    ck_assert_uint_eq(dst.data[1], 0xFF);

    src    = 8;
    retval = UA_Byte_encodeBinary(&src, &dst, &pos);

    ck_assert_uint_eq(dst.data[0], 0x08);
    ck_assert_uint_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(pos, 1);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // Test2
    // given
    src = 0xFF;
    dst.data[1] = 0x00;
    pos         = 0;
    retval      = UA_Byte_encodeBinary(&src, &dst, &pos);

    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0x00);
    ck_assert_int_eq(pos, 1);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

}
END_TEST

START_TEST(UA_UInt16_encodeNegativeShallEncodeLittleEndian) {
    // given
    UA_UInt16     src;
    UA_Byte       data[] = { 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst    = { 4, data };

    UA_StatusCode retval = 0;
    size_t pos = 0;

    // when test 1
    src    = -1;
    retval = UA_UInt16_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 2);
    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = -32768;
    retval = UA_UInt16_encodeBinary(&src, &dst, &pos);
    // then test 2
    ck_assert_int_eq(pos, 4);
    ck_assert_int_eq(dst.data[2], 0x00);
    ck_assert_int_eq(dst.data[3], 0x80);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_UInt16_encodeShallEncodeLittleEndian) {
    // given
    UA_UInt16     src;
    UA_Byte       data[] = {  0x55, 0x55,
            0x55,       0x55 };
    UA_ByteString dst    = { 4, data };

    UA_StatusCode retval = 0;
    size_t pos = 0;

    // when test 1
    src    = 0;
    retval = UA_UInt16_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 2);
    ck_assert_int_eq(dst.data[0], 0x00);
    ck_assert_int_eq(dst.data[1], 0x00);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = 32767;
    retval = UA_UInt16_encodeBinary(&src, &dst, &pos);
    // then test 2
    ck_assert_int_eq(pos, 4);
    ck_assert_int_eq(dst.data[2], 0xFF);
    ck_assert_int_eq(dst.data[3], 0x7F);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_UInt32_encodeShallEncodeLittleEndian) {
    // given
    UA_UInt32     src;
    UA_Byte       data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst    = { 8, data };

    UA_StatusCode retval = 0;
    size_t pos = 0;

    // when test 1
    src    = -1;
    retval = UA_UInt32_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 4);
    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(dst.data[2], 0xFF);
    ck_assert_int_eq(dst.data[3], 0xFF);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = 0x0101FF00;
    retval = UA_UInt32_encodeBinary(&src, &dst, &pos);
    // then test 2
    ck_assert_int_eq(pos, 8);
    ck_assert_int_eq(dst.data[4], 0x00);
    ck_assert_int_eq(dst.data[5], 0xFF);
    ck_assert_int_eq(dst.data[6], 0x01);
    ck_assert_int_eq(dst.data[7], 0x01);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_Int32_encodeShallEncodeLittleEndian) {
    // given
    UA_Int32 src;
    UA_Byte  data[]   = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst = { 8, data };

    UA_Int32 retval = 0;
    size_t pos = 0;

    // when test 1
    src    = 1;
    retval = UA_Int32_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 4);
    ck_assert_int_eq(dst.data[0], 0x01);
    ck_assert_int_eq(dst.data[1], 0x00);
    ck_assert_int_eq(dst.data[2], 0x00);
    ck_assert_int_eq(dst.data[3], 0x00);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    // when test 2
    src    = 0x7FFFFFFF;
    retval = UA_Int32_encodeBinary(&src, &dst, &pos);
    // then test 2
    ck_assert_int_eq(pos, 8);
    ck_assert_int_eq(dst.data[4], 0xFF);
    ck_assert_int_eq(dst.data[5], 0xFF);
    ck_assert_int_eq(dst.data[6], 0xFF);
    ck_assert_int_eq(dst.data[7], 0x7F);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_Int32_encodeNegativeShallEncodeLittleEndian) {
    // given
    UA_Int32 src;
    UA_Byte  data[]   = {  0x55, 0x55,    0x55,  0x55,
            0x55,  0x55,    0x55,  0x55 };
    UA_ByteString dst = { 8, data };

    UA_Int32  retval  = 0;
    size_t pos = 0;

    // when test 1
    src    = -1;
    retval = UA_Int32_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 4);
    ck_assert_int_eq(dst.data[0], 0xFF);
    ck_assert_int_eq(dst.data[1], 0xFF);
    ck_assert_int_eq(dst.data[2], 0xFF);
    ck_assert_int_eq(dst.data[3], 0xFF);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_UInt64_encodeShallWorkOnExample) {
    // given
    UA_UInt64     src;
    UA_Byte       data[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                             0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 };
    UA_ByteString dst    = { 16, data };

    UA_StatusCode retval = 0;
    size_t pos    = 0;

    // when test 1
    src    = -1;
    retval = UA_UInt64_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 8);
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
    retval = UA_UInt64_encodeBinary(&src, &dst, &pos);
    // then test 2
    ck_assert_int_eq(pos, 16);
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
    UA_Int64 src;
    UA_Byte  data[]   = {  0x55, 0x55,    0x55,  0x55,    0x55,    0x55,  0x55,       0x55,
            0x55,  0x55,    0x55,  0x55,    0x55,    0x55,  0x55,       0x55 };
    UA_ByteString dst = { 16, data };

    UA_Int32  retval  = 0;
    size_t pos     = 0;

    // when test 1
    src    = 0x7F0033AA44EE6611;
    retval = UA_Int64_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 8);
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
    UA_Int64 src;
    UA_Byte  data[]   = {  0x55, 0x55,    0x55,  0x55,    0x55,    0x55,  0x55,       0x55,
            0x55,  0x55,    0x55,  0x55,    0x55,    0x55,  0x55,       0x55 };
    UA_ByteString dst = { 16, data };

    UA_Int32  retval  = 0;
    size_t pos     = 0;

    // when test 1
    src    = -1;
    retval = UA_Int64_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 8);
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
    // given
    UA_Float src;
    UA_Byte  data[]   = {  0x55, 0x55,    0x55,  0x55,    0x55,    0x55,  0x55,       0x55,
            0x55,  0x55,    0x55,  0x55,    0x55,    0x55,  0x55,       0x55 };
    UA_ByteString dst = { 16, data };

    UA_Int32  retval  = 0;
    size_t pos     = 0;

    // when test 1
    src    = -6.5;
    retval = UA_Float_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 4);
    ck_assert_int_eq(dst.data[2], 0xD0);
    ck_assert_int_eq(dst.data[3], 0xC0);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(UA_Double_encodeShallWorkOnExample)
   {
    // given
    UA_Double src;
    UA_Byte data[] = {  0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
                        0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55
                    };
    UA_ByteString dst = {16,data};

    UA_Int32 retval;
    size_t pos = 0;

    // when test 1
    src = -6.5;
    retval = UA_Double_encodeBinary(&src, &dst, &pos);
    // then test 1
    ck_assert_int_eq(pos, 8);
    ck_assert_int_eq(dst.data[6], 0x1A);
    ck_assert_int_eq(dst.data[7], 0xC0);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
   }
END_TEST

START_TEST(UA_String_encodeShallWorkOnExample) {
    // given
    UA_String src;
    src.length = 11;
    UA_Byte   mem[11] = "ACPLT OPCUA";
    src.data   = mem;

    UA_Byte data[] = {  0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,      0x55,
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,      0x55,
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,      0x55 };
    UA_ByteString dst = { 24, data };

    UA_Int32  retval  = 0;
    size_t pos     = 0;

    // when
    retval = UA_String_encodeBinary(&src, &dst, &pos);
    // then
    ck_assert_int_eq(pos, sizeof(UA_Int32)+11);
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

START_TEST(UA_DataValue_encodeShallWorkOnExampleWithoutVariant) {
    // given
    UA_DataValue src;
    UA_DataValue_init(&src);
    src.serverTimestamp = 80;
    src.hasServerTimestamp = UA_TRUE;

    UA_Byte data[] = {  0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,       0x55,
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,       0x55,
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,       0x55 };
    UA_ByteString dst = { 24, data };

    UA_Int32  retval  = 0;
    size_t pos     = 0;

    // when
    retval = UA_DataValue_encodeBinary(&src, &dst, &pos);
    // then
    ck_assert_int_eq(pos, 9);            // represents the length
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
    src.hasValue = UA_TRUE;
    src.hasServerTimestamp = UA_TRUE;
    src.value.type = &UA_TYPES[UA_TYPES_INT32];
    src.value.arrayLength  = -1; // one element (encoded as not an array)
    UA_Int32  vdata  = 45;
    src.value.data = (void *)&vdata;

    UA_Byte data[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    UA_ByteString dst = { 24, data };

    UA_Int32  retval  = 0;
    size_t pos     = 0;

    // when
    retval = UA_DataValue_encodeBinary(&src, &dst, &pos);
    // then
    ck_assert_int_eq(pos, 1+(1+4)+8);           // represents the length
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

START_TEST(UA_DateTime_toStringShallWorkOnExample) {
    // given
    UA_DateTime src = 13974671891234567 + (11644473600 * 10000000); // ua counts since 1601, unix since 1970
    //1397467189... is Mon, 14 Apr 2014 09:19:49 GMT
    //...1234567 are the milli-, micro- and nanoseconds

    UA_String dst;

    // when
    UA_DateTime_toString(src, &dst);
    // then
    ck_assert_int_eq(dst.data[0], '0');
    ck_assert_int_eq(dst.data[1], '4');
    ck_assert_int_eq(dst.data[2], '/');
    ck_assert_int_eq(dst.data[3], '1');
    ck_assert_int_eq(dst.data[4], '4');
    UA_String_deleteMembers(&dst);
}
END_TEST

START_TEST(UA_ExtensionObject_copyShallWorkOnExample) {
    // given
    UA_Byte data[3] = { 1, 2, 3 };

    UA_ExtensionObject value, valueCopied;
    UA_ExtensionObject_init(&value);
    UA_ExtensionObject_init(&valueCopied);

    value.typeId = UA_TYPES[UA_TYPES_BYTE].typeId;
    value.encoding    = UA_EXTENSIONOBJECT_ENCODINGMASK_NOBODYISENCODED;
    value.encoding    = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
    value.body.data   = data;
    value.body.length = 3;

    //when
    UA_ExtensionObject_copy(&value, &valueCopied);

    for(UA_Int32 i = 0;i < 3;i++)
        ck_assert_int_eq(valueCopied.body.data[i], value.body.data[i]);

    ck_assert_int_eq(valueCopied.encoding, value.encoding);
    ck_assert_int_eq(valueCopied.typeId.identifierType, value.typeId.identifierType);
    ck_assert_int_eq(valueCopied.typeId.identifier.numeric, value.typeId.identifier.numeric);

    //finally
    value.body.data = UA_NULL; // we cannot free the static string
    UA_ExtensionObject_deleteMembers(&value);
    UA_ExtensionObject_deleteMembers(&valueCopied);
}
END_TEST

START_TEST(UA_Array_copyByteArrayShallWorkOnExample) {
    //given
    UA_String testString;
    UA_Byte  *dstArray;
    UA_Int32  size = 5;
    UA_Int32  i    = 0;
    testString.data = UA_malloc(size);
    testString.data[0] = 'O';
    testString.data[1] = 'P';
    testString.data[2] = 'C';
    testString.data[3] = 'U';
    testString.data[4] = 'A';
    testString.length  = 5;

    //when
    UA_Array_copy((const void *)testString.data, (void **)&dstArray, &UA_TYPES[UA_TYPES_BYTE], 5);
    //then
    for(i = 0;i < size;i++)
        ck_assert_int_eq(testString.data[i], dstArray[i]);

    //finally
    UA_String_deleteMembers(&testString);
    UA_free((void *)dstArray);

}
END_TEST

START_TEST(UA_Array_copyUA_StringShallWorkOnExample) {
    // given
    UA_Int32   i, j;
    UA_String *srcArray = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], 3);
    UA_String *dstArray;

    srcArray[0] = UA_STRING_ALLOC("open");
    srcArray[1] = UA_STRING_ALLOC("62541");
    srcArray[2] = UA_STRING_ALLOC("opc ua");
    //when
    UA_Array_copy((const void *)srcArray, (void **)&dstArray, &UA_TYPES[UA_TYPES_STRING], 3);
    //then
    for(i = 0;i < 3;i++) {
        for(j = 0;j < 3;j++)
            ck_assert_int_eq(srcArray[i].data[j], dstArray[i].data[j]);
        ck_assert_int_eq(srcArray[i].length, dstArray[i].length);
    }
    //finally
    UA_Array_delete(srcArray, &UA_TYPES[UA_TYPES_STRING], 3);
    UA_Array_delete(dstArray, &UA_TYPES[UA_TYPES_STRING], 3);
}
END_TEST

START_TEST(UA_DiagnosticInfo_copyShallWorkOnExample) {
    //given
    UA_DiagnosticInfo value, innerValue, copiedValue;
    UA_String testString = (UA_String){5, (UA_Byte*)"OPCUA"};

    UA_DiagnosticInfo_init(&value);
    UA_DiagnosticInfo_init(&innerValue);
    value.hasInnerDiagnosticInfo = UA_TRUE;
    value.innerDiagnosticInfo = &innerValue;
    value.hasAdditionalInfo = UA_TRUE;
    value.additionalInfo = testString;

    //when
    UA_DiagnosticInfo_copy(&value, &copiedValue);

    //then
    for(UA_Int32 i = 0;i < testString.length;i++)
        ck_assert_int_eq(copiedValue.additionalInfo.data[i], value.additionalInfo.data[i]);
    ck_assert_int_eq(copiedValue.additionalInfo.length, value.additionalInfo.length);

    ck_assert_int_eq(copiedValue.hasInnerDiagnosticInfo, value.hasInnerDiagnosticInfo);
    ck_assert_int_eq(copiedValue.innerDiagnosticInfo->locale, value.innerDiagnosticInfo->locale);
    ck_assert_int_eq(copiedValue.innerStatusCode, value.innerStatusCode);
    ck_assert_int_eq(copiedValue.locale, value.locale);
    ck_assert_int_eq(copiedValue.localizedText, value.localizedText);
    ck_assert_int_eq(copiedValue.namespaceUri, value.namespaceUri);
    ck_assert_int_eq(copiedValue.symbolicId, value.symbolicId);

    //finally
    value.additionalInfo.data = UA_NULL; // do not delete the static string
    value.innerDiagnosticInfo = UA_NULL; // do not delete the static innerdiagnosticinfo
    UA_DiagnosticInfo_deleteMembers(&value);
    UA_DiagnosticInfo_deleteMembers(&copiedValue);

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

    for(UA_Int32 i = 0;i < appString.length;i++)
        ck_assert_int_eq(copiedValue.applicationUri.data[i], value.applicationUri.data[i]);
    ck_assert_int_eq(copiedValue.applicationUri.length, value.applicationUri.length);

    for(UA_Int32 i = 0;i < discString.length;i++)
        ck_assert_int_eq(copiedValue.discoveryProfileUri.data[i], value.discoveryProfileUri.data[i]);
    ck_assert_int_eq(copiedValue.discoveryProfileUri.length, value.discoveryProfileUri.length);

    for(UA_Int32 i = 0;i < gateWayString.length;i++)
        ck_assert_int_eq(copiedValue.gatewayServerUri.data[i], value.gatewayServerUri.data[i]);
    ck_assert_int_eq(copiedValue.gatewayServerUri.length, value.gatewayServerUri.length);

    //String Array Test
    for(UA_Int32 i = 0;i < 3;i++) {
        for(UA_Int32 j = 0;j < 6;j++)
            ck_assert_int_eq(value.discoveryUrls[i].data[j], copiedValue.discoveryUrls[i].data[j]);
        ck_assert_int_eq(value.discoveryUrls[i].length, copiedValue.discoveryUrls[i].length);
    }
    ck_assert_int_eq(copiedValue.discoveryUrls[0].data[2], 'o');
    ck_assert_int_eq(copiedValue.discoveryUrls[0].data[3], 'p');
    ck_assert_int_eq(copiedValue.discoveryUrlsSize, value.discoveryUrlsSize);

    //finally
    // UA_ApplicationDescription_deleteMembers(&value); // do not free the members as they are statically allocated
    UA_ApplicationDescription_deleteMembers(&copiedValue);
}
END_TEST

START_TEST(UA_QualifiedName_copyShallWorkOnInputExample) {
    // given
    UA_String srcName = (UA_String){8, (UA_Byte*)"tEsT123!"};
    UA_QualifiedName src = {5, srcName};
    UA_QualifiedName dst;

    // when
    UA_StatusCode ret = UA_QualifiedName_copy(&src, &dst);
    // then
    ck_assert_int_eq(ret, UA_STATUSCODE_GOOD);
    ck_assert_int_eq('E', dst.name.data[1]);
    ck_assert_int_eq('!', dst.name.data[7]);
    ck_assert_int_eq(8, dst.name.length);
    ck_assert_int_eq(5, dst.namespaceIndex);
    // finally
    UA_QualifiedName_deleteMembers(&dst);
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
    ck_assert_int_eq(0, dst.locale.length);
    ck_assert_int_eq(7, dst.text.length);
}
END_TEST

START_TEST(UA_DataValue_copyShallWorkOnInputExample) {
    // given
    UA_Variant srcVariant;
    UA_Variant_init(&srcVariant);
    UA_DataValue src;
    UA_DataValue_init(&src);
    src.hasSourceTimestamp = UA_TRUE;
    src.sourceTimestamp = 4;
    src.hasSourcePicoseconds = UA_TRUE;
    src.sourcePicoseconds = 77;
    src.hasServerPicoseconds = UA_TRUE;
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
    ck_assert_int_eq(copiedString.length, testString.length);

    ck_assert_int_eq(value.arrayDimensionsSize, copiedValue.arrayDimensionsSize);
    ck_assert_int_eq(value.arrayLength, copiedValue.arrayLength);

    //finally
    ((UA_String*)value.data)->data = UA_NULL; // the string is statically allocated. do not free it.
    UA_Variant_deleteMembers(&value);
    UA_Variant_deleteMembers(&copiedValue);
}
END_TEST

START_TEST(UA_Variant_copyShallWorkOn1DArrayExample) {
    // given
    UA_String *srcArray = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], 3);
    srcArray[0] = UA_STRING_ALLOC("__open");
    srcArray[1] = UA_STRING_ALLOC("_62541");
    srcArray[2] = UA_STRING_ALLOC("opc ua");

    UA_Int32 *dimensions;
    dimensions = UA_malloc(sizeof(UA_Int32));
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
    UA_Int32 i1 = value.arrayDimensions[0];
    UA_Int32 i2 = copiedValue.arrayDimensions[0];
    ck_assert_int_eq(i1, i2);

    for(UA_Int32 i = 0;i < 3;i++) {
        for(UA_Int32 j = 0;j < 6;j++) {
            ck_assert_int_eq(((UA_String *)value.data)[i].data[j],
                    ((UA_String *)copiedValue.data)[i].data[j]);
        }
        ck_assert_int_eq(((UA_String *)value.data)[i].length,
                ((UA_String *)copiedValue.data)[i].length);
    }
    ck_assert_int_eq(((UA_String *)copiedValue.data)[0].data[2], 'o');
    ck_assert_int_eq(((UA_String *)copiedValue.data)[0].data[3], 'p');
    ck_assert_int_eq(value.arrayDimensionsSize, copiedValue.arrayDimensionsSize);
    ck_assert_int_eq(value.arrayLength, copiedValue.arrayLength);

    //finally
    UA_Variant_deleteMembers(&value);
    UA_Variant_deleteMembers(&copiedValue);
}
END_TEST

START_TEST(UA_Variant_copyShallWorkOn2DArrayExample) {
    // given
    UA_Int32 *srcArray = UA_Array_new(&UA_TYPES[UA_TYPES_INT32], 6);
    srcArray[0] = 0;
    srcArray[1] = 1;
    srcArray[2] = 2;
    srcArray[3] = 3;
    srcArray[4] = 4;
    srcArray[5] = 5;

    UA_Int32 *dimensions = UA_Array_new(&UA_TYPES[UA_TYPES_INT32], 2);
    UA_Int32 dim1 = 3;
    UA_Int32 dim2 = 2;
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
    UA_Int32 i1 = value.arrayDimensions[0];
    UA_Int32 i2 = copiedValue.arrayDimensions[0];
    ck_assert_int_eq(i1, i2);
    ck_assert_int_eq(i1, dim1);


    //2nd dimension
    i1 = value.arrayDimensions[1];
    i2 = copiedValue.arrayDimensions[1];
    ck_assert_int_eq(i1, i2);
    ck_assert_int_eq(i1, dim2);


    for(UA_Int32 i = 0;i < 6;i++) {
        i1 = ((UA_Int32 *)value.data)[i];
        i2 = ((UA_Int32 *)copiedValue.data)[i];
        ck_assert_int_eq(i1, i2);
        ck_assert_int_eq(i2, i);
    }

    ck_assert_int_eq(value.arrayDimensionsSize, copiedValue.arrayDimensionsSize);
    ck_assert_int_eq(value.arrayLength, copiedValue.arrayLength);

    //finally
    UA_Variant_deleteMembers(&value);
    UA_Variant_deleteMembers(&copiedValue);
}
END_TEST

START_TEST(UA_ExtensionObject_encodeDecodeShallWorkOnExtensionObject) {
    UA_Int32 val = 42;
    UA_VariableAttributes varAttr;
    UA_VariableAttributes_init(&varAttr);
    varAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    UA_Variant_init(&varAttr.value);
    varAttr.value.type = &UA_TYPES[UA_TYPES_INT32];
    varAttr.value.data = &val;
    varAttr.value.arrayLength = -1;
    varAttr.userWriteMask = 41;
    varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DATATYPE;
    varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_VALUE;
    varAttr.specifiedAttributes |= UA_NODEATTRIBUTESMASK_USERWRITEMASK;

    /* wrap it into a extension object attributes */
    UA_ExtensionObject extensionObject;
    UA_ExtensionObject_init(&extensionObject);
    extensionObject.typeId = UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES].typeId;
    UA_Byte extensionData[50];
    extensionObject.body = (UA_ByteString){.data = extensionData, .length=50};
    size_t posEncode = 0;
    UA_VariableAttributes_encodeBinary(&varAttr, &extensionObject.body, &posEncode);
    extensionObject.body.length = posEncode;
    extensionObject.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;

    UA_Byte data[50];
    UA_ByteString dst = {.data = data, .length=50};

    posEncode = 0;
    UA_ExtensionObject_encodeBinary(&extensionObject, &dst, &posEncode);

    UA_ExtensionObject extensionObjectDecoded;
    size_t posDecode = 0;
    UA_ExtensionObject_decodeBinary(&dst, &posDecode, &extensionObjectDecoded);

    ck_assert_int_eq(posEncode, posDecode);
    ck_assert_int_eq(extensionObjectDecoded.body.length, extensionObject.body.length);

    UA_VariableAttributes varAttrDecoded;
    UA_VariableAttributes_init(&varAttrDecoded);
    posDecode = 0;
    UA_VariableAttributes_decodeBinary(&extensionObjectDecoded.body, &posDecode, &varAttrDecoded);
    ck_assert_uint_eq(41, varAttrDecoded.userWriteMask);
    ck_assert_int_eq(-1, varAttrDecoded.value.arrayLength);

    // finally
    UA_ExtensionObject_deleteMembers(&extensionObjectDecoded);
    UA_Variant_deleteMembers(&varAttrDecoded.value);
}
END_TEST

static Suite *testSuite_builtin(void) {
    Suite *s = suite_create("Built-in Data Types 62541-6 Table 1");

    TCase *tc_calcSize = tcase_create("calcSize");
    tcase_add_test(tc_calcSize, UA_ExtensionObject_calcSizeShallWorkOnExample);
    tcase_add_test(tc_calcSize, UA_DataValue_calcSizeShallWorkOnExample);
    tcase_add_test(tc_calcSize, UA_DiagnosticInfo_calcSizeShallWorkOnExample);
    tcase_add_test(tc_calcSize, UA_String_calcSizeShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_String_calcSizeWithNegativLengthShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_String_calcSizeWithNegativLengthAndValidPointerShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_String_calcSizeWithZeroLengthShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_String_calcSizeWithZeroLengthAndValidPointerShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_NodeId_calcSizeEncodingTwoByteShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_NodeId_calcSizeEncodingFourByteShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_NodeId_calcSizeEncodingStringShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_NodeId_calcSizeEncodingStringNegativLengthShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_NodeId_calcSizeEncodingStringZeroLengthShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_ExpandedNodeId_calcSizeEncodingStringAndServerIndexShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_ExpandedNodeId_calcSizeEncodingStringAndNamespaceUriShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_Guid_calcSizeShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_Guid_calcSizeShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_LocalizedText_calcSizeTextOnlyShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_LocalizedText_calcSizeLocaleOnlyShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_LocalizedText_calcSizeTextAndLocaleShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_Variant_calcSizeFixedSizeArrayShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_Variant_calcSizeVariableSizeArrayShallReturnEncodingSize);
    tcase_add_test(tc_calcSize, UA_Variant_decodeWithOutDeleteMembersShallFailInCheckMem);
    suite_add_tcase(s, tc_calcSize);

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
    tcase_add_test(tc_encode, UA_DataValue_encodeShallWorkOnExampleWithoutVariant);
    tcase_add_test(tc_encode, UA_DataValue_encodeShallWorkOnExampleWithVariant);
    tcase_add_test(tc_encode, UA_ExtensionObject_encodeDecodeShallWorkOnExtensionObject);
    suite_add_tcase(s, tc_encode);

    TCase *tc_convert = tcase_create("convert");
    tcase_add_test(tc_convert, UA_DateTime_toStructShallWorkOnExample);
    tcase_add_test(tc_convert, UA_DateTime_toStringShallWorkOnExample);
    suite_add_tcase(s, tc_convert);

    TCase *tc_copy = tcase_create("copy");
    tcase_add_test(tc_copy, UA_Array_copyByteArrayShallWorkOnExample);
    tcase_add_test(tc_copy, UA_Array_copyUA_StringShallWorkOnExample);
    tcase_add_test(tc_copy, UA_ExtensionObject_copyShallWorkOnExample);

    tcase_add_test(tc_copy, UA_Variant_copyShallWorkOnSingleValueExample);
    tcase_add_test(tc_copy, UA_Variant_copyShallWorkOn1DArrayExample);
    tcase_add_test(tc_copy, UA_Variant_copyShallWorkOn2DArrayExample);

    tcase_add_test(tc_copy, UA_DiagnosticInfo_copyShallWorkOnExample);
    tcase_add_test(tc_copy, UA_ApplicationDescription_copyShallWorkOnExample);
    tcase_add_test(tc_copy, UA_QualifiedName_copyShallWorkOnInputExample);
    tcase_add_test(tc_copy, UA_Guid_copyShallWorkOnInputExample);
    tcase_add_test(tc_copy, UA_LocalizedText_copycstringShallWorkOnInputExample);
    tcase_add_test(tc_copy, UA_DataValue_copyShallWorkOnInputExample);
    suite_add_tcase(s, tc_copy);
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
