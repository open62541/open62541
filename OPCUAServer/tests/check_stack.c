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
#include "check.h"

START_TEST(test_getPacketType_correctArgument)
{

	char buf[] = {'C','L','O'};
	AD_RawMessage rawMessage;
	rawMessage.message = buf;
	rawMessage.length = 3;

	ck_assert_int_eq(TL_getPacketType(&rawMessage),packetType_CLO);

}END_TEST

Suite* TL_testSuite_getPacketType(void)
{
	Suite *s = suite_create("getPacketType");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core,test_getPacketType_correctArgument);
	suite_add_tcase(s,tc_core);
	return s;
}

int main (void)
{
	int number_failed;
	Suite *s = TL_testSuite_getPacketType();
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
