/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021, 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "eventloop_posix.h"
#include <signal.h>

/* Different implementation approaches:
 * - Linux: Use signalfd
 * - Other: Use the self-pipe trick (http://cr.yp.to/docs/selfpipe.html) */

typedef struct UA_RegisteredSignal {
#ifdef UA_HAVE_EPOLL
    /* With epoll, register each signal with a socket.
     * This has to be the first element of the struct to allow casting. */
    UA_RegisteredFD rfd;
#endif

    LIST_ENTRY(UA_RegisteredSignal) listPointers;

    UA_InterruptCallback signalCallback;
    void *context;
    int signal; /* POSIX identifier of the interrupt signal */

    UA_Boolean active; /* Signals are only active when the EventLoop is started */
    UA_Boolean triggered;
} UA_RegisteredSignal;

typedef struct {
    UA_InterruptManager im;

    LIST_HEAD(, UA_RegisteredSignal) signals; /* Registered signals */

#ifndef UA_HAVE_EPOLL
    UA_DelayedCallback dc; /* Process all triggered signals */
#endif
} UA_POSIXInterruptManager;

/* The following methods have to be implemented for epoll/self-pipe each. */
static void activateSignal(UA_RegisteredSignal *rs);
static void deactivateSignal(UA_RegisteredSignal *rs);

#ifdef UA_HAVE_EPOLL
#include <sys/signalfd.h>

static void
handlePOSIXInterruptEvent(UA_EventSource *es, UA_RegisteredFD *rfd, short event) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)es->eventLoop;
    (void)el;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    UA_RegisteredSignal *rs = (UA_RegisteredSignal*)rfd;
    struct signalfd_siginfo fdsi;
    ssize_t s = read(rfd->fd, &fdsi, sizeof(fdsi));
    if(s < (ssize_t)sizeof(fdsi)) {
        /* A problem occured */
        deactivateSignal(rs);
        return;
    }

    /* Signal received */
    UA_LOG_DEBUG(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt %u\t| Received a signal %u",
                 (unsigned)rfd->fd, fdsi.ssi_signo);

    UA_UNLOCK(&el->elMutex);
    rs->signalCallback((UA_InterruptManager *)es, (uintptr_t)rfd->fd,
                       rs->context, &UA_KEYVALUEMAP_NULL);
    UA_LOCK(&el->elMutex);
}

static void
activateSignal(UA_RegisteredSignal *rs) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)rs->rfd.es->eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    if(rs->active)
        return;

    /* Block the normal signal handling */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, rs->signal);
    int res2 = sigprocmask(SIG_BLOCK, &mask, NULL);
    if(res2 == -1) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Interrupt\t| Could not block the default "
                           "signal handling with an error: %s",
                           errno_str));
        return;
    }

    /* Create the fd */
    UA_FD newfd = signalfd(-1, &mask, 0);
    if(newfd < 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Interrupt\t| Could not create a signal file "
                           "description with error: %s",
                           errno_str));
        sigprocmask(SIG_UNBLOCK, &mask, NULL); /* restore signal */
        return;
    }

    rs->rfd.fd = newfd;
    rs->rfd.eventSourceCB = handlePOSIXInterruptEvent;
    rs->rfd.listenEvents = UA_FDEVENT_IN;

    /* Register the fd in the EventLoop */
    UA_StatusCode res = UA_EventLoopPOSIX_registerFD(el, &rs->rfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Interrupt\t| Could not register the a signal file "
                       "description in the EventLoop");
        UA_close(newfd);
        sigprocmask(SIG_UNBLOCK, &mask, NULL); /* restore signal */
        return;
    }

    rs->active = true;
}

static void
deactivateSignal(UA_RegisteredSignal *rs) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)rs->rfd.es->eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    /* Only dectivate if active */
    if(!rs->active)
        return;
    rs->active = false;

    /* Stop receiving the signal on the FD */
    UA_EventLoopPOSIX_deregisterFD(el, &rs->rfd);

    /* Unblock the signal */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, (int)rs->signal);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    /* Clean up locally */
    UA_close(rs->rfd.fd);
}

#else /* !UA_HAVE_EPOLL */

