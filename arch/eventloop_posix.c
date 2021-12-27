/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "eventloop_posix.h"

static UA_StatusCode
POSIX_EL_deregisterEventSource(POSIX_EL *el, UA_EventSource *es);

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
POSIX_EL_nextCyclicTime(UA_EventLoop *public_el) {
    POSIX_EL *el = (POSIX_EL*)public_el;
    return UA_Timer_nextRepeatedTime(&el->timer);
}

static UA_StatusCode
POSIX_EL_addTimedCallback(UA_EventLoop *public_el, UA_Callback callback,
                          void *application, void *data,
                          UA_DateTime date,
                          UA_UInt64 *callbackId) {
    POSIX_EL *el = (POSIX_EL*)public_el;
    return UA_Timer_addTimedCallback(&el->timer, callback, application,
                                     data, date, callbackId);
}

static UA_StatusCode
POSIX_EL_addCyclicCallback(UA_EventLoop *public_el, UA_Callback cb,
                           void *application, void *data,
                           UA_Double interval_ms,
                           UA_DateTime *baseTime,
                           UA_TimerPolicy timerPolicy,
                           UA_UInt64 *callbackId) {
    POSIX_EL *el = (POSIX_EL*)public_el;
    return UA_Timer_addRepeatedCallback(&el->timer, cb, application,
                                        data, interval_ms, baseTime,
                                        timerPolicy, callbackId);
}

static UA_StatusCode
POSIX_EL_modifyCyclicCallback(UA_EventLoop *public_el,
                              UA_UInt64 callbackId,
                              UA_Double interval_ms,
                              UA_DateTime *baseTime,
                              UA_TimerPolicy timerPolicy) {
    POSIX_EL *el = (POSIX_EL*)public_el;
    return UA_Timer_changeRepeatedCallback(&el->timer, callbackId,
                                           interval_ms, baseTime,
                                           timerPolicy);
}

static void
POSIX_EL_removeCyclicCallback(UA_EventLoop *public_el,
                              UA_UInt64 callbackId) {
    POSIX_EL *el = (POSIX_EL*)public_el;
    UA_Timer_removeCallback(&el->timer, callbackId);
}

static void
POSIX_EL_addDelayedCallback(UA_EventLoop *public_el,
                            UA_DelayedCallback *dc) {
    POSIX_EL *el = (POSIX_EL*)public_el;
    UA_LOCK(&el->elMutex);
    dc->next = el->delayedCallbacks;
    el->delayedCallbacks = dc;
    UA_UNLOCK(&el->elMutex);
}

