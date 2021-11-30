/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_posix.h"
#include "common/ua_timer.h"

typedef struct {
    UA_FD fd;
    short eventMask;
    UA_EventSource *es;
    UA_FDCallback callback;
    void *fdcontext;
} UA_RegisteredFD;

struct UA_EventLoop {
    UA_EventLoopState state;
    const UA_Logger *logger;
    
    /* Timer */
    UA_Timer timer;

    /* Linked List of Delayed Callbacks */
    UA_DelayedCallback *delayedCallbacks;

    /* Pointers to registered EventSources */
    UA_EventSource *eventSources;

    /* Registered file descriptors */
    size_t fdsSize;
    UA_RegisteredFD *fds;

    /* Flag determining whether the eventloop is currently within the "run" method */
    UA_Boolean executing;

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
};

/*********/
/* Timer */
/*********/

static UA_StatusCode
processFDs(UA_EventLoop *el, UA_DateTime usedTimeout);

static void
timerExecutionTrampoline(void *executionApplication, UA_ApplicationCallback cb,
                         void *callbackApplication, void *data) {
    cb(callbackApplication, data);
}

UA_StatusCode
UA_EventLoop_addTimedCallback(UA_EventLoop *el, UA_Callback callback,
                              void *application, void *data, UA_DateTime date,
                              UA_UInt64 *callbackId) {
    return UA_Timer_addTimedCallback(&el->timer, callback, application,
                                     data, date, callbackId);
}

UA_StatusCode
UA_EventLoop_addCyclicCallback(UA_EventLoop *el, UA_Callback cb,
                               void *application, void *data, UA_Double interval_ms,
                               UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                               UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&el->timer, cb, application, data,
                                        interval_ms, baseTime, timerPolicy, callbackId);
}

UA_StatusCode
UA_EventLoop_modifyCyclicCallback(UA_EventLoop *el, UA_UInt64 callbackId,
                                  UA_Double interval_ms, UA_DateTime *baseTime,
                                  UA_TimerPolicy timerPolicy) {
    return UA_Timer_changeRepeatedCallback(&el->timer, callbackId, interval_ms,
                                           baseTime, timerPolicy);
}

void
UA_EventLoop_removeCyclicCallback(UA_EventLoop *el, UA_UInt64 callbackId) {
    UA_Timer_removeCallback(&el->timer, callbackId);
}

void
UA_EventLoop_addDelayedCallback(UA_EventLoop *el, UA_DelayedCallback *dc) {
    UA_LOCK(&el->elMutex);
    dc->next = el->delayedCallbacks;
    el->delayedCallbacks = dc;
    UA_UNLOCK(&el->elMutex);
}

/* Process and then free registered delayed callbacks */
static void
processDelayed(UA_EventLoop *el) {
    UA_LOCK_ASSERT(&el->elMutex, 1);
    while(el->delayedCallbacks) {
        UA_DelayedCallback *dc = el->delayedCallbacks;
        el->delayedCallbacks = dc->next;
        /* Delayed Callbacks might have no cb pointer if all we want to do is
         * free the memory */
        if(dc->callback) {
            UA_UNLOCK(&el->elMutex);
            dc->callback(dc->application, dc->data);
            UA_LOCK(&el->elMutex);
        }
        UA_free(dc);
    }
}

/***********************/
/* EventLoop Lifecycle */
/***********************/

UA_EventLoop *
UA_EventLoop_new(const UA_Logger *logger) {
    UA_EventLoop *el = (UA_EventLoop*)UA_malloc(sizeof(UA_EventLoop));
    if(!el)
        return NULL;
    memset(el, 0, sizeof(UA_EventLoop));
    UA_LOCK_INIT(&el->elMutex);
    el->logger = logger;
    UA_Timer_init(&el->timer);
    return el;
}

