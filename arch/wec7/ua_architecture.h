/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Stephan Kantelberg
 */

#ifdef UA_ARCHITECTURE_WEC7

#ifndef PLUGINS_ARCH_WEC7_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_WEC7_UA_ARCHITECTURE_H_

#include <open62541/architecture_base.h>

#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

/* Disable some security warnings on MSVC */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
# define _CRT_SECURE_NO_WARNINGS
#endif

#include "stdint.h"
#ifdef UNDER_CE
#define MAX_STRERROR 31
static char *errorStrings[]= {"Error 0","","No such file or directory","","","","","Arg list too long",
                              "Exec format error","Bad file number","","","Not enough core","Permission denied","","",
                              "","File exists","Cross-device link","","","","Invalid argument","","Too many open files",
                              "","","","No space left on device","","","","","Math argument","Result too large","",
                              "Resource deadlock would occur", "Unknown error under wince"};

char *strerror(int errnum);
#endif

#include <stdlib.h>
#if defined(_WIN32)
# include <malloc.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#ifdef _MSC_VER
# ifndef UNDER_CE
#  include <io.h> //access
#  define UA_access _access
# endif
#else
# include <unistd.h> //access and tests
# define UA_access access
#endif

#define ssize_t int
#define OPTVAL_TYPE char
#ifndef UA_sleep_ms
# define UA_sleep_ms(X) Sleep(X)
#endif

// Windows does not support ansi colors
// #define UA_ENABLE_LOG_COLORS

#if defined(__MINGW32__) //mingw defines SOCKET as long long unsigned int, giving errors in logging and when comparing with UA_Int32
# define UA_SOCKET int
# define UA_INVALID_SOCKET -1
#else
# define UA_SOCKET SOCKET
# define UA_INVALID_SOCKET INVALID_SOCKET
#endif
#define UA_ERRNO WSAGetLastError()
#define UA_INTERRUPTED WSAEINTR
#define UA_AGAIN WSAEWOULDBLOCK
#define UA_EAGAIN EAGAIN
#define UA_WOULDBLOCK WSAEWOULDBLOCK
#define UA_ERR_CONNECTION_PROGRESS WSAEWOULDBLOCK

#define UA_fd_set(fd, fds) FD_SET((UA_SOCKET)fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET((UA_SOCKET)fd, fds)

#ifdef UNDER_CE
#define UA_ERRNO WSAGetLastError()
#endif

#define UA_getnameinfo getnameinfo
#define UA_send(sockfd, buf, len, flags) send(sockfd, buf, (int)(len), flags)
#define UA_recv recv
#define UA_sendto(sockfd, buf, len, flags, dest_addr, addrlen) sendto(sockfd, (const char*)(buf), (int)(len), flags, dest_addr, (int) (addrlen))
#define UA_recvfrom(sockfd, buf, len, flags, src_addr, addrlen) recvfrom(sockfd, (char*)(buf), (int)(len), flags, src_addr, addrlen)
#define UA_htonl htonl
#define UA_ntohl ntohl
#define UA_close closesocket
#define UA_select(nfds, readfds, writefds, exceptfds, timeout) select((int)(nfds), readfds, writefds, exceptfds, timeout)
#define UA_shutdown shutdown
#define UA_socket socket
#define UA_bind bind
#define UA_listen listen
#define UA_accept accept
#define UA_connect(sockfd, addr, addrlen) connect(sockfd, addr, (int)(addrlen))
#define UA_getaddrinfo getaddrinfo
#define UA_getsockopt getsockopt
#define UA_setsockopt(sockfd, level, optname, optval, optlen) setsockopt(sockfd, level, optname, (const char*) (optval), optlen)
#define UA_freeaddrinfo freeaddrinfo
#define UA_gethostname gethostname
#define UA_getsockname getsockname
#define UA_inet_pton InetPton

#ifdef maxStringLength //defined in mingw64
# undef maxStringLength
#endif

#ifndef UA_free
#define UA_free free
#endif
#ifndef UA_malloc
#define UA_malloc malloc
#endif
#ifndef UA_calloc
#define UA_calloc calloc
#endif
#ifndef UA_realloc
#define UA_realloc realloc
#endif

#define UA_snprintf(source, size, string, ...) _snprintf_s(source, size, _TRUNCATE, string, __VA_ARGS__)

#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    LPVOID errno_str = NULL; \
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
    NULL, WSAGetLastError(), \
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
    (LPWSTR)&errno_str, \
    0, NULL); \
    LOG; \
    LocalFree(errno_str); \
}
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP UA_LOG_SOCKET_ERRNO_WRAP

#include <open62541/architecture_functions.h>

/* Fix redefinition of SLIST_ENTRY on mingw winnt.h */
#if !defined(_SYS_QUEUE_H_) && defined(SLIST_ENTRY)
# undef SLIST_ENTRY
#endif

#endif /* PLUGINS_ARCH_WEC7_UA_ARCHITECTURE_H_ */

#endif /* UA_ARCHITECTURE_WEC7 */
