#include "ua_util.h"
#include "ua_server_internal.h"

/**
 * There are four types of job execution:
 *
 * 1. Normal jobs (dispatched to worker threads if multithreading is activated)
 *
 * 2. Repeated jobs with a repetition interval (dispatched to worker threads)
 *
 * 3. Mainloop jobs are executed (once) from the mainloop and not in the worker threads. The server
 * contains a stack structure where all threads can add mainloop jobs for the next mainloop
 * iteration. This is used e.g. to trigger adding and removing repeated jobs without blocking the
 * mainloop.
 *
 * 4. Delayed jobs are executed once in a worker thread. But only when all normal jobs that were
 * dispatched earlier have been executed. This is achieved by a counter in the worker threads. We
 * compute from the counter if all previous jobs have finished. The delay can be very long, since we
 * try to not interfere too much with normal execution. A use case is to eventually free obsolete
 * structures that _could_ still be accessed from concurrent threads.
 *
 * - Remove the entry from the list
 * - mark it as "dead" with an atomic operation
 * - add a delayed job that frees the memory when all concurrent operations have completed
 */

#define MAXTIMEOUT 50000 // max timeout in microsec until the next main loop iteration
#define BATCHSIZE 20 // max number of jobs that are dispatched at once to workers

static void processJobs(UA_Server *server, UA_Job *jobs, size_t jobsSize) {
    for(size_t i = 0; i < jobsSize; i++) {
        UA_Job *job = &jobs[i];
        switch(job->type) {
        case UA_JOBTYPE_BINARYMESSAGE:
            UA_Server_processBinaryMessage(server, job->job.binaryMessage.connection,
                                           &job->job.binaryMessage.message);
            UA_Connection *c = job->job.binaryMessage.connection;
            c->releaseBuffer(job->job.binaryMessage.connection, &job->job.binaryMessage.message);
            break;
        case UA_JOBTYPE_CLOSECONNECTION:
            UA_Connection_detachSecureChannel(job->job.closeConnection);
            job->job.closeConnection->close(job->job.closeConnection);
            break;
        case UA_JOBTYPE_METHODCALL:
        case UA_JOBTYPE_DELAYEDMETHODCALL:
            job->job.methodCall.method(server, job->job.methodCall.data);
            break;
        default:
            UA_LOG_WARNING(server->logger, UA_LOGCATEGORY_SERVER, "Trying to execute a job of unknown type");
            break;
        }
    }
}

/*******************************/
/* Worker Threads and Dispatch */
/*******************************/

#ifdef UA_MULTITHREADING

struct MainLoopJob {
    struct cds_lfs_node node;
    UA_Job job;
};

/** Entry in the dispatch queue */
struct DispatchJobsList {
    struct cds_wfcq_node node; // node for the queue
    size_t jobsSize;
    UA_Job *jobs;
};

/** Dispatch jobs to workers. Slices the job array up if it contains more than BATCHSIZE items. The jobs
    array is freed in the worker threads. */
static void dispatchJobs(UA_Server *server, UA_Job *jobs, size_t jobsSize) {
    size_t startIndex = jobsSize; // start at the end
    while(jobsSize > 0) {
        size_t size = BATCHSIZE;
        if(size > jobsSize)
            size = jobsSize;
        startIndex = startIndex - size;
        struct DispatchJobsList *wln = UA_malloc(sizeof(struct DispatchJobsList));
        if(startIndex > 0) {
            wln->jobs = UA_malloc(size * sizeof(UA_Job));
            UA_memcpy(wln->jobs, &jobs[startIndex], size * sizeof(UA_Job));
            wln->jobsSize = size;
        } else {
            /* forward the original array */
            wln->jobsSize = size;
            wln->jobs = jobs;
        }
        cds_wfcq_node_init(&wln->node);
        cds_wfcq_enqueue(&server->dispatchQueue_head, &server->dispatchQueue_tail, &wln->node);
        jobsSize -= size;
    } 
}

