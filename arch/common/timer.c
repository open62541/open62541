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

/* Global variables, only used behind the mutex */
static UA_DateTime earliest, latest, adjustedNextTime;

static void *
findTimer2Batch(void *context, UA_TimerEntry *compare) {
    UA_TimerEntry *te = (UA_TimerEntry*)context;

    /* NextTime deviation within interval? */
    if(compare->nextTime < earliest || compare->nextTime > latest)
        return NULL;

    /* Check if one interval is a multiple of the other */
    if(te->interval < compare->interval && compare->interval % te->interval != 0)
        return NULL;
    if(te->interval > compare->interval && te->interval % compare->interval != 0)
        return NULL;

    adjustedNextTime = compare->nextTime; /* Candidate found */

    /* Abort when a perfect match is found */
    return (te->interval == compare->interval) ? te : NULL;
}

/* Adjust the nextTime to batch cyclic callbacks. Look in an interval around the
 * original nextTime. Deviate from the original nextTime by at most 1/4 of the
 * interval and at most by 1s. */
static void
batchTimerEntry(UA_Timer *t, UA_TimerEntry *te) {
    if(te->timerPolicy != UA_TIMERPOLICY_CURRENTTIME)
        return;
    UA_DateTime deviate = te->interval / 4;
    if(deviate > UA_DATETIME_SEC)
        deviate = UA_DATETIME_SEC;
    earliest = te->nextTime - deviate;
    latest = te->nextTime + deviate;
    adjustedNextTime = te->nextTime;
    ZIP_ITER(UA_TimerIdTree, &t->idTree, findTimer2Batch, te);
    te->nextTime = adjustedNextTime;
}

/* Adding repeated callbacks: Add an entry with the "nextTime" timestamp in the
 * future. This will be picked up in the next iteration and inserted at the
 * correct place. So that the next execution takes place ät "nextTime". */
