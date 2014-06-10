#include <stdlib.h> // EXIT_SUCCESS
#include "ua_base64.h"

#include "check.h"


START_TEST(base64_test_basic)
{


	//this is base64'd ASCII string "open62541"
	UA_String encodedString = {12, (UA_Byte*)"b3BlbjYyNTQx"};

	//assure that we allocate at least 9 bytes
	ck_assert_int_ge(UA_base64_getDecodedSizeUB(&encodedString), 9);

	UA_Byte* decodedData = (UA_Byte*)malloc(UA_base64_getDecodedSizeUB(&encodedString));
	UA_base64_decode(&encodedString, decodedData);

	//check the string
	ck_assert_int_eq(decodedData[0], 'o');
	ck_assert_int_eq(decodedData[1], 'p');
	ck_assert_int_eq(decodedData[2], 'e');
	ck_assert_int_eq(decodedData[3], 'n');
	ck_assert_int_eq(decodedData[4], '6');
	ck_assert_int_eq(decodedData[5], '2');
	ck_assert_int_eq(decodedData[6], '5');
	ck_assert_int_eq(decodedData[7], '4');
	ck_assert_int_eq(decodedData[8], '1');
}
END_TEST

Suite*base64_testSuite(void)
{
	Suite *s = suite_create("base64_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, base64_test_basic);
	suite_add_tcase(s,tc_core);
	return s;
}

int main (void)
{
	int number_failed = 0;

	Suite* s = base64_testSuite();
	SRunner* sr = srunner_create(s);
	srunner_run_all(sr,CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}

