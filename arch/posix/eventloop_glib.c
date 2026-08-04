/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "eventloop_glib.h"

#if defined(UA_ENABLE_EVENTLOOP_GLIB) && defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)

/**
 * GLib EventLoop
 * ==============
 * This EventLoop implementation reuses the generic parts of the "POSIX"
 * EventLoop (arch/posix/eventloop_posix.c) -- the timer, the delayed-callback
 * queue, the EventSource lifecycle and the ConnectionManagers (TCP, UDP,
 * Ethernet, ...) are all shared. What differs is *how* the EventLoop waits
 * for file-descriptor readiness and timer expiry: instead of calling
 * select()/epoll_wait() from within a blocking `run()` method, the whole
 * EventLoop is exposed as a GSource that is attached to a GMainContext.
 *
 * Once started, the GSource's prepare/check/dispatch callbacks service
 * open62541's sockets and timers whenever the GMainContext they are attached
 * to is iterated -- e.g. via g_main_loop_run(), gtk_main(),
 * g_application_run(), or any other GLib-based application main loop. It is
 * therefore *not* necessary to call `run()` on this EventLoop at all as long
 * as something else drives the GMainContext. `run()` is still provided for
 * backwards compatibility with code that pumps the EventLoop itself (e.g.
 * UA_Server_run); it performs a single bounded iteration of the
 * GMainContext.
 */

/* A GSource with a back-pointer to the EventLoop it drives. GLib requires
 * the GSource to be the first member so it can freely up/down-cast. */
typedef struct {
    GSource source;
    UA_EventLoopPOSIX *el;
} UA_EventLoopGLibSource;

static void checkClosedGLib(UA_EventLoopPOSIX *el);

/**********************/
/* FD Poll Management */
/**********************/

