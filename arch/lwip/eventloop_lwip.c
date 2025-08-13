/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include "eventloop_lwip.h"

#if defined(UA_ARCHITECTURE_LWIP)

#include "open62541/plugin/eventloop.h"

#if defined(UA_ARCHITECTURE_POSIX)
#include <netif/tapif.h>
#endif

/* These are just default values and not unchangeable IP assignments. They can be overridden at runtime */
#define LWIP_PORT_INIT_IPADDR(addr)   IP4_ADDR(&addr, 192, 168, 0, 200);
#define LWIP_PORT_INIT_GW(addr)       IP4_ADDR(&addr, 192, 168, 0, 1);
#define LWIP_PORT_INIT_NETMASK(addr)  IP4_ADDR(&addr, 255, 255, 255, 0);

#define IPV4_ADDRESS_STRING_LENGTH 16

/* Configuration parameters */
#define LWIPEVENTLOOP_PARAMETERSSIZE 3
#define LWIPEVENTLOOP_PARAMINDEX_IPADDR 0
#define LWIPEVENTLOOP_PARAMINDEX_NETMASK 1
#define LWIPEVENTLOOP_PARAMINDEX_GATEWAY 2

static UA_KeyValueRestriction LWIPEventLoopConfigParameters[LWIPEVENTLOOP_PARAMETERSSIZE] = {
    {{0, UA_STRING_STATIC("ipaddr")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("netmask")}, &UA_TYPES[UA_TYPES_STRING], false, true, false},
    {{0, UA_STRING_STATIC("gateway")}, &UA_TYPES[UA_TYPES_STRING], false, true, false}
};

#include <time.h>

/*********/
/* Timer */
/*********/

static UA_DateTime
UA_EventLoopLWIP_nextTimer(UA_EventLoop *public_el) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    if(el->delayedHead1 > (UA_DelayedCallback *)0x01 ||
       el->delayedHead2 > (UA_DelayedCallback *)0x01)
        return el->eventLoop.dateTime_nowMonotonic(&el->eventLoop);
    return UA_Timer_next(&el->timer);
}

static UA_StatusCode
UA_EventLoopLWIP_addTimer(UA_EventLoop *public_el,
                                    UA_Callback cb,
                                    void *application, void *data,
                                    UA_Double interval_ms,
                                    UA_DateTime *baseTime,
                                    UA_TimerPolicy timerPolicy,
                                    UA_UInt64 *callbackId) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    return UA_Timer_add(&el->timer, cb, application, data, interval_ms,
                        public_el->dateTime_nowMonotonic(public_el),
                        baseTime, timerPolicy, callbackId);
}

static UA_StatusCode
UA_EventLoopLWIP_modifyTimer(UA_EventLoop *public_el,
                              UA_UInt64 callbackId,
                              UA_Double interval_ms,
                              UA_DateTime *baseTime,
                              UA_TimerPolicy timerPolicy) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    return UA_Timer_modify(&el->timer, callbackId, interval_ms,
                           public_el->dateTime_nowMonotonic(public_el),
                           baseTime, timerPolicy);
}

static void
UA_EventLoopLWIP_removeTimer(UA_EventLoop *public_el,
                              UA_UInt64 callbackId) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    UA_Timer_remove(&el->timer, callbackId);
}

void
UA_EventLoopLWIP_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    dc->next = NULL;

    /* el->delayedTail points either to prev->next or to the head.
     * We need to update two locations:
     * 1: el->delayedTail = &dc->next;
     * 2: *oldtail = dc; (equal to &dc->next)
     *
     * Once we have (1), we "own" the previous-to-last entry. No need to worry
     * about (2), we can adjust it with a delay. This makes the queue
     * "eventually consistent". */
    UA_DelayedCallback **oldtail = (UA_DelayedCallback**)
        UA_atomic_xchg((void**)&el->delayedTail, &dc->next);
    UA_atomic_xchg((void**)oldtail, &dc->next);
}

