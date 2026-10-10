/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>
#include <lwip/netif.h>
#include <pthread.h>
#include <check.h>
#include <stdlib.h>

static UA_EventLoop *
newLoop(UA_EventLoopConfiguration *config) {
    UA_EventLoop *el = UA_EventLoop_new_LWIP(UA_Log_Stdout, config);
    ck_assert_ptr_nonnull(el);
    return el;
}

static void
stopAndFree(UA_EventLoop *el) {
    el->stop(el);
    for(unsigned i = 0; i < 100 && el->state == UA_EVENTLOOPSTATE_STOPPING; i++)
        ck_assert_uint_eq(el->run(el, 0), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);
}

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    unsigned ready;
    UA_Boolean go;
} StartGate;

typedef struct {
    StartGate *gate;
    UA_EventLoop *el;
    UA_StatusCode result;
} StartContext;

static void *
startLoop(void *data) {
    StartContext *ctx = (StartContext*)data;
    pthread_mutex_lock(&ctx->gate->mutex);
    ctx->gate->ready++;
    pthread_cond_broadcast(&ctx->gate->condition);
    while(!ctx->gate->go)
        pthread_cond_wait(&ctx->gate->condition, &ctx->gate->mutex);
    pthread_mutex_unlock(&ctx->gate->mutex);
    ctx->result = ctx->el->start(ctx->el);
    return NULL;
}

/* Run first in this process, so both starts compete to initialize lwIP. Each
 * loop's self-pipe also exercises socket creation through the shared stack. */
START_TEST(concurrentFirstStart) {
    StartGate gate = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, false};
    StartContext contexts[2] = {{&gate, newLoop(NULL), UA_STATUSCODE_BADINTERNALERROR},
                                {&gate, newLoop(NULL), UA_STATUSCODE_BADINTERNALERROR}};
    pthread_t threads[2];
    for(unsigned i = 0; i < 2; i++)
        ck_assert_int_eq(pthread_create(&threads[i], NULL, startLoop, &contexts[i]), 0);
    pthread_mutex_lock(&gate.mutex);
    while(gate.ready < 2)
        pthread_cond_wait(&gate.condition, &gate.mutex);
    gate.go = true;
    pthread_cond_broadcast(&gate.condition);
    pthread_mutex_unlock(&gate.mutex);
    for(unsigned i = 0; i < 2; i++) {
        ck_assert_int_eq(pthread_join(threads[i], NULL), 0);
        ck_assert_uint_eq(contexts[i].result, UA_STATUSCODE_GOOD);
    }
    pthread_cond_destroy(&gate.condition);
    pthread_mutex_destroy(&gate.mutex);

    struct netif *interface = netif_default;
    ck_assert_ptr_nonnull(interface);
    stopAndFree(contexts[0].el);
    /* ASan detects a dangling interface here with the old per-loop storage. */
    ck_assert_ptr_eq(netif_default, interface);
    ck_assert(netif_is_up(interface));
    ck_assert_uint_eq(contexts[1].el->run(contexts[1].el, 0), UA_STATUSCODE_GOOD);
    stopAndFree(contexts[1].el);
    ck_assert(netif_is_up(interface));
} END_TEST

START_TEST(sequentialLoopsAndRestart) {
    struct netif *interface = NULL;
    for(unsigned i = 0; i < 3; i++) {
        UA_EventLoop *el = newLoop(NULL);
        ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
        if(!interface)
            interface = netif_default;
        ck_assert_ptr_eq(netif_default, interface);
        ck_assert(netif_is_up(interface));
        el->stop(el);
        ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);
        ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
        ck_assert_uint_eq(el->run(el, 0), UA_STATUSCODE_GOOD);
        stopAndFree(el);
        ck_assert(netif_is_up(interface));
    }
} END_TEST

static UA_EventLoop *
newLoopWithAddress(const char *address) {
    UA_EventLoopConfiguration config = {0};
    UA_String value = UA_STRING((char*)(uintptr_t)address);
    ck_assert_uint_eq(UA_KeyValueMap_setScalar(&config.params,
                      UA_QUALIFIEDNAME(0, "ipaddr"), &value, &UA_TYPES[UA_TYPES_STRING]),
                      UA_STATUSCODE_GOOD);
    return newLoop(&config);
}

START_TEST(conflictingAndInvalidConfiguration) {
    /* Initialize the default interface even when this test is selected alone. */
    UA_EventLoop *el = newLoop(NULL);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    stopAndFree(el);

    el = newLoopWithAddress("192.168.0.201");
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_BADCONFIGURATIONERROR);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_FRESH);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);

    el = newLoopWithAddress("192.168.0.200xxx");
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_BADINVALIDARGUMENT);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);

    /* Explicit matching settings and omitted settings both reuse the interface. */
    el = newLoopWithAddress("192.168.0.200");
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    stopAndFree(el);
} END_TEST

