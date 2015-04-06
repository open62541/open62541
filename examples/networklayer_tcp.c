 /*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifdef NOT_AMALGATED
# define _XOPEN_SOURCE 500 //some users need this for some reason
# define __USE_BSD
# include <stdlib.h> // malloc, free
# include <stdio.h>
# include <string.h> // memset
#endif

#ifdef _WIN32
# include <malloc.h>
# include <winsock2.h>
# include <sys/types.h>
# include <windows.h>
# include <ws2tcpip.h>
# define CLOSESOCKET(S) closesocket(S)
#else
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

/* with a space so amalgamation does not remove the includes */
# include <errno.h> // errno, EINTR
# include <fcntl.h> // fcntl

struct ServerNetworklayer_TCP;

/* Forwarded to the server as a (UA_Connection) and used for callbacks back into
   the networklayer */
typedef struct {
	UA_Connection connection;
	UA_Int32 sockfd;
	void *layer;
} TCPConnection;

/***************************/
/* Server NetworkLayer TCP */
/***************************/

#define MAXBACKLOG 100

/* Internal mapping of sockets to connections */
typedef struct {
    TCPConnection *connection;
#ifdef _WIN32
	UA_UInt32 sockfd;
#else
	UA_Int32 sockfd;
#endif
} ConnectionLink;

typedef struct ServerNetworkLayerTCP {
	UA_ConnectionConfig conf;
	fd_set fdset;
#ifdef _WIN32
	UA_UInt32 serversockfd;
	UA_UInt32 highestfd;
#else
	UA_Int32 serversockfd;
	UA_Int32 highestfd;
#endif
    UA_UInt16 conLinksSize;
    ConnectionLink *conLinks;
    UA_UInt32 port;
    UA_String discoveryUrl;
    /* We remove the connection links only in the main thread. Attach
       to-be-deleted links with atomic operations */
    struct deleteLink {
#ifdef _WIN32
		UA_UInt32 sockfd;
#else
		UA_Int32 sockfd;
#endif
        struct deleteLink *next;

    } *deleteLinkList;
} ServerNetworkLayerTCP;

typedef struct ClientNetworkLayerTCP {
	fd_set read_fds;
#ifdef _WIN32
	UA_UInt32 sockfd;
#else
	UA_Int32 sockfd;
#endif
} ClientNetworkLayerTCP;

static UA_StatusCode setNonBlocking(int sockid) {
#ifdef _WIN32
	u_long iMode = 1;
	if(ioctlsocket(sockid, FIONBIO, &iMode) != NO_ERROR)
		return UA_STATUSCODE_BADINTERNALERROR;
#else
	int opts = fcntl(sockid,F_GETFL);
	if(opts < 0 || fcntl(sockid,F_SETFL,opts|O_NONBLOCK) < 0)
		return UA_STATUSCODE_BADINTERNALERROR;
#endif
	return UA_STATUSCODE_GOOD;
}

static void freeConnectionCallback(UA_Server *server, TCPConnection *connection) {
    free(connection);
}

// after every select, reset the set of sockets we want to listen on
static void setFDSet(ServerNetworkLayerTCP *layer) {
	FD_ZERO(&layer->fdset);
	FD_SET(layer->serversockfd, &layer->fdset);
	layer->highestfd = layer->serversockfd;
	for(UA_Int32 i=0;i<layer->conLinksSize;i++) {
		FD_SET(layer->conLinks[i].sockfd, &layer->fdset);
		if(layer->conLinks[i].sockfd > layer->highestfd)
			layer->highestfd = layer->conLinks[i].sockfd;
	}
}

// the callbacks are thread-safe if UA_MULTITHREADING is defined
void closeConnection(TCPConnection *handle);
void writeCallback(TCPConnection *handle, UA_ByteStringArray gather_buf);

static UA_StatusCode ServerNetworkLayerTCP_add(ServerNetworkLayerTCP *layer, UA_Int32 newsockfd) {
    setNonBlocking(newsockfd);
    TCPConnection *c = malloc(sizeof(TCPConnection));
	if(!c)
		return UA_STATUSCODE_BADINTERNALERROR;
	c->sockfd = newsockfd;
    c->layer = layer;
    c->connection.state = UA_CONNECTION_OPENING;
    c->connection.localConf = layer->conf;
    c->connection.channel = (void*)0;
    c->connection.close = (void (*)(void*))closeConnection;
    c->connection.write = (void (*)(void*, UA_ByteStringArray))writeCallback;

    layer->conLinks = realloc(layer->conLinks, sizeof(ConnectionLink)*(layer->conLinksSize+1));
	if(!layer->conLinks) {
		free(c);
		return UA_STATUSCODE_BADINTERNALERROR;
	}
    layer->conLinks[layer->conLinksSize].connection = c;
    layer->conLinks[layer->conLinksSize].sockfd = newsockfd;
    layer->conLinksSize++;
	return UA_STATUSCODE_GOOD;
}

