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


/*
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
*/


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

START_TEST(encode_builtInDatatypeArray_test_String)
{
	UA_Int32 noElements = 2;
	UA_ByteString s1 = { 6, (UA_Byte*) "OPC UA" };
	UA_ByteString s2 = { -1, UA_NULL };
	UA_ByteString* array[] = { &s1, &s2	};
	UA_Int32 pos = 0;
	UA_UInt32 i;
	UA_Byte buf[256];
	UA_Byte result[] = {
			0x02, 0x00, 0x00, 0x00,		// noElements
			0x06, 0x00, 0x00, 0x00,		// s1.Length
			'O', 'P', 'C', ' ', 'U', 'A', // s1.Data
			0xFF, 0xFF, 0xFF, 0xFF		// s2.Length
	};

	UA_Array_encode((void const**)array, noElements, UA_BYTESTRING, &pos, buf);

	// check size
	ck_assert_int_eq(pos, 4 + 4 + 6 + 4);
	// check result
	for (i=0; i < sizeof(result); i++) {
		ck_assert_int_eq(buf[i],result[i]);
	}
}
END_TEST

/**Suite *testSuite_getPacketType(void)
{
	Suite *s = suite_create("getPacketType");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,test_getPacketType_validParameter);
	suite_add_tcase(s,tc_core);
	return s;
}**/

Suite* testSuite_encode_builtInDatatypeArray()
{
	Suite *s = suite_create("encode_builtInDatatypeArray");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, encode_builtInDatatypeArray_test_String);
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


int main (void)
{
	int number_failed = 0;

	Suite *s;
	SRunner *sr;

	/* s = testSuite_getPacketType();
	sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr); */

	s = testSuite_encode_builtInDatatypeArray();
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


