 /*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#define _XOPEN_SOURCE 500 //some users need this for some reason
#define __USE_BSD
#include <stdlib.h> // malloc, free
#include <stdio.h>
#include <string.h> // memset
#include "networklayer_udp.h"
#ifdef UA_MULTITHREADING
# include <urcu/uatomic.h>
#endif

/* with a space so amalgamation does not remove the includes */
# include <errno.h> // errno, EINTR
# include <fcntl.h> // fcntl

#ifdef _WIN32
# include <malloc.h>
# include <winsock2.h>
# include <sys/types.h>
# include <windows.h>
# include <ws2tcpip.h>
# define CLOSESOCKET(S) closesocket(S)
#else
# include <strings.h> //bzero
# include <sys/select.h> 
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <sys/socketvar.h>
# include <sys/ioctl.h>
# include <unistd.h> // read, write, close
# include <arpa/inet.h>
# define CLOSESOCKET(S) close(S)
#endif

#define MAXBACKLOG 100

struct ServerNetworklayerUDP;

/* Forwarded to the server as a (UA_Connection) and used for callbacks back into
   the networklayer */
typedef struct {
	UA_Connection connection;
	struct sockaddr from;
	socklen_t fromlen;
	struct ServerNetworkLayerUDP *layer;
} UDPConnection;

typedef struct ServerNetworkLayerUDP {
	UA_ConnectionConfig conf;
	fd_set fdset;
#ifdef _WIN32
	UA_UInt32 serversockfd;
#else
	UA_Int32 serversockfd;
#endif
    UA_UInt32 port;
} ServerNetworkLayerUDP;

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

static void setFDSet(ServerNetworkLayerUDP *layer) {
	FD_ZERO(&layer->fdset);
	FD_SET(layer->serversockfd, &layer->fdset);
}

// the callbacks are thread-safe if UA_MULTITHREADING is defined
static void closeConnectionUDP(UDPConnection *handle) {
	free(handle);
}

void writeCallbackUDP(UDPConnection *handle, UA_ByteStringArray gather_buf);

/** Accesses only the sockfd in the handle. Can be run from parallel threads. */
void writeCallbackUDP(UDPConnection *handle, UA_ByteStringArray gather_buf) {
	UA_UInt32 total_len = 0, nWritten = 0;
#ifdef _WIN32
	/*
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
			result = WSASendto(handle->layer->serversockfd, buf, gather_buf.stringsSize ,
                             (LPDWORD)&n, 0, 
                             handle->from, handle->fromlen,
                             NULL, NULL);
			//FIXME:
			if(result != 0)
				printf("Error WSASend, code: %d \n", WSAGetLastError());
		} while(errno == EINTR);
		nWritten += n;
	}
	*/
	#error fixme: udp not yet implemented for windows
#else
	struct iovec iov[gather_buf.stringsSize];
	for(UA_UInt32 i=0;i<gather_buf.stringsSize;i++) {
		iov[i] = (struct iovec) {.iov_base = gather_buf.strings[i].data,
                                 .iov_len = gather_buf.strings[i].length};
		total_len += gather_buf.strings[i].length;
	}


	struct sockaddr_in *sin = NULL;
	if (handle->from.sa_family == AF_INET) {
        
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#endif
	    sin = (struct sockaddr_in *) &(handle->from);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

	} else {
		//FIXME:
		return;
	}

	struct msghdr message = {.msg_name = sin, .msg_namelen = handle->fromlen, .msg_iov = iov,
							 .msg_iovlen = gather_buf.stringsSize, .msg_control = NULL,
							 .msg_controllen = 0, .msg_flags = 0};
	while (nWritten < total_len) {
		UA_Int32 n = 0;
		do {
            n = sendmsg(handle->layer->serversockfd, &message, 0);
            if(n==-1L){
            	printf("ERROR:%i\n", errno);
            }
        } while (n == -1L && errno == EINTR);
        nWritten += n;
	}
#endif
}

