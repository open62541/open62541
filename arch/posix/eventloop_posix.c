/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_posix.h"
#include "open62541/plugin/eventloop.h"

#if defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32)

#if defined(UA_ARCHITECTURE_POSIX) && !defined(__APPLE__) && !defined(__MACH__)
#include <time.h>
#endif

/*********/
/* Timer */
/*********/

static UA_DateTime
UA_EventLoopPOSIX_nextTimer(UA_EventLoop *public_el) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    return UA_Timer_next(&el->timer);
}

static UA_StatusCode
UA_EventLoopPOSIX_addTimer(UA_EventLoop *public_el,
                                    UA_Callback cb,
                                    void *application, void *data,
                                    UA_Double interval_ms,
                                    UA_DateTime *baseTime,
                                    UA_TimerPolicy timerPolicy,
                                    UA_UInt64 *callbackId) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    return UA_Timer_add(&el->timer, cb, application, data, interval_ms,
                        public_el->dateTime_nowMonotonic(public_el),
                        baseTime, timerPolicy, callbackId);
}

static UA_StatusCode
UA_EventLoopPOSIX_modifyTimer(UA_EventLoop *public_el,
                              UA_UInt64 callbackId,
                              UA_Double interval_ms,
                              UA_DateTime *baseTime,
                              UA_TimerPolicy timerPolicy) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    return UA_Timer_modify(&el->timer, callbackId, interval_ms,
                           public_el->dateTime_nowMonotonic(public_el),
                           baseTime, timerPolicy);
}

static void
UA_EventLoopPOSIX_removeTimer(UA_EventLoop *public_el,
                              UA_UInt64 callbackId) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    UA_Timer_remove(&el->timer, callbackId);
}

void
UA_EventLoopPOSIX_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    dc->next = NULL;

    /* el->delayedTail points either to prev->next or to the head.
     * We need to update two locations:
     * 1: el->delayedTail = &dc->next;
     * 2: *oldtail = dc; (equal to &dc->next)
     *
     * Once we have (1), we "own" the previous-to-last entry. No need to worry
     * about (2), we can adjust it with a delay. This makes the queue
     * "eventually consistent". */
    UA_DelayedCallback **oldtail = (UA_DelayedCallback**)
        UA_atomic_xchg((void**)&el->delayedTail, &dc->next);
    UA_atomic_xchg((void**)oldtail, &dc->next);
}

/* Resets the delayed queue and returns the previous head and tail */
static void
resetDelayedQueue(UA_EventLoopPOSIX *el, UA_DelayedCallback **oldHead,
                  UA_DelayedCallback **oldTail) {
    if(el->delayedHead1 <= (UA_DelayedCallback *)0x01 &&
       el->delayedHead2 <= (UA_DelayedCallback *)0x01)
        return; /* The queue is empty */

    UA_Boolean active1 = (el->delayedHead1 != (UA_DelayedCallback*)0x01);
    UA_DelayedCallback **activeHead = (active1) ? &el->delayedHead1 : &el->delayedHead2;
    UA_DelayedCallback **inactiveHead = (active1) ? &el->delayedHead2 : &el->delayedHead1;

    /* Switch active/inactive by resetting the sentinel values. The (old) active
     * head points to an element which we return. Parallel threads continue to
     * add elements to the queue "below" the first element. */
    UA_atomic_xchg((void**)inactiveHead, NULL);
    *oldHead = (UA_DelayedCallback *)
        UA_atomic_xchg((void**)activeHead, (void*)0x01);

    /* Make the tail point to the (new) active head. Return the value of last
     * tail. When iterating over the queue elements, we need to find this tail
     * as the last element. If we find a NULL next-pointer before hitting the
     * tail spinlock until the pointer updates (eventually consistent). */
    *oldTail = (UA_DelayedCallback*)
        UA_atomic_xchg((void**)&el->delayedTail, inactiveHead);
}

