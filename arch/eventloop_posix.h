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

/* Include architecture-specific definitions */
#if defined(UA_ARCHITECTURE_WIN32)
#include "win32/ua_architecture.h"
#elif defined(UA_ARCHITECTURE_POSIX)
#include "posix/ua_architecture.h"
#endif

#include "common/ua_timer.h"
#include "../deps/mp_printf.h"

#if !defined(__QNX__)
# include "../deps/open62541_queue.h"
#endif

/* epoll_pwait returns bogus data with the tc compiler */
#if defined(__linux__) && !defined(__TINYC__)
# define UA_HAVE_EPOLL
# include <sys/epoll.h>
#endif

#define UA_MAXBACKLOG 100
#define UA_MAXHOSTNAME_LENGTH 256
#define UA_MAXPORTSTR_LENGTH 6

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

_UA_BEGIN_DECLS

/* POSIX events are based on sockets / file descriptors. The EventSources can
 * register their fd in the EventLoop so that they are considered by the
 * EventLoop dropping into "poll" to wait for events. */

/* TODO: Move the macro-forest from /arch/<arch>/ua_architecture.h */

#define UA_FD UA_SOCKET
#define UA_INVALID_FD UA_INVALID_SOCKET

struct UA_RegisteredFD;
typedef struct UA_RegisteredFD UA_RegisteredFD;

/* Bitmask to be used for the UA_FDCallback event argument */
#define UA_FDEVENT_IN 1
#define UA_FDEVENT_OUT 2
#define UA_FDEVENT_ERR 4

typedef void (*UA_FDCallback)(UA_EventSource *es, UA_RegisteredFD *rfd, short event);

struct UA_RegisteredFD {
    UA_DelayedCallback dc; /* Used for async closing. Must be the first member
                            * because the rfd is freed by the delayed callback
                            * mechanism. */

    ZIP_ENTRY(UA_RegisteredFD) zipPointers; /* Register FD in the EventSource */
    UA_FD fd;
    short listenEvents; /* UA_FDEVENT_IN | UA_FDEVENT_OUT*/

    UA_EventSource *es; /* Backpointer to the EventSource */
    UA_FDCallback eventSourceCB;
};

enum ZIP_CMP cmpFD(const UA_FD *a, const UA_FD *b);
typedef ZIP_HEAD(UA_FDTree, UA_RegisteredFD) UA_FDTree;
ZIP_FUNCTIONS(UA_FDTree, UA_RegisteredFD, zipPointers, UA_FD, fd, cmpFD)

/* All ConnectionManager in the POSIX EventLoop can be cast to
 * UA_ConnectionManagerPOSIX. They carry a sorted tree of their open
 * sockets/file-descriptors. */
typedef struct {
    UA_ConnectionManager cm;

    /* Reused receive buffer. The size is configured via
     * the recv-bufsize parameter.*/
    UA_ByteString rxBuffer;

    /* Sorted tree of the FDs */
    size_t fdsSize;
    UA_FDTree fds;
} UA_POSIXConnectionManager;

typedef struct {
    UA_EventLoop eventLoop;

    /* Timer */
    UA_Timer timer;

    /* Linked List of Delayed Callbacks */
    UA_DelayedCallback *delayedCallbacks;

    /* Flag determining whether the eventloop is currently within the
     * "run" method */
    UA_Boolean executing;

#if defined(UA_HAVE_EPOLL)
    UA_FD epollfd;
#else
    UA_RegisteredFD **fds;
    size_t fdsSize;
#endif

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
} UA_EventLoopPOSIX;

/*
 * The following functions differ between epoll and normal select
 */

/* Register to start receiving events */
UA_StatusCode
UA_EventLoopPOSIX_registerFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);

/* Modify the events that the fd listens on */
UA_StatusCode
UA_EventLoopPOSIX_modifyFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);

/* Deregister but do not close the fd. No further events are received. */
void
UA_EventLoopPOSIX_deregisterFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);

UA_StatusCode
UA_EventLoopPOSIX_pollFDs(UA_EventLoopPOSIX *el, UA_DateTime listenTimeout);

/* Helper functions between EventSources */

UA_StatusCode
UA_EventLoopPOSIX_allocateRXBuffer(UA_POSIXConnectionManager *pcm);

UA_StatusCode
UA_EventLoopPOSIX_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize);

void
UA_EventLoopPOSIX_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf);

/*
 * Helper functions to be used across protocols
 */

/* Set the socket non-blocking. If the listen-socket is nonblocking, incoming
 * connections inherit this state. */
UA_StatusCode
UA_EventLoopPOSIX_setNonBlocking(UA_FD sockfd);

/* Don't have the socket create interrupt signals */
UA_StatusCode
UA_EventLoopPOSIX_setNoSigPipe(UA_FD sockfd);

/* Enables sharing of the same listening address on different sockets */
UA_StatusCode
UA_EventLoopPOSIX_setReusable(UA_FD sockfd);

_UA_END_DECLS

#endif /* defined(UA_ARCHITECTURE_POSIX) || defined(UA_ARCHITECTURE_WIN32) */

#endif /* UA_EVENTLOOP_POSIX_H_ */
