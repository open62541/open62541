/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
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

#ifdef UA_ENABLE_MULTITHREADING

struct UA_Worker {
    UA_Client *client;
    pthread_t thr;
    UA_UInt32 counter;
    volatile UA_Boolean running;

    /* separate cache lines */
    char padding[64 - sizeof(void*) - sizeof(pthread_t) -
                 sizeof(UA_UInt32) - sizeof(UA_Boolean)];
};

typedef struct {
    struct cds_wfcq_node node;
    UA_ClientCallback callback;
    void *data;

    UA_Boolean delayed;         /* Is it a delayed callback? */
    UA_Boolean countersSampled; /* Have the worker counters been sampled? */
    UA_UInt32 workerCounters[]; /* Counter value for each worker */
} WorkerCallback;

/* Forward Declaration */
static void
processDelayedCallback(UA_Client *client, WorkerCallback *dc);

static void *
workerLoop(UA_Worker *worker) {
    UA_Client *client = worker->client;
    UA_UInt32 *counter = &worker->counter;
    volatile UA_Boolean *running = &worker->running;

    /* Initialize the (thread local) random seed with the ram address
     * of the worker. Not for security-critical entropy! */
    UA_random_seed((uintptr_t)worker);
    rcu_register_thread();

    while(*running) {
        UA_atomic_add(counter, 1);
        WorkerCallback *dc = (WorkerCallback*)
            cds_wfcq_dequeue_blocking(&client->dispatchQueue_head,
                                      &client->dispatchQueue_tail);
        if(!dc) {
            /* Nothing to do. Sleep until a callback is dispatched */
            pthread_mutex_lock(&client->dispatchQueue_mutex);
            pthread_cond_wait(&client->dispatchQueue_condition,
                              &client->dispatchQueue_mutex);
            pthread_mutex_unlock(&client->dispatchQueue_mutex);
            continue;
        }

        if(dc->delayed) {
            processDelayedCallback(client, dc);
            continue;
        }

        UA_RCU_LOCK();
        dc->callback(client, dc->data);
        UA_free(dc);
        UA_RCU_UNLOCK();
    }

    UA_ASSERT_RCU_UNLOCKED();
    rcu_barrier();
    rcu_unregister_thread();
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SERVER,
                 "Worker shut down");
    return NULL;
}

static void
emptyDispatchQueue(UA_Client *client) {
    while(!cds_wfcq_empty(&client->dispatchQueue_head,
                          &client->dispatchQueue_tail)) {
        WorkerCallback *dc = (WorkerCallback*)
            cds_wfcq_dequeue_blocking(&client->dispatchQueue_head,
                                      &client->dispatchQueue_tail);
        dc->callback(client, dc->data);
        UA_free(dc);
    }
}

#endif

/**
 * Repeated Callbacks
 * ------------------
 * Repeated Callbacks are handled by UA_Timer (used in both client and client).
 * In the multi-threaded case, callbacks are dispatched to workers. Otherwise,
 * they are executed immediately. */

