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
 * 
 * This approach to concurrently accessible memory is known as epoch based reclamation [1]. According to
 * [2], it performs competitively well on many-core systems. Our version of EBR does however not require
 * a global epoch. Instead, every worker thread has its own epoch counter that we observe for changes.
 * 
 * [1] Fraser, K. 2003. Practical lock freedom. Ph.D. thesis. Computer Laboratory, University of Cambridge.
 * [2] Hart, T. E., McKenney, P. E., Brown, A. D., & Walpole, J. (2007). Performance of memory reclamation
 *     for lockless synchronization. Journal of Parallel and Distributed Computing, 67(12), 1270-1285.
 * 
 * 
 */

#define MAXTIMEOUT 50 // max timeout in millisec until the next main loop iteration
#define BATCHSIZE 20 // max number of jobs that are dispatched at once to workers

static void processJobs(UA_Server *server, UA_Job *jobs, size_t jobsSize) {
    UA_ASSERT_RCU_UNLOCKED();
    UA_RCU_LOCK();
    for(size_t i = 0; i < jobsSize; i++) {
        UA_Job *job = &jobs[i];
        switch(job->type) {
        case UA_JOBTYPE_NOTHING:
            break;
        case UA_JOBTYPE_DETACHCONNECTION:
            UA_Connection_detachSecureChannel(job->job.closeConnection);
            break;
        case UA_JOBTYPE_BINARYMESSAGE_NETWORKLAYER:
            UA_Server_processBinaryMessage(server, job->job.binaryMessage.connection,
                                           &job->job.binaryMessage.message);
            UA_Connection *connection = job->job.binaryMessage.connection;
            connection->releaseRecvBuffer(connection, &job->job.binaryMessage.message);
            break;
        case UA_JOBTYPE_BINARYMESSAGE_ALLOCATED:
            UA_Server_processBinaryMessage(server, job->job.binaryMessage.connection,
                                           &job->job.binaryMessage.message);
            UA_ByteString_deleteMembers(&job->job.binaryMessage.message);
            break;
        case UA_JOBTYPE_METHODCALL:
        case UA_JOBTYPE_METHODCALL_DELAYED:
            job->job.methodCall.method(server, job->job.methodCall.data);
            break;
        default:
            UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Trying to execute a job of unknown type");
            break;
        }
    }
    UA_RCU_UNLOCK();
}

/*******************************/
/* Worker Threads and Dispatch */
/*******************************/

#ifdef UA_ENABLE_MULTITHREADING

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

static void * workerLoop(UA_Worker *worker) {
    UA_Server *server = worker->server;
    UA_UInt32 *counter = &worker->counter;
    volatile UA_Boolean *running = &worker->running;
    
    /* Initialize the (thread local) random seed with the ram address of worker */
    UA_random_seed((uintptr_t)worker);
   	rcu_register_thread();

    pthread_mutex_t mutex; // required for the condition variable
    pthread_mutex_init(&mutex,0);
    pthread_mutex_lock(&mutex);

    while(*running) {
        struct DispatchJobsList *wln = (struct DispatchJobsList*)
            cds_wfcq_dequeue_blocking(&server->dispatchQueue_head, &server->dispatchQueue_tail);
        if(!wln) {
            uatomic_inc(counter);
            /* sleep until a work arrives (and wakes up all worker threads) */
            pthread_cond_wait(&server->dispatchQueue_condition, &mutex);
            continue;
        }
        processJobs(server, wln->jobs, wln->jobsSize);
        UA_free(wln->jobs);
        UA_free(wln);
        uatomic_inc(counter);
    }

    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    UA_ASSERT_RCU_UNLOCKED();
    rcu_barrier(); // wait for all scheduled call_rcu work to complete
   	rcu_unregister_thread();
    return NULL;
}

/** Dispatch jobs to workers. Slices the job array up if it contains more than
    BATCHSIZE items. The jobs array is freed in the worker threads. */
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
            memcpy(wln->jobs, &jobs[startIndex], size * sizeof(UA_Job));
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

