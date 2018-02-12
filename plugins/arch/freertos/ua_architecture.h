/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef PLUGINS_ARCH_FREERTOS_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_FREERTOS_UA_ARCHITECTURE_H_


//------------------------------------------------------------------
// NOT WORKING YET!!!!!!!!!!!!!!!!!!!!!
//------------------------------------------------------------------

#define AI_PASSIVE 0x01

#define UA_FREERTOS_HOSTNAME "10.200.4.114"

#include <stdlib.h>
#include <string.h>

#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_COMPAT_MUTEX 0
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
#ifdef LWIP_COMPAT_SOCKETS
#undef LWIP_COMPAT_SOCKETS
#endif
#define LWIP_COMPAT_SOCKETS 0
//#define __USE_W32_SOCKETS 1 //needed to avoid redefining of select in sys/select.h

#include <lwip/tcpip.h>
#include <lwip/netdb.h>
#define sockaddr_storage sockaddr
#ifdef BYTE_ORDER
# undef BYTE_ORDER
#endif
#define UA_sleep_ms(X) vTaskDelay(pdMS_TO_TICKS(X))

#define OPTVAL_TYPE int

#include <unistd.h> // read, write, close

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

#include "ua_types.h"

static UA_INLINE int gethostname_freertos(char* name, size_t len){
  if(strlen(UA_FREERTOS_HOSTNAME) > (len))
    return -1;
  strcpy(name, UA_FREERTOS_HOSTNAME);
  return 0;
}
#define gethostname gethostname_freertos

#define ua_send lwip_send
#define ua_recv lwip_recv
#define ua_close lwip_close
#define ua_select lwip_select
#define ua_shutdown lwip_shutdown
#define ua_socket lwip_socket
#define ua_bind lwip_bind
#define ua_listen lwip_listen
#define ua_accept lwip_accept
#define ua_connect lwip_connect
#define ua_getsockopt lwip_getsockopt
#define ua_setsockopt lwip_setsockopt
#define ua_translate_error(x) ""
#define ua_freeaddrinfo lwip_freeaddrinfo

static UA_INLINE int ua_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res){
  if(NULL == node){
    const char* hostname = UA_FREERTOS_HOSTNAME;
    return lwip_getaddrinfo(hostname, service, hints, res);
  }else{
    return lwip_getaddrinfo(node, service, hints, res);
  }
}

static UA_INLINE uint32_t socket_set_blocking(UA_SOCKET sockfd){
  int on = 0;
  if(lwip_ioctl(sockfd, FIONBIO, &on) < 0)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

static UA_INLINE uint32_t socket_set_nonblocking(UA_SOCKET sockfd){
  int on = 1;
  if(lwip_ioctl(sockfd, FIONBIO, &on) < 0)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

#include <stdio.h>
#define ua_snprintf snprintf

static UA_INLINE void ua_initialize_architecture_network(void){
  return;
}

static UA_INLINE void ua_deinitialize_architecture_network(void){
  return;
}


#endif /* PLUGINS_ARCH_FREERTOS_UA_ARCHITECTURE_H_ */
