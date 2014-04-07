/*
 ============================================================================
 Name        : check_encode.c
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


START_TEST(encodeByte_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeByte
	UA_Byte *mem = malloc(sizeof(UA_Byte));
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
	UA_UInt16 val;
	UA_UInt16_decode(rawMessage.data, &p, &val);
	ck_assert_int_eq(val,testUInt16);
	//ck_assert_int_eq(rawMessage.data[0], 0xAB);

	free(mem);
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
START_TEST(encodeUInt32_test)
{
	UA_ByteString rawMessage;
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
START_TEST(encodeInt32_test)
{

}
END_TEST
START_TEST(encodeUInt64_test)
{
	UA_ByteString rawMessage;
	UA_UInt64 value = 0x0101FF00FF00FF00;
	//EncodeUInt16

	rawMessage.data = (UA_Byte*) malloc(sizeof(UA_UInt64));

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
START_TEST(encodeInt64_test)
{
	UA_ByteString rawMessage;
	UA_UInt64 value = 0x0101FF00FF00FF00;
	//EncodeUInt16

	rawMessage.data = (UA_Byte*) malloc(sizeof(UA_UInt64));

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
START_TEST(encodeFloat_test)
{
	UA_Float value = -6.5;
	UA_Int32 pos = 0;
	UA_Byte* buf = (UA_Byte*)malloc(sizeof(UA_Float));

	UA_Float_encode(&value,&pos,buf);

	ck_assert_uint_eq(buf[2],0xD0);
	ck_assert_uint_eq(buf[3],0xC0);
	free(buf);
}
END_TEST
/*START_TEST(encodeDouble_test)
{
	UA_Double value = -6.5;
	UA_Int32 pos = 0;
	UA_Byte* buf = (char*)malloc(sizeof(UA_Double));

	UA_Double_encode(&value,&pos,buf);

	ck_assert_uint_eq(buf[6],0xD0);
	ck_assert_uint_eq(buf[7],0xC0);
	free(buf);
}
END_TEST*/
START_TEST(encodeUAString_test)
{

	UA_Int32 pos = 0;
	UA_String string;
	UA_Int32 l = 11;
	UA_Byte mem[11] = "ACPLT OPCUA";
	UA_Byte *dstBuf = (UA_Byte*) malloc(sizeof(UA_Int32)+l);
	string.data =  mem;
	string.length = 11;

	UA_String_encode(&string, &pos, dstBuf);

	ck_assert_int_eq(dstBuf[0],11);
	ck_assert_int_eq(dstBuf[0+sizeof(UA_Int32)],'A');

	free(dstBuf);
}
END_TEST
START_TEST(encodeDataValue_test)
{
	UA_DataValue dataValue;
	UA_Int32 pos = 0, retval;
	UA_Byte* buf = (UA_Byte*) malloc(15);
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


Suite *testSuite_encodeByte(void)
{
	Suite *s = suite_create("encodeByte_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeByte_test);
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
Suite*testSuite_encodeUInt16(void)
{
	Suite *s = suite_create("encodeUInt16_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeUInt16_test);
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
Suite*testSuite_encodeInt32(void)
{
	Suite *s = suite_create("encodeInt32_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeInt32_test);
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
/*Suite *testSuite_encodeDouble(void)
{
	Suite *s = suite_create("encodeDouble_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeDouble_test);
	suite_add_tcase(s,tc_core);
	return s;
}*/
Suite * testSuite_encodeUAString(void)
{
	Suite *s = suite_create("encodeUAString_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encodeUAString_test);
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


int main (void)
{
	int number_failed = 0;

	Suite *s = testSuite_encodeByte();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeUInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeUInt32();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeInt32();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeUInt64();
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

/*	s = testSuite_encodeDouble();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);*/

	s = testSuite_encodeUAString();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_encodeDataValue();
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
