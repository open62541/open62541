#include <stdlib.h> // EXIT_SUCCESS
#include "util/ua_base64.h"
#include "check.h"

START_TEST(base64_test_2padding)
{
	//this is base64'd ASCII string "open62541!"
	UA_String encodedString = {16, "b3BlbjYyNTQxIQ=="};

	//assure that we allocate exactly 10 bytes
	ck_assert_int_eq(UA_base64_getDecodedSize(&encodedString), 10);

	UA_Byte* decodedData = (UA_Byte*)malloc(UA_base64_getDecodedSize(&encodedString));

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
	ck_assert_int_eq(decodedData[9], '!');

	free(decodedData);
}
END_TEST

START_TEST(base64_test_1padding)
{

	//this is base64'd ASCII string "open62541!!"
	UA_String encodedString = {16, "b3BlbjYyNTQxISE="};

	//assure that we allocate exactly 11 bytes
	ck_assert_int_eq(UA_base64_getDecodedSize(&encodedString), 11);

	UA_Byte* decodedData = (UA_Byte*)malloc(UA_base64_getDecodedSize(&encodedString));

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
	ck_assert_int_eq(decodedData[9], '!');
	ck_assert_int_eq(decodedData[10], '!');

	free(decodedData);
}
END_TEST

START_TEST(base64_test_0padding)
{

	//this is base64'd ASCII string "open62541"
	UA_String encodedString = {12, "b3BlbjYyNTQx"};

	//assure that we allocate exactly 9 bytes
	ck_assert_int_eq(UA_base64_getDecodedSize(&encodedString), 9);

	UA_Byte* decodedData = (UA_Byte*)malloc(UA_base64_getDecodedSize(&encodedString));

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

	free(decodedData);
}
END_TEST

Suite*base64_testSuite(void)
{
	Suite *s = suite_create("base64_test");
	TCase *tc_core = tcase_create("Core");
	tcase_add_test(tc_core, base64_test_2padding);
	tcase_add_test(tc_core, base64_test_1padding);
	tcase_add_test(tc_core, base64_test_0padding);
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

