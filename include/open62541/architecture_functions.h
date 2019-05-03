/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Jose Cabral, fortiss GmbH
 */

/*
 * This header has all the functions that are architecture dependent. The declaration is behind a ifndef since
 * they can be previously defined in the ua_architecture.h which include this files at the end
 */

#ifndef PLUGINS_ARCH_UA_ARCHITECTURE_FUNCTIONS_H_
#define PLUGINS_ARCH_UA_ARCHITECTURE_FUNCTIONS_H_

#include <open62541/config.h>

_UA_BEGIN_DECLS

/*
 * Allocation functions
 */

#ifndef UA_malloc
void* UA_malloc(size_t size); //allocate memory in the heap with size bytes
#endif

#ifndef UA_calloc
void* UA_calloc(size_t num, size_t size); //allocate memory in the heap with size*num bytes and set the memory to zero
#endif

#ifndef UA_realloc
void* UA_realloc(void *ptr, size_t new_size);//re-allocate memory in the heap with new_size bytes from previously allocated memory ptr
#endif

#ifndef UA_free
void UA_free(void* ptr); //de-allocate memory previously allocated with UA_malloc, UA_calloc or UA_realloc
#endif

#ifndef UA_alloca
# if defined(__GNUC__) || defined(__clang__)
#  define UA_alloca(size) __builtin_alloca (size)
# elif defined(_WIN32)
#  define UA_alloca(SIZE) _alloca(SIZE)
# else
#  include <alloca.h>
#  define UA_alloca(SIZE) alloca(SIZE)
# endif
#endif

#ifndef UA_STACKARRAY
/* Stack-allocation of memory. Use C99 variable-length arrays if possible.
 * Otherwise revert to alloca. Note that alloca is not supported on some
 * plattforms. */
# if defined(__GNUC__) || defined(__clang__)
#  define UA_STACKARRAY(TYPE, NAME, SIZE) TYPE NAME[SIZE]
# else
#  define UA_STACKARRAY(TYPE, NAME, SIZE) \
    TYPE *NAME = (TYPE*)UA_alloca(sizeof(TYPE) * SIZE)
# endif
#endif

/*
 * Sleep function
 */

#ifndef UA_sleep_ms
int UA_sleep_ms(unsigned int miliSeconds); //suspend the thread for a certain amount of mili seconds
#endif

/*
 * Socket functions
 */

#ifndef UA_send
ssize_t UA_send(UA_SOCKET sockfd, const void *buf, size_t len, int flags); //equivalent to posix send implementation
#endif

#ifndef UA_sendto
ssize_t sendto(UA_SOCKET sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen); //equivalent to posix sendto implementation
#endif

#ifndef UA_select
int UA_select(UA_SOCKET nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout); //equivalent to posix select implementation
#endif

#ifndef UA_recv
ssize_t UA_recv(UA_SOCKET sockfd, void *buf, size_t len, int flags); //equivalent to posix recv implementation
#endif

#ifndef UA_recvfrom
ssize_t recvfrom(UA_SOCKET sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
#endif

#ifndef UA_shutdown
int UA_shutdown(UA_SOCKET sockfd, int how); //equivalent to posix shutdown implementation
#endif

#ifndef UA_socket
UA_SOCKET UA_socket(int domain, int type, int protocol);//equivalent to posix socket implementation
#endif

#ifndef UA_bind
int UA_bind(UA_SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen);//equivalent to posix bind implementation
#endif

#ifndef UA_listen
int UA_listen(UA_SOCKET sockfd, int backlog);//equivalent to posix listen implementation
#endif

#ifndef UA_accept
int UA_accept(UA_SOCKET sockfd, struct sockaddr *addr, socklen_t *addrlen);//equivalent to posix accept implementation
#endif

#ifndef UA_close
int UA_close(UA_SOCKET sockfd);//equivalent to posix close implementation
#endif

#ifndef UA_connect
int UA_connect(UA_SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen);//equivalent to posix connect implementation
#endif

#ifndef UA_fd_set
void UA_fd_set(UA_SOCKET fd, fd_set *set); //equivalent to posix FD_SET implementation
#endif

#ifndef UA_fd_isset
int UA_fd_isset(UA_SOCKET fd, fd_set *set);//equivalent to posix FD_ISSET implementation
#endif

#ifndef UA_getaddrinfo
int UA_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);//equivalent to posix getaddrinfo implementation
#endif

#ifndef UA_htonl
uint32_t UA_htonl(uint32_t hostlong);//equivalent to posix UA_htonl implementation
#endif

#ifndef UA_ntohl
uint32_t UA_ntohl(uint32_t netlong);//equivalent to posix ntohl implementation
#endif

#ifndef UA_inet_pton
int UA_inet_pton(int af, const char *src, void *dst);//equivalent to ANSI inet_pton implementation
#endif

#if UA_IPV6
# ifndef UA_if_nametoindex
unsigned int UA_if_nametoindex(const char *ifname);//equivalent to posix if_nametoindex implementation
# endif
#endif

#ifndef UA_socket_set_blocking
unsigned int UA_socket_set_blocking(UA_SOCKET sockfd);//set a socket as blocking. Returns 0 if OK, other value otherwise
#endif

#ifndef UA_socket_set_nonblocking
unsigned int UA_socket_set_nonblocking(UA_SOCKET sockfd);//set a socket as non-blocking. Returns 0 if OK, other value otherwise
#endif

#ifndef UA_getsockopt
int UA_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen); //equivalent to posix getsockopt implementation. Only in non windows architectures
#endif

#ifndef UA_setsockopt
int UA_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);//equivalent to posix setsockopt implementation
#endif

#ifndef UA_freeaddrinfo
void UA_freeaddrinfo(struct addrinfo *res);//equivalent to posix freeaddrinfo implementatio
#endif

#ifndef UA_gethostname
int UA_gethostname(char *name, size_t len);//equivalent to posix gethostname implementation
#endif

#ifndef UA_getsockname
int UA_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);//equivalent to posix getsockname implementation
#endif

#ifndef UA_initialize_architecture_network
void UA_initialize_architecture_network(void);//initializes all needed for using the network interfaces
#endif

#ifndef UA_deinitialize_architecture_network
void UA_deinitialize_architecture_network(void);//de-initializes the network interfaces
#endif

/*
 * Print function
 */
#ifndef UA_snprintf
int UA_snprintf(char* pa_stream, size_t pa_size, const char* pa_format, ...); //prints text to output
#endif

/*
 * Access to file function
 */

#ifndef UA_access
int UA_access(const char *pathname, int mode); //equivalent implementation of https://linux.die.net/man/2/access
#endif

#ifndef UA_fileExists
#define UA_fileExists(X) ( UA_access(X, 0) == 0)
#endif

#include <open62541/architecture_definitions.h>

_UA_END_DECLS

#endif /* PLUGINS_ARCH_UA_ARCHITECTURE_FUNCTIONS_H_ */