// throwaway struct to bring data into the worker threads
struct workerStartData {
    UA_Server *server;
    UA_UInt32 **workerCounter;
};

/** Waits until jobs arrive in the dispatch queue and processes them. */
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
        struct DispatchJobsList *wln = (struct DispatchJobsList*)
            cds_wfcq_dequeue_blocking(&server->dispatchQueue_head, &server->dispatchQueue_tail);
        if(wln) {
            processJobs(server, wln->jobs, wln->jobsSize);
            UA_free(wln->jobs);
            UA_free(wln);
        } else {
            /* sleep until a work arrives (and wakes up all worker threads) */
            clock_gettime(CLOCK_REALTIME, &to);
            to.tv_sec += 2;
            pthread_cond_timedwait(&server->dispatchQueue_condition, &mutex, &to);
        }
        uatomic_inc(c); // increase the workerCounter;
    }
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);

    rcu_barrier(); // wait for all scheduled call_rcu work to complete
   	rcu_unregister_thread();

    /* we need to return _something_ for pthreads */
    return UA_NULL;
}

static void emptyDispatchQueue(UA_Server *server) {
    while(!cds_wfcq_empty(&server->dispatchQueue_head, &server->dispatchQueue_tail)) {
        struct DispatchJobsList *wln = (struct DispatchJobsList*)
            cds_wfcq_dequeue_blocking(&server->dispatchQueue_head, &server->dispatchQueue_tail);
        processJobs(server, wln->jobs, wln->jobsSize);
        UA_free(wln->jobs);
        UA_free(wln);
    }
}

#endif

/*****************/
/* Repeated Jobs */
/*****************/

struct IdentifiedJob {
    UA_Job job;
    UA_Guid id;
};

/**
 * The RepeatedJobs structure contains an array of jobs that are either executed with the same
 * repetition interval. The linked list is sorted, so we can stop traversing when the first element
 * has nextTime > now.
 */
struct RepeatedJobs {
    LIST_ENTRY(RepeatedJobs) pointers; ///> Links to the next list of repeated jobs (with a different) interval
    UA_DateTime nextTime; ///> The next time when the jobs are to be executed
    UA_UInt32 interval; ///> Interval in 100ns resolution
    size_t jobsSize; ///> Number of jobs contained
    struct IdentifiedJob jobs[]; ///> The jobs. This is not a pointer, instead the struct is variable sized.
};

/* throwaway struct for the mainloop callback */
struct AddRepeatedJob {
    struct IdentifiedJob job;
    UA_UInt32 interval;
};

