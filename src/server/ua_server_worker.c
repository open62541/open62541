#include <stdio.h>
#define __USE_POSIX199309 //nanosleep
#include <time.h> //nanosleep
#include "ua_server_internal.h"

/**
 * There are three types of work:
 *
 * 1. Ordinary WorkItems (that are dispatched to worker threads if
 *    multithreading is activated)
 *
 * 2. Timed work that is executed at a precise date (with an optional repetition
 *    interval)
 *
 * 3. Delayed work that is executed at a later time when it is guaranteed that
 *    all previous work has actually finished (only for multithreading)
 */

#define MAXTIMEOUT 5000 // max timeout in usec until the next main loop iteration
#define BATCHSIZE 20 // max size of worklists that are dispatched to workers

static void processWork(UA_Server *server, const UA_WorkItem *work, UA_Int32 workSize) {
    for(UA_Int32 i = 0;i<workSize;i++) {
        const UA_WorkItem *item = &work[i];
        switch(item->type) {
        case UA_WORKITEMTYPE_BINARYNETWORKMESSAGE:
            UA_Server_processBinaryMessage(server, item->work.binaryNetworkMessage.connection,
                                           &item->work.binaryNetworkMessage.message);
            UA_free(item->work.binaryNetworkMessage.message.data);
            break;

        case UA_WORKITEMTYPE_METHODCALL:
        case UA_WORKITEMTYPE_DELAYEDMETHODCALL:
            item->work.methodCall.method(server, item->work.methodCall.data);
            break;

        default:
            break;
        }
    }
}

/*******************************/
/* Worker Threads and Dispatch */
/*******************************/

#ifdef UA_MULTITHREADING

/** Entry in the dipatch queue */
struct workListNode {
    struct cds_wfcq_node node; // node for the queue
    UA_UInt32 workSize;
    UA_WorkItem *work;
};

/** Dispatch work to workers. Slices the work up if it contains more than
    BATCHSIZE items. The work array is freed by the worker threads. */
static void dispatchWork(UA_Server *server, UA_Int32 workSize, UA_WorkItem *work) {
    UA_Int32 startIndex = workSize; // start at the end
    while(workSize > 0) {
        UA_Int32 size = BATCHSIZE;
        if(size > workSize)
            size = workSize;
        startIndex = startIndex - size;
        struct workListNode *wln = UA_malloc(sizeof(struct workListNode));
        if(startIndex > 0) {
            UA_WorkItem *workSlice = UA_malloc(size * sizeof(UA_WorkItem));
            UA_memcpy(workSlice, &work[startIndex], size * sizeof(UA_WorkItem));
            *wln = (struct workListNode){.workSize = size, .work = workSlice};
        }
        else {
            // do not alloc, but forward the original array
            *wln = (struct workListNode){.workSize = size, .work = work};
        }
        cds_wfcq_node_init(&wln->node);
        cds_wfcq_enqueue(&server->dispatchQueue_head, &server->dispatchQueue_tail, &wln->node);
        workSize -= size;
    } 
}

// throwaway struct to bring data into the worker threads
struct workerStartData {
    UA_Server *server;
    UA_UInt32 **workerCounter;
};

/** Waits until work arrives in the dispatch queue (restart after 10ms) and
    processes it. */
static void * workerLoop(struct workerStartData *startInfo) {
   	rcu_register_thread();
    UA_UInt32 *c = UA_malloc(sizeof(UA_UInt32));
    uatomic_set(c, 0);

    *startInfo->workerCounter = c;
    UA_Server *server = startInfo->server;
    UA_free(startInfo);
    
    while(*server->running) {
        struct workListNode *wln = (struct workListNode*)
            cds_wfcq_dequeue_blocking(&server->dispatchQueue_head, &server->dispatchQueue_tail);
        if(wln) {
            processWork(server, wln->work, wln->workSize);
            UA_free(wln->work);
            UA_free(wln);
        }
        uatomic_inc(c); // increase the workerCounter;
    }
   	rcu_unregister_thread();
    return UA_NULL;
}

static void emptyDispatchQueue(UA_Server *server) {
    while(!cds_wfcq_empty(&server->dispatchQueue_head, &server->dispatchQueue_tail)) {
        struct workListNode *wln = (struct workListNode*)
            cds_wfcq_dequeue_blocking(&server->dispatchQueue_head, &server->dispatchQueue_tail);
        processWork(server, wln->work, wln->workSize);
        UA_free(wln->work);
        UA_free(wln);
    }
}

