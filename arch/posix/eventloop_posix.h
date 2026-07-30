/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_EVENTLOOP_POSIX_H_
#define UA_EVENTLOOP_POSIX_H_

#include <open62541/config.h>
#include <open62541/plugin/eventloop.h>

#include "../common/timer.h"
#include "../common/eventloop_common.h"
#include "../../deps/mp_printf.h"

#if !defined(__QNX__)
# include "../deps/open62541_queue.h"
#endif

#if defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP)

_UA_BEGIN_DECLS

#include <errno.h>


/*********************/
/* POSIX Definitions */
/*********************/

#include <time.h>

/*---------------------*/
/* Network Definitions */
/*---------------------*/

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <net/if.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <ifaddrs.h>

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
# include <sys/param.h>
# if defined(BSD)
#  include <sys/socket.h>
# endif
#endif

#if defined (__APPLE__)
typedef int SOCKET;
#endif

#define UA_IPV6 1
#define UA_SOCKET int
#define UA_INVALID_SOCKET -1
#define UA_RESET_ERRNO do { errno = 0; } while(0)
#define UA_ERRNO errno
#define UA_INTERRUPTED EINTR
#define UA_AGAIN EAGAIN /* the same as wouldblock on nearly every system */
#define UA_INPROGRESS EINPROGRESS
#define UA_WOULDBLOCK EWOULDBLOCK
#define UA_CONNABORTED ECONNABORTED
#define UA_MFILE EMFILE
#define UA_NFILE ENFILE
#define UA_NOBUFS ENOBUFS
#define UA_POLLIN POLLIN
#define UA_POLLOUT POLLOUT
#define UA_SHUT_RDWR SHUT_RDWR

#define UA_IS_TEMPORARY_ACCEPT_ERROR(err) \
    ((err) == UA_INTERRUPTED || (err) == UA_CONNABORTED || (err) == UA_MFILE || (err) == UA_NFILE || (err) == UA_NOBUFS)

#define UA_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags) \
    getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)
#define UA_poll poll
#define UA_send send
#define UA_recv recv
#define UA_sendto sendto
#define UA_close close
#define UA_select select
#define UA_connect connect
#define UA_getsockopt getsockopt
#define UA_setsockopt setsockopt
#define UA_inet_pton inet_pton
#define UA_if_nametoindex if_nametoindex
#define UA_socket socket
#define UA_bind bind
#define UA_recvfrom recvfrom
#define UA_accept accept
#define UA_listen listen
#define UA_shutdown shutdown
#define UA_getaddrinfo getaddrinfo
#define UA_freeaddrinfo freeaddrinfo
#define UA_inet_ntop inet_ntop
#define UA_getsockname getsockname
#define UA_gethostname gethostname

#define UA_clean_errno(STR_FUN) \
    (errno == 0 ? (char*) "None" : (STR_FUN)(errno))
#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) \
    do { char *errno_str = UA_clean_errno(strerror); LOG; errno = 0; } while (0)
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP(LOG) \
    do { const char *errno_str = UA_clean_errno(gai_strerror); LOG; errno = 0; } while (0)

/* epoll_pwait returns bogus data with the tc compiler */
#if defined(__linux__) && !defined(__TINYC__)
# define UA_HAVE_EPOLL
# include <sys/epoll.h>
#endif

/*---------------------------*/
/* File Handling Definitions */
/*---------------------------*/

#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#ifdef __linux__
# include <sys/inotify.h>
#endif /* __linux__ */
#include <sys/stat.h>

#define UA_STAT stat
#define UA_DIR DIR
#define UA_DIRENT dirent
#define UA_FILE FILE
#define UA_MODE mode_t

#define UA_stat stat
#define UA_opendir opendir
#define UA_readdir readdir
#define UA_rewinddir rewinddir
#define UA_closedir closedir
#define UA_mkdir mkdir
#define UA_fopen fopen
#define UA_fread fread
#define UA_fwrite fwrite
#define UA_fseek fseek
#define UA_ftell ftell
#define UA_fclose fclose
#define UA_remove remove
#define UA_dirname dirname

#define UA_SEEK_END SEEK_END
#define UA_SEEK_SET SEEK_SET
#define UA_DT_REG DT_REG
#define UA_DT_DIR DT_DIR
#define UA_PATH_MAX PATH_MAX
#define UA_FILENAME_MAX FILENAME_MAX


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

    /* Information to reopen listen socket */
    UA_String hostname;
    UA_UInt16 port;
    UA_Boolean reuseaddr;

    UA_EventSource *es; /* Backpointer to the EventSource */
    UA_FDCallback eventSourceCB;