static UA_StatusCode
registerFD_glib(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Registering fd: %u", (unsigned)rfd->fd);

    GPollFD *gfd = (GPollFD*)UA_malloc(sizeof(GPollFD));
    if(!gfd)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    gfd->fd = (gint)rfd->fd;
    gfd->events = 0;
    if(rfd->listenEvents & UA_FDEVENT_IN)
        gfd->events |= G_IO_IN;
    if(rfd->listenEvents & UA_FDEVENT_OUT)
        gfd->events |= G_IO_OUT;
    gfd->revents = 0;

    /* Track the fd in a flat array (like the "select" backend) so dispatch
     * can iterate all registered fds. Realloc here is safe -- unlike the
     * GPollFD itself, nothing outside of this array holds a pointer into
     * it. */
    UA_RegisteredFD **fds_tmp = (UA_RegisteredFD**)
        UA_realloc(el->fds, sizeof(UA_RegisteredFD*) * (el->fdsSize + 1));
    if(!fds_tmp) {
        UA_free(gfd);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    el->fds = fds_tmp;
    el->fds[el->fdsSize] = rfd;
    el->fdsSize++;

    rfd->glibPollFD = gfd;
    if(el->glibSource)
        g_source_add_poll((GSource*)el->glibSource, gfd);

    /* Wake a context that is currently blocked in poll() so the new fd is
     * considered immediately instead of only at the next scheduled
     * wakeup. */
    if(el->glibContext)
        g_main_context_wakeup((GMainContext*)el->glibContext);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
modifyFD_glib(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
    GPollFD *gfd = (GPollFD*)rfd->glibPollFD;
    UA_assert(gfd != NULL);
    gfd->events = 0;
    if(rfd->listenEvents & UA_FDEVENT_IN)
        gfd->events |= G_IO_IN;
    if(rfd->listenEvents & UA_FDEVENT_OUT)
        gfd->events |= G_IO_OUT;
    if(el->glibContext)
        g_main_context_wakeup((GMainContext*)el->glibContext);
    return UA_STATUSCODE_GOOD;
}

static void
deregisterFD_glib(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Unregistering fd: %u", (unsigned)rfd->fd);

    /* Find and remove the entry from the flat array */
    size_t i = 0;
    for(; i < el->fdsSize; i++) {
        if(el->fds[i] == rfd)
            break;
    }
    if(i < el->fdsSize) {
        if(el->fdsSize > 1) {
            el->fdsSize--;
            el->fds[i] = el->fds[el->fdsSize];
            UA_RegisteredFD **fds_tmp = (UA_RegisteredFD**)
                UA_realloc(el->fds, sizeof(UA_RegisteredFD*) * el->fdsSize);
            if(fds_tmp)
                el->fds = fds_tmp;
        } else {
            UA_free(el->fds);
            el->fds = NULL;
            el->fdsSize = 0;
        }
    }

    GPollFD *gfd = (GPollFD*)rfd->glibPollFD;
    if(gfd) {
        if(el->glibSource)
            g_source_remove_poll((GSource*)el->glibSource, gfd);
        UA_free(gfd);
        rfd->glibPollFD = NULL;
    }

    if(el->glibContext)
        g_main_context_wakeup((GMainContext*)el->glibContext);
}

/***********************/
/* GSource Vtable */
/***********************/

static gboolean
glibSourcePrepare(GSource *source, gint *timeout_) {
    UA_EventLoopPOSIX *el = ((UA_EventLoopGLibSource*)source)->el;
    UA_LOCK(&el->elMutex);
    UA_DateTime now = el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    UA_DateTime next = UA_EventLoopPOSIX_nextTimer(&el->eventLoop);
    UA_UNLOCK(&el->elMutex);

    UA_DateTime diff = next - now;
    if(diff <= 0) {
        if(timeout_)
            *timeout_ = 0;
        return TRUE;
    }

    gint64 ms = diff / UA_DATETIME_MSEC;
    if(ms > G_MAXINT)
        ms = G_MAXINT;
    if(timeout_)
        *timeout_ = (gint)ms;
    return FALSE;
}

static gboolean
glibSourceCheck(GSource *source) {
    UA_EventLoopPOSIX *el = ((UA_EventLoopGLibSource*)source)->el;
    UA_LOCK(&el->elMutex);

    UA_Boolean ready = false;
    for(size_t i = 0; i < el->fdsSize; i++) {
        GPollFD *gfd = (GPollFD*)el->fds[i]->glibPollFD;
        if(gfd && gfd->revents != 0) {
            ready = true;
            break;
        }
    }

    if(!ready) {
        UA_DateTime now = el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
        UA_DateTime next = UA_EventLoopPOSIX_nextTimer(&el->eventLoop);
        ready = (next <= now);
    }

    UA_UNLOCK(&el->elMutex);
    return ready;
}

static gboolean
glibSourceDispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
    UA_EventLoopPOSIX *el = ((UA_EventLoopGLibSource*)source)->el;
    UA_LOCK(&el->elMutex);

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Dispatching the GLib EventLoop source");

    /* Process due cyclic callbacks and delayed callbacks. Delayed callbacks
     * are handled here (and not just implicitly via the next dispatch) so
     * that closed sockets are removed promptly. */
    UA_DateTime now = el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    UA_Timer_process(&el->timer, now);
    UA_EventLoopPOSIX_processDelayed(el);

    /* Process all fds with pending events */
    for(size_t i = 0; i < el->fdsSize; i++) {
        UA_RegisteredFD *rfd = el->fds[i];

        /* Already registered for (delayed) removal */
        if(rfd->dc.callback)
            continue;

        GPollFD *gfd = (GPollFD*)rfd->glibPollFD;
        if(!gfd)
            continue;

        short event = 0;
        if(gfd->revents & G_IO_IN)
            event = UA_FDEVENT_IN;
        else if(gfd->revents & G_IO_OUT)
            event = UA_FDEVENT_OUT;
        else if(gfd->revents & (G_IO_ERR | G_IO_HUP | G_IO_NVAL))
            event = UA_FDEVENT_ERR;
        else
            continue;

        UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                     "Processing event %u on fd %u", (unsigned)event,
                     (unsigned)rfd->fd);

        /* Call the EventSource callback */
        rfd->eventSourceCB(rfd->es, rfd, event);

        /* The fd may have deregistered itself, moving a different entry
         * into slot i (see deregisterFD_glib). Re-check the same index. */
        if(i >= el->fdsSize || rfd != el->fds[i])
            i--;
    }

    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STOPPING)
        checkClosedGLib(el);

    UA_UNLOCK(&el->elMutex);
    return G_SOURCE_CONTINUE;
}

static GSourceFuncs glibEventLoopSourceFuncs = {
    glibSourcePrepare, glibSourceCheck, glibSourceDispatch, NULL, NULL, NULL
};

/***********************/
/* EventLoop Lifecycle */
/***********************/

