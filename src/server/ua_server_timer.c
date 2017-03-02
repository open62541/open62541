/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_util.h"
#include "ua_server_internal.h"

#define MAXTIMEOUT 50 // max timeout in millisec until the next main loop iteration

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
addRepeatedJob(UA_Server *server, struct RepeatedJob * UA_RESTRICT rj) {
    /* Search for the best position on the repeatedJobs sorted list. The goal is
     * to have many repeated jobs with the same repetition interval in a
     * "block". This helps to reduce the (linear) search to find the next entry
     * in the repeatedJobs list when dispatching the repeated jobs.
     * For this, we search between "nexttime_max - 1s" and "nexttime_max" for
     * entries with the same repetition interval and adjust the "nexttime".
     * Otherwise, add entry after the first element before "nexttime_max". */
    UA_DateTime nextTime_max = UA_DateTime_nowMonotonic() + (UA_Int64) rj->interval;

    struct RepeatedJob *afterRj = NULL;
    struct RepeatedJob *tmpRj;
    LIST_FOREACH(tmpRj, &server->repeatedJobs, next) {
        if(tmpRj->nextTime >= nextTime_max)
            break;
        if(tmpRj->interval == rj->interval &&
           tmpRj->nextTime > (nextTime_max - UA_SEC_TO_DATETIME))
            nextTime_max = tmpRj->nextTime; /* break in the next iteration */
        afterRj = tmpRj;
    }

    /* add the repeated job */
    rj->nextTime = nextTime_max;
    if(afterRj)
        LIST_INSERT_AFTER(afterRj, rj, next);
    else
        LIST_INSERT_HEAD(&server->repeatedJobs, rj, next);
}

UA_StatusCode
UA_Server_addRepeatedJob(UA_Server *server, UA_Job job,
                         UA_UInt32 interval, UA_Guid *jobId) {
    /* the interval needs to be at least 5ms */
    if(interval < 5)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UInt64 interval_dt =
        (UA_UInt64)interval * (UA_UInt64)UA_MSEC_TO_DATETIME; // from ms to 100ns resolution

    /* Create and fill the repeated job structure */
    struct RepeatedJob *rj = (struct RepeatedJob *)UA_malloc(sizeof(struct RepeatedJob));
    if(!rj)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    /* done inside addRepeatedJob:
     * rj->nextTime = UA_DateTime_nowMonotonic() + interval_dt; */
    rj->interval = interval_dt;
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

/* - Dispatches all repeated jobs that have timed out
 * - Reinserts dispatched job at their new position in the sorted list
 * - Returns the next datetime when a repeated job is scheduled */
UA_DateTime
UA_Server_processRepeatedJobs(UA_Server *server, UA_DateTime current, UA_Boolean *dispatched) {
    /* Find the last job that is executed in this iteration */
    struct RepeatedJob *lastNow = NULL, *tmp;
    LIST_FOREACH(tmp, &server->repeatedJobs, next) {
        if(tmp->nextTime > current)
            break;
        lastNow = tmp;
    }

    /* Keep pointer to the previously dispatched job to avoid linear search for
     * "batched" jobs with the same nexttime and interval */
    struct RepeatedJob tmp_last;
    tmp_last.nextTime = current-1; /* never matches. just to avoid if(last_added && ...) */
    struct RepeatedJob *last_dispatched = &tmp_last;

    /* Iterate over the list of elements (sorted according to the nextTime timestamp) */
    struct RepeatedJob *rj, *tmp_rj;
    LIST_FOREACH_SAFE(rj, &server->repeatedJobs, next, tmp_rj) {
        if(rj->nextTime > current)
            break;

        /* Dispatch/process job */
#ifdef UA_ENABLE_MULTITHREADING
        UA_Server_dispatchJob(server, &rj->job);
        *dispatched = true;
#else
        struct RepeatedJob **previousNext = rj->next.le_prev;
        UA_Server_processJob(server, &rj->job);
        /* See if the current job was deleted during processJob. That means the
         * le_next field of the previous repeated job (could also be the list
         * head) does no longer point to the current repeated job */
        if((void*)*previousNext != (void*)rj) {
            UA_LOG_DEBUG(server->config.logger, UA_LOGCATEGORY_SERVER,
                         "The current repeated job removed itself");
            continue;
        }
#endif

        /* Set the time for the next execution */
        rj->nextTime += (UA_Int64)rj->interval;

        /* Prevent an infinite loop when the repeated jobs took more time than
         * rj->interval */
        if(rj->nextTime < current)
            rj->nextTime = current + 1;

        /* Find new position for rj to keep the list sorted */
        struct RepeatedJob *prev_rj;
        if(last_dispatched->nextTime == rj->nextTime) {
            /* We "batch" repeatedJobs with the same interval in
             * addRepeatedJobs. So this might occur quite often. */
            UA_assert(last_dispatched != &tmp_last);
            prev_rj = last_dispatched;
        } else {
            /* Find the position by a linear search starting at the first
             * possible job */
            UA_assert(lastNow); /* Not NULL. Otherwise, we never reach this point. */
            prev_rj = lastNow;
            while(true) {
                struct RepeatedJob *n = LIST_NEXT(prev_rj, next);
                if(!n || n->nextTime >= rj->nextTime)
                    break;
                prev_rj = n;
            }
        }

        /* Add entry */
        if(prev_rj != rj) {
            LIST_REMOVE(rj, next);
            LIST_INSERT_AFTER(prev_rj, rj, next);
        }

        /* Update last_dispatched and loop */
        last_dispatched = rj;
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
