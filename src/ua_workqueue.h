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

#ifndef UA_WORKQUEUE_H_
#define UA_WORKQUEUE_H_

#include "ua_util_internal.h"
#include "open62541_queue.h"

#ifdef UA_ENABLE_MULTITHREADING
#include <pthread.h>
#endif

_UA_BEGIN_DECLS

/* Callback where the application is either a client or a server */
typedef void (*UA_ApplicationCallback)(void *application, void *data);

/* Delayed callbacks are executed when all previously enqueue work is finished.
 * This is used to free memory that might used by a parallel worker or where the
 * current threat has remaining pointers to until the current operation
 * finishes. */
typedef struct UA_DelayedCallback {
    SIMPLEQ_ENTRY(UA_DelayedCallback) next;
    UA_ApplicationCallback callback;
    void *application;
    void *data;
} UA_DelayedCallback;

struct UA_WorkQueue;
typedef struct UA_WorkQueue UA_WorkQueue;

#ifdef UA_ENABLE_MULTITHREADING

/* Workers take out callbacks from the work queue and execute them.
 *
 * Future Plans: Use work-stealing to load-balance between cores.
 * Le, Nhat Minh, et al. "Correct and efficient work-stealing for weak memory
 * models." ACM SIGPLAN Notices. Vol. 48. No. 8. ACM, 2013. */
typedef struct {
    pthread_t thread;
    volatile UA_Boolean running;
    UA_WorkQueue *queue;
    UA_UInt32 counter;
    UA_UInt32 checkpointCounter; /* Counter when the last checkpoint was made
                                  * for the delayed callbacks */

    /* separate cache lines */
    char padding[64 - sizeof(void*) - sizeof(pthread_t) -
                 sizeof(UA_UInt32) - sizeof(UA_Boolean)];
} UA_Worker;

#endif

struct UA_WorkQueue {
    /* Worker threads and work queue. Without multithreading, work is executed
       immediately. */
#ifdef UA_ENABLE_MULTITHREADING
    UA_Worker *workers;
    size_t workersSize;

    /* Work queue */
    SIMPLEQ_HEAD(, UA_DelayedCallback) dispatchQueue; /* Dispatch queue for the worker threads */
    pthread_mutex_t dispatchQueue_accessMutex; /* mutex for access to queue */
    pthread_cond_t dispatchQueue_condition; /* so the workers don't spin if the queue is empty */
    pthread_mutex_t dispatchQueue_conditionMutex; /* mutex for access to condition variable */
#endif

    /* Delayed callbacks
     * To be executed after all curretly dispatched works has finished */
    SIMPLEQ_HEAD(, UA_DelayedCallback) delayedCallbacks;
#ifdef UA_ENABLE_MULTITHREADING
    pthread_mutex_t delayedCallbacks_accessMutex;
    UA_DelayedCallback *delayedCallbacks_checkpoint;
    size_t delayedCallbacks_sinceDispatch; /* How many have been added since we
                                            * tried to dispatch callbacks? */
#endif
};

void UA_WorkQueue_init(UA_WorkQueue *wq);

/* Enqueue a delayed callback. It is executed when all previous work in the
 * queue has been finished. The ``cb`` pointer is freed afterwards. ``cb`` can
 * have a NULL callback that is not executed.
 *
 * This method checks internally if existing delayed work can be moved from the
 * delayed queue to the worker dispatch queue. */
void UA_WorkQueue_enqueueDelayed(UA_WorkQueue *wq, UA_DelayedCallback *cb);

/* Stop the workers, process all enqueued work in the calling thread, clean up
 * mutexes etc. */
void UA_WorkQueue_cleanup(UA_WorkQueue *wq);

#ifndef UA_ENABLE_MULTITHREADING

/* Process all enqueued delayed work. This is not needed when workers are
 * running for the multithreading case. (UA_WorkQueue_cleanup still calls this
 * method during cleanup when the workers are shut down.) */
void UA_WorkQueue_manuallyProcessDelayed(UA_WorkQueue *wq);

#else

/* Spin up a number of worker threads that listen on the work queue */
UA_StatusCode UA_WorkQueue_start(UA_WorkQueue *wq, size_t workersCount);

void UA_WorkQueue_stop(UA_WorkQueue *wq);

/* Enqueue work for the worker threads */
void UA_WorkQueue_enqueue(UA_WorkQueue *wq, UA_ApplicationCallback cb,
                          void *application, void *data);

#endif

_UA_END_DECLS

#endif /* UA_SERVER_WORKQUEUE_H_ */
