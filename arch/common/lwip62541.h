/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifndef ARCH_COMMON_LWIP62541_H_
#define ARCH_COMMON_LWIP62541_H_

#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#ifdef LWIP_COMPAT_SOCKETS
#undef LWIP_COMPAT_SOCKETS
#endif
#define LWIP_COMPAT_SOCKETS 0

#include <lwip/tcpip.h>
#include <lwip/netdb.h>
#define sockaddr_storage sockaddr

#define OPTVAL_TYPE int

#define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)

#define UA_IPV6 0
#define UA_SOCKET int
#define UA_INVALID_SOCKET -1
#define UA_ERRNO errno
#define UA_INTERRUPTED EINTR
#define UA_AGAIN EAGAIN
#define UA_EAGAIN EAGAIN
#define UA_WOULDBLOCK EWOULDBLOCK
#define UA_ERR_CONNECTION_PROGRESS EINPROGRESS

#define UA_send lwip_send
#define UA_recv lwip_recv
#define UA_close lwip_close
#define UA_select lwip_select
#define UA_shutdown lwip_shutdown
#define UA_socket lwip_socket
#define UA_bind lwip_bind
#define UA_listen lwip_listen
#define UA_accept lwip_accept
#define UA_connect lwip_connect
#define UA_getsockopt lwip_getsockopt
#define UA_setsockopt lwip_setsockopt
#define UA_freeaddrinfo lwip_freeaddrinfo
#define UA_gethostname gethostname_lwip
#define UA_getaddrinfo lwip_getaddrinfo

int gethostname_lwip(char* name, size_t len);

#endif /* ARCH_COMMON_LWIP62541_H_ */
