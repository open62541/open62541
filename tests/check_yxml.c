/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_types_encoding_xml.h"
#include <check.h>

START_TEST(parseElement) {
    const char *xml = "<test attr1=\"attr\">my-content</test>";
    xml_token tokens[32];
    xml_result r = xml_tokenize(xml, (unsigned int)strlen(xml), tokens, 32);
    ck_assert(r.error == XML_ERROR_NONE);
} END_TEST

static Suite *testSuite_yxml(void) {
    TCase *tc_parse= tcase_create("yxml_parse");
    tcase_add_test(tc_parse, parseElement);

    Suite *s = suite_create("Test XML decoding with the yxml library");
    suite_add_tcase(s, tc_parse);
    return s;
}

int main(void) {
    int number_failed = 0;
    Suite *s = testSuite_yxml();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
