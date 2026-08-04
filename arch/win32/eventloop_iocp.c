/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_iocp.h"
#include "open62541/plugin/eventloop.h"


/*********/
/* Timer */
/*********/

UA_DateTime
UA_EventLoopWIN32_nextTimer(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    if(el->delayedHead1 > (UA_DelayedCallback *)0x01 ||
       el->delayedHead2 > (UA_DelayedCallback *)0x01)
        return el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    return UA_Timer_next(&el->timer);
}

UA_StatusCode
UA_EventLoopWIN32_addTimer(UA_EventLoop *public_el, UA_Callback cb,
                           void *application, void *data, UA_Double interval_ms,
                           UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                           UA_UInt64 *callbackId) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    return UA_Timer_add(&el->timer, cb, application, data, interval_ms,
                        public_el->dateTime_nowMonotonic(public_el),
                        baseTime, timerPolicy, callbackId);
}

UA_StatusCode
UA_EventLoopWIN32_modifyTimer(UA_EventLoop *public_el,
                              UA_UInt64 callbackId,
                              UA_Double interval_ms,
                              UA_DateTime *baseTime,
                              UA_TimerPolicy timerPolicy) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    return UA_Timer_modify(&el->timer, callbackId, interval_ms,
                           public_el->dateTime_nowMonotonic(public_el),
                           baseTime, timerPolicy);
}

void
UA_EventLoopWIN32_removeTimer(UA_EventLoop *public_el,
                              UA_UInt64 callbackId) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_Timer_remove(&el->timer, callbackId);
}

void
UA_EventLoopWIN32_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    dc->next = NULL;

    /* el->delayedTail points either to prev->next or to the head. In an atomic
     * xchg-operation we make the tail point to dc. This also gives us
     * prev->next. Then we make prev->next point to dc.
     *
     * This is thread-safe. Another thread might retrieve dc from the tail.
     * Then he can set dc->next while we are still updating prev->next.
     * It is ensured that only on thread can updated dc->next. */
    UA_atomic(UA_atomic(UA_DelayedCallback*)*) prev_next;
    UA_atomic_xchg(&el->delayedTail, &dc->next, &prev_next);
    UA_atomic_store(prev_next, dc);
}

/* Resets the delayed queue and returns the previous head and tail */
static void
resetDelayedQueue(UA_EventLoopWIN32 *el,
                  UA_atomic(UA_DelayedCallback*)* oldHead,
                  UA_atomic(UA_atomic(UA_DelayedCallback*)*)* oldTail) {
    if(el->delayedHead1 <= (UA_DelayedCallback *)0x01 &&
       el->delayedHead2 <= (UA_DelayedCallback *)0x01)
        return; /* The queue is empty */

    /* Get the location of the active and the inactive head */
    UA_Boolean active1 = (el->delayedHead1 != (UA_DelayedCallback*)0x01);
    UA_atomic(UA_DelayedCallback*)* activeHead = (active1) ? &el->delayedHead1 : &el->delayedHead2;
    UA_atomic(UA_DelayedCallback*)* inactiveHead = (active1) ? &el->delayedHead2 : &el->delayedHead1;

    /* Set NULL to the inactive head. This indicates it is now active. */
    UA_atomic_store(inactiveHead, NULL);

    /* Set a sentinel value to "inactivate" the active head. Return the old
     * active head. Parallel threads may continue to add elements below the old
     * "activeHead" if they already have a pointer. */
    UA_atomic_xchg(activeHead, (UA_DelayedCallback*)0x01, oldHead);

    /* Make the inactiveHead the new "active" by pointing to it from the tail.
     * Also return the old tail. From the consumer-thread we can then iterate
     * the linked-list until we find the old tail as the last element. */
    UA_atomic_xchg(&el->delayedTail, inactiveHead, oldTail);
}

void
UA_EventLoopWIN32_removeDelayedCallback(UA_EventLoop *public_el,
                                        UA_DelayedCallback *dc) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    /* Reset and get the old head and tail */
    UA_atomic(UA_DelayedCallback *) cur = NULL;
    UA_atomic(UA_atomic(UA_DelayedCallback*)*) tail = NULL;
    resetDelayedQueue(el, &cur, &tail);

    /* tail points to the location where the next element shall be inserted: The
     * next-pointer of the last element. Since the next-pointer is the first
     * struct member, we can directly cast to the last element. */
    UA_DelayedCallback *last = (UA_DelayedCallback*)(uintptr_t)tail;

    /* Loop until we reach the tail (or head and tail are both NULL) */
    UA_DelayedCallback *next;
    for(; cur; cur = next) {
        /* Spin-loop until the next-pointer of cur is updated.
         * The element pointed to by tail must appear eventually. */
        next = cur->next;
        while(!next && cur != last)
            next = UA_atomic_load(&cur->next);
        if(cur == dc)
            continue;
        UA_EventLoopWIN32_addDelayedCallback(public_el, cur);
    }

    UA_UNLOCK(&el->elMutex);
}

