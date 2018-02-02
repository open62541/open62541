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

#define CLOSESOCKET(S) close(S)
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

#define SOCKET int
#define WIN32_INT
#define OPTVAL_TYPE int
#define ERR_CONNECTION_PROGRESS EINPROGRESS

#include <fcntl.h>
#include <unistd.h> // read, write, close
#include <netinet/tcp.h>

#define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
#define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)

#define errno__ errno
#define INTERRUPTED EINTR
#define WOULDBLOCK EWOULDBLOCK
#define AGAIN EAGAIN

#endif /* PLUGINS_ARCH_VXWORKS_UA_ARCHITECTURE_H_ */