#endif

/**************/
/* Timed Work */
/**************/

struct UA_TimedWork {
    LIST_ENTRY(UA_TimedWork) pointers;
    UA_UInt16 workSize;
    UA_WorkItem *work;
    UA_Guid *workIds;
    UA_DateTime time;
    UA_UInt32 repetitionInterval; // in 100ns resolution, 0 means no repetition
};

/* The item is copied and not freed by this function. */
static UA_Guid addTimedWork(UA_Server *server, UA_WorkItem *item, UA_DateTime firstTime,
                            UA_UInt32 repetitionInterval) {
    UA_TimedWork *tw, *lastTw = UA_NULL;

    // search for matching entry
    LIST_FOREACH(tw, &server->timedWork, pointers) {
        if(tw->repetitionInterval == repetitionInterval &&
           (repetitionInterval > 0 || tw->time == firstTime))
            break; // found a matching entry
        if(tw->time > firstTime) {
            tw = UA_NULL; // not matchin entry exists
            lastTw = tw;
            break;
        }
    }
    
    if(tw) {
        // append to matching entry
        tw->workSize++;
        tw->work = UA_realloc(tw->work, sizeof(UA_WorkItem)*tw->workSize);
        tw->workIds = UA_realloc(tw->workIds, sizeof(UA_Guid)*tw->workSize);
        tw->work[tw->workSize-1] = *item;
        tw->workIds[tw->workSize-1] = UA_Guid_random(&server->random_seed);
        return tw->workIds[tw->workSize-1];
    }

    // create a new entry
    tw = UA_malloc(sizeof(UA_TimedWork));
    tw->workSize = 1;
    tw->time = firstTime;
    tw->repetitionInterval = repetitionInterval;
    tw->work = UA_malloc(sizeof(UA_WorkItem));
    tw->work[0] = *item;
    tw->workIds = UA_malloc(sizeof(UA_Guid));
    tw->workIds[0] = UA_Guid_random(&server->random_seed);
    if(lastTw)
        LIST_INSERT_AFTER(lastTw, tw, pointers);
    else
        LIST_INSERT_HEAD(&server->timedWork, tw, pointers);

    return tw->workIds[0];
}

// Currently, these functions need to get the server mutex, but should be sufficiently fast
UA_Guid UA_Server_addTimedWorkItem(UA_Server *server, UA_WorkItem *work, UA_DateTime time) {
    return addTimedWork(server, work, time, 0);
}

UA_Guid UA_Server_addRepeatedWorkItem(UA_Server *server, UA_WorkItem *work, UA_UInt32 interval) {
    return addTimedWork(server, work, UA_DateTime_now() + interval, interval);
}

/** Dispatches timed work, returns the timeout until the next timed work in ms */
static UA_UInt16 processTimedWork(UA_Server *server) {
    UA_DateTime current = UA_DateTime_now();
    UA_TimedWork *next = LIST_FIRST(&server->timedWork);
    UA_TimedWork *tw = UA_NULL;

    while(next) {
        tw = next;
        if(tw->time > current)
            break;
        next = LIST_NEXT(tw, pointers);

#ifdef UA_MULTITHREADING
        if(tw->repetitionInterval > 0) {
            // copy the entry and insert at the new location
            UA_WorkItem *workCopy = UA_malloc(sizeof(UA_WorkItem) * tw->workSize);
            UA_memcpy(workCopy, tw->work, sizeof(UA_WorkItem) * tw->workSize);
            dispatchWork(server, tw->workSize, workCopy); // frees the work pointer
            tw->time += tw->repetitionInterval;

            UA_TimedWork *prevTw = tw; // after which tw do we insert?
            while(UA_TRUE) {
                UA_TimedWork *next = LIST_NEXT(prevTw, pointers);
                if(!next || next->time > tw->time)
                    break;
                prevTw = next;
            }
            if(prevTw != tw) {
                LIST_REMOVE(tw, pointers);
                LIST_INSERT_AFTER(prevTw, tw, pointers);
            }
        } else {
            dispatchWork(server, tw->workSize, tw->work); // frees the work pointer
            LIST_REMOVE(tw, pointers);
            UA_free(tw->workIds);
            UA_free(tw);
        }
#else
        processWork(server,tw->work, tw->workSize); // does not free the work
        if(tw->repetitionInterval > 0) {
            tw->time += tw->repetitionInterval;
            UA_TimedWork *prevTw = tw;
            while(UA_TRUE) {
                UA_TimedWork *next = LIST_NEXT(prevTw, pointers);
                if(!next || next->time > tw->time)
                    break;
                prevTw = next;
            }
            if(prevTw != tw) {
                LIST_REMOVE(tw, pointers);
                LIST_INSERT_AFTER(prevTw, tw, pointers);
            }
        } else {
            LIST_REMOVE(tw, pointers);
            UA_free(tw->work);
            UA_free(tw->workIds);
            UA_free(tw);
        }
#endif
    }

    tw = LIST_FIRST(&server->timedWork);
    UA_UInt16 timeout = MAXTIMEOUT;
    if(tw)
        timeout = (tw->time - current)/10;
    if(timeout>MAXTIMEOUT)timeout = MAXTIMEOUT;
    return timeout;
}