#ifdef UA_ENABLE_EVENTLOOP_GLIB
    /* Heap-allocated GPollFD used to register this fd with the GLib
     * EventLoop backend (arch/posix/eventloop_glib.c). Must have a stable
     * address for the lifetime of the registration, as GLib keeps a pointer
     * to it. NULL unless the fd is registered in a GLib-backed EventLoop. */
    void *glibPollFD;
#endif
};

enum ZIP_CMP cmpFD(const UA_FD *a, const UA_FD *b);
typedef ZIP_HEAD(UA_FDTree, UA_RegisteredFD) UA_FDTree;
ZIP_FUNCTIONS(UA_FDTree, UA_RegisteredFD, zipPointers, UA_FD, fd, cmpFD)

typedef struct UA_DeregisteredListenFD {
    LIST_ENTRY(UA_DeregisteredListenFD) pointers;
    UA_RegisteredFD *listenFd;
} UA_DeregisteredListenFD;

typedef LIST_HEAD(UA_DeregisteredListenFDList, UA_DeregisteredListenFD) UA_DeregisteredListenFDList;

/* All ConnectionManager in the POSIX EventLoop can be cast to
 * UA_ConnectionManagerPOSIX. They carry a sorted tree of their open
 * sockets/file-descriptors. */
typedef struct {
    UA_ConnectionManager cm;

    /* Statically allocated buffers */
    UA_ByteString rxBuffer;
    UA_ByteString txBuffer;

    /* Sorted tree of the FDs */
    size_t fdsSize;
    UA_FDTree fds;

    /* Closed listening sockets queued for later reopening */
    UA_DeregisteredListenFDList listenFDs;
} UA_POSIXConnectionManager;

typedef struct UA_EventLoopPOSIX UA_EventLoopPOSIX;
struct UA_EventLoopPOSIX {
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
    UA_atomic(UA_DelayedCallback *) delayedHead1;
    UA_atomic(UA_DelayedCallback *) delayedHead2;
    UA_atomic(UA_atomic(UA_DelayedCallback *)*) delayedTail;

    /* Flag determining whether the eventloop is currently within the
     * "run" method */
    volatile UA_Boolean executing;

    /* Indicates that the maximum number of sockets has been reached.
     * All listening sockets will be closed. */
    UA_Boolean maxSocketsLimitReached;

    /* Clocks for the eventloop's time domain */
    UA_Int32 clockSource;
    UA_Int32 clockSourceMonotonic;

    /* FD polling backend. Selected once when the EventLoop instance is
     * created (UA_EventLoop_new_POSIX or UA_EventLoop_new_GLib) and not
     * changed afterwards. This indirection allows EventLoop instances with
     * different polling backends (select, epoll, GLib, ...) to coexist in
     * the same process, while the ConnectionManagers (TCP, UDP, Ethernet,
     * ...) always call the same UA_EventLoopPOSIX_registerFD/modifyFD/
     * deregisterFD wrapper functions regardless of the backend in use. */
    UA_StatusCode (*registerFD)(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);
    UA_StatusCode (*modifyFD)(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);
    void (*deregisterFD)(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);

#if defined(UA_HAVE_EPOLL)
    UA_FD epollfd;
#endif
    /* Flat array of all fds registered in the EventLoop. Used by the select
     * and GLib backends (epoll keeps the rfd pointer directly in
     * epoll_event.data.ptr instead and does not need this list). */
    UA_RegisteredFD **fds;
    size_t fdsSize;

    /* Self-pipe to cancel a blocking select()/epoll_wait() */
    UA_FD selfpipe[2]; /* 0: read, 1: write */

#ifdef UA_ENABLE_LWS
    /* One libwebsockets context shared by all users of this EventLoop */
    void *lwsContext;
    void *lwsLogContext;
    size_t lwsContextUsers;
    UA_EventLoop *lwsForeignLoop;
#endif

#ifdef UA_ENABLE_EVENTLOOP_GLIB
    /* GLib backend state. Only used for EventLoop instances created via
     * UA_EventLoop_new_GLib (see arch/posix/eventloop_glib.c). Kept here
     * (instead of a separate struct) so that the ConnectionManagers can cast
     * any EventLoop of this architecture to UA_EventLoopPOSIX* regardless of
     * the backend that created it. */
    void *glibContext; /* GMainContext* */
    void *glibSource;  /* GSource* */
#endif

#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
};

