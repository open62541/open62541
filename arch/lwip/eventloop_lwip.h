/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#ifndef UA_EVENTLOOP_LWIP_H_
#define UA_EVENTLOOP_LWIP_H_

#include <open62541/config.h>
#include <open62541/plugin/eventloop.h>

#if defined(UA_ARCHITECTURE_LWIP)

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <lwip/errno.h>

#include "../../deps/mp_printf.h"
#include "../common/timer.h"

#include "settings_lwip.h"

/*********************/
/* LWIP Definitions */
/*********************/

#define NI_NUMERICHOST 1  // Return numeric host
#define NI_NUMERICSERV 2  // Return numeric service
#define NI_NOFQDN       4  // Do not return FQDN (fully qualified domain name)
#define NI_NAMEREQD     8  // Error if the name cannot be resolved

#define NI_MAXHOST 1025
#define NI_MAXSERV 32

#if !LWIP_DNS
#define AI_PASSIVE      0x01
#define AI_CANONNAME    0x02
#define AI_NUMERICHOST  0x04
#define AI_NUMERICSERV  0x08
#define AI_V4MAPPED     0x10
#define AI_ALL          0x20
#define AI_ADDRCONFIG   0x40

struct addrinfo {
    int               ai_flags;      /* Input flags. */
    int               ai_family;     /* Address family of socket. */
    int               ai_socktype;   /* Socket type. */
    int               ai_protocol;   /* Protocol of socket. */
    socklen_t         ai_addrlen;    /* Length of socket address. */
    struct sockaddr  *ai_addr;       /* Socket address of socket. */
    char             *ai_canonname;  /* Canonical name of service location. */
    struct addrinfo  *ai_next;       /* Pointer to next in list. */
};
#endif


/* Custom function to replicate getnameinfo */
static UA_INLINE int
lwip_getnameinfo(const struct sockaddr *sa, socklen_t salen,
                 char *host, size_t hostlen,
                 char *serv, size_t servlen, int flags) {
    if(sa->sa_family == AF_INET) {
        const struct sockaddr_in *addr_in = (const struct sockaddr_in *)sa;

        if(serv) {
            mp_snprintf(serv, servlen, "%u", ntohs(addr_in->sin_port));
        }

        if(host) {
            if(flags & NI_NUMERICHOST) {
                /* Return numeric IP address */
                strncpy(host, inet_ntoa(addr_in->sin_addr), hostlen);
            } else {
                /* Perform reverse DNS lookup (not provided by lwIP, can use lwIP's DNS if desired) */
                strncpy(host, inet_ntoa(addr_in->sin_addr), hostlen); // Fallback to numeric IP
            }
        }
    } else {
        /* AF_INET6 handling can be added similarly for IPv6 addresses */
        return -1; /* Unsupported address family */
    }
    return 0;
}

static UA_INLINE int
lwip_gethostname(char *name, size_t len) {
    if(!name || len <= 0)
        return -1;

    size_t hostnameLen = strlen(HOSTNAME);

    if(hostnameLen >= len)
        return -1;

    strncpy(name, HOSTNAME, len);
    name[len - 1] = '\0';

    return 0;
}

#define UA_IPV6 0
#define UA_SOCKET int
#define UA_INVALID_SOCKET -1
#define UA_ERRNO errno
#define UA_INTERRUPTED EINTR
#define UA_AGAIN EAGAIN /* the same as wouldblock on nearly every system */
#define UA_INPROGRESS EINPROGRESS
#define UA_WOULDBLOCK EWOULDBLOCK
#define UA_POLLIN POLLIN
#define UA_POLLOUT POLLOUT
#define UA_SHUT_RDWR SHUT_RDWR

#define UA_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags) \
    lwip_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)
#define UA_gethostname(name, len) lwip_gethostname(name, len)
#define UA_poll lwip_poll
#define UA_socket lwip_socket
#define UA_bind lwip_bind
#define UA_listen lwip_listen
#define UA_accept lwip_accept
#define UA_send lwip_send
#define UA_recv lwip_recv
#define UA_sendto lwip_sendto
#define UA_close lwip_close
#define UA_select lwip_select
#define UA_connect lwip_connect
#define UA_getsockopt lwip_getsockopt
#define UA_setsockopt lwip_setsockopt
#define UA_inet_pton lwip_inet_pton
#define UA_if_nametoindex lwip_if_nametoindex
#define UA_getsockname lwip_getsockname
// #define UA_getaddrinfo lwip_getaddrinfo
// #define UA_freeaddrinfo lwip_freeaddrinfo

