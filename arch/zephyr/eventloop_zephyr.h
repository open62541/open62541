/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and
 * umati) Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer) Copyright 2021 (c)
 *    Copyright 2024 (c) Julian Weiß, PRIMES GmbH
 * Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_EVENTLOOP_Zephyr_H_
#define UA_EVENTLOOP_Zephyr_H_

#include <open62541/config.h>
#include <open62541/plugin/eventloop.h>

#if defined(UA_ARCHITECTURE_ZEPHYR)

#include <errno.h>

#include "../../deps/mp_printf.h"
#include "../../deps/open62541_queue.h"
#include "../eventloop_common/eventloop_common.h"
#include "../eventloop_common/timer.h"


/*********************/
/* Zephyr Definitions */
/*********************/

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/fcntl.h>

#define UA_IPV6 1
#define UA_INVALID_SOCKET -1
#define UA_ERRNO errno
#define UA_INTERRUPTED EINTR
#define UA_AGAIN EAGAIN /* the same as wouldblock on nearly every system */
#define UA_INPROGRESS EINPROGRESS
#define UA_WOULDBLOCK EWOULDBLOCK
#define UA_POLLIN ZSOCK_POLLIN
#define UA_POLLOUT ZSOCK_POLLOUT
#define UA_SHUT_RDWR ZSOCK_SHUT_RDWR

#define UA_socket zsock_socket
#define UA_accept zsock_accept
#define UA_getnameinfo zsock_getnameinfo
#define UA_listen zsock_listen
#define UA_bind zsock_bind
#define UA_poll zsock_poll
#define UA_send zsock_send
#define UA_recv zsock_recv
#define UA_sendto zsock_sendto
#define UA_close zsock_close
#define UA_select zsock_select
#define UA_connect zsock_connect
#define UA_getsockopt zsock_getsockopt
#define UA_setsockopt zsock_setsockopt
#define UA_getsockname zsock_getsockname
#define UA_inet_pton zsock_inet_pton
#define UA_if_nametoindex zsock_if_nametoindex
#define UA_FD_SET ZSOCK_FD_SET
#define UA_FD_ISSET ZSOCK_FD_ISSET
#define UA_FD_ZERO ZSOCK_FD_ZERO
#define UA_fcntl zsock_fcntl
#define UA_getaddrinfo zsock_getaddrinfo
#define UA_gai_strerror zsock_gai_strerror
#define UA_shutdown zsock_shutdown
#define UA_freeaddrinfo zsock_freeaddrinfo
#define UA_gethostname zsock_gethostname

typedef int UA_socket_fd;
typedef struct zsock_fd_set UA_fd_set;
typedef struct zsock_addrinfo UA_addrinfo;
typedef struct zsock_pollfd UA_pollfd;

#define UA_clean_errno(STR_FUN) (errno == 0 ? (char *)"None" : (STR_FUN)(errno))
#define UA_LOG_SOCKET_ERRNO_WRAP(LOG)                                                    \
    {                                                                                    \
        char *errno_str = UA_clean_errno(strerror);                                      \
        LOG;                                                                             \
        errno = 0;                                                                       \
    }
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP(LOG)                                                \
    {                                                                                    \
        const char *errno_str = UA_clean_errno(gai_strerror);                            \
        LOG;                                                                             \
        errno = 0;                                                                       \
    }

/***********************/
/* General Definitions */
/***********************/

#define UA_MAXBACKLOG 100
#define UA_MAXHOSTNAME_LENGTH 256
#define UA_MAXPORTSTR_LENGTH 6

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

/* Zephyr events are based on sockets / file descriptors. The EventSources can
 * register their fd in the EventLoop so that they are considered by the
 * EventLoop dropping into "poll" to wait for events. */

/* TODO: Move the macro-forest from /arch/<arch>/ua_architecture.h */

typedef UA_socket_fd UA_fd;
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
    UA_fd fd;
    short listenEvents; /* UA_FDEVENT_IN | UA_FDEVENT_OUT*/

    UA_EventSource *es; /* Backpointer to the EventSource */
    UA_FDCallback eventSourceCB;
};

enum ZIP_CMP
cmpFD(const UA_fd *a, const UA_fd *b);
typedef ZIP_HEAD(UA_FDTree, UA_RegisteredFD) UA_FDTree;
ZIP_FUNCTIONS(UA_FDTree, UA_RegisteredFD, zipPointers, UA_fd, fd, cmpFD)

/* All ConnectionManager in the Zephyr EventLoop can be cast to
 * UA_ConnectionManagerZephyr. They carry a sorted tree of their open
 * sockets/file-descriptors. */
typedef struct {
    UA_ConnectionManager cm;

    /* Statically allocated buffers */
    UA_ByteString rxBuffer;
    UA_ByteString txBuffer;

    /* Sorted tree of the FDs */
    size_t fdsSize;
    UA_FDTree fds;
} UA_ZephyrConnectionManager;

typedef struct {
    UA_EventLoop eventLoop;

    /* Timer */
    UA_Timer timer;

    /* Linked List of Delayed Callbacks */
    UA_DelayedCallback *delayedCallbacks;

    /* Flag determining whether the eventloop is currently within the
     * "run" method */
    volatile UA_Boolean executing;

    UA_RegisteredFD **fds;
    size_t fdsSize;

    /* Event fd to cancel blocking wait */
    UA_fd ef;

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
} UA_EventLoopZephyr;

/* The following functions differ between epoll and normal select */

/* Register to start receiving events */
UA_StatusCode
UA_EventLoopZephyr_registerFD(UA_EventLoopZephyr *el, UA_RegisteredFD *rfd);

/* Modify the events that the fd listens on */
UA_StatusCode
UA_EventLoopZephyr_modifyFD(UA_EventLoopZephyr *el, UA_RegisteredFD *rfd);

/* Deregister but do not close the fd. No further events are received. */
void
UA_EventLoopZephyr_deregisterFD(UA_EventLoopZephyr *el, UA_RegisteredFD *rfd);

UA_StatusCode
UA_EventLoopZephyr_pollFDs(UA_EventLoopZephyr *el, UA_DateTime listenTimeout);

/* Helper functions across EventSources */

UA_StatusCode
UA_EventLoopZephyr_allocateStaticBuffers(UA_ZephyrConnectionManager *pcm);

UA_StatusCode
UA_EventLoopZephyr_allocNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                                      UA_ByteString *buf, size_t bufSize);

void
UA_EventLoopZephyr_freeNetworkBuffer(UA_ConnectionManager *cm, uintptr_t connectionId,
                                     UA_ByteString *buf);

/* Set the socket non-blocking. If the listen-socket is nonblocking, incoming
 * connections inherit this state. */
UA_StatusCode
UA_EventLoopZephyr_setNonBlocking(UA_fd sockfd);

/* Don't have the socket create interrupt signals */
UA_StatusCode
UA_EventLoopZephyr_setNoSigPipe(UA_fd sockfd);

/* Enables sharing of the same listening address on different sockets */
UA_StatusCode
UA_EventLoopZephyr_setReusable(UA_fd sockfd);

/* Windows has no pipes. Use a local TCP connection for the self-pipe trick.
 * https://stackoverflow.com/a/3333565 */
int
UA_EventLoopZephyr_pipe(UA_fd fds[2]);

/* Cancel the current _run by sending to the self-pipe */
void
UA_EventLoopZephyr_cancel(UA_EventLoopZephyr *el);

void
UA_EventLoopZephyr_addDelayedCallback(UA_EventLoop *public_el, UA_DelayedCallback *dc);

#endif /* defined(UA_ARCHITECTURE_Zephyr) */
#endif
