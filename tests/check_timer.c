/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../arch/common/timer.h"
#include "thread_wrapper.h"

#include <check.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define N_EVENTS 10000

static size_t count = 0;

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
            UA_Timer_add(t, timerCallback, NULL, NULL, interval, 0, NULL,
                         UA_TIMERPOLICY_CURRENTTIME, NULL);
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

START_TEST(batchWithinWindow) {
    UA_Timer timer;
    UA_Timer_init(&timer);
    count = 0;
    ck_assert_uint_eq(UA_Timer_add(&timer, timerCallback, NULL, NULL, 100, 0,
                                   NULL, UA_TIMERPOLICY_CURRENTTIME, NULL),
                      UA_STATUSCODE_GOOD);
    /* Equal intervals within the quarter-interval window run together. */
    ck_assert_uint_eq(UA_Timer_add(&timer, timerCallback, NULL, NULL, 100,
                                   5 * UA_DATETIME_MSEC, NULL,
                                   UA_TIMERPOLICY_CURRENTTIME, NULL),
                      UA_STATUSCODE_GOOD);
    /* An entry outside that window must retain its own deadline. */
    ck_assert_uint_eq(UA_Timer_add(&timer, timerCallback, NULL, NULL, 100,
                                   30 * UA_DATETIME_MSEC, NULL,
                                   UA_TIMERPOLICY_CURRENTTIME, NULL),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Timer_process(&timer, 100 * UA_DATETIME_MSEC) ==
              130 * UA_DATETIME_MSEC);
    ck_assert_uint_eq(count, 2);
    UA_Timer_clear(&timer);
} END_TEST

