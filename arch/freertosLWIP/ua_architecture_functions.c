/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 *    Copyright 2018 (c) Stefan Profanter, fortiss GmbH
 */

#ifdef UA_ARCHITECTURE_FREERTOSLWIP

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

int gethostname_lwip(char* name, size_t len){
  // use UA_ServerConfig_set_customHostname to set your hostname as the IP
  return -1;
}

void UA_initialize_architecture_network(void){
  return;
}

void UA_deinitialize_architecture_network(void){
  return;
}

#if UA_IPV6
# if LWIP_VERSION_IS_RELEASE //lwip_if_nametoindex is not yet released
unsigned int lwip_if_nametoindex(const char *ifname){
  return 1; //TODO: assume for now that it only has one interface
}
# endif //LWIP_VERSION_IS_RELEASE
#endif //UA_IPV6

#endif /* UA_ARCHITECTURE_FREERTOSLWIP */
