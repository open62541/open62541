/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_ECOS

#include <open62541/types.h>

unsigned int UA_socket_set_blocking(UA_SOCKET sockfd){
  int on = 0;
  if(ioctl(sockfd, FIONBIO, &on) < 0)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

unsigned int UA_socket_set_nonblocking(UA_SOCKET sockfd){
  int on = 1;
  if(ioctl(sockfd, FIONBIO, &on) < 0)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

void UA_initialize_architecture_network(void){
  return;
}

void UA_deinitialize_architecture_network(void){
  return;
}

int gethostname_ecos(char* name, size_t len){
  if(strlen(UA_ECOS_HOSTNAME) > (len))
    return -1;
  strcpy(name, UA_ECOS_HOSTNAME);
  return 0;
}

#endif /* UA_ARCHITECTURE_ECOS */
