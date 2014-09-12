/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifdef WIN32
#include <winsock2.h>
#include <sys/types.h>
#include <Windows.h>
#include <ws2tcpip.h>
#define CLOSESOCKET(S) closesocket(S)
#define IOCTLSOCKET ioctlsocket
#else
#include <sys/select.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>
#include <unistd.h> // read, write, close
#define CLOSESOCKET(S) close(S)
#define IOCTLSOCKET ioctl
#endif /* WIN32 */

#include <stdlib.h> // exit
#include <errno.h> // errno, EINTR
#include <memory.h> // memset
#include <fcntl.h> // fcntl

#include "networklayer_tcp.h"
#include "ua_transport.h"

typedef struct TCPConnection {
	UA_Int32 sockfd;
	UA_Connection connection;
} TCPConnection;

struct NetworklayerTCP {
	UA_ConnectionConfig localConf;
	UA_UInt32 port;
	fd_set fdset;
	UA_Int32 serversockfd;
	UA_Int32 highestfd;
	UA_UInt32 connectionsSize;
	TCPConnection *connections;
};

/** This structure is stored in the UA_Connection for callbacks into the
	network layer. */
typedef struct TCPConnectionHandle {
	UA_Int32 sockfd;
	NetworklayerTCP *layer;
} TCPConnectionHandle;

UA_Int32 NetworklayerTCP_new(NetworklayerTCP **newlayer, UA_ConnectionConfig localConf,
							 UA_UInt32 port) {
    UA_UInt32 retval = UA_SUCCESS;
    retval |= UA_alloc((void**)newlayer, sizeof(NetworklayerTCP));
    if(retval != UA_SUCCESS)
        return UA_ERROR;
	(*newlayer)->localConf = localConf;
	(*newlayer)->port = port;
	(*newlayer)->connectionsSize = 0;
	(*newlayer)->connections = UA_NULL;
	return UA_SUCCESS;
}

// copy the array of connections, but _loose_ one. This does not close the
// actual socket.
UA_Int32 NetworklayerTCP_remove(NetworklayerTCP *layer, UA_Int32 sockfd) {
	UA_UInt32 index;
	for(index = 0;index < layer->connectionsSize;index++) {
		if(layer->connections[index].sockfd == sockfd)
			break;
	}
	UA_Connection_deleteMembers(&layer->connections[index].connection);

	TCPConnection *newconnections;
	UA_alloc((void**)&newconnections, sizeof(TCPConnection) * (layer->connectionsSize-1));
	UA_memcpy(newconnections, &layer->connections, sizeof(TCPConnection) * index);
	UA_memcpy(&newconnections[index], &layer->connections[index+1],
			  sizeof(TCPConnection) * (layer->connectionsSize - index - 1));
	layer->connections = newconnections;
	layer->connectionsSize--;
	return UA_SUCCESS;
}

void NetworklayerTCP_delete(NetworklayerTCP *layer) {
	for(UA_UInt32 index = 0;index < layer->connectionsSize;index++) {
		shutdown(layer->connections[index].sockfd, 2);
        UA_Connection_deleteMembers(&layer->connections[index].connection);
		CLOSESOCKET(layer->connections[index].sockfd);
	}
	UA_free(layer->connections);
	UA_free(layer);
}

/** Callback function */
void closeCallback(TCPConnectionHandle *handle) {
	shutdown(handle->sockfd,2);
	CLOSESOCKET(handle->sockfd);
	NetworklayerTCP_remove(handle->layer, handle->sockfd);
}

/** Callback function */
UA_Int32 writeCallback(TCPConnectionHandle *handle, UA_ByteStringArray gather_buf) {
	UA_UInt32 total_len = 0;
	UA_UInt32 nWritten = 0;
#ifdef WIN32
	LPWSABUF buf = malloc(gather_buf.stringsSize * sizeof(WSABUF));
	int result = 0;
	for(UA_UInt32 i = 0; i<gather_buf.stringsSize; i++) {
		buf[i].buf = gather_buf.strings[i].data;
		buf[i].len = gather_buf.strings[i].length;
		total_len += gather_buf.strings[i].length;
	}
	while (nWritten < total_len) {
		UA_UInt32 n=0;
		do {
			result = WSASend(handle->sockfd, buf, gather_len , (LPDWORD)&n, 0, NULL, NULL);
			if(result != 0)
				printf("NL_TCP_Writer - Error WSASend, code: %d \n", WSAGetLastError());
		} while (errno == EINTR);
		nWritten += n;
	}
	free(buf);
#else
	struct iovec iov[gather_buf.stringsSize];
	for(UA_UInt32 i=0;i<gather_buf.stringsSize;i++) {
		iov[i].iov_base = gather_buf.strings[i].data;
		iov[i].iov_len = gather_buf.strings[i].length;
		total_len += gather_buf.strings[i].length;
	}
	struct msghdr message = {.msg_name = UA_NULL, .msg_namelen = 0, .msg_iov = iov,
							 .msg_iovlen = gather_buf.stringsSize, .msg_control = UA_NULL,
							 .msg_controllen = 0, .msg_flags = 0};
	while (nWritten < total_len) {
		UA_Int32 n = 0;
		do {
			n = sendmsg(handle->sockfd, &message, 0);
		} while (n == -1L && errno == EINTR);

		if (n >= 0) {
			// TODO: handle incompletely send messages
			/* nWritten += n; */
			break;
		} else {
			// TODO: error handling
			break;
		}
	}
#endif
	return UA_SUCCESS;
}

