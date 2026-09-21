/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/plugin/eventloop.h>
#if defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)
#include "../arch/posix/eventloop_posix.h"
#endif
#include "testing_clock.h"
#include <time.h>
#include <stdio.h>

#include <stdlib.h>
#include <check.h>

#ifdef UA_ARCHITECTURE_WIN32
# define UA_TEST_EVENTLOOP_NEW UA_EventLoop_new_WIN32
#else
# define UA_TEST_EVENTLOOP_NEW UA_EventLoop_new_POSIX
#endif

#define N_EVENTS 10000

static UA_EventLoop *el;
static size_t count = 0;

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
            el->addTimer(el, timerCallback, NULL, NULL, interval, NULL,
                         UA_TIMERPOLICY_CURRENTTIME, NULL);
        ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
    }
}

START_TEST(benchmarkTimer) {
#if defined(UA_ARCHITECTURE_LWIP)
    el = UA_EventLoop_new_LWIP(NULL, NULL);
#elif defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32)
    el = UA_TEST_EVENTLOOP_NEW(NULL);
#else
#error Add other EventLoop implementations here
#endif

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

#if defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)
static short readyEvents;

static void
recordReadyEvents(UA_EventSource *es, UA_RegisteredFD *rfd, short event) {
    (void)es;
    (void)rfd;
    readyEvents |= event;
}

/* Pending input must not starve writable callbacks on the same socket. */
START_TEST(simultaneousReadWrite) {
    int sockets[2];
    ck_assert_int_eq(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets), 0);
    UA_EventLoop *loop =
#ifdef UA_ENABLE_EVENTLOOP_GLIB
        _i ? UA_EventLoop_new_GLib(NULL, NULL) :
#endif
        UA_EventLoop_new_POSIX(NULL);
    ck_assert_ptr_nonnull(loop);
    ck_assert_uint_eq(loop->start(loop), UA_STATUSCODE_GOOD);

    UA_RegisteredFD rfd = {0};
    rfd.fd = sockets[0];
    rfd.listenEvents = UA_FDEVENT_IN | UA_FDEVENT_OUT;
    rfd.eventSourceCB = recordReadyEvents;
    UA_EventLoopPOSIX *posixLoop = (UA_EventLoopPOSIX*)loop;
    UA_LOCK(&posixLoop->elMutex);
    ck_assert_uint_eq(UA_EventLoopPOSIX_registerFD(posixLoop, &rfd),
                      UA_STATUSCODE_GOOD);
    UA_UNLOCK(&posixLoop->elMutex);
    ck_assert_int_eq(write(sockets[1], "x", 1), 1);
    readyEvents = 0;
    ck_assert_uint_eq(loop->run(loop, 0), UA_STATUSCODE_GOOD);

    UA_LOCK(&posixLoop->elMutex);
    UA_EventLoopPOSIX_deregisterFD(posixLoop, &rfd);
    UA_UNLOCK(&posixLoop->elMutex);
    close(sockets[0]);
    close(sockets[1]);
    loop->stop(loop);
    loop->run(loop, 0);
    loop->free(loop);
    ck_assert_int_eq(readyEvents, UA_FDEVENT_IN | UA_FDEVENT_OUT);
} END_TEST
#endif

#ifdef UA_ARCHITECTURE_POSIX
START_TEST(localTimeOffset) {
    const char *old = getenv("TZ");
    UA_Boolean hadTimezone = (old != NULL);
    char *saved = old ? strdup(old) : NULL;
    ck_assert(!hadTimezone || saved);
    const char *zones[] = {"UTC0", "EST5", "IST-5:30", "NPT-5:45", "PLUS-14", "MINUS12"};
    const UA_Int64 offsets[] = {0, -18000, 19800, 20700, 50400, -43200};
    for(size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
        ck_assert_int_eq(setenv("TZ", zones[i], 1), 0);
        tzset();
        ck_assert(UA_DateTime_localTimeUtcOffset() == offsets[i] * UA_DATETIME_SEC);
    }
    if(hadTimezone) {
        ck_assert_int_eq(setenv("TZ", saved, 1), 0);
    } else {
        ck_assert_int_eq(unsetenv("TZ"), 0);
    }
    free(saved);
    tzset();
} END_TEST
#endif

int main(void) {
    Suite *s  = suite_create("Test EventLoop");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, benchmarkTimer);
#ifdef UA_ARCHITECTURE_POSIX
#ifndef UA_ARCHITECTURE_LWIP
#ifdef UA_ENABLE_EVENTLOOP_GLIB
    tcase_add_loop_test(tc, simultaneousReadWrite, 0, 2);
#else
    tcase_add_test(tc, simultaneousReadWrite);
#endif
#endif
    tcase_add_test(tc, localTimeOffset);
#endif
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