/* When using signal() a global pointer to the interrupt manager is required.
 * We have no other we to get additional data with the interrupt. */
static UA_POSIXInterruptManager *singletonIM = NULL;

/* Execute all triggered interrupts in a delayed callback by the EventLoop */
static void
executeTriggeredPOSIXInterrupts(UA_POSIXInterruptManager *im, void *_) {
    im->dc.callback = NULL; /* Allow to re-arm the delayed callback */

    UA_RegisteredSignal *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &im->signals, listPointers, rs_tmp) {
        rs->triggered = false;
        rs->signalCallback(&im->im, (uintptr_t)rs->signal,
                           rs->context, &UA_KEYVALUEMAP_NULL);
    }
}

/* Mark the signal entry as triggered and make the EventLoop process it "soon"
 * with a delayed callback. This is an interrupt handler and cannot take a
 * lock. */
static void
triggerPOSIXInterruptEvent(int sig) {
    UA_assert(singletonIM != NULL);

    /* Find the signal */
    UA_RegisteredSignal *rs;
    LIST_FOREACH(rs, &singletonIM->signals, listPointers) {
        if(rs->signal == sig)
            break;
    }
    if(!rs || rs->triggered || !rs->active)
        return;

    /* Mark as triggered */
    rs->triggered = true;

#ifdef _WIN32
    /* On WIN32 we have to re-arm the signal or it will go back to SIG_DFL */
    signal(sig, triggerPOSIXInterruptEvent);
#endif

    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)singletonIM->im.eventSource.eventLoop;

    /* Arm the delayed callback for processing the received signals */
    if(!singletonIM->dc.callback) {
        singletonIM->dc.callback = (UA_Callback)executeTriggeredPOSIXInterrupts;
        singletonIM->dc.application = singletonIM;
        singletonIM->dc.context = NULL;
        UA_EventLoopPOSIX_addDelayedCallback(&el->eventLoop, &singletonIM->dc);
    }

    /* Cancel the EventLoop if it is currently waiting with a timeout */
    UA_EventLoopPOSIX_cancel(el);
}

static void
activateSignal(UA_RegisteredSignal *rs) {
    UA_assert(singletonIM != NULL);
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)singletonIM->im.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    /* Register the signal on the OS level */
    if(rs->active)
        return;

    void (*prev)(int);
    prev = signal(rs->signal, triggerPOSIXInterruptEvent);
    if(prev == SIG_ERR) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(singletonIM->im.eventSource.eventLoop->logger,
                          UA_LOGCATEGORY_EVENTLOOP,
                          "Error registering the signal: %s", errno_str));
        return;
    }

    rs->active = true;
}

static void
deactivateSignal(UA_RegisteredSignal *rs) {
    UA_assert(singletonIM != NULL);
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)singletonIM->im.eventSource.eventLoop;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    /* Only dectivate if active */
    if(!rs->active)
        return;

    /* Stop receiving the signal */
    signal(rs->signal, SIG_DFL);

    rs->triggered = false;
    rs->active = false;
}

#endif /* !UA_HAVE_EPOLL */

static UA_StatusCode
registerPOSIXInterrupt(UA_InterruptManager *im, uintptr_t interruptHandle,
                       const UA_KeyValueMap *params,
                       UA_InterruptCallback callback, void *interruptContext) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)im->eventSource.eventLoop;
    if(!UA_KeyValueMap_isEmpty(params)) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Interrupt\t| Supplied parameters invalid for the "
                     "POSIX InterruptManager");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOCK(&el->elMutex);

    /* Was the signal already registered? */
    int signal = (int)interruptHandle;
    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager *)im;
    UA_RegisteredSignal *rs;
    LIST_FOREACH(rs, &pim->signals, listPointers) {
        if(rs->signal == signal)
            break;
    }
    if(rs) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Interrupt\t| Signal %u already registered",
                       (unsigned)interruptHandle);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Create and populate the new context object */
    rs = (UA_RegisteredSignal *)UA_calloc(1, sizeof(UA_RegisteredSignal));
    if(!rs) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

#ifdef UA_HAVE_EPOLL
    rs->rfd.es = &im->eventSource;
