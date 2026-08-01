/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "eventloop_glib.h"

#if defined(UA_ENABLE_EVENTLOOP_GLIB) && \
    defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)

#include <glib-unix.h>

/**
 * GLib Interrupt Manager
 * =======================
 * Unlike the POSIX InterruptManager (arch/posix/eventloop_posix_interrupt.c),
 * which installs its own sigaction handler writing into a self-pipe and then
 * registers the read end as a generic fd, this InterruptManager registers
 * each signal directly as a native GLib GSource (g_unix_signal_source_new)
 * attached to the GMainContext of the owning EventLoop. GLib owns the
 * OS-level signal handling -- so unlike the POSIX implementation, no atomic
 * refcounting or "previous action" save/restore is required here.
 *
 * This comes with two GLib-inherent semantic differences from the POSIX
 * variant (verified against plain GLib, independent of open62541):
 *
 * - Delivery is asynchronous. GLib forwards a received signal to the
 *   watching GMainContext via an internal worker thread, so a signal is not
 *   guaranteed to be observed by a single non-blocking main-context
 *   iteration performed immediately after it was raised.
 * - GLib does not fan a signal out to multiple independent watchers of the
 *   same signal number: per GLib's own documented "shared handler"
 *   semantics for g_unix_signal_source_new/g_unix_signal_add, one raised
 *   signal is delivered to only one of several competing GSources for that
 *   signum, not to all of them. Two independent UA_GLibInterruptManager
 *   instances must therefore watch different signal numbers to both
 *   reliably receive their own notifications.
 */

typedef struct UA_RegisteredGLibSignal {
    LIST_ENTRY(UA_RegisteredGLibSignal) listPointers;
    UA_EventSource *eventSource;

    UA_InterruptCallback signalCallback;
    void *context;
    int signal; /* POSIX identifier of the interrupt signal */

    GSource *source; /* NULL unless the EventLoop is started */
} UA_RegisteredGLibSignal;

typedef struct UA_GLibInterruptManager {
    UA_InterruptManager im;
    LIST_HEAD(, UA_RegisteredGLibSignal) signals; /* Registered signals */
} UA_GLibInterruptManager;

static gboolean
glibSignalDispatch(gpointer user_data) {
    UA_RegisteredGLibSignal *rs = (UA_RegisteredGLibSignal*)user_data;
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)rs->eventSource->eventLoop;

    UA_LOCK(&el->elMutex);
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt\t| Received signal %i", rs->signal);
    UA_InterruptManager *im = (UA_InterruptManager*)rs->eventSource;
    UA_InterruptCallback cb = rs->signalCallback;
    void *ctx = rs->context;
    int signal = rs->signal;
    UA_UNLOCK(&el->elMutex);

    cb(im, (uintptr_t)signal, ctx, &UA_KEYVALUEMAP_NULL);
    return G_SOURCE_CONTINUE;
}

static void
activateGLibSignal(UA_RegisteredGLibSignal *rs) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)rs->eventSource->eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    if(rs->source)
        return;

    GSource *source = g_unix_signal_source_new(rs->signal);
    if(!source) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Interrupt\t| Could not register the handler for "
                     "signal %i", rs->signal);
        return;
    }

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
               "Interrupt\t| Registering the handler for signal %i", rs->signal);

    g_source_set_callback(source, glibSignalDispatch, rs, NULL);
    g_source_attach(source, (GMainContext*)el->glibContext);
    rs->source = source;
}

static void
deactivateGLibSignal(UA_RegisteredGLibSignal *rs) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)rs->eventSource->eventLoop;
    UA_LOCK_ASSERT(&el->elMutex);

    if(!rs->source)
        return;

    UA_LOG_INFO(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
               "Interrupt\t| Deregistering the handler for signal %i", rs->signal);

    g_source_destroy(rs->source);
    g_source_unref(rs->source);
    rs->source = NULL;
}

