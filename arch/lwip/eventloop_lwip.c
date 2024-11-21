/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include "eventloop_lwip.h"

#if defined(UA_ARCHITECTURE_LWIP)

#include "open62541/plugin/eventloop.h"

/*********/
/* Timer */
/*********/

static UA_DateTime
UA_EventLoopLWIP_nextCyclicTime(UA_EventLoop *public_el) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    return UA_Timer_nextRepeatedTime(&el->timer);
}

static UA_StatusCode
UA_EventLoopLWIP_addTimedCallback(UA_EventLoop *public_el,
                                   UA_Callback callback,
                                   void *application, void *data,
                                   UA_DateTime date,
                                   UA_UInt64 *callbackId) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    return UA_Timer_addTimedCallback(&el->timer, callback, application,
                                     data, date, callbackId);
}

static UA_StatusCode
UA_EventLoopLWIP_addCyclicCallback(UA_EventLoop *public_el,
                                    UA_Callback cb,
                                    void *application, void *data,
                                    UA_Double interval_ms,
                                    UA_DateTime *baseTime,
                                    UA_TimerPolicy timerPolicy,
                                    UA_UInt64 *callbackId) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    return UA_Timer_addRepeatedCallback(&el->timer, cb, application,
                                        data, interval_ms, baseTime,
                                        timerPolicy, callbackId);
}

static UA_StatusCode
UA_EventLoopLWIP_modifyCyclicCallback(UA_EventLoop *public_el,
                                       UA_UInt64 callbackId,
                                       UA_Double interval_ms,
                                       UA_DateTime *baseTime,
                                       UA_TimerPolicy timerPolicy) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    return UA_Timer_changeRepeatedCallback(&el->timer, callbackId,
                                           interval_ms, baseTime,
                                           timerPolicy);
}

static void
UA_EventLoopLWIP_removeCyclicCallback(UA_EventLoop *public_el,
                                       UA_UInt64 callbackId) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    UA_Timer_removeCallback(&el->timer, callbackId);
}

static void
UA_EventLoopLWIP_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    UA_LOCK(&el->elMutex);
    dc->next = el->delayedCallbacks;
    el->delayedCallbacks = dc;
    UA_UNLOCK(&el->elMutex);
}

static void
UA_EventLoopLWIP_removeDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    UA_LOCK(&el->elMutex);
    UA_DelayedCallback **prev = &el->delayedCallbacks;
    while(*prev) {
        if(*prev == dc) {
            *prev = (*prev)->next;
            UA_UNLOCK(&el->elMutex);
            return;
        }
        prev = &(*prev)->next;
    }
    UA_UNLOCK(&el->elMutex);
}

/* Process and then free registered delayed callbacks */
static void
processDelayed(UA_EventLoopLWIP *el) {
    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Process delayed callbacks");

    UA_LOCK_ASSERT(&el->elMutex, 1);

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
        UA_UNLOCK(&el->elMutex);
        dc->callback(dc->application, dc->context);
        UA_LOCK(&el->elMutex);
    }
}

/***********************/
/* EventLoop Lifecycle */
/***********************/

