/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_posix.h"
#include "open62541/plugin/eventloop.h"

/*********/
/* Timer */
/*********/

static void
timerExecutionTrampoline(void *executionApplication,
                         UA_ApplicationCallback cb,
                         void *callbackApplication,
                         void *data) {
    cb(callbackApplication, data);
}

static UA_DateTime
UA_EventLoopPOSIX_nextCyclicTime(UA_EventLoop *public_el) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    return UA_Timer_nextRepeatedTime(&el->timer);
}

static UA_StatusCode
UA_EventLoopPOSIX_addTimedCallback(UA_EventLoop *public_el,
                                   UA_Callback callback,
                                   void *application, void *data,
                                   UA_DateTime date,
                                   UA_UInt64 *callbackId) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    return UA_Timer_addTimedCallback(&el->timer, callback, application,
                                     data, date, callbackId);
}

static UA_StatusCode
UA_EventLoopPOSIX_addCyclicCallback(UA_EventLoop *public_el,
                                    UA_Callback cb,
                                    void *application, void *data,
                                    UA_Double interval_ms,
                                    UA_DateTime *baseTime,
                                    UA_TimerPolicy timerPolicy,
                                    UA_UInt64 *callbackId) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    return UA_Timer_addRepeatedCallback(&el->timer, cb, application,
                                        data, interval_ms, baseTime,
                                        timerPolicy, callbackId);
}

static UA_StatusCode
UA_EventLoopPOSIX_modifyCyclicCallback(UA_EventLoop *public_el,
                                       UA_UInt64 callbackId,
                                       UA_Double interval_ms,
                                       UA_DateTime *baseTime,
                                       UA_TimerPolicy timerPolicy) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    return UA_Timer_changeRepeatedCallback(&el->timer, callbackId,
                                           interval_ms, baseTime,
                                           timerPolicy);
}

static void
UA_EventLoopPOSIX_removeCyclicCallback(UA_EventLoop *public_el,
                                       UA_UInt64 callbackId) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    UA_Timer_removeCallback(&el->timer, callbackId);
}

static void
UA_EventLoopPOSIX_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    UA_LOCK(&el->elMutex);
    dc->next = el->delayedCallbacks;
    el->delayedCallbacks = dc;
    UA_UNLOCK(&el->elMutex);
}

/* Process and then free registered delayed callbacks */
static void
processDelayed(UA_EventLoopPOSIX *el) {
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Process delayed callbacks");

    UA_LOCK_ASSERT(&el->elMutex, 1);
    while(el->delayedCallbacks) {
        UA_DelayedCallback *dc = el->delayedCallbacks;
        el->delayedCallbacks = dc->next;
        /* Delayed Callbacks might have no cb pointer if all
         * we want to do is free the memory */
        if(dc->callback) {
            UA_UNLOCK(&el->elMutex);
            dc->callback(dc->application, dc->context);
            UA_LOCK(&el->elMutex);
        }
        UA_free(dc);
    }
}

/***********************/
/* EventLoop Lifecycle */
/***********************/

static UA_StatusCode
UA_EventLoopPOSIX_start(UA_EventLoopPOSIX *el) {
    UA_LOCK(&el->elMutex);
    if(el->eventLoop.state != UA_EVENTLOOPSTATE_FRESH &&
       el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "Starting the EventLoop");

#ifdef UA_HAVE_EPOLL
    el->epollfd = epoll_create1(0);
    if(el->epollfd == -1) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "Eventloop\t| Could not create the epoll socket (%s)",
                          errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
#endif

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_EventSource *es = el->eventLoop.eventSources;
    while(es) {
        UA_UNLOCK(&el->elMutex);
        res |= es->start(es);
        UA_LOCK(&el->elMutex);
        es = es->next;
    }

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STARTED;

    UA_UNLOCK(&el->elMutex);
    return res;
}

