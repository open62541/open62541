/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017-2018 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_FREERTOSLWIP

#ifndef PLUGINS_ARCH_FREERTOSLWIP_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_FREERTOSLWIP_UA_ARCHITECTURE_H_

#include <stdlib.h>
#include <string.h>

#ifdef BYTE_ORDER
# undef BYTE_ORDER
#endif

#define UA_sleep_ms(X) vTaskDelay(pdMS_TO_TICKS(X))

#ifdef OPEN62541_FEERTOS_USE_OWN_MEM
# define UA_free vPortFree
# define UA_malloc pvPortMalloc
# define UA_calloc pvPortCalloc
# define UA_realloc pvPortRealloc
#else
# define UA_free free
# define UA_malloc malloc
# define UA_calloc calloc
# define UA_realloc realloc
#endif

#ifdef UA_ENABLE_DISCOVERY_SEMAPHORE
# ifndef UA_fileExists
#  define UA_fileExists(X) (0) //file managing is not part of freeRTOS. If the system provides it, please define it before
# endif // UA_fileExists
#endif

// No log colors on freeRTOS
// #define UA_ENABLE_LOG_COLORS

#include <stdio.h>
#define UA_snprintf snprintf

#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    char *errno_str = ""; \
    LOG; \
}

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

#include <open62541/arch_common.h>

#endif /* PLUGINS_ARCH_FREERTOSLWIP_UA_ARCHITECTURE_H_ */

#endif /* UA_ARCHITECTURE_FREERTOSLWIP */