static void
UA_EventLoopPOSIX_removeDelayedCallback(UA_EventLoop *public_el,
                                        UA_DelayedCallback *dc) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    UA_LOCK(&el->elMutex);

    /* Reset and get the old head and tail */
    UA_DelayedCallback *cur = NULL, *tail = NULL;
    resetDelayedQueue(el, &cur, &tail);

    /* Loop until we reach the tail (or head and tail are both NULL) */
    UA_DelayedCallback *next;
    for(; cur; cur = next) {
        /* Spin-loop until the next-pointer of cur is updated.
         * The element pointed to by tail must appear eventually. */
        next = cur->next;
        while(!next && cur != tail)
            next = (UA_DelayedCallback *)UA_atomic_load((void**)&cur->next);
        if(cur == dc)
            continue;
        UA_EventLoopPOSIX_addDelayedCallback(public_el, cur);
    }

    UA_UNLOCK(&el->elMutex);
}

static void
processDelayed(UA_EventLoopPOSIX *el) {
    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Process delayed callbacks");

    UA_LOCK_ASSERT(&el->elMutex);

    /* Reset and get the old head and tail */
    UA_DelayedCallback *dc = NULL, *tail = NULL;
    resetDelayedQueue(el, &dc, &tail);

    /* Loop until we reach the tail (or head and tail are both NULL) */
    UA_DelayedCallback *next;
    for(; dc; dc = next) {
        next = dc->next;
        while(!next && dc != tail)
            next = (UA_DelayedCallback *)UA_atomic_load((void**)&dc->next);
        if(!dc->callback)
            continue;
        dc->callback(dc->application, dc->context);
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

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Starting the EventLoop");

    /* Setting the clock source */
    const UA_Int32 *cs = (const UA_Int32*)
        UA_KeyValueMap_getScalar(&el->eventLoop.params,
                                 UA_QUALIFIEDNAME(0, "clock-source"),
                                 &UA_TYPES[UA_TYPES_INT32]);
    const UA_Int32 *csm = (const UA_Int32*)
        UA_KeyValueMap_getScalar(&el->eventLoop.params,
                                 UA_QUALIFIEDNAME(0, "clock-source-monotonic"),
                                 &UA_TYPES[UA_TYPES_INT32]);
#if defined(UA_ARCHITECTURE_POSIX) && !defined(__APPLE__) && !defined(__MACH__)
    el->clockSource = CLOCK_REALTIME;
    if(cs)
        el->clockSource = *cs;

# ifdef CLOCK_MONOTONIC_RAW
    el->clockSourceMonotonic = CLOCK_MONOTONIC_RAW;
# else
    el->clockSourceMonotonic = CLOCK_MONOTONIC;
# endif
    if(csm)
        el->clockSourceMonotonic = *csm;
#else
    if(cs || csm) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "Eventloop\t| Cannot set a custom clock source");
    }
#endif

    /* Create the self-pipe */
    int err = UA_EventLoopPOSIX_pipe(el->selfpipe);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "Eventloop\t| Could not create the self-pipe (%s)",
                          errno_str));
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create the epoll socket */
#ifdef UA_HAVE_EPOLL
    el->epollfd = epoll_create1(0);
    if(el->epollfd == -1) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "Eventloop\t| Could not create the epoll socket (%s)",
                          errno_str));
        UA_close(el->selfpipe[0]);
        UA_close(el->selfpipe[1]);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* epoll always listens on the self-pipe. This is the only epoll_event that
     * has a NULL data pointer. */
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.events = EPOLLIN;
    err = epoll_ctl(el->epollfd, EPOLL_CTL_ADD, el->selfpipe[0], &event);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "Eventloop\t| Could not register the self-pipe for epoll (%s)",
                          errno_str));
        UA_close(el->selfpipe[0]);
        UA_close(el->selfpipe[1]);
        close(el->epollfd);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