#define UA_clean_errno(STR_FUN) (errno == 0 ? (char*) "None" : (STR_FUN)(errno))
#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) {                                                  \
    char *errno_str = UA_clean_errno(strerror);                                          \
    LOG;                                                                                 \
    errno = 0;                                                                           \
}
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP(LOG) {                                              \
    const char *errno_str = UA_clean_errno(strerror);                                    \
    LOG;                                                                                 \
    errno = 0;                                                                           \
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

/* LWIP events are based on sockets / file descriptors. The EventSources can
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

/* All ConnectionManager in the LWIP EventLoop can be cast to
 * UA_ConnectionManagerLWIP. They carry a sorted tree of their open
 * sockets/file-descriptors. */
typedef struct {
    UA_ConnectionManager cm;

    /* Statically allocated buffers */
    UA_ByteString rxBuffer;
    UA_ByteString txBuffer;

    /* Sorted tree of the FDs */
    size_t fdsSize;
    UA_FDTree fds;
} UA_LWIPConnectionManager;

typedef struct {
    UA_EventLoop eventLoop;

    /* Timer */
    UA_Timer timer;

    /* Singly-linked FIFO queue (lock-free multi-producer single-consumer) of
     * delayed callbacks. Insertion happens by chasing the tail-pointer. We
     * "check out" the current queue and reset by switching the tail to the
     * alternative head-pointer.
     *
     * This could be a simple singly-linked list. But we want to do in-order
     * processing so we can wait until the worker jobs already in the queue get
     * finished before.
     *
     * The currently unused head gets marked with the 0x01 sentinel. */
    UA_DelayedCallback *delayedHead1;
    UA_DelayedCallback *delayedHead2;
    UA_DelayedCallback **delayedTail;

    /* Flag determining whether the eventloop is currently within the
     * "run" method */
    volatile UA_Boolean executing;

#if defined(UA_ARCHITECTURE_LWIP) && !defined(__APPLE__) && !defined(__MACH__)
    /* Clocks for the eventloop's time domain */
    UA_Int32 clockSource;
    UA_Int32 clockSourceMonotonic;
#endif

    UA_RegisteredFD **fds;
    size_t fdsSize;

    /* Self-pipe to cancel blocking wait */
    UA_FD selfpipe[2]; /* 0: read, 1: write */

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
} UA_EventLoopLWIP;

/* The following functions differ between epoll and normal select */

/* Register to start receiving events */
UA_StatusCode
UA_EventLoopLWIP_registerFD(UA_EventLoopLWIP *el, UA_RegisteredFD *rfd);

/* Modify the events that the fd listens on */
UA_StatusCode
UA_EventLoopLWIP_modifyFD(UA_EventLoopLWIP *el, UA_RegisteredFD *rfd);

/* Deregister but do not close the fd. No further events are received. */
void
UA_EventLoopLWIP_deregisterFD(UA_EventLoopLWIP *el, UA_RegisteredFD *rfd);

UA_StatusCode
UA_EventLoopLWIP_pollFDs(UA_EventLoopLWIP *el, UA_DateTime listenTimeout);

/* Helper functions across EventSources */

UA_StatusCode
UA_EventLoopLWIP_allocateStaticBuffers(UA_LWIPConnectionManager *pcm);

UA_StatusCode
UA_EventLoopLWIP_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize);

void
UA_EventLoopLWIP_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf);

/* Set the socket non-blocking. If the listen-socket is nonblocking, incoming
 * connections inherit this state. */
UA_StatusCode
UA_EventLoopLWIP_setNonBlocking(UA_FD sockfd);

/* Don't have the socket create interrupt signals */
UA_StatusCode
UA_EventLoopLWIP_setNoSigPipe(UA_FD sockfd);

/* Enables sharing of the same listening address on different sockets */
UA_StatusCode
UA_EventLoopLWIP_setReusable(UA_FD sockfd);

/* Windows has no pipes. Use a local TCP connection for the self-pipe trick.
 * https://stackoverflow.com/a/3333565 */
int
UA_EventLoopLWIP_pipe(UA_FD fds[2]);

/* Cancel the current _run by sending to the self-pipe */
void
UA_EventLoopLWIP_cancel(UA_EventLoopLWIP *el);

void
UA_EventLoopLWIP_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc);

_UA_END_DECLS

#endif /* defined(UA_ARCHITECTURE_LWIP) */

#endif /* UA_EVENTLOOP_LWIP_H_ */