static UA_StatusCode
registerGLibInterrupt(UA_InterruptManager *im, uintptr_t interruptHandle,
                      const UA_KeyValueMap *params,
                      UA_InterruptCallback callback, void *interruptContext) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)im->eventSource.eventLoop;
    if(!UA_KeyValueMap_isEmpty(params)) {
        UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Interrupt\t| Supplied parameters invalid for the "
                     "GLib InterruptManager");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOCK(&el->elMutex);

    /* Was the signal already registered? */
    int signal = (int)interruptHandle;
    UA_GLibInterruptManager *gim = (UA_GLibInterruptManager *)im;
    UA_RegisteredGLibSignal *rs;
    LIST_FOREACH(rs, &gim->signals, listPointers) {
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
    rs = (UA_RegisteredGLibSignal *)UA_calloc(1, sizeof(UA_RegisteredGLibSignal));
    if(!rs) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    rs->eventSource = &im->eventSource;
    rs->signal = signal;
    rs->signalCallback = callback;
    rs->context = interruptContext;

    /* Add to the InterruptManager */
    LIST_INSERT_HEAD(&gim->signals, rs, listPointers);

    /* Activate if we are already running */
    if(gim->im.eventSource.state == UA_EVENTSOURCESTATE_STARTED)
        activateGLibSignal(rs);

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static void
deregisterGLibInterrupt(UA_InterruptManager *im, uintptr_t interruptHandle) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)im->eventSource.eventLoop;
    UA_GLibInterruptManager *gim = (UA_GLibInterruptManager *)im;
    UA_LOCK(&el->elMutex);

    int signal = (int)interruptHandle;
    UA_RegisteredGLibSignal *rs;
    LIST_FOREACH(rs, &gim->signals, listPointers) {
        if(rs->signal == signal)
            break;
    }
    if(rs) {
        deactivateGLibSignal(rs);
        LIST_REMOVE(rs, listPointers);
        UA_free(rs);
    }

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
startGLibInterruptManager(UA_EventSource *es) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)es->eventLoop;
    UA_LOCK(&el->elMutex);

    /* Check the state */
    if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_ERROR(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Interrupt\t| To start the InterruptManager, "
                     "it has to be registered in an EventLoop and not started");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_DEBUG(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt\t| Starting the InterruptManager");

    /* Activate the registered signal handlers */
    UA_GLibInterruptManager *gim = (UA_GLibInterruptManager *)es;
    UA_RegisteredGLibSignal *rs;
    LIST_FOREACH(rs, &gim->signals, listPointers) {
        activateGLibSignal(rs);
    }

    /* Set the EventSource to the started state */
    es->state = UA_EVENTSOURCESTATE_STARTED;

    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static void
stopGLibInterruptManager(UA_EventSource *es) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX *)es->eventLoop;
    UA_LOCK(&el->elMutex);

    if(es->state != UA_EVENTSOURCESTATE_STARTED) {
        UA_UNLOCK(&el->elMutex);
        return;
    }

    UA_LOG_DEBUG(es->eventLoop->logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Interrupt\t| Stopping the InterruptManager");

    /* Deactivate all registered signals */
    UA_GLibInterruptManager *gim = (UA_GLibInterruptManager *)es;
    UA_RegisteredGLibSignal *rs;
    LIST_FOREACH(rs, &gim->signals, listPointers) {
        deactivateGLibSignal(rs);
    }

    /* Immediately set to stopped */
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    UA_UNLOCK(&el->elMutex);
}

static UA_StatusCode
freeGLibInterruptManager(UA_EventSource *es) {
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
    UA_GLibInterruptManager *gim = (UA_GLibInterruptManager *)es;
    UA_RegisteredGLibSignal *rs, *rs_tmp;
    LIST_FOREACH_SAFE(rs, &gim->signals, listPointers, rs_tmp) {
        deactivateGLibSignal(rs);
        LIST_REMOVE(rs, listPointers);
        UA_free(rs);
    }

    UA_String_clear(&es->name);
    UA_free(es);

    return UA_STATUSCODE_GOOD;
}

UA_InterruptManager *
UA_InterruptManager_new_GLib(const UA_String eventSourceName) {
    UA_GLibInterruptManager *gim = (UA_GLibInterruptManager *)
        UA_calloc(1, sizeof(UA_GLibInterruptManager));
    if(!gim)
        return NULL;

    UA_InterruptManager *im = &gim->im;
    im->eventSource.eventSourceType = UA_EVENTSOURCETYPE_INTERRUPTMANAGER;
    UA_String_copy(&eventSourceName, &im->eventSource.name);
    im->eventSource.start = startGLibInterruptManager;
    im->eventSource.stop = stopGLibInterruptManager;
    im->eventSource.free = freeGLibInterruptManager;
    im->registerInterrupt = registerGLibInterrupt;
    im->deregisterInterrupt = deregisterGLibInterrupt;
    return im;
}

#endif /* UA_ENABLE_EVENTLOOP_GLIB && UA_ARCHITECTURE_POSIX && !UA_ARCHITECTURE_LWIP */
