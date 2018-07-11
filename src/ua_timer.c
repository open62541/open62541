/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_util_internal.h"
#include "ua_timer.h"

/* Only one thread operates on the repeated jobs. This is usually the "main"
 * thread with the event loop. All other threads introduce changes via a
 * multi-producer single-consumer (MPSC) queue. The queue is based on a design
 * by Dmitry Vyukov.
 * http://www.1024cores.net/home/lock-free-algorithms/queues/intrusive-mpsc-node-based-queue
 *
 * The RepeatedCallback structure is used both in the sorted list of callbacks
 * and in the MPSC changes queue. For the changes queue, we differentiate
 * between three cases encoded in the callback pointer.
 *
 * callback > 0x01: add the new repeated callback to the sorted list
 * callback == 0x00: remove the callback with the same id
 * callback == 0x01: change the interval of the existing callback */

#define REMOVE_SENTINEL 0x00
#define CHANGE_SENTINEL 0x01

struct UA_TimerCallbackEntry {
    SLIST_ENTRY(UA_TimerCallbackEntry) next; /* Next element in the list */
    UA_DateTime nextTime;                    /* The next time when the callbacks
                                              * are to be executed */
    UA_UInt64 interval;                      /* Interval in 100ns resolution */
    UA_UInt64 id;                            /* Id of the repeated callback */

    UA_TimerCallback callback;
    void *data;
};

void
UA_Timer_init(UA_Timer *t) {
    SLIST_INIT(&t->repeatedCallbacks);
    t->changes_head = (UA_TimerCallbackEntry*)&t->changes_stub;
    t->changes_tail = (UA_TimerCallbackEntry*)&t->changes_stub;
    t->changes_stub = NULL;
    t->idCounter = 0;
}

static void
enqueueChange(UA_Timer *t, UA_TimerCallbackEntry *tc) {
    tc->next.sle_next = NULL;
    UA_TimerCallbackEntry *prev = (UA_TimerCallbackEntry*)
        UA_atomic_xchg((void * volatile *)&t->changes_head, tc);
    /* Nothing can be dequeued while the producer is blocked here */
    prev->next.sle_next = tc; /* Once this change is visible in the consumer,
                               * the node is dequeued in the following
                               * iteration */
}

static UA_TimerCallbackEntry *
dequeueChange(UA_Timer *t) {
    UA_TimerCallbackEntry *tail = t->changes_tail;
    UA_TimerCallbackEntry *next = tail->next.sle_next;
    if(tail == (UA_TimerCallbackEntry*)&t->changes_stub) {
        if(!next)
            return NULL;
        t->changes_tail = next;
        tail = next;
        next = next->next.sle_next;
    }
    if(next) {
        t->changes_tail = next;
        return tail;
    }
    UA_TimerCallbackEntry* head = t->changes_head;
    if(tail != head)
        return NULL;
    enqueueChange(t, (UA_TimerCallbackEntry*)&t->changes_stub);
    next = tail->next.sle_next;
    if(next) {
        t->changes_tail = next;
        return tail;
    }
    return NULL;
}

/* Adding repeated callbacks: Add an entry with the "nextTime" timestamp in the
 * future. This will be picked up in the next iteration and inserted at the
 * correct place. So that the next execution takes place ät "nextTime". */