/* Resets the delayed queue and returns the previous head and tail */
static void
resetDelayedQueue(UA_EventLoopLWIP *el, UA_DelayedCallback **oldHead,
                  UA_DelayedCallback **oldTail) {
    if(el->delayedHead1 <= (UA_DelayedCallback *)0x01 &&
       el->delayedHead2 <= (UA_DelayedCallback *)0x01)
        return; /* The queue is empty */

    UA_Boolean active1 = (el->delayedHead1 != (UA_DelayedCallback*)0x01);
    UA_DelayedCallback **activeHead = (active1) ? &el->delayedHead1 : &el->delayedHead2;
    UA_DelayedCallback **inactiveHead = (active1) ? &el->delayedHead2 : &el->delayedHead1;

    /* Switch active/inactive by resetting the sentinel values. The (old) active
     * head points to an element which we return. Parallel threads continue to
     * add elements to the queue "below" the first element. */
    UA_atomic_xchg((void**)inactiveHead, NULL);
    *oldHead = (UA_DelayedCallback *)
        UA_atomic_xchg((void**)activeHead, (void*)0x01);

    /* Make the tail point to the (new) active head. Return the value of last
     * tail. When iterating over the queue elements, we need to find this tail
     * as the last element. If we find a NULL next-pointer before hitting the
     * tail spinlock until the pointer updates (eventually consistent). */
    *oldTail = (UA_DelayedCallback*)
        UA_atomic_xchg((void**)&el->delayedTail, inactiveHead);
}

static void
UA_EventLoopLWIP_removeDelayedCallback(UA_EventLoop *public_el,
                                        UA_DelayedCallback *dc) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)public_el;
    UA_LOCK(&el->elMutex);

    /* Reset and get the old head and tail */
    UA_DelayedCallback *cur = NULL, *tail = NULL;
    resetDelayedQueue(el, &cur, &tail);

    /* Loop until we reach the tail (or head and tail are both NULL) */
    UA_DelayedCallback *next;
    for(; cur; cur = next) {
        /* Spin-loop until the next-pointer of cur is updated.
         * The element pointed to by tail must appear eventually. */
        next = cur->next;
        while(!next && cur != tail)
            next = (UA_DelayedCallback *)UA_atomic_load((void**)&cur->next);
        if(cur == dc)
            continue;
        UA_EventLoopLWIP_addDelayedCallback(public_el, cur);
    }

    UA_UNLOCK(&el->elMutex);
}

static void
processDelayed(UA_EventLoopLWIP *el) {
    UA_LOG_TRACE(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Process delayed callbacks");

    UA_LOCK_ASSERT(&el->elMutex);

    /* Reset and get the old head and tail */
    UA_DelayedCallback *dc = NULL, *tail = NULL;
    resetDelayedQueue(el, &dc, &tail);

    /* Loop until we reach the tail (or head and tail are both NULL) */
    UA_DelayedCallback *next;
    for(; dc; dc = next) {
        next = dc->next;
        while(!next && dc != tail)
            next = (UA_DelayedCallback *)UA_atomic_load((void**)&dc->next);
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

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                 "Starting the EventLoop");

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(el->netifInit) {
        const UA_String *ipaddr = (const UA_String*)
            UA_KeyValueMap_getScalar(&el->config.params, LWIPEventLoopConfigParameters[LWIPEVENTLOOP_PARAMINDEX_IPADDR].name,
                             &UA_TYPES[UA_TYPES_STRING]);
        const UA_String *netmask = (const UA_String*)
            UA_KeyValueMap_getScalar(&el->config.params, LWIPEventLoopConfigParameters[LWIPEVENTLOOP_PARAMINDEX_NETMASK].name,
                                     &UA_TYPES[UA_TYPES_STRING]);
        const UA_String *gateway = (const UA_String*)
            UA_KeyValueMap_getScalar(&el->config.params, LWIPEventLoopConfigParameters[LWIPEVENTLOOP_PARAMINDEX_GATEWAY].name,
                                     &UA_TYPES[UA_TYPES_STRING]);

        res = el->netifInit((UA_EventLoop*)el, ipaddr, netmask, gateway);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                         "Error during the initialisation of the network interface");
            return res;
        }
    } else {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                      "No function defined to initialize the network interface."
                      "Initialization must be performed externally");
    }

    /* Setting the clock source */
    const UA_Int32 *cs = (const UA_Int32*)
        UA_KeyValueMap_getScalar(&el->eventLoop.params,
                                 UA_QUALIFIEDNAME(0, "clock-source"),
                                 &UA_TYPES[UA_TYPES_INT32]);
    const UA_Int32 *csm = (const UA_Int32*)
        UA_KeyValueMap_getScalar(&el->eventLoop.params,
                                 UA_QUALIFIEDNAME(0, "clock-source-monotonic"),
                                 &UA_TYPES[UA_TYPES_INT32]);
#if defined(UA_ARCHITECTURE_LWIP) && !defined(UA_ARCHITECTURE_FREERTOS) && !defined(__APPLE__) && !defined(__MACH__)
    el->clockSource = CLOCK_REALTIME;
    if(cs)
        el->clockSource = *cs;