static void
emptyDispatchQueue(UA_Server *server) {
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
static UA_StatusCode addRepeatedJob(UA_Server *server, struct AddRepeatedJob * UA_RESTRICT arw) {
    struct RepeatedJobs *matchingTw = NULL; // add the item here
    struct RepeatedJobs *lastTw = NULL; // if there is no repeated job, add a new one this entry
    struct RepeatedJobs *tempTw;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* search for matching entry */
    UA_DateTime firstTime = UA_DateTime_nowMonotonic() + arw->interval;
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
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
        }
        /* link the reallocated tw into the list */
        LIST_REPLACE(matchingTw, matchingTw, pointers);
    } else {
        /* create a new entry */
        matchingTw = UA_malloc(sizeof(struct RepeatedJobs) + sizeof(struct IdentifiedJob));
        if(!matchingTw) {
            retval = UA_STATUSCODE_BADOUTOFMEMORY;
            goto cleanup;
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

 cleanup:
#ifdef UA_ENABLE_MULTITHREADING
    UA_free(arw);
#endif
    return retval;
}

UA_StatusCode UA_Server_addRepeatedJob(UA_Server *server, UA_Job job, UA_UInt32 interval, UA_Guid *jobId) {
    /* the interval needs to be at least 5ms */
    if(interval < 5)
        return UA_STATUSCODE_BADINTERNALERROR;
    interval *= (UA_UInt32)UA_MSEC_TO_DATETIME; // from ms to 100ns resolution

#ifdef UA_ENABLE_MULTITHREADING
    struct AddRepeatedJob *arw = UA_malloc(sizeof(struct AddRepeatedJob));
    if(!arw)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    arw->interval = interval;
    arw->job.job = job;
    if(jobId) {
        arw->job.id = UA_Guid_random();
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
        arw.job.id = UA_Guid_random();
        *jobId = arw.job.id;
    } else
        UA_Guid_init(&arw.job.id);
    addRepeatedJob(server, &arw);
#endif
    return UA_STATUSCODE_GOOD;
}

/* Returns the next datetime when a repeated job is scheduled */
static UA_DateTime processRepeatedJobs(UA_Server *server, UA_DateTime current) {
    struct RepeatedJobs *tw = NULL;
    while((tw = LIST_FIRST(&server->repeatedJobs)) != NULL) {
        if(tw->nextTime > current)
            break;

#ifdef UA_ENABLE_MULTITHREADING
        // copy the entry and insert at the new location
        UA_Job *jobsCopy = UA_malloc(sizeof(UA_Job) * tw->jobsSize);
        if(!jobsCopy) {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Not enough memory to dispatch delayed jobs");
            break;
        }
        for(size_t i=0;i<tw->jobsSize;i++)
            jobsCopy[i] = tw->jobs[i].job;
        dispatchJobs(server, jobsCopy, tw->jobsSize); // frees the job pointer
#else
        for(size_t i=0;i<tw->jobsSize;i++)
            //processJobs may sort the list but dont delete entries
            processJobs(server, &tw->jobs[i].job, 1); // does not free the job ptr
#endif

        /* set the time for the next execution */
        tw->nextTime += tw->interval;
        if(tw->nextTime < current)
            tw->nextTime = current;

        //start iterating the list from the beginning
        struct RepeatedJobs *prevTw = LIST_FIRST(&server->repeatedJobs); // after which tw do we insert?
        while(true) {
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
    // calc in 32 bit must be ok
    struct RepeatedJobs *first = LIST_FIRST(&server->repeatedJobs);
    UA_DateTime next = current + (MAXTIMEOUT * UA_MSEC_TO_DATETIME);
    if(first && first->nextTime < next)
        next = first->nextTime;
    return next;
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
#ifdef UA_ENABLE_MULTITHREADING
    UA_free(jobId);
#endif
    return;
}

UA_StatusCode UA_Server_removeRepeatedJob(UA_Server *server, UA_Guid jobId) {
#ifdef UA_ENABLE_MULTITHREADING
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
    struct RepeatedJobs *current, *temp;
    LIST_FOREACH_SAFE(current, &server->repeatedJobs, pointers, temp) {
        LIST_REMOVE(current, pointers);
        UA_free(current);
    }
}

/****************/
/* Delayed Jobs */
/****************/

#ifdef UA_ENABLE_MULTITHREADING

#define DELAYEDJOBSSIZE 100 // Collect delayed jobs until we have DELAYEDWORKSIZE items

struct DelayedJobs {
    struct DelayedJobs *next;
    UA_UInt32 *workerCounters; // initially NULL until the counter are set
    UA_UInt32 jobsCount; // the size of the array is DELAYEDJOBSSIZE, the count may be less
    UA_Job jobs[DELAYEDJOBSSIZE]; // when it runs full, a new delayedJobs entry is created
};

/* Dispatched as an ordinary job when the DelayedJobs list is full */
static void getCounters(UA_Server *server, struct DelayedJobs *delayed) {
    UA_UInt32 *counters = UA_malloc(server->config.nThreads * sizeof(UA_UInt32));
    for(UA_UInt16 i = 0; i < server->config.nThreads; i++)
        counters[i] = server->workers[i].counter;
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
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Not enough memory to add a delayed job");
            return;
        }
        dj->jobsCount = 0;
        dj->workerCounters = NULL;
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

static void server_free(UA_Server *server, void *data) {
    UA_free(data);
}

UA_StatusCode UA_Server_delayedFree(UA_Server *server, void *data) {
    UA_Job *j = UA_malloc(sizeof(UA_Job));
    if(!j)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    j->type = UA_JOBTYPE_METHODCALL;
    j->job.methodCall.data = data;
    j->job.methodCall.method = server_free;
    struct MainLoopJob *mlw = UA_malloc(sizeof(struct MainLoopJob));
    mlw->job = (UA_Job) {.type = UA_JOBTYPE_METHODCALL, .job.methodCall =
                         {.data = j, .method = (UA_ServerCallback)addDelayedJobAsync}};
    cds_lfs_push(&server->mainLoopJobs, &mlw->node);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_delayedCallback(UA_Server *server, UA_ServerCallback callback, void *data) {
    UA_Job *j = UA_malloc(sizeof(UA_Job));
    if(!j)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    j->type = UA_JOBTYPE_METHODCALL;
    j->job.methodCall.data = data;
    j->job.methodCall.method = callback;
    struct MainLoopJob *mlw = UA_malloc(sizeof(struct MainLoopJob));
    mlw->job = (UA_Job) {.type = UA_JOBTYPE_METHODCALL, .job.methodCall =
                         {.data = j, .method = (UA_ServerCallback)addDelayedJobAsync}};
    cds_lfs_push(&server->mainLoopJobs, &mlw->node);
    return UA_STATUSCODE_GOOD;
}

/* Find out which delayed jobs can be executed now */
static void
dispatchDelayedJobs(UA_Server *server, void *_) {
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
        UA_Boolean allMoved = true;
        for(size_t i = 0; i < server->config.nThreads; i++) {
            if(dw->workerCounters[i] == server->workers[i].counter) {
                allMoved = false;
                break;
            }
        }
        if(allMoved)
            break;
        beforedw = dw;
        dw = dw->next;
    }

#if (__GNUC__ <= 4 && __GNUC_MINOR__ <= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
    /* process and free all delayed jobs from here on */
    while(dw) {
        processJobs(server, dw->jobs, dw->jobsCount);
        struct DelayedJobs *next = uatomic_xchg(&beforedw->next, NULL);
        UA_free(dw);
        UA_free(dw->workerCounters);
        dw = next;
    }
#if (__GNUC__ <= 4 && __GNUC_MINOR__ <= 6)
#pragma GCC diagnostic pop
#endif

}

#endif

/********************/
/* Main Server Loop */
/********************/

#ifdef UA_ENABLE_MULTITHREADING
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

UA_StatusCode UA_Server_run_startup(UA_Server *server) {
#ifdef UA_ENABLE_MULTITHREADING
    /* Spin up the worker threads */
    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                "Spinning up %u worker thread(s)", server->config.nThreads);
    pthread_cond_init(&server->dispatchQueue_condition, 0);
    server->workers = UA_malloc(server->config.nThreads * sizeof(UA_Worker));
    if(!server->workers)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < server->config.nThreads; i++) {
        UA_Worker *worker = &server->workers[i];
        worker->server = server;
        worker->counter = 0;
        worker->running = true;
        pthread_create(&worker->thr, NULL, (void* (*)(void*))workerLoop, worker);
    }

    /* Try to execute delayed callbacks every 10 sec */
    UA_Job processDelayed = {.type = UA_JOBTYPE_METHODCALL,
                             .job.methodCall = {.method = dispatchDelayedJobs, .data = NULL} };
    UA_Server_addRepeatedJob(server, processDelayed, 10000, NULL);
#endif

    /* Start the networklayers */
    UA_StatusCode result = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < server->config.networkLayersSize; i++) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        result |= nl->start(nl, server->config.logger);
        for(size_t j = 0; j < server->endpointDescriptionsSize; j++) {
            UA_String_copy(&nl->discoveryUrl, &server->endpointDescriptions[j].endpointUrl);
        }
    }

    return result;
}

