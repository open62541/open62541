/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef OPEN62541_CHECK_SUBTEST_LOGGING_H_
#define OPEN62541_CHECK_SUBTEST_LOGGING_H_

#include <open62541/config.h>
#include <check.h>
#include <stdio.h>

static void
ua_check_subtest_begin(void) {
    const char *name = tcase_name();
    fprintf(stderr, "[check][begin] %s\n", name ? name : "<unknown>");
    fflush(stderr);
}

static void
ua_check_subtest_end(void) {
    const char *name = tcase_name();
    fprintf(stderr, "[check][end] %s\n", name ? name : "<unknown>");
    fflush(stderr);
}

static TCase *
ua_check_tcase_create_with_subtest_logging(const char *name) {
    TCase *tc = tcase_create(name);
    tcase_add_checked_fixture(tc, ua_check_subtest_begin, ua_check_subtest_end);
    return tc;
}

#undef tcase_create
#define tcase_create(name) ua_check_tcase_create_with_subtest_logging(name)

#endif /* OPEN62541_CHECK_SUBTEST_LOGGING_H_ */
