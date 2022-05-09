/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2021 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Kalycito Infotech Private Limited
 */

#ifdef UA_ARCHITECTURE_WIN32

#ifndef PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_

/* Disable some security warnings on MSVC */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
# define _CRT_SECURE_NO_WARNINGS
#endif

/* Assume that Windows versions are newer than Windows XP */
#if defined(__MINGW32__) && (!defined(WINVER) || WINVER < 0x501)
# undef WINVER
# undef _WIN32_WINDOWS
# undef _WIN32_WINNT
# define WINVER 0x0600
# define _WIN32_WINDOWS 0x0600
# define _WIN32_WINNT 0x0600 //windows vista version, which included InepPton
#endif

#include <stdlib.h>
#if defined(_WIN32) && !defined(__clang__)
# include <malloc.h>
#endif

#include <open62541/architecture_definitions.h>

#include <stdio.h>
#include <errno.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#if defined (_MSC_VER) || defined(__clang__)
# ifndef UNDER_CE
#  include <io.h> //access
#  define UA_access _access
# endif
#else
# include <unistd.h> //access and tests
# define UA_access access
#endif

#ifndef _SSIZE_T_DEFINED
# define ssize_t int
#endif

#define OPTVAL_TYPE int
#ifdef UA_sleep_ms
void UA_sleep_ms(unsigned long ms);
#else
# define UA_sleep_ms(X) Sleep(X)
#endif

// Windows does not support ansi colors
// #define UA_ENABLE_LOG_COLORS

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

#define UA_fd_set(fd, fds) FD_SET((UA_SOCKET)fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET((UA_SOCKET)fd, fds)

#ifdef UNDER_CE
# define errno
#endif

#define UA_getnameinfo(sa, salen, host, hostlen, serv, servlen, flags) \
    getnameinfo(sa, (socklen_t)salen, host, (DWORD)hostlen, serv, (DWORD)servlen, flags)
#define UA_poll(fds,nfds,timeout) WSAPoll((LPWSAPOLLFD)fds, nfds, timeout)
#define UA_send(sockfd, buf, len, flags) send(sockfd, buf, (int)(len), flags)
#define UA_recv(sockfd, buf, len, flags) recv(sockfd, buf, (int)(len), flags)
#define UA_sendto(sockfd, buf, len, flags, dest_addr, addrlen) sendto(sockfd, (const char*)(buf), (int)(len), flags, dest_addr, (int) (addrlen))
#define UA_recvfrom(sockfd, buf, len, flags, src_addr, addrlen) recvfrom(sockfd, (char*)(buf), (int)(len), flags, src_addr, addrlen)
#define UA_recvmsg
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
#define UA_getsockopt(sockfd, level, optname, optval, optlen) getsockopt(sockfd, level, optname, (char*) (optval), optlen)
#define UA_setsockopt(sockfd, level, optname, optval, optlen) setsockopt(sockfd, level, optname, (const char*) (optval), optlen)
#define UA_ioctl
#define UA_freeaddrinfo freeaddrinfo
#define UA_gethostname gethostname
#define UA_getsockname getsockname
#define UA_inet_pton InetPton

#ifdef UA_ENABLE_FILETYPE_OBJECT_SUPPORT
#define UA_file_seek_end SEEK_END
#define UA_file_seek_current SEEK_CUR
#define UA_file_seek_set SEEK_SET
#define UA_file FILE
#define UA_file_open(filePtr, mode) fopen(filePtr, mode)
#define UA_file_close(filePtr) fclose(filePtr)
#define UA_file_seek(filePtr, offset, seekPosition) fseek(filePtr, offset, seekPosition)
#define UA_file_tell(filePtr) ftell(filePtr)
#define UA_file_no(fileptr) _fileno(fileptr)
#define UA_file_print fprintf
#define UA_file_read(readbuffer, elementSize, count, filePtr) fread(readbuffer, elementSize, count, filePtr)
#define UA_file_error(filePtr) ferror(filePtr)
#define UA_file_eof(filePtr) feof(filePtr)
#endif

#if UA_IPV6
# if defined(__WINCRYPT_H__) && defined(UA_ENABLE_ENCRYPTION_LIBRESSL)
#  error "Wincrypt is not compatible with LibreSSL"
# endif
# ifdef UA_ENABLE_ENCRYPTION_LIBRESSL
/* Hack: Prevent Wincrypt-Includes */
#  define __WINCRYPT_H__
# endif

# include <iphlpapi.h>

# ifdef UA_ENABLE_ENCRYPTION_LIBRESSL
#  undef __WINCRYPT_H__
# endif

# define UA_if_nametoindex if_nametoindex
#endif

#ifdef maxStringLength //defined in mingw64
# undef maxStringLength
#endif

/* Use the standard malloc */
#ifndef UA_free
# define UA_free free
# define UA_malloc malloc
# define UA_calloc calloc
# define UA_realloc realloc
#endif

#ifdef __CODEGEARC__
#define _snprintf_s(a,b,c,...) snprintf(a,b,__VA_ARGS__)
#endif

/* 3rd Argument is the string */
#define UA_snprintf(source, size, ...) _snprintf_s(source, size, _TRUNCATE, __VA_ARGS__)
#define UA_strncasecmp _strnicmp

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

#if UA_MULTITHREADING >= 100

typedef struct {
    CRITICAL_SECTION mutex;
    int mutexCounter;
} UA_Lock;

static UA_INLINE void
UA_LOCK_INIT(UA_Lock *lock) {
    InitializeCriticalSection(&lock->mutex);
    lock->mutexCounter = 0;
}

static UA_INLINE void
UA_LOCK_DESTROY(UA_Lock *lock) {
    DeleteCriticalSection(&lock->mutex);
}

static UA_INLINE void
UA_LOCK(UA_Lock *lock) {
    EnterCriticalSection(&lock->mutex);
    UA_assert(++(lock->mutexCounter) == 1);
}

static UA_INLINE void
UA_UNLOCK(UA_Lock *lock) {
    UA_assert(--(lock->mutexCounter) == 0);
    LeaveCriticalSection(&lock->mutex);
}

static UA_INLINE void
UA_LOCK_ASSERT(UA_Lock *lock, int num) {
    UA_assert(lock->mutexCounter == num);
}
#else
#define UA_LOCK_INIT(lock)
#define UA_LOCK_DESTROY(lock)
#define UA_LOCK(lock)
#define UA_UNLOCK(lock)
#define UA_LOCK_ASSERT(lock, num)
#endif

#include <open62541/architecture_functions.h>

/* Fix redefinition of SLIST_ENTRY on mingw winnt.h */
#if !defined(_SYS_QUEUE_H_) && defined(SLIST_ENTRY)
# undef SLIST_ENTRY
#endif

#endif /* PLUGINS_ARCH_WIN32_UA_ARCHITECTURE_H_ */

#endif /* UA_ARCHITECTURE_WIN32 */
