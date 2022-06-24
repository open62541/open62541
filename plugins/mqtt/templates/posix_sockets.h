#if !defined(__POSIX_SOCKET_TEMPLATE_H__)
#define __POSIX_SOCKET_TEMPLATE_H__

#include <stdio.h>
#include <sys/types.h>
#if !defined(WIN32)
#include <sys/socket.h>
#include <netdb.h>
#else
#include <ws2tcpip.h>
#endif
#if defined(__VMS)
#include <ioctl.h>
#endif
#include <fcntl.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>

/*
    A template for opening a non-blocking POSIX socket.
*/
UA_StatusCode open_nb_socket(int* sockfd, const char* addr, const char* port);

UA_StatusCode open_nb_socket(int* sockfd, const char* addr, const char* port) {
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Must be TCP */
    *sockfd = -1;
    int rv;
    struct addrinfo *p, *servinfo;

    /* get address information */
    rv = getaddrinfo(addr, port, &hints, &servinfo);
    if(rv != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "MQTT PubSub: Failed to open socket (getaddrinfo): %s", gai_strerror(rv));
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* open the first possible socket */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        *sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (*sockfd == -1) continue;

        /* connect to server */
        rv = connect(*sockfd, p->ai_addr, p->ai_addrlen);
        if(rv == -1) {
          close(*sockfd);
          *sockfd = -1;
          continue;
        }
        break;
    }

    /* free servinfo */
    freeaddrinfo(servinfo);

    /* make non-blocking */
#if !defined(WIN32)
    if (*sockfd != -1) fcntl(*sockfd, F_SETFL, fcntl(*sockfd, F_GETFL) | O_NONBLOCK);
#else
    if (*sockfd != INVALID_SOCKET) {
        int iMode = 1;
        ioctlsocket(*sockfd, FIONBIO, &iMode);
    }
#endif
#if defined(__VMS)
    /*
        OpenVMS only partially implements fcntl. It works on file descriptors
        but silently fails on socket descriptors. So we need to fall back on
        to the older ioctl system to set non-blocking IO
    */
    int on = 1;
    if (*sockfd != -1) ioctl(*sockfd, FIONBIO, &on);
#endif

    /* return the new socket fd */
    if(*sockfd == -1)
        return UA_STATUSCODE_BADINTERNALERROR;
    else
        return UA_STATUSCODE_GOOD;
}

#endif
