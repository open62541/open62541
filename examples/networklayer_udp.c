 /*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <stdlib.h> // malloc, free
#ifdef _WIN32
#include <malloc.h>
#include <winsock2.h>
#include <sys/types.h>
#include <Windows.h>
#include <ws2tcpip.h>
#define CLOSESOCKET(S) closesocket(S)
#else
#include <strings.h> //bzero
#include <sys/select.h> 
#include <netinet/in.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <unistd.h> // read, write, close
#include <arpa/inet.h>
#define CLOSESOCKET(S) close(S)
#endif

#include <stdio.h>
#include <errno.h> // errno, EINTR
#include <fcntl.h> // fcntl

#include "networklayer_udp.h" // UA_MULTITHREADING is defined in here

#ifdef UA_MULTITHREADING
#include <urcu/uatomic.h>
#endif

#define MAXBACKLOG 100

struct Networklayer_UDP;

/* Forwarded to the server as a (UA_Connection) and used for callbacks back into
   the networklayer */
typedef struct {
	UA_Connection connection;
	struct sockaddr from;
	socklen_t fromlen;
	struct NetworkLayerUDP *layer;
} UDPConnection;

typedef struct NetworkLayerUDP {
	UA_ConnectionConfig conf;
	fd_set fdset;
	UA_Int32 serversockfd;
    UA_UInt32 port;
    /* We remove the connection links only in the main thread. Attach
       to-be-deleted links with atomic operations */
    struct deleteLink {
        UA_Int32 sockfd;
        struct deleteLink *next;
    } *deleteLinkList;
} NetworkLayerUDP;

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

// after every select, reset the set of sockets we want to listen on
static void setFDSet(NetworkLayerUDP *layer) {
	FD_ZERO(&layer->fdset);
	FD_SET(layer->serversockfd, &layer->fdset);
}

// the callbacks are thread-safe if UA_MULTITHREADING is defined
void closeConnectionUDP(UDPConnection *handle){
	free(handle);
}
void writeCallbackUDP(UDPConnection *handle, UA_ByteStringArray gather_buf);

/** Accesses only the sockfd in the handle. Can be run from parallel threads. */
void writeCallbackUDP(UDPConnection *handle, UA_ByteStringArray gather_buf) {
	UA_UInt32 total_len = 0, nWritten = 0;
#ifdef _WIN32
	LPWSABUF buf = _alloca(gather_buf.stringsSize * sizeof(WSABUF));
	int result = 0;
	for(UA_UInt32 i = 0; i<gather_buf.stringsSize; i++) {
		buf[i].buf = gather_buf.strings[i].data;
		buf[i].len = gather_buf.strings[i].length;
		total_len += gather_buf.strings[i].length;
	}
	while(nWritten < total_len) {
		UA_UInt32 n = 0;
		do {
			result = WSASendto(handle->sockfd, buf, gather_buf.stringsSize ,
                             (LPDWORD)&n, 0, NULL, NULL);
			//FIXME:
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


	struct sockaddr_in *sin = UA_NULL;
	if (handle->from.sa_family == AF_INET)
	{
	    sin = (struct sockaddr_in *) &(handle->from);
	}else{
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

UA_StatusCode NetworkLayerUDP_start(NetworkLayerUDP *layer) {
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

    printf("Listening for UDP connections on %s:%d\n",
           inet_ntoa(serv_addr.sin_addr),
           ntohs(serv_addr.sin_port));
    return UA_STATUSCODE_GOOD;
}

UA_Int32 NetworkLayerUDP_getWork(NetworkLayerUDP *layer, UA_WorkItem **workItems,
                                 UA_UInt16 timeout) {
    UA_WorkItem *items = UA_NULL;
    UA_Int32 itemsCount = 0;
    setFDSet(layer);
    struct timeval tmptv = {0, timeout};
    UA_Int32 resultsize = select(layer->serversockfd+1, &layer->fdset, NULL, NULL, &tmptv);

    if(resultsize <= 0) {
        *workItems = items;
        return itemsCount;
    }

    items = malloc(sizeof(UA_WorkItem)*(itemsCount+resultsize));

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
            c->connection.channel = UA_NULL;
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

UA_Int32 NetworkLayerUDP_stop(NetworkLayerUDP * layer, UA_WorkItem **workItems) {
	CLOSESOCKET(layer->serversockfd);
	return 0;
}

void NetworkLayerUDP_delete(NetworkLayerUDP *layer) {
	free(layer);
}

UA_NetworkLayer NetworkLayerUDP_new(UA_ConnectionConfig conf, UA_UInt32 port) {
    NetworkLayerUDP *udplayer = malloc(sizeof(NetworkLayerUDP));
	udplayer->conf = conf;
    udplayer->port = port;
    udplayer->deleteLinkList = UA_NULL;

    UA_NetworkLayer nl;
    nl.nlHandle = udplayer;
    nl.start = (UA_StatusCode (*)(void*))NetworkLayerUDP_start;
    nl.getWork = (UA_Int32 (*)(void*, UA_WorkItem**, UA_UInt16)) NetworkLayerUDP_getWork;
    nl.stop = (UA_Int32 (*)(void*, UA_WorkItem**)) NetworkLayerUDP_stop;
    nl.delete = (void (*)(void*))NetworkLayerUDP_delete;
    return nl;
}