void
UA_EventLoopWIN32_processDelayed(UA_EventLoopWIN32 *el) {
    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Process delayed callbacks");

    UA_LOCK_ASSERT(&el->elMutex);

    /* Reset and get the old head and tail */
    UA_atomic(UA_DelayedCallback *) dc = NULL;
    UA_atomic(UA_atomic(UA_DelayedCallback*)*) tail = NULL;
    resetDelayedQueue(el, &dc, &tail);

    /* tail points to the location where the next element shall be inserted: The
     * next-pointer of the last element. Since the next-pointer is the first
     * struct member, we can directly cast to the last element. */
    UA_DelayedCallback *last = (UA_DelayedCallback*)(uintptr_t)tail;

    /* Loop until we reach the tail (or head and tail are both NULL) */
    UA_DelayedCallback *next;
    for(; dc; dc = next) {
        next = dc->next;
        while(!next && dc != last)
            next = UA_atomic_load(&dc->next);
        if(!dc->callback)
            continue;
        dc->callback(dc->application, dc->context);
    }
}

/***********************/
/* EventLoop Lifecycle */
/***********************/

static UA_StatusCode
UA_EventLoopWIN32_start(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_FRESH &&
       el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Starting the EventLoop");

    /* Setting custom clock source */
    const UA_Int32 *cs = (const UA_Int32*)
        UA_KeyValueMap_getScalar(&el->eventLoop.params,
                                 UA_QUALIFIEDNAME(0, "clock-source"),
                                 &UA_TYPES[UA_TYPES_INT32]);
    if(cs)
        el->clockSource = *cs;

    const UA_Int32 *csm = (const UA_Int32*)
        UA_KeyValueMap_getScalar(&el->eventLoop.params,
                                 UA_QUALIFIEDNAME(0, "clock-source-monotonic"),
                                 &UA_TYPES[UA_TYPES_INT32]);
    if(csm) {
        if(el->clockSourceMonotonic != *csm && el->timer.idTree.root) {
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Eventloop\t| Setting a different monotonic clock, ",
                           "but existing timers have been registered with a "
                           "different clock source");
        }
        el->clockSourceMonotonic = *csm;
    }

    if(cs || csm) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Eventloop\t| Setting a different clock source ",
                       "not supported for this architecture");
    }

    /* Create the self-pipe */
    int err = UA_EventLoopWIN32_pipe(el->selfpipe);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "Eventloop\t| Could not create the self-pipe (%s)",
                          errno_str));
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create the epoll socket */

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
checkClosed(UA_EventLoopWIN32 *el) {
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

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "The EventLoop has stopped");
}