#endif

    /* Start the EventSources */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_EventSource *es = el->eventLoop.eventSources;
    while(es) {
        res |= es->start(es);
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
    UA_LOCK_ASSERT(&el->elMutex);

    UA_EventSource *es = el->eventLoop.eventSources;
    while(es) {
        if(es->state != UA_EVENTSOURCESTATE_STOPPED)
            return;
        es = es->next;
    }

    /* Not closed until all delayed callbacks are processed */
    if(el->delayedHead1 != NULL && el->delayedHead2 != NULL)
        return;

    /* Close the self-pipe when everything else is done */
    UA_close(el->selfpipe[0]);
    UA_close(el->selfpipe[1]);

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPED;

    /* Close the epoll/IOCP socket once all EventSources have shut down */
#ifdef UA_HAVE_EPOLL
    UA_close(el->epollfd);
#endif

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
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

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Stopping the EventLoop");

    /* Set to STOPPING to prevent "normal use" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPING;

    /* Stop all event sources (asynchronous) */
    UA_EventSource *es = el->eventLoop.eventSources;
    for(; es; es = es->next) {
        if(es->state == UA_EVENTSOURCESTATE_STARTING ||
           es->state == UA_EVENTSOURCESTATE_STARTED) {
            es->stop(es);
        }
    }

    /* Set to STOPPED if all EventSources are STOPPED */
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
                       "Cannot run a stopped EventLoop");
        el->executing = false;
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Iterate the EventLoop");

    /* Process cyclic callbacks */
    UA_DateTime dateBefore =
        el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);

    UA_DateTime dateNext = UA_Timer_process(&el->timer, dateBefore);

    /* Process delayed callbacks here:
     * - Removes closed sockets already here instead of polling them again.
     * - The timeout for polling is selected to be ready in time for the next
     *   cyclic callback. So we want to do little work between the timeout
     *   running out and executing the due cyclic callbacks. */
    processDelayed(el);

    /* A delayed callback could create another delayed callback (or re-add
     * itself). In that case we don't want to wait (indefinitely) for an event
     * to happen. Process queued events but don't sleep. Then process the
     * delayed callbacks in the next iteration. */
    if(el->delayedHead1 != NULL && el->delayedHead2 != NULL)
        timeout = 0;

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
    UA_LOCK(&el->elMutex);

    /* Already registered? */
    if(es->state != UA_EVENTSOURCESTATE_FRESH) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "Cannot register the EventSource \"%.*s\": "
                     "already registered",
                     (int)es->name.length, (char*)es->name.data);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add to linked list */
    es->next = el->eventLoop.eventSources;
    el->eventLoop.eventSources = es;

    es->eventLoop = &el->eventLoop;
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    /* Start if the entire EventLoop is started */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STARTED)
        res = es->start(es);

    UA_UNLOCK(&el->elMutex);
    return res;
}

static UA_StatusCode
UA_EventLoopPOSIX_deregisterEventSource(UA_EventLoopPOSIX *el,
                                        UA_EventSource *es) {
    UA_LOCK(&el->elMutex);

    if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot deregister the EventSource %.*s: "
                       "Has to be stopped first",
                       (int)es->name.length, es->name.data);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Remove from the linked list */
    UA_EventSource **s = &el->eventLoop.eventSources;
    while(*s) {
        if(*s == es) {
            *s = es->next;
            break;
        }
        s = &(*s)->next;
    }

    /* Set the state to non-registered */
    es->state = UA_EVENTSOURCESTATE_FRESH;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

/***************/
/* Time Domain */
/***************/

static UA_DateTime
UA_EventLoopPOSIX_DateTime_now(UA_EventLoop *el) {
#if defined(UA_ARCHITECTURE_POSIX) && !defined(__APPLE__) && !defined(__MACH__)
    UA_EventLoopPOSIX *pel = (UA_EventLoopPOSIX*)el;
    struct timespec ts;
    int res = clock_gettime(pel->clockSource, &ts);
    if(UA_UNLIKELY(res != 0))
        return 0;
    return (ts.tv_sec * UA_DATETIME_SEC) + (ts.tv_nsec / 100) + UA_DATETIME_UNIX_EPOCH;
#else
    return UA_DateTime_now();
#endif
}

static UA_DateTime
UA_EventLoopPOSIX_DateTime_nowMonotonic(UA_EventLoop *el) {
#if defined(UA_ARCHITECTURE_POSIX) && !defined(__APPLE__) && !defined(__MACH__)
    UA_EventLoopPOSIX *pel = (UA_EventLoopPOSIX*)el;
    struct timespec ts;
    int res = clock_gettime(pel->clockSourceMonotonic, &ts);
    if(UA_UNLIKELY(res != 0))
        return 0;
    /* Also add the unix epoch for the monotonic clock. So we get a "normal"
     * output when a "normal" source is configured. */
    return (ts.tv_sec * UA_DATETIME_SEC) + (ts.tv_nsec / 100) + UA_DATETIME_UNIX_EPOCH;
#else
    return UA_DateTime_nowMonotonic();
#endif
}

static UA_Int64
UA_EventLoopPOSIX_DateTime_localTimeUtcOffset(UA_EventLoop *el) {
    /* TODO: Fix for custom clock sources */
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
        UA_EventLoopPOSIX_deregisterEventSource(el, es);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&el->timer);

    /* Process remaining delayed callbacks */
    processDelayed(el);

