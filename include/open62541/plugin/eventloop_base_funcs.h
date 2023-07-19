/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_EVENTLOOP_BASE_FUNCS_H_
#define UA_EVENTLOOP_BASE_FUNCS_H_

#include <open62541/plugin/eventloop.h>

_UA_BEGIN_DECLS

/**
 * Event Loop base functions
 * =========================
 *
 * This header provides base functions for generic EventLoop handling for
 * the type UA_EventLoop, which can be used in the EventLoop callback
 * functions for EventLoops that are developed by users.
 */

/* User callback for EventLoop specific code
 * If the provided status code is not equal to UA_STATUSCODE_GOOD, the
 * calling function returns immediately with this return code. */
typedef UA_StatusCode (*UA_EventLoopCallback)(UA_EventLoop *el, void *user_arg);

/**
 * Event Loop
 * ----------
 * The EventLoop implementation is part of the selected architecture. For
 * example, "Win32/POSIX" stands for a Windows environment with an EventLoop
 * that uses the POSIX API. Several EventLoops can be instantiated in parallel.
 * But the globally defined functions are the same everywhere. */

/*****************************/
/* EventLoop handling        */
/*****************************/

/* Start the EventLoop and start all already registered EventSources
 * The start_callback is called with the user argument after checking the
 * state of the EventLoop.
 * The start_callback is called with locked EventLoop mutex. */
UA_StatusCode
UA_EventLoop_start(UA_EventLoop *el,
                   UA_EventLoopCallback start_callback,
                   void *user_arg);

/* Stop all EventSources. This is asynchronous and might need a few
 * iterations of the main-loop to succeed.
 * The stop_callback is called with the user argument before setting the
 * state of the EventLoop to stopped.
 * The stop_callback is called with locked EventLoop mutex.
 */
UA_StatusCode
UA_EventLoop_stop(UA_EventLoop *el,
                  UA_EventLoopCallback stop_callback,
                  void *user_arg);

/* Check if all EventSources are stopped. This is asynchronous and might
 * need a few iterations of the main-loop to succeed.
 * The check_stopped_callback is called with the user argument after
 * checking the state of the EventLoop before stopping the EventSources.
 * The check_stopped_callback is called with locked EventLoop mutex.
 *
 * The stopCallback is called with the user argument after successful
 * checking of the stopped state and setting the state of the EventLoop
 * to stopped.
 * The stopCallback is called with locked EventLoop mutex.
 *
 * The return code is either UA_STATUSCODE_GOOD, UA_STATUSCODE_GOODCALLAGAIN
 * or UA_STATUSCODE_BADINVALIDSTATE (or any return code from the user
 * callbacks).
 */
UA_StatusCode
UA_EventLoop_check_stopped(UA_EventLoop *el,
                           UA_EventLoopCallback check_stopped_callback,
                           UA_EventLoopCallback stop_callback,
                           void *user_arg);

/* Clean up the EventLoop and free allocated memory. Can fail if the
 * EventLoop is not stopped.
 * The free_callback is called with the user argument after deleting
 * the event sources.
 * The free_callback is called with locked EventLoop mutex. */
UA_StatusCode
UA_EventLoop_free(UA_EventLoop *el,
                  UA_EventLoopCallback free_callback,
                  void *user_arg);

/*****************************/
/* Registering Event Sources */
/*****************************/

/* Register the ES. Immediately starts the ES if the EventLoop is already
 * started. Otherwise the ES is started together with the EventLoop. */
UA_StatusCode
UA_EventLoop_registerEventSource(UA_EventLoop *el,
                                 UA_EventSource *es);

/* Stops the EventSource before deregistering it */
UA_StatusCode
UA_EventLoop_deregisterEventSource(UA_EventLoop *el,
                                   UA_EventSource *es);

_UA_END_DECLS

#endif /* UA_EVENTLOOP_BASE_FUNCS_H_ */
