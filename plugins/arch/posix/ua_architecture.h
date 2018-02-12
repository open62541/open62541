/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_

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
#define UA_sleep_ms(X) usleep(X * 1000)

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
  int opts = fcntl(sockfd, F_GETFL);
  if(opts < 0 || fcntl(sockfd, F_SETFL, opts & (~O_NONBLOCK)) < 0)
      return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

static UA_INLINE uint32_t socket_set_nonblocking(UA_SOCKET sockfd){
  int opts = fcntl(sockfd, F_GETFL);
  if(opts < 0 || fcntl(sockfd, F_SETFL, opts | O_NONBLOCK) < 0)
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

#endif /* PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_ */
