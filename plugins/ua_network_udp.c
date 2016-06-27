/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include "networklayer_udp.h"
#include <stdlib.h> // malloc, free
#include <stdio.h>
#include <string.h> // memset

#ifdef UA_ENABLE_MULTITHREADING
# include <urcu/uatomic.h>
#endif

/* with a space so amalgamation does not remove the includes */
# include <errno.h> // errno, EINTR
# include <fcntl.h> // fcntl
# include <strings.h> //bzero
# include <sys/select.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <sys/socketvar.h>
# include <sys/ioctl.h>
# include <unistd.h> // read, write, close
# include <arpa/inet.h>
#ifdef __QNX__
#include <sys/socket.h>
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
    return UA_ByteString_newMembers(buf, connection->remoteConf.recvBufferSize);
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
    UA_ServerNetworkLayer layer;
    UA_ConnectionConfig conf;
    fd_set fdset;
    UA_Int32 serversockfd;
    UA_UInt32 port;
} ServerNetworkLayerUDP;

/** Accesses only the sockfd in the handle. Can be run from parallel threads. */
static UA_StatusCode sendUDP(UA_Connection *connection, UA_ByteString *buf) {
    UDPConnection *udpc = (UDPConnection*)connection;
    ServerNetworkLayerUDP *layer = (ServerNetworkLayerUDP*)connection->handle;
    size_t nWritten = 0;
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

    while (nWritten < (size_t)buf->length) {
        UA_Int32 n = sendto(layer->serversockfd, buf->data, buf->length, 0,
                            (struct sockaddr*)sin, sizeof(struct sockaddr_in));
        if(n == -1L) {
            UA_LOG_WARNING(layer->layer.logger, UA_LOGCATEGORY_NETWORK, "UDP send error %i", errno);
            UA_ByteString_deleteMembers(buf);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        nWritten += n;
    }
    UA_ByteString_deleteMembers(buf);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode socket_set_nonblocking(UA_Int32 sockfd) {
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0 || fcntl(sockfd, F_SETFL, opts|O_NONBLOCK) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    return UA_STATUSCODE_GOOD;
}

static void setFDSet(ServerNetworkLayerUDP *layer) {
    FD_ZERO(&layer->fdset);
    FD_SET(layer->serversockfd, &layer->fdset);
}

static void closeConnectionUDP(UA_Connection *handle) {
    free(handle);
}

static UA_StatusCode ServerNetworkLayerUDP_start(ServerNetworkLayerUDP *layer, UA_Logger logger) {
    layer->layer.logger = logger;
    layer->serversockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if(layer->serversockfd < 0) {
        UA_LOG_WARNING(layer->layer.logger, UA_LOGCATEGORY_NETWORK, "Error opening socket");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    const struct sockaddr_in serv_addr =
        {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY,
         .sin_port = htons(layer->port), .sin_zero = {0}};
    int optval = 1;
    if(setsockopt(layer->serversockfd, SOL_SOCKET,
                  SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(layer->layer.logger, UA_LOGCATEGORY_NETWORK, "Could not setsockopt");
        CLOSESOCKET(layer->serversockfd);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(bind(layer->serversockfd, (const struct sockaddr *)&serv_addr,
            sizeof(serv_addr)) < 0) {
        UA_LOG_WARNING(layer->layer.logger, UA_LOGCATEGORY_NETWORK, "Could not bind the socket");
        CLOSESOCKET(layer->serversockfd);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    socket_set_nonblocking(layer->serversockfd);
    UA_LOG_WARNING(layer->layer.logger, UA_LOGCATEGORY_NETWORK, "Listening for UDP connections on %s:%d",
                   inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    return UA_STATUSCODE_GOOD;
}

static size_t ServerNetworkLayerUDP_getJobs(ServerNetworkLayerUDP *layer, UA_Job **jobs, UA_UInt16 timeout) {
    UA_Job *items = NULL;
    setFDSet(layer);
    struct timeval tmptv = {0, timeout};
    UA_Int32 resultsize = select(layer->serversockfd+1, &layer->fdset, NULL, NULL, &tmptv);
    if(resultsize <= 0 || !FD_ISSET(layer->serversockfd, &layer->fdset)) {
        *jobs = items;
        return 0;
    }
    items = malloc(sizeof(UA_Job)*resultsize);
    // read from established sockets
    UA_Int32 j = 0;
    UA_ByteString buf = {-1, NULL};
    if(!buf.data) {
        buf.data = malloc(sizeof(UA_Byte) * layer->conf.recvBufferSize);
        if(!buf.data)
            UA_LOG_WARNING(layer->layer.logger, UA_LOGCATEGORY_NETWORK, "malloc failed");
    }
    struct sockaddr sender;
    socklen_t sendsize = sizeof(sender);
    bzero(&sender, sizeof(sender));
    ssize_t rec_result = recvfrom(layer->serversockfd, buf.data, layer->conf.recvBufferSize, 0, &sender, &sendsize);
    if (rec_result > 0) {
        buf.length = rec_result;
        UDPConnection *c = malloc(sizeof(UDPConnection));
        if(!c){
                free(items);
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
        free(items);
        *jobs = NULL;
    }
    if(buf.data)
        free(buf.data);
    return j;
}

static UA_Int32 ServerNetworkLayerUDP_stop(ServerNetworkLayerUDP * layer, UA_Job **jobs) {
    CLOSESOCKET(layer->serversockfd);
    return 0;
}

static void ServerNetworkLayerUDP_deleteMembers(ServerNetworkLayerUDP *layer) {
}

UA_ServerNetworkLayer * ServerNetworkLayerUDP_new(UA_ConnectionConfig conf, UA_UInt32 port) {
    ServerNetworkLayerUDP *layer = malloc(sizeof(ServerNetworkLayerUDP));
    if(!layer)
        return NULL;
    memset(layer, 0, sizeof(ServerNetworkLayerUDP));

    layer->conf = conf;
    layer->port = port;
    layer->layer.start = (UA_StatusCode(*)(UA_ServerNetworkLayer*,UA_Logger))ServerNetworkLayerUDP_start;
    layer->layer.getJobs = (size_t(*)(UA_ServerNetworkLayer*,UA_Job**,UA_UInt16))ServerNetworkLayerUDP_getJobs;
    layer->layer.stop = (size_t(*)(UA_ServerNetworkLayer*, UA_Job**))ServerNetworkLayerUDP_stop;
    layer->layer.deleteMembers = (void(*)(UA_ServerNetworkLayer*))ServerNetworkLayerUDP_deleteMembers;
    return &layer->layer;
}
