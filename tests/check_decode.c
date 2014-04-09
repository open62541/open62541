/*
 ============================================================================
 Name        : check_decode.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "opcua.h"
#include "ua_transportLayer.h"
#include "check.h"

START_TEST(decodeByte_test)
{
	UA_ByteString rawMessage;
	UA_Int32 position = 0;
	//EncodeByte
		UA_Byte* mem = (UA_Byte*) malloc(sizeof(UA_Byte));
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
START_TEST(decodeInt16_test_positives)
{
	UA_Int32 p = 0;
	UA_Int16 val;
	UA_ByteString rawMessage;
	UA_Byte mem[] = {
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
START_TEST(decodeUInt16_test)
{

	UA_ByteString rawMessage;
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
START_TEST(decodeUInt32_test)
{
	UA_ByteString rawMessage;
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
START_TEST(decodeInt32_test)
{
	UA_ByteString rawMessage;
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
START_TEST(decodeUInt64_test)
{
	UA_ByteString rawMessage;
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
START_TEST(decodeInt64_test)
{
	UA_ByteString rawMessage;
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
START_TEST(decodeFloat_test)
{
	UA_Int32 pos = 0;
	UA_Byte buf[4] = {0x00,0x00,0xD0,0xC0};


	UA_Float fval;

	UA_Float_decode(buf, &pos, &fval);
	//val should be -6.5
	UA_Int32 val = (fval > -6.501 && fval < -6.499);
	ck_assert_int_gt(val,0);
}
END_TEST
START_TEST(decodeDouble_test)
{

}
END_TEST
START_TEST(decodeUAString_test)
{

	UA_Int32 pos = 0;
	UA_String string;
	UA_Byte binString[12] = {0x08,0x00,0x00,0x00,'A','C','P','L','T',' ','U','A'};

	UA_String_decode(binString, &pos, &string);

	ck_assert_int_eq(string.length,8);
	ck_assert_ptr_eq(string.data,UA_alloc_lastptr);
	ck_assert_int_eq(string.data[3],'L');

	UA_String_deleteMembers(&string);
}
END_TEST

Suite *testSuite_decodeByte(void)
{
	Suite *s = suite_create("encodeByte_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeByte_test);
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
Suite *testSuite_decodeUInt16(void)
{
	Suite *s = suite_create("decodeUInt16_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeUInt16_test);
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
Suite*testSuite_decodeInt32(void)
{
	Suite *s = suite_create("decodeInt32_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeInt32_test);
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
Suite*testSuite_decodeUInt64(void)
{
	Suite *s = suite_create("decodeUInt64_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeUInt64_test);
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
Suite *testSuite_decodeDouble(void)
{
	Suite *s = suite_create("decodeDouble_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, decodeDouble_test);
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

int main (void)
{
	int number_failed = 0;

	Suite *s = testSuite_decodeByte();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeUInt16();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeUInt32();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeInt32();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeUInt64();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeInt64();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	s = testSuite_decodeUAString();
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