START_TEST(addWithoutCallback) {
    UA_Timer timer;
    UA_Timer_init(&timer);
    ck_assert_uint_eq(UA_Timer_add(&timer, NULL, NULL, NULL, 10, 0, NULL,
                                   UA_TIMERPOLICY_CURRENTTIME, NULL),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert(UA_Timer_next(&timer) == UA_INT64_MAX);
    ck_assert(UA_Timer_process(&timer, 100 * UA_DATETIME_MSEC) == UA_INT64_MAX);
    UA_Timer_clear(&timer);
} END_TEST

START_TEST(addNonpositiveRepeatedInterval) {
    const UA_TimerPolicy policies[] = {UA_TIMERPOLICY_CURRENTTIME,
                                       UA_TIMERPOLICY_BASETIME};
    const UA_Double intervals[] = {0, -10};
    UA_Timer timer;
    UA_Timer_init(&timer);
    count = 0;

    ck_assert_uint_eq(UA_Timer_add(&timer, timerCallback, NULL, NULL,
                                   intervals[_i % 2], 0, NULL,
                                   policies[_i / 2], NULL),
                      UA_STATUSCODE_BADINTERNALERROR);
    ck_assert(UA_Timer_next(&timer) == UA_INT64_MAX);
    ck_assert(UA_Timer_process(&timer, 100 * UA_DATETIME_MSEC) == UA_INT64_MAX);
    ck_assert_uint_eq(count, 0);
    UA_Timer_clear(&timer);
} END_TEST

START_TEST(modifyNonpositiveRepeatedInterval) {
    const UA_TimerPolicy policies[] = {UA_TIMERPOLICY_CURRENTTIME,
                                       UA_TIMERPOLICY_BASETIME};
    const UA_Double intervals[] = {0, -10};
    UA_Timer timer;
    UA_Timer_init(&timer);
    count = 0;
    UA_UInt64 callbackId;
    ck_assert_uint_eq(UA_Timer_add(&timer, timerCallback, NULL, NULL, 10, 0,
                                   NULL, policies[_i / 2], &callbackId),
                      UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(UA_Timer_modify(&timer, callbackId, intervals[_i % 2],
                                      5 * UA_DATETIME_MSEC, NULL,
                                      policies[_i / 2]),
                      UA_STATUSCODE_BADINTERNALERROR);

    /* A rejected modification must preserve the original repeating schedule. */
    ck_assert(UA_Timer_next(&timer) == 10 * UA_DATETIME_MSEC);
    ck_assert(UA_Timer_process(&timer, 10 * UA_DATETIME_MSEC - 1) ==
              10 * UA_DATETIME_MSEC);
    ck_assert_uint_eq(count, 0);
    ck_assert(UA_Timer_process(&timer, 10 * UA_DATETIME_MSEC) ==
              20 * UA_DATETIME_MSEC);
    ck_assert_uint_eq(count, 1);
    ck_assert(UA_Timer_process(&timer, 20 * UA_DATETIME_MSEC) ==
              30 * UA_DATETIME_MSEC);
    ck_assert_uint_eq(count, 2);
    UA_Timer_clear(&timer);
} END_TEST

START_TEST(modifyToOneShot) {
    const struct {
        UA_Double interval;
        UA_DateTime deadline;
        UA_Boolean useBaseTime;
    } cases[] = {
        {0, 100 * UA_DATETIME_MSEC, false},
        {-10, 90 * UA_DATETIME_MSEC, false},
        {0, 90 * UA_DATETIME_MSEC, true},
        {0, 100 * UA_DATETIME_MSEC, true},
        {0, 110 * UA_DATETIME_MSEC, true}
    };
    UA_DateTime now = 100 * UA_DATETIME_MSEC;
    UA_DateTime deadline = cases[_i].deadline;
    UA_DateTime baseTime = deadline;
    UA_Timer timer;
    UA_Timer_init(&timer);
    count = 0;
    UA_UInt64 callbackId;
    ck_assert_uint_eq(UA_Timer_add(&timer, timerCallback, NULL, NULL, 100,
                                   now, NULL, UA_TIMERPOLICY_CURRENTTIME,
                                   &callbackId),
                      UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(UA_Timer_modify(&timer, callbackId, cases[_i].interval, now,
                                      cases[_i].useBaseTime ? &baseTime : NULL,
                                      UA_TIMERPOLICY_ONCE),
                      UA_STATUSCODE_GOOD);
    ck_assert(UA_Timer_next(&timer) == deadline);
    ck_assert_uint_eq(count, 0);

    if(deadline > now) {
        ck_assert(UA_Timer_process(&timer, deadline - 1) == deadline);
        ck_assert_uint_eq(count, 0);
    }

    /* Overdue one-shot timers run on the next process call, without repeating. */
    ck_assert(UA_Timer_process(&timer, deadline > now ? deadline : now) ==
              UA_INT64_MAX);
    ck_assert_uint_eq(count, 1);
    ck_assert(UA_Timer_next(&timer) == UA_INT64_MAX);
    ck_assert(UA_Timer_process(&timer, now + UA_DATETIME_SEC) == UA_INT64_MAX);
    ck_assert_uint_eq(count, 1);
    ck_assert_uint_eq(UA_Timer_modify(&timer, callbackId, 10, now, NULL,
                                      UA_TIMERPOLICY_CURRENTTIME),
                      UA_STATUSCODE_BADNOTFOUND);
    UA_Timer_clear(&timer);
} END_TEST

#if UA_MULTITHREADING >= 100
typedef struct {
    UA_DateTime now;
    UA_StatusCode status;
} TimerThreadContext;

/* Independent timers must not share mutable batching state. */
THREAD_CALLBACK_PARAM(addTimers, argument) {
    TimerThreadContext *context = (TimerThreadContext*)argument;
    UA_Timer timer;
    UA_Timer_init(&timer);
    for(size_t i = 0; i < 1000; i++) {
        context->status = UA_Timer_add(&timer, timerCallback, NULL, NULL,
                                      (UA_Double)(i % 100 + 1), context->now,
                                      NULL, UA_TIMERPOLICY_CURRENTTIME, NULL);
        if(context->status != UA_STATUSCODE_GOOD)
            break;
    }
    UA_Timer_clear(&timer);
    return 0;
}

START_TEST(independentTimerBatching) {
    TimerThreadContext contexts[4] = {{0, 0}, {1000000, 0}, {2000000, 0}, {3000000, 0}};
    THREAD_HANDLE threads[4];
    for(size_t i = 0; i < 4; i++)
        THREAD_CREATE_PARAM(threads[i], addTimers, contexts[i]);
    for(size_t i = 0; i < 4; i++) {
        THREAD_JOIN(threads[i]);
        ck_assert_uint_eq(contexts[i].status, UA_STATUSCODE_GOOD);
    }
} END_TEST
#endif

int main(void) {
    Suite *s  = suite_create("Test Event Timer");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, benchmarkTimer);
    tcase_add_test(tc, batchWithinWindow);
    tcase_add_test(tc, addWithoutCallback);
    tcase_add_loop_test(tc, addNonpositiveRepeatedInterval, 0, 4);
    tcase_add_loop_test(tc, modifyNonpositiveRepeatedInterval, 0, 4);
    tcase_add_loop_test(tc, modifyToOneShot, 0, 5);
#if UA_MULTITHREADING >= 100
    tcase_add_test(tc, independentTimerBatching);
#endif
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
