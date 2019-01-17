/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2014-2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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

#include "ua_workqueue.h"

void UA_WorkQueue_init(UA_WorkQueue *wq) {
    /* Initialized the linked list for delayed callbacks */
    SIMPLEQ_INIT(&wq->delayedCallbacks);

#ifdef UA_ENABLE_MULTITHREADING
    wq->delayedCallbacks_checkpoint = NULL;
    pthread_mutex_init(&wq->delayedCallbacks_accessMutex,  NULL);

    /* Initialize the dispatch queue for worker threads */
    SIMPLEQ_INIT(&wq->dispatchQueue);
    pthread_mutex_init(&wq->dispatchQueue_accessMutex, NULL);
    pthread_cond_init(&wq->dispatchQueue_condition, NULL);
    pthread_mutex_init(&wq->dispatchQueue_conditionMutex, NULL);
#endif
}

#ifdef UA_ENABLE_MULTITHREADING
/* Forward declaration */
static void UA_WorkQueue_manuallyProcessDelayed(UA_WorkQueue *wq);
#endif

void UA_WorkQueue_cleanup(UA_WorkQueue *wq) {
#ifdef UA_ENABLE_MULTITHREADING
    /* Shut down workers */
    UA_WorkQueue_stop(wq);

    /* Execute remaining work in the dispatch queue */
    while(true) {
        pthread_mutex_lock(&wq->dispatchQueue_accessMutex);
        UA_DelayedCallback *dc = SIMPLEQ_FIRST(&wq->dispatchQueue);
        if(!dc) {
            pthread_mutex_unlock(&wq->dispatchQueue_accessMutex);
            break;
        }
        SIMPLEQ_REMOVE_HEAD(&wq->dispatchQueue, next);
        pthread_mutex_unlock(&wq->dispatchQueue_accessMutex);
        dc->callback(dc->application, dc->data);
        UA_free(dc);
    }
#endif

    /* All workers are shut down. Execute remaining delayed work here. */
    UA_WorkQueue_manuallyProcessDelayed(wq);

#ifdef UA_ENABLE_MULTITHREADING
    wq->delayedCallbacks_checkpoint = NULL;
    pthread_mutex_destroy(&wq->dispatchQueue_accessMutex);
    pthread_cond_destroy(&wq->dispatchQueue_condition);
    pthread_mutex_destroy(&wq->dispatchQueue_conditionMutex);
    pthread_mutex_destroy(&wq->delayedCallbacks_accessMutex);
#endif
}

/***********/
/* Workers */
/***********/

#ifdef UA_ENABLE_MULTITHREADING

static void *
workerLoop(UA_Worker *worker) {
    UA_WorkQueue *wq = worker->queue;
    UA_UInt32 *counter = &worker->counter;
    volatile UA_Boolean *running = &worker->running;

    /* Initialize the (thread local) random seed with the ram address
     * of the worker. Not for security-critical entropy! */
    UA_random_seed((uintptr_t)worker);

    while(*running) {
        UA_atomic_addUInt32(counter, 1);

        /* Remove a callback from the queue */
        pthread_mutex_lock(&wq->dispatchQueue_accessMutex);
        UA_DelayedCallback *dc = SIMPLEQ_FIRST(&wq->dispatchQueue);
        if(dc)
            SIMPLEQ_REMOVE_HEAD(&wq->dispatchQueue, next);
        pthread_mutex_unlock(&wq->dispatchQueue_accessMutex);

        /* Nothing to do. Sleep until a callback is dispatched */
        if(!dc) {
            pthread_mutex_lock(&wq->dispatchQueue_conditionMutex);
            pthread_cond_wait(&wq->dispatchQueue_condition,
                              &wq->dispatchQueue_conditionMutex);
            pthread_mutex_unlock(&wq->dispatchQueue_conditionMutex);
            continue;
        }

        /* Execute */
        if(dc->callback)
            dc->callback(dc->application, dc->data);
        UA_free(dc);
    }

    return NULL;
}