void UA_Server_deleteTimedWork(UA_Server *server) {
    UA_TimedWork *tw;
    while((tw = LIST_FIRST(&server->timedWork))) {
        LIST_REMOVE(tw, pointers);
        UA_free(tw->work);
        UA_free(tw->workIds);
        UA_free(tw);
    }
}

/****************/
/* Delayed Work */
/****************/

#ifdef UA_MULTITHREADING

#define DELAYEDWORKSIZE 100 // Collect delayed work until we have DELAYEDWORKSIZE items

struct UA_DelayedWork {
    UA_DelayedWork *next;
    UA_UInt32 *workerCounters; // initially UA_NULL until a workitem gets the counters
    UA_UInt32 workItemsCount; // the size of the array is DELAYEDWORKSIZE, the count may be less
    UA_WorkItem *workItems; // when it runs full, a new delayedWork entry is created
};

// Dispatched as a methodcall-WorkItem when the delayedwork is added
static void getCounters(UA_Server *server, UA_DelayedWork *delayed) {
    UA_UInt32 *counters = UA_malloc(server->nThreads * sizeof(UA_UInt32));
    for(UA_UInt16 i = 0;i<server->nThreads;i++)
        counters[i] = *server->workerCounters[i];
    delayed->workerCounters = counters;
}

// Call from the main thread only. This is the only function that modifies
// server->delayedWork. processDelayedWorkQueue modifies the "next" (after the
// head).
static void addDelayedWork(UA_Server *server, UA_WorkItem work) {
    UA_DelayedWork *dw = server->delayedWork;
    if(!dw || dw->workItemsCount >= DELAYEDWORKSIZE) {
        UA_DelayedWork *newwork = UA_malloc(sizeof(UA_DelayedWork));
        newwork->workItems = UA_malloc(sizeof(UA_WorkItem)*DELAYEDWORKSIZE);
        newwork->workItemsCount = 0;
        newwork->workerCounters = UA_NULL;
        newwork->next = server->delayedWork;

        // dispatch a method that sets the counter
        if(dw && dw->workItemsCount >= DELAYEDWORKSIZE) {
            UA_WorkItem *setCounter = UA_malloc(sizeof(UA_WorkItem));
            *setCounter = (UA_WorkItem)
                {.type = UA_WORKITEMTYPE_METHODCALL,
                 .work.methodCall = {.method = (void (*)(UA_Server*, void*))getCounters, .data = dw}};
            dispatchWork(server, 1, setCounter);
        }

        server->delayedWork = newwork;
        dw = newwork;
    }
    dw->workItems[dw->workItemsCount] = work;
    dw->workItemsCount++;
}

static void processDelayedWork(UA_Server *server) {
    UA_DelayedWork *dw = server->delayedWork;
    while(dw) {
        processWork(server, dw->workItems, dw->workItemsCount);
        UA_DelayedWork *next = dw->next;
        UA_free(dw->workerCounters);
        UA_free(dw->workItems);
        UA_free(dw);
        dw = next;
    }
}

