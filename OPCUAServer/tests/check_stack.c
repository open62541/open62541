/*
 ============================================================================
 Name        : opcuaServer.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "../src/opcua_transportLayer.h"
#include "../src/opcua_binaryEncDec.h"
#include "../src/opcua_encodingLayer.h"
#include "../src/opcua_advancedDatatypes.h"
#include "check.h"

START_TEST(test_getPacketType_validParameter)
{

	char buf[] = {'C','L','O'};
	AD_RawMessage rawMessage;
	rawMessage.message = buf;
	rawMessage.length = 3;

	ck_assert_int_eq(TL_getPacketType(&rawMessage),packetType_CLO);

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

		encodeByte(testByte, &position, &rawMessage);

		ck_assert_int_eq(rawMessage.message[0], 0x08);
		ck_assert_int_eq(rawMessage.length, 1);
		ck_assert_int_eq(position, 1);
		free(mem);
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
	UInt16 val = decodeUInt16(rawMessage.message,&p);
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

	encodeUInt16(testUInt16, &position, &rawMessage);
	//encodeUInt16(testUInt16, &position, &rawMessage);

	ck_assert_int_eq(position, 2);
	Int32 p = 0;
	Int16 val = decodeUInt16(rawMessage.message,&p);
	ck_assert_int_eq(val,testUInt16);
	//ck_assert_int_eq(rawMessage.message[0], 0xAB);

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

	extensionObject.TypeId.EncodingByte = NIEVT_TWO_BYTE;; // Numeric TWO BYTES
	extensionObject.TypeId.Identifier.Numeric = 0;

	extensionObject.Encoding = 0x00;
	extensionObject.Length = 0;
	//extensionObject.Body = &data;

	valcalc = extensionObject_calcSize(&extensionObject);
	valreal = 3;
	ck_assert_int_eq(valcalc, valreal);

}
END_TEST

START_TEST(responseHeader_calcSize_test)
{
	Int32 valreal = 0;
	Int32 valcalc = 0;
	T_ResponseHeader responseHeader;
	UA_DiagnosticInfo diagnosticInfo;
	UA_ExtensionObject extensionObject;

	//Should have the size of 16 Bytes
	responseHeader.timestamp = 150014;
	responseHeader.requestHandle = 514;
	responseHeader.serviceResult = 504;

	//Should have the size of 26 Bytes
	diagnosticInfo.EncodingMask = 0x01 | 0x02 | 0x04 | 0x08 | 0x10;
	diagnosticInfo.SymbolicId = 30;
	diagnosticInfo.NamespaceUri = 25;
	diagnosticInfo.LocalizedText = 22;
	diagnosticInfo.AdditionalInfo.Data = "OPCUA";
	diagnosticInfo.AdditionalInfo.Length = 5;
	responseHeader.serviceDiagnostics = &diagnosticInfo;
	//Should have the size of 4 Bytes
	responseHeader.noOfStringTable = 0;
	//Should have the size of 3 Bytes
	extensionObject.TypeId.EncodingByte = NIEVT_TWO_BYTE;
	extensionObject.TypeId.Identifier.Numeric = 0;
	extensionObject.Encoding = 0x00; //binaryBody = false, xmlBody = false
	responseHeader.additionalHeader = extensionObject;

	valcalc = responseHeader_calcSize(&responseHeader);
	valreal = 49;
	ck_assert_int_eq(valcalc,valreal);

}
END_TEST
Suite* TL_testSuite_getPacketType(void)
{
	Suite *s = suite_create("getPacketType");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,test_getPacketType_validParameter);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* TL_testSuite_decodeUInt16(void)
{
	Suite *s = suite_create("decodeUInt16_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeUInt16_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* TL_testSuite_encodeUInt16(void)
{
	Suite *s = suite_create("encodeUInt16_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeUInt16_test);
	suite_add_tcase(s,tc_core);
	return s;
}

Suite* TL_testSuite_encodeByte(void)
{
	Suite *s = suite_create("encodeByte_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeByte_test);
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
Suite* TL_testSuite_diagnosticInfo_calcSize()
{
	Suite *s = suite_create("diagnosticInfo_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, diagnosticInfo_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* TL_testSuite_extensionObject_calcSize()
{
	Suite *s = suite_create("extensionObject_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, extensionObject_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
Suite* TL_testSuite_responseHeader_calcSize()
{
	Suite *s = suite_create("responseHeader_calcSize");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, responseHeader_calcSize_test);
	suite_add_tcase(s,tc_core);
	return s;
}
int main (void)
{
	int number_failed = 0;

	Suite *s = TL_testSuite_getPacketType();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	s = TL_testSuite_decodeUInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = TL_testSuite_encodeUInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = TL_testSuite_encodeByte();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = TL_testSuite_diagnosticInfo_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = TL_testSuite_extensionObject_calcSize();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = TL_testSuite_responseHeader_calcSize();
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


