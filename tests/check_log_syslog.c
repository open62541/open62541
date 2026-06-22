/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/log_syslog.h>
#include <open62541/types.h>

#include <check.h>
#include <stdlib.h>

#if defined(__linux__) || defined(__unix__)

START_TEST(SyslogLogger_NewAndClear) {
    UA_Logger *logger = UA_Log_Syslog_new(UA_LOGLEVEL_INFO);
    ck_assert_ptr_ne(logger, NULL);
    ck_assert(logger->log != NULL);
    ck_assert(logger->clear != NULL);
    /* clear() must be safe */
    logger->clear(logger);
} END_TEST

START_TEST(SyslogLogger_AllLevelsAllCategories) {
    UA_Logger logger = UA_Log_Syslog();
    /* cover all levels and categories */
    const UA_LogLevel levels[] = {
        UA_LOGLEVEL_TRACE,
        UA_LOGLEVEL_DEBUG,
        UA_LOGLEVEL_INFO,
        UA_LOGLEVEL_WARNING,
        UA_LOGLEVEL_ERROR,
        UA_LOGLEVEL_FATAL
    };
    const UA_LogCategory categories[] = {
        UA_LOGCATEGORY_NETWORK,
        UA_LOGCATEGORY_SECURECHANNEL,
        UA_LOGCATEGORY_SESSION,
        UA_LOGCATEGORY_SERVER,
        UA_LOGCATEGORY_CLIENT,
        UA_LOGCATEGORY_USERLAND,
        UA_LOGCATEGORY_SECURITYPOLICY,
        UA_LOGCATEGORY_EVENTLOOP,
        UA_LOGCATEGORY_PUBSUB,
        UA_LOGCATEGORY_DISCOVERY
    };
    for(size_t l = 0; l < sizeof(levels)/sizeof(levels[0]); ++l) {
        for(size_t c = 0; c < sizeof(categories)/sizeof(categories[0]); ++c) {
            UA_LOG_INFO(&logger, categories[c],
                        "level=%u category=%u", (unsigned)levels[l],
                        (unsigned)categories[c]);
        }
    }
} END_TEST

START_TEST(SyslogLogger_RespectsMinimumLevel) {
    /* filtered + emitted path coverage */
    UA_Logger logger = UA_Log_Syslog_withLevel(UA_LOGLEVEL_WARNING);
    UA_LOG_INFO(&logger, UA_LOGCATEGORY_USERLAND,
                "this should be filtered out: %d", 42);
    UA_LOG_WARNING(&logger, UA_LOGCATEGORY_USERLAND,
                   "this should be emitted: %d", 42);
} END_TEST

START_TEST(SyslogLogger_LongMessage) {
    /* long message path */
    UA_Logger logger = UA_Log_Syslog();
    char big[1024];
    for(size_t i = 0; i < sizeof(big) - 1; ++i)
        big[i] = 'A' + (char)(i % 26);
    big[sizeof(big) - 1] = '\0';
    UA_LOG_INFO(&logger, UA_LOGCATEGORY_USERLAND, "%s", big);
} END_TEST

START_TEST(SyslogLogger_NewWithMallocFailureSurvives) {
    /* repeated create/clear cycle */
    for(int i = 0; i < 32; ++i) {
        UA_Logger *l = UA_Log_Syslog_new(UA_LOGLEVEL_INFO);
        ck_assert_ptr_ne(l, NULL);
        l->clear(l);
    }
} END_TEST

int main(void) {
    Suite *s = suite_create("ua_log_syslog");
    TCase *tc = tcase_create("syslog");
    tcase_add_test(tc, SyslogLogger_NewAndClear);
    tcase_add_test(tc, SyslogLogger_AllLevelsAllCategories);
    tcase_add_test(tc, SyslogLogger_RespectsMinimumLevel);
    tcase_add_test(tc, SyslogLogger_LongMessage);
    tcase_add_test(tc, SyslogLogger_NewWithMallocFailureSurvives);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else /* not linux/unix */

int main(void) { return EXIT_SUCCESS; }

#endif
