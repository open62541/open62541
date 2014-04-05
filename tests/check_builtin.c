/*
 ============================================================================
 Name        : check_stack.c
 Author      :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "opcua.h"
#include "check.h"

START_TEST(UA_Boolean_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Boolean* arg = UA_NULL;
	// when
	UA_Boolean storageSize = UA_Boolean_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, 1);
}
END_TEST

START_TEST(UA_SByte_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_SByte* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_SByte_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 1);
}
END_TEST
START_TEST(UA_Byte_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Byte* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_Byte_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 1);
}
END_TEST
START_TEST(UA_Int16_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Int16* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_Int16_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 2);
}
END_TEST
START_TEST(UA_UInt16_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_UInt16* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_UInt16_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 2);
}
END_TEST
START_TEST(UA_Int32_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Int32* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_Int32_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 4);
}
END_TEST
START_TEST(UA_UInt32_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_UInt32* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_UInt32_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 4);
}
END_TEST
START_TEST(UA_Int64_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Int64* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_Int64_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 8);
}
END_TEST
START_TEST(UA_UInt64_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_UInt64* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_UInt64_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 8);
}
END_TEST
START_TEST(UA_Float_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Float* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_Float_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 4);
}
END_TEST
START_TEST(UA_Double_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Double* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_Double_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 8);
}
END_TEST
START_TEST(UA_String_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_String* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_String_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_String));
	ck_assert_int_ge(storageSize, UA_Int32_calcSize(UA_NULL) + sizeof(UA_NULL));
}
END_TEST
START_TEST(UA_DateTime_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_DateTime* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_DateTime_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 8);
}
END_TEST
START_TEST(UA_Guid_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Guid* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_Guid_calcSize(arg);
	// then
	ck_assert_int_ge(storageSize, 16);
}
END_TEST
START_TEST(UA_ByteString_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_ByteString* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_ByteString_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_ByteString));
	ck_assert_int_ge(storageSize, UA_Int32_calcSize(UA_NULL)+sizeof(UA_NULL));
}
END_TEST
START_TEST(UA_XmlElement_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_XmlElement* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_XmlElement_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_XmlElement));
	ck_assert_int_ge(storageSize, UA_Int32_calcSize(UA_NULL)+sizeof(UA_NULL));
}
END_TEST
START_TEST(UA_NodeId_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_NodeId* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_NodeId_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_NodeId));
}
END_TEST
START_TEST(UA_ExpandedNodeId_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_ExpandedNodeId* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_ExpandedNodeId_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_ExpandedNodeId));
}
END_TEST
START_TEST(UA_StatusCode_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_StatusCode* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_StatusCode_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_StatusCode));
}
END_TEST
START_TEST(UA_QualifiedName_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_QualifiedName* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_QualifiedName_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_QualifiedName));
}
END_TEST
START_TEST(UA_LocalizedText_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_LocalizedText* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_LocalizedText_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_LocalizedText));
}
END_TEST
START_TEST(UA_ExtensionObject_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_ExtensionObject* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_ExtensionObject_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_ExtensionObject));
}
END_TEST
START_TEST(UA_DataValue_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_DataValue* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_DataValue_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_DataValue));
}
END_TEST
START_TEST(UA_Variant_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_Variant* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_Variant_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_Variant));
}
END_TEST
START_TEST(UA_DiagnosticInfo_calcSizeWithNullArgumentShallReturnStorageSize)
{
	// given
	UA_DiagnosticInfo* arg = UA_NULL;
	// when
	UA_Int32 storageSize = UA_DiagnosticInfo_calcSize(arg);
	// then
	ck_assert_int_eq(storageSize, sizeof(UA_DiagnosticInfo));
}
END_TEST
START_TEST(UA_String_calcSizeWithNegativLengthShallReturnEncodingSize)
{
	// given
	UA_String arg = {-1, UA_NULL};
	// when
	UA_Int32 encodingSize = UA_String_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 4);
}
END_TEST
START_TEST(UA_String_calcSizeWithNegativLengthAndValidPointerShallReturnEncodingSize)
{
	// given
	UA_String arg = {-1, "OPC"};
	// when
	UA_Int32 encodingSize = UA_String_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 4);
}
END_TEST
START_TEST(UA_String_calcSizeWithZeroLengthShallReturnEncodingSize)
{
	// given
	UA_String arg = {0, UA_NULL};
	// when
	UA_Int32 encodingSize = UA_String_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 4);
}
END_TEST
START_TEST(UA_String_calcSizeWithZeroLengthAndValidPointerShallReturnEncodingSize)
{
	// given
	UA_String arg = {0, "OPC"};
	// when
	UA_Int32 encodingSize = UA_String_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 4);
}
END_TEST
START_TEST(UA_String_calcSizeShallReturnEncodingSize)
{
	// given
	UA_String arg = {3, "OPC"};
	// when
	UA_Int32 encodingSize = UA_String_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 4+3);
}
END_TEST
START_TEST(UA_NodeId_calcSizeEncodingTwoByteShallReturnEncodingSize)
{
	// given
	UA_NodeId arg;
	arg.encodingByte = UA_NODEIDTYPE_TWOBYTE;
	// when
	UA_Int32 encodingSize = UA_NodeId_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 2);
}
END_TEST
START_TEST(UA_NodeId_calcSizeEncodingFourByteShallReturnEncodingSize)
{
	// given
	UA_NodeId arg;
	arg.encodingByte = UA_NODEIDTYPE_FOURBYTE;
	// when
	UA_Int32 encodingSize = UA_NodeId_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 4);
}
END_TEST
START_TEST(UA_NodeId_calcSizeEncodingStringShallReturnEncodingSize)
{
	// given
	UA_NodeId arg;
	arg.encodingByte = UA_NODEIDTYPE_STRING;
	arg.identifier.string.length = 3;
	arg.identifier.string.data = "PLT";
	// when
	UA_Int32 encodingSize = UA_NodeId_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+2+4+3);
}
END_TEST
START_TEST(UA_NodeId_calcSizeEncodingStringNegativLengthShallReturnEncodingSize)
{
	// given
	UA_NodeId arg;
	arg.encodingByte = UA_NODEIDTYPE_STRING;
	arg.identifier.string.length = -1;
	// when
	UA_Int32 encodingSize = UA_NodeId_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+2+4+0);
}
END_TEST
START_TEST(UA_NodeId_calcSizeEncodingStringZeroLengthShallReturnEncodingSize)
{
	// given
	UA_NodeId arg;
	arg.encodingByte = UA_NODEIDTYPE_STRING;
	arg.identifier.string.length = 0;
	// when
	UA_Int32 encodingSize = UA_NodeId_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+2+4+0);
}
END_TEST
START_TEST(UA_ExpandedNodeId_calcSizeEncodingStringAndServerIndexShallReturnEncodingSize)
{
	// given
	UA_ExpandedNodeId arg;
	arg.nodeId.encodingByte = UA_NODEIDTYPE_STRING | UA_NODEIDTYPE_SERVERINDEX_FLAG;
	arg.nodeId.identifier.string.length = 3;
	// when
	UA_Int32 encodingSize = UA_ExpandedNodeId_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+2+4+3+4);
}
END_TEST
START_TEST(UA_ExpandedNodeId_calcSizeEncodingStringAndNamespaceUriShallReturnEncodingSize)
{
	// given
	UA_ExpandedNodeId arg;
	arg.nodeId.encodingByte = UA_NODEIDTYPE_STRING | UA_NODEIDTYPE_NAMESPACE_URI_FLAG;
	arg.nodeId.identifier.string.length = 3;
	arg.namespaceUri.length = 7;
	// when
	UA_Int32 encodingSize = UA_ExpandedNodeId_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+2+4+3+4+7);
}
END_TEST
START_TEST(UA_Guid_calcSizeShallReturnEncodingSize)
{
	// given
	UA_Guid arg;
	// when
	UA_Int32 encodingSize = UA_Guid_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 16);
}
END_TEST
START_TEST(UA_LocalizedText_calcSizeTextOnlyShallReturnEncodingSize)
{
	// given
	UA_LocalizedText arg;
	arg.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
	arg.text.length = 42;
	// when
	UA_Int32 encodingSize = UA_LocalizedText_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+4+42);
}
END_TEST
START_TEST(UA_LocalizedText_calcSizeLocaleOnlyShallReturnEncodingSize)
{
	// given
	UA_LocalizedText arg;
	arg.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE;
	arg.locale.length = 11;
	// when
	UA_Int32 encodingSize = UA_LocalizedText_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+4+11);
}
END_TEST
START_TEST(UA_LocalizedText_calcSizeTextAndLocaleShallReturnEncodingSize)
{
	// given
	UA_LocalizedText arg;
	arg.encodingMask = UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_LOCALE | UA_LOCALIZEDTEXT_ENCODINGMASKTYPE_TEXT;
	arg.text.length = 47;
	arg.locale.length = 11;
	// when
	UA_Int32 encodingSize = UA_LocalizedText_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+4+11+4+47);
}
END_TEST
START_TEST(UA_Variant_calcSizeFixedSizeArrayShallReturnEncodingSize)
{
	// given
	UA_Variant arg;
	arg.encodingMask = UA_INT32_NS0 | UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
	arg.vt = &UA_[UA_INT32];
#define ARRAY_LEN 8
	arg.arrayLength = ARRAY_LEN;
	UA_Int32* data[ARRAY_LEN];
	arg.data = (void**) &data;
	// when
	UA_Int32 encodingSize = UA_Variant_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+4+ARRAY_LEN*4);
#undef ARRAY_LEN
}
END_TEST
START_TEST(UA_Variant_calcSizeVariableSizeArrayShallReturnEncodingSize)
{
	// given
	UA_Variant arg;
	arg.encodingMask = UA_STRING_NS0 | UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
	arg.vt = &UA_[UA_STRING];
#define ARRAY_LEN 3
	arg.arrayLength = ARRAY_LEN;
	UA_String s1 = {-1, UA_NULL };
	UA_String s2 = {3, "PLT" };
	UA_String s3 = {47, UA_NULL };
	UA_String* data[ARRAY_LEN] = { &s1, &s2, &s3 };
	arg.data = (void**) &data;
	// when
	UA_Int32 encodingSize = UA_Variant_calcSize(&arg);
	// then
	ck_assert_int_eq(encodingSize, 1+4+(4+0)+(4+3)+(4+47));
#undef ARRAY_LEN
}
END_TEST
START_TEST(UA_Variant_calcSizeVariableSizeArrayWithNullPtrWillReturnWrongButLargeEnoughEncodingSize)
{
	// given
	UA_Variant arg;
	arg.encodingMask = UA_STRING_NS0 | UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
	arg.vt = &UA_[UA_STRING];
#define ARRAY_LEN 6
	arg.arrayLength = ARRAY_LEN;
	UA_String s1 = {-1, UA_NULL };
	UA_String s2 = {3, "PLT" };
	UA_String s3 = {47, UA_NULL };
	UA_String* data[ARRAY_LEN] = { &s1, &s2, &s3 }; // will be filled with null-ptrs
	arg.data = (void**) &data;
	// when
	UA_Int32 encodingSize = UA_Variant_calcSize(&arg);
	// then
	ck_assert_int_ge(encodingSize, 1+4+(4+0)+(4+3)+(4+47)+(ARRAY_LEN-3)*(4+0));
#undef ARRAY_LEN
}
END_TEST
START_TEST(UA_Byte_decodeShallCopyAndAdvancePosition)
{
	// given
	UA_Byte dst;
	UA_Byte src[] = { 0x08 };
	UA_Int32 pos = 0;
	// when
	UA_Int32 retval = UA_Byte_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_uint_eq(pos, 1);
	ck_assert_uint_eq(dst, 0x08);
}
END_TEST
START_TEST(UA_Byte_decodeShallModifyOnlyCurrentPosition)
{
	// given
	UA_Byte dst[] = { 0xFF, 0xFF, 0xFF };
	UA_Byte src[] = { 0x08 };
	UA_Int32 pos = 0;
	// when
	UA_Int32 retval = UA_Byte_decode(src, &pos, &dst[1]);
	// then
	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_int_eq(pos, 1);
	ck_assert_uint_eq(dst[0], 0xFF);
	ck_assert_uint_eq(dst[1], 0x08);
	ck_assert_uint_eq(dst[2], 0xFF);
}
END_TEST
START_TEST(UA_Int16_decodeShallAssumeLittleEndian)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = {
			0x01,0x00,	// 1
			0x00,0x01,	// 256
	};
	// when
	UA_Int16 val_01_00, val_00_01;
	UA_Int32 retval = UA_Int16_decode(src,&pos,&val_01_00);
	retval |= UA_Int16_decode(src,&pos,&val_00_01);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(val_01_00,1);
	ck_assert_int_eq(val_00_01,256);
	ck_assert_int_eq(pos,4);
}
END_TEST
START_TEST(UA_Int16_decodeShallRespectSign)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = {
			0xFF,0xFF,	// -1
			0x00,0x80,	// -32768
	};
	// when
	UA_Int16 val_ff_ff, val_00_80;
	UA_Int32 retval = UA_Int16_decode(src,&pos,&val_ff_ff);
	retval |= UA_Int16_decode(src,&pos,&val_00_80);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(val_ff_ff,-1);
	ck_assert_int_eq(val_00_80,-32768);
}
END_TEST
START_TEST(UA_UInt16_decodeShallNotRespectSign)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = {
			0xFF,0xFF,	// -1
			0x00,0x80,	// -32768
	};
	// when
	UA_UInt16 val_ff_ff, val_00_80;
	UA_Int32 retval = UA_UInt16_decode(src,&pos,&val_ff_ff);
	retval |= UA_UInt16_decode(src,&pos,&val_00_80);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(pos,4);
	ck_assert_uint_eq(val_ff_ff, (0x01 << 16)-1);
	ck_assert_uint_eq(val_00_80, (0x01 << 15));
}
END_TEST
START_TEST(UA_String_decodeShallAllocateMemoryAndCopyString)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = {0x08,0x00,0x00,0x00,'A','C','P','L','T',' ','U','A',0xFF,0xFF,0xFF,0xFF,0xFF};
	UA_String dst;
	// when
	UA_Int32 retval = UA_String_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(dst.length,8);
	ck_assert_ptr_eq(dst.data,UA_alloc_lastptr);
	ck_assert_int_eq(dst.data[3],'L');
	// finally
	UA_String_deleteMembers(&dst);
}
END_TEST
START_TEST(UA_String_decodeWithNegativeSizeShallNotAllocateMemoryAndNullPtr)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = {0xFF,0xFF,0xFF,0xFF,'A','C','P','L','T',' ','U','A',0xFF,0xFF,0xFF,0xFF,0xFF};
	UA_String dst;
	// when
	UA_Int32 retval = UA_String_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(dst.length,-1);
	ck_assert_ptr_eq(dst.data,UA_NULL);
}
END_TEST
START_TEST(UA_String_decodeWithZeroSizeShallNotAllocateMemoryAndNullPtr)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = {0x00,0x00,0x00,0x00,'A','C','P','L','T',' ','U','A',0xFF,0xFF,0xFF,0xFF,0xFF};
	UA_String dst = { 2, "XX" };
	// when
	UA_Int32 retval = UA_String_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(dst.length,0);
	ck_assert_ptr_eq(dst.data,UA_NULL);
}
END_TEST
START_TEST(UA_NodeId_decodeTwoByteShallReadTwoBytesAndSetNamespaceToZero)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = { UA_NODEIDTYPE_TWOBYTE, 0x10 };
	UA_NodeId dst;
	// when
	UA_Int32 retval = UA_NodeId_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(pos,2);
	ck_assert_int_eq(dst.encodingByte, UA_NODEIDTYPE_TWOBYTE);
	ck_assert_int_eq(dst.identifier.numeric,16);
	ck_assert_int_eq(dst.namespace,0);
}
END_TEST
START_TEST(UA_NodeId_decodeFourByteShallReadFourBytesAndRespectNamespace)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = { UA_NODEIDTYPE_FOURBYTE, 0x01, 0x00, 0x01 };
	UA_NodeId dst;
	// when
	UA_Int32 retval = UA_NodeId_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(pos,4);
	ck_assert_int_eq(dst.encodingByte, UA_NODEIDTYPE_FOURBYTE);
	ck_assert_int_eq(dst.identifier.numeric,256);
	ck_assert_int_eq(dst.namespace,1);
}
END_TEST
START_TEST(UA_NodeId_decodeStringShallAllocateMemory)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = { UA_NODEIDTYPE_STRING, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 'P', 'L', 'T' };
	UA_NodeId dst;
	// when
	UA_Int32 retval = UA_NodeId_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(pos,10);
	ck_assert_int_eq(dst.encodingByte, UA_NODEIDTYPE_STRING);
	ck_assert_int_eq(dst.namespace,1);
	ck_assert_int_eq(dst.identifier.string.length,3);
	ck_assert_ptr_eq(dst.identifier.string.data,UA_alloc_lastptr);
	ck_assert_int_eq(dst.identifier.string.data[1],'L');
	// finally
	UA_NodeId_deleteMembers(&dst);
}
END_TEST
START_TEST(UA_Variant_decodeWithOutArrayFlagSetShallSetVTAndAllocateMemoryForArray)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = { UA_INT32_NS0, 0xFF, 0x00, 0x00, 0x00};
	UA_Variant dst;
	// when
	UA_Int32 retval = UA_Variant_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(pos,5);
	ck_assert_uint_eq(dst.encodingMask, UA_INT32_NS0);
	ck_assert_ptr_eq(dst.vt, &UA_[UA_INT32]);
	ck_assert_int_eq(dst.arrayLength,1);
	ck_assert_ptr_ne(dst.data,UA_NULL);
	ck_assert_ptr_eq(dst.data[0],UA_alloc_lastptr);
	ck_assert_int_eq(*(UA_Int32*)dst.data[0],255);
	// finally
	UA_Variant_deleteMembers(&dst);
}
END_TEST
START_TEST(UA_Variant_decodeWithArrayFlagSetShallSetVTAndAllocateMemoryForArray)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = { UA_INT32_NS0 | UA_VARIANT_ENCODINGMASKTYPE_ARRAY, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
	UA_Variant dst;
	// when
	UA_Int32 retval = UA_Variant_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(pos,1+4+2*4);
	ck_assert_uint_eq(dst.encodingMask & UA_VARIANT_ENCODINGMASKTYPE_TYPEID_MASK, UA_INT32_NS0);
	ck_assert_uint_eq(dst.encodingMask & UA_VARIANT_ENCODINGMASKTYPE_ARRAY, UA_VARIANT_ENCODINGMASKTYPE_ARRAY);
	ck_assert_ptr_eq(dst.vt, &UA_[UA_INT32]);
	ck_assert_int_eq(dst.arrayLength,2);
	ck_assert_ptr_ne(dst.data,UA_NULL);
	ck_assert_ptr_eq(dst.data[1],UA_alloc_lastptr);
	ck_assert_int_eq(*((UA_Int32*)dst.data[0]),255);
	ck_assert_int_eq(*((UA_Int32*)dst.data[1]),-1);
	// finally
	UA_Variant_deleteMembers(&dst);
}
END_TEST
START_TEST(UA_Variant_decodeWithOutDeleteMembersShallFailInCheckMem)
{
	// given
	UA_Int32 pos = 0;
	UA_Byte src[] = { UA_INT32_NS0 | UA_VARIANT_ENCODINGMASKTYPE_ARRAY, 0x02, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
	UA_Variant dst;
	// when
	UA_Int32 retval = UA_Variant_decode(src, &pos, &dst);
	// then
	ck_assert_int_eq(retval,UA_SUCCESS);
	// finally - unfortunately we cannot express that not freeing three chunks is what we expect
	// UA_Variant_deleteMembers(&dst);
}
END_TEST

START_TEST(UA_Byte_encode_test)
{
	UA_Byte src;
	UA_Byte dst[2] = { 0x00, 0xFF };
	UA_Int32 retval, pos = 0;

	ck_assert_uint_eq(dst[1], 0xFF);

	src = 8;
	retval = UA_Byte_encode(&src, &pos, dst);

	ck_assert_uint_eq(dst[0], 0x08);
	ck_assert_uint_eq(dst[1], 0xFF);
	ck_assert_int_eq(pos, 1);
	ck_assert_int_eq(retval, UA_SUCCESS);

	src = 0xFF;
	dst[1] = 0x00;
	pos = 0;
	retval = UA_Byte_encode(&src, &pos, dst);

	ck_assert_int_eq(dst[0], 0xFF);
	ck_assert_int_eq(dst[1], 0x00);
	ck_assert_int_eq(pos, 1);
	ck_assert_int_eq(retval, UA_SUCCESS);

}
END_TEST

Suite *testSuite_builtin(void)
{
	Suite *s = suite_create("Built-in Data Types 62541-6 Table 1");

	TCase *tc_calcSize = tcase_create("calcSize");
	tcase_add_test(tc_calcSize, UA_Boolean_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_SByte_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_Byte_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_Int16_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_UInt16_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_Int32_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_UInt32_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_Int64_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_UInt64_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_Float_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_Double_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_String_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_DateTime_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_Guid_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_ByteString_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_XmlElement_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_NodeId_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_ExpandedNodeId_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_StatusCode_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_QualifiedName_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_LocalizedText_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_ExtensionObject_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_DataValue_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_Variant_calcSizeWithNullArgumentShallReturnStorageSize);
	tcase_add_test(tc_calcSize, UA_DiagnosticInfo_calcSizeWithNullArgumentShallReturnStorageSize);
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
	tcase_add_test(tc_calcSize, UA_Variant_calcSizeVariableSizeArrayWithNullPtrWillReturnWrongButLargeEnoughEncodingSize);
	tcase_add_test(tc_calcSize, UA_Variant_decodeWithOutDeleteMembersShallFailInCheckMem);
	suite_add_tcase(s,tc_calcSize);



	TCase *tc_decode = tcase_create("decode");
	tcase_add_test(tc_decode, UA_Byte_decodeShallCopyAndAdvancePosition);
	tcase_add_test(tc_decode, UA_Byte_decodeShallModifyOnlyCurrentPosition);
	tcase_add_test(tc_decode, UA_Int16_decodeShallAssumeLittleEndian);
	tcase_add_test(tc_decode, UA_Int16_decodeShallRespectSign);
	tcase_add_test(tc_decode, UA_UInt16_decodeShallNotRespectSign);
	tcase_add_test(tc_decode, UA_String_decodeShallAllocateMemoryAndCopyString);
	tcase_add_test(tc_decode, UA_String_decodeWithNegativeSizeShallNotAllocateMemoryAndNullPtr);
	tcase_add_test(tc_decode, UA_String_decodeWithZeroSizeShallNotAllocateMemoryAndNullPtr);
	tcase_add_test(tc_decode, UA_NodeId_decodeTwoByteShallReadTwoBytesAndSetNamespaceToZero);
	tcase_add_test(tc_decode, UA_NodeId_decodeFourByteShallReadFourBytesAndRespectNamespace);
	tcase_add_test(tc_decode, UA_NodeId_decodeStringShallAllocateMemory);
	tcase_add_test(tc_decode, UA_Variant_decodeWithOutArrayFlagSetShallSetVTAndAllocateMemoryForArray);
	tcase_add_test(tc_decode, UA_Variant_decodeWithArrayFlagSetShallSetVTAndAllocateMemoryForArray);
	suite_add_tcase(s,tc_decode);

	return s;
}


int main (void)
{
	int number_failed = 0;
	Suite* s;
	SRunner*  sr;

	s = testSuite_builtin();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
