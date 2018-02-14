/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

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

#define OPTVAL_TYPE int

#include <fcntl.h>
#include <unistd.h> // read, write, close
#include <netinet/tcp.h>

#define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)

#define UA_IPV6 1
#define UA_SOCKET int
#define UA_INVALID_SOCKET -1
#define UA_ERRNO errno
#define UA_INTERRUPTED EINTR
#define UA_AGAIN EAGAIN
#define UA_EAGAIN EAGAIN
#define UA_WOULDBLOCK EWOULDBLOCK
#define UA_ERR_CONNECTION_PROGRESS EINPROGRESS

#include "ua_types.h"

#define ua_getnameinfo getnameinfo
#define ua_send send
#define ua_recv recv
#define ua_close close
#define ua_select select
#define ua_shutdown shutdown
#define ua_socket socket
#define ua_bind bind
#define ua_listen listen
#define ua_accept accept
#define ua_connect connect
#define ua_translate_error gai_strerror
#define ua_getaddrinfo getaddrinfo
#define ua_getsockopt getsockopt
#define ua_setsockopt setsockopt
#define ua_freeaddrinfo freeaddrinfo

static UA_INLINE uint32_t socket_set_blocking(UA_SOCKET sockfd){
  int on = FALSE;
  if(ioctl(sockfd, FIONBIO, &on) < 0)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

static UA_INLINE uint32_t socket_set_nonblocking(UA_SOCKET sockfd){
  int on = TRUE;
  if(ioctl(sockfd, FIONBIO, &on) < 0)
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

#endif /* PLUGINS_ARCH_VXWORKS_UA_ARCHITECTURE_H_ */
