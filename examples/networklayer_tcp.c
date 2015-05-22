 /*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifdef NOT_AMALGATED
# define _XOPEN_SOURCE 500 //some users need this for some reason
# include <stdlib.h> // malloc, free
# include <string.h> // memset
#endif

#ifdef _WIN32
# include <malloc.h>
# include <winsock2.h>
# include <ws2tcpip.h>
# define CLOSESOCKET(S) closesocket(S)
#else
# include <fcntl.h>
# include <sys/select.h> 
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <sys/ioctl.h>
# include <netdb.h> //gethostbyname for the client
# include <unistd.h> // read, write, close
# include <arpa/inet.h>
# define CLOSESOCKET(S) close(S)
#endif

#include "networklayer_tcp.h" // UA_MULTITHREADING is defined in here
#ifdef UA_MULTITHREADING
# include <urcu/uatomic.h>
#endif

/* with a space, so the include is not removed during amalgamation */
# include <errno.h>

/****************************/
/* Generic Socket Functions */
/****************************/

static UA_StatusCode socket_write(UA_Connection *connection, const UA_ByteString *buf) {
    if(buf->length < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Int32 nWritten = 0;
    while (nWritten < buf->length) {
        UA_Int32 n = 0;
        do {
#ifdef _WIN32
            n = send((SOCKET)connection->sockfd, (const char*)buf->data, buf->length, 0);
            if(n < 0 && WSAGetLastError() != WSAEINTR && WSAGetLastError() != WSAEWOULDBLOCK)
                return UA_STATUSCODE_BADCONNECTIONCLOSED;
#else
            n = send(connection->sockfd, (const char*)buf->data, buf->length, MSG_NOSIGNAL);
            if(n == -1L && errno != EINTR && errno != EAGAIN)
                return UA_STATUSCODE_BADCONNECTIONCLOSED;
#endif
        } while (n == -1L);
        nWritten += n;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode socket_recv(UA_Connection *connection, UA_ByteString *response, UA_UInt32 timeout) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
	if(response->data == NULL)
        retval = connection->getBuffer(connection, response, connection->localConf.recvBufferSize);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    struct timeval tmptv = {0, timeout * 1000};
    if(0 != setsockopt(connection->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tmptv, sizeof(struct timeval))){
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    int ret = recv(connection->sockfd, (char*)response->data, response->length, 0);
	if(ret == 0) {
		connection->releaseBuffer(connection, response);
		connection->close(connection);
		return UA_STATUSCODE_BADCONNECTIONCLOSED;
	} else if(ret < 0) {
#ifdef _WIN32
		if(WSAGetLastError() == WSAEINTR || WSAGetLastError() == WSAEWOULDBLOCK) {
#else
		if (errno == EAGAIN) {
#endif
            return UA_STATUSCODE_BADCOMMUNICATIONERROR;
        }
        connection->releaseBuffer(connection, response);
		connection->close(connection);
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    response->length = ret;
    *response = UA_Connection_completeMessages(connection, *response);
    return UA_STATUSCODE_GOOD;
}

static void socket_close(UA_Connection *connection) {
    connection->state = UA_CONNECTION_CLOSED;
    shutdown(connection->sockfd,2);
    CLOSESOCKET(connection->sockfd);
}

static UA_StatusCode socket_set_nonblocking(UA_Int32 sockfd) {
#ifdef _WIN32
    u_long iMode = 1;
    if(ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
        return UA_STATUSCODE_BADINTERNALERROR;
#else
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0 || fcntl(sockfd, F_SETFL, opts|O_NONBLOCK) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}

/*****************************/
/* Generic Buffer Management */
/*****************************/

static UA_StatusCode GetMallocedBuffer(UA_Connection *connection, UA_ByteString *buf, size_t minSize) {
    return UA_ByteString_newMembers(buf, minSize);
}

static void ReleaseMallocedBuffer(UA_Connection *connection, UA_ByteString *buf) {
    UA_ByteString_deleteMembers(buf);
}

/***************************/
/* Server NetworkLayer TCP */
/***************************/

/**
 * For the multithreaded mode, assume a single thread that periodically "gets work" from the network
 * layer. In addition, several worker threads are asynchronously calling into the callbacks of the
 * UA_Connection that holds a single connection.
 *
 * Creating a connection: When "GetWork" encounters a new connection, it creates a UA_Connection
 * with the socket information. This is added to the mappings array that links sockets to
 * UA_Connection structs.
 *
 * Reading data: In "GetWork", we listen on the sockets in the mappings array. If data arrives (or
 * the connection closes), a WorkItem is created that carries the work and a pointer to the
 * connection.
 *
 * Closing a connection: Closing can happen in two ways. Either it is triggered by the server in an
 * asynchronous callback. Or the connection is close by the client and this is detected in
 * "GetWork". The server needs to do some internal cleanups (close attached securechannels, etc.).
 * So even when a closed connection is detected in "GetWork", we trigger the server to close the
 * connection (with a WorkItem) and continue from the callback.
 *
 * - Server calls close-callback: We close the socket, set the connection-state to closed and add
 *   the connection to a linked list from which it is deleted later. The connection cannot be freed
 *   right away since other threads might still be using it.
 *
 * - GetWork: We remove the connection from the mappings array. In the non-multithreaded case, the
 *   connection is freed. For multithreading, we return a workitem that is delayed, i.e. that is
 *   called only after all workitems created before are finished in all threads. This workitems
 *   contains a callback that goes through the linked list of connections to be freed.
 *
 */

#define MAXBACKLOG 100

typedef struct {
    /* config */
    UA_Logger *logger;
    UA_UInt32 port;
    UA_String discoveryUrl;
    UA_ConnectionConfig conf; /* todo: rename to localconf. */

    /* open sockets and connections */
    fd_set fdset;
    UA_Int32 serversockfd;
    UA_Int32 highestfd;
    size_t mappingsSize;
    struct ConnectionMapping {
        UA_Connection *connection;
        UA_Int32 sockfd;
    } *mappings;

    /* to-be-deleted connections */
    struct DeleteList {
        struct DeleteList *next;
        UA_Connection *connection;
    } *deletes;
} ServerNetworkLayerTCP;

/* after every select, we need to reset the sockets we want to listen on */
static void setFDSet(ServerNetworkLayerTCP *layer) {
    FD_ZERO(&layer->fdset);
#ifdef _WIN32
    FD_SET((UA_UInt32)layer->serversockfd, &layer->fdset);
#else
    FD_SET(layer->serversockfd, &layer->fdset);
#endif
    layer->highestfd = layer->serversockfd;
    for(size_t i = 0; i < layer->mappingsSize; i++) {
#ifdef _WIN32
        FD_SET((UA_UInt32)layer->mappings[i].sockfd, &layer->fdset);
#else
        FD_SET(layer->mappings[i].sockfd, &layer->fdset);
#endif
        if(layer->mappings[i].sockfd > layer->highestfd)
            layer->highestfd = layer->mappings[i].sockfd;
    }
}

/* callback triggered from the server */
static void ServerNetworkLayerTCP_closeConnection(UA_Connection *connection) {
#ifdef UA_MULTITHREADING
    if(uatomic_xchg(&connection->state, UA_CONNECTION_CLOSED) == UA_CONNECTION_CLOSED)
        return;
#else
    if(connection->state == UA_CONNECTION_CLOSED)
        return;
    connection->state = UA_CONNECTION_CLOSED;
#endif
    socket_close(connection);
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP*)connection->handle;
    struct DeleteList *d = malloc(sizeof(struct DeleteList));
    if(!d){
        return;
    }
    d->connection = connection;
#ifdef UA_MULTITHREADING
    while(1) {
        d->next = layer->deletes;
        if(uatomic_cmpxchg(&layer->deletes, d->next, d) == d->next)
            break;
    }
#else
    d->next = layer->deletes;
    layer->deletes = d;
#endif
}

/* call only from the single networking thread */
static UA_StatusCode ServerNetworkLayerTCP_add(ServerNetworkLayerTCP *layer, UA_Int32 newsockfd) {
    UA_Connection *c = malloc(sizeof(UA_Connection));
    if(!c)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Connection_init(c);
    c->sockfd = newsockfd;
    c->handle = layer;
    c->localConf = layer->conf;
    c->write = socket_write;
    c->close = ServerNetworkLayerTCP_closeConnection;
    c->getBuffer = GetMallocedBuffer;
    c->releaseBuffer = ReleaseMallocedBuffer;
    c->state = UA_CONNECTION_OPENING;
    struct ConnectionMapping *nm =
        realloc(layer->mappings, sizeof(struct ConnectionMapping)*(layer->mappingsSize+1));
    if(!nm) {
        free(c);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    layer->mappings = nm;
    layer->mappings[layer->mappingsSize] = (struct ConnectionMapping){c, newsockfd};
    layer->mappingsSize++;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode ServerNetworkLayerTCP_start(ServerNetworkLayerTCP *layer, UA_Logger *logger) {
    layer->logger = logger;
#ifdef _WIN32
    if((layer->serversockfd = socket(PF_INET, SOCK_STREAM,0)) == (UA_Int32)INVALID_SOCKET) {
        UA_LOG_WARNING((*layer->logger), UA_LOGCATEGORY_COMMUNICATION, "Error opening socket, code: %d",
                       WSAGetLastError());
        return UA_STATUSCODE_BADINTERNALERROR;
    }
#else
    if((layer->serversockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        UA_LOG_WARNING((*layer->logger), UA_LOGCATEGORY_COMMUNICATION, "Error opening socket");
        return UA_STATUSCODE_BADINTERNALERROR;
    } 
#endif
    const struct sockaddr_in serv_addr =
        {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY,
         .sin_port = htons(layer->port), .sin_zero = {0}};
    int optval = 1;
    if(setsockopt(layer->serversockfd, SOL_SOCKET,
                  SO_REUSEADDR, (const char *)&optval,
                  sizeof(optval)) == -1) {
        UA_LOG_WARNING((*layer->logger), UA_LOGCATEGORY_COMMUNICATION, "Error during setting of socket options");
        CLOSESOCKET(layer->serversockfd);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(bind(layer->serversockfd, (const struct sockaddr *)&serv_addr,
            sizeof(serv_addr)) < 0) {
        UA_LOG_WARNING((*layer->logger), UA_LOGCATEGORY_COMMUNICATION, "Error during socket binding");
        CLOSESOCKET(layer->serversockfd);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    socket_set_nonblocking(layer->serversockfd);
    listen(layer->serversockfd, MAXBACKLOG);
    UA_LOG_INFO((*layer->logger), UA_LOGCATEGORY_COMMUNICATION, "Listening on %.*s",
                layer->discoveryUrl.length, layer->discoveryUrl.data);
    return UA_STATUSCODE_GOOD;
}

/* delayed callback that frees old connections */
static void freeConnections(UA_Server *server, struct DeleteList *d) {
    while(d) {
        UA_Connection_deleteMembers(d->connection);
        free(d->connection);
        struct DeleteList *old = d;
        d = d->next;
        free(old);
    }
}

/* remove the closed sockets from the mappings array */
static void removeMappings(ServerNetworkLayerTCP *layer, struct DeleteList *d) {
    while(d) {
        size_t i = 0;
        for(; i < layer->mappingsSize; i++) {
            if(layer->mappings[i].sockfd == d->connection->sockfd)
                break;
        }
        if(i >= layer->mappingsSize)
            continue;
        layer->mappingsSize--;
        layer->mappings[i] = layer->mappings[layer->mappingsSize];
        d = d->next;
    }
}

static UA_Int32 ServerNetworkLayerTCP_getWork(ServerNetworkLayerTCP *layer, UA_WorkItem **workItems,
                                              UA_UInt16 timeout) {
    struct DeleteList *deletes;
#ifdef UA_MULTITHREADING
        deletes = uatomic_xchg(&layer->deletes, NULL);
#else
        deletes = layer->deletes;
        layer->deletes = NULL;
#endif
    removeMappings(layer, deletes);
    setFDSet(layer);
    struct timeval tmptv = {0, timeout};
    UA_Int32 resultsize = select(layer->highestfd+1, &layer->fdset, NULL, NULL, &tmptv);
    UA_WorkItem *items;
    if(resultsize < 0 || !(items = malloc(sizeof(UA_WorkItem)*(resultsize+1)))) {
        /* abort .. reattach the deletes so that they get deleted eventually.. */
#ifdef UA_MULTITHREADING
        struct DeleteList *last_delete = deletes;
        while(last_delete->next != NULL)
            last_delete = last_delete->next;
        while(1) {
            last_delete->next = layer->deletes;
            if(uatomic_cmpxchg(&layer->deletes, last_delete->next, deletes) == last_delete->next)
                break;
        }
#else
        layer->deletes = deletes;
#endif
        *workItems = NULL;
        return 0;
    }

    /* accept new connections (can only be a single one) */
    if(FD_ISSET(layer->serversockfd, &layer->fdset)) {
        resultsize--;
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int newsockfd = accept(layer->serversockfd, (struct sockaddr *) &cli_addr, &cli_len);
        int i = 1;
        setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));
        if (newsockfd >= 0) {
            socket_set_nonblocking(newsockfd);
            ServerNetworkLayerTCP_add(layer, newsockfd);
        }
    }
    
    /* read from established sockets */
    UA_Int32 j = 0;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    for(size_t i = 0; i < layer->mappingsSize && j < resultsize; i++) {
        if(!(FD_ISSET(layer->mappings[i].sockfd, &layer->fdset)))
            continue;
        if(!buf.data) {
            buf.data = malloc(sizeof(UA_Byte) * layer->conf.recvBufferSize);
            buf.length = layer->conf.recvBufferSize;
            if(!buf.data)
                break;
        }
        if(socket_recv(layer->mappings[i].connection, &buf, 0) == UA_STATUSCODE_GOOD) {
            items[j].type = UA_WORKITEMTYPE_BINARYMESSAGE;
            items[j].work.binaryMessage.message = buf;
            items[j].work.binaryMessage.connection = layer->mappings[i].connection;
            buf.data = NULL;
        } else {
            items[j].type = UA_WORKITEMTYPE_CLOSECONNECTION;
            items[j].work.closeConnection = layer->mappings[i].connection;
        }
        j++;
    }

    /* add the delayed work that frees the connections */
    if(deletes) {
        items[j].type = UA_WORKITEMTYPE_DELAYEDMETHODCALL;
        items[j].work.methodCall.data = deletes;
        items[j].work.methodCall.method = (void (*)(UA_Server *server, void *data))freeConnections;
        j++;
    }

    if(buf.data)
        free(buf.data);

    /* free the array if there is no work */
    if(j == 0) {
        free(items);
        *workItems = NULL;
    } else
        *workItems = items;
    return j;
}

static UA_Int32 ServerNetworkLayerTCP_stop(ServerNetworkLayerTCP *layer, UA_WorkItem **workItems) {
    struct DeleteList *deletes;
#ifdef UA_MULTITHREADING
        deletes = uatomic_xchg(&layer->deletes, NULL);
#else
        deletes = layer->deletes;
        layer->deletes = NULL;
#endif
    removeMappings(layer, deletes);
    UA_WorkItem *items = malloc(sizeof(UA_WorkItem) * layer->mappingsSize);
    if(!items)
        return 0;
    for(size_t i = 0; i < layer->mappingsSize; i++) {
        items[i].type = UA_WORKITEMTYPE_CLOSECONNECTION;
        items[i].work.closeConnection = layer->mappings[i].connection;
    }
#ifdef _WIN32
    WSACleanup();
#endif
    *workItems = items;
    return layer->mappingsSize;
}

/* run only when the server is stopped */
static void ServerNetworkLayerTCP_delete(ServerNetworkLayerTCP *layer) {
    UA_String_deleteMembers(&layer->discoveryUrl);
    removeMappings(layer, layer->deletes);
    freeConnections(NULL, layer->deletes);
    for(size_t i = 0; i < layer->mappingsSize; i++)
        free(layer->mappings[i].connection);
    free(layer->mappings);
    free(layer);
}

UA_ServerNetworkLayer ServerNetworkLayerTCP_new(UA_ConnectionConfig conf, UA_UInt32 port) {
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
#endif
    UA_ServerNetworkLayer nl;
    memset(&nl, 0, sizeof(UA_ServerNetworkLayer));

    ServerNetworkLayerTCP *layer = malloc(sizeof(ServerNetworkLayerTCP));
    if(!layer){
        return nl;
    }
    layer->conf = conf;
    layer->mappingsSize = 0;
    layer->mappings = NULL;
    layer->port = port;
    layer->deletes = NULL;
    char hostname[256];
    gethostname(hostname, 255);
    UA_String_copyprintf("opc.tcp://%s:%d", &layer->discoveryUrl, hostname, port);

    nl.nlHandle = layer;
    nl.start = (UA_StatusCode (*)(void*, UA_Logger *logger))ServerNetworkLayerTCP_start;
    nl.getWork = (UA_Int32 (*)(void*, UA_WorkItem**, UA_UInt16))ServerNetworkLayerTCP_getWork;
    nl.stop = (UA_Int32 (*)(void*, UA_WorkItem**))ServerNetworkLayerTCP_stop;
    nl.free = (void (*)(void*))ServerNetworkLayerTCP_delete;
    nl.discoveryUrl = &layer->discoveryUrl;
    return nl;
}

/***************************/
/* Client NetworkLayer TCP */
/***************************/

UA_Connection ClientNetworkLayerTCP_connect(char *endpointUrl, UA_Logger *logger) {
    UA_Connection connection;
    UA_Connection_init(&connection);

    size_t urlLength = strlen(endpointUrl);
    if(urlLength < 11 || urlLength >= 512) {
        UA_LOG_WARNING((*logger), UA_LOGCATEGORY_COMMUNICATION, "Server url size invalid");
        return connection;
    }
    if(strncmp(endpointUrl, "opc.tcp://", 10) != 0) {
        UA_LOG_WARNING((*logger), UA_LOGCATEGORY_COMMUNICATION, "Server url does not begin with opc.tcp://");
        return connection;
    }

    UA_UInt16 portpos = 9;
    UA_UInt16 port = 0;
    for(;portpos < urlLength-1; portpos++) {
        if(endpointUrl[portpos] == ':') {
            port = atoi(&endpointUrl[portpos+1]);
            break;
        }
    }
    if(port == 0) {
        UA_LOG_WARNING((*logger), UA_LOGCATEGORY_COMMUNICATION, "Port invalid");
        return connection;
    }

    char hostname[512];
    for(int i=10; i < portpos; i++)
        hostname[i-10] = endpointUrl[i];
    hostname[portpos-10] = 0;
#ifdef _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
    if((connection.sockfd = socket(PF_INET, SOCK_STREAM,0)) == (UA_Int32)INVALID_SOCKET) {
#else
    if((connection.sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#endif
        UA_LOG_WARNING((*logger), UA_LOGCATEGORY_COMMUNICATION, "Could not create socket");
        return connection;
    }
    struct hostent *server = gethostbyname(hostname);
    if(server == NULL) {
        UA_LOG_WARNING((*logger), UA_LOGCATEGORY_COMMUNICATION, "DNS lookup of %s failed", hostname);
        return connection;
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    memcpy((char *)&server_addr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if(connect(connection.sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        UA_LOG_WARNING((*logger), UA_LOGCATEGORY_COMMUNICATION, "Connection failed");
        return connection;
    }
    connection.state = UA_CONNECTION_OPENING;
    //socket_set_nonblocking(connection.sockfd);
    connection.write = socket_write;
    connection.recv = socket_recv;
    connection.close = socket_close;
    connection.getBuffer = GetMallocedBuffer;
    connection.releaseBuffer = ReleaseMallocedBuffer;
    return connection;
}