static void
checkClosedGLib(UA_EventLoopPOSIX *el) {
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

    /* Detach and destroy the GSource. It is recreated the next time the
     * EventLoop is started. */
    if(el->glibSource) {
        g_source_destroy((GSource*)el->glibSource);
        g_source_unref((GSource*)el->glibSource);
        el->glibSource = NULL;
    }

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPED;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "The EventLoop has stopped");
}

static UA_StatusCode
UA_EventLoopGLib_start(UA_EventLoopPOSIX *el) {
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_FRESH &&
       el->eventLoop.state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Starting the GLib EventLoop");

    /* Setting a custom clock source, same parameters as UA_EventLoop_new_POSIX */
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
                           "Eventloop\t| Setting a different monotonic clock, "
                           "but existing timers have been registered with a "
                           "different clock source");
        }
        el->clockSourceMonotonic = *csm;
    }

    /* Create and attach the GSource that drives the EventLoop. From this
     * point on, iterating el->glibContext (by this EventLoop's own `run`, or
     * by any other code that pumps the same GMainContext) services the
     * registered sockets and timers. */
    GSource *source = g_source_new(&glibEventLoopSourceFuncs,
                                   sizeof(UA_EventLoopGLibSource));
    ((UA_EventLoopGLibSource*)source)->el = el;
    g_source_set_name(source, "open62541 EventLoop");
    g_source_set_can_recurse(source, FALSE);
    g_source_set_priority(source, G_PRIORITY_DEFAULT);
    g_source_attach(source, (GMainContext*)el->glibContext);
    el->glibSource = source;

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
UA_EventLoopGLib_stop(UA_EventLoopPOSIX *el) {
    UA_LOCK(&el->elMutex);

    if(el->eventLoop.state != UA_EVENTLOOPSTATE_STARTED) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                       "The EventLoop is not running, cannot be stopped");
        UA_UNLOCK(&el->elMutex);
        return;
    }

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Stopping the GLib EventLoop");

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

    /* Set to STOPPED if all EventSources are already STOPPED */
    checkClosedGLib(el);

    GMainContext *ctx = (GMainContext*)el->glibContext;
    UA_UNLOCK(&el->elMutex);

    /* Wake a blocked context so pending stop-related dispatches (e.g. the
     * asynchronous closing of sockets) are not delayed */
    if(ctx)
        g_main_context_wakeup(ctx);
}

/* Dummy callback for the temporary timeout source used in `run` below. GLib
 * requires every attached source to have a callback. */
static gboolean
glibRunTimeoutCallback(gpointer data) {
    return G_SOURCE_REMOVE;
}

