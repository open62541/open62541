/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017-2018 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_FREERTOSLWIP

#ifndef PLUGINS_ARCH_FREERTOS_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_FREERTOS_UA_ARCHITECTURE_H_

#define AI_PASSIVE 0x01

#define UA_THREAD_LOCAL

#include <stdlib.h>
#include <string.h>

#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#ifdef LWIP_COMPAT_SOCKETS
#undef LWIP_COMPAT_SOCKETS
#endif
#define LWIP_COMPAT_SOCKETS 0

#include <lwip/tcpip.h>
#include <lwip/netdb.h>
#define sockaddr_storage sockaddr
#ifdef BYTE_ORDER
# undef BYTE_ORDER
#endif
#define UA_sleep_ms(X) vTaskDelay(pdMS_TO_TICKS(X))

#define OPTVAL_TYPE int

#include <unistd.h> // read, write, close

#define UA_fd_set(fd, fds) FD_SET(fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET(fd, fds)

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

// No log colors on FreeRTOS
// #define UA_ENABLE_LOG_COLORS

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
#define UA_gethostname gethostname_freertos

#define UA_free vPortFree
#define UA_malloc pvPortMalloc
#define UA_calloc pvPortCalloc
#define UA_realloc pvPortRealloc

#include <stdio.h>
#define UA_snprintf snprintf

int gethostname_freertos(char* name, size_t len);

#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    char *errno_str = ""; \
    LOG; \
}

#define UA_LOG_SOCKET_ERRNO_GAI_WRAP UA_LOG_SOCKET_ERRNO_WRAP

#include "../ua_architecture_functions.h"

#endif /* PLUGINS_ARCH_FREERTOS_UA_ARCHITECTURE_H_ */

#endif /* UA_ARCHITECTURE_FREERTOSLWIP */
