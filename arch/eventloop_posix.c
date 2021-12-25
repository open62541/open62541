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
    /* The members fd and events are stored in the separate pollfds array:
     * UA_FD fd;
     * short events; */
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
    struct pollfd *pollfds; /* has the same size as "fds" */

    /* Flag determining whether the eventloop is currently within the "run" method */
    UA_Boolean executing;

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
};

/*********/
/* Timer */
/*********/

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

    /* All file descriptors were removed together with the coresponding
     * EventSource */
    UA_assert(el->fdsSize == 0);

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

static UA_StatusCode
pollFDs(UA_EventLoop *el, UA_DateTime listenTimeout) {
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
            UA_LOG_WARNING(UA_EventLoop_getLogger(el),
                           UA_LOGCATEGORY_EVENTLOOP,
                           "Error during poll: %s", errno_str));
        return UA_STATUSCODE_GOODCALLAGAIN;
    }

    /* Loop over all registered FD to see if an event arrived. Yes, this is why
     * poll is slow for many open sockets. */
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

UA_StatusCode
UA_EventLoop_run(UA_EventLoop *el, UA_UInt32 timeout) {
    UA_LOCK(&el->elMutex);

    UA_LOG_TRACE(el->logger, UA_LOGCATEGORY_EVENTLOOP, "Iterate the EventLoop");

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

    /* Compute the remaining time */
    UA_DateTime maxDate = dateBeforeCallback + (timeout * UA_DATETIME_MSEC);
    if(dateOfNextCallback > maxDate)
        dateOfNextCallback = maxDate;
    UA_DateTime listenTimeout = dateOfNextCallback - UA_DateTime_nowMonotonic();
    if(listenTimeout < 0)
        listenTimeout = 0;

    /* Listen on the active file-descriptors (sockets) from the ConnectionManagers */
    UA_StatusCode rv = pollFDs(el, listenTimeout);

    /* Process and then free registered delayed callbacks */
    processDelayed(el);

    /* Check if the last EventSource was successfully stopped */
    if(el->state == UA_EVENTLOOPSTATE_STOPPING)
        checkClosed(el);

    el->executing = false;
    UA_UNLOCK(&el->elMutex);
    return rv;
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

UA_EventSource *
UA_EventLoop_findEventSource(UA_EventLoop *el, const UA_String name) {
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
UA_EventLoop_modifyFD(UA_EventLoop *el, UA_FD fd, short eventMask,
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
UA_EventLoop_deregisterFD(UA_EventLoop *el, UA_FD fd) {
    UA_LOCK(&el->elMutex);

    UA_LOG_DEBUG(el->logger, UA_LOGCATEGORY_EVENTLOOP,
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
UA_EventLoop_iterateFD(UA_EventLoop *el, UA_EventSource *es, UA_FDCallback cb) {
    for(size_t i = 0; i < el->fdsSize; i++) {
        if(el->fds[i].es != es)
            continue;

        UA_FD fd = el->pollfds[i].fd;
        cb(es, fd, el->fds[i].fdcontext, 0);

        /* The fd has removed itself from within the callback? */
        if(i >= el->fdsSize || fd != el->pollfds[i].fd)
            i--;
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
