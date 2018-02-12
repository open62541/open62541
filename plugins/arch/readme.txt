To port to a new architecture you should follow this file:

1- Create a folder with your architecture, let's call it new_arch
2- In the CMakeLists.txt file located next to this file, add 
  
  add_subdirectory(new_arch)
  
  at the end of it
  
3- Create a CMakeLists.txt file in the new_arch folder
4- Use the following template for it (remember that when you see new_arch you should replace with the name of your architecture)


# ---------------------------------------------------
# ---- Beginning of the CMakeLists.txt template -----
# ---------------------------------------------------

SET(SOURCE_GROUP ${SOURCE_GROUP}\\new_arch)

ua_add_architecture("new_arch")

if("${UA_ARCHITECTURE}" STREQUAL "new_arch")

    ua_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_clock.c)
    
    #
    # Add here below all the things that are specific for your architecture
    #
    
    #You can use the following available CMake functions:
     
    #ua_include_directories() include some directories specific to your architecture when compiling the open62541 stack
    #ua_architecture_remove_definitions() remove compiler flags from the general ../../CMakeLists.txt file that won't work with your architecture
    #ua_architecture_add_definitions() add compiler flags that your architecture needs
    #ua_architecture_append_to_library() add libraries to be linked to the open62541 that are needed by your architecture
    #ua_add_architecture_header() add header files to compilation (Don't add the file ua_architecture.h)
    #ua_add_architecture_file() add .c files to compilation    
    
endif()

# ---------------------------------------------------
# ---- End of the CMakeLists.txt template -----
# ---------------------------------------------------

5- Create a ua_clock.c file that implements the following functions defined in ua_types.h:

    UA_DateTime UA_DateTime_now(void);
    UA_Int64 UA_DateTime_localTimeUtcOffset(void);
    UA_DateTime UA_DateTime_nowMonotonic(void);

6- Create a file in the folder new_arch called ua_architecture.h 
7- Use the following template for it :
  a: Change YEAR, YOUR_NAME and YOUR_COMPANY in the header
  b: Change NEW_ARCH at the beginning in PLUGINS_ARCH_NEW_ARCH_UA_ARCHITECTURE_H_ for your own name in uppercase
   

/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright YEAR (c) YOUR_NAME, YOUR_COMPANY
 */

#ifndef PLUGINS_ARCH_NEW_ARCH_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_NEW_ARCH_UA_ARCHITECTURE_H_

/*
* Define and include all that's needed for your architecture
*/



/*
* Implement int UA_sleep_ms(unsigned int miliSeconds);
*/ 

/*
* Define OPTVAL_TYPE for non windows systems. In doubt, use int //TODO: Is this really necessary
*/


/*
* Define the following network options for ipv4 and ipv6 if they are enabled or not
*/


//#define UA_IPV6 1 //or 0
//#define UA_SOCKET
//#define UA_INVALID_SOCKET
//#define UA_ERRNO  
//#define UA_INTERRUPTED
//#define UA_AGAIN
//#define UA_EAGAIN
//#define UA_WOULDBLOCK
//#define UA_ERR_CONNECTION_PROGRESS
//#define UA_INTERRUPTED



/*
* Implement the following socket functions (you can #define them): 
*/ 

#include "ua_types.h"

/* ssize_t ua_send(UA_SOCKET sockfd, const void *buf, size_t len, int flags);
* int ua_select(UA_SOCKET nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
* ssize_t ua_recv(UA_SOCKET sockfd, void *buf, size_t len, int flags);
* int ua_shutdown(UA_SOCKET sockfd, int how);
* UA_SOCKET ua_socket(int domain, int type, int protocol);
* int ua_bind(UA_SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen);
* int ua_listen(UA_SOCKET sockfd, int backlog);
* int ua_accept(UA_SOCKET sockfd, struct sockaddr *addr, socklen_t *addrlen);
* int ua_close(UA_SOCKET sockfd);
* int ua_connect(UA_SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen);
* void UA_fd_set(UA_SOCKET fd, fd_set *set);
* int UA_fd_isset(UA_SOCKET fd, fd_set *set)
* const char* ua_translate_error(int errorCode)
* int ua_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
* UA_StatusCode socket_set_blocking(UA_SOCKET sockfd); 
* UA_StatusCode socket_set_nonblocking(UA_SOCKET sockfd); 
* int ua_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen); //Only in non windows architectures
* int ua_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
* void ua_freeaddrinfo(struct addrinfo *res);
*/

/*
* Define int ua_snprintf(char* pa_stream, size_t pa_size, const char* pa_format, ...);
*/


/*
* Define
* void ua_initialize_architecture_network(void);
* void ua_deinitialize_architecture_network(void);
*/



#endif /* PLUGINS_ARCH_NEW_ARCH_UA_ARCHITECTURE_H_ */