static UA_StatusCode
UA_EventLoopGLib_run(UA_EventLoopPOSIX *el, UA_UInt32 timeout) {
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

    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Iterate the GLib EventLoop");

    GMainContext *ctx = (GMainContext*)el->glibContext;
    UA_UNLOCK(&el->elMutex);

    /* Process one iteration of the GMainContext. A temporary timeout source
     * bounds a blocking wait to `timeout` ms so the documented `run`
     * contract (process events for at most `timeout` ms) is respected even
     * though GLib itself has no "iterate up to N ms" primitive. */
    GSource *timeoutSource = NULL;
    if(timeout > 0) {
        timeoutSource = g_timeout_source_new(timeout);
        g_source_set_callback(timeoutSource, glibRunTimeoutCallback, NULL, NULL);
        g_source_attach(timeoutSource, ctx);
    }

    g_main_context_iteration(ctx, (timeout > 0) ? TRUE : FALSE);

    if(timeoutSource) {
        g_source_destroy(timeoutSource);
        g_source_unref(timeoutSource);
    }

    UA_LOCK(&el->elMutex);

    /* Check if the last EventSource was successfully stopped */
    if(el->eventLoop.state == UA_EVENTLOOPSTATE_STOPPING)
        checkClosedGLib(el);

    el->executing = false;
    UA_UNLOCK(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

static void
UA_EventLoopGLib_cancel(UA_EventLoopPOSIX *el) {
    /* Nothing to do if the EventLoop is not executing */
    if(!el->executing)
        return;
    if(el->glibContext)
        g_main_context_wakeup((GMainContext*)el->glibContext);
}

static UA_StatusCode
UA_EventLoopGLib_free(UA_EventLoopPOSIX *el) {
    UA_LOCK(&el->elMutex);

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
        UA_EventLoopPOSIX_deregisterEventSource(&el->eventLoop, es);
        es->free(es);
    }

    /* Remove the repeated timed callbacks */
    UA_Timer_clear(&el->timer);

    /* Process remaining delayed callbacks */
    UA_EventLoopPOSIX_processDelayed(el);

    /* The GSource is normally already destroyed via checkClosedGLib. Cover
     * the case where the EventLoop was never started (state == FRESH). */
    if(el->glibSource) {
        g_source_destroy((GSource*)el->glibSource);
        g_source_unref((GSource*)el->glibSource);
        el->glibSource = NULL;
    }
    if(el->glibContext) {
        g_main_context_unref((GMainContext*)el->glibContext);
        el->glibContext = NULL;
    }

    UA_KeyValueMap_clear(&el->eventLoop.params);

    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

/*************************/
/* Initialize and Delete */
/*************************/

UA_EventLoop *
UA_EventLoop_new_GLib(const UA_Logger *logger, void *glibMainContext) {
    UA_EventLoopPOSIX *el = (UA_EventLoopPOSIX*)
        UA_calloc(1, sizeof(UA_EventLoopPOSIX));
    if(!el)
        return NULL;

    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);

    /* Initialize the delayed-callback queue */
    el->delayedTail = &el->delayedHead1;
    el->delayedHead2 = (UA_DelayedCallback*)0x01; /* sentinel value */

    el->eventLoop.logger = logger;

    /* Initialize the clock source to the default */
    el->clockSource = CLOCK_REALTIME;
# ifdef CLOCK_MONOTONIC_RAW
    el->clockSourceMonotonic = CLOCK_MONOTONIC_RAW;
# else
    el->clockSourceMonotonic = CLOCK_MONOTONIC;
# endif

    /* Reference the GMainContext this EventLoop attaches to. If none is
     * given, use the process-wide default context -- the same context that
     * g_main_loop_new(NULL, ...) or a GTK/GNOME application's main loop
     * iterates by default. */
    GMainContext *ctx = glibMainContext ?
        (GMainContext*)glibMainContext : g_main_context_default();
    el->glibContext = g_main_context_ref(ctx);

    /* Set the method pointers for the interface */
    el->eventLoop.start = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopGLib_start;
    el->eventLoop.stop = (void (*)(UA_EventLoop*))UA_EventLoopGLib_stop;
    el->eventLoop.free = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopGLib_free;
    el->eventLoop.run = (UA_StatusCode (*)(UA_EventLoop*, UA_UInt32))UA_EventLoopGLib_run;
    el->eventLoop.cancel = (void (*)(UA_EventLoop*))UA_EventLoopGLib_cancel;

    el->eventLoop.dateTime_now = UA_EventLoopPOSIX_DateTime_now;
    el->eventLoop.dateTime_nowMonotonic =
        UA_EventLoopPOSIX_DateTime_nowMonotonic;
    el->eventLoop.dateTime_localTimeUtcOffset =
        UA_EventLoopPOSIX_DateTime_localTimeUtcOffset;

    el->eventLoop.nextTimer = UA_EventLoopPOSIX_nextTimer;
    el->eventLoop.addTimer = UA_EventLoopPOSIX_addTimer;
    el->eventLoop.modifyTimer = UA_EventLoopPOSIX_modifyTimer;
    el->eventLoop.removeTimer = UA_EventLoopPOSIX_removeTimer;
    el->eventLoop.addDelayedCallback = UA_EventLoopPOSIX_addDelayedCallback;
    el->eventLoop.removeDelayedCallback = UA_EventLoopPOSIX_removeDelayedCallback;

    el->eventLoop.registerEventSource = UA_EventLoopPOSIX_registerEventSource;
    el->eventLoop.deregisterEventSource = UA_EventLoopPOSIX_deregisterEventSource;

    el->eventLoop.lock = UA_EventLoopPOSIX_lock;
    el->eventLoop.unlock = UA_EventLoopPOSIX_unlock;

    /* Select the GLib FD polling backend */
    el->registerFD = registerFD_glib;
    el->modifyFD = modifyFD_glib;
    el->deregisterFD = deregisterFD_glib;

    return &el->eventLoop;
}

#endif /* UA_ENABLE_EVENTLOOP_GLIB && UA_ARCHITECTURE_POSIX && !UA_ARCHITECTURE_LWIP */
