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
#include "opcua_transportLayer.h"
#include "opcua_binaryEncDec.h"
#include "opcua_encodingLayer.h"
#include "opcua_advancedDatatypes.h"
//#include "check_stdint.h"
#include "check.h"



START_TEST(test_getPacketType_validParameter)
{

	char buf[] = {'C','L','O'};
	Int32 pos = 0;
	UA_ByteString msg;
	msg.Data = buf;
	msg.Length = 3;

	ck_assert_int_eq(TL_getPacketType(&msg, &pos),packetType_CLO);
}
END_TEST

START_TEST(encodeByte_test)
{
	AD_RawMessage rawMessage;
	Int32 position = 0;
	//EncodeByte
		char *mem = malloc(sizeof(Byte));
		rawMessage.message = mem;
		Byte testByte = 0x08;
		rawMessage.length = 1;
		position = 0;

		encodeByte(testByte, &position, rawMessage.message);

		ck_assert_int_eq(rawMessage.message[0], 0x08);
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
		AD_RawMessage rawMessage;
		rawMessage.message = &testMessage;
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
	Int32 p = 0;
	Int16 val;
	AD_RawMessage rawMessage;
	char mem[] = {
			0x00,0x00,	// 0
			0x01,0x00,	// 1
			0xFF,0x00,	// 255
			0x00,0x01,	// 256
	};

	rawMessage.message = mem;
	rawMessage.length = sizeof(mem);
	ck_assert_int_eq(rawMessage.length,8);

	decoder_decodeBuiltInDatatype(rawMessage.message,INT16,&p,&val);
	ck_assert_int_eq(val,0);
	decoder_decodeBuiltInDatatype(rawMessage.message,INT16,&p,&val);
	ck_assert_int_eq(val,1);
	decoder_decodeBuiltInDatatype(rawMessage.message,INT16,&p,&val);
	ck_assert_int_eq(val,255);
	decoder_decodeBuiltInDatatype(rawMessage.message,INT16,&p,&val);
	ck_assert_int_eq(val,256);
}
END_TEST
START_TEST(decodeInt16_test_negatives)
{
	Int32 p = 0;
	Int16 val;
	AD_RawMessage rawMessage;
	char mem[] = {
			0xFF,0xFF,	// -1
			0x00,0x80,	// -32768
	};

	rawMessage.message = mem;
	rawMessage.length = sizeof(mem);
	ck_assert_int_eq(rawMessage.length,4);

	decoder_decodeBuiltInDatatype(rawMessage.message,INT16,&p,&val);
	ck_assert_int_eq(val,-1);
	decoder_decodeBuiltInDatatype(rawMessage.message,INT16,&p,&val);
	ck_assert_int_eq(val,-32768);
}
END_TEST

START_TEST(encodeInt16_test)
{

	AD_RawMessage rawMessage;
	Int32 position = 0;
	//EncodeUInt16
	char *mem = malloc(sizeof(UInt16));
	rawMessage.message = mem;
	UInt16 testUInt16 = 1;
	rawMessage.length = 2;
	position = 0;

	encodeUInt16(testUInt16, &position, rawMessage.message);
	//encodeUInt16(testUInt16, &position, &rawMessage);

	ck_assert_int_eq(position, 2);
	Int32 p = 0;
	Int16 val;
	decoder_decodeBuiltInDatatype(rawMessage.message, INT16, &p, &val);
	ck_assert_int_eq(val,testUInt16);
	//ck_assert_int_eq(rawMessage.message[0], 0xAB);

}
END_TEST

START_TEST(decodeUInt16_test)
{

	AD_RawMessage rawMessage;
	Int32 position = 0;
	//EncodeUInt16
	char mem[2] = {0x01,0x00};

	rawMessage.message = mem;

	rawMessage.length = 2;

	//encodeUInt16(testUInt16, &position, &rawMessage);

	Int32 p = 0;
	UInt16 val;
	decoder_decodeBuiltInDatatype(rawMessage.message,UINT16,&p,&val);

	ck_assert_int_eq(val,1);
	//ck_assert_int_eq(p, 2);
	//ck_assert_int_eq(rawMessage.message[0], 0xAB);

}
END_TEST
START_TEST(encodeUInt16_test)
{

	AD_RawMessage rawMessage;
	Int32 position = 0;
	//EncodeUInt16
	char *mem = malloc(sizeof(UInt16));
	rawMessage.message = mem;
	UInt16 testUInt16 = 1;
	rawMessage.length = 2;
	position = 0;

	encodeUInt16(testUInt16, &position, rawMessage.message);
	//encodeUInt16(testUInt16, &position, &rawMessage);

	ck_assert_int_eq(position, 2);
	Int32 p = 0;
	UInt16 val;
	decoder_decodeBuiltInDatatype(rawMessage.message, UINT16, &p, &val);
	ck_assert_int_eq(val,testUInt16);
	//ck_assert_int_eq(rawMessage.message[0], 0xAB);

}
END_TEST


