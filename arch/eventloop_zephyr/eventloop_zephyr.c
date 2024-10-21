/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_zephyr.h"

#if defined(UA_ARCHITECTURE_ZEPHYR)

#include "open62541/plugin/eventloop.h"

#include <time.h>

/*********/
/* Timer */
/*********/

static UA_DateTime
UA_EventLoopZephyr_nextTimer(UA_EventLoop *public_el) {
    UA_EventLoopZephyr *el = (UA_EventLoopZephyr *)public_el;
    return UA_Timer_next(&el->timer);
}

static UA_StatusCode
UA_EventLoopZephyr_addTimer(UA_EventLoop *public_el, UA_Callback cb, void *application,
                            void *data, UA_Double interval_ms, UA_DateTime *baseTime,
                            UA_TimerPolicy timerPolicy, UA_UInt64 *callbackId) {
    UA_EventLoopZephyr *el = (UA_EventLoopZephyr *)public_el;
    return UA_Timer_add(&el->timer, cb, application, data, interval_ms,
                        public_el->dateTime_nowMonotonic(public_el), baseTime,
                        timerPolicy, callbackId);
}

static UA_StatusCode
UA_EventLoopZephyr_modifyTimer(UA_EventLoop *public_el, UA_UInt64 callbackId,
                               UA_Double interval_ms, UA_DateTime *baseTime,
                               UA_TimerPolicy timerPolicy) {
    UA_EventLoopZephyr *el = (UA_EventLoopZephyr *)public_el;
    return UA_Timer_modify(&el->timer, callbackId, interval_ms,
                           public_el->dateTime_nowMonotonic(public_el), baseTime,
                           timerPolicy);
}

static void
UA_EventLoopZephyr_removeTimer(UA_EventLoop *public_el, UA_UInt64 callbackId) {
    UA_EventLoopZephyr *el = (UA_EventLoopZephyr *)public_el;
    UA_Timer_remove(&el->timer, callbackId);
}

void
UA_EventLoopZephyr_addDelayedCallback(UA_EventLoop *public_el, UA_DelayedCallback *dc) {
    UA_EventLoopZephyr *el = (UA_EventLoopZephyr *)public_el;
    UA_DelayedCallback *old;
    do {
        old = el->delayedCallbacks;
        dc->next = old;
    } while(UA_atomic_cmpxchg((void *volatile *)&el->delayedCallbacks, old, dc) != old);
}

static void
UA_EventLoopZephyr_removeDelayedCallback(UA_EventLoop *public_el,
                                         UA_DelayedCallback *dc) {
    UA_EventLoopZephyr *el = (UA_EventLoopZephyr *)public_el;
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
processDelayed(UA_EventLoopZephyr *el) {
    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Process delayed callbacks");

    UA_LOCK_ASSERT(&el->elMutex);

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
UA_EventLoopZephyr_start(UA_EventLoopZephyr *el) {
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_FRESH &&
       el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Starting the EventLoop");

    /* Setting the clock source */
    const UA_Int32 *cs = (const UA_Int32 *)UA_KeyValueMap_getScalar(
        &el->eventLoop.params, UA_QUALIFIEDNAME(0, "clock-source"),
        &UA_TYPES[UA_TYPES_INT32]);
    const UA_Int32 *csm = (const UA_Int32 *)UA_KeyValueMap_getScalar(
        &el->eventLoop.params, UA_QUALIFIEDNAME(0, "clock-source-monotonic"),
        &UA_TYPES[UA_TYPES_INT32]);
    if(cs || csm) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "Eventloop\t| Cannot set a custom clock source");
    }
    /* Create the self-pipe */
    int err = UA_EventLoopZephyr_pipe(el->selfpipe);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(
            el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
            "Eventloop\t| Could not create the self-pipe (%s)", errno_str));
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Start the EventSources */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_EventSource *es = el->eventLoop.eventSources;
    while(es) {
        UA_UNLOCK(&el->elMutex);
        res |= es->start(es);
        UA_LOCK(&el->elMutex);
        es = es->next;
    }

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState *)(uintptr_t)&el->eventLoop.state = UA_EVENTLOOPSTATE_STARTED;

    UA_UNLOCK(&el->elMutex);
    return res;
}

static void
checkClosed(UA_EventLoopZephyr *el) {
    UA_LOCK_ASSERT(&el->elMutex);

    UA_EventSource *es = el->eventLoop.eventSources;
    while(es) {
        if(es->state != UA_EVENTSOURCESTATE_STOPPED)
            return;
        es = es->next;
    }

    /* Not closed until all delayed callbacks are processed */
    if(el->delayedCallbacks != NULL)
        return;

    /* Close the self-pipe when everything else is done */
    UA_close(el->selfpipe[0]);
    UA_close(el->selfpipe[1]);

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState *)(uintptr_t)&el->eventLoop.state = UA_EVENTLOOPSTATE_STOPPED;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "The EventLoop has stopped");
}

