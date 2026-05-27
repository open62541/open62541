/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021, 2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include "eventloop_posix.h"
#include <signal.h>

#if defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP) || defined(UA_ARCHITECTURE_WIN32)

#define UA_MAX_INTERRUPT_MANAGERS 8
#ifdef NSIG
# define UA_INTERRUPT_SIGNAL_SLOTS NSIG
#else
# define UA_INTERRUPT_SIGNAL_SLOTS 128
#endif

typedef struct UA_RegisteredSignal {
    LIST_ENTRY(UA_RegisteredSignal) listPointers;
    UA_EventSource *eventSource;

    UA_InterruptCallback signalCallback;
    void *context;
    int signal; /* POSIX identifier of the interrupt signal */

    UA_Boolean active; /* Signals are only active when the EventLoop is started */
} UA_RegisteredSignal;

typedef struct UA_POSIXInterruptManager {
    UA_InterruptManager im;

    LIST_HEAD(, UA_RegisteredSignal) signals; /* Registered signals */

    UA_RegisteredFD readfd;
    UA_FD writefd;
    UA_atomic(struct UA_POSIXInterruptManager*)* managerSlot;
} UA_POSIXInterruptManager;

/* Signal handlers are process-global. Route each signal marker to all active
 * interrupt managers so multiple EventLoops can coexist. */
static UA_atomic(UA_POSIXInterruptManager*) interruptManagers[UA_MAX_INTERRUPT_MANAGERS];
static UA_atomic(uintptr_t) interruptWriteFDs[UA_INTERRUPT_SIGNAL_SLOTS][UA_MAX_INTERRUPT_MANAGERS];
static UA_atomic(uintptr_t) signalRefCounts[UA_INTERRUPT_SIGNAL_SLOTS];

/* Keep the previous action to restore when we deregister a signal */
#ifdef UA_ARCHITECTURE_WIN32
static void (*previousActions[UA_INTERRUPT_SIGNAL_SLOTS])(int);
#else
static struct sigaction previousActions[UA_INTERRUPT_SIGNAL_SLOTS];
#endif

static UA_Boolean
signalIsSupported(int signal) {
    return (signal >= 0 &&
            signal < UA_INTERRUPT_SIGNAL_SLOTS &&
            signal <= UCHAR_MAX);
}

static UA_Boolean
registerInterruptManager(UA_POSIXInterruptManager *pim) {
    for(size_t i = 0; i < UA_MAX_INTERRUPT_MANAGERS; i++) {
        UA_POSIXInterruptManager *prev = NULL;
        UA_atomic_cmpxchg(&interruptManagers[i], &prev, pim);
        if(prev == NULL) {
            pim->managerSlot = &interruptManagers[i];
            return true;
        }
    }
    return false;
}

static void
unregisterInterruptManager(UA_POSIXInterruptManager *pim) {
    UA_assert(*pim->managerSlot == pim);
    UA_atomic_store(pim->managerSlot, NULL);
    pim->managerSlot = NULL;
}

static void
closeSignalPipe(UA_POSIXInterruptManager *pim) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pim->im.eventSource.eventLoop;
    (void)el;
    UA_LOCK_ASSERT(&el->elMutex);

    if(pim->readfd.fd == UA_INVALID_FD)
        return;

    UA_EventLoopPOSIX_deregisterFD(el, &pim->readfd);
    UA_close(pim->readfd.fd);
    UA_close(pim->writefd);
    pim->readfd.fd = UA_INVALID_FD;
    pim->writefd = UA_INVALID_FD;
}

static void
handlePOSIXInterruptEvent(UA_EventSource *es, UA_RegisteredFD *rfd,
                          short event) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)es->eventLoop;
    (void)el;
    (void)event;
    UA_LOCK_ASSERT(&el->elMutex);

    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager*)es;
    unsigned char buf[32];

    for(;;) {
        ssize_t received = UA_recv(rfd->fd, (char*)buf, sizeof(buf), 0);
        if(received > 0) {
            for(ssize_t i = 0; i < received; i++) {
                UA_RegisteredSignal *rs;
                LIST_FOREACH(rs, &pim->signals, listPointers) {
                    if(rs->signal == (int)buf[i] && rs->active)
                        break;
                }
                if(!rs)
                    continue;

                UA_LOG_DEBUG(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                             "Interrupt %u\t| Received signal %u",
                             (unsigned)rfd->fd, (unsigned)rs->signal);
                UA_UNLOCK(&el->elMutex);
                rs->signalCallback((UA_InterruptManager *)es,
                                   (uintptr_t)rs->signal, rs->context,
                                   &UA_KEYVALUEMAP_NULL);
                UA_LOCK(&el->elMutex);
            }
            continue;
        }

        if(received == 0) {
            closeSignalPipe(pim);
            return;
        }

        if(UA_ERRNO == UA_INTERRUPTED)
            continue;

        if(UA_ERRNO == UA_AGAIN || UA_ERRNO == UA_WOULDBLOCK)
            return;

        closeSignalPipe(pim);
        return;
    }
}