#ifdef UA_ARCHITECTURE_WIN32
    /* Stop the Windows networking subsystem */
    WSACleanup();
#endif

    UA_KeyValueMap_clear(&el->eventLoop.params);

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

static void
UA_EventLoopPOSIX_lock(UA_EventLoop *public_el) {
    UA_LOCK(&((UA_EventLoopPOSIX*)public_el)->elMutex);
}
static void
UA_EventLoopPOSIX_unlock(UA_EventLoop *public_el) {
    UA_UNLOCK(&((UA_EventLoopPOSIX*)public_el)->elMutex);
}

UA_EventLoop *
UA_EventLoop_new_POSIX(const UA_Logger *logger) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)
        UA_calloc(1, sizeof(UA_EventLoopPOSIX));
    if(!el)
        return NULL;

    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);

    /* Initialize the queue */
    el->delayedTail = &el->delayedHead1;
    el->delayedHead2 = (UA_DelayedCallback*)0x01; /* sentinel value */

#ifdef UA_ARCHITECTURE_WIN32
    /* Start the WSA networking subsystem on Windows */
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    /* Set the public EventLoop content */
    el->eventLoop.logger = logger;

    el->eventLoop.start = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopPOSIX_start;
    el->eventLoop.stop = (void (*)(UA_EventLoop*))UA_EventLoopPOSIX_stop;
    el->eventLoop.free = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopPOSIX_free;
    el->eventLoop.run = (UA_StatusCode (*)(UA_EventLoop*, UA_UInt32))UA_EventLoopPOSIX_run;
    el->eventLoop.cancel = (void (*)(UA_EventLoop*))UA_EventLoopPOSIX_cancel;

    el->eventLoop.dateTime_now = UA_EventLoopPOSIX_DateTime_now;
    el->eventLoop.dateTime_nowMonotonic =
        UA_EventLoopPOSIX_DateTime_nowMonotonic;
    el->eventLoop.dateTime_localTimeUtcOffset =
        UA_EventLoopPOSIX_DateTime_localTimeUtcOffset;

    el->eventLoop.nextTimer = UA_EventLoopPOSIX_nextTimer;
    el->eventLoop.addTimer = UA_EventLoopPOSIX_addTimer;
    el->eventLoop.modifyTimer = UA_EventLoopPOSIX_modifyTimer;
    el->eventLoop.removeTimer = UA_EventLoopPOSIX_removeTimer;
    el->eventLoop.addDelayedCallback = UA_EventLoopPOSIX_addDelayedCallback;
    el->eventLoop.removeDelayedCallback = UA_EventLoopPOSIX_removeDelayedCallback;

    el->eventLoop.registerEventSource =
        (UA_StatusCode (*)(UA_EventLoop*, UA_EventSource*))
        UA_EventLoopPOSIX_registerEventSource;
    el->eventLoop.deregisterEventSource =
        (UA_StatusCode (*)(UA_EventLoop*, UA_EventSource*))
        UA_EventLoopPOSIX_deregisterEventSource;

    el->eventLoop.lock = UA_EventLoopPOSIX_lock;
    el->eventLoop.unlock = UA_EventLoopPOSIX_unlock;

    return &el->eventLoop;
}

/***************************/
/* Network Buffer Handling */
/***************************/

UA_StatusCode
UA_EventLoopPOSIX_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    if(pcm->txBuffer.length == 0)
        return UA_ByteString_allocBuffer(buf, bufSize);
    if(pcm->txBuffer.length < bufSize)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *buf = pcm->txBuffer;
    buf->length = bufSize;
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopPOSIX_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf) {
    UA_POSIXConnectionManager *pcm = (UA_POSIXConnectionManager*)cm;
    if(pcm->txBuffer.data == buf->data)
        UA_ByteString_init(buf);
    else
        UA_ByteString_clear(buf);
}