/* Removes all connections from the network layer. Returns the work items to close them properly. */
static UA_UInt32 removeAllConnections(ServerNetworkLayerTCP *layer, UA_WorkItem **returnWork) {
    UA_WorkItem *work;
	if (layer->conLinksSize <= 0 || !(work = malloc(sizeof(UA_WorkItem)*layer->conLinksSize))) {
		*returnWork = NULL;
		return 0;
	}
#ifdef UA_MULTITHREADING
    struct deleteLink *d = uatomic_xchg(&layer->deleteLinkList, (void*)0);
#else
    struct deleteLink *d = layer->deleteLinkList;
    layer->deleteLinkList = (void*)0;
#endif
    UA_UInt32 count = 0;
    while(d) {
        UA_Int32 i;
        for(i = 0;i<layer->conLinksSize;i++) {
            if(layer->conLinks[i].sockfd == d->sockfd)
                break;
        }
        if(i < layer->conLinksSize) {
            TCPConnection *c = layer->conLinks[i].connection;
            layer->conLinksSize--;
            layer->conLinks[i] = layer->conLinks[layer->conLinksSize];
            work[count] = (UA_WorkItem)
                {.type = UA_WORKITEMTYPE_DELAYEDMETHODCALL,
                 .work.methodCall = {.data = c,
                                     .method = (void (*)(UA_Server*,void*))freeConnectionCallback} };
        }
        struct deleteLink *oldd = d;
        d = d->next;
        free(oldd);
        count++;
    }
    *returnWork = work;
    return count;
}

#ifdef UA_MULTITHREADING
void closeConnection(TCPConnection *handle) {
    if(uatomic_xchg(&handle->connection.state, UA_CONNECTION_CLOSING) == UA_CONNECTION_CLOSING)
        return;
    
    UA_Connection_detachSecureChannel(&handle->connection);
	shutdown(handle->sockfd,2);
	CLOSESOCKET(handle->sockfd);

    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP*)handle->layer;

    // Remove the link later in the main thread
    struct deleteLink *d = malloc(sizeof(struct deleteLink));
    d->sockfd = handle->sockfd;
    while(1) {
        d->next = layer->deleteLinkList;
        if(uatomic_cmpxchg(&layer->deleteLinkList, d->next, d) == d->next)
            break;
    }
}
#else
void closeConnection(TCPConnection *handle) {
    if(handle->connection.state == UA_CONNECTION_CLOSING)
        return;

	struct deleteLink *d = malloc(sizeof(struct deleteLink));
	if(!d)
		return;
    handle->connection.state = UA_CONNECTION_CLOSING;

    UA_Connection_detachSecureChannel(&handle->connection);
	shutdown(handle->sockfd,2);
	CLOSESOCKET(handle->sockfd);

    // Remove the link later in the main thread
    d->sockfd = handle->sockfd;
    ServerNetworkLayerTCP *layer = (ServerNetworkLayerTCP*)handle->layer;
    d->next = layer->deleteLinkList;
    layer->deleteLinkList = d;
}
#endif

