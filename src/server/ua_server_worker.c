/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2016 (c) Sten Grüner
 *    Copyright 2015 (c) Chris Iatrou
 *    Copyright 2015 (c) Nick Goossens
 *    Copyright 2015 (c) Jörg Schüler-Maroldt
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016-2017 (c) Florian Palm
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) Lorenz Haas
 *    Copyright 2017 (c) Jonas Green
 */

#include "ua_util.h"
#include "ua_server_internal.h"

#ifdef UA_ENABLE_VALGRIND_INTERACTIVE
#include <valgrind/memcheck.h>
#endif

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
    UA_Server *server;
    pthread_t thr;
    UA_UInt32 counter;
    volatile UA_Boolean running;

    /* separate cache lines */
    char padding[64 - sizeof(void*) - sizeof(pthread_t) -
                 sizeof(UA_UInt32) - sizeof(UA_Boolean)];
};

struct UA_WorkerCallback {
    SIMPLEQ_ENTRY(UA_WorkerCallback) next;
    UA_ServerCallback callback;
    void *data;

    UA_Boolean delayed;         /* Is it a delayed callback? */
    UA_Boolean countersSampled; /* Have the worker counters been sampled? */
    UA_UInt32 workerCounters[]; /* Counter value for each worker */
};
typedef struct UA_WorkerCallback WorkerCallback;

/* Forward Declaration */
static void
processDelayedCallback(UA_Server *server, WorkerCallback *dc);

static void *
workerLoop(UA_Worker *worker) {
    UA_Server *server = worker->server;
    UA_UInt32 *counter = &worker->counter;
    volatile UA_Boolean *running = &worker->running;

    /* Initialize the (thread local) random seed with the ram address
     * of the worker. Not for security-critical entropy! */
    UA_random_seed((uintptr_t)worker);

    while(*running) {
        UA_atomic_addUInt32(counter, 1);
        pthread_mutex_lock(&server->dispatchQueue_accessMutex);
        WorkerCallback *dc = SIMPLEQ_FIRST(&server->dispatchQueue);
        if(dc) {
            SIMPLEQ_REMOVE_HEAD(&server->dispatchQueue, next);
        }
        pthread_mutex_unlock(&server->dispatchQueue_accessMutex);
        if(!dc) {
            /* Nothing to do. Sleep until a callback is dispatched */
            pthread_mutex_lock(&server->dispatchQueue_conditionMutex);
            pthread_cond_wait(&server->dispatchQueue_condition,
                              &server->dispatchQueue_conditionMutex);
            pthread_mutex_unlock(&server->dispatchQueue_conditionMutex);
            continue;
        }

        if(dc->delayed) {
            processDelayedCallback(server, dc);
            continue;
        }

        dc->callback(server, dc->data);
        UA_free(dc);
    }

    UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                 "Worker shut down");
    return NULL;
}

void UA_Server_cleanupDispatchQueue(UA_Server *server) {
    while(true) {
        pthread_mutex_lock(&server->dispatchQueue_accessMutex);
        WorkerCallback *dc = SIMPLEQ_FIRST(&server->dispatchQueue);
        if(!dc) {
            pthread_mutex_unlock(&server->dispatchQueue_accessMutex);
            break;
        }
        SIMPLEQ_REMOVE_HEAD(&server->dispatchQueue, next);
        pthread_mutex_unlock(&server->dispatchQueue_accessMutex);
        dc->callback(server, dc->data);
        UA_free(dc);
    }
}

#endif

/**
 * Repeated Callbacks
 * ------------------
 * Repeated Callbacks are handled by UA_Timer (used in both client and server).
 * In the multi-threaded case, callbacks are dispatched to workers. Otherwise,
 * they are executed immediately. */

void
UA_Server_workerCallback(UA_Server *server, UA_ServerCallback callback,
                         void *data) {
#ifndef UA_ENABLE_MULTITHREADING
    /* Execute immediately */
    callback(server, data);
#else
    /* Execute immediately if memory could not be allocated */
    WorkerCallback *dc = (WorkerCallback*)UA_malloc(sizeof(WorkerCallback));
    if(!dc) {
        callback(server, data);
        return;
    }

    /* Enqueue for the worker threads */
    dc->callback = callback;
    dc->data = data;
    dc->delayed = false;
    pthread_mutex_lock(&server->dispatchQueue_accessMutex);
    SIMPLEQ_INSERT_TAIL(&server->dispatchQueue, dc, next);
    pthread_mutex_unlock(&server->dispatchQueue_accessMutex);

    /* Wake up sleeping workers */
    pthread_cond_broadcast(&server->dispatchQueue_condition);
#endif
}

/**
 * Delayed Callbacks
 * -----------------
 *
 * Delayed Callbacks are called only when all callbacks that were dispatched
 * prior are finished. In the single-threaded case, the callback is added to a
 * singly-linked list that is processed at the end of the server's main-loop. In
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

/* Delayed callback to free the subscription memory */
static void
freeCallback(UA_Server *server, void *data) {
    UA_free(data);
}