UA_StatusCode
UA_EventLoopPOSIX_allocateStaticBuffers(UA_POSIXConnectionManager *pcm) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_UInt32 rxBufSize = 2u << 16; /* The default is 64kb */
    const UA_UInt32 *configRxBufSize = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&pcm->cm.eventSource.params,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(configRxBufSize)
        rxBufSize = *configRxBufSize;
    if(pcm->rxBuffer.length != rxBufSize) {
        UA_ByteString_clear(&pcm->rxBuffer);
        res = UA_ByteString_allocBuffer(&pcm->rxBuffer, rxBufSize);
    }

    const UA_UInt32 *txBufSize = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&pcm->cm.eventSource.params,
                                 UA_QUALIFIEDNAME(0, "send-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(txBufSize && pcm->txBuffer.length != *txBufSize) {
        UA_ByteString_clear(&pcm->txBuffer);
        res |= UA_ByteString_allocBuffer(&pcm->txBuffer, *txBufSize);
    }
    return res;
}

/******************/
/* Socket Options */
/******************/

enum ZIP_CMP
cmpFD(const UA_FD *a, const UA_FD *b) {
    if(*a == *b)
        return ZIP_CMP_EQ;
    return (*a < *b) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

UA_StatusCode
UA_EventLoopPOSIX_setNonBlocking(UA_FD sockfd) {
#ifndef UA_ARCHITECTURE_WIN32
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0 || fcntl(sockfd, F_SETFL, opts | O_NONBLOCK) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#else
    u_long iMode = 1;
    if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopPOSIX_setNoSigPipe(UA_FD sockfd) {
#ifdef SO_NOSIGPIPE
    int val = 1;
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
    if(res < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopPOSIX_setReusable(UA_FD sockfd) {
    int enableReuseVal = 1;
#ifndef UA_ARCHITECTURE_WIN32
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                            (const char*)&enableReuseVal, sizeof(enableReuseVal));
    res |= UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT,
                            (const char*)&enableReuseVal, sizeof(enableReuseVal));
    return (res == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
#else
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                            (const char*)&enableReuseVal, sizeof(enableReuseVal));
    return (res == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
#endif
}

/************************/
/* Select / epoll Logic */
/************************/

/* Re-arm the self-pipe socket for the next signal by reading from it */
static void
flushSelfPipe(UA_SOCKET s) {
    char buf[128];
#ifdef UA_ARCHITECTURE_WIN32
    recv(s, buf, 128, 0);
#else
    ssize_t i;
    do {
        i = read(s, buf, 128);
    } while(i > 0);
#endif
}

#if !defined(UA_HAVE_EPOLL)

UA_StatusCode
UA_EventLoopPOSIX_registerFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Registering fd: %u", (unsigned)rfd->fd);

    /* Realloc */
    UA_RegisteredFD **fds_tmp = (UA_RegisteredFD**)
        UA_realloc(el->fds, sizeof(UA_RegisteredFD*) * (el->fdsSize + 1));
    if(!fds_tmp) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    el->fds = fds_tmp;

    /* Add to the last entry */
    el->fds[el->fdsSize] = rfd;
    el->fdsSize++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopPOSIX_modifyFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    /* Do nothing, it is enough if the data was changed in the rfd */
    UA_LOCK_ASSERT(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopPOSIX_deregisterFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Unregistering fd: %u", (unsigned)rfd->fd);

    /* Find the entry */
    size_t i = 0;
    for(; i < el->fdsSize; i++) {
        if(el->fds[i] == rfd)
            break;
    }

    /* Not found? */
    if(i == el->fdsSize)
        return;

    if(el->fdsSize > 1) {
        /* Move the last entry in the ith slot and realloc. */
        el->fdsSize--;
        el->fds[i] = el->fds[el->fdsSize];
        UA_RegisteredFD **fds_tmp = (UA_RegisteredFD**)
            UA_realloc(el->fds, sizeof(UA_RegisteredFD*) * el->fdsSize);
        /* if realloc fails the fds are still in a correct state with
         * possibly lost memory, so failing silently here is ok */
        if(fds_tmp)
            el->fds = fds_tmp;
    } else {
        /* Remove the last entry */
        UA_free(el->fds);
        el->fds = NULL;
        el->fdsSize = 0;
    }
}

static UA_FD
setFDSets(UA_EventLoopPOSIX *el, fd_set *readset, fd_set *writeset, fd_set *errset) {
    UA_LOCK_ASSERT(&el->elMutex);

    FD_ZERO(readset);
    FD_ZERO(writeset);
    FD_ZERO(errset);

    /* Always listen on the read-end of the pipe */
    UA_FD highestfd = el->selfpipe[0];
    FD_SET(el->selfpipe[0], readset);

    for(size_t i = 0; i < el->fdsSize; i++) {
        UA_FD currentFD = el->fds[i]->fd;

        /* Add to the fd_sets */
        if(el->fds[i]->listenEvents & UA_FDEVENT_IN)
            FD_SET(currentFD, readset);
        if(el->fds[i]->listenEvents & UA_FDEVENT_OUT)
            FD_SET(currentFD, writeset);

        /* Always return errors */
        FD_SET(currentFD, errset);

        /* Highest fd? */
        if(currentFD > highestfd)
            highestfd = currentFD;
    }
    return highestfd;
}

UA_StatusCode
UA_EventLoopPOSIX_pollFDs(UA_EventLoopPOSIX *el, UA_DateTime listenTimeout) {
    UA_assert(listenTimeout >= 0);
    UA_LOCK_ASSERT(&el->elMutex);

    fd_set readset, writeset, errset;
    UA_FD highestfd = setFDSets(el, &readset, &writeset, &errset);

    /* Nothing to do? */
    if(highestfd == UA_INVALID_FD) {
        UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "No valid FDs for processing");
        return UA_STATUSCODE_GOOD;
    }

    struct timeval tmptv = {
#ifndef UA_ARCHITECTURE_WIN32
        (time_t)(listenTimeout / UA_DATETIME_SEC),
        (suseconds_t)((listenTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC)
#else
        (long)(listenTimeout / UA_DATETIME_SEC),
        (long)((listenTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC)
#endif
    };

    UA_UNLOCK(&el->elMutex);
    int selectStatus = UA_select(highestfd+1, &readset, &writeset, &errset, &tmptv);
    UA_LOCK(&el->elMutex);
    if(selectStatus < 0) {
        /* We will retry, only log the error */
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Error during select: %s", errno_str));
        return UA_STATUSCODE_GOOD;
    }

    /* The self-pipe has received. Clear the buffer by reading. */
    if(UA_UNLIKELY(FD_ISSET(el->selfpipe[0], &readset)))
        flushSelfPipe(el->selfpipe[0]);

    /* Loop over all registered FD to see if an event arrived. Yes, this is why
     * select is slow for many open sockets. */
    for(size_t i = 0; i < el->fdsSize; i++) {
        UA_RegisteredFD *rfd = el->fds[i];

        /* The rfd is already registered for removal. Don't process incoming
         * events any longer. */
        if(rfd->dc.callback)
            continue;

        /* Event signaled for the fd? */
        short event = 0;
        if(FD_ISSET(rfd->fd, &readset)) {
            event = UA_FDEVENT_IN;
        } else if(FD_ISSET(rfd->fd, &writeset)) {
            event = UA_FDEVENT_OUT;
        } else if(FD_ISSET(rfd->fd, &errset)) {
            event = UA_FDEVENT_ERR;
        } else {
            continue;
        }

        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Processing event %u on fd %u", (unsigned)event,
                     (unsigned)rfd->fd);

        /* Call the EventSource callback */
        rfd->eventSourceCB(rfd->es, rfd, event);

        /* The fd has removed itself */
        if(i == el->fdsSize || rfd != el->fds[i])
            i--;
    }
    return UA_STATUSCODE_GOOD;
}

#else /* defined(UA_HAVE_EPOLL) */

UA_StatusCode
UA_EventLoopPOSIX_registerFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.ptr = rfd;
    event.events = 0;
    if(rfd->listenEvents & UA_FDEVENT_IN)
        event.events |= EPOLLIN;
    if(rfd->listenEvents & UA_FDEVENT_OUT)
        event.events |= EPOLLOUT;

    int err = epoll_ctl(el->epollfd, EPOLL_CTL_ADD, rfd->fd, &event);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Could not register for epoll (%s)",
                          rfd->fd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopPOSIX_modifyFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.data.ptr = rfd;
    event.events = 0;
    if(rfd->listenEvents & UA_FDEVENT_IN)
        event.events |= EPOLLIN;
    if(rfd->listenEvents & UA_FDEVENT_OUT)
        event.events |= EPOLLOUT;

    int err = epoll_ctl(el->epollfd, EPOLL_CTL_MOD, rfd->fd, &event);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Could not modify for epoll (%s)",
                          rfd->fd, errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopPOSIX_deregisterFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    int res = epoll_ctl(el->epollfd, EPOLL_CTL_DEL, rfd->fd, NULL);
    if(res != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP %u\t| Could not deregister from epoll (%s)",
                          rfd->fd, errno_str));
    }
}

