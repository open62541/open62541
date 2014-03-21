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
#include "opcua_transportLayer.h"
#include "check.h"



START_TEST(test_getPacketType_validParameter)
{

	char buf[] = {'C','L','O'};
	UA_Int32 pos = 0;
	UA_ByteString msg;

	msg.data = buf;
	msg.length = 3;

	ck_assert_int_eq(TL_getPacketType(&msg, &pos),packetType_CLO);
}
END_TEST


START_TEST(decodeByte_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeByte
		char *mem = malloc(sizeof(UA_Byte));
		UA_Byte val;

		rawMessage.data = mem;
		rawMessage.length = 1;
		mem[0] = 0x08;

		position = 0;

		UA_Byte_decode(rawMessage.data, &position, &val);

		ck_assert_int_eq(val, 0x08);
		ck_assert_int_eq(position, 1);
		free(mem);
}
END_TEST

START_TEST(encodeByte_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeByte
		char *mem = malloc(sizeof(UA_Byte));
		rawMessage.data = mem;
		UA_Byte testByte = 0x08;
		rawMessage.length = 1;
		position = 0;

		UA_Byte_encode(&(testByte), &position, rawMessage.data);

		ck_assert_int_eq(rawMessage.data[0], 0x08);
		ck_assert_int_eq(rawMessage.length, 1);
		ck_assert_int_eq(position, 1);

		free(mem);
}
END_TEST

/*
START_TEST(decodeRequestHeader_test_validParameter)
{
		char testMessage = {0x00,0x00,0x72,0xf1,0xdc,0xc9,0x87,0x0b,

							0xcf,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
							0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,
							0x00,0x00,0x00,0x00,0x00};
		UA_ByteString rawMessage;
		rawMessage.data = &testMessage;
		rawMessage.length = 29;
		Int32 position = 0;
		T_RequestHeader requestHeader;
		decodeRequestHeader(rawMessage,&position,&requestHeader);

		ck_assert_int_eq(requestHeader.authenticationToken.EncodingByte,0);

		ck_assert_int_eq(requestHeader.returnDiagnostics,0);

		ck_assert_int_eq(requestHeader.authenticationToken.EncodingByte,0);

}
END_TEST
*/

START_TEST(decodeInt16_test_positives)
{
	UA_Int32 p = 0;
	UA_Int16 val;
	UA_ByteString rawMessage;
	char mem[] = {
			0x00,0x00,	// 0
			0x01,0x00,	// 1
			0xFF,0x00,	// 255
			0x00,0x01,	// 256
	};

	rawMessage.data = mem;
	rawMessage.length = sizeof(mem);
	ck_assert_int_eq(rawMessage.length,8);

	UA_Int16_decode(rawMessage.data,&p,&val);
	ck_assert_int_eq(val,0);
	UA_Int16_decode(rawMessage.data,&p,&val);
	ck_assert_int_eq(val,1);
	UA_Int16_decode(rawMessage.data,&p,&val);
	ck_assert_int_eq(val,255);
	UA_Int16_decode(rawMessage.data,&p,&val);
	ck_assert_int_eq(val,256);
}
END_TEST
START_TEST(decodeInt16_test_negatives)
{
	UA_Int32 p = 0;
	UA_Int16 val;
	UA_ByteString rawMessage;
	UA_Byte mem[] = {
			0xFF,0xFF,	// -1
			0x00,0x80,	// -32768
	};

	rawMessage.data = mem;
	rawMessage.length = sizeof(mem);
	ck_assert_int_eq(rawMessage.length,4);

	UA_Int16_decode(rawMessage.data,&p,&val);
	ck_assert_int_eq(val,-1);
	UA_Int16_decode(rawMessage.data,&p,&val);
	ck_assert_int_eq(val,-32768);
}
END_TEST

START_TEST(encodeInt16_test)
{

	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeUInt16
	UA_Byte *mem = malloc(sizeof(UA_UInt16));
	rawMessage.data = mem;
	UA_UInt16 testUInt16 = 1;
	rawMessage.length = 2;
	position = 0;

	UA_UInt16_encode(&testUInt16, &position, rawMessage.data);

	ck_assert_int_eq(position, 2);
	UA_Int32 p = 0;
	UA_Int16 val;
	UA_UInt16_decode(rawMessage.data, &p, &val);
	ck_assert_int_eq(val,testUInt16);
	//ck_assert_int_eq(rawMessage.data[0], 0xAB);

	free(mem);
}
END_TEST