static UA_StatusCode
openSignalPipe(UA_POSIXInterruptManager *pim) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)pim->im.eventSource.eventLoop;
    (void)el;
    UA_LOCK_ASSERT(&el->elMutex);

    if(pim->readfd.fd != UA_INVALID_FD)
        return UA_STATUSCODE_GOOD;

    UA_FD fds[2];
    int err = UA_EventLoopPOSIX_pipe(fds);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                          "Interrupt\t| Could not create the signal pipe: %s",
                          errno_str));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    pim->readfd.es = &pim->im.eventSource;
    pim->readfd.eventSourceCB = handlePOSIXInterruptEvent;
    pim->readfd.listenEvents = UA_FDEVENT_IN;
    pim->readfd.fd = fds[0];
    pim->writefd = fds[1];

    UA_StatusCode res = UA_EventLoopPOSIX_registerFD(el, &pim->readfd);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Interrupt\t| Could not register the signal pipe in "
                       "the EventLoop");
        UA_close(pim->readfd.fd);
        UA_close(pim->writefd);
        pim->readfd.fd = UA_INVALID_FD;
        pim->writefd = UA_INVALID_FD;
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

/* Signal handlers can only perform async-signal-safe operations. Writing one
 * byte into the signal-specific pipe wakes up the EventLoop. */
static void
triggerPOSIXInterruptEvent(int sig) {
    if(!signalIsSupported(sig))
        return;

#ifdef UA_ARCHITECTURE_WIN32
    /* On Win32 the signal handler is one-shot and must be re-armed. */
    signal(sig, triggerPOSIXInterruptEvent);
#endif

    int savedErrno = errno;
    unsigned char signalMarker = (unsigned char)sig;

    for(size_t i = 0; i < UA_MAX_INTERRUPT_MANAGERS; i++) {
        uintptr_t encodedFD = UA_atomic_load(&interruptWriteFDs[sig][i]);
        UA_FD writefd = UA_INVALID_FD;
        if(encodedFD != 0)
            writefd = (UA_FD)(encodedFD - 1u);
        if(writefd == UA_INVALID_FD)
            continue;

        ssize_t res;
        do {
            res = UA_send(writefd, (const char*)&signalMarker, 1, 0);
        } while(res == -1 && UA_ERRNO == UA_INTERRUPTED);
    }

    errno = savedErrno;
}

static void
activateSignal(UA_RegisteredSignal *rs) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)rs->eventSource->eventLoop;
    (void)el;
    UA_LOCK_ASSERT(&el->elMutex);

    if(rs->active)
        return;

    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager*)rs->eventSource;
    UA_assert(pim->managerSlot != NULL);

    size_t managerSlot = (size_t)(pim->managerSlot - interruptManagers);

    /* Store the fd to be picked up by triggerPOSIXInterruptEvent */
    rs->active = true;
    UA_atomic_store(&interruptWriteFDs[rs->signal][managerSlot],
                    ((uintptr_t)pim->writefd) + 1u);

    /* signalRefCounts is an atomic state machine:
     *   0                         -> signal handler not installed
     *   1..N                      -> installed and referenced by N managers
     *   >= UINT16_MAX             -> handler is being installed (sentinel)
     * Only the thread that flips 0 -> UINT16_MAX installs the handler.
     * Other threads can increment the refcount even if the sentinel is present. */
    for(;;) {
        uintptr_t refCount = UA_atomic_load(&signalRefCounts[rs->signal]);

        /* Interrupt installing or already installed. Just increment. */
        if(refCount > 0) {
            uintptr_t expected = refCount;
            UA_atomic_cmpxchg(&signalRefCounts[rs->signal], &expected, refCount + 1);
            if(expected != refCount)
                continue; /* Another thread changed the refCount. Retry. */
            return; /* Done */
        }

        /* Set the sentinel */
        uintptr_t expected = 0;
        UA_atomic_cmpxchg(&signalRefCounts[rs->signal], &expected, UINT16_MAX);
        if(expected != 0)
            continue;  /* Another thread changed the refCount. Retry. */
        break; /* This thread installs the interrupt handler */
    }

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt %i\t| Register Wait %u", rs->signal,
                 signalRefCounts[rs->signal]);

    /* From here on we are the first (and only) thread to register the signal.
     * There might still be an ongoing deactivateSignal which reached
     * refCount==0 just before. Loop until previousAction is zeroed to make sure
     * no deactivateSignal is ongoing. */
    UA_atomic(uintptr_t)* action_sentinel =
        (UA_atomic(uintptr_t)*)&previousActions[rs->signal];
    for(;;) {
        if(UA_atomic_load(action_sentinel) == 0)
            break;
    }

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt %i\t| Register Begin %u", rs->signal,
                 signalRefCounts[rs->signal]);

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "Interrupt\t| Registering the handler for signal %i", rs->signal);

    /* Install the interrupt handler */
