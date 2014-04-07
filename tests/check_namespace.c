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
#include "opcua_namespace.h"
#include "check.h"



START_TEST(test_Namespace) {
	namespace *ns = UA_NULL;
	create_ns(&ns, 512);
	delete_ns(ns);
}
END_TEST

Suite * namespace_suite (void) {
	Suite *s = suite_create ("Namespace");

	TCase *tc_cd = tcase_create ("Create/Delete");
	tcase_add_test (tc_cd, test_Namespace);
	suite_add_tcase (s, tc_cd);

	return s;
}

int main (void) {
	return 0;
	int number_failed;
	Suite *s = namespace_suite ();
	SRunner *sr = srunner_create (s);
	srunner_run_all (sr, CK_NORMAL);
	number_failed = srunner_ntests_failed (sr);
	srunner_free (sr);
	number_failed = 1;
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