START_TEST(decodeUInt16_test)
{

	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeUInt16
	UA_Byte mem[2] = {0x01,0x00};

	rawMessage.data = mem;

	rawMessage.length = 2;

	//encodeUInt16(testUInt16, &position, &rawMessage);

	UA_Int32 p = 0;
	UA_UInt16 val;
	UA_UInt16_decode(rawMessage.data,&p,&val);

	ck_assert_int_eq(val,1);
	//ck_assert_int_eq(p, 2);
	//ck_assert_int_eq(rawMessage.data[0], 0xAB);

}
END_TEST
START_TEST(encodeUInt16_test)
{

	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeUInt16
	UA_Byte *mem = (UA_Byte*) malloc(sizeof(UA_UInt16));
	rawMessage.data = mem;
	UA_UInt16 testUInt16 = 1;
	rawMessage.length = 2;
	position = 0;

	UA_UInt16_encode(&testUInt16, &position, rawMessage.data);
	ck_assert_int_eq(position, 2);

	UA_Int32 p = 0;
	UA_UInt16 val;
	UA_UInt16_decode(rawMessage.data, &p, &val);
	ck_assert_int_eq(val,testUInt16);
	//ck_assert_int_eq(rawMessage.data[0], 0xAB);

	free(mem);
}
END_TEST


START_TEST(decodeUInt32_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeUInt16
	UA_Byte mem[4] = {0xFF,0x00,0x00,0x00};

	rawMessage.data = mem;
	rawMessage.length = 4;

	UA_Int32 p = 0;
	UA_UInt32 val;
	UA_UInt32_decode(rawMessage.data, &p, &val);
	ck_assert_uint_eq(val,255);

}
END_TEST
START_TEST(encodeUInt32_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	UA_UInt32 value = 0x0101FF00;
	//EncodeUInt16

	rawMessage.data = (UA_Byte*) malloc(2 * sizeof(UA_UInt32));

	rawMessage.length = 8;

	UA_Int32 p = 4;
	UA_UInt32_encode(&value,&p,rawMessage.data);
	ck_assert_uint_eq(rawMessage.data[4],0x00);
	ck_assert_uint_eq(rawMessage.data[5],0xFF);
	ck_assert_uint_eq(rawMessage.data[6],0x01);
	ck_assert_uint_eq(rawMessage.data[7],0x01);
	ck_assert_int_eq(p,8);

	free(rawMessage.data);

}
END_TEST

START_TEST(decodeInt32_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeUInt16
	UA_Byte mem[4] = {0x00,0xCA,0x9A,0x3B};

	rawMessage.data = mem;

	rawMessage.length = 4;


	UA_Int32 p = 0;
	UA_Int32 val;
	UA_Int32_decode(rawMessage.data, &p, &val);
	ck_assert_int_eq(val,1000000000);
}
END_TEST
START_TEST(encodeInt32_test)
{

}
END_TEST


START_TEST(decodeUInt64_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	UA_UInt64 expectedVal = 0xFF;
	expectedVal = expectedVal << 56;
	UA_Byte mem[8] = {00,00,00,00,0x00,0x00,0x00,0xFF};

	rawMessage.data = mem;

	rawMessage.length = 8;

	UA_Int32 p = 0;
	UA_UInt64 val;
	UA_UInt64_decode(rawMessage.data, &p, &val);
	ck_assert_uint_eq(val, expectedVal);
}
END_TEST
START_TEST(encodeUInt64_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	UA_UInt64 value = 0x0101FF00FF00FF00;
	//EncodeUInt16

	rawMessage.data = (UA_Byte*) malloc(sizeof(UA_UInt32));

	rawMessage.length = 8;

	UA_Int32 p = 0;
	UA_UInt64_encode(&value, &p,rawMessage.data);

	ck_assert_uint_eq((UA_Byte)rawMessage.data[0],0x00);
	ck_assert_uint_eq((UA_Byte)rawMessage.data[1],0xFF);
	ck_assert_uint_eq((UA_Byte)rawMessage.data[2],0x00);
	ck_assert_uint_eq((UA_Byte)rawMessage.data[3],0xFF);
	ck_assert_uint_eq((UA_Byte)rawMessage.data[4],0x00);
	ck_assert_uint_eq((UA_Byte)rawMessage.data[5],0xFF);
	ck_assert_uint_eq((UA_Byte)rawMessage.data[6],0x01);
	ck_assert_uint_eq((UA_Byte)rawMessage.data[7],0x01);

	free(rawMessage.data);
}
END_TEST

