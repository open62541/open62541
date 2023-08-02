/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/types.h"
#include "open62541/types_generated.h"

#include "testing_clock.h"

#include <signal.h>
#include <stdlib.h>
#include <check.h>

#ifndef _WIN32
#define TESTSIG SIGUSR1
#else
#define TESTSIG SIGINT
#endif

unsigned counter = 0;

static void
interruptCallback(UA_InterruptManager *im,
                  uintptr_t interruptHandle, void *interruptContext,
                  const UA_KeyValueMap *instanceInfos) {
    counter++;
}

START_TEST(catchInterrupt) {
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    UA_InterruptManager *im = UA_InterruptManager_new_POSIX(UA_STRING("im1"));
    el->registerEventSource(el, &im->eventSource);

    im->registerInterrupt(im, TESTSIG, &UA_KEYVALUEMAP_NULL, interruptCallback, NULL);
    el->start(el);

    /* Send signal to self*/
    raise(TESTSIG);
    el->run(el, 0);
    ck_assert_uint_eq(counter, 1);

    /* Send signal to self*/
    raise(TESTSIG);
    el->run(el, 0);
    ck_assert_uint_eq(counter, 2);

    /* Stop the EventLoop */
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    el->free(el);
    el = NULL;
} END_TEST

START_TEST(registerDuplicate) {
    UA_EventLoop *el = UA_EventLoop_new_POSIX(UA_Log_Stdout);
    UA_InterruptManager *im = UA_InterruptManager_new_POSIX(UA_STRING("im1"));
    el->registerEventSource(el, &im->eventSource);

    el->start(el);

    UA_StatusCode res =
        im->registerInterrupt(im, TESTSIG, &UA_KEYVALUEMAP_NULL, interruptCallback, NULL);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);

    /* Registering the same signal twice must fail */
    res = im->registerInterrupt(im, TESTSIG, &UA_KEYVALUEMAP_NULL, interruptCallback, NULL);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);

    /* Stop the EventLoop */
    el->stop(el);
    while(el->state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    el->free(el);
    el = NULL;
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test EventLoop Interrupts");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, catchInterrupt);
    tcase_add_test(tc, registerDuplicate);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
