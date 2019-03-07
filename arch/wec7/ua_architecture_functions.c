/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Stephan Kantelberg
 */

#ifdef UA_ARCHITECTURE_WEC7

#include <open62541/types.h>

#undef UA_fileExists
UA_Boolean UA_fileExists(const char* path) {
  FILE *fp = fopen(path,"rb");
  UA_Boolean exists = (fp==NULL);
  if(fp)
      fclose(fp);
  return exists;
}

unsigned int UA_socket_set_blocking(UA_SOCKET sockfd){
  u_long iMode = 0;
  if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;;
}

#ifdef UNDER_CE
char *strerror(int errnum)
{
    if (errnum > MAX_STRERROR)
        return errorStrings[MAX_STRERROR];
    else
        return errorStrings[errnum];
}
#endif

unsigned int UA_socket_set_nonblocking(UA_SOCKET sockfd){
  u_long iMode = 1;
  if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
    return UA_STATUSCODE_BADINTERNALERROR;
  return UA_STATUSCODE_GOOD;;
}

void UA_initialize_architecture_network(void){
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void UA_deinitialize_architecture_network(void){
  WSACleanup();
}

#endif /* UA_ARCHITECTURE_WEC7 */
