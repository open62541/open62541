/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_EVENTLOOP_POSIX_H_
#define UA_EVENTLOOP_POSIX_H_

#include <open62541/config.h>
#include <open62541/plugin/eventloop.h>

#if defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32)

#include "common/ua_timer.h"

_UA_BEGIN_DECLS

/* POSIX events are based on sockets / file descriptors. The EventSources can
 * register their fd in the EventLoop so that they are considered by the
 * EventLoop dropping into "poll" to wait for events. */

/* TODO: Move the macro-forest from /arch/<arch>/ua_architecture.h */

#define UA_FD UA_SOCKET
#define UA_INVALID_FD UA_INVALID_SOCKET

typedef void
(*UA_FDCallback)(UA_EventSource *es, UA_FD fd, void **fdcontext, short event);

typedef struct {
    /* The members fd and events are stored in the separate
     * pollfds array:
     * - UA_FD fd;
     * - short events; */
    UA_EventSource *es;
    UA_FDCallback callback;
    void *fdcontext;
} UA_RegisteredFD;

typedef struct {
    UA_EventLoop eventLoop;
    
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

    /* Flag determining whether the eventloop is currently within the
     * "run" method */
    UA_Boolean executing;

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
} POSIX_EL;

UA_StatusCode
POSIX_EL_registerFD(POSIX_EL *el, UA_FD fd, short eventMask,
                    UA_FDCallback cb, UA_EventSource *es, void *fdcontext);

/* Change the fd settings (event mask, callback) in-place. Fails only if the fd
 * no longer exists. */
UA_StatusCode
POSIX_EL_modifyFD(POSIX_EL *el, UA_FD fd, short eventMask,
                  UA_FDCallback cb, void *fdcontext);

/* During processing of an fd-event, the fd may deregister itself. But in the
 * fd-callback they must not deregister another fd. */
UA_StatusCode
POSIX_EL_deregisterFD(POSIX_EL *el, UA_FD fd);

/* abort the iteration if the returned boolean is true */
typedef UA_Boolean
(*POSIX_EL_IterateCallback)(UA_EventSource *es, UA_FD fd,
                            void *fdContext, void *iterateContext);

/* Call the callback for all fd that are registered from that event source. The
 * callback is called with the 'event' argument set to zero to disambiguate from
 * a callback after 'poll'. */
void
POSIX_EL_iterateFD(POSIX_EL *el, UA_EventSource *es,
                   POSIX_EL_IterateCallback callback,
                   void *iterateContext);

_UA_END_DECLS

#endif /* defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32) */

#endif /* UA_EVENTLOOP_POSIX_H_ */