static void
checkClosed(UA_EventLoopPOSIX *el) {
    UA_EventSource *es = el->eventLoop.eventSources;
    while(es) {
        if(es->state != UA_EVENTSOURCESTATE_STOPPED)
            return;
        es = es->next;
    }

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPED;

    /* Close the epoll/IOCP socket once all EventSources have shut down */
#ifdef UA_HAVE_EPOLL
    close(el->epollfd);
#endif

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "The EventLoop has stopped");
}

static void
UA_EventLoopPOSIX_stop(UA_EventLoopPOSIX *el) {
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_STARTED) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "The EventLoop is not running, cannot be stopped");
        UA_UNLOCK(&el->elMutex);
        return;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "Stopping the EventLoop");

    /* Shutdown all event sources. This closes open connections. */
    UA_EventSource *es = el->eventLoop.eventSources;
    while(es) {
        if(es->state == UA_EVENTSOURCESTATE_STARTING ||
           es->state == UA_EVENTSOURCESTATE_STARTED) {
            UA_UNLOCK(&el->elMutex);
            es->stop(es);
            UA_LOCK(&el->elMutex);
        }
        es = es->next;
    }

    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPING;
    checkClosed(el);

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
UA_EventLoopPOSIX_run(UA_EventLoopPOSIX *el, UA_UInt32 timeout) {
    UA_LOCK(&el->elMutex);

    if(el->executing) {
        UA_LOG_ERROR(el->eventLoop.logger,
                     UA_LOGCATEGORY_EVENTLOOP,
                     "Cannot run EventLoop from the run method itself");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    el->executing = true;

    if(el->eventLoop.state == UA_EVENTLOOPSTATE_FRESH ||
       el->eventLoop.state == UA_EVENTLOOPSTATE_STOPPED) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot iterate a stopped EventLoop");
        el->executing = false;
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Iterate the EventLoop");

    /* Process cyclic callbacks */
    UA_DateTime dateBefore =
        el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);

    UA_UNLOCK(&el->elMutex);
    UA_DateTime dateNext =
        UA_Timer_process(&el->timer, dateBefore,
                         timerExecutionTrampoline, NULL);
    UA_LOCK(&el->elMutex);

    processDelayed(el); /* Process delayed callbacks. Remove closed sockets
                         * already here instead of calling select for them. */

    /* Compute the remaining time */
    UA_DateTime maxDate = dateBefore + (timeout * UA_DATETIME_MSEC);
    if(dateNext > maxDate)
        dateNext = maxDate;
    UA_DateTime listenTimeout =
        dateNext - el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    if(listenTimeout < 0)
        listenTimeout = 0;

    /* Listen on the active file-descriptors (sockets) from the
     * ConnectionManagers */
    UA_StatusCode rv = UA_EventLoopPOSIX_pollFDs(el, listenTimeout);

    /* Check if the last EventSource was successfully stopped */
    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STOPPING)
        checkClosed(el);

    el->executing = false;
    UA_UNLOCK(&el->elMutex);
    return rv;
}

/*****************************/
/* Registering Event Sources */
/*****************************/

static UA_StatusCode
UA_EventLoopPOSIX_registerEventSource(UA_EventLoopPOSIX *el,
                                      UA_EventSource *es) {
    /* Already registered? */
    if(es->state != UA_EVENTSOURCESTATE_FRESH) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "Cannot register the EventSource \"%.*s\": "
                     "already registered",
                     (int)es->name.length, (char*)es->name.data);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add to linked list */
    UA_LOCK(&el->elMutex);
    es->next = el->eventLoop.eventSources;
    el->eventLoop.eventSources = es;
    UA_UNLOCK(&el->elMutex);

    es->eventLoop = &el->eventLoop;
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    /* Start if the entire EventLoop is started */
    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STARTED)
        return es->start(es);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_EventLoopPOSIX_deregisterEventSource(UA_EventLoopPOSIX *el,
                                        UA_EventSource *es) {
    if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot deregister the EventSource %.*s: "
                       "Has to be stopped first",
                       (int)es->name.length, es->name.data);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Remove from the linked list */
    UA_LOCK(&el->elMutex);
    UA_EventSource **s = &el->eventLoop.eventSources;
    while(*s) {
        if(*s == es) {
            *s = es->next;
            break;
        }
        s = &(*s)->next;
    }
    UA_UNLOCK(&el->elMutex);

    /* Set the state to non-registered */
    es->state = UA_EVENTSOURCESTATE_FRESH;

    return UA_STATUSCODE_GOOD;
}