static UA_StatusCode ServerNetworkLayerUDP_start(ServerNetworkLayerUDP *layer, UA_Logger *logger) {
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
	if((layer->serversockfd = socket(PF_INET, SOCK_DGRAM,0)) == INVALID_SOCKET) {
		printf("ERROR opening socket, code: %d\n", WSAGetLastError());
		return UA_STATUSCODE_BADINTERNALERROR;
	}
#else
    if((layer->serversockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
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
    char msg[256];
    sprintf(msg, "Listening for UDP connections on %s:%d",
            inet_ntoa(serv_addr.sin_addr),
            ntohs(serv_addr.sin_port));
    UA_LOG_INFO((*logger), UA_LOGGERCATEGORY_SERVER, msg);
    return UA_STATUSCODE_GOOD;
}

static UA_Int32 ServerNetworkLayerUDP_getWork(ServerNetworkLayerUDP *layer, UA_WorkItem **workItems,
                                        UA_UInt16 timeout) {
    UA_WorkItem *items = NULL;
    setFDSet(layer);
    struct timeval tmptv = {0, timeout};
    UA_Int32 resultsize = select(layer->serversockfd+1, &layer->fdset, NULL, NULL, &tmptv);

    if(resultsize <= 0 || !FD_ISSET(layer->serversockfd, &layer->fdset)) {
        *workItems = items;
        return 0;
    }

    items = malloc(sizeof(UA_WorkItem)*(resultsize));

	// read from established sockets
    UA_Int32 j = 0;
	UA_ByteString buf = { -1, NULL};
		if(!buf.data) {
			buf.data = malloc(sizeof(UA_Byte) * layer->conf.recvBufferSize);
			if(!buf.data){
				//TODO:
				printf("malloc failed");
			}
		}

		struct sockaddr sender;
		socklen_t sendsize = sizeof(sender);
		bzero(&sender, sizeof(sender));

#ifdef _WIN32
        buf.length = recvfrom(layer->conLinks[i].sockfd, (char *)buf.data,
                          layer->conf.recvBufferSize, 0);
        //todo: fixme
#else
        buf.length = recvfrom(layer->serversockfd, buf.data, layer->conf.recvBufferSize, 0, &sender, &sendsize);
#endif

        if (buf.length <= 0) {
        } else {
            UDPConnection *c = malloc(sizeof(UDPConnection));

        	if(!c)
        		return UA_STATUSCODE_BADINTERNALERROR;
            c->layer = layer;
            c->from = sender;
            c->fromlen = sendsize;
            c->connection.state = UA_CONNECTION_OPENING;
            c->connection.localConf = layer->conf;
            c->connection.channel = NULL;
            c->connection.close = (void (*)(void*))closeConnectionUDP;
            c->connection.write = (void (*)(void*, UA_ByteStringArray))writeCallbackUDP;


            items[j].type = UA_WORKITEMTYPE_BINARYNETWORKMESSAGE;
            items[j].work.binaryNetworkMessage.message = buf;
            items[j].work.binaryNetworkMessage.connection = (UA_Connection*)c;
            buf.data = NULL;
            j++;
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

static UA_Int32 ServerNetworkLayerUDP_stop(ServerNetworkLayerUDP * layer, UA_WorkItem **workItems) {
	CLOSESOCKET(layer->serversockfd);
	return 0;
}

static void ServerNetworkLayerUDP_delete(ServerNetworkLayerUDP *layer) {
	free(layer);
}

UA_ServerNetworkLayer ServerNetworkLayerUDP_new(UA_ConnectionConfig conf, UA_UInt32 port){
    ServerNetworkLayerUDP *udplayer = malloc(sizeof(ServerNetworkLayerUDP));
	udplayer->conf = conf;
    udplayer->port = port;

    UA_ServerNetworkLayer nl;
    nl.nlHandle = udplayer;
    nl.start = (UA_StatusCode (*)(void*))ServerNetworkLayerUDP_start;
    nl.getWork = (UA_Int32 (*)(void*, UA_WorkItem**, UA_UInt16)) ServerNetworkLayerUDP_getWork;
    nl.stop = (UA_Int32 (*)(void*, UA_WorkItem**)) ServerNetworkLayerUDP_stop;
    nl.free = (void (*)(void*))ServerNetworkLayerUDP_delete;
    nl.discoveryUrl = NULL;
    return nl;
}
