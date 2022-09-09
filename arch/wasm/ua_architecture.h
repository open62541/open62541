#ifndef PLUGINS_ARCH_WASM_UA_ARCHITECTURE_H_
#define PLUGINS_ARCH_WASM_UA_ARCHITECTURE_H_

/*
* Define and include all that's needed for your architecture
*/

#if UA_MULTITHREADING >= 100
#error Multithreading unsupported
#else
#define UA_LOCK_INIT(lock)
#define UA_LOCK_DESTROY(lock)
#define UA_LOCK(lock)
#define UA_UNLOCK(lock)
#define UA_LOCK_ASSERT(lock, num)
#endif


#ifndef UA_free
# include <stdlib.h>
# define UA_free free
# define UA_malloc malloc
# define UA_calloc calloc
# define UA_realloc realloc
#endif

#ifndef UA_snprintf
# include <stdio.h>
# define UA_snprintf snprintf
#endif

/*
* Define OPTVAL_TYPE for non windows systems. In doubt, use int //TODO: Is this really necessary
*/

/*
* Define the following network options
*/


#define UA_IPV6 0
#define UA_SOCKET int
#define UA_INVALID_SOCKET -1
//#define UA_ERRNO  
//#define UA_INTERRUPTED
//#define UA_AGAIN
//#define UA_EAGAIN
//#define UA_WOULDBLOCK
//#define UA_ERR_CONNECTION_PROGRESS
//#define UA_INTERRUPTED

/*
* Define the ua_getnameinfo if your architecture supports it
*/

/*
* Use #define for the functions defined in ua_architecture_functions.h
* or implement them in a ua_architecture_functions.c file and 
* put it in your new_arch folder and add it in the CMakeLists.txt file 
* using ua_add_architecture_file(${CMAKE_CURRENT_SOURCE_DIR}/ua_architecture_functions.c)
*/ 

void NotImplemented(void *, ...);

#define UA_sleep_ms NotImplemented
#define UA_send NotImplemented
#define UA_sendto NotImplemented
#define UA_select NotImplemented
#define UA_recv NotImplemented
#define UA_recvfrom NotImplemented
#define UA_recvmsg NotImplemented
#define UA_shutdown NotImplemented
#define UA_socket NotImplemented
#define UA_bind NotImplemented
#define UA_listen NotImplemented
#define UA_accept NotImplemented
#define UA_close NotImplemented
#define UA_connect NotImplemented
#define UA_fd_set NotImplemented
#define UA_fd_isset NotImplemented
#define UA_getaddrinfo NotImplemented
#define UA_htonl NotImplemented
#define UA_ntohl NotImplemented
#define UA_inet_pton NotImplemented
#define UA_socket_set_blocking NotImplemented
#define UA_socket_set_nonblocking NotImplemented
#define UA_ioctl NotImplemented
#define UA_getsockopt NotImplemented
#define UA_setsockopt NotImplemented
#define UA_freeaddrinfo NotImplemented
#define UA_gethostname(name, len) -1
#define UA_getsockname NotImplemented
#define UA_initialize_architecture_network NotImplemented
#define UA_deinitialize_architecture_network NotImplemented

/*
* Define UA_LOG_SOCKET_ERRNO_WRAP(LOG) which prints the string error given a char* errno_str variable
* Do the same for UA_LOG_SOCKET_ERRNO_GAI_WRAP(LOG) for errors related to getaddrinfo
*/

#include <open62541/architecture_functions.h>

#endif /* PLUGINS_ARCH_WASM_UA_ARCHITECTURE_H_ */