#ifdef UA_ARCHITECTURE_WIN32
    UA_RESET_ERRNO;
    void (*prev)(int) = signal(rs->signal, triggerPOSIXInterruptEvent);
    if(prev != SIG_ERR) {
        /* Store previous action with non-NULL value.
         * Use 0x01 to indicate no previous action was stored. */
        if(prev == NULL)
            prev = (void (*)(int))0x01;
        previousActions[rs->signal] = prev;
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                        "Interrupt\t| Could not register the signal handler: %s",
                        errno_str));
    }
#else
    struct sigaction previousAction;
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = triggerPOSIXInterruptEvent;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if(sigaction(rs->signal, &action, &previousAction) == 0) {
        /* Store previous action with non-NULL value.
         * Use 0x01 to indicate no previous action was stored. */
        uintptr_t *prevContent = (uintptr_t*)&previousAction;
        if(*prevContent == 0)
            *prevContent = 0x01;
        previousActions[rs->signal] = previousAction;
    } else {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                        "Interrupt\t| Could not register the signal handler: %s",
                        errno_str));
    }
#endif

    /* Subtract UINT16_MAX - 1 for the final refcount */
    for(;;) {
        uintptr_t refCount = UA_atomic_load(&signalRefCounts[rs->signal]);
        UA_assert(refCount >= UA_UINT16_MAX);
        uintptr_t expected = refCount;
        UA_atomic_cmpxchg(&signalRefCounts[rs->signal], &expected,
                          refCount - (UA_UINT16_MAX - 1u));
        if(expected == refCount)
            break;
    }

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt %i\t| Register End %u", rs->signal,
                 signalRefCounts[rs->signal]);
}

static void
deactivateSignal(UA_RegisteredSignal *rs) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)rs->eventSource->eventLoop;
    (void)el;
    UA_LOCK_ASSERT(&el->elMutex);

    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager*)rs->eventSource;
    UA_assert(pim->managerSlot != NULL);

    if(!rs->active)
        return;

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt %i\t| Register Decrease Begin %u", rs->signal,
                 signalRefCounts[rs->signal]);

    size_t managerSlot = (size_t)(pim->managerSlot - interruptManagers);

    /* Stop routing new signals to this interrupt manager */
    UA_atomic_store(&interruptWriteFDs[rs->signal][managerSlot], 0);
    rs->active = false;

    /* Decrease the refcount */
    uintptr_t refCount;
    for(;;) {
        refCount = UA_atomic_load(&signalRefCounts[rs->signal]);
        uintptr_t expected = refCount;
        UA_atomic_cmpxchg(&signalRefCounts[rs->signal], &expected, refCount - 1);
        if(expected == refCount)
            break;
    }
    refCount--; /* Adjust, this is still the old refCount before decreasing */

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt %i\t| Register Decrease End %u", rs->signal,
                 signalRefCounts[rs->signal]);

    /* Another interrupt manager still uses the signal */
    if(refCount > 0)
        return;

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt %i\t| Deregister Begin", rs->signal);

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                "Interrupt\t| Deregistering the handler for signal %i", rs->signal);

    /* Deactivate the signal, reset to the previous action */
#ifdef UA_ARCHITECTURE_WIN32
    void (*prev)(int) = previousActions[rs->signal];
    if((uintptr_t)prev == (uintptr_t)0x01)
        prev = NULL;
    signal(rs->signal, prev);
#else
    struct sigaction prev = previousActions[rs->signal];
    uintptr_t *prevContent = (uintptr_t*)&prev;
    if(*prevContent == 0x01)
        *prevContent = 0x0;
    sigaction(rs->signal, &prev, NULL);
#endif

    /* Zero out the previousAction. So an activateSignal that has already
     * started knows we are done. */
    UA_atomic(uintptr_t)* action_sentinel =
        (UA_atomic(uintptr_t)*)&previousActions[rs->signal];
    UA_atomic_store(action_sentinel, 0);

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt %i\t| Deregister End", rs->signal);
}

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

    if(!signalIsSupported(signal)) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Interrupt\t| Signal %u is out of range",
                     (unsigned)interruptHandle);
        UA_free(rs);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set the callback and context */
    rs->eventSource = &im->eventSource;
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

    UA_StatusCode res = openSignalPipe(pim);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&el->elMutex);
        return res;
    }

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

    closeSignalPipe(pim);

    /* Immediately set to stopped */
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
freePOSIXInterruptmanager(UA_EventSource *es) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)es->eventLoop;
    (void)el;
    UA_LOCK_ASSERT(&el->elMutex);

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
    unregisterInterruptManager(pim);
    UA_free(es);

    return UA_STATUSCODE_GOOD;
}

UA_InterruptManager *
UA_InterruptManager_new_POSIX(const UA_String eventSourceName) {
    UA_POSIXInterruptManager *pim = (UA_POSIXInterruptManager *)
        UA_calloc(1, sizeof(UA_POSIXInterruptManager));
    if(!pim)
        return NULL;

    /* Initialize the FD as invalid before adding the interrupt manager to the
     * global array */
    pim->readfd.fd = UA_INVALID_FD;
    pim->writefd = UA_INVALID_FD;
    pim->managerSlot = NULL;

    if(!registerInterruptManager(pim)) {
        UA_free(pim);
        return NULL;
    }

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

#endif
