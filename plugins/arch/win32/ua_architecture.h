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

# include <errno.h>
# include <winsock2.h>
# include <ws2tcpip.h>
# define CLOSESOCKET(S) closesocket((SOCKET)S)
# define ssize_t int
# define WIN32_INT (int)
# define OPTVAL_TYPE char
# define ERR_CONNECTION_PROGRESS WSAEWOULDBLOCK
# define UA_sleep_ms(X) Sleep(X)

/* unsigned int for windows and workaround to a glibc bug */
/* Additionally if GNU_LIBRARY is not defined, it may be using
 * musl libc (e.g. Docker Alpine) */
#if defined(_WIN32) || defined(__OpenBSD__) || \
    (defined(__GNU_LIBRARY__) && (__GNU_LIBRARY__ <= 6) && \
     (__GLIBC__ <= 2) && (__GLIBC_MINOR__ < 16) || \
    !defined(__GNU_LIBRARY__))
# define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
# define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)
#else
# define UA_fd_set(fd, fds) FD_SET(fd, fds)
# define UA_fd_isset(fd, fds) FD_ISSET(fd, fds)
#endif

#ifdef UNDER_CE
# define errno WSAGetLastError()
#endif

# define errno__ WSAGetLastError()
# define INTERRUPTED WSAEINTR
# define WOULDBLOCK WSAEWOULDBLOCK
# define AGAIN WSAEWOULDBLOCK

#endif /* PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_ */
