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
//#  define UA_close(s) closesocket(s) /* closesocket() takes a SOCKET (and sock() an int) */
# endif
# define UA_ERRNO WSAGetLastError()
# define UA_INTERRUPTED WSAEINTR
# define UA_AGAIN WSAEWOULDBLOCK
# define UA_EAGAIN EAGAIN
# define UA_WOULDBLOCK WSAEWOULDBLOCK
# define UA_ERR_CONNECTION_PROGRESS WSAEWOULDBLOCK
#else /* Unix */
# include <sys/select.h>
#endif

/* Catch-all for the architectures that are "actually POSIX" */
#ifndef UA_FD
# define UA_FD int
# define UA_INVALID_FD -1
//# define UA_close(s) close(s)
#endif
#ifndef UA_ERRNO
# define UA_ERRNO errno
# define UA_INTERRUPTED EINTR
# define UA_AGAIN EAGAIN
# define UA_EAGAIN EAGAIN
# define UA_WOULDBLOCK EWOULDBLOCK
# define UA_ERR_CONNECTION_PROGRESS EINPROGRESS
#endif

/* Workaround a bug in early glibc. Additionally, some non-glibc implementations
 * use a macro for FD_SET that triggers a cast-warning (e.g. early BSD libc or
 * musl libc). */
#if (!defined(__GNU_LIBRARY__) && defined(FD_SET)) ||       \
    (defined(__GNU_LIBRARY__) && (__GNU_LIBRARY__ <= 6) &&  \
     (__GLIBC__ <= 2) && (__GLIBC_MINOR__ < 16))
# define UA_FD_SET(fd, fds) FD_SET((unsigned int)fd, fds)
# define UA_FD_ISSET(fd, fds) FD_ISSET((unsigned int)fd, fds)
#else
# define UA_FD_SET(fd, fds) FD_SET((UA_FD)fd, fds)
# define UA_FD_ISSET(fd, fds) FD_ISSET((UA_FD)fd, fds)
#endif

_UA_BEGIN_DECLS

/* POSIX events are based on sockets / file descriptors. The EventSources can
 * register their fd in the EventLoop so that they are considered by the
 * EventLoop dropping into "select" to wait for events. */

/* POSIX-select can listen for three types of events. It has to be selected
 * for each registered fd which events they are interested in.  */
#define UA_POSIX_EVENT_READ 1
#define UA_POSIX_EVENT_WRITE 2
#define UA_POSIX_EVENT_ERR 4

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
