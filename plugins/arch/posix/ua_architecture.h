/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2016-2017 (c) Julius Pfrommer, Fraunhofer IOSB
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_

/* Enable POSIX features */
#if !defined(_XOPEN_SOURCE) && !defined(_WRS_KERNEL)
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

#if !defined(UA_FREERTOS)
# include <errno.h>
#else
# define AI_PASSIVE 0x01
# define TRUE 1
# define FALSE 0
# define ioctl ioctlsocket
#endif

# if defined(UA_FREERTOS)
#  define UA_FREERTOS_HOSTNAME "10.200.4.114"
static inline int gethostname_freertos(char* name, size_t len){
  if(strlen(UA_FREERTOS_HOSTNAME) > (len))
    return -1;
  strcpy(name, UA_FREERTOS_HOSTNAME);
  return 0;
}
#define gethostname gethostname_freertos
#  include <lwip/tcpip.h>
#  include <lwip/netdb.h>
#  define CLOSESOCKET(S) lwip_close(S)
#  define sockaddr_storage sockaddr
#  ifdef BYTE_ORDER
#   undef BYTE_ORDER
#  endif
#  define UA_sleep_ms(X) vTaskDelay(pdMS_TO_TICKS(X))
# else /* Not freeRTOS */
#  define CLOSESOCKET(S) close(S)
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <sys/ioctl.h>
#  if defined(_WRS_KERNEL)
#   include <hostLib.h>
#   include <selectLib.h>
#   define UA_sleep_ms(X)                            \
    {                                                \
    struct timespec timeToSleep;                     \
      timeToSleep.tv_sec = X / 1000;                 \
      timeToSleep.tv_nsec = 1000000 * (X % 1000);    \
      nanosleep(&timeToSleep, NULL);                 \
    }
#  else /* defined(_WRS_KERNEL) */
#   include <sys/select.h>
#   define UA_sleep_ms(X) usleep(X * 1000)
#  endif /* defined(_WRS_KERNEL) */
# endif /* Not freeRTOS */

# define SOCKET int
# define WIN32_INT
# define OPTVAL_TYPE int
# define ERR_CONNECTION_PROGRESS EINPROGRESS


# include <fcntl.h>
# include <unistd.h> // read, write, close

# ifdef __QNX__
#  include <sys/socket.h>
# endif
# if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#  include <sys/param.h>
#  if defined(BSD)
#   include<sys/socket.h>
#  endif
# endif
# if !defined(__CYGWIN__) && !defined(UA_FREERTOS)
#  include <netinet/tcp.h>
# endif

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

# define errno__ errno
# define INTERRUPTED EINTR
# define WOULDBLOCK EWOULDBLOCK
# define AGAIN EAGAIN

#endif /* PLUGINS_ARCH_POSIX_UA_ARCHITECTURE_H_ */
