/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 */

#ifndef UA_LOG_SOCKET_ERROR_H_
#define UA_LOG_SOCKET_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _WIN32
#include <winsock2.h>
#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    char *errno_str = NULL; \
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
    NULL, WSAGetLastError(), \
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
    (LPSTR)&errno_str, 0, NULL); \
    LOG; \
    LocalFree(errno_str); \
}
#else
#define UA_LOG_SOCKET_ERRNO_WRAP(LOG) { \
    char *errno_str = strerror(errno); \
    LOG; \
}
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_LOG_SOCKET_ERROR_H_ */
