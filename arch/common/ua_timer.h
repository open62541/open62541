/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017, 2018, 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_TIMER_H_
#define UA_TIMER_H_

#include <open62541/types.h>
#include <open62541/util.h>
#include "aa_tree.h"

_UA_BEGIN_DECLS

/* The timer is protected by its own mutex. The mutex is released before calling
 * into the callbacks. So the timer can be modified from the callbacks it is
 * executing. Also, the timer mutex can never lead to locking. Because the timer
 * mutex will be left without acquiring another mutex.
 *
 * Obviously, the timer must not be deleted from within one of its
 * callbacks. */

/* Callback where the application is either a client or a server */
typedef void (*UA_ApplicationCallback)(void *application, void *data);

typedef struct UA_TimerEntry {
    struct aa_entry treeEntry;
    UA_TimerPolicy timerPolicy;              /* Timer policy to handle cycle misses */
    UA_DateTime nextTime;                    /* The next time when the callback
                                              * is to be executed */
    UA_UInt64 interval;                      /* Interval in 100ns resolution. If
                                                the interval is zero, the
                                                callback is not repeated and
                                                removed after execution. */
    UA_ApplicationCallback callback;
    void *application;
    void *data;

    struct aa_entry idTreeEntry;
    UA_UInt64 id;                            /* Id of the entry */
} UA_TimerEntry;

typedef struct {
    struct aa_head root;   /* The root of the time-sorted tree */
    struct aa_head idRoot; /* The root of the id-sorted tree */
    UA_UInt64 idCounter;   /* Generate unique identifiers. Identifiers are
                            * always above zero. */
#if UA_MULTITHREADING >= 100
    UA_Lock timerMutex;
#endif
} UA_Timer;

void
UA_Timer_init(UA_Timer *t);

UA_DateTime
UA_Timer_nextRepeatedTime(UA_Timer *t);

UA_StatusCode
UA_Timer_addTimedCallback(UA_Timer *t, UA_ApplicationCallback callback,
                          void *application, void *data, UA_DateTime date,
                          UA_UInt64 *callbackId);

/* Add a pre-allocated and pre-filled UA_TimerEntry. This cannot fail. It is
 * used, for example, for delayed memory reclamation where the data structure
 * begins with a UA_TimerEntry. */
void
UA_Timer_addTimerEntry(UA_Timer *t, UA_TimerEntry *te, UA_UInt64 *callbackId);

UA_StatusCode
UA_Timer_addRepeatedCallback(UA_Timer *t, UA_ApplicationCallback callback,
                             void *application, void *data, UA_Double interval_ms,
                             UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                             UA_UInt64 *callbackId);

UA_StatusCode
UA_Timer_changeRepeatedCallback(UA_Timer *t, UA_UInt64 callbackId,
                                UA_Double interval_ms, UA_DateTime *baseTime,
                                UA_TimerPolicy timerPolicy);

void
UA_Timer_removeCallback(UA_Timer *t, UA_UInt64 callbackId);

/* Process (dispatch) the repeated callbacks that have timed out. Returns the
 * timestamp of the next scheduled repeated callback. Not thread-safe.
 * Application is a pointer to the client / server environment for the callback.
 * Dispatched is set to true when at least one callback was run / dispatched. */
typedef void
(*UA_TimerExecutionCallback)(void *executionApplication, UA_ApplicationCallback cb,
                             void *callbackApplication, void *data);

UA_DateTime
UA_Timer_process(UA_Timer *t, UA_DateTime nowMonotonic,
                 UA_TimerExecutionCallback executionCallback,
                 void *executionApplication);

void
UA_Timer_clear(UA_Timer *t);

_UA_END_DECLS

#endif /* UA_TIMER_H_ */