static void
UA_EventLoopZephyr_stop(UA_EventLoopZephyr *el) {
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
    *(UA_EventLoopState *)(uintptr_t)&el->eventLoop.state = UA_EVENTLOOPSTATE_STOPPING;

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
UA_EventLoopZephyr_run(UA_EventLoopZephyr *el, UA_UInt32 timeout) {
    UA_LOCK(&el->elMutex);

    if(el->executing) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
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

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP, "Iterate the EventLoop");

    /* Process cyclic callbacks */
    UA_DateTime dateBefore = el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);

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
    UA_StatusCode rv = UA_EventLoopZephyr_pollFDs(el, listenTimeout);

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
UA_EventLoopZephyr_registerEventSource(UA_EventLoopZephyr *el, UA_EventSource *es) {
    UA_LOCK(&el->elMutex);

    /* Already registered? */
    if(es->state != UA_EVENTSOURCESTATE_FRESH) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                     "Cannot register the EventSource \"%.*s\": "
                     "already registered",
                     (int)es->name.length, (char *)es->name.data);
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
UA_EventLoopZephyr_deregisterEventSource(UA_EventLoopZephyr *el, UA_EventSource *es) {
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
UA_EventLoopZephyr_DateTime_now(UA_EventLoop *el) {
    return UA_DateTime_now();
}

static UA_DateTime
UA_EventLoopZephyr_DateTime_nowMonotonic(UA_EventLoop *el) {
    return UA_DateTime_nowMonotonic();
}

static UA_Int64
UA_EventLoopZephyr_DateTime_localTimeUtcOffset(UA_EventLoop *el) {
    /* TODO: Fix for custom clock sources */
    return UA_DateTime_localTimeUtcOffset();
}

/*************************/
/* Initialize and Delete */
/*************************/

static UA_StatusCode
UA_EventLoopZephyr_free(UA_EventLoopZephyr *el) {
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
        UA_EventLoopZephyr_deregisterEventSource(el, es);
        UA_LOCK(&el->elMutex);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&el->timer);

    /* Process remaining delayed callbacks */
    processDelayed(el);

    UA_KeyValueMap_clear(&el->eventLoop.params);

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

UA_EventLoop *
UA_EventLoop_new_Zephyr(const UA_Logger *logger) {
    UA_EventLoopZephyr *el =
        (UA_EventLoopZephyr *)UA_calloc(1, sizeof(UA_EventLoopZephyr));
    if(!el)
        return NULL;

    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);

    /* Set the public EventLoop content */
    el->eventLoop.logger = logger;

    el->eventLoop.start = (UA_StatusCode(*)(UA_EventLoop *))UA_EventLoopZephyr_start;
    el->eventLoop.stop = (void (*)(UA_EventLoop *))UA_EventLoopZephyr_stop;
    el->eventLoop.free = (UA_StatusCode(*)(UA_EventLoop *))UA_EventLoopZephyr_free;
    el->eventLoop.run =
        (UA_StatusCode(*)(UA_EventLoop *, UA_UInt32))UA_EventLoopZephyr_run;
    el->eventLoop.cancel = (void (*)(UA_EventLoop *))UA_EventLoopZephyr_cancel;

    el->eventLoop.dateTime_now = UA_EventLoopZephyr_DateTime_now;
    el->eventLoop.dateTime_nowMonotonic = UA_EventLoopZephyr_DateTime_nowMonotonic;
    el->eventLoop.dateTime_localTimeUtcOffset =
        UA_EventLoopZephyr_DateTime_localTimeUtcOffset;

    el->eventLoop.nextTimer = UA_EventLoopZephyr_nextTimer;
    el->eventLoop.addTimer = UA_EventLoopZephyr_addTimer;
    el->eventLoop.modifyTimer = UA_EventLoopZephyr_modifyTimer;
    el->eventLoop.removeTimer = UA_EventLoopZephyr_removeTimer;
    el->eventLoop.addDelayedCallback = UA_EventLoopZephyr_addDelayedCallback;
    el->eventLoop.removeDelayedCallback = UA_EventLoopZephyr_removeDelayedCallback;

    el->eventLoop.registerEventSource = (UA_StatusCode(*)(
        UA_EventLoop *, UA_EventSource *))UA_EventLoopZephyr_registerEventSource;
    el->eventLoop.deregisterEventSource = (UA_StatusCode(*)(
        UA_EventLoop *, UA_EventSource *))UA_EventLoopZephyr_deregisterEventSource;

    return &el->eventLoop;
}