START_TEST(decodeInt64_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	UA_Int64 expectedVal = 0xFF;
	expectedVal = expectedVal << 56;
	UA_Byte mem[8] = {00,00,00,00,0x00,0x00,0x00,0xFF};

	rawMessage.data = mem;
	rawMessage.length = 8;

	UA_Int32 p = 0;
	UA_Int64 val;
	UA_Int64_decode(rawMessage.data, &p, &val);
	ck_assert_uint_eq(val, expectedVal);
}
END_TEST
START_TEST(encodeInt64_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	UA_UInt64 value = 0x0101FF00FF00FF00;
	//EncodeUInt16

	rawMessage.data = (UA_Byte*) malloc(sizeof(UA_UInt32));

	rawMessage.length = 8;

	UA_Int32 p = 0;
	UA_UInt64_encode(&value, &p,rawMessage.data);

	ck_assert_uint_eq(rawMessage.data[0],0x00);
	ck_assert_uint_eq(rawMessage.data[1],0xFF);
	ck_assert_uint_eq(rawMessage.data[2],0x00);
	ck_assert_uint_eq(rawMessage.data[3],0xFF);
	ck_assert_uint_eq(rawMessage.data[4],0x00);
	ck_assert_uint_eq(rawMessage.data[5],0xFF);
	ck_assert_uint_eq(rawMessage.data[6],0x01);
	ck_assert_uint_eq(rawMessage.data[7],0x01);

	free(rawMessage.data);
}
END_TEST


START_TEST(decodeFloat_test)
{
	UA_Float expectedValue = -6.5;
	UA_Int32 pos = 0;
	UA_Byte buf[4] = {0x00,0x00,0xD0,0xC0};


	UA_Float fval;

	UA_Float_decode(buf, &pos, &fval);
	//val should be -6.5
	UA_Int32 val = (fval > -6.501 && fval < -6.499);
	ck_assert_int_gt(val,0);
}
END_TEST
START_TEST(encodeFloat_test)
{
	UA_Float value = -6.5;
	UA_Int32 pos = 0;
	UA_Byte* buf = (char*)malloc(sizeof(UA_Float));

	UA_Float_encode(&value,&pos,buf);

	ck_assert_uint_eq(buf[2],0xD0);
	ck_assert_uint_eq(buf[3],0xC0);
	free(buf);
}
END_TEST

START_TEST(decodeDouble_test)
{

}
END_TEST
START_TEST(encodeDouble_test)
{
	UA_Double value = -6.5;
	UA_Int32 pos = 0;
	UA_Byte* buf = (char*)malloc(sizeof(UA_Double));

	UA_Double_encode(&value,&pos,buf);

	ck_assert_uint_eq(buf[6],0xD0);
	ck_assert_uint_eq(buf[7],0xC0);
	free(buf);
}
END_TEST


START_TEST(encodeUAString_test)
{

	UA_Int32 pos = 0;
	UA_String string;
	UA_Int32 l = 11;
	UA_Byte mem[11] = "ACPLT OPCUA";
	UA_Byte *dstBuf = (char*) malloc(sizeof(UA_Int32)+l);
	string.data =  mem;
	string.length = 11;

	UA_String_encode(&string, &pos, dstBuf);

	ck_assert_int_eq(dstBuf[0],11);
	ck_assert_int_eq(dstBuf[0+sizeof(UA_Int32)],'A');


}
END_TEST
START_TEST(decodeUAString_test)
{

	UA_Int32 pos = 0;
	UA_String string;
	UA_Int32 l = 12;
	char binString[12] = {0x08,0x00,0x00,0x00,'A','C','P','L','T',' ','U','A'};

	UA_String_decode(binString, &pos, &string);

	ck_assert_int_eq(string.length,8);
	ck_assert_ptr_eq(string.data,UA_alloc_lastptr);
	ck_assert_int_eq(string.data[3],'L');

	UA_String_deleteMembers(&string);
}
END_TEST

