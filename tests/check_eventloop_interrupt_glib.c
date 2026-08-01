/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Mirrors check_eventloop_interrupt.c, but drives the native GLib
 * InterruptManager (UA_InterruptManager_new_GLib, arch/posix/
 * eventloop_glib_interrupt.c) on the GLib-backed EventLoop
 * (UA_EventLoop_new_GLib) instead of UA_InterruptManager_new_POSIX on
 * UA_EventLoop_new_POSIX. Unlike the POSIX InterruptManager -- which
 * installs a sigaction handler writing into a self-pipe and registers the
 * read end as a generic fd -- the GLib InterruptManager registers each
 * signal as a native GLib GSource (g_unix_signal_source_new), so GLib itself
 * handles the signal delivery.
 *
 * Two behavioral differences from the POSIX variant, both inherent to GLib
 * (verified against plain GLib, independent of open62541) rather than bugs
 * in the InterruptManager:
 *
 * - GLib delivers unix signals via an internal worker thread that wakes the
 *   target GMainContext asynchronously. A raise() is therefore not
 *   guaranteed to be observed by a single immediate non-blocking iteration
 *   (may_block=FALSE) right after it -- even many back-to-back non-blocking
 *   iterations can race and see nothing. Only a *blocking* iteration (or
 *   several, each blocking) reliably picks it up. Tests here poll with
 *   `el->run(el, <timeout>)` in a retry loop instead of a single
 *   `el->run(el, 0)`.
 *
 * - If two independent GSources (e.g. from two independent
 *   UA_InterruptManager_new_GLib instances) both watch the *same* signal
 *   number, GLib delivers one raised signal to only one of them, not both --
 *   this matches GLib's own documented "shared handler" semantics for
 *   g_unix_signal_source_new/g_unix_signal_add, which differ from the POSIX
 *   InterruptManager's fan-out-to-all-registered-listeners behavior. The
 *   multipleInterruptManagers case below therefore uses two *different*
 *   signals (one per manager/EventLoop) to verify that two independent
 *   GLib-backed EventLoop+InterruptManager pairs work correctly side by
 *   side, rather than testing same-signal fan-out (which GLib does not
 *   support). It also gives each EventLoop its own GMainContext, since both
 *   attaching to the default context would make one EventLoop's `run()`
 *   dispatch the other's sources too. */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/types.h"
#include "open62541/types_generated.h"

#include "testing_clock.h"

#include <glib.h>
#include <signal.h>
#include <stdlib.h>
#include <check.h>

#define TESTSIG SIGUSR1
#define TESTSIG2 SIGUSR2

static unsigned counter = 0;

static void
interruptCallback(UA_InterruptManager *im,
                  uintptr_t interruptHandle, void *interruptContext,
                  const UA_KeyValueMap *instanceInfos) {
    counter++;
}

START_TEST(catchInterrupt) {
    UA_EventLoop *el = UA_EventLoop_new_GLib(UA_Log_Stdout, NULL);
    UA_InterruptManager *im = UA_InterruptManager_new_GLib(UA_STRING("im1"));
    el->registerEventSource(el, &im->eventSource);

    im->registerInterrupt(im, TESTSIG, &UA_KEYVALUEMAP_NULL, interruptCallback, NULL);
    el->start(el);

    /* Send signal to self. GLib forwards it asynchronously (see file header
     * comment), so poll with blocking iterations until it arrives. */
    raise(TESTSIG);
    for(size_t i = 0; i < 20 && counter < 1; i++)
        el->run(el, 50);
    ck_assert_uint_eq(counter, 1);

    raise(TESTSIG);
    for(size_t i = 0; i < 20 && counter < 2; i++)
        el->run(el, 50);
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
    UA_EventLoop *el = UA_EventLoop_new_GLib(UA_Log_Stdout, NULL);
    UA_InterruptManager *im = UA_InterruptManager_new_GLib(UA_STRING("im1"));
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

START_TEST(multipleInterruptManagers) {
    counter = 0;

    /* Independent GMainContexts so el1->run()/el2->run() only ever dispatch
     * their own EventLoop's sources. */
    GMainContext *ctx1 = g_main_context_new();
    GMainContext *ctx2 = g_main_context_new();

    UA_EventLoop *el1 = UA_EventLoop_new_GLib(UA_Log_Stdout, ctx1);
    UA_EventLoop *el2 = UA_EventLoop_new_GLib(UA_Log_Stdout, ctx2);
    ck_assert_ptr_ne(el1, NULL);
    ck_assert_ptr_ne(el2, NULL);

    UA_InterruptManager *im1 = UA_InterruptManager_new_GLib(UA_STRING("im1"));
    UA_InterruptManager *im2 = UA_InterruptManager_new_GLib(UA_STRING("im2"));
    ck_assert_ptr_ne(im1, NULL);
    ck_assert_ptr_ne(im2, NULL);

    el1->registerEventSource(el1, &im1->eventSource);
    el2->registerEventSource(el2, &im2->eventSource);

    /* Different signals -- see the file header comment on why the same
     * signal cannot be fanned out to two independent GLib watchers. */
    UA_StatusCode res1 =
        im1->registerInterrupt(im1, TESTSIG, &UA_KEYVALUEMAP_NULL, interruptCallback, NULL);
    UA_StatusCode res2 =
        im2->registerInterrupt(im2, TESTSIG2, &UA_KEYVALUEMAP_NULL, interruptCallback, NULL);
    ck_assert_uint_eq(res1, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(res2, UA_STATUSCODE_GOOD);

    el1->start(el1);
    el2->start(el2);

    raise(TESTSIG);
    raise(TESTSIG2);

    for(size_t i = 0; i < 20 && counter < 2; i++) {
        el1->run(el1, 50);
        el2->run(el2, 50);
    }
    ck_assert_uint_eq(counter, 2);

    el2->stop(el2);
    while(el2->state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_DateTime next = el2->run(el2, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    el2->free(el2);

    el1->stop(el1);
    while(el1->state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_DateTime next = el1->run(el1, 1);
        UA_fakeSleep((UA_UInt32)((next - UA_DateTime_now()) / UA_DATETIME_MSEC));
    }
    el1->free(el1);

    g_main_context_unref(ctx1);
    g_main_context_unref(ctx2);
} END_TEST

int main(void) {
    Suite *s  = suite_create("Test EventLoop Interrupts (GLib)");
    TCase *tc = tcase_create("test cases");
    tcase_add_test(tc, catchInterrupt);
    tcase_add_test(tc, registerDuplicate);
    tcase_add_test(tc, multipleInterruptManagers);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all (sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