/** Accesses only the sockfd in the handle. Can be run from parallel threads. */
void writeCallback(TCPConnection *handle, UA_ByteStringArray gather_buf) {
	UA_UInt32 total_len = 0, nWritten = 0;
#ifdef _WIN32
	LPWSABUF buf = _alloca(gather_buf.stringsSize * sizeof(WSABUF));
	memset(buf, 0, sizeof(gather_buf.stringsSize * sizeof(WSABUF)));
	int result = 0;
	for(UA_UInt32 i = 0; i<gather_buf.stringsSize; i++) {
		buf[i].buf = (char*)gather_buf.strings[i].data;
		buf[i].len = gather_buf.strings[i].length;
		total_len += gather_buf.strings[i].length;
	}
	while(nWritten < total_len) {
		UA_UInt32 n = 0;
		do {
			result = WSASend(handle->sockfd, buf, gather_buf.stringsSize ,
                             (LPDWORD)&n, 0, NULL, NULL);
			if(result != 0)
				printf("Error WSASend, code: %d \n", WSAGetLastError());
		} while(errno == EINTR);
		nWritten += n;
	}
#else
	struct iovec iov[gather_buf.stringsSize];
	memset(iov, 0, sizeof(struct iovec)*gather_buf.stringsSize);
	for(UA_UInt32 i=0;i<gather_buf.stringsSize;i++) {
		iov[i].iov_base = gather_buf.strings[i].data;
		iov[i].iov_len = gather_buf.strings[i].length;
		total_len += gather_buf.strings[i].length;
	}
	struct msghdr message;
	memset(&message, 0, sizeof(message));
	message.msg_iov = iov;
	message.msg_iovlen = gather_buf.stringsSize;
	while (nWritten < total_len) {
		UA_Int32 n = 0;
		do {
            n = sendmsg(handle->sockfd, &message, 0);
        } while (n == -1L && errno == EINTR);
        nWritten += n;
	}
#endif
}

static UA_StatusCode ServerNetworkLayerTCP_start(ServerNetworkLayerTCP *layer, UA_Logger *logger) {
#ifdef _WIN32
	if((layer->serversockfd = socket(PF_INET, SOCK_STREAM,0)) == INVALID_SOCKET) {
		printf("ERROR opening socket, code: %d\n", WSAGetLastError());
		return UA_STATUSCODE_BADINTERNALERROR;
	}
#else
    if((layer->serversockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR opening socket");
		return UA_STATUSCODE_BADINTERNALERROR;
	} 
#endif

	const struct sockaddr_in serv_addr = {
        .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(layer->port), .sin_zero = {0}};

	int optval = 1;
	if(setsockopt(layer->serversockfd, SOL_SOCKET,
                  SO_REUSEADDR, (const char *)&optval,
                  sizeof(optval)) == -1) {
		perror("setsockopt");
		CLOSESOCKET(layer->serversockfd);
		return UA_STATUSCODE_BADINTERNALERROR;
	}
		
	if(bind(layer->serversockfd, (const struct sockaddr *)&serv_addr,
            sizeof(serv_addr)) < 0) {
		perror("binding");
		CLOSESOCKET(layer->serversockfd);
		return UA_STATUSCODE_BADINTERNALERROR;
	}

	setNonBlocking(layer->serversockfd);
	listen(layer->serversockfd, MAXBACKLOG);
    char msg[256];
    sprintf(msg, "Listening on %.*s\n", layer->discoveryUrl.length, layer->discoveryUrl.data);
    UA_LOG_INFO((*logger), UA_LOGGERCATEGORY_SERVER, msg);
    return UA_STATUSCODE_GOOD;
}

static UA_Int32 ServerNetworkLayerTCP_getWork(ServerNetworkLayerTCP *layer, UA_WorkItem **workItems,
                                        UA_UInt16 timeout) {
    UA_WorkItem *items = (void*)0;
    UA_Int32 itemsCount = removeAllConnections(layer, &items);
    setFDSet(layer);
    struct timeval tmptv = {0, timeout};
    UA_Int32 resultsize = select(layer->highestfd+1, &layer->fdset, NULL, NULL, &tmptv);

    if(resultsize < 0) {
        *workItems = items;
        return itemsCount;
    }

	// accept new connections (can only be a single one)
	if(FD_ISSET(layer->serversockfd,&layer->fdset)) {
		resultsize--;
		struct sockaddr_in cli_addr;
		socklen_t cli_len = sizeof(cli_addr);
		int newsockfd = accept(layer->serversockfd, (struct sockaddr *) &cli_addr, &cli_len);
		int i = 1;
		setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));
		if (newsockfd >= 0)
			ServerNetworkLayerTCP_add(layer, newsockfd);
	}
    
    items = realloc(items, sizeof(UA_WorkItem)*(itemsCount+resultsize));

	// read from established sockets
    UA_Int32 j = itemsCount;
	UA_ByteString buf = { -1, NULL};
	for(UA_Int32 i=0;i<layer->conLinksSize && j<itemsCount+resultsize;i++) {
		if(!(FD_ISSET(layer->conLinks[i].sockfd, &layer->fdset)))
            continue;

		if(!buf.data) {
			buf.data = malloc(sizeof(UA_Byte) * layer->conf.recvBufferSize);
			if(!buf.data)
				break;
		}
        
#ifdef _WIN32
        buf.length = recv(layer->conLinks[i].sockfd, (char *)buf.data,
                          layer->conf.recvBufferSize, 0);
#else
        buf.length = read(layer->conLinks[i].sockfd, buf.data, layer->conf.recvBufferSize);
#endif
        if (buf.length <= 0) {
            closeConnection(layer->conLinks[i].connection); // work is returned in the next iteration
        } else {
            items[j].type = UA_WORKITEMTYPE_BINARYNETWORKMESSAGE;
            items[j].work.binaryNetworkMessage.message = buf;
            items[j].work.binaryNetworkMessage.connection = &layer->conLinks[i].connection->connection;
            buf.data = NULL;
            j++;
        }
    }

    if(buf.data)
        free(buf.data);

    if(j == 0) {
        free(items);
        *workItems = NULL;
    } else
        *workItems = items;
    return j;
}

