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
 */

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
# include <unistd.h> // gethostname
#endif

#define MAXTIMEOUT 50 // max timeout in millisec until the next main loop iteration

static void
processJob(UA_Server *server, UA_Job *job) {
    UA_ASSERT_RCU_UNLOCKED();
    UA_RCU_LOCK();
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

struct DispatchJob {
    struct cds_wfcq_node node; // node for the queue
    UA_Job job;
};

static void *
workerLoop(UA_Worker *worker) {
    UA_Server *server = worker->server;
    UA_UInt32 *counter = &worker->counter;
    volatile UA_Boolean *running = &worker->running;

    /* Initialize the (thread local) random seed with the ram address of worker */
    UA_random_seed((uintptr_t)worker);
    rcu_register_thread();

    pthread_mutex_t mutex; // required for the condition variable
    pthread_mutex_init(&mutex, 0);
    pthread_mutex_lock(&mutex);

    while(*running) {
        struct DispatchJob *dj = (struct DispatchJob*)
            cds_wfcq_dequeue_blocking(&server->dispatchQueue_head, &server->dispatchQueue_tail);
        if(dj) {
            processJob(server, &dj->job);
            UA_free(dj);
        } else {
            /* nothing to do. sleep until a job is dispatched (and wakes up all worker threads) */
            pthread_cond_wait(&server->dispatchQueue_condition, &mutex);
        }
        uatomic_inc(counter);
    }

    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    UA_ASSERT_RCU_UNLOCKED();
    rcu_barrier(); // wait for all scheduled call_rcu work to complete
    rcu_unregister_thread();
    return NULL;
}

static void
dispatchJob(UA_Server *server, const UA_Job *job) {
    struct DispatchJob *dj = UA_malloc(sizeof(struct DispatchJob));
    dj->job = *job;
    cds_wfcq_node_init(&dj->node);
    cds_wfcq_enqueue(&server->dispatchQueue_head, &server->dispatchQueue_tail, &dj->node);
}

static void
emptyDispatchQueue(UA_Server *server) {
    while(!cds_wfcq_empty(&server->dispatchQueue_head, &server->dispatchQueue_tail)) {
        struct DispatchJob *dj = (struct DispatchJob*)
            cds_wfcq_dequeue_blocking(&server->dispatchQueue_head, &server->dispatchQueue_tail);
        processJob(server, &dj->job);
        UA_free(dj);
    }
}

#endif

/*****************/
/* Repeated Jobs */
/*****************/

/* The linked list of jobs is sorted according to the next execution timestamp */
struct RepeatedJob {
    LIST_ENTRY(RepeatedJob) next;  /* Next element in the list */
    UA_DateTime nextTime;          /* The next time when the jobs are to be executed */
    UA_UInt64 interval;            /* Interval in 100ns resolution */
    UA_Guid id;                    /* Id of the repeated job */
    UA_Job job;                    /* The job description itself */
};

/* internal. call only from the main loop. */
static void
addRepeatedJob(UA_Server *server, struct RepeatedJob * UA_RESTRICT rj)
{
    /* search for matching entry */
    struct RepeatedJob *lastRj = NULL; /* Add after this entry or at LIST_HEAD if NULL */
    struct RepeatedJob *tempRj = LIST_FIRST(&server->repeatedJobs);
    while(tempRj) {
        if(tempRj->nextTime > rj->nextTime)
            break;
        lastRj = tempRj;
        tempRj = LIST_NEXT(lastRj, next);
    }

    /* add the repeated job */
    if(lastRj)
        LIST_INSERT_AFTER(lastRj, rj, next);
    else
        LIST_INSERT_HEAD(&server->repeatedJobs, rj, next);
}

UA_StatusCode
UA_Server_addRepeatedJob(UA_Server *server, UA_Job job,
                         UA_UInt32 intervalMs, UA_Guid *jobId) {
    /* the interval needs to be at least 5ms */
    if(intervalMs < 5)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UInt64 interval = (UA_UInt64)intervalMs * (UA_UInt64)UA_MSEC_TO_DATETIME; // from ms to 100ns resolution

    /* Create and fill the repeated job structure */
    struct RepeatedJob *rj = UA_malloc(sizeof(struct RepeatedJob));
    if(!rj)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    rj->nextTime = UA_DateTime_nowMonotonic() + (UA_Int64) interval;
    rj->interval = interval;
    rj->id = UA_Guid_random();
    rj->job = job;

#ifdef UA_ENABLE_MULTITHREADING
    /* Call addRepeatedJob from the main loop */
    struct MainLoopJob *mlw = UA_malloc(sizeof(struct MainLoopJob));
    if(!mlw) {
        UA_free(rj);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    mlw->job = (UA_Job) {
        .type = UA_JOBTYPE_METHODCALL,
        .job.methodCall = {.data = rj, .method = (void (*)(UA_Server*, void*))addRepeatedJob}};
    cds_lfs_push(&server->mainLoopJobs, &mlw->node);
#else
    /* Add directly */
    addRepeatedJob(server, rj);
#endif
    if(jobId)
        *jobId = rj->id;
    return UA_STATUSCODE_GOOD;
}

/* Returns the next datetime when a repeated job is scheduled */
static UA_DateTime
processRepeatedJobs(UA_Server *server, UA_DateTime current) {
    struct RepeatedJob *rj, *tmp_rj;
    /* Iterate over the list of elements (sorted according to the next execution timestamp) */
    LIST_FOREACH_SAFE(rj, &server->repeatedJobs, next, tmp_rj) {
        if(rj->nextTime > current)
            break;

        /* Dispatch/process job */
#ifdef UA_ENABLE_MULTITHREADING
        dispatchJob(server, &rj->job);
#else
        struct RepeatedJob **previousNext = rj->next.le_prev;
        processJob(server, &rj->job);
        /* See if the current job was deleted during processJob. That means the
           le_next field of the previous repeated job (could also be the list
           head) does no longer point to the current repeated job */
        if((void*)*previousNext != (void*)rj) {
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                         "The current repeated job removed itself");
            continue;
        }
#endif

        /* Set the time for the next execution */
        rj->nextTime += (UA_Int64)rj->interval;
        if(rj->nextTime < current)
            rj->nextTime = current;

        /* Keep the list sorted */
        struct RepeatedJob *prev_rj = LIST_FIRST(&server->repeatedJobs);
        while(true) {
            struct RepeatedJob *n = LIST_NEXT(prev_rj, next);
            if(!n || n->nextTime > rj->nextTime)
                break;
            prev_rj = n;
        }
        if(prev_rj != rj) {
            LIST_REMOVE(rj, next);
            LIST_INSERT_AFTER(prev_rj, rj, next);
        }
    }

    /* Check if the next repeated job is sooner than the usual timeout */
    struct RepeatedJob *first = LIST_FIRST(&server->repeatedJobs);
    UA_DateTime next = current + (MAXTIMEOUT * UA_MSEC_TO_DATETIME);
    if(first && first->nextTime < next)
        next = first->nextTime;
    return next;
}

/* Call this function only from the main loop! */
static void
removeRepeatedJob(UA_Server *server, UA_Guid *jobId) {
    struct RepeatedJob *rj;
    LIST_FOREACH(rj, &server->repeatedJobs, next) {
        if(!UA_Guid_equal(jobId, &rj->id))
            continue;
        LIST_REMOVE(rj, next);
        UA_free(rj);
        break;
    }
#ifdef UA_ENABLE_MULTITHREADING
    UA_free(jobId);
#endif
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
    struct RepeatedJob *current, *temp;
    LIST_FOREACH_SAFE(current, &server->repeatedJobs, next, temp) {
        LIST_REMOVE(current, next);
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

/* Call from the main thread only. This is the only function that modifies */
/* server->delayedWork. processDelayedWorkQueue modifies the "next" (after the */
/* head). */
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
            UA_Job setCounter = (UA_Job){
                .type = UA_JOBTYPE_METHODCALL, .job.methodCall =
                {.method = (void (*)(UA_Server*, void*))getCounters, .data = dj->next}};
            dispatchJob(server, &setCounter);
        }
    }
    dj->jobs[dj->jobsCount] = *job;
    dj->jobsCount++;
}