UA_StatusCode
UA_Timer_add(UA_Timer *t, UA_Callback callback,
             void *application, void *data, UA_Double interval_ms,
             UA_DateTime now, UA_DateTime *baseTime,
             UA_TimerPolicy timerPolicy, UA_UInt64 *callbackId) {
    /* A callback method needs to be present */
    if(!callback)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* The interval needs to be positive. The exception is for the "once" policy
     * where we allow baseTime + interval < now. Then the timer executes once in
     * the next processing iteration. */
    UA_DateTime interval = (UA_DateTime)(interval_ms * UA_DATETIME_MSEC);
    if(interval <= 0) {
        if(timerPolicy != UA_TIMERPOLICY_ONCE)
            return UA_STATUSCODE_BADINTERNALERROR;
        /* Ensure that (now + interval) == *baseTime for setting nextTime */
        if(baseTime) {
            interval = *baseTime - now;
            baseTime = NULL;
        }
    }

    /* Compute the first time for execution */
    UA_DateTime nextTime = (baseTime == NULL) ?
        now + interval : calculateNextTime(now, *baseTime, interval);

    /* Allocate the repeated callback structure */
    UA_TimerEntry *te = (UA_TimerEntry*)UA_malloc(sizeof(UA_TimerEntry));
    if(!te)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set the repeated callback */
    te->interval = interval;
    te->cb = callback;
    te->application = application;
    te->data = data;
    te->nextTime = nextTime;
    te->timerPolicy = timerPolicy;

    /* Adjust the nextTime to batch cyclic callbacks */
    batchTimerEntry(t, te);

    /* Insert into the timer */
    UA_LOCK(&t->timerMutex);
    te->id = ++t->idCounter;
    if(callbackId)
        *callbackId = te->id;
    ZIP_INSERT(UA_TimerTree, &t->tree, te);
    ZIP_INSERT(UA_TimerIdTree, &t->idTree, te);
    UA_UNLOCK(&t->timerMutex);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Timer_modify(UA_Timer *t, UA_UInt64 callbackId,
                UA_Double interval_ms, UA_DateTime now,
                UA_DateTime *baseTime, UA_TimerPolicy timerPolicy) {
    /* The interval needs to be positive. The exception is for the "once" policy
     * where we allow baseTime + interval < now. Then the timer executes once in
     * the next processing iteration. */
    UA_DateTime interval = (UA_DateTime)(interval_ms * UA_DATETIME_MSEC);
    if(interval <= 0) {
        if(timerPolicy != UA_TIMERPOLICY_ONCE)
            return UA_STATUSCODE_BADINTERNALERROR;
        /* Ensure that (now + interval) == *baseTime for setting nextTime */
        if(baseTime) {
            interval = *baseTime - now;
            baseTime = NULL;
        }
    }

    UA_LOCK(&t->timerMutex);

    /* Find timer entry based on id */
    UA_TimerEntry *te = ZIP_FIND(UA_TimerIdTree, &t->idTree, &callbackId);
    if(!te) {
        UA_UNLOCK(&t->timerMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* The entry is either in the timer tree or current processed. If
     * in-process, the entry is re-added to the timer-tree right after. */
    UA_Boolean processing = (ZIP_REMOVE(UA_TimerTree, &t->tree, te) == NULL);

    /* The nextTime must only be modified after ZIP_REMOVE. The logic is
     * identical to the creation of a new timer. */
    te->nextTime = (baseTime == NULL) ?
        now + interval : calculateNextTime(now, *baseTime, interval);
    te->interval = interval;
    te->timerPolicy = timerPolicy;

    /* Adjust the nextTime to batch cyclic callbacks */
    batchTimerEntry(t, te);

    if(processing)
        te->nextTime -= interval; /* adjust for re-adding after processing */
    else
        ZIP_INSERT(UA_TimerTree, &t->tree, te);

    UA_UNLOCK(&t->timerMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_Timer_remove(UA_Timer *t, UA_UInt64 callbackId) {
    UA_LOCK(&t->timerMutex);
    UA_TimerEntry *te = ZIP_FIND(UA_TimerIdTree, &t->idTree, &callbackId);
    if(!te) {
        UA_UNLOCK(&t->timerMutex);
        return;
    }

    /* The entry is either in the timer tree or in the process tree. If in the
     * process tree, leave a sentinel (callback == NULL) to delete it during
     * processing. Do not edit the process tree while iterating over it. */
    UA_Boolean processing = (ZIP_REMOVE(UA_TimerTree, &t->tree, te) == NULL);
    if(!processing) {
        ZIP_REMOVE(UA_TimerIdTree, &t->idTree, te);
        UA_free(te);
    } else {
        te->cb = NULL;
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

    /* Execute the callback */
    if(te->cb) {
        te->cb(te->application, te->data);
    }

    /* Remove the entry if marked for deletion or a "once" policy */
    if(!te->cb || te->timerPolicy == UA_TIMERPOLICY_ONCE) {
        ZIP_REMOVE(UA_TimerIdTree, &t->idTree, te);
        UA_free(te);
        return NULL;
    }

    /* Set the time for the next regular execution */
    te->nextTime += te->interval;

    /* Handle the case where the execution "window" was missed. E.g. due to
     * congestion of the application or if the clock was shifted.
     *
     * If the timer policy is "CurrentTime", then there is at least the
     * interval between executions. This is used for Monitoreditems, for
     * which the spec says: The sampling interval indicates the fastest rate
     * at which the Server should sample its underlying source for data
     * changes. (Part 4, 5.12.1.2).
     *
     * Otherwise calculate the next execution time based on the original base
     * time. */
    if(te->nextTime < tpc->now) {
        te->nextTime = (te->timerPolicy == UA_TIMERPOLICY_CURRENTTIME) ?
            tpc->now + te->interval :
            calculateNextTime(tpc->now, te->nextTime, te->interval);
    }

    /* Insert back into the time-sorted tree */
    ZIP_INSERT(UA_TimerTree, &t->tree, te);
    return NULL;
}

UA_DateTime
UA_Timer_process(UA_Timer *t, UA_DateTime now) {
    UA_LOCK(&t->timerMutex);

    /* Move all entries <= now to the processTree */
    UA_TimerTree processTree;
    ZIP_INIT(&processTree);
    ZIP_UNZIP(UA_TimerTree, &t->tree, &now, &processTree, &t->tree);

    /* Consistency check. The smallest not-processed entry isn't ready. */
    UA_assert(!ZIP_MIN(UA_TimerTree, &t->tree) ||
              ZIP_MIN(UA_TimerTree, &t->tree)->nextTime > now);
        
    /* Iterate over the entries that need processing in-order. This also
     * moves them back to the regular time-ordered tree. */
    struct TimerProcessContext ctx;
    ctx.t = t;
    ctx.now = now;
    ZIP_ITER(UA_TimerTree, &processTree, processEntryCallback, &ctx);
        
    /* Compute the timestamp of the earliest next callback */
    UA_TimerEntry *first = ZIP_MIN(UA_TimerTree, &t->tree);
    UA_DateTime next = (first) ? first->nextTime : UA_INT64_MAX;
    UA_UNLOCK(&t->timerMutex);
    return next;
}

UA_DateTime
UA_Timer_next(UA_Timer *t) {
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