START_TEST(decodeUInt32_test)
{
	AD_RawMessage rawMessage;
	Int32 position = 0;
	//EncodeUInt16
	char mem[4] = {0xFF,0x00,0x00,0x00};

	rawMessage.message = mem;
	rawMessage.length = 4;

	Int32 p = 0;
	UInt32 val;
	decoder_decodeBuiltInDatatype(rawMessage.message, UINT32, &p, &val);
	ck_assert_uint_eq(val,255);

}
END_TEST
START_TEST(encodeUInt32_test)
{
	AD_RawMessage rawMessage;
	Int32 position = 0;
	UInt32 value = 0x0101FF00;
	//EncodeUInt16

	rawMessage.message = (char*)opcua_malloc(2 * sizeof(UInt32));

	rawMessage.length = 8;

	Int32 p = 4;
	//encodeUInt32(value, &p,rawMessage.message);
	encoder_encodeBuiltInDatatype(&value,UINT32,&p,rawMessage.message);
	ck_assert_uint_eq((Byte)rawMessage.message[4],0x00);
	ck_assert_uint_eq((Byte)rawMessage.message[5],0xFF);
	ck_assert_uint_eq((Byte)rawMessage.message[6],0x01);
	ck_assert_uint_eq((Byte)rawMessage.message[7],0x01);
	ck_assert_int_eq(p,8);


}
END_TEST

START_TEST(decodeInt32_test)
{
	AD_RawMessage rawMessage;
	Int32 position = 0;
	//EncodeUInt16
	char mem[4] = {0x00,0xCA,0x9A,0x3B};

	rawMessage.message = mem;

	rawMessage.length = 4;


	Int32 p = 0;
	Int32 val;
	decoder_decodeBuiltInDatatype(rawMessage.message, INT32, &p, &val);
	ck_assert_int_eq(val,1000000000);
}
END_TEST
START_TEST(encodeInt32_test)
{

}
END_TEST


START_TEST(decodeUInt64_test)
{
	AD_RawMessage rawMessage;
	Int32 position = 0;
	UInt64 expectedVal = 0xFF;
	expectedVal = expectedVal << 56;
	char mem[8] = {00,00,00,00,0x00,0x00,0x00,0xFF};

	rawMessage.message = mem;

	rawMessage.length = 8;

	Int32 p = 0;
	UInt64 val;
	decoder_decodeBuiltInDatatype(rawMessage.message, UINT64, &p, &val);
	ck_assert_uint_eq(val, expectedVal);
}
END_TEST
START_TEST(encodeUInt64_test)
{
	AD_RawMessage rawMessage;
	Int32 position = 0;
	UInt64 value = 0x0101FF00FF00FF00;
	//EncodeUInt16

	rawMessage.message = (char*)opcua_malloc(sizeof(UInt32));

	rawMessage.length = 8;

	Int32 p = 0;
	encodeUInt64(value, &p,rawMessage.message);

	ck_assert_uint_eq((Byte)rawMessage.message[0],0x00);
	ck_assert_uint_eq((Byte)rawMessage.message[1],0xFF);
	ck_assert_uint_eq((Byte)rawMessage.message[2],0x00);
	ck_assert_uint_eq((Byte)rawMessage.message[3],0xFF);
	ck_assert_uint_eq((Byte)rawMessage.message[4],0x00);
	ck_assert_uint_eq((Byte)rawMessage.message[5],0xFF);
	ck_assert_uint_eq((Byte)rawMessage.message[6],0x01);
	ck_assert_uint_eq((Byte)rawMessage.message[7],0x01);
}
END_TEST

START_TEST(decodeInt64_test)
{
	AD_RawMessage rawMessage;
	Int32 position = 0;
	Int64 expectedVal = 0xFF;
	expectedVal = expectedVal << 56;
	char mem[8] = {00,00,00,00,0x00,0x00,0x00,0xFF};

	rawMessage.message = mem;

	rawMessage.length = 8;

	Int32 p = 0;
	Int64 val;
	decoder_decodeBuiltInDatatype(rawMessage.message, INT64, &p, &val);
	ck_assert_uint_eq(val, expectedVal);
}
END_TEST
START_TEST(encodeInt64_test)
{
	AD_RawMessage rawMessage;
	Int32 position = 0;
	UInt64 value = 0x0101FF00FF00FF00;
	//EncodeUInt16

	rawMessage.message = (char*)opcua_malloc(sizeof(UInt32));

	rawMessage.length = 8;

	Int32 p = 0;
	encodeUInt64(value, &p,rawMessage.message);

	ck_assert_uint_eq((Byte)rawMessage.message[0],0x00);
	ck_assert_uint_eq((Byte)rawMessage.message[1],0xFF);
	ck_assert_uint_eq((Byte)rawMessage.message[2],0x00);
	ck_assert_uint_eq((Byte)rawMessage.message[3],0xFF);
	ck_assert_uint_eq((Byte)rawMessage.message[4],0x00);
	ck_assert_uint_eq((Byte)rawMessage.message[5],0xFF);
	ck_assert_uint_eq((Byte)rawMessage.message[6],0x01);
	ck_assert_uint_eq((Byte)rawMessage.message[7],0x01);
}
END_TEST