START_TEST(diagnosticInfo_calcSize_test)
{

	UA_Int32 valreal = 0;
	UA_Int32 valcalc = 0;
	UA_DiagnosticInfo diagnosticInfo;
	diagnosticInfo.encodingMask = 0x01 | 0x02 | 0x04 | 0x08 | 0x10;
	diagnosticInfo.symbolicId = 30;
	diagnosticInfo.namespaceUri = 25;
	diagnosticInfo.localizedText = 22;
	diagnosticInfo.additionalInfo.data = "OPCUA";
	diagnosticInfo.additionalInfo.length = 5;

	ck_assert_int_eq(UA_DiagnosticInfo_calcSize(&diagnosticInfo),26);

}
END_TEST

START_TEST(extensionObject_calcSize_test)
{

	UA_Int32 valreal = 0;
	UA_Int32 valcalc = 0;
	UA_Byte data[3] = {1,2,3};
	UA_ExtensionObject extensionObject;

	// empty ExtensionObject, handcoded
	extensionObject.typeId.encodingByte = UA_NODEIDTYPE_TWOBYTE;
	extensionObject.typeId.identifier.numeric = 0;
	extensionObject.encoding = UA_EXTENSIONOBJECT_NOBODYISENCODED;
	ck_assert_int_eq(UA_ExtensionObject_calcSize(&extensionObject), 1 + 1 + 1);

	// ExtensionObject with ByteString-Body
	extensionObject.encoding = UA_EXTENSIONOBJECT_BODYISBYTESTRING;
	extensionObject.body.data = data;
	extensionObject.body.length = 3;
	ck_assert_int_eq(UA_ExtensionObject_calcSize(&extensionObject), 3 + 4 + 3);
}
END_TEST

START_TEST(responseHeader_calcSize_test)
{
	UA_ResponseHeader responseHeader;
	UA_DiagnosticInfo diagnosticInfo;
	UA_ExtensionObject extensionObject;
	UA_DiagnosticInfo  emptyDO = {0x00};
	UA_ExtensionObject emptyEO = {{UA_NODEIDTYPE_TWOBYTE,0},UA_EXTENSIONOBJECT_NOBODYISENCODED};
	//Should have the size of 26 Bytes
	diagnosticInfo.encodingMask = UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_SYMBOLICID | UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_NAMESPACE | UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALIZEDTEXT | UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_LOCALE | UA_DIAGNOSTICINFO_ENCODINGMASKTYPE_ADDITIONALINFO;		// Byte:   1
	// Indices into to Stringtable of the responseHeader (62541-6 §5.5.12 )
	diagnosticInfo.symbolicId = -1;										// Int32:  4
	diagnosticInfo.namespaceUri = -1;									// Int32:  4
	diagnosticInfo.localizedText = -1;									// Int32:  4
	diagnosticInfo.locale = -1;											// Int32:  4
	// Additional Info
	diagnosticInfo.additionalInfo.length = 5;							// Int32:  4
	diagnosticInfo.additionalInfo.data = "OPCUA";						// Byte[]: 5
	responseHeader.serviceDiagnostics = &diagnosticInfo;
	ck_assert_int_eq(UA_DiagnosticInfo_calcSize(&diagnosticInfo),1+(4+4+4+4)+(4+5));

	responseHeader.stringTableSize = -1;								// Int32:	4
	responseHeader.stringTable = NULL;

	responseHeader.additionalHeader = &emptyEO;	//		    3
	ck_assert_int_eq(UA_ResponseHeader_calcSize(&responseHeader),16+26+4+3);

	responseHeader.serviceDiagnostics = &emptyDO;
	ck_assert_int_eq(UA_ResponseHeader_calcSize(&responseHeader),16+1+4+3);
}
END_TEST

//ToDo: Function needs to be filled
START_TEST(expandedNodeId_calcSize_test)
{
	UA_Int32 valreal = 300;
	UA_Int32 valcalc = 0;
	ck_assert_int_eq(valcalc,valreal);
}
END_TEST

