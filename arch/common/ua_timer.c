/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017, 2018, 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_timer.h"

/* There may be several entries with the same nextTime in the tree. We give them
 * an absolute order by considering the memory address to break ties. Because of
 * this, the nextTime property cannot be used to lookup specific entries. */
static enum aa_cmp
cmpDateTime(const UA_DateTime *a, const UA_DateTime *b) {
    if(*a < *b)
        return AA_CMP_LESS;
    if(*a > *b)
        return AA_CMP_MORE;
    if(a == b)
        return AA_CMP_EQ;
    if(a < b)
        return AA_CMP_LESS;
    return AA_CMP_MORE;
}

/* The identifiers of entries are unique */
static enum aa_cmp
cmpId(const UA_UInt64 *a, const UA_UInt64 *b) {
    if(*a < *b)
        return AA_CMP_LESS;
    if(*a == *b)
        return AA_CMP_EQ;
    return AA_CMP_MORE;
}

static UA_DateTime
calculateNextTime(UA_DateTime currentTime, UA_DateTime baseTime,
                  UA_DateTime interval) {
    /* Take the difference between current and base time */
    UA_DateTime diffCurrentTimeBaseTime = currentTime - baseTime;

    /* Take modulo of the diff time with the interval. This is the duration we
     * are already "into" the current interval. Subtract it from (current +
     * interval) to get the next execution time. */
    UA_DateTime cycleDelay = diffCurrentTimeBaseTime % interval;

    /* Handle the special case where the baseTime is in the future */
    if(UA_UNLIKELY(cycleDelay < 0))
        cycleDelay += interval;

    return currentTime + interval - cycleDelay;
}

void
UA_Timer_init(UA_Timer *t) {
    memset(t, 0, sizeof(UA_Timer));
    aa_init(&t->root,
            (enum aa_cmp (*)(const void*, const void*))cmpDateTime,
            offsetof(UA_TimerEntry, treeEntry),
            offsetof(UA_TimerEntry, nextTime));
    aa_init(&t->idRoot,
            (enum aa_cmp (*)(const void*, const void*))cmpId,
            offsetof(UA_TimerEntry, idTreeEntry),
            offsetof(UA_TimerEntry, id));
    UA_LOCK_INIT(&t->timerMutex);
}

void
UA_Timer_addTimerEntry(UA_Timer *t, UA_TimerEntry *te, UA_UInt64 *callbackId) {
    UA_LOCK(&t->timerMutex);
    te->id = ++t->idCounter;
    if(callbackId)
        *callbackId = te->id;
    aa_insert(&t->root, te);
    aa_insert(&t->idRoot, te);
    UA_UNLOCK(&t->timerMutex);
}