static void
UA_EventLoopWIN32_stop(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
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
UA_EventLoopWIN32_run(UA_EventLoop *public_el, UA_UInt32 timeout) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
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
    UA_EventLoopWIN32_processDelayed(el);

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
    UA_StatusCode rv = UA_EventLoopWIN32_pollFDs(el, listenTimeout);

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

UA_StatusCode
UA_EventLoopWIN32_registerEventSource(UA_EventLoop *public_el,
                                      UA_EventSource *es) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
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

UA_StatusCode
UA_EventLoopWIN32_deregisterEventSource(UA_EventLoop *public_el,
                                        UA_EventSource *es) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
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

UA_DateTime
UA_EventLoopWIN32_DateTime_now(UA_EventLoop *el) {
    return UA_DateTime_now();
}

UA_DateTime
UA_EventLoopWIN32_DateTime_nowMonotonic(UA_EventLoop *el) {
    return UA_DateTime_nowMonotonic();
}

UA_Int64
UA_EventLoopWIN32_DateTime_localTimeUtcOffset(UA_EventLoop *el) {
    /* TODO: Fix for custom clock sources */
    return UA_DateTime_localTimeUtcOffset();
}

/*************************/
/* Initialize and Delete */
/*************************/

static UA_StatusCode
UA_EventLoopWIN32_free(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
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
        UA_EventLoopWIN32_deregisterEventSource(public_el, es);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&el->timer);

    /* Process remaining delayed callbacks */
    UA_EventLoopWIN32_processDelayed(el);

    /* Stop the Windows networking subsystem */
    WSACleanup();

    UA_KeyValueMap_clear(&el->eventLoop.params);

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopWIN32_lock(UA_EventLoop *public_el) {
    UA_LOCK(&((UA_EventLoopWIN32*)public_el)->elMutex);
}
void
UA_EventLoopWIN32_unlock(UA_EventLoop *public_el) {
    UA_UNLOCK(&((UA_EventLoopWIN32*)public_el)->elMutex);
}

/* Forward declarations for the FD-polling backend implementations further
 * down in this file (select or epoll, chosen at compile time). */
static UA_StatusCode registerFD_select(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);
static UA_StatusCode modifyFD_select(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);
static void deregisterFD_select(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);

UA_EventLoop *
UA_EventLoop_new_WIN32(const UA_Logger *logger) {
    /* Start the WSA networking subsystem on Windows */
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if(iResult != 0) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Initializing the WSA subsystem failed: %d", iResult);
        return NULL;
    }

    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)
        UA_calloc(1, sizeof(UA_EventLoopWIN32));
    if(!el)
        return NULL;

    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);

    /* Initialize the queue */
    el->delayedTail = &el->delayedHead1;
    el->delayedHead2 = (UA_DelayedCallback*)0x01; /* sentinel value */

    /* Set the public EventLoop content */
    el->eventLoop.logger = logger;

    /* Initialize the clock source to the default */

    /* Set the method pointers for the interface */
    el->eventLoop.start = UA_EventLoopWIN32_start;
    el->eventLoop.stop = UA_EventLoopWIN32_stop;
    el->eventLoop.free = UA_EventLoopWIN32_free;
    el->eventLoop.run = UA_EventLoopWIN32_run;
    el->eventLoop.cancel = UA_EventLoopWIN32_cancel;

    el->eventLoop.dateTime_now = UA_EventLoopWIN32_DateTime_now;
    el->eventLoop.dateTime_nowMonotonic =
        UA_EventLoopWIN32_DateTime_nowMonotonic;
    el->eventLoop.dateTime_localTimeUtcOffset =
        UA_EventLoopWIN32_DateTime_localTimeUtcOffset;

    el->eventLoop.nextTimer = UA_EventLoopWIN32_nextTimer;
    el->eventLoop.addTimer = UA_EventLoopWIN32_addTimer;
    el->eventLoop.modifyTimer = UA_EventLoopWIN32_modifyTimer;
    el->eventLoop.removeTimer = UA_EventLoopWIN32_removeTimer;
    el->eventLoop.addDelayedCallback = UA_EventLoopWIN32_addDelayedCallback;
    el->eventLoop.removeDelayedCallback = UA_EventLoopWIN32_removeDelayedCallback;

    el->eventLoop.registerEventSource = UA_EventLoopWIN32_registerEventSource;
    el->eventLoop.deregisterEventSource = UA_EventLoopWIN32_deregisterEventSource;

    el->eventLoop.lock = UA_EventLoopWIN32_lock;
    el->eventLoop.unlock = UA_EventLoopWIN32_unlock;

    /* Select the FD polling backend */
    el->registerFD = registerFD_select;
    el->modifyFD = modifyFD_select;
    el->deregisterFD = deregisterFD_select;

    return &el->eventLoop;
}

/***************************/
/* Network Buffer Handling */
/***************************/

UA_StatusCode
UA_EventLoopWIN32_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize) {
    UA_WIN32ConnectionManager *pcm = (UA_WIN32ConnectionManager*)cm;
    /* Reuse the static tx buffer; fall back to allocation for larger messages. */
    if(pcm->txBuffer.length < bufSize)
        return UA_ByteString_allocBuffer(buf, bufSize);
    *buf = pcm->txBuffer;
    buf->length = bufSize;
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopWIN32_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf) {
    UA_WIN32ConnectionManager *pcm = (UA_WIN32ConnectionManager*)cm;
    if(pcm->txBuffer.data == buf->data)
        UA_ByteString_init(buf);
    else
        UA_ByteString_clear(buf);
}