/* internal. call only from the main loop. */
static UA_StatusCode addRepeatedJob(UA_Server *server, struct AddRepeatedJob * restrict arw) {
    struct RepeatedJobs *matchingTw = UA_NULL; // add the item here
    struct RepeatedJobs *lastTw = UA_NULL; // if there is no repeated job, add a new one this entry
    struct RepeatedJobs *tempTw;

    /* search for matching entry */
    UA_DateTime firstTime = UA_DateTime_now() + arw->interval;
    tempTw = LIST_FIRST(&server->repeatedJobs);
    while(tempTw) {
        if(arw->interval == tempTw->interval) {
            matchingTw = tempTw;
            break;
        }
        if(tempTw->nextTime > firstTime)
            break;
        lastTw = tempTw;
        tempTw = LIST_NEXT(lastTw, pointers);
    }
    
    if(matchingTw) {
        /* append to matching entry */
        matchingTw = UA_realloc(matchingTw, sizeof(struct RepeatedJobs) +
                                (sizeof(struct IdentifiedJob) * (matchingTw->jobsSize + 1)));
        if(!matchingTw) {
#ifdef UA_MULTITHREADING
            UA_free(arw);
#endif
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        /* point the realloced struct */
        if(matchingTw->pointers.le_next)
            matchingTw->pointers.le_next->pointers.le_prev = &matchingTw->pointers.le_next;
        if(matchingTw->pointers.le_prev)
            *matchingTw->pointers.le_prev = matchingTw;
    } else {
        /* create a new entry */
        matchingTw = UA_malloc(sizeof(struct RepeatedJobs) + sizeof(struct IdentifiedJob));
        if(!matchingTw) {
#ifdef UA_MULTITHREADING
            UA_free(arw);
#endif
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        matchingTw->jobsSize = 0;
        matchingTw->nextTime = firstTime;
        matchingTw->interval = arw->interval;
        if(lastTw)
            LIST_INSERT_AFTER(lastTw, matchingTw, pointers);
        else
            LIST_INSERT_HEAD(&server->repeatedJobs, matchingTw, pointers);
    }
    matchingTw->jobs[matchingTw->jobsSize] = arw->job;
    matchingTw->jobsSize++;
#ifdef UA_MULTITHREADING
    UA_free(arw);
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_addRepeatedJob(UA_Server *server, UA_Job job, UA_UInt32 interval, UA_Guid *jobId) {
    /* the interval needs to be at least 5ms */
    if(interval < 5)
        return UA_STATUSCODE_BADINTERNALERROR;
    interval *= 10000; // from ms to 100ns resolution

#ifdef UA_MULTITHREADING
    struct AddRepeatedJob *arw = UA_malloc(sizeof(struct AddRepeatedJob));
    if(!arw)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    arw->interval = interval;
    arw->job.job = job;
    if(jobId) {
        arw->job.id = UA_Guid_random(&server->random_seed);
        *jobId = arw->job.id;
    } else
        UA_Guid_init(&arw->job.id);

    struct MainLoopJob *mlw = UA_malloc(sizeof(struct MainLoopJob));
    if(!mlw) {
        UA_free(arw);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    mlw->job = (UA_Job) {
        .type = UA_JOBTYPE_METHODCALL,
        .job.methodCall = {.data = arw, .method = (void (*)(UA_Server*, void*))addRepeatedJob}};
    cds_lfs_push(&server->mainLoopJobs, &mlw->node);
#else
    struct AddRepeatedJob arw;
    arw.interval = interval;
    arw.job.job = job;
    if(jobId) {
        arw.job.id = UA_Guid_random(&server->random_seed);
        *jobId = arw.job.id;
    } else
        UA_Guid_init(&arw.job.id);
    addRepeatedJob(server, &arw);
#endif
    return UA_STATUSCODE_GOOD;
}

/* Returns the timeout until the next repeated job in ms */
static UA_UInt16 processRepeatedJobs(UA_Server *server) {
    UA_DateTime current = UA_DateTime_now();
    struct RepeatedJobs *next = LIST_FIRST(&server->repeatedJobs);
    struct RepeatedJobs *tw = UA_NULL;

    while(next) {
        tw = next;
        if(tw->nextTime > current)
            break;
        next = LIST_NEXT(tw, pointers);

#ifdef UA_MULTITHREADING
        // copy the entry and insert at the new location
        UA_Job *jobsCopy = UA_malloc(sizeof(UA_Job) * tw->jobsSize);
        if(!jobsCopy) {
            UA_LOG_ERROR(server->logger, UA_LOGCATEGORY_SERVER, "Not enough memory to dispatch delayed jobs");
            break;
        }
        for(size_t i=0;i<tw->jobsSize;i++)
            jobsCopy[i] = tw->jobs[i].job;
        dispatchJobs(server, jobsCopy, tw->jobsSize); // frees the job pointer
#else
        for(size_t i=0;i<tw->jobsSize;i++)
            processJobs(server, &tw->jobs[i].job, 1); // does not free the job ptr
#endif
        tw->nextTime += tw->interval;
        struct RepeatedJobs *prevTw = tw; // after which tw do we insert?
        while(UA_TRUE) {
            struct RepeatedJobs *n = LIST_NEXT(prevTw, pointers);
            if(!n || n->nextTime > tw->nextTime)
                break;
            prevTw = n;
        }
        if(prevTw != tw) {
            LIST_REMOVE(tw, pointers);
            LIST_INSERT_AFTER(prevTw, tw, pointers);
        }
    }

    // check if the next repeated job is sooner than the usual timeout
    struct RepeatedJobs *first = LIST_FIRST(&server->repeatedJobs);
    UA_UInt16 timeout = MAXTIMEOUT;
    if(first) {
        timeout = (first->nextTime - current)/10;
        if(timeout > MAXTIMEOUT)
            return MAXTIMEOUT;
    }
    return timeout;
}

/* Call this function only from the main loop! */
static void removeRepeatedJob(UA_Server *server, UA_Guid *jobId) {
    struct RepeatedJobs *tw;
    LIST_FOREACH(tw, &server->repeatedJobs, pointers) {
        for(size_t i = 0; i < tw->jobsSize; i++) {
            if(!UA_Guid_equal(jobId, &tw->jobs[i].id))
                continue;
            if(tw->jobsSize == 1) {
                LIST_REMOVE(tw, pointers);
                UA_free(tw);
            } else {
                tw->jobsSize--;
                tw->jobs[i] = tw->jobs[tw->jobsSize]; // move the last entry to overwrite
            }
            goto finish; // ugly break
        }
    }
 finish:
#ifdef UA_MULTITHREADING
    UA_free(jobId);
#endif
    return;
}

UA_StatusCode UA_Server_removeRepeatedJob(UA_Server *server, UA_Guid jobId) {
#ifdef UA_MULTITHREADING
    UA_Guid *idptr = UA_malloc(sizeof(UA_Guid));
    if(!idptr)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *idptr = jobId;
    // dispatch to the mainloopjobs stack
    struct MainLoopJob *mlw = UA_malloc(sizeof(struct MainLoopJob));
    mlw->job = (UA_Job) {
        .type = UA_JOBTYPE_METHODCALL,
        .job.methodCall = {.data = idptr, .method = (void (*)(UA_Server*, void*))removeRepeatedJob}};
    cds_lfs_push(&server->mainLoopJobs, &mlw->node);
#else
    removeRepeatedJob(server, &jobId);
#endif
    return UA_STATUSCODE_GOOD;
}

void UA_Server_deleteAllRepeatedJobs(UA_Server *server) {
    struct RepeatedJobs *current;
    while((current = LIST_FIRST(&server->repeatedJobs))) {
        LIST_REMOVE(current, pointers);
        UA_free(current);
    }
}

/****************/
/* Delayed Jobs */
/****************/

#ifdef UA_MULTITHREADING

#define DELAYEDJOBSSIZE 100 // Collect delayed jobs until we have DELAYEDWORKSIZE items

struct DelayedJobs {
    struct DelayedJobs *next;
    UA_UInt32 *workerCounters; // initially UA_NULL until the counter are set
    UA_UInt32 jobsCount; // the size of the array is DELAYEDJOBSSIZE, the count may be less
    UA_Job jobs[DELAYEDJOBSSIZE]; // when it runs full, a new delayedJobs entry is created
};

/* Dispatched as an ordinary job when the DelayedJobs list is full */
static void getCounters(UA_Server *server, struct DelayedJobs *delayed) {
    UA_UInt32 *counters = UA_malloc(server->nThreads * sizeof(UA_UInt32));
    for(UA_UInt16 i = 0;i<server->nThreads;i++)
        counters[i] = *server->workerCounters[i];
    delayed->workerCounters = counters;
}

// Call from the main thread only. This is the only function that modifies
// server->delayedWork. processDelayedWorkQueue modifies the "next" (after the
// head).
static void addDelayedJob(UA_Server *server, UA_Job *job) {
    struct DelayedJobs *dj = server->delayedJobs;
    if(!dj || dj->jobsCount >= DELAYEDJOBSSIZE) {
        /* create a new DelayedJobs and add it to the linked list */
        dj = UA_malloc(sizeof(struct DelayedJobs));
        if(!dj) {
            UA_LOG_ERROR(server->logger, UA_LOGCATEGORY_SERVER, "Not enough memory to add a delayed job");
            return;
        }
        dj->jobsCount = 0;
        dj->workerCounters = UA_NULL;
        dj->next = server->delayedJobs;
        server->delayedJobs = dj;

        /* dispatch a method that sets the counter for the full list that comes afterwards */
        if(dj->next) {
            UA_Job *setCounter = UA_malloc(sizeof(UA_Job));
            *setCounter = (UA_Job) {.type = UA_JOBTYPE_METHODCALL, .job.methodCall =
                                    {.method = (void (*)(UA_Server*, void*))getCounters, .data = dj->next}};
            dispatchJobs(server, setCounter, 1);
        }
    }
    dj->jobs[dj->jobsCount] = *job;
    dj->jobsCount++;
}

static void addDelayedJobAsync(UA_Server *server, UA_Job *job) {
    addDelayedJob(server, job);
    UA_free(job);
}

UA_StatusCode UA_Server_addDelayedJob(UA_Server *server, UA_Job job) {
    UA_Job *j = UA_malloc(sizeof(UA_Job));
    if(!j)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *j = job;
    struct MainLoopJob *mlw = UA_malloc(sizeof(struct MainLoopJob));
    mlw->job = (UA_Job) {.type = UA_JOBTYPE_METHODCALL, .job.methodCall =
                         {.data = j, .method = (void (*)(UA_Server*, void*))addDelayedJobAsync}};
    cds_lfs_push(&server->mainLoopJobs, &mlw->node);
    return UA_STATUSCODE_GOOD;
}

/* Find out which delayed jobs can be executed now */
static void dispatchDelayedJobs(UA_Server *server, void *data /* not used, but needed for the signature*/) {
    /* start at the second */
    struct DelayedJobs *dw = server->delayedJobs, *beforedw = dw;
    if(dw)
        dw = dw->next;

    /* find the first delayedwork where the counters have been set and have moved */
    while(dw) {
        if(!dw->workerCounters) {
            beforedw = dw;
            dw = dw->next;
            continue;
        }
        UA_Boolean allMoved = UA_TRUE;
        for(UA_UInt16 i=0;i<server->nThreads;i++) {
            if(dw->workerCounters[i] == *server->workerCounters[i]) {
                allMoved = UA_FALSE;
                break;
            }
        }
        if(allMoved)
            break;
        beforedw = dw;
        dw = dw->next;
    }

    /* process and free all delayed jobs from here on */
    while(dw) {
        processJobs(server, dw->jobs, dw->jobsCount);
        struct DelayedJobs *next = uatomic_xchg(&beforedw->next, UA_NULL);
        UA_free(dw);
        UA_free(dw->workerCounters);
        dw = next;
    }
}

#endif

/********************/
/* Main Server Loop */
/********************/

#ifdef UA_MULTITHREADING
static void processMainLoopJobs(UA_Server *server) {
    /* no synchronization required if we only use push and pop_all */
    struct cds_lfs_head *head = __cds_lfs_pop_all(&server->mainLoopJobs);
    if(!head)
        return;
    struct MainLoopJob *mlw = (struct MainLoopJob*)&head->node;
    struct MainLoopJob *next;
    do {
        processJobs(server, &mlw->job, 1);
        next = (struct MainLoopJob*)mlw->node.next;
        UA_free(mlw);
    } while((mlw = next));
    //UA_free(head);
}
#endif

UA_StatusCode UA_Server_run_startup(UA_Server *server, UA_UInt16 nThreads, UA_Boolean *running) {
#ifdef UA_MULTITHREADING
    /* Prepare the worker threads */
    server->running = running; // the threads need to access the variable
    server->nThreads = nThreads;
    pthread_cond_init(&server->dispatchQueue_condition, 0);
    server->thr = UA_malloc(nThreads * sizeof(pthread_t));
    server->workerCounters = UA_malloc(nThreads * sizeof(UA_UInt32 *));
    for(UA_UInt32 i=0;i<nThreads;i++) {
        struct workerStartData *startData = UA_malloc(sizeof(struct workerStartData));
        startData->server = server;
        startData->workerCounter = &server->workerCounters[i];
        pthread_create(&server->thr[i], UA_NULL, (void* (*)(void*))workerLoop, startData);
    }

    /* try to execute the delayed callbacks every 10 sec */
    UA_Job processDelayed = {.type = UA_JOBTYPE_METHODCALL,
                             .job.methodCall = {.method = dispatchDelayedJobs, .data = UA_NULL} };
    UA_Server_addRepeatedJob(server, processDelayed, 10000, UA_NULL);
#endif

    /* Start the networklayers */
    for(size_t i = 0; i < server->networkLayersSize; i++)
        server->networkLayers[i].start(server->networkLayers[i].nlHandle, &server->logger);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_run_mainloop(UA_Server *server, UA_Boolean *running) {
#ifdef UA_MULTITHREADING
    /* Run Work in the main loop */
    processMainLoopJobs(server);
#endif
    /* Process repeated work */
    UA_UInt16 timeout = processRepeatedJobs(server);

    /* Get work from the networklayer */
    for(size_t i = 0; i < server->networkLayersSize; i++) {
        UA_ServerNetworkLayer *nl = &server->networkLayers[i];
        UA_Job *jobs;
        UA_Int32 jobsSize;
        if(*running) {
            if(i == server->networkLayersSize-1)
                jobsSize = nl->getJobs(nl->nlHandle, &jobs, timeout);
            else
                jobsSize = nl->getJobs(nl->nlHandle, &jobs, 0);
        } else
            jobsSize = server->networkLayers[i].stop(nl->nlHandle, &jobs);

#ifdef UA_MULTITHREADING
        /* Filter out delayed work */
        for(UA_Int32 k=0;k<jobsSize;k++) {
            if(jobs[k].type != UA_JOBTYPE_DELAYEDMETHODCALL)
                continue;
            addDelayedJob(server, &jobs[k]);
            jobs[k].type = UA_JOBTYPE_NOTHING;
        }

        /* Dispatch work to the worker threads */
        dispatchJobs(server, jobs, jobsSize);

        /* Trigger sleeping worker threads */
        if(jobsSize > 0)
            pthread_cond_broadcast(&server->dispatchQueue_condition);
#else
        processJobs(server, jobs, jobsSize);
        if(jobsSize > 0)
            UA_free(jobs);
#endif
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_run_shutdown(UA_Server *server, UA_UInt16 nThreads){
#ifdef UA_MULTITHREADING
    /* Wait for all worker threads to finish */
    for(UA_UInt32 i=0;i<nThreads;i++) {
        pthread_join(server->thr[i], UA_NULL);
        UA_free(server->workerCounters[i]);
    }
    UA_free(server->workerCounters);
    UA_free(server->thr);

    /* Manually finish the work still enqueued */
    emptyDispatchQueue(server);

    /* Process the remaining delayed work */
    struct DelayedJobs *dw = server->delayedJobs;
    while(dw) {
        processJobs(server, dw->jobs, dw->jobsCount);
        struct DelayedJobs *next = dw->next;
        UA_free(dw->workerCounters);
        UA_free(dw);
        dw = next;
    }
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_run(UA_Server *server, UA_UInt16 nThreads, UA_Boolean *running) {
    UA_Server_run_startup(server, nThreads, running);
    while(*running) {
        UA_Server_run_mainloop(server, running);
    }
    UA_Server_run_shutdown(server, nThreads);
    return UA_STATUSCODE_GOOD;
}