static void
delayed_free(UA_Server *server, void *data) {
    UA_free(data);
}

UA_StatusCode UA_Server_delayedFree(UA_Server *server, void *data) {
    return UA_Server_delayedCallback(server, delayed_free, data);
}

static void
addDelayedJobAsync(UA_Server *server, UA_Job *job) {
    addDelayedJob(server, job);
    UA_free(job);
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
        for(size_t i = 0; i < dw->jobsCount; i++)
            processJob(server, &dw->jobs[i]);
        struct DelayedJobs *next = uatomic_xchg(&beforedw->next, NULL);
        UA_free(dw->workerCounters);
        UA_free(dw);
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
        processJob(server, &mlw->job);
        next = (struct MainLoopJob*)mlw->node.next;
        UA_free(mlw);
        //cppcheck-suppress unreadVariable
    } while((mlw = next));
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
    }


#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if (server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER) {

        char *appName = malloc(server->config.mdnsServerName.length +1);
        memcpy(appName, server->config.mdnsServerName.data, server->config.mdnsServerName.length);
        appName[server->config.mdnsServerName.length] = '\0';

        for(size_t i = 0; i < server->config.networkLayersSize; i++) {
            UA_ServerNetworkLayer* nl = &server->config.networkLayers[i];
            UA_UInt16 port = 0;
            char hostname[256]; hostname[0] = '\0';
            const char *path;
            {
                char* uri = malloc(sizeof(char) * nl->discoveryUrl.length + 1);
                strncpy(uri, (char*) nl->discoveryUrl.data, nl->discoveryUrl.length);
                uri[nl->discoveryUrl.length] = '\0';
                UA_StatusCode retval;
                if ((retval = UA_EndpointUrl_split(uri, hostname, &port, &path)) != UA_STATUSCODE_GOOD) {
                    if (retval == UA_STATUSCODE_BADOUTOFRANGE)
                        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_NETWORK, "Server url '%s' size invalid", uri);
                    else if (retval == UA_STATUSCODE_BADATTRIBUTEIDINVALID)
                        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_NETWORK, "Server url '%s' does not begin with opc.tcp://", uri);
                    free(uri);
                    free(appName);
                    return retval;
                }
                free(uri);
            }
            UA_Discovery_addRecord(server, appName, hostname, port, path != NULL && strlen(path) ? path : "", UA_DISCOVERY_TCP, UA_TRUE,
                                   server->config.serverCapabilities, &server->config.serverCapabilitiesSize);

        }
        free(appName);

        // find any other server on the net
        UA_Discovery_multicastQuery(server);