START_TEST(encodeDataValue_test)
{
	UA_DataValue dataValue;
	UA_Int32 pos = 0, retval;
	UA_Byte* buf = (char*) malloc(15);
	UA_DateTime dateTime;
	dateTime = 80;
	dataValue.serverTimestamp = dateTime;

	//--without Variant
	dataValue.encodingMask = UA_DATAVALUE_SERVERTIMPSTAMP; //Only the sourcePicoseconds
	UA_DataValue_encode(&dataValue, &pos, buf);

	ck_assert_int_eq(pos, 9);// represents the length
	ck_assert_uint_eq(buf[0], 0x08); // encodingMask
	ck_assert_uint_eq(buf[1], 80); // 8 Byte serverTimestamp
	ck_assert_uint_eq(buf[2], 0);
	ck_assert_uint_eq(buf[3], 0);
	ck_assert_uint_eq(buf[4], 0);
	ck_assert_uint_eq(buf[5], 0);
	ck_assert_uint_eq(buf[6], 0);
	ck_assert_uint_eq(buf[7], 0);
	ck_assert_uint_eq(buf[8], 0);

	//TestCase for a DataValue with a Variant!
	dataValue.encodingMask = UA_DATAVALUE_VARIANT | UA_DATAVALUE_SERVERTIMPSTAMP; //Variant & SourvePicoseconds
	dataValue.value.vt = &UA_[UA_INT32];
	dataValue.value.arrayLength = 0;
	dataValue.value.encodingMask = UA_INT32_NS0;
	UA_Int32 data = 45;
	UA_Int32* pdata = &data;
	dataValue.value.data = (void**) &pdata;

	pos = 0;
	retval = UA_DataValue_encode(&dataValue, &pos, buf);

	ck_assert_int_eq(retval, UA_SUCCESS);
	ck_assert_int_eq(pos, 1+(1+4)+8);// represents the length
	ck_assert_uint_eq(buf[0], 0x08 | 0x01); // encodingMask
	ck_assert_uint_eq(buf[1], 0x06); // Variant's Encoding Mask - INT32
	ck_assert_uint_eq(buf[2], 45);  // the single value
	ck_assert_uint_eq(buf[3], 0);
	ck_assert_uint_eq(buf[4], 0);
	ck_assert_uint_eq(buf[5], 0);
	ck_assert_uint_eq(buf[6], 80);  // the server timestamp
	ck_assert_uint_eq(buf[7], 0);

	free(buf);
}
END_TEST

START_TEST(DataValue_calcSize_test)
{
	UA_DataValue dataValue;
	dataValue.encodingMask = UA_DATAVALUE_STATUSCODE |  UA_DATAVALUE_SOURCETIMESTAMP |  UA_DATAVALUE_SOURCEPICOSECONDS;
	dataValue.status = 12;
	UA_DateTime dateTime;
	dateTime = 80;
	dataValue.sourceTimestamp = dateTime;
	UA_DateTime sourceTime;
	dateTime = 214;
	dataValue.sourcePicoseconds = sourceTime;

	int size = 0;
	size = UA_DataValue_calcSize(&dataValue);

	ck_assert_int_eq(size, 21);
}
END_TEST

START_TEST(encode_builtInDatatypeArray_test_String)
{
	UA_Int32 noElements = 2;
	UA_ByteString s1 = { 6, "OPC UA" };
	UA_ByteString s2 = { -1, NULL };
	UA_ByteString* array[] = { &s1, &s2	};
	UA_Int32 pos = 0, i;
	char buf[256];
	char result[] = {
			0x02, 0x00, 0x00, 0x00,		// noElements
			0x06, 0x00, 0x00, 0x00,		// s1.Length
			'O', 'P', 'C', ' ', 'U', 'A', // s1.Data
			0xFF, 0xFF, 0xFF, 0xFF		// s2.Length
	};

	UA_Array_encode((void const**)array, noElements, UA_BYTESTRING, &pos, buf);

	// check size
	ck_assert_int_eq(pos, 4 + 4 + 6 + 4);
	// check result
	for (i=0; i< sizeof(result); i++) {
		ck_assert_int_eq(buf[i],result[i]);
	}
}
END_TEST

