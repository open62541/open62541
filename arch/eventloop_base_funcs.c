/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include <open62541/config.h>
#include "open62541/plugin/eventloop_base_funcs.h"

/*****************************/
/* EventLoop handling        */
/*****************************/

UA_StatusCode
UA_EventLoop_start(UA_EventLoop *el,
                   UA_EventLoopCallback start_callback,
                   void *user_arg) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_LOCK(&el->elMutex);

    if(el->state != UA_EVENTLOOPSTATE_FRESH &&
       el->state != UA_EVENTLOOPSTATE_STOPPED) {
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    UA_LOG_INFO(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                "Starting the EventLoop");

    if(start_callback) {
        res = start_callback(el, user_arg);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                         "Eventloop\t| Error calling user callback in UA_EventLoop_start");
            UA_UNLOCK(&el->elMutex);
            return res;
        }
    }

    UA_EventSource *es = el->eventSources;
    while(es) {
        UA_UNLOCK(&el->elMutex);
        res |= es->start(es);
        UA_LOCK(&el->elMutex);
        es = es->next;
    }

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->state = UA_EVENTLOOPSTATE_STARTED;

    UA_UNLOCK(&el->elMutex);
    return res;
}

UA_StatusCode
UA_EventLoop_stop(UA_EventLoop *el,
                  UA_EventLoopCallback stop_callback,
                  void *user_arg) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_LOCK(&el->elMutex);

    if(el->state != UA_EVENTLOOPSTATE_STARTED) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                       "The EventLoop is not running, cannot be stopped");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    UA_LOG_INFO(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                "Stopping the EventLoop");

    /* Set to STOPPING to prevent "normal use" */
    *(UA_EventLoopState*)(uintptr_t)&el->state =
        UA_EVENTLOOPSTATE_STOPPING;

    if(stop_callback) {
        res = stop_callback(el, user_arg);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                         "Eventloop\t| Error calling user callback in UA_EventLoop_stop");
        }
    }

    /* Stop all event sources (asynchronous) */
    UA_EventSource *es = el->eventSources;
    for(; es; es = es->next) {
        if(es->state == UA_EVENTSOURCESTATE_STARTING ||
           es->state == UA_EVENTSOURCESTATE_STARTED) {
            UA_UNLOCK(&el->elMutex);
            es->stop(es);
            UA_LOCK(&el->elMutex);
        }
    }

    UA_UNLOCK(&el->elMutex);

    return res;
}

UA_StatusCode
UA_EventLoop_check_stopped(UA_EventLoop *el,
                           UA_EventLoopCallback check_stopped_callback,
                           UA_EventLoopCallback stop_callback,
                           void *user_arg) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_LOCK(&el->elMutex);

    if(el->state != UA_EVENTLOOPSTATE_STOPPING) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                       "The EventLoop is not stopping, cannot be stopped");
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* Check all event sources */
    UA_EventSource *es = el->eventSources;
    while(es) {
        if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
            UA_UNLOCK(&el->elMutex);
            return UA_STATUSCODE_GOODCALLAGAIN;
        }
        es = es->next;
    }

    if(check_stopped_callback) {
        res = check_stopped_callback(el, user_arg);
        if(res != UA_STATUSCODE_GOOD) {
            UA_UNLOCK(&el->elMutex);
            return res;
        }
    }

    /* Dirty-write the state that is const "from the outside" */
    *(UA_EventLoopState*)(uintptr_t)&el->state =
        UA_EVENTLOOPSTATE_STOPPED;

    if(stop_callback) {
        res = stop_callback(el, user_arg);
        if(res != UA_STATUSCODE_GOOD) {
            UA_UNLOCK(&el->elMutex);
            return res;
        }
    }

    UA_LOG_INFO(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                "The EventLoop has stopped");

    UA_UNLOCK(&el->elMutex);

    return res;
}

UA_StatusCode
UA_EventLoop_free(UA_EventLoop *el,
                  UA_EventLoopCallback free_callback,
                  void *user_arg) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
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

    if(free_callback) {
        res = free_callback(el, user_arg);
        if(res != UA_STATUSCODE_GOOD) {
            UA_UNLOCK(&el->elMutex);
            return res;
        }
    }

    /* Clean up */
    UA_UNLOCK(&el->elMutex);
    UA_LOCK_DESTROY(&el->elMutex);
    UA_free(el);
    return UA_STATUSCODE_GOOD;
}

/*****************************/
/* Registering Event Sources */
/*****************************/

UA_StatusCode
UA_EventLoop_registerEventSource(UA_EventLoop *el,
                                 UA_EventSource *es) {
    UA_LOCK(&el->elMutex);

    /* Already registered? */
    if(es->state != UA_EVENTSOURCESTATE_FRESH) {
        UA_LOG_ERROR(el->logger, UA_LOGCATEGORY_NETWORK,
                     "Cannot register the EventSource \"%.*s\": "
                     "already registered",
                     (int)es->name.length, (char*)es->name.data);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Add to linked list */
    es->next = el->eventSources;
    el->eventSources = es;

    es->eventLoop = el;
    es->state = UA_EVENTSOURCESTATE_STOPPED;

    /* Start if the entire EventLoop is started */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(el->state == UA_EVENTLOOPSTATE_STARTED)
        res = es->start(es);

    UA_UNLOCK(&el->elMutex);
    return res;
}

UA_StatusCode
UA_EventLoop_deregisterEventSource(UA_EventLoop *el,
                                   UA_EventSource *es) {
    UA_LOCK(&el->elMutex);

    if(es->state != UA_EVENTSOURCESTATE_STOPPED) {
        UA_LOG_WARNING(el->logger, UA_LOGCATEGORY_EVENTLOOP,
                       "Cannot deregister the EventSource %.*s: "
                       "Has to be stopped first",
                       (int)es->name.length, es->name.data);
        UA_UNLOCK(&el->elMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Remove from the linked list */
    UA_EventSource **s = &el->eventSources;
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
