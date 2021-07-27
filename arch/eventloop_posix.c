/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "eventloop_posix.h"
#include "ziptree.h"

typedef struct UA_TimerEntry {
    ZIP_ENTRY(UA_TimerEntry) zipfields;
    UA_TimerPolicy timerPolicy;              /* Timer policy to handle cycle misses */
    UA_DateTime nextTime;                    /* The next time when the callback
                                              * is to be executed */
    UA_UInt64 interval;                      /* Interval in 100ns resolution. If
                                              * the interval is zero, the
                                              * callback is not repeated and
                                              * removed after execution. */
    UA_Callback callback;
    void *application;
    void *data;

    ZIP_ENTRY(UA_TimerEntry) idZipfields;
    UA_UInt64 id;                            /* Id of the entry */
} UA_TimerEntry;

ZIP_HEAD(UA_TimerZip, UA_TimerEntry);
typedef struct UA_TimerZip UA_TimerZip;

ZIP_HEAD(UA_TimerIdZip, UA_TimerEntry);
typedef struct UA_TimerIdZip UA_TimerIdZip;

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
    UA_TimerZip timerRoot;     /* The root of the time-sorted zip tree */
    UA_TimerIdZip timerIdRoot; /* The root of the id-sorted zip tree */
    UA_UInt64 timerIdCounter;  /* Generate unique identifiers. Identifiers are always
                                * above zero. */

    /* Linked List of Delayed Callbacks */
    UA_DelayedCallback *delayedCallbacks;

    /* Pointers to registered EventSources */
    UA_EventSource *eventSources;

    /* Registered file descriptors */
    size_t fdsSize;
    UA_RegisteredFD *fds;

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
};

/*********/
/* Timer */
/*********/

/* There may be several entries with the same nextTime in the tree. We give them
 * an absolute order by considering the memory address to break ties. Because of
 * this, the nextTime property cannot be used to lookup specific entries. */
static enum ZIP_CMP
cmpDateTime(const UA_DateTime *a, const UA_DateTime *b) {
    if(*a < *b)
        return ZIP_CMP_LESS;
    if(*a > *b)
        return ZIP_CMP_MORE;
    if(a == b)
        return ZIP_CMP_EQ;
    if(a < b)
        return ZIP_CMP_LESS;
    return ZIP_CMP_MORE;
}

ZIP_PROTOTYPE(UA_TimerZip, UA_TimerEntry, UA_DateTime)
ZIP_IMPL(UA_TimerZip, UA_TimerEntry, zipfields, UA_DateTime, nextTime, cmpDateTime)

/* The identifiers of entries are unique */
static enum ZIP_CMP
cmpId(const UA_UInt64 *a, const UA_UInt64 *b) {
    if(*a < *b)
        return ZIP_CMP_LESS;
    if(*a == *b)
        return ZIP_CMP_EQ;
    return ZIP_CMP_MORE;
}

ZIP_PROTOTYPE(UA_TimerIdZip, UA_TimerEntry, UA_UInt64)
ZIP_IMPL(UA_TimerIdZip, UA_TimerEntry, idZipfields, UA_UInt64, id, cmpId)

static UA_DateTime
calculateNextTime(UA_DateTime currentTime, UA_DateTime baseTime,
                  UA_DateTime interval) {
    /* Take the difference between current and base time */
    UA_DateTime diffCurrentTimeBaseTime = currentTime - baseTime;

    /* Take modulo of the diff time with the interval. This is the duration we
     * are already "into" the current interval. Subtract it from (current +
     * interval) to get the next execution time. */
    UA_DateTime cycleDelay = diffCurrentTimeBaseTime % interval;

    /* Handle the special case where the baseTime is in the future */
    if(UA_UNLIKELY(cycleDelay < 0))
        cycleDelay += interval;

    return currentTime + interval - cycleDelay;
}