/***************/
/* Time Domain */
/***************/

/* No special synchronization with an external source, just use the globally
 * defined functions. */

static UA_DateTime
UA_EventLoopPOSIX_DateTime_now(UA_EventLoop *el) {
    return UA_DateTime_now();
}

static UA_DateTime
UA_EventLoopPOSIX_DateTime_nowMonotonic(UA_EventLoop *el) {
    return UA_DateTime_nowMonotonic();
}

static UA_Int64
UA_EventLoopPOSIX_DateTime_localTimeUtcOffset(UA_EventLoop *el) {
    return UA_DateTime_localTimeUtcOffset();
}

/*************************/
/* Initialize and Delete */
/*************************/

static UA_StatusCode
UA_EventLoopPOSIX_free(UA_EventLoopPOSIX *el) {
    UA_LOCK(&el->elMutex);

    /* Check if the EventLoop can be deleted */
    if(el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPED &&
       el->eventLoop.state != UA_EVENTLOOPSTATE_FRESH) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot delete a running EventLoop");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Deregister and delete all the EventSources */
    while(el->eventLoop.eventSources) {
        UA_EventSource *es = el->eventLoop.eventSources;
        UA_UNLOCK(&el->elMutex);
        UA_EventLoopPOSIX_deregisterEventSource(el, es);
        UA_LOCK(&el->elMutex);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&el->timer);

    /* Process remaining delayed callbacks */
    processDelayed(el);

#ifdef _WIN32
    /* Stop the Windows networking subsystem */
    WSACleanup();
#endif

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

UA_EventLoop *
UA_EventLoop_new_POSIX(const UA_Logger *logger) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)
        UA_calloc(1, sizeof(UA_EventLoopPOSIX));
    if(!el)
        return NULL;

    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);

#ifdef _WIN32
    /* Start the WSA networking subsystem on Windows */
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    /* Set the public EventLoop content */
    el->eventLoop.logger = logger;

    el->eventLoop.start = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopPOSIX_start;
    el->eventLoop.stop = (void (*)(UA_EventLoop*))UA_EventLoopPOSIX_stop;
    el->eventLoop.run = (UA_StatusCode (*)(UA_EventLoop*, UA_UInt32))UA_EventLoopPOSIX_run;
    el->eventLoop.free = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopPOSIX_free;

    el->eventLoop.dateTime_now = UA_EventLoopPOSIX_DateTime_now;
    el->eventLoop.dateTime_nowMonotonic =
        UA_EventLoopPOSIX_DateTime_nowMonotonic;
    el->eventLoop.dateTime_localTimeUtcOffset =
        UA_EventLoopPOSIX_DateTime_localTimeUtcOffset;

    el->eventLoop.nextCyclicTime = UA_EventLoopPOSIX_nextCyclicTime;
    el->eventLoop.addCyclicCallback = UA_EventLoopPOSIX_addCyclicCallback;
    el->eventLoop.modifyCyclicCallback = UA_EventLoopPOSIX_modifyCyclicCallback;
    el->eventLoop.removeCyclicCallback = UA_EventLoopPOSIX_removeCyclicCallback;
    el->eventLoop.addTimedCallback = UA_EventLoopPOSIX_addTimedCallback;
    el->eventLoop.addDelayedCallback = UA_EventLoopPOSIX_addDelayedCallback;

    el->eventLoop.registerEventSource =
        (UA_StatusCode (*)(UA_EventLoop*, UA_EventSource*))
        UA_EventLoopPOSIX_registerEventSource;
    el->eventLoop.deregisterEventSource =
        (UA_StatusCode (*)(UA_EventLoop*, UA_EventSource*))
        UA_EventLoopPOSIX_deregisterEventSource;

    return &el->eventLoop;
}