UA_StatusCode
UA_EventLoopPOSIX_pollFDs(UA_EventLoopPOSIX *el, UA_DateTime listenTimeout) {
    UA_assert(listenTimeout >= 0);

    /* Poll the registered sockets */
    struct epoll_event epoll_events[64];
    int epollfd = el->epollfd;
    UA_UNLOCK(&el->elMutex);
    int events = epoll_wait(epollfd, epoll_events, 64,
                            (int)(listenTimeout / UA_DATETIME_MSEC));
    /* TODO: Replace with pwait2 for higher-precision timeouts once this is
     * available in the standard library.
     *
     * struct timespec precisionTimeout = {
     *  (long)(listenTimeout / UA_DATETIME_SEC),
     *   (long)((listenTimeout % UA_DATETIME_SEC) * 100)
     * };
     * int events = epoll_pwait2(epollfd, epoll_events, 64,
     *                        precisionTimeout, NULL); */
    UA_LOCK(&el->elMutex);

    /* Handle error conditions */
    if(events == -1) {
        if(errno == EINTR) {
            /* We will retry, only log the error */
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Timeout during poll");
            return UA_STATUSCODE_GOOD;
        }
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "TCP\t| Error %s, closing the server socket",
                          errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Process all received events */
    for(int i = 0; i < events; i++) {
        UA_RegisteredFD *rfd = (UA_RegisteredFD*)epoll_events[i].data.ptr;

        /* The self-pipe has received */
        if(!rfd) {
            flushSelfPipe(el->selfpipe[0]);
            continue;
        }

        /* The rfd is already registered for removal. Don't process incoming
         * events any longer. */
        if(rfd->dc.callback)
            continue;

        /* Get the event */
        short revent = 0;
        if((epoll_events[i].events & EPOLLIN) == EPOLLIN) {
            revent = UA_FDEVENT_IN;
        } else if((epoll_events[i].events & EPOLLOUT) == EPOLLOUT) {
            revent = UA_FDEVENT_OUT;
        } else {
            revent = UA_FDEVENT_ERR;
        }

        /* Call the EventSource callback */
        rfd->eventSourceCB(rfd->es, rfd, revent);
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* defined(UA_HAVE_EPOLL) */

#if defined(UA_ARCHITECTURE_WIN32) || defined(__APPLE__)
int UA_EventLoopPOSIX_pipe(SOCKET fds[2]) {
    struct sockaddr_in inaddr;
    memset(&inaddr, 0, sizeof(inaddr));
    inaddr.sin_family = AF_INET;
    inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    inaddr.sin_port = 0;

    SOCKET lst = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    bind(lst, (struct sockaddr *)&inaddr, sizeof(inaddr));
    listen(lst, 1);

    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    int len = sizeof(addr);
    getsockname(lst, (struct sockaddr*)&addr, &len);

    fds[0] = socket(AF_INET, SOCK_STREAM, 0);
    int err = connect(fds[0], (struct sockaddr*)&addr, len);
    fds[1] = accept(lst, 0, 0);
#ifdef UA_ARCHITECTURE_WIN32
    closesocket(lst);
#endif
#ifdef __APPLE__
    close(lst);
#endif

    UA_EventLoopPOSIX_setNoSigPipe(fds[0]);
    UA_EventLoopPOSIX_setReusable(fds[0]);
    UA_EventLoopPOSIX_setNonBlocking(fds[0]);
    UA_EventLoopPOSIX_setNoSigPipe(fds[1]);
    UA_EventLoopPOSIX_setReusable(fds[1]);
    UA_EventLoopPOSIX_setNonBlocking(fds[1]);
    return err;
}
#endif

void
UA_EventLoopPOSIX_cancel(UA_EventLoopPOSIX *el) {
    /* Nothing to do if the EventLoop is not executing */
    if(!el->executing)
        return;

    /* Trigger the self-pipe */
#ifdef UA_ARCHITECTURE_WIN32
    int err = send(el->selfpipe[1], ".", 1, 0);
#else
    ssize_t err = write(el->selfpipe[1], ".", 1);
#endif
    if(err <= 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Eventloop\t| Error signaling self-pipe (%s)", errno_str));
    }
}

#endif
