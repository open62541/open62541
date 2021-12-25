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

/* A macro-forest to work around small differences between POSIX and
 * nearly-POSIX architectures */

#if defined(_WIN32) /* Windows */
# include <winsock2.h>
# if !defined(__MINGW32__) || defined(__clang__)
#  define UA_FD SOCKET /* On MSVC, a socket is a pointer and not an int */
#  define UA_INVALID_FD INVALID_SOCKET
# endif
# define UA_ERRNO WSAGetLastError()
# define UA_INTERRUPTED WSAEINTR
# define UA_AGAIN WSAEWOULDBLOCK
# define UA_EAGAIN EAGAIN
# define UA_WOULDBLOCK WSAEWOULDBLOCK
#else /* Unix */
# include <sys/poll.h>
#endif

/* Catch-all for the architectures that are "actually POSIX" */
#ifndef UA_FD
# define UA_FD int
# define UA_INVALID_FD -1
#endif
#ifndef UA_ERRNO
# define UA_ERRNO errno
# define UA_INTERRUPTED EINTR
# define UA_AGAIN EAGAIN
# define UA_EAGAIN EAGAIN
# define UA_WOULDBLOCK EWOULDBLOCK
#endif

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