UA_StatusCode
UA_Timer_addRepeatedCallback(UA_Timer *t, UA_TimerCallback callback,
                             void *data, UA_UInt32 interval,
                             UA_UInt64 *callbackId) {
    /* A callback method needs to be present */
    if(!callback)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* The interval needs to be at least 5ms */
    if(interval < 5)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the repeated callback structure */
    UA_TimerCallbackEntry *tc =
        (UA_TimerCallbackEntry*)UA_malloc(sizeof(UA_TimerCallbackEntry));
    if(!tc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set the repeated callback */
    tc->interval = (UA_UInt64)interval * UA_DATETIME_MSEC;
    tc->id = ++t->idCounter;
    tc->callback = callback;
    tc->data = data;
    tc->nextTime = UA_DateTime_nowMonotonic() + (UA_DateTime)tc->interval;

    /* Set the output identifier */
    if(callbackId)
        *callbackId = tc->id;

    /* Enqueue the changes in the MPSC queue */
    enqueueChange(t, tc);
    return UA_STATUSCODE_GOOD;
}

static void
addTimerCallbackEntry(UA_Timer *t, UA_TimerCallbackEntry * UA_RESTRICT tc) {
    /* Find the last entry before this callback */
    UA_TimerCallbackEntry *tmpTc, *afterTc = NULL;
    SLIST_FOREACH(tmpTc, &t->repeatedCallbacks, next) {
        if(tmpTc->nextTime >= tc->nextTime)
            break;

        /* The goal is to have many repeated callbacks with the same repetition
         * interval in a "block" in order to reduce linear search for re-entry
         * to the sorted list after processing. Allow the first execution to lie
         * between "nextTime - 1s" and "nextTime" if this adjustment groups
         * callbacks with the same repetition interval.
         * Callbacks of a block are added in reversed order. This design allows
         * the monitored items of a subscription (if created in a sequence with the
         * same publish/sample interval) to be executed before the subscription
         * publish the notifications */
        if(tmpTc->interval == tc->interval &&
           tmpTc->nextTime > (tc->nextTime - UA_DATETIME_SEC)) {
            tc->nextTime = tmpTc->nextTime;
            break;
        }

        /* tc is neither in the same interval nor supposed to be executed sooner
         * than tmpTc. Update afterTc to push tc further back in the timer list. */
        afterTc = tmpTc;
    }

    /* Add the repeated callback */
    if(afterTc)
        SLIST_INSERT_AFTER(afterTc, tc, next);
    else
        SLIST_INSERT_HEAD(&t->repeatedCallbacks, tc, next);
}

UA_StatusCode
UA_Timer_changeRepeatedCallbackInterval(UA_Timer *t, UA_UInt64 callbackId,
                                        UA_UInt32 interval) {
    /* The interval needs to be at least 5ms */
    if(interval < 5)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the repeated callback structure */
    UA_TimerCallbackEntry *tc =
        (UA_TimerCallbackEntry*)UA_malloc(sizeof(UA_TimerCallbackEntry));
    if(!tc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set the repeated callback */
    tc->interval = (UA_UInt64)interval * UA_DATETIME_MSEC;
    tc->id = callbackId;
    tc->nextTime = UA_DateTime_nowMonotonic() + (UA_DateTime)tc->interval;
    tc->callback = (UA_TimerCallback)CHANGE_SENTINEL;

    /* Enqueue the changes in the MPSC queue */
    enqueueChange(t, tc);
    return UA_STATUSCODE_GOOD;
}

static void
changeTimerCallbackEntryInterval(UA_Timer *t, UA_UInt64 callbackId,
                                 UA_UInt64 interval, UA_DateTime nextTime) {
    /* Remove from the sorted list */
    UA_TimerCallbackEntry *tc, *prev = NULL;
    SLIST_FOREACH(tc, &t->repeatedCallbacks, next) {
        if(callbackId == tc->id) {
            if(prev)
                SLIST_REMOVE_AFTER(prev, next);
            else
                SLIST_REMOVE_HEAD(&t->repeatedCallbacks, next);
            break;
        }
        prev = tc;
    }
    if(!tc)
        return;

    /* Adjust settings */
    tc->interval = interval;
    tc->nextTime = nextTime;

    /* Reinsert at the new position */
    addTimerCallbackEntry(t, tc);
}

/* Removing a repeated callback: Add an entry with the "nextTime" timestamp set
 * to UA_INT64_MAX. The next iteration picks this up and removes the repated
 * callback from the linked list. */
UA_StatusCode
UA_Timer_removeRepeatedCallback(UA_Timer *t, UA_UInt64 callbackId) {
    /* Allocate the repeated callback structure */
    UA_TimerCallbackEntry *tc =
        (UA_TimerCallbackEntry*)UA_malloc(sizeof(UA_TimerCallbackEntry));
    if(!tc)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Set the repeated callback with the sentinel nextTime */
    tc->id = callbackId;
    tc->callback = (UA_TimerCallback)REMOVE_SENTINEL;

    /* Enqueue the changes in the MPSC queue */
    enqueueChange(t, tc);
    return UA_STATUSCODE_GOOD;
}

static void
removeRepeatedCallback(UA_Timer *t, UA_UInt64 callbackId) {
    UA_TimerCallbackEntry *tc, *prev = NULL;
    SLIST_FOREACH(tc, &t->repeatedCallbacks, next) {
        if(callbackId == tc->id) {
            if(prev)
                SLIST_REMOVE_AFTER(prev, next);
            else
                SLIST_REMOVE_HEAD(&t->repeatedCallbacks, next);
            UA_free(tc);
            break;
        }
        prev = tc;
    }
}

/* Process the changes that were added to the MPSC queue (by other threads) */
static void
processChanges(UA_Timer *t) {
    UA_TimerCallbackEntry *change;
    while((change = dequeueChange(t))) {
        switch((uintptr_t)change->callback) {
        case REMOVE_SENTINEL:
            removeRepeatedCallback(t, change->id);
            UA_free(change);
            break;
        case CHANGE_SENTINEL:
            changeTimerCallbackEntryInterval(t, change->id, change->interval,
                                           change->nextTime);
            UA_free(change);
            break;
        default:
            addTimerCallbackEntry(t, change);
        }
    }
}

UA_DateTime
UA_Timer_process(UA_Timer *t, UA_DateTime nowMonotonic,
                 UA_TimerDispatchCallback dispatchCallback,
                 void *application) {
    /* Insert and remove callbacks */
    processChanges(t);

    /* Find the last callback to be executed now */
    UA_TimerCallbackEntry *firstAfter, *lastNow = NULL;
    SLIST_FOREACH(firstAfter, &t->repeatedCallbacks, next) {
        if(firstAfter->nextTime > nowMonotonic)
            break;
        lastNow = firstAfter;
    }

    /* Nothing to do */
    if(!lastNow) {
        if(firstAfter)
            return firstAfter->nextTime;
        return UA_INT64_MAX;
    }

    /* Put the callbacks that are executed now in a separate list */
    UA_TimerCallbackList executedNowList;
    executedNowList.slh_first = SLIST_FIRST(&t->repeatedCallbacks);
    lastNow->next.sle_next = NULL;

    /* Fake entry to represent the first element in the newly-sorted list */
    UA_TimerCallbackEntry tmp_first;
    tmp_first.nextTime = nowMonotonic - 1; /* never matches for last_dispatched */
    tmp_first.next.sle_next = firstAfter;
    UA_TimerCallbackEntry *last_dispatched = &tmp_first;

    /* Iterate over the list of callbacks to process now */
    UA_TimerCallbackEntry *tc;
    while((tc = SLIST_FIRST(&executedNowList))) {
        /* Remove from the list */
        SLIST_REMOVE_HEAD(&executedNowList, next);

        /* Dispatch/process callback */
        dispatchCallback(application, tc->callback, tc->data);

        /* Set the time for the next execution. Prevent an infinite loop by
         * forcing the next processing into the next iteration. */
        tc->nextTime += (UA_Int64)tc->interval;
        if(tc->nextTime < nowMonotonic)
            tc->nextTime = nowMonotonic + 1;

        /* Find the new position for tc to keep the list sorted */
        UA_TimerCallbackEntry *prev_tc;
        if(last_dispatched->nextTime == tc->nextTime) {
            /* We try to "batch" repeatedCallbacks with the same interval. This
             * saves a linear search when the last dispatched entry has the same
             * nextTime timestamp as this entry. */
            UA_assert(last_dispatched != &tmp_first);
            prev_tc = last_dispatched;
        } else {
            /* Find the position for the next execution by a linear search
             * starting at last_dispatched or the first element */
            if(last_dispatched->nextTime < tc->nextTime)
                prev_tc = last_dispatched;
            else
                prev_tc = &tmp_first;

            while(true) {
                UA_TimerCallbackEntry *n = SLIST_NEXT(prev_tc, next);
                if(!n || n->nextTime >= tc->nextTime)
                    break;
                prev_tc = n;
            }
        }

        /* Update last_dispatched to make sure batched callbacks are added in the
         * same sequence as before they were executed and to save some iterations
         * of the linear search for callbacks to be added further back in the list. */
        last_dispatched = tc;

        /* Add entry to the new position in the sorted list */
        SLIST_INSERT_AFTER(prev_tc, tc, next);
    }

    /* Set the entry-point for the newly sorted list */
    t->repeatedCallbacks.slh_first = tmp_first.next.sle_next;

    /* Re-repeat processAddRemoved since one of the callbacks might have removed
     * or added a callback. So we return a correct timeout. */
    processChanges(t);

    /* Return timestamp of next repetition */
    tc = SLIST_FIRST(&t->repeatedCallbacks);
    if(!tc)
        return UA_INT64_MAX; /* Main-loop has a max timeout / will continue earlier */
    return tc->nextTime;
}

void
UA_Timer_deleteMembers(UA_Timer *t) {
    /* Process changes to empty the MPSC queue */
    processChanges(t);

    /* Remove repeated callbacks */
    UA_TimerCallbackEntry *current;
    while((current = SLIST_FIRST(&t->repeatedCallbacks))) {
        SLIST_REMOVE_HEAD(&t->repeatedCallbacks, next);
        UA_free(current);
    }
}
