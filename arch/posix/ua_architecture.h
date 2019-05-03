/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_POSIX

#ifndef PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_

#include <open62541/architecture_base.h>

/* Enable POSIX features */
#if !defined(_XOPEN_SOURCE)
# define _XOPEN_SOURCE 600
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif
/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <net/if.h>
#ifndef UA_sleep_ms
# include <unistd.h>
# define UA_sleep_ms(X) usleep(X * 1000)
#endif

#define OPTVAL_TYPE int

#include <fcntl.h>
#include <unistd.h> // read, write, close

#ifdef __QNX__
# include <sys/socket.h>
#endif
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
# include <sys/param.h>
# if defined(BSD)
#  include<sys/socket.h>
# endif
#endif
#if !defined(__CYGWIN__)
# include <netinet/tcp.h>
#endif

/* unsigned int for windows and workaround to a glibc bug */
/* Additionally if GNU_LIBRARY is not defined, it may be using
 * musl libc (e.g. Docker Alpine) */
#if  defined(__OpenBSD__) || \
    (defined(__GNU_LIBRARY__) && (__GNU_LIBRARY__ <= 6) && \
     (__GLIBC__ <= 2) && (__GLIBC_MINOR__ < 16) || \
    !defined(__GNU_LIBRARY__))
# define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
# define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)
#else
# define UA_fd_set(fd, fds) FD_SET(fd, fds)
# define UA_fd_isset(fd, fds) FD_ISSET(fd, fds)
#endif

#define UA_access access

#define UA_IPV6 1
#define UA_SOCKET int
#define UA_INVALID_SOCKET -1
#define UA_ERRNO errno
#define UA_INTERRUPTED EINTR
#define UA_AGAIN EAGAIN
#define UA_EAGAIN EAGAIN
#define UA_WOULDBLOCK EWOULDBLOCK
#define UA_ERR_CONNECTION_PROGRESS EINPROGRESS

#define UA_ENABLE_LOG_COLORS

#define UA_getnameinfo getnameinfo
#define UA_send send
#define UA_recv recv
#define UA_sendto sendto
#define UA_recvfrom recvfrom
#define UA_htonl htonl
#define UA_ntohl ntohl
#define UA_close close
#define UA_select select
#define UA_shutdown shutdown
#define UA_socket socket
#define UA_bind bind
#define UA_listen listen
#define UA_accept accept
#define UA_connect connect
#define UA_getaddrinfo getaddrinfo
#define UA_getsockopt getsockopt
#define UA_setsockopt setsockopt
#define UA_freeaddrinfo freeaddrinfo
#define UA_gethostname gethostname
#define UA_getsockname getsockname
#define UA_inet_pton inet_pton
#if UA_IPV6
# define UA_if_nametoindex if_nametoindex
#endif

#ifdef UA_ENABLE_MALLOC_SINGLETON
extern void * (*UA_globalMalloc)(size_t size);
extern void (*UA_globalFree)(void *ptr);
extern void * (*UA_globalCalloc)(size_t nelem, size_t elsize);
extern void * (*UA_globalRealloc)(void *ptr, size_t size);
# define UA_free(ptr) UA_globalFree(ptr)
# define UA_malloc(size) UA_globalMalloc(size)
# define UA_calloc(num, size) UA_globalCalloc(num, size)
# define UA_realloc(ptr, size) UA_globalRealloc(ptr, size)
#endif

#include <stdlib.h>
#ifndef UA_free
# define UA_free free
#endif
#ifndef UA_malloc
# define UA_malloc malloc
#endif
#ifndef UA_calloc
# define UA_calloc calloc
#endif
#ifndef UA_realloc
# define UA_realloc realloc
#endif

#include <stdio.h>
#define UA_snprintf snprintf

#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    char *errno_str = strerror(errno); \
    LOG; \
}
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP(LOG) { \
    const char *errno_str = gai_strerror(errno); \
    LOG; \
}

#include <open62541/architecture_functions.h>

#if defined(__APPLE__)  && defined(_SYS_QUEUE_H_)
//  in some compilers there's already a _SYS_QUEUE_H_ which is included first and doesn't have all functions

