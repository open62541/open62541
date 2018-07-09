/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 *
 *    Copyright 2016-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 */

#include "ua_network_udp.h"
#include <stdio.h>
#include <string.h> // memset

/* with a space so amalgamation does not remove the includes */
# include <errno.h> // errno, EINTR
# include <fcntl.h> // fcntl
# include <strings.h> //bzero

#if defined(UA_FREERTOS)
 # include <lwip/udp.h>
 # include <lwip/tcpip.h>
#else
# ifndef _WRS_KERNEL
#  include <sys/select.h>
# else
#  include <selectLib.h>
# endif
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <sys/socketvar.h>
#  include <sys/ioctl.h>
#  include <unistd.h> // read, write, close
#  include <arpa/inet.h>
#endif
#ifdef __QNX__
#include <sys/socket.h>
#endif
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
# include <sys/param.h>
# if defined(BSD)
#  include<sys/socket.h>
# endif
#endif
# define CLOSESOCKET(S) close(S)

#define MAXBACKLOG 100

#ifdef _WIN32
# error udp not yet implemented for windows
#endif

/*****************************/
/* Generic Buffer Management */
/*****************************/

static UA_StatusCode GetMallocedBuffer(UA_Connection *connection, size_t length, UA_ByteString *buf) {
    if(length > connection->remoteConf.recvBufferSize)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    return UA_ByteString_allocBuffer(buf, connection->remoteConf.recvBufferSize);
}

