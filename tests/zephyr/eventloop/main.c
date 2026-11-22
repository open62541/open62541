#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <open62541/plugin/eventloop.h>
#include <open62541/plugin/log_stdout.h>

/* Queue two wakeups from inside run(), before it enters select(). */
static void
cancelTwice(void *application, void *context) {
    UA_EventLoop *el = (UA_EventLoop *)application;
    el->cancel(el);
    el->cancel(el);
}

ZTEST(eventloop, test_wakeup_is_consumed) {
    UA_EventLoop *el = UA_EventLoop_new_Zephyr(UA_Log_Stdout);
    zassert_not_null(el);
    zassert_equal(el->start(el), UA_STATUSCODE_GOOD);

    UA_DelayedCallback dc = {0};
    dc.callback = cancelTwice;
    dc.application = el;
    el->addDelayedCallback(el, &dc);

    int64_t start = k_uptime_get();
    zassert_equal(el->run(el, 1000), UA_STATUSCODE_GOOD);
    zassert_true(k_uptime_get() - start < 500);

    /* No descriptors are active. A stale eventfd counter would return at once. */
    start = k_uptime_get();
    zassert_equal(el->run(el, 100), UA_STATUSCODE_GOOD);
    zassert_true(k_uptime_get() - start >= 90);

    el->stop(el);
    zassert_equal(el->free(el), UA_STATUSCODE_GOOD);
}

ZTEST_SUITE(eventloop, NULL, NULL, NULL, NULL, NULL);
