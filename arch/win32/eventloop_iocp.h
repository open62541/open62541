/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#ifndef UA_EVENTLOOP_IOCP_H_
#define UA_EVENTLOOP_IOCP_H_

#include <open62541/config.h>
#include <open62541/plugin/eventloop.h>

#include "../common/timer.h"
#include "../common/eventloop_common.h"
#include "../../deps/mp_printf.h"

# include "../deps/open62541_queue.h"


_UA_BEGIN_DECLS

#include <errno.h>


/*********************/
/* Win32 Definitions */
/*********************/

/* Disable some security warnings on MSVC */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
# define _CRT_SECURE_NO_WARNINGS
#endif

/*---------------------*/
/* Network Definitions */
/*---------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <basetsd.h>

#ifndef _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#endif

#define UA_IPV6 1
#define UA_SOCKET SOCKET
#define UA_INVALID_SOCKET INVALID_SOCKET
#define UA_RESET_ERRNO do { } while(0)
#define UA_ERRNO WSAGetLastError()
#define UA_INTERRUPTED WSAEINTR
#define UA_AGAIN EAGAIN /* the same as wouldblock on nearly every system */
#define UA_INPROGRESS WSAEINPROGRESS
#define UA_WOULDBLOCK WSAEWOULDBLOCK
#define UA_CONNRESET WSAECONNRESET
#define UA_NOBUFS WSAENOBUFS
#define UA_MFILE WSAEMFILE
#define UA_POLLIN POLLRDNORM
#define UA_POLLOUT POLLWRNORM
#define UA_SHUT_RDWR SD_BOTH

#define UA_IS_TEMPORARY_ACCEPT_ERROR(err) \
    ((err) == UA_INTERRUPTED || (err) == UA_CONNRESET || (err) == UA_NOBUFS || (err) == UA_MFILE)

#define UA_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags) \
    getnameinfo(sa, (socklen_t)salen, host, (DWORD)hostlen, serv, (DWORD)servlen, flags)
#define UA_poll(fds,nfds,timeout) WSAPoll((LPWSAPOLLFD)fds, nfds, timeout)
#define UA_send(sockfd, buf, len, flags) send(sockfd, buf, (int)(len), flags)
#define UA_recv(sockfd, buf, len, flags) recv(sockfd, buf, (int)(len), flags)
#define UA_sendto(sockfd, buf, len, flags, dest_addr, addrlen) \
    sendto(sockfd, (const char*)(buf), (int)(len), flags, dest_addr, (int) (addrlen))
#define UA_close closesocket
#define UA_select(nfds, readfds, writefds, exceptfds, timeout) \
    select((int)(nfds), readfds, writefds, exceptfds, timeout)
#define UA_connect(sockfd, addr, addrlen) connect(sockfd, addr, (int)(addrlen))
#define UA_getsockopt(sockfd, level, optname, optval, optlen) \
    getsockopt(sockfd, level, optname, (char*) (optval), optlen)
#define UA_setsockopt(sockfd, level, optname, optval, optlen) \
    setsockopt(sockfd, level, optname, (const char*) (optval), optlen)
#define UA_inet_pton InetPton
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

#if UA_IPV6
# define UA_if_nametoindex if_nametoindex

# include <iphlpapi.h>

#endif

#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) do { \
    char *errno_str = NULL; \
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
    NULL, WSAGetLastError(), \
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
    (LPSTR)&errno_str, 0, NULL); \
    LOG; \
    LocalFree(errno_str); \
} while (0)
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP UA_LOG_SOCKET_ERRNO_WRAP

/* Fix redefinition of SLIST_ENTRY on mingw winnt.h */
#if !defined(_SYS_QUEUE_H_) && defined(SLIST_ENTRY)
# undef SLIST_ENTRY
#endif

/*---------------------------*/
/* File Handling Definitions */
/*---------------------------*/

#include <direct.h>
#include <minwindef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include "tr_dirent.h"

_UA_BEGIN_DECLS
char *
_UA_dirname_minimal(char *path);
_UA_END_DECLS

#define UA_STAT stat
#define UA_DIR DIR
#define UA_DIRENT dirent
#define UA_FILE FILE
#define UA_MODE uint16_t

#define UA_stat stat
#define UA_opendir opendir
#define UA_readdir readdir
#define UA_rewinddir rewinddir
#define UA_closedir closedir
#define UA_mkdir(path, mode) _mkdir(path)
#define UA_fopen fopen
#define UA_fread fread
#define UA_fwrite fwrite
#define UA_fseek fseek
#define UA_ftell ftell
#define UA_fclose fclose
#define UA_remove remove
#define UA_dirname _UA_dirname_minimal

#define UA_SEEK_END SEEK_END
#define UA_SEEK_SET SEEK_SET
#define UA_DT_REG DT_REG
#define UA_DT_DIR DT_DIR
#define UA_PATH_MAX MAX_PATH
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
} UA_WIN32ConnectionManager;

typedef struct UA_EventLoopWIN32 UA_EventLoopWIN32;
struct UA_EventLoopWIN32 {
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
     * ...) always call the same UA_EventLoopWIN32_registerFD/modifyFD/
     * deregisterFD wrapper functions regardless of the backend in use. */
    UA_StatusCode (*registerFD)(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);
    UA_StatusCode (*modifyFD)(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);
    void (*deregisterFD)(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);

    /* Flat array of all fds registered in the EventLoop. Used by the select
     * and GLib backends (epoll keeps the rfd pointer directly in
     * epoll_event.data.ptr instead and does not need this list). */
    UA_RegisteredFD **fds;
    size_t fdsSize;

    /* Self-pipe to cancel a blocking select()/epoll_wait() */
    UA_FD selfpipe[2]; /* 0: read, 1: write */



