/*
 ============================================================================
 Name        : check_stack.c
 Author      :
 Version     :
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

START_TEST(UA_Byte_decode_test)
{
	UA_Byte dst;
	UA_Byte src[] = { 0x08 };
	UA_Int32 retval, pos = 0;

	retval = UA_Byte_decode(src, &pos, &dst);

	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_uint_eq(pos, 1);
	ck_assert_uint_eq(dst, 8);
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
START_TEST(UA_Int16_decode_test_positives)
{
	UA_Int32 p = 0;
	UA_Int16 val;
	UA_Int32 retval;
	UA_Byte buf[] = {
			0x00,0x00,	// 0
			0x01,0x00,	// 1
			0xFF,0x00,	// 255
			0x00,0x01,	// 256
	};

	retval = UA_Int16_decode(buf,&p,&val);
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(val,0);
	retval = UA_Int16_decode(buf,&p,&val);
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(val,1);
	retval = UA_Int16_decode(buf,&p,&val);
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(val,255);
	retval = UA_Int16_decode(buf,&p,&val);
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(val,256);
}
END_TEST
START_TEST(UA_Int16_decode_test_negatives)
{
	UA_Int32 p = 0;
	UA_Int16 val;
	UA_Int32 retval;
	UA_Byte mem[] = {
			0xFF,0xFF,	// -1
			0x00,0x80,	// -32768
	};


	retval = UA_Int16_decode(mem,&p,&val);
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(val,-1);
	retval = UA_Int16_decode(mem,&p,&val);
	ck_assert_int_eq(retval,UA_SUCCESS);
	ck_assert_int_eq(val,-32768);
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
	suite_add_tcase(s,tc_calcSize);

	TCase *tc_decode = tcase_create("decode");
	tcase_add_test(tc_decode, UA_Byte_decode_test);
	tcase_add_test(tc_decode, UA_Byte_encode_test);
	tcase_add_test(tc_decode, UA_Int16_decode_test_negatives);
	tcase_add_test(tc_decode, UA_Int16_decode_test_positives);
	tcase_add_test(tc_decode, UA_Byte_encode_test);
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