/* Can be called repeatedly and starts additional workers */
UA_StatusCode
UA_WorkQueue_start(UA_WorkQueue *wq, size_t workersCount) {
    if(wq->workersSize > 0 || workersCount == 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    /* Create the worker array */
    wq->workers = (UA_Worker*)UA_calloc(workersCount, sizeof(UA_Worker));
    if(!wq->workers)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    wq->workersSize = workersCount;

    /* Spin up the workers */
    for(size_t i = 0; i < workersCount; ++i) {
        UA_Worker *w = &wq->workers[i];
        w->queue = wq;
        w->counter = 0;
        w->running = true;
        pthread_create(&w->thread, NULL, (void* (*)(void*))workerLoop, w);
    }
    return UA_STATUSCODE_GOOD;
}

void UA_WorkQueue_stop(UA_WorkQueue *wq) {
    if(wq->workersSize == 0)
        return;

    /* Signal the workers to stop */
    for(size_t i = 0; i < wq->workersSize; ++i)
        wq->workers[i].running = false;

    /* Wake up all workers */
    pthread_cond_broadcast(&wq->dispatchQueue_condition);

    /* Wait for the workers to finish, then clean up */
    for(size_t i = 0; i < wq->workersSize; ++i)
        pthread_join(wq->workers[i].thread, NULL);

    UA_free(wq->workers);
    wq->workers = NULL;
    wq->workersSize = 0;
}

void UA_WorkQueue_enqueue(UA_WorkQueue *wq, UA_ApplicationCallback cb,
                          void *application, void *data) {
    UA_DelayedCallback *dc = (UA_DelayedCallback*)UA_malloc(sizeof(UA_DelayedCallback));
    if(!dc) {
        cb(application, data); /* Execute immediately if the memory could not be allocated */
        return;
    }

    dc->callback = cb;
    dc->application = application;
    dc->data = data;

    /* Enqueue for the worker threads */
    pthread_mutex_lock(&wq->dispatchQueue_accessMutex);
    SIMPLEQ_INSERT_TAIL(&wq->dispatchQueue, dc, next);
    pthread_mutex_unlock(&wq->dispatchQueue_accessMutex);

    /* Wake up sleeping workers */
    pthread_cond_broadcast(&wq->dispatchQueue_condition);
}

#endif

/*********************/
/* Delayed Callbacks */
/*********************/

#ifdef UA_ENABLE_MULTITHREADING

/* Delayed Callbacks are called only when all callbacks that were dispatched
 * prior are finished. After every UA_MAX_DELAYED_SAMPLE delayed Callbacks that
 * were added to the queue, we sample the counters from the workers. The
 * counters are compared to the last counters that were sampled. If every worker
 * has proceeded the counter, then we know that all delayed callbacks prior to
 * the last sample-point are safe to execute. */

/* Sample the worker counter for every nth delayed callback. This is used to
 * test that all workers have **finished** their current job before the delayed
 * callback is processed. */
#define UA_MAX_DELAYED_SAMPLE 100

/* Call only with a held mutex for the delayed callbacks */
static void
dispatchDelayedCallbacks(UA_WorkQueue *wq, UA_DelayedCallback *cb) {
    /* Are callbacks before the last checkpoint ready? */
    for(size_t i = 0; i < wq->workersSize; ++i) {
        if(wq->workers[i].counter == wq->workers[i].checkpointCounter)
            return;
    }

    /* Dispatch all delayed callbacks up to the checkpoint.
     * TODO: Move over the entire queue up to the checkpoint in one step. */
    if(wq->delayedCallbacks_checkpoint != NULL) {
        UA_DelayedCallback *iter, *tmp_iter;
        SIMPLEQ_FOREACH_SAFE(iter, &wq->delayedCallbacks, next, tmp_iter) {
            pthread_mutex_lock(&wq->dispatchQueue_accessMutex);
            SIMPLEQ_INSERT_TAIL(&wq->dispatchQueue, iter, next);
            pthread_mutex_unlock(&wq->dispatchQueue_accessMutex);
            if(iter == wq->delayedCallbacks_checkpoint)
                break;
        }
    }

    /* Create the new sample point */
    for(size_t i = 0; i < wq->workersSize; ++i)
        wq->workers[i].checkpointCounter = wq->workers[i].counter;
    wq->delayedCallbacks_checkpoint = cb;
}

#endif

void
UA_WorkQueue_enqueueDelayed(UA_WorkQueue *wq, UA_DelayedCallback *cb) {
#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_lock(&wq->dispatchQueue_accessMutex);
#endif

    SIMPLEQ_INSERT_HEAD(&wq->delayedCallbacks, cb, next);

#ifdef UA_ENABLE_MULTITHREADING
    wq->delayedCallbacks_sinceDispatch++;
    if(wq->delayedCallbacks_sinceDispatch > UA_MAX_DELAYED_SAMPLE) {
        dispatchDelayedCallbacks(wq, cb);
        wq->delayedCallbacks_sinceDispatch = 0;
    }
    pthread_mutex_unlock(&wq->dispatchQueue_accessMutex);
#endif
}

/* Assumes all workers are shut down */
void UA_WorkQueue_manuallyProcessDelayed(UA_WorkQueue *wq) {
    UA_DelayedCallback *dc, *dc_tmp;
    SIMPLEQ_FOREACH_SAFE(dc, &wq->delayedCallbacks, next, dc_tmp) {
        SIMPLEQ_REMOVE_HEAD(&wq->delayedCallbacks, next);
        if(dc->callback)
            dc->callback(dc->application, dc->data);
        UA_free(dc);
    }
#ifdef UA_ENABLE_MULTITHREADING
    wq->delayedCallbacks_checkpoint = NULL;
#endif
}