/* The following wrapper functions dispatch through el->registerFD/modifyFD/
 * deregisterFD. The actual implementation differs between the select, epoll
 * and GLib backends (see UA_EventLoop_new_POSIX / UA_EventLoop_new_GLib). */

/* Register to start receiving events */
UA_StatusCode
UA_EventLoopPOSIX_registerFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);

/* Modify the events that the fd listens on */
UA_StatusCode
UA_EventLoopPOSIX_modifyFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);

/* Deregister but do not close the fd. No further events are received. */
void
UA_EventLoopPOSIX_deregisterFD(UA_EventLoopPOSIX *el, UA_RegisteredFD *rfd);

/* Only used internally by the select/epoll backend's own "run" method. */
UA_StatusCode
UA_EventLoopPOSIX_pollFDs(UA_EventLoopPOSIX *el, UA_DateTime listenTimeout);

/* Timer, delayed-callback and DateTime methods. These only operate on the
 * generic (backend-independent) parts of UA_EventLoopPOSIX. They are shared
 * between the select/epoll backend (UA_EventLoop_new_POSIX) and the GLib
 * backend (UA_EventLoop_new_GLib, see eventloop_glib.c). */

UA_DateTime
UA_EventLoopPOSIX_nextTimer(UA_EventLoop *public_el);

UA_StatusCode
UA_EventLoopPOSIX_addTimer(UA_EventLoop *public_el, UA_Callback cb,
                           void *application, void *data, UA_Double interval_ms,
                           UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                           UA_UInt64 *callbackId);

UA_StatusCode
UA_EventLoopPOSIX_modifyTimer(UA_EventLoop *public_el, UA_UInt64 callbackId,
                              UA_Double interval_ms, UA_DateTime *baseTime,
                              UA_TimerPolicy timerPolicy);

void
UA_EventLoopPOSIX_removeTimer(UA_EventLoop *public_el, UA_UInt64 callbackId);

void
UA_EventLoopPOSIX_removeDelayedCallback(UA_EventLoop *public_el,
                                        UA_DelayedCallback *dc);

/* Executes all queued delayed callbacks. Caller must hold el->elMutex. */
void
UA_EventLoopPOSIX_processDelayed(UA_EventLoopPOSIX *el);

UA_DateTime
UA_EventLoopPOSIX_DateTime_now(UA_EventLoop *el);

UA_DateTime
UA_EventLoopPOSIX_DateTime_nowMonotonic(UA_EventLoop *el);

UA_Int64
UA_EventLoopPOSIX_DateTime_localTimeUtcOffset(UA_EventLoop *el);

/* Register/deregister an EventSource. Caller must hold el->elMutex is NOT
 * required -- these take the lock themselves. Shared between the select/
 * epoll and GLib backends. */
UA_StatusCode
UA_EventLoopPOSIX_registerEventSource(UA_EventLoop *el, UA_EventSource *es);

UA_StatusCode
UA_EventLoopPOSIX_deregisterEventSource(UA_EventLoop *el, UA_EventSource *es);

void
UA_EventLoopPOSIX_lock(UA_EventLoop *public_el);

void
UA_EventLoopPOSIX_unlock(UA_EventLoop *public_el);

/* Helper functions across EventSources */

UA_StatusCode
UA_EventLoopPOSIX_allocateStaticBuffers(UA_POSIXConnectionManager *pcm);

UA_StatusCode
UA_EventLoopPOSIX_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize);

void
UA_EventLoopPOSIX_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf);

#ifdef UA_ENABLE_LWS
void
UA_LWS_destroyContext(UA_EventLoop *eventLoop);
#endif

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

/* Use a socketpair (AF_UNIX) for uniform socket semantics. */
int UA_EventLoopPOSIX_pipe(UA_FD fds[2]);

/* Cancel the current _run by sending to the self-pipe */
void
UA_EventLoopPOSIX_cancel(UA_EventLoop *el);

void
UA_EventLoopPOSIX_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc);

_UA_END_DECLS

#endif /* defined(UA_ARCHITECTURE_POSIX) && !defined(UA_ARCHITECTURE_LWIP) */

#endif /* UA_EVENTLOOP_POSIX_H_ */
