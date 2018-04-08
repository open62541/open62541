/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
#include "ua_client.h"
#include "ua_client_internal.h"
#define UA_MAXTIMEOUT 50 /* Max timeout in ms between main-loop iterations */

/**
 * Worker Threads and Dispatch Queue
 * ---------------------------------
 * The worker threads dequeue callbacks from a central Multi-Producer
 * Multi-Consumer Queue (MPMC). When there are no callbacks, workers go idle.
 * The condition to wake them up is triggered whenever a callback is
 * dispatched.
 *
 * Future Plans: Use work-stealing to load-balance between cores.
 * Le, Nhat Minh, et al. "Correct and efficient work-stealing for weak memory
 * models." ACM SIGPLAN Notices. Vol. 48. No. 8. ACM, 2013. */

/**
 * Repeated Callbacks
 * ------------------
 * Repeated Callbacks are handled by UA_Timer (used in both client and client).
 * In the multi-threaded case, callbacks are dispatched to workers. Otherwise,
 * they are executed immediately. */

void UA_Client_workerCallback(UA_Client *client, UA_ClientCallback callback,
        void *data) {
    /* Execute immediately */
    callback(client, data);
}

/**
 * Delayed Callbacks
 * -----------------
 *
 * Delayed Callbacks are called only when all callbacks that were dispatched
 * prior are finished. In the single-threaded case, the callback is added to a
 * singly-linked list that is processed at the end of the client's main-loop. In
 * the multi-threaded case, the delay is ensure by a three-step procedure:
 *
 * 1. The delayed callback is dispatched to the worker queue. So it is only
 *    dequeued when all prior callbacks have been dequeued.
 *
 * 2. When the callback is first dequeued by a worker, sample the counter of all
 *    workers. Once all counters have advanced, the callback is ready.
 *
 * 3. Check regularly if the callback is ready by adding it back to the dispatch
 *    queue. */

typedef struct UA_DelayedClientCallback {
    SLIST_ENTRY(UA_DelayedClientCallback)
    next;
    UA_ClientCallback callback;
    void *data;
} UA_DelayedClientCallback;

UA_StatusCode UA_Client_delayedCallback(UA_Client *client,
        UA_ClientCallback callback, void *data) {
    UA_DelayedClientCallback *dc = (UA_DelayedClientCallback*) UA_malloc(
            sizeof(UA_DelayedClientCallback));
    if (!dc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dc->callback = callback;
    dc->data = data;
    SLIST_INSERT_HEAD(&client->delayedClientCallbacks, dc, next);
    return UA_STATUSCODE_GOOD;
}

void
processDelayedClientCallbacks(UA_Client *client);

void processDelayedClientCallbacks(UA_Client *client) {
    UA_DelayedClientCallback *dc, *dc_tmp;
    SLIST_FOREACH_SAFE(dc, &client->delayedClientCallbacks, next, dc_tmp)
    {
        SLIST_REMOVE(&client->delayedClientCallbacks, dc,
                UA_DelayedClientCallback, next);
        dc->callback(client, dc->data);
        UA_free(dc);
    }
}

/**
 * Main Client Loop
 * ----------------
 * Start: Spin up the workers and the network layer
 * Iterate: Process repeated callbacks and events in the network layer.
 *          This part can be driven from an external main-loop in an
 *          event-driven single-threaded architecture.
 * Stop: Stop workers, finish all callbacks, stop the network layer,
 *       clean up */

UA_StatusCode UA_Client_run_iterate(UA_Client *client, UA_Boolean *timedOut) {
    UA_StatusCode retval;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_StatusCode retvalPublish = UA_Client_Subscriptions_backgroundPublish(client);
    if (client->state >= UA_CLIENTSTATE_SESSION && retvalPublish != UA_STATUSCODE_GOOD)
        return retvalPublish;
#endif
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_Timer_process(&client->timer, now,
            (UA_TimerDispatchCallback) UA_Client_workerCallback, client);

    if(*timedOut)
        return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

    if (client->config.stateCallback)
        client->config.stateCallback(client, client->state);

    UA_ClientState cs = UA_Client_getState(client);
    retval = UA_Client_connect_iterate(client);

    /*connection failed, drop the rest*/
    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    if (cs == UA_CLIENTSTATE_SECURECHANNEL || cs == UA_CLIENTSTATE_SESSION) {
#ifdef UA_ENABLE_SUBSCRIPTIONS
        if (client->state >= UA_CLIENTSTATE_SESSION) {
            retvalPublish =
                    UA_Client_Subscriptions_backgroundPublish(client);
            if (retvalPublish != UA_STATUSCODE_GOOD)
                return retvalPublish;
        }
#endif
        /* check for new data */
        retval = receiveServiceResponse_async(client, NULL, NULL);

#ifdef UA_ENABLE_SUBSCRIPTIONS
        /* The inactivity check must be done after receiveServiceResponse*/
        UA_Client_Subscriptions_backgroundPublishInactivityCheck(client);
#endif

    } else
        retval=receivePacket_async(client);

#ifndef UA_ENABLE_MULTITHREADING
    /* Process delayed callbacks when all callbacks and
     * network events are done */
    processDelayedClientCallbacks(client);
#endif
    return retval;
}