#endif
    rs->signal = (int)interruptHandle;
    rs->signalCallback = callback;
    rs->context = interruptContext;

    /* Add to the InterruptManager */
    LIST_INSERT_HEAD(&pim->signals, rs, listPointers);

    /* Activate if we are already running */
    if(pim->im.eventSource.state == UA_EVENTSOURCESTATE_STARTED)
        activateSignal(rs);

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static void
deregisterPOSIXInterrupt(UA_InterruptManager *im, uintptr_t interruptHandle) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)im->eventSource.eventLoop;
    (void)el;
    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager *)im;
    UA_LOCK(&el->elMutex);

    int signal = (int)interruptHandle;
    UA_RegisteredSignal *rs;
    LIST_FOREACH(rs, &pim->signals, listPointers) {
        if(rs->signal == signal)
            break;
    }
    if(rs) {
        deactivateSignal(rs);
        LIST_REMOVE(rs, listPointers);
        UA_free(rs);
    }

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
startPOSIXInterruptManager(UA_EventSource *es) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)es->eventLoop;
    (void)el;
    UA_LOCK(&el->elMutex);

    /* Check the state */
    if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Interrupt\t| To start the InterruptManager, "
                     "it has to be registered in an EventLoop and not started");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager *)es;
    UA_LOG_DEBUG(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt\t| Starting the InterruptManager");

    /* Activate the registered signal handlers */
    UA_RegisteredSignal*rs;
    LIST_FOREACH(rs, &pim->signals, listPointers) {
        activateSignal(rs);
    }

    /* Set the EventSource to the started state */
    es->state = UA_EVENTSOURCESTATE_STARTED;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static void
stopPOSIXInterruptManager(UA_EventSource *es) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)es->eventLoop;
    (void)el;
    UA_LOCK(&el->elMutex);

    if(es->state != UA_EVENTSOURCESTATE_STARTED) {
        UA_UNLOCK(&el->elMutex);
        return;
    }

    UA_LOG_DEBUG(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt\t| Stopping the InterruptManager");

    /* Close all registered signals */
    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager *)es;
    UA_RegisteredSignal*rs;
    LIST_FOREACH(rs, &pim->signals, listPointers) {
        deactivateSignal(rs);
    }

    /* Immediately set to stopped */
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
freePOSIXInterruptmanager(UA_EventSource *es) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)es->eventLoop;
    (void)el;
    UA_LOCK_ASSERT(&el->elMutex, 1);

    if(es->state >= UA_EVENTSOURCESTATE_STARTING) {
        UA_LOG_ERROR(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Interrupt\t| The EventSource must be stopped "
                     "before it can be deleted");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Deactivate and remove all registered signals */
    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager *)es;
    UA_RegisteredSignal *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &pim->signals, listPointers, rs_tmp) {
        deactivateSignal(rs);
        LIST_REMOVE(rs, listPointers);
        UA_free(rs);
    }

    UA_String_clear(&es->name);
    UA_free(es);

#ifndef UA_HAVE_EPOLL
    singletonIM = NULL; /* Reset the global singleton pointer */
#endif

    return UA_STATUSCODE_GOOD;
}

UA_InterruptManager *
UA_InterruptManager_new_POSIX(const UA_String eventSourceName) {
#ifndef UA_HAVE_EPOLL
    /* There can be only one InterruptManager if epoll is not present */
    if(singletonIM)
        return NULL;
#endif

    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager *)
        UA_calloc(1, sizeof(UA_POSIXInterruptManager));
    if(!pim)
        return NULL;

#ifndef UA_HAVE_EPOLL
    singletonIM = pim; /* Register the singleton singleton pointer */
#endif

    UA_InterruptManager *im = &pim->im;
    im->eventSource.eventSourceType = UA_EVENTSOURCETYPE_INTERRUPTMANAGER;
    UA_String_copy(&eventSourceName, &im->eventSource.name);
    im->eventSource.start = startPOSIXInterruptManager;
    im->eventSource.stop = stopPOSIXInterruptManager;
    im->eventSource.free = freePOSIXInterruptmanager;
    im->registerInterrupt = registerPOSIXInterrupt;
    im->deregisterInterrupt = deregisterPOSIXInterrupt;
    return im;
}
