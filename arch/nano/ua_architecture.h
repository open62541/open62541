/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2021 (c) Mark JACKSON, Newflow Ltd
 */

#ifdef UA_ARCHITECTURE_NANO

#ifndef PLUGINS_ARCH_NANO_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_NANO_UA_ARCHITECTURE_H_

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

#include <open62541/architecture_definitions.h>

#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <net/if.h>
#ifdef UA_sleep_ms
void UA_sleep_ms(unsigned long ms);
#else
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

#include <netinet/tcp.h>

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

#define UA_IPV6 0
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

/* Use the standard malloc */
#include <stdlib.h>
#ifndef UA_free
# define UA_free free
# define UA_malloc malloc
# define UA_calloc calloc
# define UA_realloc realloc
#endif

#include <stdio.h>
#include <strings.h>
#define UA_snprintf snprintf
#define UA_strncasecmp strncasecmp

#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    char *errno_str = strerror(errno); \
    LOG; \
}
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP(LOG) { \
    const char *errno_str = gai_strerror(errno); \
    LOG; \
}

#if UA_MULTITHREADING >= 100

#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexAttr;
    int mutexCounter;
} UA_Lock;

static UA_INLINE void
UA_LOCK_INIT(UA_Lock *lock) {
    pthread_mutexattr_init(&lock->mutexAttr);
    pthread_mutexattr_settype(&lock->mutexAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lock->mutex, &lock->mutexAttr);
    lock->mutexCounter = 0;
}

static UA_INLINE void
UA_LOCK_DESTROY(UA_Lock *lock) {
    pthread_mutex_destroy(&lock->mutex);
    pthread_mutexattr_destroy(&lock->mutexAttr);
}

static UA_INLINE void
UA_LOCK(UA_Lock *lock) {
    pthread_mutex_lock(&lock->mutex);
    UA_assert(++(lock->mutexCounter) == 1);
}

static UA_INLINE void
UA_UNLOCK(UA_Lock *lock) {
    UA_assert(--(lock->mutexCounter) == 0);
    pthread_mutex_unlock(&lock->mutex);
}

static UA_INLINE void
UA_LOCK_ASSERT(UA_Lock *lock, int num) {
    UA_assert(lock->mutexCounter == num);
}
#else
#define UA_LOCK_INIT(lock)
#define UA_LOCK_DESTROY(lock)
#define UA_LOCK(lock)
#define UA_UNLOCK(lock)
#define UA_LOCK_ASSERT(lock, num)
#endif

#include <open62541/architecture_functions.h>

#endif /* PLUGINS_ARCH_NANO_UA_ARCHITECTURE_H_ */

#endif /* UA_ARCHITECTURE_NANO */