#if UA_MULTITHREADING >= 100
    UA_Lock elMutex;
#endif
};

/* The following wrapper functions dispatch through el->registerFD/modifyFD/
 * deregisterFD. The actual implementation differs between the select, epoll
 * and GLib backends (see UA_EventLoop_new_POSIX / UA_EventLoop_new_GLib). */

/* Register to start receiving events */
UA_StatusCode
UA_EventLoopWIN32_registerFD(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);

/* Modify the events that the fd listens on */
UA_StatusCode
UA_EventLoopWIN32_modifyFD(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);

/* Deregister but do not close the fd. No further events are received. */
void
UA_EventLoopWIN32_deregisterFD(UA_EventLoopWIN32 *el, UA_RegisteredFD *rfd);

/* Only used internally by the select/epoll backend's own "run" method. */
UA_StatusCode
UA_EventLoopWIN32_pollFDs(UA_EventLoopWIN32 *el, UA_DateTime listenTimeout);

/* Timer, delayed-callback and DateTime methods. These only operate on the
 * generic (backend-independent) parts of UA_EventLoopWIN32. They are shared
 * between the select/epoll backend (UA_EventLoop_new_POSIX) and the GLib
 * backend (UA_EventLoop_new_GLib, see eventloop_glib.c). */

UA_DateTime
UA_EventLoopWIN32_nextTimer(UA_EventLoop *public_el);

UA_StatusCode
UA_EventLoopWIN32_addTimer(UA_EventLoop *public_el, UA_Callback cb,
                           void *application, void *data, UA_Double interval_ms,
                           UA_DateTime *baseTime, UA_TimerPolicy timerPolicy,
                           UA_UInt64 *callbackId);

UA_StatusCode
UA_EventLoopWIN32_modifyTimer(UA_EventLoop *public_el, UA_UInt64 callbackId,
                              UA_Double interval_ms, UA_DateTime *baseTime,
                              UA_TimerPolicy timerPolicy);

void
UA_EventLoopWIN32_removeTimer(UA_EventLoop *public_el, UA_UInt64 callbackId);

void
UA_EventLoopWIN32_removeDelayedCallback(UA_EventLoop *public_el,
                                        UA_DelayedCallback *dc);

/* Executes all queued delayed callbacks. Caller must hold el->elMutex. */
void
UA_EventLoopWIN32_processDelayed(UA_EventLoopWIN32 *el);

UA_DateTime
UA_EventLoopWIN32_DateTime_now(UA_EventLoop *el);

UA_DateTime
UA_EventLoopWIN32_DateTime_nowMonotonic(UA_EventLoop *el);

UA_Int64
UA_EventLoopWIN32_DateTime_localTimeUtcOffset(UA_EventLoop *el);

/* Register/deregister an EventSource. Caller must hold el->elMutex is NOT
 * required -- these take the lock themselves. Shared between the select/
 * epoll and GLib backends. */
UA_StatusCode
UA_EventLoopWIN32_registerEventSource(UA_EventLoop *el, UA_EventSource *es);

UA_StatusCode
UA_EventLoopWIN32_deregisterEventSource(UA_EventLoop *el, UA_EventSource *es);

void
UA_EventLoopWIN32_lock(UA_EventLoop *public_el);

void
UA_EventLoopWIN32_unlock(UA_EventLoop *public_el);

/* Helper functions across EventSources */

UA_StatusCode
UA_EventLoopWIN32_allocateStaticBuffers(UA_WIN32ConnectionManager *pcm);

UA_StatusCode
UA_EventLoopWIN32_allocNetworkBuffer(UA_ConnectionManager *cm,
                                     uintptr_t connectionId,
                                     UA_ByteString *buf,
                                     size_t bufSize);

void
UA_EventLoopWIN32_freeNetworkBuffer(UA_ConnectionManager *cm,
                                    uintptr_t connectionId,
                                    UA_ByteString *buf);

/* Set the socket non-blocking. If the listen-socket is nonblocking, incoming
 * connections inherit this state. */
UA_StatusCode
UA_EventLoopWIN32_setNonBlocking(UA_FD sockfd);

/* Don't have the socket create interrupt signals */
UA_StatusCode
UA_EventLoopWIN32_setNoSigPipe(UA_FD sockfd);

/* Enables sharing of the same listening address on different sockets */
UA_StatusCode
UA_EventLoopWIN32_setReusable(UA_FD sockfd);

/* Windows has no pipes. Use a local TCP connection for the self-pipe trick.
 * https://stackoverflow.com/a/3333565
 * On POSIX, use a socketpair (AF_UNIX) for uniform socket semantics. */
int UA_EventLoopWIN32_pipe(SOCKET fds[2]);

/* Cancel the current _run by sending to the self-pipe */
void
UA_EventLoopWIN32_cancel(UA_EventLoop *el);

void
UA_EventLoopWIN32_addDelayedCallback(UA_EventLoop *public_el,
                                     UA_DelayedCallback *dc);

_UA_END_DECLS


#endif /* UA_EVENTLOOP_IOCP_H_ */