UA_StatusCode
UA_EventLoop_delete(UA_EventLoop *el) {
    UA_LOCK(&el->elMutex);

    /* Check if the EventLoop can be deleted */
    if(el->state != UA_EVENTLOOPSTATE_STOPPED &&
       el->state != UA_EVENTLOOPSTATE_FRESH) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot delete a running EventLoop");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Deregister and delete all the EventSources */
    while(el->eventSources) {
        UA_EventSource *es = el->eventSources;
        UA_UNLOCK(&el->elMutex);
        UA_EventLoop_deregisterEventSource(el, es);
        UA_LOCK(&el->elMutex);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&el->timer);

    /* Process remaining delayed callbacks */
    processDelayed(el);

    /* free the file descriptors */
    UA_free(el->fds);

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

UA_EventLoopState
UA_EventLoop_getState(UA_EventLoop *el) {
    return el->state;
}

UA_DateTime
UA_EventLoop_nextCyclicTime(UA_EventLoop *el) {
    return UA_Timer_nextRepeatedTime(&el->timer);
}

UA_StatusCode
UA_EventLoop_start(UA_EventLoop *el) {
    UA_LOCK(&el->elMutex);
    if(el->state != UA_EVENTLOOPSTATE_FRESH &&
       el->state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(el->logger, UA_LOGCATEGORY_EVENTLOOP, "Starting the EventLoop");

    UA_EventSource *es = el->eventSources;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    while(es) {
        UA_UNLOCK(&el->elMutex);
        res |= es->start(es);
        UA_LOCK(&el->elMutex);
        es = es->next;
    }

    el->state = UA_EVENTLOOPSTATE_STARTED;
    UA_UNLOCK(&el->elMutex);
    return res;
}

static void
checkClosed(UA_EventLoop *el) {
    UA_EventSource *es = el->eventSources;
    while(es) {
        if(es->state != UA_EVENTSOURCESTATE_STOPPED)
            return;
        es = es->next;
    }

    UA_LOG_INFO(el->logger, UA_LOGCATEGORY_EVENTLOOP, "The EventLoop has stopped");
    el->state = UA_EVENTLOOPSTATE_STOPPED;
}

void
UA_EventLoop_stop(UA_EventLoop *el) {
    UA_LOCK(&el->elMutex);

    UA_LOG_INFO(el->logger, UA_LOGCATEGORY_EVENTLOOP, "Stopping the EventLoop");

    /* Shutdown all event sources. This will close open connections. */
    UA_EventSource *es = el->eventSources;
    while(es) {
        if(es->state == UA_EVENTSOURCESTATE_STARTING ||
           es->state == UA_EVENTSOURCESTATE_STARTED)
            es->stop(es);
        es = es->next;
    }

    UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "All EventSources are stopped");

    el->state = UA_EVENTLOOPSTATE_STOPPING;
    checkClosed(el);
    UA_UNLOCK(&el->elMutex);
}

/* After every select, reset the file-descriptors to listen on */
static UA_FD
setFDSets(UA_EventLoop *el, fd_set *readset, fd_set *writeset, fd_set *errset) {
    FD_ZERO(readset);
    FD_ZERO(writeset);
    FD_ZERO(errset);
    UA_FD highestfd = UA_INVALID_FD;
    for(size_t i = 0; i < el->fdsSize; i++) {

        UA_FD currentFD = el->fds[i].fd;
        /* Add to the fd_sets */
        if(el->fds[i].eventMask & UA_POSIX_EVENT_READ)
            UA_fd_set(currentFD, readset);
        if(el->fds[i].eventMask & UA_POSIX_EVENT_WRITE)
            UA_fd_set(currentFD, writeset);
        if(el->fds[i].eventMask & UA_POSIX_EVENT_ERR)
            UA_fd_set(currentFD, errset);

        /* Highest fd? */
        if(currentFD > highestfd || highestfd == UA_INVALID_FD)
            highestfd = currentFD;
    }
    return highestfd;
}

static UA_StatusCode
processFDs(UA_EventLoop *el, UA_DateTime usedTimeout) {
    fd_set readset, writeset, errset;
    UA_FD highestfd = setFDSets(el, &readset, &writeset, &errset);
    if(highestfd == UA_INVALID_FD) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                       "No valid FDs for processing");

        return UA_STATUSCODE_GOOD;
    }

    struct timeval tmptv = {
#ifndef _WIN32
        (time_t)(usedTimeout / UA_DATETIME_SEC),
        (suseconds_t)((usedTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC)
#else
        (long)(usedTimeout / UA_DATETIME_SEC),
        (long)((usedTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC)
#endif
    };

    int selectStatus =  UA_select(highestfd+1, &readset, &writeset, &errset, &tmptv);
    if(selectStatus < 0) {
        /* We will retry, only log the error */
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(UA_EventLoop_getLogger(el),
                           UA_LOGCATEGORY_EVENTLOOP,
                           "Error during select: %s", errno_str));
        el->executing = false;
        return UA_STATUSCODE_GOODCALLAGAIN;
    }

    /* Loop over all registered FD to see if an event arrived. Yes, this is why
      * select is slow for many open sockets. */
    for(size_t i = 0; i < el->fdsSize; i++) {
        UA_RegisteredFD *rfd = &el->fds[i];
        UA_FD fd = rfd->fd;
        UA_assert(fd > 0);

        UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Processing fd: %u", (unsigned)fd);

        /* Error Event */
        if((rfd->eventMask & UA_POSIX_EVENT_ERR) && UA_fd_isset(fd, &errset)) {
            UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                         "Processing error event for fd: %u", (unsigned)fd);
            UA_UNLOCK(&el->elMutex);
            rfd->callback(rfd->es, fd, &rfd->fdcontext, UA_POSIX_EVENT_ERR);
            UA_LOCK(&el->elMutex);
            if(i == el->fdsSize || fd != el->fds[i].fd)
                i--; /* The fd has removed itself */
            continue;
        }

        /* Read Event */
        if((rfd->eventMask & UA_POSIX_EVENT_READ) && UA_fd_isset(fd, &readset)) {
            UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                         "Processing read event for fd: %u", (unsigned)fd);
            UA_UNLOCK(&el->elMutex);
            rfd->callback(rfd->es, fd, &rfd->fdcontext, UA_POSIX_EVENT_READ);
            UA_LOCK(&el->elMutex);
            if(i == el->fdsSize || fd != el->fds[i].fd)
                i--; /* The fd has removed itself */
            continue;
        }

        /* Write Event */
        if((rfd->eventMask & UA_POSIX_EVENT_WRITE) && UA_fd_isset(fd, &writeset)) {
            UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                         "Processing write event for fd: %u", (unsigned)fd);
            UA_UNLOCK(&el->elMutex);
            rfd->callback(rfd->es, fd, &rfd->fdcontext, UA_POSIX_EVENT_WRITE);
            UA_LOCK(&el->elMutex);
            if(i == el->fdsSize || fd != el->fds[i].fd)
                i--; /* The fd has removed itself */
            continue;
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoop_run(UA_EventLoop *el, UA_UInt32 timeout) {
    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP, "iterate the EventLoop");

    if(el->executing) {
        UA_LOG_ERROR(el->logger,
                     UA_LOGCATEGORY_EVENTLOOP,
                     "Cannot run EventLoop from the run method itself");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    /* TODO: use check macros instead
    UA_CHECK_ERROR(!el->executing, return UA_STATUSCODE_BADINTERNALERROR, el->logger,
                   UA_LOGCATEGORY_EVENTLOOP,
                   "Cannot run eventloop from the run method itself");
    */

    el->executing = true;

    if(el->state == UA_EVENTLOOPSTATE_FRESH ||
       el->state == UA_EVENTLOOPSTATE_STOPPED) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot iterate a stopped EventLoop");
        el->executing = false;
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Process cyclic callbacks */
    UA_DateTime dateBeforeCallback = UA_DateTime_nowMonotonic();

    UA_UNLOCK(&el->elMutex);
    UA_DateTime dateOfNextCallback =
        UA_Timer_process(&el->timer, dateBeforeCallback, timerExecutionTrampoline, NULL);
    UA_LOCK(&el->elMutex);

    UA_DateTime dateAfterCallback = UA_DateTime_nowMonotonic();

    UA_DateTime processTimerDuration = dateAfterCallback - dateBeforeCallback;

    UA_DateTime callbackTimeout = dateOfNextCallback - dateAfterCallback;
    UA_DateTime maxTimeout = UA_MAX(timeout * UA_DATETIME_MSEC - processTimerDuration, 0);

    UA_DateTime usedTimeout = UA_MIN(callbackTimeout, maxTimeout);

    /* Listen on the active file-descriptors (sockets) from the ConnectionManagers */
    UA_StatusCode rv = processFDs(el, usedTimeout);
    if(rv == UA_STATUSCODE_GOODCALLAGAIN) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_GOOD;
    }
    if(rv != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&el->elMutex);
        return rv;
    }

    /* Process and then free registered delayed callbacks */
    processDelayed(el);

    /* Check if the last EventSource was successfully stopped */
    if(el->state == UA_EVENTLOOPSTATE_STOPPING)
        checkClosed(el);

    el->executing = false;
    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}