/***************************/
/* Network Buffer Handling */
/***************************/

UA_StatusCode
UA_EventLoopZephyr_allocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                                      UA_ByteString *buf, size_t bufSize) {
    UA_ZephyrConnectionManager *pcm = (UA_ZephyrConnectionManager *)cm;
    if(pcm->txBuffer.length == 0)
        return UA_ByteString_allocBuffer(buf, bufSize);
    if(pcm->txBuffer.length < bufSize)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *buf = pcm->txBuffer;
    buf->length = bufSize;
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopZephyr_freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                                     UA_ByteString *buf) {
    UA_ZephyrConnectionManager *pcm = (UA_ZephyrConnectionManager *)cm;
    if(pcm->txBuffer.data == buf->data)
        UA_ByteString_init(buf);
    else
        UA_ByteString_clear(buf);
}

UA_StatusCode
UA_EventLoopZephyr_allocateStaticBuffers(UA_ZephyrConnectionManager *pcm) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_UInt32 rxBufSize = 2u << 13; /* The default is 64kb */
    const UA_UInt32 *configRxBufSize = (const UA_UInt32 *)UA_KeyValueMap_getScalar(
        &pcm->cm.eventSource.params, UA_QUALIFIEDNAME(0, "recv-bufsize"),
        &UA_TYPES[UA_TYPES_UINT32]);
    if(configRxBufSize)
        rxBufSize = *configRxBufSize;
    if(pcm->rxBuffer.length != rxBufSize) {
        UA_ByteString_clear(&pcm->rxBuffer);
        res = UA_ByteString_allocBuffer(&pcm->rxBuffer, rxBufSize);
    }

    const UA_UInt32 *txBufSize = (const UA_UInt32 *)UA_KeyValueMap_getScalar(
        &pcm->cm.eventSource.params, UA_QUALIFIEDNAME(0, "send-bufsize"),
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
cmpFD(const UA_fd *a, const UA_fd *b) {
    if(*a == *b)
        return ZIP_CMP_EQ;
    return (*a < *b) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
}

UA_StatusCode
UA_EventLoopZephyr_setNonBlocking(UA_fd sockfd) {
    int fl = UA_fcntl(sockfd, F_GETFL, 0);
    if(fl == -1) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    fl |= O_NONBLOCK;
    fl = UA_fcntl(sockfd, F_SETFL, fl);
    if(fl == -1) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopZephyr_setNoSigPipe(UA_fd sockfd) {
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopZephyr_setReusable(UA_fd sockfd) {
    return UA_STATUSCODE_GOOD;
}

/************************/
/* Select / epoll Logic */
/************************/

/* Re-arm the self-pipe socket for the next signal by reading from it */
static void
flushSelfPipe(UA_fd s) {
    char buf[128];
    UA_recv(s, buf, sizeof(buf), 0);
}

UA_StatusCode
UA_EventLoopZephyr_registerFD(UA_EventLoopZephyr *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP, "Registering fd: %u",
                 (unsigned)rfd->fd);

    /* Realloc */
    UA_RegisteredFD **fds_tmp = (UA_RegisteredFD **)UA_realloc(
        el->fds, sizeof(UA_RegisteredFD *) * (el->fdsSize + 1));
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
UA_EventLoopZephyr_modifyFD(UA_EventLoopZephyr *el, UA_RegisteredFD *rfd) {
    /* Do nothing, it is enough if the data was changed in the rfd */
    UA_LOCK_ASSERT(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopZephyr_deregisterFD(UA_EventLoopZephyr *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP, "Unregistering fd: %u",
                 (unsigned)rfd->fd);

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
        UA_RegisteredFD **fds_tmp = (UA_RegisteredFD **)UA_realloc(
            el->fds, sizeof(UA_RegisteredFD *) * el->fdsSize);
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

static UA_fd
setFDSets(UA_EventLoopZephyr *el, UA_fd_set *readset, UA_fd_set *writeset,
          UA_fd_set *errset) {
    UA_LOCK_ASSERT(&el->elMutex);
    /* Always listen on the read-end of the pipe */
    UA_fd highestfd = el->selfpipe[0];
    UA_FD_ZERO(readset);
    UA_FD_ZERO(writeset);
    UA_FD_ZERO(errset);
    UA_FD_SET(el->selfpipe[0], readset);

    for(size_t i = 0; i < el->fdsSize; i++) {
        UA_fd currentFD = el->fds[i]->fd;

        /* Add to the fd_sets */
        if(el->fds[i]->listenEvents & UA_FDEVENT_IN)
            UA_FD_SET(currentFD, readset);
        if(el->fds[i]->listenEvents & UA_FDEVENT_OUT)
            UA_FD_SET(currentFD, writeset);

        /* Always return errors */
        UA_FD_SET(currentFD, errset);

        /* Highest fd? */
        if(currentFD > highestfd)
            highestfd = currentFD;
    }
    return highestfd;
}

UA_StatusCode
UA_EventLoopZephyr_pollFDs(UA_EventLoopZephyr *el, UA_DateTime listenTimeout) {
    UA_assert(listenTimeout >= 0);
    UA_LOCK_ASSERT(&el->elMutex);

    UA_fd_set readset, writeset, errset;
    UA_fd highestfd = setFDSets(el, &readset, &writeset, &errset);

    /* Nothing to do? */
    if(highestfd == UA_INVALID_FD) {
        UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "No valid FDs for processing");
        return UA_STATUSCODE_GOOD;
    }

    struct timeval tmptv = {(long)(listenTimeout / UA_DATETIME_SEC),
                            (long)((listenTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC)};

    UA_UNLOCK(&el->elMutex);
    int selectStatus =
        UA_select((int)highestfd + 1, &readset, &writeset, &errset, &tmptv);
    UA_LOCK(&el->elMutex);
    if(selectStatus < 0) {
        /* We will retry, only log the error */
        UA_LOG_SOCKET_ERRNO_WRAP(UA_LOG_WARNING(el->eventLoop.logger,
                                                UA_LOGCATEGORY_EVENTLOOP,
                                                "Error during select: %s", errno_str));
        return UA_STATUSCODE_GOOD;
    }

    /* The self-pipe has received. Clear the buffer by reading. */
    if(UA_UNLIKELY(UA_FD_ISSET(el->selfpipe[0], &readset)))
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
        if(UA_FD_ISSET(rfd->fd, &readset)) {
            event = UA_FDEVENT_IN;
        } else if(UA_FD_ISSET(rfd->fd, &writeset)) {
            event = UA_FDEVENT_OUT;
        } else if(UA_FD_ISSET(rfd->fd, &errset)) {
            event = UA_FDEVENT_ERR;
        } else {
            continue;
        }

        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Processing event %u on fd %u", (unsigned)event, (unsigned)rfd->fd);

        /* Call the EventSource callback */
        rfd->eventSourceCB(rfd->es, rfd, event);

        /* The fd has removed itself */
        if(i == el->fdsSize || rfd != el->fds[i])
            i--;
    }
    return UA_STATUSCODE_GOOD;
}

int
UA_EventLoopZephyr_pipe(UA_fd fds[2]) {
    UA_fd lst;
    struct sockaddr_in inaddr;
    memset(&inaddr, 0, sizeof(inaddr));
    inaddr.sin_family = AF_INET;
    inaddr.sin_port = 0;
    const struct in_addr ipv4_loopback = INADDR_LOOPBACK_INIT;
    inaddr.sin_addr = ipv4_loopback;

    lst = UA_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    UA_bind(lst, (struct sockaddr *)&inaddr, sizeof(inaddr));
    UA_listen(lst, 1);

    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    int len = sizeof(addr);
    UA_getsockname(lst, (struct sockaddr *)&addr, &len);

    fds[0] = UA_socket(AF_INET, SOCK_STREAM, 0);
    int err = UA_connect(fds[0], (struct sockaddr *)&addr, len);
    fds[1] = UA_accept(lst, 0, 0);
    UA_close(lst);

    UA_EventLoopZephyr_setNoSigPipe(fds[0]);
    UA_EventLoopZephyr_setReusable(fds[0]);
    UA_EventLoopZephyr_setNonBlocking(fds[0]);
    UA_EventLoopZephyr_setNoSigPipe(fds[1]);
    UA_EventLoopZephyr_setReusable(fds[1]);
    UA_EventLoopZephyr_setNonBlocking(fds[1]);
    return err;
}

void
UA_EventLoopZephyr_cancel(UA_EventLoopZephyr *el) {
    /* Nothing to do if the EventLoop is not executing */
    if(!el->executing)
        return;

    /* Trigger the self-pipe */
    int err = UA_send(el->selfpipe[1], ".", 1, 0);
    if(err <= 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Eventloop\t| Error signaling self-pipe (%s)", errno_str));
    }
}

#endif