UA_StatusCode
UA_EventLoop_addCyclicCallback(UA_EventLoop *el, UA_Callback cb,
                               void *application, void *data, UA_Double interval_ms,
                               UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                               UA_UInt64 *callbackId) {
    /* A callback method needs to be present */
    if(!cb)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* The interval needs to be positive */
    if(interval_ms <= 0.0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UInt64 interval = (UA_UInt64)(interval_ms * UA_DATETIME_MSEC);
    if(interval == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate the repeated callback structure */
    UA_TimerEntry *te = (UA_TimerEntry*)UA_malloc(sizeof(UA_TimerEntry));
    if(!te)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Compute the first time for execution */
    UA_DateTime currentTime = UA_DateTime_nowMonotonic();
    UA_DateTime nextTime;
    if(baseTime == NULL) {
        /* Use "now" as the basetime */
        nextTime = currentTime + (UA_DateTime)interval;
    } else {
        nextTime = calculateNextTime(currentTime, *baseTime, (UA_DateTime)interval);
    }

    /* Set the repeated callback */
    te->interval = interval;
    te->callback = cb;
    te->application = application;
    te->data = data;
    te->nextTime = nextTime;
    te->timerPolicy = timerPolicy;

    /* Insert into the timer */
    UA_LOCK(&el->elMutex);
    te->id = ++el->timerIdCounter;
    if(callbackId)
        *callbackId = te->id;
    ZIP_INSERT(UA_TimerZip, &el->timerRoot, te, ZIP_FFS32(UA_UInt32_random()));
    ZIP_INSERT(UA_TimerIdZip, &el->timerIdRoot, te, ZIP_RANK(te, zipfields));
    UA_UNLOCK(&el->elMutex);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoop_modifyCyclicCallback(UA_EventLoop *el, UA_UInt64 callbackId,
                                  UA_Double interval_ms, UA_DateTime *baseTime,
                                  UA_TimerPolicy timerPolicy) {
    /* The interval needs to be positive */
    if(interval_ms <= 0.0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UInt64 interval = (UA_UInt64)(interval_ms * UA_DATETIME_MSEC);
    if(interval == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Find in the sorted list */
    UA_LOCK(&el->elMutex);
    UA_TimerEntry *te = ZIP_FIND(UA_TimerIdZip, &el->timerIdRoot, &callbackId);
    if(!te) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    ZIP_REMOVE(UA_TimerZip, &el->timerRoot, te);

    /* Compute the next time for execution. The logic is identical to the
     * creation of a new repeated callback. */
    UA_DateTime currentTime = UA_DateTime_nowMonotonic();
    if(baseTime == NULL) {
        /* Use "now" as the basetime */
        te->nextTime = currentTime + (UA_DateTime)interval;
    } else {
        te->nextTime = calculateNextTime(currentTime, *baseTime, (UA_DateTime)interval);
    }

    te->interval = interval;
    te->timerPolicy = timerPolicy;

    ZIP_INSERT(UA_TimerZip, &el->timerRoot, te, ZIP_RANK(te, zipfields));

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoop_removeCyclicCallback(UA_EventLoop *el, UA_UInt64 callbackId) {
    UA_LOCK(&el->elMutex);
    UA_TimerEntry *te = ZIP_FIND(UA_TimerIdZip, &el->timerIdRoot, &callbackId);
    if(!te) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    ZIP_REMOVE(UA_TimerZip, &el->timerRoot, te);
    ZIP_REMOVE(UA_TimerIdZip, &el->timerIdRoot, te);
    UA_UNLOCK(&el->elMutex);
    UA_free(te);
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoop_addDelayedCallback(UA_EventLoop *el, UA_DelayedCallback *dc) {
    UA_LOCK(&el->elMutex);
    dc->next = el->delayedCallbacks;
    el->delayedCallbacks = dc;
    UA_UNLOCK(&el->elMutex);
}

static void
processTimerEntry(
    UA_EventLoop *el, UA_DateTime nowMonotonic,
    UA_TimerEntry *first) {

    /* Reinsert / remove to their new position first. Because the
    * callback can interact with the zip tree and expects the
    * same entries in the root and idRoot trees. */
    ZIP_REMOVE(UA_TimerZip, &el->timerRoot, first);

    /* Set the time for the next execution. Prevent an infinite loop by
    * forcing the execution time in the next iteration.
    *
    * If the timer policy is "CurrentTime", then there is at least the
    * interval between executions. This is used for Monitoreditems, for
    * which the spec says: The sampling interval indicates the fastest rate
    * at which the Server should sample its underlying source for data
    * changes. (Part 4, 5.12.1.2) */
    first->nextTime += (UA_DateTime)first->interval;
    if(first->nextTime < nowMonotonic) {
        if(first->timerPolicy == UA_TIMER_HANDLE_CYCLEMISS_WITH_BASETIME)
            first->nextTime = calculateNextTime(nowMonotonic, first->nextTime,
                                                (UA_DateTime)first->interval);
        else
            first->nextTime = nowMonotonic + (UA_DateTime)first->interval;
    }

    ZIP_INSERT(UA_TimerZip, &el->timerRoot, first, ZIP_RANK(first, zipfields));

    /* Unlock the mutex before dropping into the callback. So that the timer
     * itself can be edited within the callback. When we return, only the
     * pointer to el must still exist. */
    UA_Callback cb = first->callback;
    void *app = first->application;
    void *data = first->data;
    UA_UNLOCK(&el->elMutex);
    cb(app, data);
    UA_LOCK(&el->elMutex);
}

/* Returns the DateTime of the next cylic callback */
static UA_DateTime
processTimer(UA_EventLoop *el, UA_DateTime nowMonotonic) {
    UA_TimerEntry *first = ZIP_MIN(UA_TimerZip, &el->timerRoot);
    while(first && first->nextTime <= nowMonotonic) {

        processTimerEntry(el, nowMonotonic, first);
        first = ZIP_MIN(UA_TimerZip, &el->timerRoot);
    }
    /* Return the timestamp of the earliest next callback */
    return (first) ? first->nextTime : UA_INT64_MAX;
}

static void
freeTimerEntry(UA_TimerEntry *te, void *data) {
    UA_free(te);
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
    UA_LOCK_INIT(&t->elMutex);
    el->logger = logger;
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
        UA_EventLoop_deregisterEventSource(el, es);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    ZIP_ITER(UA_TimerZip, &el->timerRoot, freeTimerEntry, NULL);

    /* Process remaining delayed callbacks */
    processDelayed(el);

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
        res |= es->start(es);
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

    UA_LOG_INFO(el->logger, UA_LOGCATEGORY_EVENTLOOP, "The EventLoop stopped");
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
    UA_FD highestfd = 0;
    for(size_t i = 0; i < el->fdsSize; i++) {
        /* Add to the fd_sets */
        if(el->fds[i].eventMask & UA_POSIX_EVENT_READ)
            FD_SET(el->fds[i].fd, readset);
        if(el->fds[i].eventMask & UA_POSIX_EVENT_WRITE)
            FD_SET(el->fds[i].fd, writeset);
        if(el->fds[i].eventMask & UA_POSIX_EVENT_ERR)
            FD_SET(el->fds[i].fd, errset);

        /* Highest fd? */
        if(el->fds[i].fd > highestfd)
            highestfd = el->fds[i].fd;
    }
    return highestfd;
}

UA_StatusCode
UA_EventLoop_run(UA_EventLoop *el, UA_UInt32 timeout) {
    UA_LOCK(&el->elMutex);

    if(el->state == UA_EVENTLOOPSTATE_FRESH ||
       el->state == UA_EVENTLOOPSTATE_STOPPED) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot iterate a stopped EventLoop");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Process cyclic callbacks */
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime timeToNextCallback = processTimer(el, now);

    UA_DateTime callbackTimeout = timeToNextCallback - now;
    UA_DateTime maxTimeout = timeout * UA_DATETIME_MSEC;

    UA_DateTime usedTimeout = UA_MIN(callbackTimeout, maxTimeout);

    /* Listen on the active file-descriptors (sockets) from the
     * ConnectionManagers */
    fd_set readset, writeset, errset;
    UA_FD highestfd = setFDSets(el, &readset, &writeset, &errset);

    struct timeval tmptv = {usedTimeout / UA_DATETIME_SEC,
                            (usedTimeout % UA_DATETIME_SEC) / UA_DATETIME_USEC };
    if(select(highestfd+1, &readset, &writeset, &errset, &tmptv) < 0) {
        /* We will retry, only log the error */
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(UA_EventLoop_getLogger(el),
                          UA_LOGCATEGORY_EVENTLOOP,
                          "Error during select: %s", errno_str));
        return UA_STATUSCODE_GOOD;
    }

    /* Loop over all registered FD to see if an event arrived. Yes, this is why
     * select is slow for many open sockets. */
    for(size_t i = 0; i < el->fdsSize; i++) {
        UA_RegisteredFD *rfd = &el->fds[i];
        UA_FD fd = rfd->fd;

        /* Error Event */
        if((rfd->eventMask & UA_POSIX_EVENT_ERR) && FD_ISSET(rfd->fd, &errset)) {
            UA_UNLOCK(&el->elMutex);
            rfd->callback(rfd->es, fd, &rfd->fdcontext, UA_POSIX_EVENT_ERR);
            UA_LOCK(&el->elMutex);
            if(i == el->fdsSize || fd != el->fds[i].fd)
                i--; /* The fd has removed itself */
            continue;
        }

        /* Read Event */
        if((rfd->eventMask & UA_POSIX_EVENT_READ) && FD_ISSET(fd, &readset)) {
            UA_UNLOCK(&el->elMutex);
            rfd->callback(rfd->es, fd, &rfd->fdcontext, UA_POSIX_EVENT_READ);
            UA_LOCK(&el->elMutex);
            if(i == el->fdsSize || fd != el->fds[i].fd)
                i--; /* The fd has removed itself */
            continue;
        }

        /* Write Event */
        if((rfd->eventMask & UA_POSIX_EVENT_WRITE) && FD_ISSET(fd, &writeset)) {
            UA_UNLOCK(&el->elMutex);
            rfd->callback(rfd->es, fd, &rfd->fdcontext, UA_POSIX_EVENT_WRITE);
            UA_LOCK(&el->elMutex);
            if(i == el->fdsSize || fd != el->fds[i].fd)
                i--; /* The fd has removed itself */
            continue;
        }
    }

    /* Process and then free registered delayed callbacks */
    processDelayed(el);

    /* Check if the last EventSource was successfully stopped */
    if(el->state == UA_EVENTLOOPSTATE_STOPPING)
        checkClosed(el);

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

/*****************************/
/* Registering Event Sources */
/*****************************/

UA_StatusCode
UA_EventLoop_registerEventSource(UA_EventLoop *el,
                                 UA_EventSource *es) {
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
UA_EventLoop_deregisterEventSource(UA_EventLoop *el,
                                   UA_EventSource *es) {
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
        cb(es, el->fds[i].fd, el->fds[i].fdcontext, 0);
    }
}

/* Helper Functions */

const UA_Logger *
UA_EventLoop_getLogger(UA_EventLoop *el) {
    return el->logger;
}
