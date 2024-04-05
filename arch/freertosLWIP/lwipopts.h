#ifndef LWIP_OPTS_H
#define LWIP_OPTS_H

#define LWIP_COMPAT_SOCKETS 0 // Don't do name define-transformation in networking function names.
#define LWIP_COMPAT_MUTEX 1
#define LWIP_DHCP 1
#define LWIP_SOCKET 1 // Enable Socket API (normally already set)
#define LWIP_DNS 1 // enable the lwip_getaddrinfo function, struct addrinfo and more.
#define SO_REUSE 1 // Allows to set the socket as reusable
#define LWIP_TIMEVAL_PRIVATE 0 // This is optional. Set this flag if you get a compilation error about redefinition of struct timeval

#endif