# ifdef UA_ENABLE_MULTITHREADING
        UA_Discovery_multicastListenStart(server);
# endif
    }
#endif

    return result;
}

/* completeMessages is run synchronous on the jobs returned from the network
   layer, so that the order for processing TCP packets is never mixed up. */
static void
completeMessages(UA_Server *server, UA_Job *job) {
    UA_Boolean realloced = UA_FALSE;
    UA_StatusCode retval = UA_Connection_completeMessages(job->job.binaryMessage.connection,
                                                          &job->job.binaryMessage.message, &realloced);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADOUTOFMEMORY)
            UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_NETWORK,
                           "Lost message(s) from Connection %i as memory could not be allocated",
                           job->job.binaryMessage.connection->sockfd);
        else if(retval != UA_STATUSCODE_GOOD)
            UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_NETWORK,
                        "Could not merge half-received messages on Connection %i with error 0x%08x",
                        job->job.binaryMessage.connection->sockfd, retval);
        job->type = UA_JOBTYPE_NOTHING;
        return;
    }
    if(realloced)
        job->type = UA_JOBTYPE_BINARYMESSAGE_ALLOCATED;

    /* discard the job if message is empty - also no leak is possible here */
    if(job->job.binaryMessage.message.length == 0)
        job->type = UA_JOBTYPE_NOTHING;
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

        for(size_t k = 0; k < jobsSize; k++) {
#ifdef UA_ENABLE_MULTITHREADING
            /* Filter out delayed work */
            if(jobs[k].type == UA_JOBTYPE_METHODCALL_DELAYED) {
                addDelayedJob(server, &jobs[k]);
                jobs[k].type = UA_JOBTYPE_NOTHING;
                continue;
            }
#endif
            /* Merge half-received messages */
            if(jobs[k].type == UA_JOBTYPE_BINARYMESSAGE_NETWORKLAYER)
                completeMessages(server, &jobs[k]);
        }

        /* Dispatch/process jobs */
        for(size_t j = 0; j < jobsSize; j++) {
#ifdef UA_ENABLE_MULTITHREADING
            dispatchJob(server, &jobs[j]);
#else
            processJob(server, &jobs[j]);
#endif
        }

        if(jobsSize > 0) {
#ifdef UA_ENABLE_MULTITHREADING
            /* Wake up worker threads */
            pthread_cond_broadcast(&server->dispatchQueue_condition);
#endif
            UA_free(jobs);
        }
    }

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
# ifndef UA_ENABLE_MULTITHREADING
    if (server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER) {
        UA_DateTime multicastNextRepeat;
        UA_DateTime_init(&multicastNextRepeat);
        //TODO multicastNextRepeat does not consider new input data (requests) on the socket. It will be handled on the next call.
        // if needed, we need to use select with timeout on the multicast socket server->mdnsSocket (see example in mdnsd library) on higher level.
        if (UA_Discovery_multicastIterate(server, &multicastNextRepeat, UA_TRUE)) {
            if (multicastNextRepeat < nextRepeated) {
                UA_DateTime_copy(&multicastNextRepeat, &nextRepeated);
            }
        }
    }
# endif
#endif

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
        for(size_t j = 0; j < stopJobsSize; j++)
            processJob(server, &stopJobs[j]);
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

#ifdef UA_ENABLE_DISCOVERY_MULTICAST
    if (server->config.applicationDescription.applicationType == UA_APPLICATIONTYPE_DISCOVERYSERVER) {
        char* hostname = malloc(sizeof(char) * 256);
        if (gethostname(hostname, 255) == 0) {
            char *appName = malloc(server->config.mdnsServerName.length +1);
            memcpy(appName, server->config.mdnsServerName.data, server->config.mdnsServerName.length);
            appName[server->config.mdnsServerName.length] = '\0';
            UA_Discovery_removeRecord(server,appName, hostname, 4840, UA_TRUE);
            free(appName);
        } else {
            UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Could not get hostname for multicast discovery.");
        }
        free(hostname);

# ifdef UA_ENABLE_MULTITHREADING
        UA_Discovery_multicastListenStop(server);
# else
        // send out last package with TTL = 0
        UA_Discovery_multicastIterate(server, NULL, UA_FALSE);
# endif
    }

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