static UA_StatusCode
addCallback(UA_Timer *t, UA_ApplicationCallback callback, void *application,
            void *data, UA_DateTime nextTime, UA_UInt64 interval,
            UA_TimerPolicy timerPolicy, UA_UInt64 *callbackId) {
    /* A callback method needs to be present */
    if(!callback)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the repeated callback structure */
    UA_TimerEntry *te = (UA_TimerEntry*)UA_malloc(sizeof(UA_TimerEntry));
    if(!te)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set the repeated callback */
    te->interval = (UA_UInt64)interval;
    te->id = ++t->idCounter;
    te->callback = callback;
    te->application = application;
    te->data = data;
    te->nextTime = nextTime;
    te->timerPolicy = timerPolicy;

    /* Set the output identifier */
    if(callbackId)
        *callbackId = te->id;

    aa_insert(&t->root, te);
    aa_insert(&t->idRoot, te);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Timer_addTimedCallback(UA_Timer *t, UA_ApplicationCallback callback,
                          void *application, void *data, UA_DateTime date,
                          UA_UInt64 *callbackId) {
    UA_LOCK(&t->timerMutex);
    UA_StatusCode res = addCallback(t, callback, application, data, date,
                                    0, UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
                                    callbackId);
    UA_UNLOCK(&t->timerMutex);
    return res;
}

/* Adding repeated callbacks: Add an entry with the "nextTime" timestamp in the
 * future. This will be picked up in the next iteration and inserted at the
 * correct place. So that the next execution takes place ät "nextTime". */
UA_StatusCode
UA_Timer_addRepeatedCallback(UA_Timer *t, UA_ApplicationCallback callback,
                             void *application, void *data, UA_Double interval_ms,
                             UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                             UA_UInt64 *callbackId) {
    /* The interval needs to be positive */
    if(interval_ms <= 0.0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UInt64 interval = (UA_UInt64)(interval_ms * UA_DATETIME_MSEC);
    if(interval == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute the first time for execution */
    UA_DateTime currentTime = UA_DateTime_nowMonotonic();
    UA_DateTime nextTime;
    if(baseTime == NULL) {
        /* Use "now" as the basetime */
        nextTime = currentTime + (UA_DateTime)interval;
    } else {
        nextTime = calculateNextTime(currentTime, *baseTime, (UA_DateTime)interval);
    }

    UA_LOCK(&t->timerMutex);
    UA_StatusCode res = addCallback(t, callback, application, data, nextTime,
                                    interval, timerPolicy, callbackId);
    UA_UNLOCK(&t->timerMutex);
    return res;
}

UA_StatusCode
UA_Timer_changeRepeatedCallback(UA_Timer *t, UA_UInt64 callbackId,
                                UA_Double interval_ms, UA_DateTime *baseTime,
                                UA_TimerPolicy timerPolicy) {
    /* The interval needs to be positive */
    if(interval_ms <= 0.0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UInt64 interval = (UA_UInt64)(interval_ms * UA_DATETIME_MSEC);
    if(interval == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK(&t->timerMutex);

    /* Remove from the sorted tree */
    UA_TimerEntry *te = (UA_TimerEntry*)aa_find(&t->idRoot, &callbackId);
    if(!te) {
        UA_UNLOCK(&t->timerMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    aa_remove(&t->root, te);

    /* Compute the next time for execution. The logic is identical to the
     * creation of a new repeated callback. */
    UA_DateTime currentTime = UA_DateTime_nowMonotonic();
    if(baseTime == NULL) {
        /* Use "now" as the basetime */
        te->nextTime = currentTime + (UA_DateTime)interval;
    } else {
        te->nextTime = calculateNextTime(currentTime, *baseTime, (UA_DateTime)interval);
    }

    /* Update the remaining parameters and re-insert */
    te->interval = interval;
    te->timerPolicy = timerPolicy;
    aa_insert(&t->root, te);

    UA_UNLOCK(&t->timerMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_Timer_removeCallback(UA_Timer *t, UA_UInt64 callbackId) {
    UA_LOCK(&t->timerMutex);
    UA_TimerEntry *te = (UA_TimerEntry*)aa_find(&t->idRoot, &callbackId);
    if(UA_LIKELY(te != NULL)) {
        aa_remove(&t->root, te);
        aa_remove(&t->idRoot, te);
        UA_free(te);
    }
    UA_UNLOCK(&t->timerMutex);
}

UA_DateTime
UA_Timer_process(UA_Timer *t, UA_DateTime nowMonotonic,
                 UA_TimerExecutionCallback executionCallback,
                 void *executionApplication) {
    UA_LOCK(&t->timerMutex);
    UA_TimerEntry *first;
    while((first = (UA_TimerEntry*)aa_min(&t->root)) &&
          first->nextTime <= nowMonotonic) {
        aa_remove(&t->root, first);

        /* Reinsert / remove to their new position first. Because the callback
         * can interact with the zip tree and expects the same entries in the
         * root and idRoot trees. */

        if(first->interval == 0) {
            aa_remove(&t->idRoot, first);
            if(first->callback) {
                UA_UNLOCK(&t->timerMutex);
                executionCallback(executionApplication, first->callback,
                                  first->application, first->data);
                UA_LOCK(&t->timerMutex);
            }
            UA_free(first);
            continue;
        }

        /* Set the time for the next execution. Prevent an infinite loop by
         * forcing the execution time in the next iteration.
         *
         * If the timer policy is "CurrentTime", then there is at least the
         * interval between executions. This is used for Monitoreditems, for
         * which the spec says: The sampling interval indicates the fastest rate
         * at which the Server should sample its underlying source for data
         * changes. (Part 4, 5.12.1.2) */
        first->nextTime += (UA_DateTime)first->interval;
        if(first->nextTime < nowMonotonic) {
            if(first->timerPolicy == UA_TIMER_HANDLE_CYCLEMISS_WITH_BASETIME)
                first->nextTime = calculateNextTime(nowMonotonic, first->nextTime,
                                                    (UA_DateTime)first->interval);
            else
                first->nextTime = nowMonotonic + (UA_DateTime)first->interval;
        }

        aa_insert(&t->root, first);

        if(!first->callback)
            continue;

        /* Unlock the mutes before dropping into the callback. So that the timer
         * itself can be edited within the callback. When we return, only the
         * pointer to t must still exist. */
        UA_ApplicationCallback cb = first->callback;
        void *app = first->application;
        void *data = first->data;
        UA_UNLOCK(&t->timerMutex);
        executionCallback(executionApplication, cb, app, data);
        UA_LOCK(&t->timerMutex);
    }

    /* Return the timestamp of the earliest next callback */
    first = (UA_TimerEntry*)aa_min(&t->root);
    UA_DateTime next = (first) ? first->nextTime : UA_INT64_MAX;
    if(next < nowMonotonic)
        next = nowMonotonic;
    UA_UNLOCK(&t->timerMutex);
    return next;
}

UA_DateTime
UA_Timer_nextRepeatedTime(UA_Timer *t) {
    UA_TimerEntry *first = (UA_TimerEntry*)aa_min(&t->root);
    return (first) ? first->nextTime : UA_INT64_MAX;
}

void
UA_Timer_clear(UA_Timer *t) {
    UA_LOCK(&t->timerMutex);

    /* Free all entries */
    UA_TimerEntry *top;
    while((top = (UA_TimerEntry*)aa_min(&t->idRoot))) {
        aa_remove(&t->idRoot, top);
        UA_free(top);
    }

    /* Reset the trees to avoid future access */
    t->root.root = NULL;
    t->idRoot.root = NULL;

    UA_UNLOCK(&t->timerMutex);
#if UA_MULTITHREADING >= 100
    UA_LOCK_DESTROY(&t->timerMutex);
#endif
}
