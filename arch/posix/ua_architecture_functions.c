/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_POSIX

#include <open62541/types.h>

/* Global malloc singletons */
#ifdef UA_ENABLE_MALLOC_SINGLETON
void * (*UA_globalMalloc)(size_t size) = malloc;
void (*UA_globalFree)(void *ptr) = free;
void * (*UA_globalCalloc)(size_t nelem, size_t elsize) = calloc;
void * (*UA_globalRealloc)(void *ptr, size_t size) = realloc;
#endif

unsigned int UA_socket_set_blocking(UA_SOCKET sockfd){
  int opts = fcntl(sockfd, F_GETFL);
  if(opts < 0 || fcntl(sockfd, F_SETFL, opts & (~O_NONBLOCK)) < 0)
      return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

unsigned int UA_socket_set_nonblocking(UA_SOCKET sockfd){
  int opts = fcntl(sockfd, F_GETFL);
  if(opts < 0 || fcntl(sockfd, F_SETFL, opts | O_NONBLOCK) < 0)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

void UA_initialize_architecture_network(void){
}

void UA_deinitialize_architecture_network(void){
}

#endif /* UA_ARCHITECTURE_POSIX */
