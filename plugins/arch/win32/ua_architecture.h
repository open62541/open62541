/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_

#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

/* Disable some security warnings on MSVC */
#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif

/* Assume that Windows versions are newer than Windows XP */
#if defined(__MINGW32__) && (!defined(WINVER) || WINVER < 0x501)
# undef WINVER
# undef _WIN32_WINDOWS
# undef _WIN32_WINNT
# define WINVER 0x0501
# define _WIN32_WINDOWS 0x0501
# define _WIN32_WINNT 0x0501
#endif

#include <errno.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define ssize_t int
#define OPTVAL_TYPE char
#define UA_sleep_ms(X) Sleep(X)

#define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)

#ifdef UNDER_CE
# define errno
#endif

#define UA_IPV6 1
#define UA_SOCKET int
#define UA_INVALID_SOCKET -1
#define UA_ERRNO WSAGetLastError()
#define UA_INTERRUPTED WSAEINTR
#define UA_AGAIN WSAEWOULDBLOCK
#define UA_EAGAIN EAGAIN
#define UA_WOULDBLOCK WSAEWOULDBLOCK
#define UA_ERR_CONNECTION_PROGRESS WSAEWOULDBLOCK

#include "ua_types.h"

#define ua_getnameinfo getnameinfo
#define ua_send send
#define ua_recv recv
#define ua_close closesocket
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


static UA_INLINE UA_StatusCode socket_set_blocking(UA_SOCKET sockfd){
  u_long iMode = 0;
  if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;;
}

static UA_INLINE UA_StatusCode socket_set_nonblocking(UA_SOCKET sockfd){
  u_long iMode = 1;
  if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;;
}


#include <stdio.h>
#define ua_snprintf(source, size, string, ...) _snprintf_s(source, size, _TRUNCATE, string, __VA_ARGS__)

static UA_INLINE void ua_initialize_architecture_network(void){
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  WSAStartup(wVersionRequested, &wsaData);
}

static UA_INLINE void ua_deinitialize_architecture_network(void){
  WSACleanup();
}

#endif /* PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_ */
