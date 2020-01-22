/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifndef ARCH_COMMON_LWIP62541_H_
#define ARCH_COMMON_LWIP62541_H_

/*
 * Needed flags to be set before including this file. Normally done in lwipopts.h
 * #define LWIP_COMPAT_SOCKETS 0 // Don't do name define-transformation in networking function names.
 * #define LWIP_SOCKET 1 // Enable Socket API (normally already set)
 * #define LWIP_DNS 1 // enable the lwip_getaddrinfo function, struct addrinfo and more.
 * #define SO_REUSE 1 // Allows to set the socket as reusable
 * #define LWIP_TIMEVAL_PRIVATE 0 // This is optional. Set this flag if you get a compilation error about redefinition of struct timeval
 *
 * Why not define these here? This stack is used as middleware so other code might use this header file with other flags (specially LWIP_COMPAT_SOCKETS)
 */
#include <lwip/tcpip.h>
#include <lwip/netdb.h>
#include <lwip/init.h>
#include <lwip/sockets.h>

#define OPTVAL_TYPE int

#define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)

#define UA_IPV6 LWIP_IPV6
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
#define UA_sendto lwip_sendto
#define UA_recvfrom lwip_recvfrom
#define UA_htonl lwip_htonl
#define UA_ntohl lwip_ntohl
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
#define UA_getsockname lwip_getsockname
#define UA_getaddrinfo lwip_getaddrinfo

#if UA_IPV6
# define UA_inet_pton(af, src, dst) \
    (((af) == AF_INET6) ? ip6addr_aton((src),(ip6_addr_t*)(dst)) \
     : (((af) == AF_INET) ? ip4addr_aton((src),(ip4_addr_t*)(dst)) : 0))
#else
# define UA_inet_pton(af, src, dst) \
     (((af) == AF_INET) ? ip4addr_aton((src),(ip4_addr_t*)(dst)) : 0)
#endif

#if UA_IPV6
# define UA_if_nametoindex lwip_if_nametoindex

# if LWIP_VERSION_IS_RELEASE //lwip_if_nametoindex is not yet released
unsigned int lwip_if_nametoindex(const char *ifname);
# endif
#endif

int gethostname_lwip(char* name, size_t len); //gethostname is not present in LwIP. We implement here a dummy. See ../freertosLWIP/ua_architecture_functions.c

#define UA_LOG_SOCKET_ERRNO_GAI_WRAP UA_LOG_SOCKET_ERRNO_WRAP

#endif /* ARCH_COMMON_LWIP62541_H_ */