Suite *testSuite_getPacketType(void)
{
	Suite *s = suite_create("getPacketType");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,test_getPacketType_validParameter);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite *testSuite_encodeByte(void)
{
	Suite *s = suite_create("encodeByte_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeByte_test);
	tcase_add_test(tc_core, encodeByte_test);
	suite_add_tcase(s,tc_core);
	return s;
}


Suite *testSuite_decodeInt16(void)
{
	Suite *s = suite_create("decodeInt16_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeInt16_test_positives);
	tcase_add_test(tc_core, decodeInt16_test_negatives);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite*testSuite_encodeInt16(void)
{
	Suite *s = suite_create("encodeInt16_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeInt16_test);
	suite_add_tcase(s,tc_core);
	return s;
}


Suite *testSuite_decodeUInt16(void)
{
	Suite *s = suite_create("decodeUInt16_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeUInt16_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite*testSuite_encodeUInt16(void)
{
	Suite *s = suite_create("encodeUInt16_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeUInt16_test);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite*testSuite_decodeUInt32(void)
{
	Suite *s = suite_create("decodeUInt32_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeUInt32_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite*testSuite_encodeUInt32(void)
{
	Suite *s = suite_create("encodeUInt32_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeUInt32_test);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite*testSuite_decodeInt32(void)
{
	Suite *s = suite_create("decodeInt32_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeInt32_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite*testSuite_encodeInt32(void)
{
	Suite *s = suite_create("encodeInt32_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeInt32_test);
	suite_add_tcase(s,tc_core);
	return s;
}




Suite*testSuite_decodeUInt64(void)
{
	Suite *s = suite_create("decodeUInt64_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeUInt64_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite*testSuite_encodeUInt64(void)
{
	Suite *s = suite_create("encodeUInt64_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeUInt64_test);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite*testSuite_decodeInt64(void)
{
	Suite *s = suite_create("decodeInt64_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeInt64_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite*testSuite_encodeInt64(void)
{
	Suite *s = suite_create("encodeInt64_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeInt64_test);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite *testSuite_encodeFloat(void)
{
	Suite *s = suite_create("encodeFloat_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeFloat_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite *testSuite_decodeFloat(void)
{
	Suite *s = suite_create("decodeFloat_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeFloat_test);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite *testSuite_encodeDouble(void)
{
	Suite *s = suite_create("encodeDouble_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeDouble_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite *testSuite_decodeDouble(void)
{
	Suite *s = suite_create("decodeDouble_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeDouble_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite * testSuite_encodeUAString(void)
{
	Suite *s = suite_create("encodeUAString_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeUAString_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite * testSuite_decodeUAString(void)
{
	Suite *s = suite_create("decodeUAString_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeUAString_test);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite* testSuite_encodeDataValue()
{
	Suite *s = suite_create("encodeDataValue");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeDataValue_test);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite* testSuite_encode_builtInDatatypeArray()
{
	Suite *s = suite_create("encode_builtInDatatypeArray");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encode_builtInDatatypeArray_test_String);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite* testSuite_expandedNodeId_calcSize(void)
{
	Suite *s = suite_create("expandedNodeId_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,expandedNodeId_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}

/*
Suite* TL_<TESTSUITENAME>(void)
{
	Suite *s = suite_create("<TESTSUITENAME>");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,<TEST_NAME>);
	suite_add_tcase(s,tc_core);
	return s;
}
*/
Suite* testSuite_diagnosticInfo_calcSize()
{
	Suite *s = suite_create("diagnosticInfo_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, diagnosticInfo_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* testSuite_extensionObject_calcSize()
{
	Suite *s = suite_create("extensionObject_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, extensionObject_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* testSuite_responseHeader_calcSize()
{
	Suite *s = suite_create("responseHeader_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, responseHeader_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* testSuite_dataValue_calcSize(void)
{
	Suite *s = suite_create("dataValue_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,DataValue_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}

int main (void)
{
	int number_failed = 0;

	Suite *s = testSuite_getPacketType();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeUInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeUInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeUInt32();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeUInt32();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeInt32();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeInt32();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeUInt64();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeUInt64();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeInt64();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeInt64();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);


	s = testSuite_encodeFloat();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeDouble();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);


	s = testSuite_encodeByte();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeUAString();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeUAString();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_diagnosticInfo_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_extensionObject_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_responseHeader_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeDataValue();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encode_builtInDatatypeArray();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_expandedNodeId_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_dataValue_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	/* <TESTSUITE_TEMPLATE>
	s =  <TESTSUITENAME>;
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);
	*/
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}