static void ReleaseMallocedBuffer(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

/*********************/
/* UDP Network Layer */
/*********************/

/* Forwarded to the server as a (UA_Connection) and used for callbacks back into
   the networklayer */
typedef struct {
    UA_Connection connection;
    struct sockaddr from;
    socklen_t fromlen;
} UDPConnection;

typedef struct {
    UA_ConnectionConfig conf;
    UA_UInt16 port;
    fd_set fdset;
    UA_Int32 serversockfd;

    UA_Logger logger; // Set during start
} ServerNetworkLayerUDP;

/** Accesses only the sockfd in the handle. Can be run from parallel threads. */
static UA_StatusCode sendUDP(UA_Connection *connection, UA_ByteString *buf) {
    UDPConnection *udpc = (UDPConnection*)connection;
    ServerNetworkLayerUDP *layer = (ServerNetworkLayerUDP*)connection->handle;
    long nWritten = 0;
    struct sockaddr_in *sin = NULL;

    if (udpc->from.sa_family == AF_INET) {
#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4 || defined(__clang__))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#endif
        sin = (struct sockaddr_in *) &udpc->from;
#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4 || defined(__clang__))
#pragma GCC diagnostic pop
#endif
    } else {
        UA_ByteString_deleteMembers(buf);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    while (nWritten < (long)buf->length) {
        long n = sendto(layer->serversockfd, buf->data, buf->length, 0,
                            (struct sockaddr*)sin, sizeof(struct sockaddr_in));
        if(n == -1L) {
            UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK, "UDP send error %i", errno);
            UA_ByteString_deleteMembers(buf);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        nWritten += n;
    }
    UA_ByteString_deleteMembers(buf);
    return UA_STATUSCODE_GOOD;
}

static void setFDSet(ServerNetworkLayerUDP *layer) {
    FD_ZERO(&layer->fdset);
    FD_SET(layer->serversockfd, &layer->fdset);
}

static void closeConnectionUDP(UA_Connection *handle) {
    UA_free(handle);
}

static UA_StatusCode ServerNetworkLayerUDP_start(UA_ServerNetworkLayer *nl, UA_Logger logger) {
    ServerNetworkLayerUDP *layer = nl->handle;
    layer->logger = logger;
    layer->serversockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if(layer->serversockfd < 0) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK, "Error opening socket");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    struct sockaddr_in6 serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(layer->port);
    serv_addr.sin6_addr = in6addr_any;
    int optval = 1;
    if(setsockopt(layer->serversockfd, SOL_SOCKET,
                  SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK, "Could not setsockopt");
        CLOSESOCKET(layer->serversockfd);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(bind(layer->serversockfd, (const struct sockaddr *)&serv_addr,
            sizeof(serv_addr)) < 0) {
        UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK, "Could not bind the socket");
        CLOSESOCKET(layer->serversockfd);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_socket_set_nonblocking(layer->serversockfd);
    UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK, "Listening for UDP connections on %s:%d",
                   inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    return UA_STATUSCODE_GOOD;
}

static size_t ServerNetworkLayerUDP_getJobs(UA_ServerNetworkLayer *nl, UA_Job **jobs, UA_UInt16 timeout) {
    ServerNetworkLayerUDP *layer = nl->handle;
    UA_Job *items = NULL;
    setFDSet(layer);
    struct timeval tmptv = {0, timeout};
    int resultsize = select(layer->serversockfd+1, &layer->fdset, NULL, NULL, &tmptv);
    if(resultsize <= 0 || !FD_ISSET(layer->serversockfd, &layer->fdset)) {
        *jobs = items;
        return 0;
    }
    items = UA_malloc(sizeof(UA_Job)*(unsigned long)resultsize);
    // read from established sockets
    size_t j = 0;
    UA_ByteString buf = {0, NULL};
    if(!buf.data) {
        buf.data = UA_malloc(sizeof(UA_Byte) * layer->conf.recvBufferSize);
        if(!buf.data)
            UA_LOG_WARNING(layer->logger, UA_LOGCATEGORY_NETWORK, "malloc failed");
    }
    struct sockaddr sender;
    socklen_t sendsize = sizeof(sender);
    bzero(&sender, sizeof(sender));
    ssize_t rec_result = recvfrom(layer->serversockfd, buf.data, layer->conf.recvBufferSize, 0, &sender, &sendsize);
    if (rec_result > 0) {
        buf.length = (size_t)rec_result;
        UDPConnection *c = UA_malloc(sizeof(UDPConnection));
        if(!c){
                UA_free(items);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        UA_Connection_init(&c->connection);
        c->from = sender;
        c->fromlen = sendsize;
        // c->sockfd = newsockfd;
        c->connection.getSendBuffer = GetMallocedBuffer;
        c->connection.releaseSendBuffer = ReleaseMallocedBuffer;
        c->connection.releaseRecvBuffer = ReleaseMallocedBuffer;
        c->connection.handle = layer;
        c->connection.send = sendUDP;
        c->connection.close = closeConnectionUDP;
        c->connection.localConf = layer->conf;
        c->connection.state = UA_CONNECTION_OPENING;

        items[j].type = UA_JOBTYPE_BINARYMESSAGE_NETWORKLAYER;
        items[j].job.binaryMessage.message = buf;
        items[j].job.binaryMessage.connection = (UA_Connection*)c;
        buf.data = NULL;
        j++;
        *jobs = items;
    } else {
        UA_free(items);
        *jobs = NULL;
    }
    if(buf.data)
        UA_free(buf.data);
    return j;
}

static size_t ServerNetworkLayerUDP_stop(UA_ServerNetworkLayer *nl, UA_Job **jobs) {
    ServerNetworkLayerUDP *layer = nl->handle;
    UA_LOG_INFO(layer->logger, UA_LOGCATEGORY_NETWORK,
                "Shutting down the UDP network layer");
    CLOSESOCKET(layer->serversockfd);
    return 0;
}

static void ServerNetworkLayerUDP_deleteMembers(UA_ServerNetworkLayer *nl) {
    ServerNetworkLayerUDP *layer = nl->handle;
    UA_free(layer);
    UA_String_deleteMembers(&nl->discoveryUrl);
}

UA_ServerNetworkLayer
UA_ServerNetworkLayerUDP(UA_ConnectionConfig conf, UA_UInt16 port) {
    UA_ServerNetworkLayer nl;
    memset(&nl, 0, sizeof(UA_ServerNetworkLayer));

    ServerNetworkLayerUDP *layer = UA_malloc(sizeof(ServerNetworkLayerUDP));
    if(!layer)
        return nl;
    memset(layer, 0, sizeof(ServerNetworkLayerUDP));

    layer->conf = conf;
    layer->port = port;

    nl.handle = layer;
    nl.start = ServerNetworkLayerUDP_start;
    nl.getJobs = ServerNetworkLayerUDP_getJobs;
    nl.stop = ServerNetworkLayerUDP_stop;
    nl.deleteMembers = ServerNetworkLayerUDP_deleteMembers;
    return nl;
}
