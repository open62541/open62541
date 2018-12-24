/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017, 2018 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_TIMER_H_
#define UA_TIMER_H_

#include "ua_util_internal.h"
#include "ua_workqueue.h"
#include "ziptree.h"

_UA_BEGIN_DECLS

struct UA_TimerEntry;
typedef struct UA_TimerEntry UA_TimerEntry;

ZIP_HEAD(UA_TimerZip, UA_TimerEntry);
typedef struct UA_TimerZip UA_TimerZip;

ZIP_HEAD(UA_TimerIdZip, UA_TimerEntry);
typedef struct UA_TimerIdZip UA_TimerIdZip;

/* Only for a single thread. Protect by a mutex if required. */
typedef struct {
    UA_TimerZip root; /* The root of the time-sorted zip tree */
    UA_TimerIdZip idRoot; /* The root of the id-sorted zip tree */
    UA_UInt64 idCounter;
} UA_Timer;

void UA_Timer_init(UA_Timer *t);

UA_StatusCode
UA_Timer_addTimedCallback(UA_Timer *t, UA_ApplicationCallback callback,
                          void *application, void *data, UA_DateTime date,
                          UA_UInt64 *callbackId);

UA_StatusCode
UA_Timer_addRepeatedCallback(UA_Timer *t, UA_ApplicationCallback callback,
                             void *application, void *data, UA_Double interval_ms,
                             UA_UInt64 *callbackId);

/* Change the callback interval. If this is called from within the callback. The
 * adjustment is made during the next _process call. */
UA_StatusCode
UA_Timer_changeRepeatedCallbackInterval(UA_Timer *t, UA_UInt64 callbackId,
                                        UA_Double interval_ms);

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

void UA_Timer_deleteMembers(UA_Timer *t);

_UA_END_DECLS

#endif /* UA_TIMER_H_ */