void
UA_Client_workerCallback(UA_Client *client, UA_ClientCallback callback,
                         void *data) {
#ifndef UA_ENABLE_MULTITHREADING
    /* Execute immediately */
    callback(client, data);
#else
    /* Execute immediately if memory could not be allocated */
    WorkerCallback *dc = UA_malloc(sizeof(WorkerCallback));
    if(!dc) {
        callback(client, data);
        return;
    }

    /* Enqueue for the worker threads */
    dc->callback = callback;
    dc->data = data;
    dc->delayed = false;
    cds_wfcq_node_init(&dc->node);
    cds_wfcq_enqueue(&client->dispatchQueue_head,
                     &client->dispatchQueue_tail, &dc->node);

    /* Wake up sleeping workers */
    pthread_cond_broadcast(&client->dispatchQueue_condition);
#endif
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

#ifndef UA_ENABLE_MULTITHREADING

typedef struct UA_DelayedCallback {
    SLIST_ENTRY(UA_DelayedCallback) next;
    UA_ClientCallback callback;
    void *data;
} UA_DelayedCallback;

UA_StatusCode
UA_Client_delayedCallback(UA_Client *client, UA_ClientCallback callback,
                          void *data) {
    UA_DelayedCallback *dc =
        (UA_DelayedCallback*)UA_malloc(sizeof(UA_DelayedCallback));
    if(!dc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dc->callback = callback;
    dc->data = data;
    SLIST_INSERT_HEAD(&client->delayedCallbacks, dc, next);
    return UA_STATUSCODE_GOOD;
}

static void
processDelayedCallbacks(UA_Client *client) {
    UA_DelayedCallback *dc, *dc_tmp;
    SLIST_FOREACH_SAFE(dc, &client->delayedCallbacks, next, dc_tmp) {
        SLIST_REMOVE(&client->delayedCallbacks, dc, UA_DelayedCallback, next);
        dc->callback(client, dc->data);
        UA_free(dc);
    }
}

#else /* UA_ENABLE_MULTITHREADING */

UA_StatusCode
UA_Client_delayedCallback(UA_Client *client, UA_ClientCallback callback,
                          void *data) {
    size_t dcsize = sizeof(WorkerCallback) +
        (sizeof(UA_UInt32) * client->config.nThreads);
    WorkerCallback *dc = UA_malloc(dcsize);
    if(!dc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Enqueue for the worker threads */
    dc->callback = callback;
    dc->data = data;
    dc->delayed = true;
    dc->countersSampled = false;
    cds_wfcq_node_init(&dc->node);
    cds_wfcq_enqueue(&client->dispatchQueue_head,
                     &client->dispatchQueue_tail, &dc->node);

    /* Wake up sleeping workers */
    pthread_cond_broadcast(&client->dispatchQueue_condition);
    return UA_STATUSCODE_GOOD;
}

/* Called from the worker loop */
static void
processDelayedCallback(A_Client *client, WorkerCallback *dc) {
    /* Set the worker counters */
    if(!dc->countersSampled) {
        for(size_t i = 0; i < client->config.nThreads; ++i)
            dc->workerCounters[i] = client->workers[i].counter;
        dc->countersSampled = true;

        /* Re-add to the dispatch queue */
        cds_wfcq_node_init(&dc->node);
        cds_wfcq_enqueue(&client->dispatchQueue_head,
                         &client->dispatchQueue_tail, &dc->node);

        /* Wake up sleeping workers */
        pthread_cond_broadcast(&client->dispatchQueue_condition);
        return;
    }

    /* Have all other jobs finished? */
    UA_Boolean ready = true;
    for(size_t i = 0; i < client->config.nThreads; ++i) {
        if(dc->workerCounters[i] == client->workers[i].counter) {
            ready = false;
            break;
        }
    }

    /* Re-add to the dispatch queue.
     * TODO: What is the impact of this loop?
     * Can we add a small delay here? */
    if(!ready) {
        cds_wfcq_node_init(&dc->node);
        cds_wfcq_enqueue(&client->dispatchQueue_head,
                         &client->dispatchQueue_tail, &dc->node);

        /* Wake up sleeping workers */
        pthread_cond_broadcast(&client->dispatchQueue_condition);
        return;
    }

    /* Execute the callback */
    dc->callback(client, dc->data);
    UA_free(dc);
}

#endif

/**
 * Main Server Loop
 * ----------------
 * Start: Spin up the workers and the network layer
 * Iterate: Process repeated callbacks and events in the network layer.
 *          This part can be driven from an external main-loop in an
 *          event-driven single-threaded architecture.
 * Stop: Stop workers, finish all callbacks, stop the network layer,
 *       clean up */


UA_UInt16
UA_Client_run_iterate(UA_Client *client, UA_Boolean waitInternal) {
    /* Process repeated work */
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime nextRepeated =
        UA_Timer_process(&client->timer, now,
                         (UA_TimerDispatchCallback)UA_Client_workerCallback,
                         client);
    UA_DateTime latest = now + (UA_MAXTIMEOUT * UA_MSEC_TO_DATETIME);
    if(nextRepeated > latest)
        nextRepeated = latest;

    UA_UInt16 timeout = 0;
    if(waitInternal)
        timeout = (UA_UInt16)((nextRepeated - now) / UA_MSEC_TO_DATETIME);

    /* check for new data */
    receiveServiceResponse_async(client,NULL,NULL);


#ifndef UA_ENABLE_MULTITHREADING
    /* Process delayed callbacks when all callbacks and
     * network events are done */
    processDelayedCallbacks(client);
#endif

#if defined(UA_ENABLE_DISCOVERY_MULTICAST) && !defined(UA_ENABLE_MULTITHREADING)
    if(client->config.applicationDescription.applicationType ==
       UA_APPLICATIONTYPE_DISCOVERYSERVER) {
        // TODO multicastNextRepeat does not consider new input data (requests)
        // on the socket. It will be handled on the next call. if needed, we
        // need to use select with timeout on the multicast socket
        // client->mdnsSocket (see example in mdnsd library) on higher level.
        UA_DateTime multicastNextRepeat = 0;
        UA_Boolean hasNext =
            iterateMulticastDiscoveryServer(client, &multicastNextRepeat,
                                            UA_TRUE);
        if(hasNext && multicastNextRepeat < nextRepeated)
            nextRepeated = multicastNextRepeat;
    }
#endif

    now = UA_DateTime_nowMonotonic();
    timeout = 0;
    if(nextRepeated > now)
        timeout = (UA_UInt16)((nextRepeated - now) / UA_MSEC_TO_DATETIME);
    return timeout;
}

UA_StatusCode
UA_Client_run_shutdown(UA_Client *client) {
    /* Stop the netowrk layer */
    UA_Client_disconnect(client);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_run(UA_Client *client, volatile UA_Boolean *running) {

    while(*running)
        UA_Client_run_iterate(client, true);
    return UA_Client_run_shutdown(client);
}
