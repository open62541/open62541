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

/* TODO: Move the macro-forest from /arch/<arch>/ua_architecture.h */

#define UA_FD UA_SOCKET
#define UA_INVALID_FD UA_INVALID_SOCKET

_UA_BEGIN_DECLS

/* POSIX events are based on sockets / file descriptors. The EventSources can
 * register their fd in the EventLoop so that they are considered by the
 * EventLoop dropping into "poll" to wait for events. */

typedef void
(*UA_FDCallback)(UA_EventSource *es, UA_FD fd, void *fdcontext, short event);

UA_StatusCode
UA_EventLoop_registerFD(UA_EventLoop *el, UA_FD fd, short eventMask,
                        UA_FDCallback cb, UA_EventSource *es, void *fdcontext);

/* Change the fd settings (event mask, callback) in-place. Fails only if the fd
 * no longer exists. */
UA_StatusCode
UA_EventLoop_modifyFD(UA_EventLoop *el, UA_FD fd, short eventMask,
                      UA_FDCallback cb, void *fdcontext);

/* During processing of an fd-event, the fd may deregister itself. But in the
 * fd-callback they must not deregister another fd. */
UA_StatusCode
UA_EventLoop_deregisterFD(UA_EventLoop *el, UA_FD fd);

/* Call the callback for all fd that are registered from that event source */
void
UA_EventLoop_iterateFD(UA_EventLoop *el, UA_EventSource *es, UA_FDCallback cb);

_UA_END_DECLS

#endif /* UA_EVENTLOOP_POSIX_H_ */