/* TODO: Delayed free should never fail. This can be achieved by adding a prefix
 * with the list pointers */
UA_StatusCode
UA_Server_delayedFree(UA_Server *server, void *data) {
    return UA_Server_delayedCallback(server, freeCallback, data);
}

#ifndef UA_ENABLE_MULTITHREADING

typedef struct UA_DelayedCallback {
    SLIST_ENTRY(UA_DelayedCallback) next;
    UA_ServerCallback callback;
    void *data;
} UA_DelayedCallback;

UA_StatusCode
UA_Server_delayedCallback(UA_Server *server, UA_ServerCallback callback,
                          void *data) {
    UA_DelayedCallback *dc =
        (UA_DelayedCallback*)UA_malloc(sizeof(UA_DelayedCallback));
    if(!dc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dc->callback = callback;
    dc->data = data;
    SLIST_INSERT_HEAD(&server->delayedCallbacks, dc, next);
    return UA_STATUSCODE_GOOD;
}

void UA_Server_cleanupDelayedCallbacks(UA_Server *server) {
    UA_DelayedCallback *dc, *dc_tmp;
    SLIST_FOREACH_SAFE(dc, &server->delayedCallbacks, next, dc_tmp) {
        SLIST_REMOVE(&server->delayedCallbacks, dc, UA_DelayedCallback, next);
        dc->callback(server, dc->data);
        UA_free(dc);
    }
}

#else /* UA_ENABLE_MULTITHREADING */

UA_StatusCode
UA_Server_delayedCallback(UA_Server *server, UA_ServerCallback callback,
                          void *data) {
    size_t dcsize = sizeof(WorkerCallback) +
        (sizeof(UA_UInt32) * server->config.nThreads);
    WorkerCallback *dc = (WorkerCallback*)UA_malloc(dcsize);
    if(!dc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Enqueue for the worker threads */
    dc->callback = callback;
    dc->data = data;
    dc->delayed = true;
    dc->countersSampled = false;
    pthread_mutex_lock(&server->dispatchQueue_accessMutex);
    SIMPLEQ_INSERT_TAIL(&server->dispatchQueue, dc, next);
    pthread_mutex_unlock(&server->dispatchQueue_accessMutex);

    /* Wake up sleeping workers */
    pthread_cond_broadcast(&server->dispatchQueue_condition);
    return UA_STATUSCODE_GOOD;
}

/* Called from the worker loop */
static void
processDelayedCallback(UA_Server *server, WorkerCallback *dc) {
    /* Set the worker counters */
    if(!dc->countersSampled) {
        for(size_t i = 0; i < server->config.nThreads; ++i)
            dc->workerCounters[i] = server->workers[i].counter;
        dc->countersSampled = true;

        /* Re-add to the dispatch queue */
        pthread_mutex_lock(&server->dispatchQueue_accessMutex);
        SIMPLEQ_INSERT_TAIL(&server->dispatchQueue, dc, next);
        pthread_mutex_unlock(&server->dispatchQueue_accessMutex);

        /* Wake up sleeping workers */
        pthread_cond_broadcast(&server->dispatchQueue_condition);
        return;
    }

    /* Have all other jobs finished? */
    UA_Boolean ready = true;
    for(size_t i = 0; i < server->config.nThreads; ++i) {
        if(dc->workerCounters[i] == server->workers[i].counter) {
            ready = false;
            break;
        }
    }

    /* Re-add to the dispatch queue.
     * TODO: What is the impact of this loop?
     * Can we add a small delay here? */
    if(!ready) {
        pthread_mutex_lock(&server->dispatchQueue_accessMutex);
        SIMPLEQ_INSERT_TAIL(&server->dispatchQueue, dc, next);
        pthread_mutex_unlock(&server->dispatchQueue_accessMutex);

        /* Wake up sleeping workers */
        pthread_cond_broadcast(&server->dispatchQueue_condition);
        return;
    }

    /* Execute the callback */
    dc->callback(server, dc->data);
    UA_free(dc);
}

#endif

/**
 * Main Server Loop
 * ----------------
 * Start: Spin up the workers and the network layer and sample the server's
 *        start time.
 * Iterate: Process repeated callbacks and events in the network layer.
 *          This part can be driven from an external main-loop in an
 *          event-driven single-threaded architecture.
 * Stop: Stop workers, finish all callbacks, stop the network layer,
 *       clean up */

UA_StatusCode
UA_Server_run_startup(UA_Server *server) {
    UA_Variant var;
    UA_StatusCode result = UA_STATUSCODE_GOOD;

    /* Sample the start time and set it to the Server object */
    server->startTime = UA_DateTime_now();
    UA_Variant_init(&var);
    UA_Variant_setScalar(&var, &server->startTime, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Server_writeValue(server,
                         UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME),
                         var);

    /* Start the networklayers */
    for(size_t i = 0; i < server->config.networkLayersSize; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        result |= nl->start(nl, &server->config.customHostname);
    }

    /* Spin up the worker threads */
#ifdef UA_ENABLE_MULTITHREADING
    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                "Spinning up %u worker thread(s)", server->config.nThreads);
    pthread_mutex_init(&server->dispatchQueue_accessMutex, NULL);
    pthread_cond_init(&server->dispatchQueue_condition, NULL);
    pthread_mutex_init(&server->dispatchQueue_conditionMutex, NULL);
    server->workers = (UA_Worker*)UA_malloc(server->config.nThreads * sizeof(UA_Worker));
    if(!server->workers)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < server->config.nThreads; ++i) {
        UA_Worker *worker = &server->workers[i];
        worker->server = server;
        worker->counter = 0;
        worker->running = true;
        pthread_create(&worker->thr, NULL, (void* (*)(void*))workerLoop, worker);
    }
#endif

    /* Start the multicast discovery server */
#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if(server->config.applicationDescription.applicationType ==
       UA_APPLICATIONTYPE_DISCOVERYSERVER)
        startMulticastDiscoveryServer(server);