/* Process and then free registered delayed callbacks */
static void
processDelayed(POSIX_EL *el) {
    UA_LOCK_ASSERT(&el->elMutex, 1);
    while(el->delayedCallbacks) {
        UA_DelayedCallback *dc = el->delayedCallbacks;
        el->delayedCallbacks = dc->next;
        /* Delayed Callbacks might have no cb pointer if all
         * we want to do is free the memory */
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

static UA_StatusCode
POSIX_EL_free(POSIX_EL *el) {
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
    while(el->eventSources) {
        UA_EventSource *es = el->eventSources;
        UA_UNLOCK(&el->elMutex);
        POSIX_EL_deregisterEventSource(el, es);
        UA_LOCK(&el->elMutex);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&el->timer);

    /* Process remaining delayed callbacks */
    processDelayed(el);

    /* All file descriptors were removed together with the
     * coresponding EventSource */
    UA_assert(el->fdsSize == 0);

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
POSIX_EL_start(POSIX_EL *el) {
    UA_LOCK(&el->elMutex);
    if(el->eventLoop.state != UA_EVENTLOOPSTATE_FRESH &&
       el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "Starting the EventLoop");

    UA_EventSource *es = el->eventSources;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    while(es) {
        UA_UNLOCK(&el->elMutex);
        res |= es->start(es);
        UA_LOCK(&el->elMutex);
        es = es->next;
    }

    /* Dirty-write the state that is const "outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STARTED;
    UA_UNLOCK(&el->elMutex);
    return res;
}

static void
checkClosed(POSIX_EL *el) {
    UA_EventSource *es = el->eventSources;
    while(es) {
        if(es->state != UA_EVENTSOURCESTATE_STOPPED)
            return;
        es = es->next;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "The EventLoop has stopped");
    /* Dirty-write the state that is const "outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPED;
}

static void
POSIX_EL_stop(POSIX_EL *el) {
    UA_LOCK(&el->elMutex);

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "Stopping the EventLoop");

    /* Shutdown all event sources. This closes open connections. */
    UA_EventSource *es = el->eventSources;
    while(es) {
        if(es->state == UA_EVENTSOURCESTATE_STARTING ||
           es->state == UA_EVENTSOURCESTATE_STARTED)
            es->stop(es);
        es = es->next;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "All EventSources are stopped");

    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPING;
    checkClosed(el);
    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
pollFDs(POSIX_EL *el, UA_DateTime listenTimeout) {
    UA_assert(listenTimeout >= 0);
    /* Poll the registered sockets */
#ifdef _GNU_SOURCE
    struct timespec precisionTimeout = {
        (long)(listenTimeout / UA_DATETIME_SEC),
        (long)((listenTimeout % UA_DATETIME_SEC) * 100)
    };
    int pollStatus = ppoll(el->pollfds, el->fdsSize,
                           &precisionTimeout, NULL);
#else
    int pollStatus = UA_poll(el->pollfds, el->fdsSize,
                             (int)(listenTimeout / UA_DATETIME_MSEC));
#endif

    if(pollStatus < 0) {
        /* We will retry, only log the error */
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger,
                          UA_LOGCATEGORY_EVENTLOOP,
                          "Error during poll: %s", errno_str));
        return UA_STATUSCODE_GOODCALLAGAIN;
    }

    /* Loop over all registered FD to see if an event arrived. Yes,
     * this is why poll is slow for many open sockets. */
    int processed = 0;
    for(size_t i = 0; i < el->fdsSize; i++) {
        /* All done */
        if(processed >= pollStatus)
            break;

        /* Nothing to do for this fd */
        if(el->pollfds[i].revents == 0)
            continue;

        /* Process the fd */
        UA_RegisteredFD *rfd = &el->fds[i];
        UA_FD fd = el->pollfds[i].fd;
        short revent = el->pollfds[i].revents;
        UA_UNLOCK(&el->elMutex);
        rfd->callback(rfd->es, fd, &rfd->fdcontext, revent);
        UA_LOCK(&el->elMutex);
        processed++;

        /* The fd has removed itself from within the callback? */
        if(i >= el->fdsSize || fd != el->pollfds[i].fd)
            i--;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
POSIX_EL_run(POSIX_EL *el, UA_UInt32 timeout) {
    UA_LOCK(&el->elMutex);

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Iterate the EventLoop");

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

    /* Process cyclic callbacks */
    UA_DateTime dateBefore = UA_DateTime_nowMonotonic();

    UA_UNLOCK(&el->elMutex);
    UA_DateTime dateNext =
        UA_Timer_process(&el->timer, dateBefore,
                         timerExecutionTrampoline, NULL);
    UA_LOCK(&el->elMutex);

    /* Compute the remaining time */
    UA_DateTime maxDate = dateBefore + (timeout * UA_DATETIME_MSEC);
    if(dateNext > maxDate)
        dateNext = maxDate;
    UA_DateTime listenTimeout = dateNext - UA_DateTime_nowMonotonic();
    if(listenTimeout < 0)
        listenTimeout = 0;

    /* Listen on the active file-descriptors (sockets) from the
     * ConnectionManagers */
    UA_StatusCode rv = pollFDs(el, listenTimeout);

    /* Process and then free registered delayed callbacks */
    processDelayed(el);

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
POSIX_EL_registerEventSource(POSIX_EL *el, UA_EventSource *es) {
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
    es->next = el->eventSources;
    el->eventSources = es;
    UA_UNLOCK(&el->elMutex);

    es->eventLoop = &el->eventLoop;
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    /* Start if the entire EventLoop is started */
    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STARTED)
        return es->start(es);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
POSIX_EL_deregisterEventSource(POSIX_EL *el, UA_EventSource *es) {
    if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot deregister the EventSource %.*s: "
                       "Has to be stopped first",
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

static UA_EventSource *
POSIX_EL_findEventSource(POSIX_EL *el, const UA_String name) {
    UA_LOCK(&el->elMutex);
    UA_EventSource *s = el->eventSources;
    while(s) {
        if(UA_String_equal(&name, &s->name))
            break;
        s = s->next;
    }
    UA_UNLOCK(&el->elMutex);
    return s;
}

/********************************/
/* Registering File Descriptors */
/********************************/

UA_StatusCode
POSIX_EL_registerFD(POSIX_EL *el, UA_FD fd, short eventMask,
                    UA_FDCallback cb, UA_EventSource *es,
                    void *fdcontext) {
    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Registering fd: %u", (unsigned)fd);

    /* Realloc */
    UA_RegisteredFD *fds_tmp = (UA_RegisteredFD*)
        UA_realloc(el->fds, sizeof(UA_RegisteredFD) * (el->fdsSize + 1));
    if(!fds_tmp) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    el->fds = fds_tmp;

    struct pollfd *pollfds_tmp = (struct pollfd*)
        UA_realloc(el->pollfds, sizeof(struct pollfd) *(el->fdsSize + 1));
    if(!pollfds_tmp) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    el->pollfds = pollfds_tmp;

    /* Add to the last entry */
    el->fds[el->fdsSize].callback = cb;
    el->fds[el->fdsSize].es = es;
    el->fds[el->fdsSize].fdcontext = fdcontext;
    el->pollfds[el->fdsSize].fd = fd;
    el->pollfds[el->fdsSize].events = eventMask;
    el->fdsSize++;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
POSIX_EL_modifyFD(POSIX_EL *el, UA_FD fd, short eventMask,
                  UA_FDCallback cb, void *fdcontext) {
    UA_LOCK(&el->elMutex);

    /* Find the entry */
    size_t i = 0;
    for(; i < el->fdsSize; i++) {
        if(el->pollfds[i].fd == fd)
            break;
    }

    /* Not found? */
    if(i == el->fdsSize) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Modify */
    el->fds[i].callback = cb;
    el->pollfds[i].events = eventMask;
    el->fds[i].fdcontext = fdcontext;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
POSIX_EL_deregisterFD(POSIX_EL *el, UA_FD fd) {
    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Unregistering fd: %u", (unsigned)fd);

    /* Find the entry */
    size_t i = 0;
    for(; i < el->fdsSize; i++) {
        if(el->pollfds[i].fd == fd)
            break;
    }

    /* Not found? */
    if(i == el->fdsSize) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    el->fdsSize--;
    if(el->fdsSize > 0) {
        /* Move the last entry to the ith slot and realloc. */
        el->fds[i] = el->fds[el->fdsSize];
        el->pollfds[i] = el->pollfds[el->fdsSize];

        /* If realloc fails the fds are still in a correct state with
         * possibly lost memory, so failing silently here is ok */
        UA_RegisteredFD *fds_tmp = (UA_RegisteredFD*)
            UA_realloc(el->fds, sizeof(UA_RegisteredFD) * el->fdsSize);
        if(fds_tmp)
            el->fds = fds_tmp;
        struct pollfd *pollfds_tmp = (struct pollfd*)
            UA_realloc(el->pollfds, sizeof(struct pollfd) * el->fdsSize);
        if(pollfds_tmp)
            el->pollfds = pollfds_tmp;
    } else {
        /* Free the lists */
        UA_free(el->fds);
        el->fds = NULL;
        UA_free(el->pollfds);
        el->pollfds = NULL;
    }

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

void
POSIX_EL_iterateFD(POSIX_EL *el, UA_EventSource *es,
                   POSIX_EL_IterateCallback cb,
                   void *iterateContext) {
    for(size_t i = 0; i < el->fdsSize; i++) {
        if(el->fds[i].es != es)
            continue;

        UA_FD fd = el->pollfds[i].fd;
        int done = cb(es, fd, el->fds[i].fdcontext, iterateContext);
        if(done)
            break;

        /* The fd has removed itself from within the callback? */
        if(i >= el->fdsSize || fd != el->pollfds[i].fd)
            i--;
    }
}

UA_EventLoop *
UA_EventLoop_new_POSIX(const UA_Logger *logger) {
    POSIX_EL *el = (POSIX_EL*)UA_malloc(sizeof(POSIX_EL));
    if(!el)
        return NULL;
    memset(el, 0, sizeof(POSIX_EL));
    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);

    /* Set the public EventLoop content */
    el->eventLoop.logger = logger;

    el->eventLoop.start = (UA_StatusCode (*)(UA_EventLoop*))POSIX_EL_start;
    el->eventLoop.stop = (void (*)(UA_EventLoop*))POSIX_EL_stop;
    el->eventLoop.run = (UA_StatusCode (*)(UA_EventLoop*, UA_UInt32))POSIX_EL_run;
    el->eventLoop.free = (UA_StatusCode (*)(UA_EventLoop*))POSIX_EL_free;

    el->eventLoop.nextCyclicTime = POSIX_EL_nextCyclicTime;
    el->eventLoop.addCyclicCallback = POSIX_EL_addCyclicCallback;
    el->eventLoop.modifyCyclicCallback = POSIX_EL_modifyCyclicCallback;
    el->eventLoop.removeCyclicCallback = POSIX_EL_removeCyclicCallback;
    el->eventLoop.addTimedCallback = POSIX_EL_addTimedCallback;
    el->eventLoop.addDelayedCallback = POSIX_EL_addDelayedCallback;
    
    el->eventLoop.registerEventSource =
        (UA_StatusCode (*)(UA_EventLoop*, UA_EventSource*))POSIX_EL_registerEventSource;
    el->eventLoop.deregisterEventSource =
        (UA_StatusCode (*)(UA_EventLoop*, UA_EventSource*))POSIX_EL_deregisterEventSource;
    el->eventLoop.findEventSource =
        (UA_EventSource* (*)(UA_EventLoop*, const UA_String))POSIX_EL_findEventSource;

    return &el->eventLoop;
}
