/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_posix.h"
#include "open62541/plugin/eventloop.h"
#include "open62541/plugin/eventloop_base_funcs.h"

/*********/
/* Timer */
/*********/

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
    UA_LOCK(&el->eventLoop.elMutex);
    dc->next = el->delayedCallbacks;
    el->delayedCallbacks = dc;
    UA_UNLOCK(&el->eventLoop.elMutex);
}

static void
UA_EventLoopPOSIX_removeDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)public_el;
    UA_LOCK(&el->eventLoop.elMutex);
    UA_DelayedCallback **prev = &el->delayedCallbacks;
    while(*prev) {
        if(*prev == dc) {
            *prev = (*prev)->next;
            UA_UNLOCK(&el->eventLoop.elMutex);
            return;
        }
        prev = &(*prev)->next;
    }
    UA_UNLOCK(&el->eventLoop.elMutex);
}

/* Process and then free registered delayed callbacks */
static void
processDelayed(UA_EventLoopPOSIX *el) {
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Process delayed callbacks");

    UA_LOCK_ASSERT(&el->eventLoop.elMutex, 1);

    /* First empty the linked list in the el. So a delayed callback can add
     * (itself) to the list. New entries are then processed during the next
     * iteration. */
    UA_DelayedCallback *dc = el->delayedCallbacks, *next = NULL;
    el->delayedCallbacks = NULL;

    for(; dc; dc = next) {
        next = dc->next;
        /* Delayed Callbacks might have no callback set. We don't return a
         * StatusCode during "add" and don't validate. So test here. */
        if(!dc->callback)
            continue;
        UA_UNLOCK(&el->eventLoop.elMutex);
        dc->callback(dc->application, dc->context);
        UA_LOCK(&el->eventLoop.elMutex);
    }
}

/***********************/
/* EventLoop Lifecycle */
/***********************/

static UA_StatusCode
UA_start_EventLoop_posix(UA_EventLoop *el,
                    void *user_arg) {
#ifdef UA_HAVE_EPOLL
    UA_EventLoopPOSIX *posixEl = (UA_EventLoopPOSIX *)user_arg;
    posixEl->epollfd = epoll_create1(0);
    if(posixEl->epollfd == -1) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_NETWORK,
                          "Eventloop\t| Could not create the epoll socket (%s)",
                          errno_str));
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
#else
    (void)el;
    (void)user_arg;
#endif
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_EventLoopPOSIX_start(UA_EventLoopPOSIX *el) {

    return UA_EventLoop_start(&el->eventLoop,
                              UA_start_EventLoop_posix, el);
}

