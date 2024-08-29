/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../arch/eventloop_common/timer.h"

#include <check.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define N_EVENTS 10000

size_t count = 0;

static void
timerCallback(void *application, void *data) {
    count++;
}

/* Create empty events with different callback intervals */
static void
createEvents(UA_Timer *t, UA_UInt32 events) {
    for(size_t i = 0; i < events; i++) {
        UA_Double interval = (UA_Double)i+1;
        UA_StatusCode retval =
            UA_Timer_addRepeatedCallback(t, timerCallback, NULL, NULL, interval, 0, NULL, UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    }
}

START_TEST(benchmarkTimer) {
    UA_Timer timer;
    UA_Timer_init(&timer);
    createEvents(&timer, N_EVENTS);

    clock_t begin = clock();
    UA_DateTime now = 0;
    for(size_t i = 0; i < 1000; i++) {
        UA_DateTime next = UA_Timer_process(&timer, now);
        /* At least 100 msec distance between _process */
        now = next + (UA_DATETIME_MSEC * 100);
        if(next > now)
            now = next;
    }

    clock_t finish = clock();
    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);
    printf("%lu callbacks\n", (unsigned long)count);

    UA_Timer_clear(&timer);
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test Event Timer");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, benchmarkTimer);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