/*****************************/
/* Registering Event Sources */
/*****************************/

UA_StatusCode
UA_EventLoop_registerEventSource(UA_EventLoop *el, UA_EventSource *es) {
    /* Already registered? */
    if(es->state != UA_EVENTSOURCESTATE_FRESH) {
        UA_LOG_ERROR(UA_EventLoop_getLogger(el), UA_LOGCATEGORY_NETWORK,
                     "Cannot register the EventSource \"%.*s\": already registered",
                     (int)es->name.length, (char*)es->name.data);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add to linked list */
    UA_LOCK(&el->elMutex);
    es->next = el->eventSources;
    el->eventSources = es;
    UA_UNLOCK(&el->elMutex);

    es->eventLoop = el;
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    /* Start if the entire EventLoop is started */
    if(el->state == UA_EVENTLOOPSTATE_STARTED)
        return es->start(es);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoop_deregisterEventSource(UA_EventLoop *el, UA_EventSource *es) {
    if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot deregister the EventSource %.*s. Has to be stopped first",
                       (int)es->name.length, es->name.data);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Remove from the linked list */
    UA_LOCK(&el->elMutex);
    UA_EventSource **s = &el->eventSources;
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

/********************************/
/* Registering File Descriptors */
/********************************/

UA_StatusCode
UA_EventLoop_registerFD(UA_EventLoop *el, UA_FD fd, short eventMask,
                        UA_FDCallback cb, UA_EventSource *es, void *fdcontext) {
    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Registering fd: %u", (unsigned)fd);

    /* Realloc */
    UA_RegisteredFD *fds_tmp = (UA_RegisteredFD*)
        UA_realloc(el->fds, sizeof(UA_RegisteredFD) * (el->fdsSize + 1));
    if(!fds_tmp) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    el->fds = fds_tmp;

    /* Add to the last entry */
    el->fds[el->fdsSize].callback = cb;
    el->fds[el->fdsSize].eventMask = eventMask;
    el->fds[el->fdsSize].es = es;
    el->fds[el->fdsSize].fdcontext = fdcontext;
    el->fds[el->fdsSize].fd = fd;
    el->fdsSize++;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoop_modifyFD(UA_EventLoop *el, UA_FD fd, short eventMask,
                      UA_FDCallback cb, void *fdcontext) {
    UA_LOCK(&el->elMutex);

    /* Find the entry */
    size_t i = 0;
    for(; i < el->fdsSize; i++) {
        if(el->fds[i].fd == fd)
            break;
    }

    /* Not found? */
    if(i == el->fdsSize) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Modify */
    el->fds[i].callback = cb;
    el->fds[i].eventMask = eventMask;
    el->fds[i].fdcontext = fdcontext;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoop_deregisterFD(UA_EventLoop *el, UA_FD fd) {
    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Unregistering fd: %u", (unsigned)fd);

    /* Find the entry */
    size_t i = 0;
    for(; i < el->fdsSize; i++) {
        if(el->fds[i].fd == fd)
            break;
    }

    /* Not found? */
    if(i == el->fdsSize) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    if(el->fdsSize > 1) {
        /* Move the last entry in the ith slot and realloc. */
        el->fdsSize--;
        el->fds[i] = el->fds[el->fdsSize];
        UA_RegisteredFD *fds_tmp = (UA_RegisteredFD*)
            UA_realloc(el->fds, sizeof(UA_RegisteredFD) * el->fdsSize);
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

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoop_iterateFD(UA_EventLoop *el, UA_EventSource *es, UA_FDCallback cb) {
    for(size_t i = 0; i < el->fdsSize; i++) {
        if(el->fds[i].es != es)
            continue;
        UA_FD fd = el->fds[i].fd;
        cb(es, fd, el->fds[i].fdcontext, 0);
        if(i == el->fdsSize || fd != el->fds[i].fd)
            i--; /* The fd has removed itself */
    }
}

/* Helper Functions */

const UA_Logger *
UA_EventLoop_getLogger(UA_EventLoop *el) {
    return el->logger;
}

void
UA_EventLoop_setLogger(UA_EventLoop *el, const UA_Logger *logger) {
    el->logger = logger;
}