// Execute this every N seconds (repeated work) to execute delayed work that is ready
static void dispatchDelayedWork(UA_Server *server, void *data /* not used, but needed for the signature*/) {
    UA_DelayedWork *dw = UA_NULL;
    UA_DelayedWork *readydw = UA_NULL;
    UA_DelayedWork *beforedw = server->delayedWork;

    // start at the second...
    if(beforedw)
        dw = beforedw->next;

    // find the first delayedwork where the counters are set and have been moved
    while(dw) {
        if(!dw->workerCounters) {
            beforedw = dw;
            dw = dw->next;
            continue;
        }

        UA_Boolean countersMoved = UA_TRUE;
        for(UA_UInt16 i=0;i<server->nThreads;i++) {
            if(*server->workerCounters[i] == dw->workerCounters[i])
                countersMoved = UA_FALSE;
                break;
        }
        
        if(countersMoved) {
            readydw = uatomic_xchg(&beforedw->next, UA_NULL);
            break;
        } else {
            beforedw = dw;
            dw = dw->next;
        }
    }

    // we have a ready entry. all afterwards are also ready
    while(readydw) {
        dispatchWork(server, readydw->workItemsCount, readydw->workItems);
        beforedw = readydw;
        readydw = readydw->next;
        UA_free(beforedw->workerCounters);
        UA_free(beforedw);
    }
}

#endif

/********************/
/* Main Server Loop */
/********************/

UA_StatusCode UA_Server_run(UA_Server *server, UA_UInt16 nThreads, UA_Boolean *running) {
#ifdef UA_MULTITHREADING
    // 1) Prepare the threads
    server->running = running; // the threads need to access the variable
    server->nThreads = nThreads;
    pthread_t *thr = UA_malloc(nThreads * sizeof(pthread_t));
    server->workerCounters = UA_malloc(nThreads * sizeof(UA_UInt32 *));
    for(UA_UInt32 i=0;i<nThreads;i++) {
        struct workerStartData *startData = UA_malloc(sizeof(struct workerStartData));
        startData->server = server;
        startData->workerCounter = &server->workerCounters[i];
        pthread_create(&thr[i], UA_NULL, (void* (*)(void*))workerLoop, startData);
    }

    UA_WorkItem processDelayed = {.type = UA_WORKITEMTYPE_METHODCALL,
                                  .work.methodCall = {.method = dispatchDelayedWork,
                                                      .data = UA_NULL} };
    UA_Server_addRepeatedWorkItem(server, &processDelayed, 10000000);
#endif

    // 2) Start the networklayers
    for(UA_Int32 i=0;i<server->nlsSize;i++)
        server->nls[i].start(server->nls[i].nlHandle);

    // 3) The loop
    while(1) {
        // 3.1) Process timed work
        UA_UInt16 timeout = processTimedWork(server);

        // 3.2) Get work from the networklayer and dispatch it
        for(UA_Int32 i=0;i<server->nlsSize;i++) {
            UA_NetworkLayer *nl = &server->nls[i];
            UA_WorkItem *work;
            UA_Int32 workSize;
            if(*running) {
                    workSize = nl->getWork(nl->nlHandle, &work, 0);
            } else {
                workSize = server->nls[i].stop(nl->nlHandle, &work);
            }

#ifdef UA_MULTITHREADING
            // Filter out delayed work
            for(UA_Int32 k=0;k<workSize;k++) {
                if(work[k].type != UA_WORKITEMTYPE_DELAYEDMETHODCALL)
                    continue;
                addDelayedWork(server, work[k]);
                work[k].type = UA_WORKITEMTYPE_NOTHING;
            }
            dispatchWork(server, workSize, work);
#else
            processWork(server, work, workSize);
            UA_free(work);
#endif
        }

        struct timespec reqtime;
        reqtime.tv_sec = 0;
        reqtime.tv_nsec = timeout * 1000; //from us to ns
        nanosleep(&reqtime, UA_NULL);

        // 3.3) Exit?
        if(!*running)
            break;
    }

#ifdef UA_MULTITHREADING
    // 4) Clean up: Wait until all worker threads finish, then empty the
    // dispatch queue, then process the remaining delayed work
    for(UA_UInt32 i=0;i<nThreads;i++) {
        pthread_join(thr[i], UA_NULL);
        UA_free(server->workerCounters[i]);
    }
    UA_free(server->workerCounters);
    UA_free(thr);
    emptyDispatchQueue(server);
    processDelayedWork(server);
#endif

    return UA_STATUSCODE_GOOD;
}