UA_Int32 NetworklayerTCP_add(NetworklayerTCP *layer, UA_Int32 newsockfd) {
    layer->connectionsSize++;
	layer->connections = realloc(layer->connections, sizeof(TCPConnection) * layer->connectionsSize);
	TCPConnection *newconnection = &layer->connections[layer->connectionsSize-1];
	newconnection->sockfd = newsockfd;

	struct TCPConnectionHandle *callbackhandle;
	UA_alloc((void**)&callbackhandle, sizeof(struct TCPConnectionHandle));
	callbackhandle->layer = layer;
	callbackhandle->sockfd = newsockfd;
	UA_Connection_init(&newconnection->connection, layer->localConf, callbackhandle,
					   (UA_Connection_closeCallback)closeCallback, (UA_Connection_writeCallback)writeCallback);
	return UA_SUCCESS;
}

UA_Int32 setNonBlocking(int sockid) {
#ifdef WIN32
	u_long iMode = 1;
	int opts = IOCTLSOCKET(sockid, FIONBIO, &iMode);
	if (opts != NO_ERROR){
		printf("ioctlsocket failed with error: %ld\n", opts);
		return - 1;
	}
	return 0;
#else
	int opts = fcntl(sockid,F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		return -1;
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sockid,F_SETFL,opts) < 0) {
		perror("fcntl(F_SETFL)");
		return -1;
	}
	return 0;
#endif
}

void readConnection(NetworklayerTCP *layer, UA_Server *server, TCPConnection *entry) {
	UA_ByteString readBuffer;
	UA_alloc((void**)&readBuffer.data, layer->localConf.recvBufferSize);
#ifdef WIN32
	readBuffer.length = recv(entry->sockfd, (char *)readBuffer.data,
							 layer->localConf.recvBufferSize, 0);
#else
	readBuffer.length = read(entry->sockfd, readBuffer.data, layer->localConf.recvBufferSize);
#endif
	if (errno != 0) {
		shutdown(entry->sockfd,2);
		CLOSESOCKET(entry->sockfd);
		NetworklayerTCP_remove(layer, entry->sockfd);
	} else {
		UA_Server_processBinaryMessage(server, &entry->connection, &readBuffer);
	}
	UA_ByteString_deleteMembers(&readBuffer);
}

void worksocks(NetworklayerTCP *layer, UA_Server *server, UA_UInt32 workamount) {
	// accept new connections
	if(FD_ISSET(layer->serversockfd,&layer->fdset)) {
		struct sockaddr_in cli_addr;
		socklen_t cli_len = sizeof(cli_addr);
		int newsockfd = accept(layer->serversockfd, (struct sockaddr *) &cli_addr, &cli_len);
		if (newsockfd >= 0) {
			setNonBlocking(newsockfd);
			NetworklayerTCP_add(layer, newsockfd);
		}
		workamount--;
	}

	// read from established sockets
	for(UA_UInt32 i=0;i<layer->connectionsSize && workamount > 0;i++) {
		if(FD_ISSET(layer->connections[i].sockfd, &layer->fdset)) {
			readConnection(layer, server, &layer->connections[i]);
			workamount--;
		}
	}
}

// after every select, reset the set of sockets we want to listen on
void setFDSet(NetworklayerTCP *layer) {
	FD_ZERO(&layer->fdset);
	FD_SET(layer->serversockfd, &layer->fdset);
	layer->highestfd = layer->serversockfd;
	for(UA_UInt32 i=0;i<layer->connectionsSize;i++) {
		FD_SET(layer->connections[i].sockfd, &layer->fdset);
		if(layer->connections[i].sockfd > layer->highestfd)
			layer->highestfd = layer->connections[i].sockfd;
	}
    layer->highestfd++;
}

UA_Int32 NetworkLayerTCP_run(NetworklayerTCP *layer, UA_Server *server, struct timeval tv,
							   void(*worker)(UA_Server*), UA_Boolean *running) {
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
	if((layer->serversockfd = socket(PF_INET, SOCK_STREAM,0)) == INVALID_SOCKET) {
		printf("ERROR opening socket, code: %d\n", WSAGetLastError());
		return UA_ERROR;
	}
#else
    if((layer->serversockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("ERROR opening socket");
		return UA_ERROR;
	} 
#endif
	struct sockaddr_in serv_addr;
	memset((void *)&serv_addr, sizeof(serv_addr), 1);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(layer->port);

	int optval = 1;
	if(setsockopt(layer->serversockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval)) == -1) {
		perror("setsockopt");
		close(layer->serversockfd);
		return UA_ERROR;
	}
		
	if(bind(layer->serversockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("binding");
		close(layer->serversockfd);
		return UA_ERROR;
	}

#define MAXBACKLOG 10
	setNonBlocking(layer->serversockfd);
	listen(layer->serversockfd, MAXBACKLOG);

	while (*running) {
		setFDSet(layer);
		struct timeval tmptv = tv;
		UA_Int32 resultsize = select(layer->highestfd, &layer->fdset, UA_NULL, UA_NULL, &tmptv);
		if (resultsize <= 0) {
#ifdef WIN32
			UA_Int32 err = (result == SOCKET_ERROR) ? WSAGetLastError() : 0;
			switch (err) {
			case WSANOTINITIALISED:
			case WSAEFAULT:
			case WSAENETDOWN:
			case WSAEINVAL:
			case WSAEINTR:
			case WSAEINPROGRESS:
			case WSAENOTSOCK:
#else
			UA_Int32 err = errno;
			switch (err) {
			case EBADF:
			case EINTR:
			case EINVAL:
#endif
				// todo: handle errors
				printf("UA_Stack_msgLoop - result=%d, errno={%d,%s}\n", resultsize, errno, strerror(errno));
				break;
			default:
				// timeout
				worker(server);
			}
		} else { // activity on listener or client socket
			worksocks(layer, server, resultsize);
			worker(server);
		}
	}
#ifdef WIN32
	WSACleanup();
#endif
	return UA_SUCCESS;
}