#undef SLIST_HEAD
#undef SLIST_HEAD_INITIALIZER
#undef SLIST_ENTRY
#undef SLIST_FIRST
#undef SLIST_END
#undef SLIST_EMPTY
#undef SLIST_NEXT
#undef SLIST_FOREACH
#undef SLIST_FOREACH_SAFE
#undef SLIST_INIT
#undef SLIST_INSERT_AFTER
#undef SLIST_INSERT_HEAD
#undef SLIST_REMOVE_AFTER
#undef SLIST_REMOVE_HEAD
#undef SLIST_REMOVE
#undef LIST_HEAD
#undef LIST_HEAD_INITIALIZER
#undef LIST_ENTRY
#undef LIST_FIRST
#undef LIST_END
#undef LIST_EMPTY
#undef LIST_NEXT
#undef LIST_FOREACH
#undef LIST_FOREACH_SAFE
#undef LIST_INIT
#undef LIST_INSERT_AFTER
#undef LIST_INSERT_BEFORE
#undef LIST_INSERT_HEAD
#undef LIST_REMOVE
#undef LIST_REPLACE
#undef SIMPLEQ_HEAD
#undef SIMPLEQ_HEAD_INITIALIZER
#undef SIMPLEQ_ENTRY
#undef SIMPLEQ_FIRST
#undef SIMPLEQ_END
#undef SIMPLEQ_EMPTY
#undef SIMPLEQ_NEXT
#undef SIMPLEQ_FOREACH
#undef SIMPLEQ_FOREACH_SAFE
#undef SIMPLEQ_INIT
#undef SIMPLEQ_INSERT_HEAD
#undef SIMPLEQ_INSERT_TAIL
#undef SIMPLEQ_INSERT_AFTER
#undef SIMPLEQ_REMOVE_HEAD
#undef SIMPLEQ_REMOVE_AFTER
#undef XSIMPLEQ_HEAD
#undef XSIMPLEQ_ENTRY
#undef XSIMPLEQ_XOR
#undef XSIMPLEQ_FIRST
#undef XSIMPLEQ_END
#undef XSIMPLEQ_EMPTY
#undef XSIMPLEQ_NEXT
#undef XSIMPLEQ_FOREACH
#undef XSIMPLEQ_FOREACH_SAFE
#undef XSIMPLEQ_INIT
#undef XSIMPLEQ_INSERT_HEAD
#undef XSIMPLEQ_INSERT_TAIL
#undef XSIMPLEQ_INSERT_AFTER
#undef XSIMPLEQ_REMOVE_HEAD
#undef XSIMPLEQ_REMOVE_AFTER
#undef TAILQ_HEAD
#undef TAILQ_HEAD_INITIALIZER
#undef TAILQ_ENTRY
#undef TAILQ_FIRST
#undef TAILQ_END
#undef TAILQ_NEXT
#undef TAILQ_LAST
#undef TAILQ_PREV
#undef TAILQ_EMPTY
#undef TAILQ_FOREACH
#undef TAILQ_FOREACH_SAFE
#undef TAILQ_FOREACH_REVERSE
#undef TAILQ_FOREACH_REVERSE_SAFE
#undef TAILQ_INIT
#undef TAILQ_INSERT_HEAD
#undef TAILQ_INSERT_TAIL
#undef TAILQ_INSERT_AFTER
#undef TAILQ_INSERT_BEFORE
#undef TAILQ_REMOVE
#undef TAILQ_REPLACE
#undef CIRCLEQ_HEAD
#undef CIRCLEQ_HEAD_INITIALIZER
#undef CIRCLEQ_ENTRY
#undef CIRCLEQ_FIRST
#undef CIRCLEQ_LAST
#undef CIRCLEQ_END
#undef CIRCLEQ_NEXT
#undef CIRCLEQ_PREV
#undef CIRCLEQ_EMPTY
#undef CIRCLEQ_FOREACH
#undef CIRCLEQ_FOREACH_SAFE
#undef CIRCLEQ_FOREACH_REVERSE
#undef CIRCLEQ_FOREACH_REVERSE_SAFE
#undef CIRCLEQ_INIT
#undef CIRCLEQ_INSERT_AFTER
#undef CIRCLEQ_INSERT_BEFORE
#undef CIRCLEQ_INSERT_HEAD
#undef CIRCLEQ_INSERT_TAIL
#undef CIRCLEQ_REMOVE
#undef CIRCLEQ_REPLACE

#undef _SYS_QUEUE_H_

#endif /* defined(__APPLE__)  && defined(_SYS_QUEUE_H_) */


#endif /* PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_ */

#endif /* UA_ARCHITECTURE_POSIX */