# ifdef CLOCK_MONOTONIC_RAW
    el->clockSourceMonotonic = CLOCK_MONOTONIC_RAW;
# else
    el->clockSourceMonotonic = CLOCK_MONOTONIC;
# endif
    if(csm)
        el->clockSourceMonotonic = *csm;
#else
    if(cs || csm) {
        UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                       "Eventloop\t| Cannot set a custom clock source");
    }
#endif

    /* Create the self-pipe */
    int err = UA_EventLoopLWIP_pipe(el->selfpipe);
    if(err != 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
           UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_NETWORK,
                          "Eventloop\t| Could not create the self-pipe (%s)",
                          errno_str));
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Start the EventSources */
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

    /* Close the self-pipe when everything else is done */
    UA_close(el->selfpipe[0]);
    UA_close(el->selfpipe[1]);

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->eventLoop.state =
        UA_EVENTLOOPSTATE_STOPPED;

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
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

    UA_LOG_DEBUG(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
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
                       "Cannot run a stopped EventLoop");
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
    if(el->delayedHead1 != NULL && el->delayedHead2 != NULL)
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

static UA_DateTime
UA_EventLoopLWIP_DateTime_now(UA_EventLoop *el) {
#if defined(UA_ARCHITECTURE_LWIP) && !defined(UA_ARCHITECTURE_FREERTOS) && !defined(__APPLE__) && !defined(__MACH__)
    UA_EventLoopLWIP *pel = (UA_EventLoopLWIP*)el;
    struct timespec ts;
    int res = clock_gettime(pel->clockSource, &ts);
    if(UA_UNLIKELY(res != 0))
        return 0;
    return (ts.tv_sec * UA_DATETIME_SEC) + (ts.tv_nsec / 100) + UA_DATETIME_UNIX_EPOCH;
#else
    return UA_DateTime_now();
#endif
}

static UA_DateTime
UA_EventLoopLWIP_DateTime_nowMonotonic(UA_EventLoop *el) {
#if defined(UA_ARCHITECTURE_LWIP) && !defined(UA_ARCHITECTURE_FREERTOS) && !defined(__APPLE__) && !defined(__MACH__)
    UA_EventLoopLWIP *pel = (UA_EventLoopLWIP*)el;
    struct timespec ts;
    int res = clock_gettime(pel->clockSourceMonotonic, &ts);
    if(UA_UNLIKELY(res != 0))
        return 0;
    /* Also add the unix epoch for the monotonic clock. So we get a "normal"
     * output when a "normal" source is configured. */
    return (ts.tv_sec * UA_DATETIME_SEC) + (ts.tv_nsec / 100) + UA_DATETIME_UNIX_EPOCH;
#else
    return UA_DateTime_nowMonotonic();
#endif
}