START_TEST(decodeFloat_test)
{
	Float expectedValue = -6.5;
	Int32 pos = 0;
	char buf[4] = {0x00,0x00,0xD0,0xC0};


	Float calcVal;

	decoder_decodeBuiltInDatatype(buf, FLOAT, &pos, &calcVal);
	//val should be -6.5

	Int32 val = (calcVal > -6.501 && calcVal < -6.499);


	ck_assert_int_gt(val,0);

	opcua_free(buf);
}
END_TEST
START_TEST(encodeFloat_test)
{
	Float value = -6.5;
	Int32 pos = 0;
	char *buf = (char*)opcua_malloc(sizeof(Float));

	encodeFloat(value,&pos,buf);

	ck_assert_uint_eq((Byte)buf[2],0xD0);
	ck_assert_uint_eq((Byte)buf[3],0xC0);
	opcua_free(buf);

}
END_TEST

START_TEST(decodeDouble_test)
{

}
END_TEST
START_TEST(encodeDouble_test)
{
	Float value = -6.5;
	Int32 pos = 0;
	char *buf = (char*)opcua_malloc(sizeof(Float));

	encodeDouble(value,&pos,buf);

	ck_assert_uint_eq((Byte)buf[6],0xD0);
	ck_assert_uint_eq((Byte)buf[7],0xC0);
	opcua_free(buf);
}
END_TEST


START_TEST(encodeUAString_test)
{

	Int32 pos = 0;
	UA_String string;
	Int32 l = 11;
	char mem[11] = "ACPLT OPCUA";
	char *dstBuf = (char*) malloc(sizeof(Int32)+l);
	string.Data =  mem;
	string.Length = 11;

	encodeUAString(&string, &pos, dstBuf);

	ck_assert_int_eq(dstBuf[0],11);
	ck_assert_int_eq(dstBuf[0+sizeof(Int32)],'A');


}
END_TEST
START_TEST(decodeUAString_test)
{

	Int32 pos = 0;
	UA_String string;
	Int32 l = 11;
	char binString[15] = {11,0x00,0x00,0x00,'A','C','P','L','T',' ','U','A'};

	char *dstBuf = (char*) malloc(l-sizeof(Int32));
	string.Data = dstBuf;
	string.Length = 0;
	decodeUAString(binString, &pos, &string);


	ck_assert_int_eq(string.Length,11);
	ck_assert_int_eq(string.Data[3],'L');


}
END_TEST

START_TEST(diagnosticInfo_calcSize_test)
{

	Int32 valreal = 0;
	Int32 valcalc = 0;
	UA_DiagnosticInfo diagnosticInfo;
	diagnosticInfo.EncodingMask = 0x01 | 0x02 | 0x04 | 0x08 | 0x10;
	diagnosticInfo.SymbolicId = 30;
	diagnosticInfo.NamespaceUri = 25;
	diagnosticInfo.LocalizedText = 22;
	diagnosticInfo.AdditionalInfo.Data = "OPCUA";
	diagnosticInfo.AdditionalInfo.Length = 5;

	valcalc = diagnosticInfo_calcSize(&diagnosticInfo);
	valreal = 26;
	ck_assert_int_eq(valcalc,valreal);

}
END_TEST
START_TEST(extensionObject_calcSize_test)
{

	Int32 valreal = 0;
	Int32 valcalc = 0;
	Byte data[3] = {1,2,3};
	UA_ExtensionObject extensionObject;

	// empty ExtensionObject
	valcalc = extensionObject_calcSize(&the_empty_UA_ExtensionObject);
	ck_assert_int_eq(valcalc, 1 + 1 + 1);

	// empty ExtensionObject, handcoded
	extensionObject.TypeId.EncodingByte = NIEVT_TWO_BYTE;
	extensionObject.TypeId.Identifier.Numeric = 0;
	extensionObject.Encoding = NO_BODY_IS_ENCODED;

	valcalc = extensionObject_calcSize(&extensionObject);
	ck_assert_int_eq(valcalc, 1 + 1 + 1);

	// ExtensionObject with ByteString-Body
	extensionObject.Encoding = BODY_IS_BYTE_STRING;
	extensionObject.Body.Data = data;
	extensionObject.Body.Length = 3;
	valcalc = extensionObject_calcSize(&extensionObject);
	ck_assert_int_eq(valcalc, 3 + 4 + 3);

}
END_TEST