static unsigned initCalls;
static unsigned shutdownCalls;
static UA_StatusCode initResult;

static UA_StatusCode
customInit(UA_EventLoop *el, const UA_String *ipaddr,
           const UA_String *netmask, const UA_String *gateway) {
    initCalls++;
    return initResult;
}

static void
customShutdown(UA_EventLoop *el) {
    shutdownCalls++;
}

START_TEST(customInterfaceLifecycle) {
    /* The custom callback uses the already initialized process interface. */
    UA_EventLoop *bootstrap = newLoop(NULL);
    ck_assert_uint_eq(bootstrap->start(bootstrap), UA_STATUSCODE_GOOD);
    stopAndFree(bootstrap);

    initCalls = shutdownCalls = 0;
    UA_EventLoopConfiguration config = {0};
    config.netifInit = customInit;
    config.netifShutdown = customShutdown;
    UA_EventLoop *el = newLoop(&config);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(initCalls, 0);
    ck_assert_uint_eq(shutdownCalls, 0);

    config.netifInit = customInit;
    config.netifShutdown = customShutdown;
    el = newLoop(&config);
    initResult = UA_STATUSCODE_BADINTERNALERROR;
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_BADINTERNALERROR);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(initCalls, 1);
    ck_assert_uint_eq(shutdownCalls, 0);

    config.netifInit = customInit;
    config.netifShutdown = customShutdown;
    el = newLoop(&config);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_BADINTERNALERROR);
    initResult = UA_STATUSCODE_GOOD;
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    el->stop(el);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);
    ck_assert_uint_eq(shutdownCalls, 0);
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(initCalls, 3);
    stopAndFree(el);
    ck_assert_uint_eq(shutdownCalls, 1);
} END_TEST

/* Invoke lifecycle methods through their public function pointers. In addition
 * to checking ownership, UBSan verifies the callback types at these calls. */
START_TEST(publicCallbackLifecycle) {
    UA_EventLoop *el = newLoop(NULL);
    UA_ConnectionManager *tcp = UA_ConnectionManager_new_LWIP_TCP(UA_STRING("tcp"));
    UA_ConnectionManager *udp = UA_ConnectionManager_new_LWIP_UDP(UA_STRING("udp"));
    ck_assert_ptr_nonnull(tcp);
    ck_assert_ptr_nonnull(udp);
    UA_EventSource *sources[] = {&tcp->eventSource, &udp->eventSource};
    for(unsigned i = 0; i < 2; i++) {
        ck_assert_uint_eq(el->registerEventSource(el, sources[i]), UA_STATUSCODE_GOOD);
        ck_assert_ptr_eq(sources[i]->eventLoop, el);
        ck_assert_int_eq(sources[i]->state, UA_EVENTSOURCESTATE_STOPPED);
    }
    ck_assert_uint_eq(el->start(el), UA_STATUSCODE_GOOD);
    for(unsigned i = 0; i < 2; i++)
        ck_assert_int_eq(sources[i]->state, UA_EVENTSOURCESTATE_STARTED);
    el->cancel(el);
    ck_assert_uint_eq(el->run(el, 0), UA_STATUSCODE_GOOD);
    el->stop(el);
    ck_assert_int_eq(el->state, UA_EVENTLOOPSTATE_STOPPED);

    /* Explicit deregistration and deletion for TCP; loop-owned deletion for
     * UDP exercises the other ownership path. */
    ck_assert_uint_eq(el->deregisterEventSource(el, sources[0]), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(sources[0]->state, UA_EVENTSOURCESTATE_FRESH);
    ck_assert_ptr_eq(el->eventSources, sources[1]);
    ck_assert_uint_eq(sources[0]->free(sources[0]), UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(el->free(el), UA_STATUSCODE_GOOD);
} END_TEST

int main(void) {
    Suite *suite = suite_create("lwIP EventLoop lifecycle");
    TCase *tc = tcase_create("lifecycle");
    tcase_set_timeout(tc, 60);
    tcase_add_test(tc, concurrentFirstStart);
    tcase_add_test(tc, sequentialLoopsAndRestart);
    tcase_add_test(tc, conflictingAndInvalidConfiguration);
    tcase_add_test(tc, customInterfaceLifecycle);
    tcase_add_test(tc, publicCallbackLifecycle);
    suite_add_tcase(suite, tc);
    SRunner *runner = srunner_create(suite);
    /* Exercise successive loop lifetimes within the same lwIP process. */
    srunner_set_fork_status(runner, CK_NOFORK);
    srunner_run_all(runner, CK_NORMAL);
    int failed = srunner_ntests_failed(runner);
    srunner_free(runner);
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