UA_StatusCode
UA_EventLoopWIN32_allocateStaticBuffers(UA_WIN32ConnectionManager *pcm) {
    UA_StatusCode res =
        UA_EventLoopCommon_allocStaticBuffer(&pcm->cm.eventSource.params,
                                             UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                             1u << 16, /* The default is 64 kb */
                                             &pcm->rxBuffer);

    /* Default the tx buffer to the rx size so a dedicated static send buffer
     * always exists. This avoids a malloc/free on every send without reusing
     * the rx buffer (which may still hold unprocessed received data). */
    res |= UA_EventLoopCommon_allocStaticBuffer(&pcm->cm.eventSource.params,
                                                UA_QUALIFIEDNAME(0, "send-bufsize"),
                                                (UA_UInt32)pcm->rxBuffer.length,
                                                &pcm->txBuffer);
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
UA_EventLoopWIN32_setNonBlocking(UA_FD sockfd) {
    u_long iMode = 1;
    if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopWIN32_setNoSigPipe(UA_FD sockfd) {
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopWIN32_setReusable(UA_FD sockfd) {
    int enableReuseVal = 1;
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                            (const char*)&enableReuseVal, sizeof(enableReuseVal));
    return (res == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}

/************************/
/* Select / epoll Logic */
/************************/

/* Re-arm the self-pipe socket for the next signal by reading from it */
static void
flushSelfPipe(UA_SOCKET s) {
    char buf[128];
    int i;
    do {
        i = UA_recv(s, buf, 128, 0);
    } while(i > 0);
}


static UA_StatusCode
registerFD_select(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd) {
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

static UA_StatusCode
modifyFD_select(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd) {
    /* Do nothing, it is enough if the data was changed in the rfd */
    UA_LOCK_ASSERT(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static void
deregisterFD_select(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd) {
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
setFDSets(UA_EventLoopWIN32 *el, fd_set *readset, fd_set *writeset, fd_set *errset) {
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
UA_EventLoopWIN32_pollFDs(UA_EventLoopWIN32 *el, UA_DateTime listenTimeout) {
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
        (long)(listenTimeout / UA_DATETIME_SEC),
        (long)((listenTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC)
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
        if(i >= el->fdsSize || rfd != el->fds[i])
            i--;
    }
    return UA_STATUSCODE_GOOD;
}


/* Thin wrappers dispatching through the backend selected in
 * UA_EventLoop_new_WIN32 / UA_EventLoop_new_GLib. This is what
 * ConnectionManagers (TCP, UDP, Ethernet, ...) actually call -- they do not
 * need to know which backend is behind a given EventLoop instance. */

UA_StatusCode
UA_EventLoopWIN32_registerFD(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd) {
    return el->registerFD(el, rfd);
}

UA_StatusCode
UA_EventLoopWIN32_modifyFD(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd) {
    return el->modifyFD(el, rfd);
}

void
UA_EventLoopWIN32_deregisterFD(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd) {
    el->deregisterFD(el, rfd);
}

int UA_EventLoopWIN32_pipe(SOCKET fds[2]) {
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
    socklen_t len = sizeof(addr);
    getsockname(lst, (struct sockaddr*)&addr, &len);

    fds[0] = socket(AF_INET, SOCK_STREAM, 0);
    int err = connect(fds[0], (struct sockaddr*)&addr, len);
    fds[1] = accept(lst, 0, 0);
    UA_close(lst);

    UA_EventLoopWIN32_setNoSigPipe(fds[0]);
    UA_EventLoopWIN32_setReusable(fds[0]);
    UA_EventLoopWIN32_setNonBlocking(fds[0]);
    UA_EventLoopWIN32_setNoSigPipe(fds[1]);
    UA_EventLoopWIN32_setReusable(fds[1]);
    UA_EventLoopWIN32_setNonBlocking(fds[1]);
    return err;
}

void
UA_EventLoopWIN32_cancel(UA_EventLoop *public_el) {
    UA_EventLoopWIN32 *el = (UA_EventLoopWIN32*)public_el;
    /* Nothing to do if the EventLoop is not executing */
    if(!el->executing)
        return;

    /* Trigger the self-pipe */
    int err = (int)UA_send(el->selfpipe[1], ".", 1, 0);
    if(err <= 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Eventloop\t| Error signaling self-pipe (%s)", errno_str));
    }
}

/* Compatibility alias for existing Win32 applications. */
UA_EventLoop *
UA_EventLoop_new_POSIX(const UA_Logger *logger) {
    return UA_EventLoop_new_WIN32(logger);
}