static UA_StatusCode
UA_check_stopped_EventLoop_posix(UA_EventLoop *el,
                                 void *user_arg) {

    UA_EventLoopPOSIX *posixEl = (UA_EventLoopPOSIX *)user_arg;

    (void)el;
    /* Not closed until all delayed callbacks are processed */
    if(posixEl->delayedCallbacks != NULL)
        return UA_STATUSCODE_GOODCALLAGAIN;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_stop_EventLoop_posix(UA_EventLoop *el,
                        void *user_arg) {


    (void)el;
    /* Close the epoll/IOCP socket once all EventSources have shut down */
#ifdef UA_HAVE_EPOLL
    UA_EventLoopPOSIX *posixEl = (UA_EventLoopPOSIX *)user_arg;
    close(posixEl->epollfd);
#else
    (void)user_arg;
#endif

    return UA_STATUSCODE_GOOD;
}

static void
UA_EventLoopPOSIX_stop(UA_EventLoopPOSIX *el) {

    UA_EventLoop_stop(&el->eventLoop, NULL, NULL);

    UA_EventLoop_check_stopped(&el->eventLoop,
                               UA_check_stopped_EventLoop_posix,
                               UA_stop_EventLoop_posix,
                               el);
}

static UA_StatusCode
UA_EventLoopPOSIX_run(UA_EventLoopPOSIX *el, UA_UInt32 timeout) {
    UA_LOCK(&el->eventLoop.elMutex);

    if(el->executing) {
        UA_LOG_ERROR(el->eventLoop.logger,
                     UA_LOGCATEGORY_EVENTLOOP,
                     "Cannot run EventLoop from the run method itself");
        UA_UNLOCK(&el->eventLoop.elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    el->executing = true;

    if(el->eventLoop.state == UA_EVENTLOOPSTATE_FRESH ||
       el->eventLoop.state == UA_EVENTLOOPSTATE_STOPPED) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot iterate a stopped EventLoop");
        el->executing = false;
        UA_UNLOCK(&el->eventLoop.elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Iterate the EventLoop");

    /* Process cyclic callbacks */
    UA_DateTime dateBefore =
        el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);

    UA_UNLOCK(&el->eventLoop.elMutex);
    UA_DateTime dateNext = UA_Timer_process(&el->timer, dateBefore);
    UA_LOCK(&el->eventLoop.elMutex);

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
    if(el->delayedCallbacks != NULL)
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
    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STOPPING) {
        UA_UNLOCK(&el->eventLoop.elMutex);
        UA_EventLoop_check_stopped(&el->eventLoop,
                                   UA_check_stopped_EventLoop_posix,
                                   UA_stop_EventLoop_posix,
                                   el);
        UA_LOCK(&el->eventLoop.elMutex);
    }

    el->executing = false;
    UA_UNLOCK(&el->eventLoop.elMutex);
    return rv;
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
UA_free_EventLoop_posix(UA_EventLoop *el,
                        void *user_arg) {

    UA_EventLoopPOSIX *posixEl = (UA_EventLoopPOSIX *)user_arg;

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&posixEl->timer);

    /* Process remaining delayed callbacks */
    processDelayed(posixEl);

#ifdef _WIN32
    /* Stop the Windows networking subsystem */
    WSACleanup();
#endif

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_EventLoopPOSIX_free(UA_EventLoopPOSIX *el) {

    return UA_EventLoop_free(&el->eventLoop,
                             UA_free_EventLoop_posix,
                             el);
}

UA_EventLoop *
UA_EventLoop_new_POSIX(const UA_Logger *logger) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)
        UA_calloc(1, sizeof(UA_EventLoopPOSIX));
    if(!el)
        return NULL;

    UA_LOCK_INIT(&el->eventLoop.elMutex);
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
    el->eventLoop.removeDelayedCallback = UA_EventLoopPOSIX_removeDelayedCallback;

    el->eventLoop.registerEventSource = UA_EventLoop_registerEventSource;
    el->eventLoop.deregisterEventSource = UA_EventLoop_deregisterEventSource;

    return &el->eventLoop;
}

/* Reusable EventSource functionality */

UA_StatusCode
UA_EventLoopPOSIX_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize) {
    return UA_ByteString_allocBuffer(buf, bufSize);
}

void
UA_EventLoopPOSIX_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

UA_StatusCode
UA_EventLoopPOSIX_allocateRXBuffer(UA_POSIXConnectionManager *pcm) {
    UA_UInt32 rxBufSize = 2u << 16; /* The default is 64kb */
    const UA_UInt32 *configRxBufSize = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&pcm->cm.eventSource.params,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(configRxBufSize)
        rxBufSize = *configRxBufSize;
    if(pcm->rxBuffer.length != rxBufSize) {
        UA_ByteString_clear(&pcm->rxBuffer);
        return UA_ByteString_allocBuffer(&pcm->rxBuffer, rxBufSize);
    }
    return UA_STATUSCODE_GOOD;
}

/* Socket Handling */

enum ZIP_CMP
cmpFD(const UA_FD *a, const UA_FD *b) {
    if(*a == *b)
        return ZIP_CMP_EQ;
    return (*a < *b) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

UA_StatusCode
UA_EventLoopPOSIX_setNonBlocking(UA_FD sockfd) {
#ifndef _WIN32
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
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                            (const char*)&enableReuseVal, sizeof(enableReuseVal));
    return (res == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}