UA_UInt16 UA_Server_run_iterate(UA_Server *server, UA_Boolean waitInternal) {
#ifdef UA_ENABLE_MULTITHREADING
    /* Run work assigned for the main thread */
    processMainLoopJobs(server);
#endif
    /* Process repeated work */
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime nextRepeated = processRepeatedJobs(server, now);

    UA_UInt16 timeout = 0;
    if(waitInternal)
        timeout = (UA_UInt16)((nextRepeated - now) / UA_MSEC_TO_DATETIME);

    /* Get work from the networklayer */
    for(size_t i = 0; i < server->config.networkLayersSize; i++) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        UA_Job *jobs;
        size_t jobsSize;
        /* only the last networklayer waits on the tieout */
        if(i == server->config.networkLayersSize-1)
            jobsSize = nl->getJobs(nl, &jobs, timeout);
        else
            jobsSize = nl->getJobs(nl, &jobs, 0);

#ifdef UA_ENABLE_MULTITHREADING
        /* Filter out delayed work */
        for(size_t k = 0; k < jobsSize; k++) {
            if(jobs[k].type != UA_JOBTYPE_METHODCALL_DELAYED)
                continue;
            addDelayedJob(server, &jobs[k]);
            jobs[k].type = UA_JOBTYPE_NOTHING;
        }

        dispatchJobs(server, jobs, jobsSize);
        /* Wake up worker threads */
        if(jobsSize > 0)
            pthread_cond_broadcast(&server->dispatchQueue_condition);
#else
        processJobs(server, jobs, jobsSize);
        if(jobsSize > 0)
            UA_free(jobs);
#endif
    }

    now = UA_DateTime_nowMonotonic();
    timeout = 0;
    if(nextRepeated > now)
        timeout = (UA_UInt16)((nextRepeated - now) / UA_MSEC_TO_DATETIME);
    return timeout;
}