static UA_Int32 ServerNetworkLayerTCP_stop(ServerNetworkLayerTCP * layer, UA_WorkItem **workItems) {
	for(UA_Int32 index = 0;index < layer->conLinksSize;index++)
        closeConnection(layer->conLinks[index].connection);
#ifdef _WIN32
	WSACleanup();
#endif
    return removeAllConnections(layer, workItems);
}

static void ServerNetworkLayerTCP_delete(ServerNetworkLayerTCP *layer) {
	UA_String_deleteMembers(&layer->discoveryUrl);
	for(UA_Int32 i=0;i<layer->conLinksSize;++i){
		free(layer->conLinks[i].connection);
	}
	free(layer->conLinks);
	free(layer);
}

UA_ServerNetworkLayer ServerNetworkLayerTCP_new(UA_ConnectionConfig conf, UA_UInt32 port) {
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
#endif
    ServerNetworkLayerTCP *tcplayer = malloc(sizeof(ServerNetworkLayerTCP));
	tcplayer->conf = conf;
	tcplayer->conLinksSize = 0;
	tcplayer->conLinks = NULL;
    tcplayer->port = port;
    tcplayer->deleteLinkList = (void*)0;
    char hostname[256];
    gethostname(hostname, 255);
    UA_String_copyprintf("opc.tcp://%s:%d", &tcplayer->discoveryUrl, hostname, port);

    UA_ServerNetworkLayer nl;
    nl.nlHandle = tcplayer;
    nl.start = (UA_StatusCode (*)(void*, UA_Logger *logger))ServerNetworkLayerTCP_start;
    nl.getWork = (UA_Int32 (*)(void*, UA_WorkItem**, UA_UInt16))ServerNetworkLayerTCP_getWork;
    nl.stop = (UA_Int32 (*)(void*, UA_WorkItem**))ServerNetworkLayerTCP_stop;
    nl.free = (void (*)(void*))ServerNetworkLayerTCP_delete;
	nl.discoveryUrl = &tcplayer->discoveryUrl;

    return nl;
}

/***************************/
/* Client NetworkLayer TCP */
/***************************/

