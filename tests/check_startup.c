#include "ua_application.h"
#include "check.h"

START_TEST(testAppMockup) {
	appMockup_init();
}
END_TEST


Suite *testSuite_builtin(void) {
	Suite *s = suite_create("Test server startup");

	TCase *tc_ns0 = tcase_create("initialise namespace 0");
	tcase_add_test(tc_ns0, testAppMockup);
	suite_add_tcase(s, tc_ns0);

	return s;
}


int main(void) {
	int      number_failed = 0;
	Suite   *s;
	SRunner *sr;

	s  = testSuite_builtin();
	sr = srunner_create(s);
	//srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed += srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