static UA_StatusCode
UA_EventLoopLWIP_start(UA_EventLoopLWIP *el) {
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
        UA_UNLOCK(&el->elMutex);
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
checkClosed(UA_EventLoopLWIP *el) {
    UA_LOCK_ASSERT(&el->elMutex, 1);

    UA_EventSource *es = el->eventLoop.eventSources;
    while(es) {
        if(es->state != UA_EVENTSOURCESTATE_STOPPED)
            return;
        es = es->next;
    }

    /* Not closed until all delayed callbacks are processed */
    if(el->delayedCallbacks != NULL)
        return;

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
UA_EventLoopLWIP_stop(UA_EventLoopLWIP *el) {
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_STARTED) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "The EventLoop is not running, cannot be stopped");
        UA_UNLOCK(&el->elMutex);
        return;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "Stopping the EventLoop");

    /* Set to STOPPING to prevent "normal use" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPING;

    /* Stop all event sources (asynchronous) */
    UA_EventSource *es = el->eventLoop.eventSources;
    for(; es; es = es->next) {
        if(es->state == UA_EVENTSOURCESTATE_STARTING ||
           es->state == UA_EVENTSOURCESTATE_STARTED) {
            UA_UNLOCK(&el->elMutex);
            es->stop(es);
            UA_LOCK(&el->elMutex);
        }
    }

    /* Set to STOPPED if all EventSources are STOPPED */
    checkClosed(el);

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
UA_EventLoopLWIP_run(UA_EventLoopLWIP *el, UA_UInt32 timeout) {
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
    UA_DateTime dateNext = UA_Timer_process(&el->timer, dateBefore);
    UA_LOCK(&el->elMutex);

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
    UA_StatusCode rv = UA_EventLoopLWIP_pollFDs(el, listenTimeout);

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
UA_EventLoopLWIP_registerEventSource(UA_EventLoopLWIP *el,
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
UA_EventLoopLWIP_deregisterEventSource(UA_EventLoopLWIP *el,
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

/* No special synchronization with an external source, just use the globally
 * defined functions. */

static UA_DateTime
UA_EventLoopLWIP_DateTime_now(UA_EventLoop *el) {
    return UA_DateTime_now();
}

static UA_DateTime
UA_EventLoopLWIP_DateTime_nowMonotonic(UA_EventLoop *el) {
    return UA_DateTime_nowMonotonic();
}

static UA_Int64
UA_EventLoopLWIP_DateTime_localTimeUtcOffset(UA_EventLoop *el) {
    return UA_DateTime_localTimeUtcOffset();
}

/*************************/
/* Initialize and Delete */
/*************************/

static UA_StatusCode
UA_EventLoopLWIP_free(UA_EventLoopLWIP *el) {
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
        UA_EventLoopLWIP_deregisterEventSource(el, es);
        UA_LOCK(&el->elMutex);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&el->timer);

    /* Process remaining delayed callbacks */
    processDelayed(el);

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

UA_EventLoop *
UA_EventLoop_new_LWIP(const UA_Logger *logger) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)
        UA_calloc(1, sizeof(UA_EventLoopLWIP));
    if(!el)
        return NULL;

    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);

    /* Set the public EventLoop content */
    el->eventLoop.logger = logger;

    el->eventLoop.start = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopLWIP_start;
    el->eventLoop.stop = (void (*)(UA_EventLoop*))UA_EventLoopLWIP_stop;
    el->eventLoop.run = (UA_StatusCode (*)(UA_EventLoop*, UA_UInt32))UA_EventLoopLWIP_run;
    el->eventLoop.free = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopLWIP_free;

    el->eventLoop.dateTime_now = UA_EventLoopLWIP_DateTime_now;
    el->eventLoop.dateTime_nowMonotonic =
        UA_EventLoopLWIP_DateTime_nowMonotonic;
    el->eventLoop.dateTime_localTimeUtcOffset =
        UA_EventLoopLWIP_DateTime_localTimeUtcOffset;

    el->eventLoop.nextTimer = UA_EventLoopLWIP_nextCyclicTime;
    el->eventLoop.addTimer = UA_EventLoopLWIP_addCyclicCallback;
    el->eventLoop.modifyTimer = UA_EventLoopLWIP_modifyCyclicCallback;
    el->eventLoop.removeTimer = UA_EventLoopLWIP_removeCyclicCallback;
    el->eventLoop.addDelayedCallback = UA_EventLoopLWIP_addDelayedCallback;
    el->eventLoop.removeDelayedCallback = UA_EventLoopLWIP_removeDelayedCallback;

    el->eventLoop.registerEventSource =
        (UA_StatusCode (*)(UA_EventLoop*, UA_EventSource*))
        UA_EventLoopLWIP_registerEventSource;
    el->eventLoop.deregisterEventSource =
        (UA_StatusCode (*)(UA_EventLoop*, UA_EventSource*))
        UA_EventLoopLWIP_deregisterEventSource;

    return &el->eventLoop;
}

/* Reusable EventSource functionality */

UA_StatusCode
UA_EventLoopLWIP_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize) {
    return UA_ByteString_allocBuffer(buf, bufSize);
}

void
UA_EventLoopLWIP_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf) {
    UA_ByteString_clear(buf);
}

UA_StatusCode
UA_EventLoopLWIP_allocateRXBuffer(UA_LWIPConnectionManager *pcm) {
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
UA_EventLoopLWIP_setNonBlocking(UA_FD sockfd) {
    int opts = lwip_fcntl(sockfd, F_GETFL, 0);
    if(opts < 0 || lwip_fcntl(sockfd, F_SETFL, opts | O_NONBLOCK) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopLWIP_setNoSigPipe(UA_FD sockfd) {
#ifdef SO_NOSIGPIPE
    int val = 1;
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
    if(res < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopLWIP_setReusable(UA_FD sockfd) {
    int enableReuseVal = 1;
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                            (const char*)&enableReuseVal, sizeof(enableReuseVal));
    return (res == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
}

/************************/
/* Select / epoll Logic */
/************************/

UA_StatusCode
UA_EventLoopLWIP_registerFD(UA_EventLoopLWIP *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex, 1);
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
UA_EventLoopLWIP_modifyFD(UA_EventLoopLWIP *el, UA_RegisteredFD *rfd) {
    /* Do nothing, it is enough if the data was changed in the rfd */
    UA_LOCK_ASSERT(&el->elMutex, 1);
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopLWIP_deregisterFD(UA_EventLoopLWIP *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex, 1);
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
setFDSets(UA_EventLoopLWIP *el, fd_set *readset, fd_set *writeset, fd_set *errset) {
    UA_LOCK_ASSERT(&el->elMutex, 1);

    FD_ZERO(readset);
    FD_ZERO(writeset);
    FD_ZERO(errset);
    UA_FD highestfd = UA_INVALID_FD;
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
        if(currentFD > highestfd || highestfd == UA_INVALID_FD)
            highestfd = currentFD;
    }
    return highestfd;
}

UA_StatusCode
UA_EventLoopLWIP_pollFDs(UA_EventLoopLWIP *el, UA_DateTime listenTimeout) {
    UA_assert(listenTimeout >= 0);
    UA_LOCK_ASSERT(&el->elMutex, 1);

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

#endif /* defined(UA_ARCHITECTURE_LWIP) */
