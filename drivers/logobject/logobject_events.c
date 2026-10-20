/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include "logobject_internal.h"

#ifdef UA_ENABLE_DRIVER_LOGOBJECT

#include <string.h>

/* LogOverflowEventType Events (Part 26, 6.4). A ring buffer at MaxRecords
 * evicts a record for every new one. So a LogObject emits at most one Event per
 * overflow-event-interval. And the emission of an Event logs itself, which
 * would overflow the ServerLog again in the next cycle. The overflows within
 * the interval are reported together with the next Event. */

/* Emit the Event with the LogObject as SourceNode. The server lock is taken by
 * the public API. */
static void
emitOverflowEvent(LogObjectDriver *lod, LogObjectEntry *entry, uintptr_t discarded) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    UA_String sourceName = UA_STRING("LogObject/Overflow");
    UA_KeyValuePair field;
    field.key = UA_QUALIFIEDNAME(0, "/SourceName");
    UA_Variant_setScalar(&field.value, &sourceName, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap fields = {1, &field};

    /* The LogOverflowEventType has no field for the number of discarded
     * records. It is reported in the Message. */
    char buf[96];
    UA_String text = {sizeof(buf), (UA_Byte*)buf};
    if(UA_String_format(&text, "LogObject discarded %lu records (MaxRecords reached)",
                        (unsigned long)discarded) != UA_STATUSCODE_GOOD)
        text = UA_STRING("LogObject discarded records (MaxRecords reached)");

    UA_EventDescription ed;
    memset(&ed, 0, sizeof(UA_EventDescription));
    ed.sourceNode = entry->nodeId;
    ed.eventType = UA_NS0ID(LOGOVERFLOWEVENTTYPE);
    ed.severity = 500;
    ed.message.text = text;
    ed.eventFields = &fields;
    /* A failure is not logged: the message would be captured into the
     * ServerLog and could trigger the next overflow */
    (void)UA_Server_createEventEx(lod->driver.server, &ed, NULL);
#else
    (void)lod;
    (void)entry;
    (void)discarded;
#endif
}

static void overflowTimerCallback(void *application, void *context);

/* Emit the due Events. Runs in the EventLoop with the driver lock. */
static void
processOverflowEvents(LogObjectDriver *lod) {
    if(lod->driver.state != UA_LIFECYCLESTATE_STARTED)
        return;
    UA_EventLoop *el = UA_Server_getConfig(lod->driver.server)->eventLoop;
    UA_DateTime now = el->dateTime_nowMonotonic(el);
    UA_DateTime interval = (UA_DateTime)(lod->overflowEventInterval * UA_DATETIME_MSEC);
    UA_DateTime nextDue = UA_INT64_MAX;
    LogObjectEntry *entry;
    LIST_FOREACH(entry, &lod->logObjects, pointers) {
        if(UA_atomic_load(&entry->discarded) == 0)
            continue;
        if(entry->overflowReported && now - entry->lastOverflowEvent < interval) {
            UA_DateTime due = entry->lastOverflowEvent + interval;
            if(due < nextDue)
                nextDue = due;
            continue;
        }
        uintptr_t discarded = 0;
        UA_atomic_xchg(&entry->discarded, (uintptr_t)0, &discarded);
        entry->overflowReported = true;
        entry->lastOverflowEvent = now;
        emitOverflowEvent(lod, entry, discarded);
    }

    /* Wake up when the next coalesced Event is due. A timer that is armed
     * for a later time is replaced. */
    if(nextDue == UA_INT64_MAX)
        return;
    if(UA_atomic_load(&lod->overflowTimerArmed)) {
        if(lod->overflowTimerDue <= nextDue)
            return;
        el->removeTimer(el, lod->overflowTimerId);
        UA_atomic_store(&lod->overflowTimerArmed, (uintptr_t)0);
    }
    UA_Double delay = (UA_Double)(nextDue - now) / UA_DATETIME_MSEC;
    UA_StatusCode res = el->addTimer(el, overflowTimerCallback, lod, NULL, delay, NULL,
                                     UA_TIMERPOLICY_ONCE, &lod->overflowTimerId);
    if(res == UA_STATUSCODE_GOOD) {
        lod->overflowTimerDue = nextDue;
        UA_atomic_store(&lod->overflowTimerArmed, (uintptr_t)1);
    }
}

static void
overflowDelayedCallback(void *application, void *context) {
    LogObjectDriver *lod = (LogObjectDriver*)application;
    UA_EventLoop *el = UA_Server_getConfig(lod->driver.server)->eventLoop;
    el->lock(el);
    UA_atomic_store(&lod->overflowCallbackQueued, (uintptr_t)0);
    processOverflowEvents(lod);
    el->unlock(el);
}

/* The one-shot timer is removed by the EventLoop after it fired */
static void
overflowTimerCallback(void *application, void *context) {
    LogObjectDriver *lod = (LogObjectDriver*)application;
    UA_EventLoop *el = UA_Server_getConfig(lod->driver.server)->eventLoop;
    el->lock(el);
    UA_atomic_store(&lod->overflowTimerArmed, (uintptr_t)0);
    processOverflowEvents(lod);
    el->unlock(el);
}

/* Counts the discarded record. The first record discarded since the last
 * Event of the LogObject arms the delayed callback. The following ones are
 * reported with it or, within the interval, by the timer. The delayed callback
 * clears its flag before it reads the counts and the timer stays armed while a
 * count is pending. So no overflow is lost. */
void
scheduleOverflowEvent(LogObjectDriver *lod, LogObjectEntry *entry) {
    if(logObjectAtomicAdd(&entry->discarded, 1) > 0)
        return; /* Already pending */
    uintptr_t queued = 0;
    UA_atomic_cmpxchg(&lod->overflowCallbackQueued, &queued, (uintptr_t)1);
    if(queued != 0)
        return; /* Already armed */
    UA_EventLoop *el = UA_Server_getConfig(lod->driver.server)->eventLoop;
    lod->overflowCallback.callback = overflowDelayedCallback;
    lod->overflowCallback.application = lod;
    lod->overflowCallback.context = NULL;
    el->addDelayedCallback(el, &lod->overflowCallback);
}

void
cancelOverflowEvents(LogObjectDriver *lod) {
    UA_EventLoop *el = UA_Server_getConfig(lod->driver.server)->eventLoop;
    uintptr_t queued = 1;
    UA_atomic_cmpxchg(&lod->overflowCallbackQueued, &queued, (uintptr_t)0);
    if(queued == 1)
        el->removeDelayedCallback(el, &lod->overflowCallback);
    uintptr_t armed = 1;
    UA_atomic_cmpxchg(&lod->overflowTimerArmed, &armed, (uintptr_t)0);
    if(armed == 1)
        el->removeTimer(el, lod->overflowTimerId);
}

#endif /* UA_ENABLE_DRIVER_LOGOBJECT */
