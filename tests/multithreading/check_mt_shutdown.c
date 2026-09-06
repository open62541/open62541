/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "server/ua_server_internal.h"
#include "test_helpers.h"
#include "thread_wrapper.h"

#include <check.h>
#include <stdlib.h>
#include <time.h>

/* Model an external event-loop callback that needs the server mutex to finish.
 * Start it from the particular shutdown iteration under test, not before the
 * initial unlocked iteration (which would hide the regression). */
static struct {
    UA_Server *server;
    UA_ServerComponent *component;
    UA_EventSource source;
    UA_StatusCode (*run)(UA_EventLoop *, UA_UInt32);
    THREAD_HANDLE worker;
    MUTEX_HANDLE mutex;
    UA_Boolean started;
    UA_Boolean completed;
    UA_Boolean stopComponent;
    UA_StatusCode result;
    unsigned iterations;
} probe;

THREAD_CALLBACK(shutdownCallback) {
    size_t index = 0;
    UA_String uri = UA_STRING("http://opcfoundation.org/UA/");
    /* This public API acquires the server mutex. */
    UA_StatusCode result = UA_Server_getNamespaceByName(probe.server, uri, &index);
    ck_assert(MUTEX_LOCK(probe.mutex));
    probe.result = result;
    probe.completed = true;
    ck_assert(MUTEX_UNLOCK(probe.mutex));
    return 0;
}

static UA_StatusCode
componentStart(UA_ServerComponent *sc, UA_Server *server) {
    sc->state = UA_LIFECYCLESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
componentStop(UA_ServerComponent *sc) {
    sc->state = UA_LIFECYCLESTATE_STOPPING;
}

static UA_StatusCode
componentClear(UA_ServerComponent *sc) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sourceStart(UA_EventSource *source) {
    source->state = UA_EVENTSOURCESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
sourceStop(UA_EventSource *source) {
    source->state = UA_EVENTSOURCESTATE_STOPPING;
}

static UA_StatusCode
sourceFree(UA_EventSource *source) {
    /* Embedded in probe; no allocation to free. */
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
runWithShutdownCallback(UA_EventLoop *el, UA_UInt32 timeout) {
    ++probe.iterations;
    UA_Boolean targetIteration = probe.stopComponent ?
        (probe.iterations == 2) : (el->state == UA_EVENTLOOPSTATE_STOPPING);
    if(probe.started || !targetIteration)
        return probe.run(el, timeout);

    probe.started = true;
#ifdef UA_ARCHITECTURE_WIN32
    THREAD_CREATE(probe.worker, shutdownCallback);
    ck_assert_ptr_ne(probe.worker, NULL);
#else
    ck_assert_int_eq(THREAD_CREATE(probe.worker, shutdownCallback), 0);
#endif

    /* Use the real monotonic clock, not the server's testing clock. A deadline
     * lets the unpatched implementation report failure and clean up, rather
     * than deadlocking the test runner indefinitely. */
    UA_DateTime deadline = UA_DateTime_nowMonotonic() + 5 * UA_DATETIME_SEC;
    UA_Boolean completed;
    do {
        ck_assert(MUTEX_LOCK(probe.mutex));
        completed = probe.completed;
        ck_assert(MUTEX_UNLOCK(probe.mutex));
        if(completed)
            break;
#ifdef UA_ARCHITECTURE_WIN32
        Sleep(1);
#else
        struct timespec delay = {0, 1000000};
        nanosleep(&delay, NULL);
#endif
    } while(UA_DateTime_nowMonotonic() < deadline);

    /* Release the artificial stop barrier even on failure. The worker can
     * finish once shutdown returns and releases the server mutex. */
    if(probe.stopComponent) {
        lockServer(probe.server);
        probe.component->state = UA_LIFECYCLESTATE_STOPPED;
        unlockServer(probe.server);
    } else {
        el->lock(el);
        probe.source.state = UA_EVENTSOURCESTATE_STOPPED;
        el->unlock(el);
    }
    UA_StatusCode result = probe.run(el, 0);
    return completed ? result : UA_STATUSCODE_BADTIMEOUT;
}

static void
checkShutdown(UA_Boolean stopComponent) {
    memset(&probe, 0, sizeof(probe));
    probe.stopComponent = stopComponent;
    ck_assert(MUTEX_INIT(probe.mutex));
    probe.server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(probe.server, NULL);
    UA_ServerConfig *config = UA_Server_getConfig(probe.server);
    /* Bind only to loopback and let the OS choose an unused port. */
    UA_Array_delete(config->serverUrls, config->serverUrlsSize, &UA_TYPES[UA_TYPES_STRING]);
    config->serverUrls = UA_String_new();
    ck_assert_ptr_ne(config->serverUrls, NULL);
    config->serverUrlsSize = 1;
    config->serverUrls[0] = UA_STRING_ALLOC("opc.tcp://127.0.0.1:0");
    ck_assert_ptr_ne(config->serverUrls[0].data, NULL);
    UA_EventLoop *el = config->eventLoop;

    if(stopComponent) {
        probe.component = (UA_ServerComponent*)UA_calloc(1, sizeof(UA_ServerComponent));
        ck_assert_ptr_ne(probe.component, NULL);
        probe.component->start = componentStart;
        probe.component->stop = componentStop;
        probe.component->clear = componentClear;
        addServerComponent(probe.server, probe.component, NULL);
    } else {
        probe.source.eventSourceType = UA_EVENTSOURCETYPE_INTERRUPTMANAGER;
        probe.source.start = sourceStart;
        probe.source.stop = sourceStop;
        probe.source.free = sourceFree;
        ck_assert_uint_eq(el->registerEventSource(el, &probe.source), UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(UA_Server_run_startup(probe.server), UA_STATUSCODE_GOOD);
    probe.run = el->run;
    el->run = runWithShutdownCallback;
    UA_StatusCode result = UA_Server_run_shutdown(probe.server);
    el->run = probe.run;
    if(probe.started) {
        THREAD_JOIN(probe.worker);
#ifdef UA_ARCHITECTURE_WIN32
        CloseHandle(probe.worker);
#endif
    }
    UA_Boolean stopped = (UA_Server_getLifecycleState(probe.server) ==
                          UA_LIFECYCLESTATE_STOPPED);
    UA_Boolean loopStopped = (el->state == UA_EVENTLOOPSTATE_STOPPED);
    UA_Server_delete(probe.server);
    ck_assert(MUTEX_DESTROY(probe.mutex));

    ck_assert(probe.started);
    ck_assert(probe.completed);
    ck_assert_uint_eq(probe.result, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result, UA_STATUSCODE_GOOD);
    ck_assert(stopped);
    ck_assert(loopStopped);
}

START_TEST(shutdownComponentsWithoutServerLock) {
    checkShutdown(true);
} END_TEST

START_TEST(shutdownEventLoopWithoutServerLock) {
    checkShutdown(false);
} END_TEST

int main(void) {
    Suite *s = suite_create("Server shutdown locking");
    TCase *tc = tcase_create("External event-loop callbacks");
    tcase_add_test(tc, shutdownComponentsWithoutServerLock);
    tcase_add_test(tc, shutdownEventLoopWithoutServerLock);
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
