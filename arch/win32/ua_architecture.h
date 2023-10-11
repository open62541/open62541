/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#ifdef UA_ARCHITECTURE_WIN32

#ifndef PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_

/* Disable some security warnings on MSVC */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
# define _CRT_SECURE_NO_WARNINGS
#endif

#include <open62541/config.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <basetsd.h>

#ifndef _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#endif

#define UA_IPV6 1
#define UA_SOCKET SOCKET
#define UA_INVALID_SOCKET INVALID_SOCKET
#define UA_ERRNO WSAGetLastError()
#define UA_INTERRUPTED WSAEINTR
#define UA_AGAIN EAGAIN /* the same as wouldblock on nearly every system */
#define UA_INPROGRESS WSAEINPROGRESS
#define UA_WOULDBLOCK WSAEWOULDBLOCK
#define UA_POLLIN POLLRDNORM
#define UA_POLLOUT POLLWRNORM
#define UA_SHUT_RDWR SD_BOTH

#define UA_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags) \
    getnameinfo(sa, (socklen_t)salen, host, (DWORD)hostlen, serv, (DWORD)servlen, flags)
#define UA_poll(fds,nfds,timeout) WSAPoll((LPWSAPOLLFD)fds, nfds, timeout)
#define UA_send(sockfd, buf, len, flags) send(sockfd, buf, (int)(len), flags)
#define UA_recv(sockfd, buf, len, flags) recv(sockfd, buf, (int)(len), flags)
#define UA_sendto(sockfd, buf, len, flags, dest_addr, addrlen) \
    sendto(sockfd, (const char*)(buf), (int)(len), flags, dest_addr, (int) (addrlen))
#define UA_close closesocket
#define UA_select(nfds, readfds, writefds, exceptfds, timeout) \
    select((int)(nfds), readfds, writefds, exceptfds, timeout)
#define UA_connect(sockfd, addr, addrlen) connect(sockfd, addr, (int)(addrlen))
#define UA_getsockopt(sockfd, level, optname, optval, optlen) \
    getsockopt(sockfd, level, optname, (char*) (optval), optlen)
#define UA_setsockopt(sockfd, level, optname, optval, optlen) \
    setsockopt(sockfd, level, optname, (const char*) (optval), optlen)
#define UA_inet_pton InetPton

#if UA_IPV6
# define UA_if_nametoindex if_nametoindex

# include <iphlpapi.h>

#endif

#ifdef maxStringLength //defined in mingw64
# undef maxStringLength
#endif

#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    char *errno_str = NULL; \
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
    NULL, WSAGetLastError(), \
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
    (LPSTR)&errno_str, 0, NULL); \
    LOG; \
    LocalFree(errno_str); \
}
#define UA_LOG_SOCKET_ERRNO_GAI_WRAP UA_LOG_SOCKET_ERRNO_WRAP

/* Fix redefinition of SLIST_ENTRY on mingw winnt.h */
#if !defined(_SYS_QUEUE_H_) && defined(SLIST_ENTRY)
# undef SLIST_ENTRY
#endif

#endif /* PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_ */

#endif /* UA_ARCHITECTURE_WIN32 */