static UA_Int64
UA_EventLoopLWIP_DateTime_localTimeUtcOffset(UA_EventLoop *el) {
    /* TODO: Fix for custom clock sources */
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

    /* Call netif shutdown*/
    if(el->netifShutdown)
        el->netifShutdown((UA_EventLoop*)el);

    UA_KeyValueMap_clear(&el->config.params);

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

    UA_KeyValueMap_clear(&el->eventLoop.params);

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

#if defined(UA_ARCHITECTURE_POSIX)

static bool initAlready = false;
static UA_StatusCode defaultNetifInit(UA_EventLoopLWIP *el, const UA_String *ipaddr,
                                      const UA_String *netmask, const UA_String *gw) {
    ip4_addr_t local_ipaddr, local_netmask, local_gw;
    LWIP_PORT_INIT_IPADDR(local_ipaddr);
    LWIP_PORT_INIT_NETMASK(local_netmask);
    LWIP_PORT_INIT_GW(local_gw);

    /* Override with provided parameters */
    if(ipaddr && netmask && gw) {
        char ipaddr_s[IPV4_ADDRESS_STRING_LENGTH];
        mp_snprintf(ipaddr_s, IPV4_ADDRESS_STRING_LENGTH, "%.*s",
                    (int)ipaddr->length, (char*)ipaddr->data);
        char netmask_s[IPV4_ADDRESS_STRING_LENGTH];
        mp_snprintf(netmask_s, IPV4_ADDRESS_STRING_LENGTH, "%.*s",
                    (int)netmask->length, (char*)netmask->data);
        char gw_s[IPV4_ADDRESS_STRING_LENGTH];
        mp_snprintf(gw_s, IPV4_ADDRESS_STRING_LENGTH, "%.*s",
                    (int)gw->length, (char*)gw->data);

        if(!ip4addr_aton(ipaddr_s, &local_ipaddr) || !ip4addr_aton(netmask_s, &local_netmask) ||
           !ip4addr_aton(gw_s, &local_gw))
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* The initialization can only be done once for the runtime of the process */
    if(!initAlready) {
        tcpip_init(NULL, NULL);
        LOCK_TCPIP_CORE();
        if(!netif_add(&el->netif, &local_ipaddr, &local_netmask, &local_gw, NULL, tapif_init, tcpip_input)) {
            UNLOCK_TCPIP_CORE();
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        netif_set_default(&el->netif);
        netif_set_up(&el->netif);
        UNLOCK_TCPIP_CORE();
    }
    initAlready = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode defaultNetifPoll(UA_EventLoopLWIP *el) {
    return UA_STATUSCODE_GOOD;
}

static void defaultNetifShutdown(UA_EventLoopLWIP *el) {
}

#endif

UA_EventLoop *
UA_EventLoop_new_LWIP(const UA_Logger *logger, UA_EventLoopConfiguration *config) {
    UA_EventLoopLWIP *el = (UA_EventLoopLWIP*)
        UA_calloc(1, sizeof(UA_EventLoopLWIP));
    if(!el)
        return NULL;

    UA_LOCK_INIT(&el->elMutex);
    UA_Timer_init(&el->timer);

#if defined(UA_ARCHITECTURE_POSIX)
    el->netifInit = (UA_StatusCode (*)(UA_EventLoop*, const UA_String*, const UA_String*, const UA_String*))defaultNetifInit;
    el->netifPoll = (UA_StatusCode (*)(UA_EventLoop*))defaultNetifPoll;
    el->netifShutdown = (void (*)(UA_EventLoop*))defaultNetifShutdown;
#endif

    if(config) {
        el->config = *config;
        if(config->netifInit)
            el->netifInit = config->netifInit;
        if(config->netifPoll)
            el->netifPoll = config->netifPoll;
        if(config->netifShutdown)
            el->netifShutdown = config->netifShutdown;
        /* Reset the old config */
        memset(config, 0, sizeof(UA_EventLoopConfiguration));
    }

    /* Initialize the queue */
    el->delayedTail = &el->delayedHead1;
    el->delayedHead2 = (UA_DelayedCallback*)0x01; /* sentinel value */

    /* Set the public EventLoop content */
    el->eventLoop.logger = logger;

    el->eventLoop.start = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopLWIP_start;
    el->eventLoop.stop = (void (*)(UA_EventLoop*))UA_EventLoopLWIP_stop;
    el->eventLoop.free = (UA_StatusCode (*)(UA_EventLoop*))UA_EventLoopLWIP_free;
    el->eventLoop.run = (UA_StatusCode (*)(UA_EventLoop*, UA_UInt32))UA_EventLoopLWIP_run;
    el->eventLoop.cancel = (void (*)(UA_EventLoop*))UA_EventLoopLWIP_cancel;

    el->eventLoop.dateTime_now = UA_EventLoopLWIP_DateTime_now;
    el->eventLoop.dateTime_nowMonotonic =
        UA_EventLoopLWIP_DateTime_nowMonotonic;
    el->eventLoop.dateTime_localTimeUtcOffset =
        UA_EventLoopLWIP_DateTime_localTimeUtcOffset;

    el->eventLoop.nextTimer = UA_EventLoopLWIP_nextTimer;
    el->eventLoop.addTimer = UA_EventLoopLWIP_addTimer;
    el->eventLoop.modifyTimer = UA_EventLoopLWIP_modifyTimer;
    el->eventLoop.removeTimer = UA_EventLoopLWIP_removeTimer;
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

/***************************/
/* Network Buffer Handling */
/***************************/

UA_StatusCode
UA_EventLoopLWIP_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize) {
    UA_LWIPConnectionManager *pcm = (UA_LWIPConnectionManager*)cm;
    if(pcm->txBuffer.length == 0)
        return UA_ByteString_allocBuffer(buf, bufSize);
    if(pcm->txBuffer.length < bufSize)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *buf = pcm->txBuffer;
    buf->length = bufSize;
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopLWIP_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf) {
    UA_LWIPConnectionManager *pcm = (UA_LWIPConnectionManager*)cm;
    if(pcm->txBuffer.data == buf->data)
        UA_ByteString_init(buf);
    else
        UA_ByteString_clear(buf);
}

UA_StatusCode
UA_EventLoopLWIP_allocateStaticBuffers(UA_LWIPConnectionManager *pcm) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_UInt32 rxBufSize = 1u << 10; /* The default is 64kb */
    const UA_UInt32 *configRxBufSize = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&pcm->cm.eventSource.params,
                                 UA_QUALIFIEDNAME(0, "recv-bufsize"),
                                 &UA_TYPES[UA_TYPES_UINT32]);
    if(configRxBufSize)
        rxBufSize = *configRxBufSize;
    if(pcm->rxBuffer.length != rxBufSize) {
        UA_ByteString_clear(&pcm->rxBuffer);
        res = UA_ByteString_allocBuffer(&pcm->rxBuffer, rxBufSize);
    }

    const UA_UInt32 *txBufSize = (const UA_UInt32 *)
        UA_KeyValueMap_getScalar(&pcm->cm.eventSource.params,
                                 UA_QUALIFIEDNAME(0, "send-bufsize"),
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
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_EventLoopLWIP_setReusable(UA_FD sockfd) {
#if SO_REUSE
    int enableReuseVal = 1;
    int res = UA_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                            (const char*)&enableReuseVal, sizeof(enableReuseVal));
    return (res == 0) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADINTERNALERROR;
#else
    return UA_STATUSCODE_GOOD;
#endif
}

/************************/
/* Select / epoll Logic */
/************************/

/* Re-arm the self-pipe socket for the next signal by reading from it */
static void
flushSelfPipe(UA_SOCKET s) {
    char buf[128];
    ssize_t i;
    do {
        i = lwip_read(s, buf, 128);
    } while(i > 0);
}

UA_StatusCode
UA_EventLoopLWIP_registerFD(UA_EventLoopLWIP *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
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
    UA_LOCK_ASSERT(&el->elMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_EventLoopLWIP_deregisterFD(UA_EventLoopLWIP *el, UA_RegisteredFD *rfd) {
    UA_LOCK_ASSERT(&el->elMutex);
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
    UA_LOCK_ASSERT(&el->elMutex);

    FD_ZERO(readset);
    FD_ZERO(writeset);
    FD_ZERO(errset);

    /* Always listen on the read-end of the pipe */
    UA_FD highestfd = el->selfpipe[0];
    FD_SET(el->selfpipe[0], readset);

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
        if(currentFD > highestfd)
            highestfd = currentFD;
    }
    return highestfd;
}

UA_StatusCode
UA_EventLoopLWIP_pollFDs(UA_EventLoopLWIP *el, UA_DateTime listenTimeout) {
    UA_assert(listenTimeout >= 0);
    UA_LOCK_ASSERT(&el->elMutex);

    /* Call netif poll*/
    if(el->netifPoll)
        el->netifPoll((UA_EventLoop*)el);

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

    /* The self-pipe has received. Clear the buffer by reading. */
    if(UA_UNLIKELY(FD_ISSET(el->selfpipe[0], &readset)))
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

int UA_EventLoopLWIP_pipe(UA_FD fds[2]) {
    struct sockaddr_in inaddr;
    memset(&inaddr, 0, sizeof(inaddr));
    inaddr.sin_family = AF_INET;
    inaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    inaddr.sin_port = 0;

    UA_FD lst = UA_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    UA_bind(lst, (struct sockaddr *)&inaddr, sizeof(inaddr));
    UA_listen(lst, 1);

    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    UA_getsockname(lst, (struct sockaddr*)&addr, &len);

    fds[0] = UA_socket(AF_INET, SOCK_STREAM, 0);
    int err = UA_connect(fds[0], (struct sockaddr*)&addr, len);
    fds[1] = UA_accept(lst, 0, 0);
    UA_close(lst);

    UA_EventLoopLWIP_setNoSigPipe(fds[0]);
    UA_EventLoopLWIP_setReusable(fds[0]);
    UA_EventLoopLWIP_setNonBlocking(fds[0]);
    UA_EventLoopLWIP_setNoSigPipe(fds[1]);
    UA_EventLoopLWIP_setReusable(fds[1]);
    UA_EventLoopLWIP_setNonBlocking(fds[1]);
    return err;
}

void
UA_EventLoopLWIP_cancel(UA_EventLoopLWIP *el) {
    /* Nothing to do if the EventLoop is not executing */
    if(!el->executing)
        return;

    /* Trigger the self-pipe */
    ssize_t err = lwip_write(el->selfpipe[1], ".", 1);
    if(err <= 0) {
        UA_LOG_SOCKET_ERRNO_WRAP(
            UA_LOG_WARNING(el->eventLoop.logger, UA_LOGCATEGORY_EVENTLOOP,
                           "Eventloop\t| Error signaling self-pipe (%s)", errno_str));
    }
}

#endif
