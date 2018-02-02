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


/*
 * Set LWIP_COMPAT_SOCKETS to 2 in lwipoptions.h
 */


#define AI_PASSIVE 0x01
#define TRUE 1
#define FALSE 0
#define ioctl ioctlsocket

#define UA_FREERTOS_HOSTNAME "10.200.4.114"

#include <stdlib.h>
#include <string.h>

static inline int gethostname_freertos(char* name, size_t len){
  if(strlen(UA_FREERTOS_HOSTNAME) > (len))
    return -1;
  strcpy(name, UA_FREERTOS_HOSTNAME);
  return 0;
}
#define gethostname gethostname_freertos

#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_COMPAT_MUTEX 0
#define LWIP_POSIX_SOCKETS_IO_NAMES 0
//#define __USE_W32_SOCKETS 1 //needed to avoid redefining of select in sys/select.h

#include <lwip/tcpip.h>
#include <lwip/netdb.h>
#define CLOSESOCKET(S) lwip_close(S)
#define sockaddr_storage sockaddr
#ifdef BYTE_ORDER
# undef BYTE_ORDER
#endif
#define UA_sleep_ms(X) vTaskDelay(pdMS_TO_TICKS(X))

#define SOCKET int
#define WIN32_INT
#define OPTVAL_TYPE int
#define ERR_CONNECTION_PROGRESS EINPROGRESS


//# include <fcntl.h>
# include <unistd.h> // read, write, close

# define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
# define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)

#define errno__ errno
#define INTERRUPTED EINTR
#define WOULDBLOCK EWOULDBLOCK
#define AGAIN EAGAIN

#endif /* PLUGINS_ARCH_FREERTOS_UA_ARCHITECTURE_H_ */
