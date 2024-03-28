/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017, 2018, 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "timer.h"

static enum ZIP_CMP
cmpDateTime(const UA_DateTime *a, const UA_DateTime *b) {
    if(*a == *b)
        return ZIP_CMP_EQ;
    return (*a < *b) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

static enum ZIP_CMP
cmpId(const UA_UInt64 *a, const UA_UInt64 *b) {
    if(*a == *b)
        return ZIP_CMP_EQ;
    return (*a < *b) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

ZIP_FUNCTIONS(UA_TimerTree, UA_TimerEntry, treeEntry, UA_DateTime, nextTime, cmpDateTime)
ZIP_FUNCTIONS(UA_TimerIdTree, UA_TimerEntry, idTreeEntry, UA_UInt64, id, cmpId)

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
    UA_LOCK_INIT(&t->timerMutex);
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

    ZIP_INSERT(UA_TimerTree, &t->tree, te);
    ZIP_INSERT(UA_TimerIdTree, &t->idTree, te);
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
                             UA_DateTime now, UA_DateTime *baseTime,
                             UA_TimerPolicy timerPolicy, UA_UInt64 *callbackId) {
    /* The interval needs to be positive */
    if(interval_ms <= 0.0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UInt64 interval = (UA_UInt64)(interval_ms * UA_DATETIME_MSEC);
    if(interval == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Compute the first time for execution */
    UA_DateTime nextTime;
    if(baseTime == NULL) {
        nextTime = now + (UA_DateTime)interval;
    } else {
        nextTime = calculateNextTime(now, *baseTime, (UA_DateTime)interval);
    }

    UA_LOCK(&t->timerMutex);
    UA_StatusCode res = addCallback(t, callback, application, data, nextTime,
                                    interval, timerPolicy, callbackId);
    UA_UNLOCK(&t->timerMutex);
    return res;
}

UA_StatusCode
UA_Timer_changeRepeatedCallback(UA_Timer *t, UA_UInt64 callbackId,
                                UA_Double interval_ms, UA_DateTime now,
                                UA_DateTime *baseTime, UA_TimerPolicy timerPolicy) {
    /* The interval needs to be positive */
    if(interval_ms <= 0.0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UInt64 interval = (UA_UInt64)(interval_ms * UA_DATETIME_MSEC);
    if(interval == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK(&t->timerMutex);

    /* Find according to the id */
    UA_TimerEntry *te = ZIP_FIND(UA_TimerIdTree, &t->idTree, &callbackId);
    if(!te) {
        UA_UNLOCK(&t->timerMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Try to remove from the time-sorted tree. If not found, then the entry is
     * in the processTree. If that is the case, leave it there and only adjust
     * the interval and nextTime (if the TimerPolicy uses a basetime). */
    UA_Boolean normalTree = (ZIP_REMOVE(UA_TimerTree, &t->tree, te) != NULL);

    /* Compute the next time for execution. The logic is identical to the
     * creation of a new repeated callback. */
    if(baseTime == NULL) {
        te->nextTime = now + (UA_DateTime)interval;
    } else {
        te->nextTime = calculateNextTime(now, *baseTime, (UA_DateTime)interval);
    }

    /* Update the remaining parameters and re-insert */
    te->interval = interval;
    te->timerPolicy = timerPolicy;

    if(normalTree)
        ZIP_INSERT(UA_TimerTree, &t->tree, te);

    UA_UNLOCK(&t->timerMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_Timer_removeCallback(UA_Timer *t, UA_UInt64 callbackId) {
    UA_LOCK(&t->timerMutex);
    UA_TimerEntry *te = ZIP_FIND(UA_TimerIdTree, &t->idTree, &callbackId);
    if(UA_LIKELY(te != NULL)) {
        if(t->processTree.root == NULL) {
            /* Remove/free the entry */
            ZIP_REMOVE(UA_TimerTree, &t->tree, te);
            ZIP_REMOVE(UA_TimerIdTree, &t->idTree, te);
            UA_free(te);
        } else {
            /* We are currently processing. Only mark the entry to be deleted.
             * Will be removed/freed the next time we reach it in the processing
             * callback. */
            te->callback = NULL;
        }
    }
    UA_UNLOCK(&t->timerMutex);
}

struct TimerProcessContext {
    UA_Timer *t;
    UA_DateTime now;
};

static void *
processEntryCallback(void *context, UA_TimerEntry *te) {
    struct TimerProcessContext *tpc = (struct TimerProcessContext*)context;
    UA_Timer *t = tpc->t;

    /* Execute the callback. The memory is not freed during the callback.
     * Instead, whenever t->processTree != NULL, the entries are only marked for
     * deletion by setting elm->callback to NULL. */
    if(te->callback) {
        UA_UNLOCK(&t->timerMutex);
        te->callback(te->application, te->data);
        UA_LOCK(&t->timerMutex);
    }

    /* Remove and free the entry if marked for deletion or a one-time timed
     * callback */
    if(!te->callback || te->interval == 0) {
        ZIP_REMOVE(UA_TimerIdTree, &t->idTree, te);
        UA_free(te);
        return NULL;
    }

    /* Set the time for the next regular execution */
    te->nextTime += (UA_DateTime)te->interval;

    /* Handle the case where the "window" was missed. E.g. due to congestion of
     * the application or if the clock was shifted.
     *
     * If the timer policy is "CurrentTime", then there is at least the
     * interval between executions. This is used for Monitoreditems, for
     * which the spec says: The sampling interval indicates the fastest rate
     * at which the Server should sample its underlying source for data
     * changes. (Part 4, 5.12.1.2) */
    if(te->nextTime < tpc->now) {
        if(te->timerPolicy == UA_TIMER_HANDLE_CYCLEMISS_WITH_BASETIME)
            te->nextTime = calculateNextTime(tpc->now, te->nextTime,
                                              (UA_DateTime)te->interval);
        else
            te->nextTime = tpc->now + (UA_DateTime)te->interval;
    }

    /* Insert back into the time-sorted tree */
    ZIP_INSERT(UA_TimerTree, &t->tree, te);
    return NULL;
}

UA_DateTime
UA_Timer_process(UA_Timer *t, UA_DateTime now) {
    UA_LOCK(&t->timerMutex);

    /* Not reentrant. Don't call _process from within _process. */
    if(!t->processTree.root) {
        /* Move all entries <= now to processTree */
        ZIP_UNZIP(UA_TimerTree, &t->tree, &now, &t->processTree, &t->tree);

        /* Consistency check. The smallest not-processed entry isn't ready. */
        UA_assert(!ZIP_MIN(UA_TimerTree, &t->tree) ||
                  ZIP_MIN(UA_TimerTree, &t->tree)->nextTime > now);
        
        /* Iterate over the entries that need processing in-order. This also
         * moves them back to the regular time-ordered tree. */
        struct TimerProcessContext ctx;
        ctx.t = t;
        ctx.now = now;
        ZIP_ITER(UA_TimerTree, &t->processTree, processEntryCallback, &ctx);
        
        /* Reset processTree. All entries are already moved to the normal tree. */
        t->processTree.root = NULL;
    }

    /* Compute the timestamp of the earliest next callback */
    UA_TimerEntry *first = ZIP_MIN(UA_TimerTree, &t->tree);
    UA_DateTime next = (first) ? first->nextTime : UA_INT64_MAX;
    UA_UNLOCK(&t->timerMutex);
    return next;
}

UA_DateTime
UA_Timer_nextRepeatedTime(UA_Timer *t) {
    UA_LOCK(&t->timerMutex);
    UA_TimerEntry *first = ZIP_MIN(UA_TimerTree, &t->tree);
    UA_DateTime next = (first) ? first->nextTime : UA_INT64_MAX;
    UA_UNLOCK(&t->timerMutex);
    return next;
}

static void *
freeEntryCallback(void *context, UA_TimerEntry *entry) {
    UA_free(entry);
    return NULL;
}

void
UA_Timer_clear(UA_Timer *t) {
    UA_LOCK(&t->timerMutex);

    ZIP_ITER(UA_TimerIdTree, &t->idTree, freeEntryCallback, NULL);
    t->tree.root = NULL;
    t->idTree.root = NULL;
    t->idCounter = 0;

    UA_UNLOCK(&t->timerMutex);

#if UA_MULTITHREADING >= 100
    UA_LOCK_DESTROY(&t->timerMutex);
#endif
}