static UA_StatusCode ClientNetworkLayerTCP_connect(const UA_String endpointUrl, ClientNetworkLayerTCP *resultHandle) {
	if(endpointUrl.length < 11 || endpointUrl.length >= 512) {
        printf("server url size invalid\n");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(strncmp((char*)endpointUrl.data, "opc.tcp://", 10) != 0) {
        printf("server url does not begin with opc.tcp://\n");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    //this is somewhat ugly, but atoi needs a c string
    char cstringEndpointUrl[endpointUrl.length+1];
    memset(cstringEndpointUrl, 0, endpointUrl.length+1);
    memcpy(cstringEndpointUrl, endpointUrl.data, endpointUrl.length);
    cstringEndpointUrl[endpointUrl.length+1] = '0';

    UA_UInt16 portpos = 9;
    UA_UInt16 port = 0;
    for(;portpos < endpointUrl.length; portpos++) {
        if(endpointUrl.data[portpos] == ':') {
            port = atoi(&cstringEndpointUrl[portpos+1]);
            break;
        }
    }
    if(port == 0) {
        printf("port invalid");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    char hostname[512];
    for(int i=10; i < portpos; i++)
        hostname[i-10] = endpointUrl.data[i];
    hostname[portpos-10] = 0;

#ifdef _WIN32
    UA_UInt32 sock = 0;
#else
    UA_Int32 sock = 0;
#endif
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
    if((sock = socket(PF_INET, SOCK_STREAM,0)) == INVALID_SOCKET) {
#else
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#endif
		printf("Could not create socket\n");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    struct hostent *server;
    server = gethostbyname(hostname);
    if (server == NULL) {
        printf("DNS lookup of %s failed\n", hostname);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

	struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    memcpy((char *)&server_addr.sin_addr.s_addr,
    	 (char *)server->h_addr_list[0],
         server->h_length);

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if(connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("Connect failed.\n");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    //if(setNonBlocking(*sock) != UA_STATUSCODE_GOOD) {
    //    printf("Could not switch to nonblocking.\n");
    //	FINALLY
    //    return UA_STATUSCODE_BADINTERNALERROR;
    //}
    resultHandle->sockfd = sock;
    return UA_STATUSCODE_GOOD;
}

static void ClientNetworkLayerTCP_disconnect(ClientNetworkLayerTCP* handle) {
	CLOSESOCKET(handle->sockfd);
#ifdef _WIN32
	WSACleanup();
#endif
}

static UA_StatusCode ClientNetworkLayerTCP_send(ClientNetworkLayerTCP *handle, UA_ByteStringArray gather_buf) {
	UA_UInt32 total_len = 0, nWritten = 0;
#ifdef _WIN32
	LPWSABUF buf = _alloca(gather_buf.stringsSize * sizeof(WSABUF));
	int result = 0;
	for(UA_UInt32 i = 0; i<gather_buf.stringsSize; i++) {
		buf[i].buf = (char*)gather_buf.strings[i].data;
		buf[i].len = gather_buf.strings[i].length;
		total_len += gather_buf.strings[i].length;
	}
	while(nWritten < total_len) {
		UA_UInt32 n = 0;
		do {
			result = WSASend(handle->sockfd, buf, gather_buf.stringsSize ,
                             (LPDWORD)&n, 0, NULL, NULL);
			if(result != 0)
				printf("Error WSASend, code: %d \n", WSAGetLastError());
		} while(errno == EINTR);
		nWritten += n;
	}
#else
	struct iovec iov[gather_buf.stringsSize];
	for(UA_UInt32 i=0;i<gather_buf.stringsSize;i++) {
		iov[i] = (struct iovec) {.iov_base = gather_buf.strings[i].data,
                                 .iov_len = gather_buf.strings[i].length};
		total_len += gather_buf.strings[i].length;
	}
	struct msghdr message = {.msg_name = NULL, .msg_namelen = 0, .msg_iov = iov,
							 .msg_iovlen = gather_buf.stringsSize, .msg_control = NULL,
							 .msg_controllen = 0, .msg_flags = 0};
	while (nWritten < total_len) {
        int n = sendmsg(handle->sockfd, &message, 0);
        if(n <= -1)
            return UA_STATUSCODE_BADINTERNALERROR;
        nWritten += n;
	}
#endif
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
ClientNetworkLayerTCP_awaitResponse(ClientNetworkLayerTCP *handle, UA_ByteString *response,
                                    UA_UInt32 timeout) {
    struct timeval tmptv = {0, timeout};
    setsockopt(handle->sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tmptv,sizeof(struct timeval));
    int ret = recv(handle->sockfd, (char*)response->data, response->length, 0);
    if(ret <= -1)
    	return UA_STATUSCODE_BADINTERNALERROR;
    if(ret == 0)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;
    response->length = ret;
    return UA_STATUSCODE_GOOD;
}

static void ClientNetworkLayerTCP_delete(ClientNetworkLayerTCP *layer) {
	if(layer)
		free(layer);
}

UA_ClientNetworkLayer ClientNetworkLayerTCP_new(UA_ConnectionConfig conf) {
	ClientNetworkLayerTCP *tcplayer = malloc(sizeof(ClientNetworkLayerTCP));
	tcplayer->sockfd = 0;

    UA_ClientNetworkLayer layer;
    layer.nlHandle = tcplayer;
    layer.connect = (UA_StatusCode (*)(const UA_String, void**)) ClientNetworkLayerTCP_connect;
    layer.disconnect = (void (*)(void*)) ClientNetworkLayerTCP_disconnect;
    layer.delete = (void (*)(void*)) ClientNetworkLayerTCP_delete;
    layer.send = (UA_StatusCode (*)(void*, UA_ByteStringArray)) ClientNetworkLayerTCP_send;
    layer.awaitResponse = (UA_StatusCode (*)(void*, UA_ByteString *, UA_UInt32))ClientNetworkLayerTCP_awaitResponse;
    return layer;
}
