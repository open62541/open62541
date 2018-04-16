/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss IOSB
 */

#include "ua_types.h"

unsigned int UA_socket_set_blocking(UA_SOCKET sockfd){
  int on = 0;
  if(lwip_ioctl(sockfd, FIONBIO, &on) < 0)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

unsigned int UA_socket_set_nonblocking(UA_SOCKET sockfd){
  int on = 1;
  if(lwip_ioctl(sockfd, FIONBIO, &on) < 0)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;
}

int gethostname_freertos(char* name, size_t len){
  if(strlen(UA_FREERTOS_HOSTNAME) > (len))
    return -1;
  strcpy(name, UA_FREERTOS_HOSTNAME);
  return 0;
}

int UA_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res){
  if(NULL == node){
    const char* hostname = UA_FREERTOS_HOSTNAME;
    return lwip_getaddrinfo(hostname, service, hints, res);
  }else{
    return lwip_getaddrinfo(node, service, hints, res);
  }
}

void UA_initialize_architecture_network(void){
  return;
}

void UA_deinitialize_architecture_network(void){
  return;
}
