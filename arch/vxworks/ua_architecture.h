/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_VXWORKS

#ifndef PLUGINS_ARCH_VXWORKS_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_VXWORKS_UA_ARCHITECTURE_H_

#include <../deps/queue.h>  //in some compilers there's already a _SYS_QUEUE_H_ who is included first and doesn't have all functions

#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>

#include <hostLib.h>
#include <selectLib.h>

#define UA_sleep_ms(X)                            \
 {                                                \
 struct timespec timeToSleep;                     \
   timeToSleep.tv_sec = X / 1000;                 \
   timeToSleep.tv_nsec = 1000000 * (X % 1000);    \
   nanosleep(&timeToSleep, NULL);                 \
 }

#ifdef UINT32_C
# undef UINT32_C
#endif

#define UINT32_C(x) ((x) + (UINT32_MAX - UINT32_MAX))

#ifdef UA_BINARY_OVERLAYABLE_FLOAT
# undef UA_BINARY_OVERLAYABLE_FLOAT
#endif
#define UA_BINARY_OVERLAYABLE_FLOAT 1

#define OPTVAL_TYPE int

#include <fcntl.h>
#include <unistd.h> // read, write, close
#include <netinet/tcp.h>

#define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)

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
#define UA_inet_pton inet_pton
#if UA_IPV6
# define UA_if_nametoindex if_nametoindex
#endif

#include <stdlib.h>
#define UA_free free
#define UA_malloc malloc
#define UA_calloc calloc
#define UA_realloc realloc

#include <stdio.h>
#define UA_snprintf snprintf

#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    char *errno_str = strerror(errno); \
    LOG; \
}
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP(LOG) { \
    char *errno_str = gai_strerror(errno); \
    LOG; \
}

#include "../ua_architecture_functions.h"

#endif /* PLUGINS_ARCH_VXWORKS_UA_ARCHITECTURE_H_ */

#endif /* UA_ARCHITECTURE_VXWORKS */
