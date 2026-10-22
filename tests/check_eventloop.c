/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#include "testing_clock.h"
#include "../arch/eventloop_posix.h"
#include <time.h>
#include <stdio.h>
#ifdef UA_ARCHITECTURE_POSIX
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <check.h>

#define N_EVENTS 10000

UA_EventLoop *el;
size_t count = 0;

static void
timerCallback(void *application, void *data) {
    count++;
}

/* Create empty events with different callback intervals */
static void
createEvents(UA_UInt32 events) {
    for(size_t i = 0; i < events; i++) {
        UA_Double interval = (UA_Double)i+1;
        UA_StatusCode retval =
            el->addCyclicCallback(el, timerCallback, NULL, NULL, interval, NULL,
                                  UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    }
}

START_TEST(benchmarkTimer) {
    el = UA_EventLoop_new_POSIX(NULL);
    createEvents(N_EVENTS);

    clock_t begin = clock();
    for(size_t i = 0; i < 1000; i++) {
        UA_DateTime next = el->run(el, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }

    clock_t finish = clock();
    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);
    printf("%lu callbacks\n", (unsigned long)count);

    el->stop(el);
    el->free(el);
    el = NULL;
} END_TEST

#ifdef UA_ARCHITECTURE_POSIX
/* The select backend can only handle descriptors below FD_SETSIZE. A larger
 * descriptor has to be rejected when it is registered. It must not be put into
 * a fd_set later. The other backends reject the descriptor as it is not open. */
START_TEST(registerFDOutOfSelectRange) {
    el = UA_EventLoop_new_POSIX(NULL);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    UA_EventLoopPOSIX *elp = (UA_EventLoopPOSIX*)el;

    /* Pick a descriptor above FD_SETSIZE that is not open. Tools such as
     * Valgrind keep their own descriptors in this range. */
    int badfd = FD_SETSIZE + 5;
    while(fcntl(badfd, F_GETFD) != -1)
        badfd++;

    UA_RegisteredFD rfd;
    memset(&rfd, 0, sizeof(UA_RegisteredFD));
    rfd.fd = badfd;
    rfd.listenEvents = UA_FDEVENT_IN;
    UA_LOCK(&elp->elMutex);
    UA_StatusCode res = UA_EventLoopPOSIX_registerFD(elp, &rfd);
    UA_UNLOCK(&elp->elMutex);
    ck_assert_uint_ne(res, UA_STATUSCODE_GOOD);

    /* A normal descriptor can be registered */
    int pipefd[2];
    ck_assert_int_eq(pipe(pipefd), 0);
    UA_RegisteredFD rfd2;
    memset(&rfd2, 0, sizeof(UA_RegisteredFD));
    rfd2.fd = pipefd[0];
    rfd2.listenEvents = UA_FDEVENT_IN;
    UA_LOCK(&elp->elMutex);
    res = UA_EventLoopPOSIX_registerFD(elp, &rfd2);
    ck_assert_uint_eq(res, UA_STATUSCODE_GOOD);
    UA_EventLoopPOSIX_deregisterFD(elp, &rfd2);
    UA_UNLOCK(&elp->elMutex);
    close(pipefd[0]);
    close(pipefd[1]);

    el->stop(el);
    el->free(el);
    el = NULL;
} END_TEST
#endif

int main(void) {
    Suite *s  = suite_create("Test EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, benchmarkTimer);
#ifdef UA_ARCHITECTURE_POSIX
    tcase_add_test(tc, registerFDOutOfSelectRange);
#endif
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