UA_StatusCode UA_Server_run_shutdown(UA_Server *server) {
    for(size_t i = 0; i < server->config.networkLayersSize; i++) {
        UA_ServerNetworkLayer *nl = &server->config.networkLayers[i];
        UA_Job *stopJobs;
        size_t stopJobsSize = nl->stop(nl, &stopJobs);
        processJobs(server, stopJobs, stopJobsSize);
        UA_free(stopJobs);
    }

#ifdef UA_ENABLE_MULTITHREADING
    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER,
                "Shutting down %u worker thread(s)", server->config.nThreads);
    /* Wait for all worker threads to finish */
    for(size_t i = 0; i < server->config.nThreads; i++)
        server->workers[i].running = false;
    pthread_cond_broadcast(&server->dispatchQueue_condition);
    for(size_t i = 0; i < server->config.nThreads; i++)
        pthread_join(server->workers[i].thr, NULL);
    UA_free(server->workers);

    /* Manually finish the work still enqueued.
       This especially contains delayed frees */
    emptyDispatchQueue(server);
    UA_ASSERT_RCU_UNLOCKED();
    rcu_barrier(); // wait for all scheduled call_rcu work to complete
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_Server_run(UA_Server *server, volatile UA_Boolean *running) {
    UA_StatusCode retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    while(*running)
        UA_Server_run_iterate(server, true);
    return UA_Server_run_shutdown(server);
}
