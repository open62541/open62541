#include "ua_util.h"
#include "ua_server_internal.h"

/**
 * There are three types of work:
 *
 * 1. Ordinary WorkItems (that are dispatched to worker threads if
 *    multithreading is activated)
 * 2. Timed work that is executed at a precise date (with an optional repetition
 *    interval)
 * 3. Delayed work that is executed at a later time when it is guaranteed that
 *    all previous work has actually finished (only for multithreading)
 */

#define MAXTIMEOUT 5000 // max timeout in microsec until the next main loop iteration
#define BATCHSIZE 20 // max size of worklists that are dispatched to workers

static void processWork(UA_Server *server, UA_WorkItem *work, size_t workSize) {
    for(size_t i = 0; i < workSize; i++) {
        UA_WorkItem *item = &work[i];
        switch(item->type) {
        case UA_WORKITEMTYPE_BINARYMESSAGE:
            UA_Server_processBinaryMessage(server, item->work.binaryMessage.connection,
                                           &item->work.binaryMessage.message);
            item->work.binaryMessage.connection->releaseBuffer(item->work.binaryMessage.connection,
                                                               &item->work.binaryMessage.message);
            break;
        case UA_WORKITEMTYPE_CLOSECONNECTION:
            UA_Connection_detachSecureChannel(item->work.closeConnection);
            item->work.closeConnection->close(item->work.closeConnection);
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
    
    pthread_mutex_t mutex; // required for the condition variable
    pthread_mutex_init(&mutex,0);
    pthread_mutex_lock(&mutex);
    struct timespec to;

    while(*server->running) {
        struct workListNode *wln = (struct workListNode*)
            cds_wfcq_dequeue_blocking(&server->dispatchQueue_head, &server->dispatchQueue_tail);
        if(wln) {
            processWork(server, wln->work, wln->workSize);
            UA_free(wln->work);
            UA_free(wln);
        } else {
            clock_gettime(CLOCK_REALTIME, &to);
            to.tv_sec += 2;
            pthread_cond_timedwait(&server->dispatchQueue_condition, &mutex, &to);
        }
        uatomic_inc(c); // increase the workerCounter;
    }
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
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

struct TimedWork {
    LIST_ENTRY(TimedWork) pointers;
    UA_DateTime nextTime;
    UA_UInt32 interval; ///> in ms resolution, 0 means no repetition
    size_t workSize;
    UA_WorkItem *work;
    UA_Guid workIds[];
};

/* The item is copied and not freed by this function. The interval is in 100ns (as UA_DateTime) */
static UA_StatusCode addTimedWork(UA_Server *server, const UA_WorkItem *item, UA_DateTime firstTime,
                                  UA_UInt32 interval, UA_Guid *resultWorkGuid) {
    struct TimedWork *lastTw = UA_NULL, *matchingTw = UA_NULL;
    /* search for matching entry */
    if(interval == 0) {
        LIST_FOREACH(lastTw, &server->timedWork, pointers) {
            if(lastTw->nextTime == firstTime) {
                if(lastTw->nextTime == firstTime)
                    matchingTw = lastTw;
                break;
            }
        }
    } else {
        LIST_FOREACH(matchingTw, &server->timedWork, pointers) {
            if(interval == matchingTw->interval)
                break;
        }
    }
    
    struct TimedWork *newWork;
    if(matchingTw) {
        /* append to matching entry */
        newWork = UA_realloc(matchingTw, sizeof(struct TimedWork) + sizeof(UA_Guid)*(matchingTw->workSize + 1));
        if(!newWork)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        if(newWork->pointers.le_next)
            newWork->pointers.le_next->pointers.le_prev = &newWork->pointers.le_next;
        if(newWork->pointers.le_prev)
            *newWork->pointers.le_prev = newWork;
        UA_WorkItem *newItems = UA_realloc(newWork->work, sizeof(UA_WorkItem)*(matchingTw->workSize + 1));
        if(!newItems)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        newWork->work = newItems;
    } else {
        /* create a new entry */
        newWork = UA_malloc(sizeof(struct TimedWork) + sizeof(UA_Guid));
        if(!newWork)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        newWork->work = UA_malloc(sizeof(UA_WorkItem));
        if(!newWork->work) {
            UA_free(newWork);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        newWork->workSize = 0;
        newWork->nextTime = firstTime;
        newWork->interval = interval;
        if(lastTw)
            LIST_INSERT_AFTER(lastTw, newWork, pointers);
        else
            LIST_INSERT_HEAD(&server->timedWork, newWork, pointers);
    }
    if(resultWorkGuid) {
        newWork->workIds[newWork->workSize] = UA_Guid_random(&server->random_seed);
        *resultWorkGuid = newWork->workIds[matchingTw->workSize];
    }
    newWork->work[newWork->workSize] = *item;
    newWork->workSize++;
    return UA_STATUSCODE_GOOD;
}

// Currently, these functions need to get the server mutex, but should be sufficiently fast
UA_StatusCode UA_Server_addTimedWorkItem(UA_Server *server, const UA_WorkItem *work, UA_DateTime executionTime,
                                         UA_Guid *resultWorkGuid) {
    return addTimedWork(server, work, executionTime, 0, resultWorkGuid);
}

UA_StatusCode UA_Server_addRepeatedWorkItem(UA_Server *server, const UA_WorkItem *work, UA_UInt32 interval,
                                            UA_Guid *resultWorkGuid) {
    return addTimedWork(server, work, UA_DateTime_now() + interval * 10000, interval * 10000, resultWorkGuid);
}

/** Dispatches timed work, returns the timeout until the next timed work in ms */
static UA_UInt16 processTimedWork(UA_Server *server) {
    UA_DateTime current = UA_DateTime_now();
    struct TimedWork *next = LIST_FIRST(&server->timedWork);
    struct TimedWork *tw = UA_NULL;

    while(next) {
        tw = next;
        if(tw->nextTime > current)
            break;
        next = LIST_NEXT(tw, pointers);

#ifdef UA_MULTITHREADING
        if(tw->interval > 0) {
            // copy the entry and insert at the new location
            UA_WorkItem *workCopy = UA_malloc(sizeof(UA_WorkItem) * tw->workSize);
            UA_memcpy(workCopy, tw->work, sizeof(UA_WorkItem) * tw->workSize);
            dispatchWork(server, tw->workSize, workCopy); // frees the work pointer
            tw->time += tw->interval;
            struct TimedWork *prevTw = tw; // after which tw do we insert?
            while(UA_TRUE) {
                struct TimedWork *n = LIST_NEXT(prevTw, pointers);
                if(!n || n->time > tw->time)
                    break;
                prevTw = n;
            }
            if(prevTw != tw) {
                LIST_REMOVE(tw, pointers);
                LIST_INSERT_AFTER(prevTw, tw, pointers);
            }
        } else {
            dispatchWork(server, tw->workSize, tw->work); // frees the work pointer
            LIST_REMOVE(tw, pointers);
            UA_free(tw);
        }
#else
        // 1) Process the work since it is past its due date
        processWork(server, tw->work, tw->workSize); // does not free the work ptr

        // 2) If the work is repeated, add it back into the list. Otherwise remove it.
        if(tw->interval > 0) {
            tw->nextTime += tw->interval;
            if(tw->nextTime < current)
                tw->nextTime = current;
            struct TimedWork *prevTw = tw;
            while(UA_TRUE) {
                struct TimedWork *n = LIST_NEXT(prevTw, pointers);
                if(!n || n->nextTime > tw->nextTime)
                    break;
                prevTw = n;
            }
            if(prevTw != tw) {
                LIST_REMOVE(tw, pointers);
                LIST_INSERT_AFTER(prevTw, tw, pointers);
            }
        } else {
            LIST_REMOVE(tw, pointers);
            UA_free(tw->work);
            UA_free(tw);
        }
#endif
    }

    // check if the next timed work is sooner than the usual timeout
    struct TimedWork *first = LIST_FIRST(&server->timedWork);
    UA_UInt16 timeout = MAXTIMEOUT;
    if(first) {
        timeout = (first->nextTime - current)/10;
        if(timeout > MAXTIMEOUT)
            return MAXTIMEOUT;
    }
    return timeout;
}

void UA_Server_deleteTimedWork(UA_Server *server) {
    struct TimedWork *current;
    struct TimedWork *next = LIST_FIRST(&server->timedWork);
    while(next) {
        current = next;
        next = LIST_NEXT(current, pointers);
        LIST_REMOVE(current, pointers);
        UA_free(current->workIds);
        UA_free(current);
    }
}

/****************/
/* Delayed Work */
/****************/

#ifdef UA_MULTITHREADING

#define DELAYEDWORKSIZE 100 // Collect delayed work until we have DELAYEDWORKSIZE items

struct DelayedWork {
    struct DelayedWork *next;
    UA_UInt32 *workerCounters; // initially UA_NULL until a workitem gets the counters
    UA_UInt32 workItemsCount; // the size of the array is DELAYEDWORKSIZE, the count may be less
    UA_WorkItem *workItems; // when it runs full, a new delayedWork entry is created
};

// Dispatched as a methodcall-WorkItem when the delayedwork is added
static void getCounters(UA_Server *server, DelayedWork *delayed) {
    UA_UInt32 *counters = UA_malloc(server->nThreads * sizeof(UA_UInt32));
    for(UA_UInt16 i = 0;i<server->nThreads;i++)
        counters[i] = *server->workerCounters[i];
    delayed->workerCounters = counters;
}

// Call from the main thread only. This is the only function that modifies
// server->delayedWork. processDelayedWorkQueue modifies the "next" (after the
// head).
static void addDelayedWork(UA_Server *server, UA_WorkItem work) {
    struct DelayedWork *dw = server->delayedWork;
    if(!dw || dw->workItemsCount >= DELAYEDWORKSIZE) {
        struct DelayedWork *newwork = UA_malloc(sizeof(DelayedWork));
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
    struct DelayedWork *dw = server->delayedWork;
    while(dw) {
        processWork(server, dw->workItems, dw->workItemsCount);
        struct DelayedWork *next = dw->next;
        UA_free(dw->workerCounters);
        UA_free(dw->workItems);
        UA_free(dw);
        dw = next;
    }
}

// Execute this every N seconds (repeated work) to execute delayed work that is ready
static void dispatchDelayedWork(UA_Server *server, void *data /* not used, but needed for the signature*/) {
    struct DelayedWork *dw = UA_NULL;
    struct DelayedWork *readydw = UA_NULL;
    struct DelayedWork *beforedw = server->delayedWork;

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
    pthread_cond_init(&server->dispatchQueue_condition, 0);
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
    UA_Server_addRepeatedWorkItem(server, &processDelayed, 10000000, UA_NULL);
#endif

    // 2) Start the networklayers
    for(size_t i = 0; i <server->networkLayersSize; i++)
        server->networkLayers[i].start(server->networkLayers[i].nlHandle, &server->logger);

    // 3) The loop
    while(1) {
        // 3.1) Process timed work
        UA_UInt16 timeout = processTimedWork(server);

        // 3.2) Get work from the networklayer and dispatch it
        for(size_t i = 0; i < server->networkLayersSize; i++) {
            UA_ServerNetworkLayer *nl = &server->networkLayers[i];
            UA_WorkItem *work;
            UA_Int32 workSize;
            if(*running) {
            	if(i == server->networkLayersSize-1)
            		workSize = nl->getWork(nl->nlHandle, &work, timeout);
            	else
            		workSize = nl->getWork(nl->nlHandle, &work, 0);
            } else {
                workSize = server->networkLayers[i].stop(nl->nlHandle, &work);
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
            if(workSize > 0)
                pthread_cond_broadcast(&server->dispatchQueue_condition); 
#else
            processWork(server, work, workSize);
            if(workSize > 0)
                UA_free(work);
#endif
        }

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