#endif

    return result;
}

UA_UInt16
UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal) {
    /* Process repeated work */
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime nextRepeated =
        UA_Timer_process(&server->timer, now,
                         (UA_TimerDispatchCallback)UA_Server_workerCallback,
                         server);
    UA_DateTime latest = now + (UA_MAXTIMEOUT * UA_DATETIME_MSEC);
    if(nextRepeated > latest)
        nextRepeated = latest;

    UA_UInt16 timeout = 0;

    /* round always to upper value to avoid timeout to be set to 0
    * if(nextRepeated - now) < (UA_DATETIME_MSEC/2) */
    if(waitInternal)
        timeout = (UA_UInt16)(((nextRepeated - now) + (UA_DATETIME_MSEC - 1)) / UA_DATETIME_MSEC);

    /* Listen on the networklayer */
    for(size_t i = 0; i < server->config.networkLayersSize; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        nl->listen(nl, server, timeout);
    }

#ifndef UA_ENABLE_MULTITHREADING
    /* Process delayed callbacks when all callbacks and network events are done.
     * If multithreading is enabled, the cleanup of delayed values is attempted
     * by a callback in the job queue. */
    UA_Server_cleanupDelayedCallbacks(server);
#endif

#if defined(UA_ENABLE_DISCOVERY_MULTICAST) && !defined(UA_ENABLE_MULTITHREADING)
    if(server->config.applicationDescription.applicationType ==
       UA_APPLICATIONTYPE_DISCOVERYSERVER) {
        // TODO multicastNextRepeat does not consider new input data (requests)
        // on the socket. It will be handled on the next call. if needed, we
        // need to use select with timeout on the multicast socket
        // server->mdnsSocket (see example in mdnsd library) on higher level.
        UA_DateTime multicastNextRepeat = 0;
        UA_StatusCode hasNext =
            iterateMulticastDiscoveryServer(server, &multicastNextRepeat, UA_TRUE);
        if(hasNext == UA_STATUSCODE_GOOD && multicastNextRepeat < nextRepeated)
            nextRepeated = multicastNextRepeat;
    }
#endif

    now = UA_DateTime_nowMonotonic();
    timeout = 0;
    if(nextRepeated > now)
        timeout = (UA_UInt16)((nextRepeated - now) / UA_DATETIME_MSEC);
    return timeout;
}

UA_StatusCode
UA_Server_run_shutdown(UA_Server *server) {
    /* Stop the netowrk layer */
    for(size_t i = 0; i < server->config.networkLayersSize; ++i) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        nl->stop(nl, server);
    }

#ifdef UA_ENABLE_MULTITHREADING
    /* Shut down the workers */
    if(server->workers) {
        UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Shutting down %u worker thread(s)",
                    server->config.nThreads);
        for(size_t i = 0; i < server->config.nThreads; ++i)
            server->workers[i].running = false;
        pthread_cond_broadcast(&server->dispatchQueue_condition);
        for(size_t i = 0; i < server->config.nThreads; ++i)
            pthread_join(server->workers[i].thr, NULL);
        UA_free(server->workers);
        server->workers = NULL;
    }

    /* Execute the remaining callbacks in the dispatch queue. Also executes
     * delayed callbacks. */
    UA_Server_cleanupDispatchQueue(server);
#else
    /* Process remaining delayed callbacks */
    UA_Server_cleanupDelayedCallbacks(server);
#endif

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    /* Stop multicast discovery */
    if(server->config.applicationDescription.applicationType ==
       UA_APPLICATIONTYPE_DISCOVERYSERVER)
        stopMulticastDiscoveryServer(server);
#endif

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_run(UA_Server *server, volatile UA_Boolean *running) {
    UA_StatusCode retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
#ifdef UA_ENABLE_VALGRIND_INTERACTIVE
    size_t loopCount = 0;
#endif
    while(*running) {
#ifdef UA_ENABLE_VALGRIND_INTERACTIVE
        if(loopCount == 0) {
            VALGRIND_DO_LEAK_CHECK;
        }
        ++loopCount;
        loopCount %= UA_VALGRIND_INTERACTIVE_INTERVAL;
#endif
        UA_Server_run_iterate(server, true);
    }
    return UA_Server_run_shutdown(server);
}