START_TEST(responseHeader_calcSize_test)
{
	UA_AD_ResponseHeader responseHeader;
	UA_DiagnosticInfo diagnosticInfo;
	UA_ExtensionObject extensionObject;

	//Should have the size of 26 Bytes
	diagnosticInfo.EncodingMask = DIEMT_SYMBOLIC_ID | DIEMT_NAMESPACE | DIEMT_LOCALIZED_TEXT | DIEMT_LOCALE | DIEMT_ADDITIONAL_INFO;		// Byte:   1
	// Indices into to Stringtable of the responseHeader (62541-6 §5.5.12 )
	diagnosticInfo.SymbolicId = -1;										// Int32:  4
	diagnosticInfo.NamespaceUri = -1;									// Int32:  4
	diagnosticInfo.LocalizedText = -1;									// Int32:  4
	diagnosticInfo.Locale = -1;											// Int32:  4
	// Additional Info
	diagnosticInfo.AdditionalInfo.Length = 5;							// Int32:  4
	diagnosticInfo.AdditionalInfo.Data = "OPCUA";						// Byte[]: 5
	responseHeader.serviceDiagnostics = &diagnosticInfo;
	ck_assert_int_eq(diagnosticInfo_calcSize(&diagnosticInfo),1+(4+4+4+4)+(4+5));

	responseHeader.noOfStringTable = -1;								// Int32:	4
	responseHeader.stringTable = NULL;

	responseHeader.additionalHeader = &the_empty_UA_ExtensionObject;	//		    3
	ck_assert_int_eq(responseHeader_calcSize(&responseHeader),16+26+4+3);
}
END_TEST

//ToDo: Function needs to be filled
START_TEST(expandedNodeId_calcSize_test)
{
	Int32 valreal = 300;
	Int32 valcalc = 0;
	ck_assert_int_eq(valcalc,valreal);
}
END_TEST

START_TEST(encodeDataValue_test)
{
	UA_DataValue dataValue;
	Int32 pos = 0;
	char *buf = (char*)opcua_malloc(15);
	UA_DateTime dateTime;
	dateTime = 80;
	dataValue.ServerTimestamp = dateTime;

	//--without Variant
	dataValue.EncodingMask = 0x08; //Only the SourvePicoseconds
	encodeDataValue(&dataValue, &pos, buf);

	ck_assert_int_eq(pos, 9);// represents the length
	ck_assert_int_eq(buf[0], 0x08);
	ck_assert_int_eq(buf[1], 80);
	ck_assert_int_eq(buf[2], 0);
	ck_assert_int_eq(buf[3], 0);
	ck_assert_int_eq(buf[4], 0);
	ck_assert_int_eq(buf[5], 0);
	ck_assert_int_eq(buf[6], 0);
	ck_assert_int_eq(buf[7], 0);
	ck_assert_int_eq(buf[8], 0);

	//TestCase for a DataValue with a Variant!
	//ToDo: Need to be checked after the function for encoding variants has been implemented
	pos = 0;
	dataValue.EncodingMask = 0x01 || 0x08; //Variant & SourvePicoseconds
	UA_Variant variant;
	variant.ArrayLength = 0;
	variant.EncodingMask = VTEMT_INT32;
	UA_VariantUnion variantUnion;
	//ToDo: needs to be adjusted: variantUnion.Int32 = 45;
	fail(); ////ToDo: needs to be adjusted: Just to see that see that this needs to be adjusted
	variant.Value = &variantUnion;
	dataValue.Value = variant;
	encodeDataValue(&dataValue, &pos, buf);

	ck_assert_int_eq(pos, 14);// represents the length
	ck_assert_int_eq(buf[0], 0x08);
	ck_assert_int_eq(buf[1], 0x06);
	ck_assert_int_eq(buf[2], 45);
	ck_assert_int_eq(buf[3], 0);
	ck_assert_int_eq(buf[4], 0);
	ck_assert_int_eq(buf[5], 0);
	ck_assert_int_eq(buf[6], 80);
	ck_assert_int_eq(buf[7], 0);

}
END_TEST

START_TEST(DataValue_calcSize_test)
{
	UA_DataValue dataValue;
	dataValue.EncodingMask = 0x02 + 0x04 + 0x10;
	dataValue.Status = 12;
	UA_DateTime dateTime;
	dateTime = 80;
	dataValue.SourceTimestamp = dateTime;
	UA_DateTime sourceTime;
	dateTime = 214;
	dataValue.SourcePicoseconds = sourceTime;

	int size = 0;
	size = DataValue_calcSize(&dataValue);

	ck_assert_int_eq(size, 21);
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


