#include "networklayer.h"
#include "ua_transport_connection.h"

#ifdef WIN32
#include <sys/types.h>
#include <Windows.h>
#include <ws2tcpip.h>
#define CLOSESOCKET(S) closesocket(S)
#define IOCTLSOCKET ioctlsocket
#else
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

NL_Description NL_Description_TcpBinary  = {
	NL_UA_ENCODING_BINARY,
	NL_CONNECTIONTYPE_TCPV4,
	NL_MAXCONNECTIONS_DEFAULT,
	{0,8192,8192,16384,1}
};

/* If we do not have multitasking, we implement a dispatcher-Pattern. All Connections
 * are collected in a list. From this list a fd_set is prepared and select then waits
 * for activities. We then iterate over the list, check if we've got some activites
 * and call the corresponding callback (reader, listener).
 */
#ifndef MULTITASKING
_Bool NL_ConnectionComparer(void *p1, void* p2) {
	NL_Connection* c1 = (NL_Connection*) p1;
	NL_Connection* c2 = (NL_Connection*) p2;

	return (c1->connectionHandle == c2->connectionHandle);

}

int NL_TCP_SetNonBlocking(int sock) {
#ifdef WIN32
	u_long iMode = 1;
	int opts = IOCTLSOCKET(sock, FIONBIO, &iMode);
	if (opts != NO_ERROR){
		printf("ioctlsocket failed with error: %ld\n", opts);
		return - 1;
	}
#else
	int opts = fcntl(sock,F_GETFL);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		return -1;
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sock,F_SETFL,opts) < 0) {
		perror("fcntl(F_SETFL)");
		return -1;
	}
#endif
	return 0;
}

void NL_Connection_printf(void* payload) {
  NL_Connection* c = (NL_Connection*) payload;
  printf("ListElement connectionHandle = %d\n",c->connectionHandle);
}
void NL_addHandleToSet(UA_Int32 handle, NL_data* nl) {
	FD_SET(handle, &(nl->readerHandles));
#ifdef WIN32
	// int err = WSAGetLastError();
#endif
	nl->maxReaderHandle = (handle > nl->maxReaderHandle) ? handle : nl->maxReaderHandle;
}
void NL_setFdSet(void* payload) {
  NL_Connection* c = (NL_Connection*) payload;
  NL_addHandleToSet(c->connectionHandle, c->networkLayer);
}
void NL_checkFdSet(void* payload) {
  NL_Connection* c = (NL_Connection*) payload;
  if (FD_ISSET(c->connectionHandle, &(c->networkLayer->readerHandles))) {
	  c->reader((void*)c);
  }
}

#if 0 
char _str_error[256];
char* strerror(int errno) {
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errno,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		&_str_error[0], 256, NULL);
	return &_str_error[0];
}
#endif

UA_Int32 NL_msgLoop(NL_data* nl, struct timeval *tv, UA_Int32(*worker)(void*), void *arg, UA_Boolean *running)  {
	UA_Int32 result;
	UA_Int32 err;
	while (*running) {
		// determine the largest handle
		nl->maxReaderHandle = 0;
		UA_list_iteratePayload(&(nl->connections),NL_setFdSet);
		DBG_VERBOSE(printf("\n------------\nUA_Stack_msgLoop - maxHandle=%d\n", nl->maxReaderHandle));

		// copy tv, some unixes do overwrite and return the remaining time
		// FIXME: actually we might want to do this ourselves to call the
		// worker on a more regular cyclic basis
		struct timeval tmptv;
		memcpy(&tmptv,tv,sizeof(struct timeval));

		// and wait
		DBG_VERBOSE(printf("UA_Stack_msgLoop - enter select sec=%d,usec=%d\n",(UA_Int32) tmptv.tv_sec, (UA_Int32) tmptv.tv_usec));
		result = select(nl->maxReaderHandle + 1, &(nl->readerHandles), UA_NULL, UA_NULL, &tmptv);
		DBG_VERBOSE(printf("UA_Stack_msgLoop - leave select result=%d,sec=%d,usec=%d\n",result, (UA_Int32) tmptv.tv_sec, (UA_Int32) tmptv.tv_usec));

		// handle timeout (winsock: result=0, unix: result=0,errno=0||EAGAIN) 
		// and errors (winsock: result=SOCKET_ERROR (-1), unix: result = 0)
		if (result <= 0) {
#ifdef WIN32
			err = (result == SOCKET_ERROR) ? WSAGetLastError() : 0;
#else
			err = errno;
#endif
			switch (err) {
				// handle known errors
#ifdef WIN32
			case WSANOTINITIALISED:
			case WSAEFAULT:
			case WSAENETDOWN:
			case WSAEINVAL:
			case WSAEINTR:
			case WSAEINPROGRESS:
			case WSAENOTSOCK:
#else
			case EBADF:
			case EINTR:
			case EINVAL:
#endif
				// FIXME: handle errors
				printf("UA_Stack_msgLoop - result=%d, errno={%d,%s}\n", result, errno, strerror(errno));
				break;
			// otherwise we've got a timeout and call the worker
#ifndef WIN32
	        case EAGAIN:
#endif
			default:
					DBG_VERBOSE(printf("UA_Stack_msgLoop - result=%d, errno={%d,%s}\n", result, errno, strerror(errno)));
					worker(arg);
			}
		} else { // activity on listener or client ports
			DBG_VERBOSE(printf("UA_Stack_msgLoop - activities on %d handles\n",result));
			UA_list_iteratePayload(&(nl->connections),NL_checkFdSet);
			// FIXME: Thought it would be a conceptional flaw to call the worker
			// here. However, there is no guarantee that the timeout would be
			// triggered, so we call it in this branch as well.
			worker(arg);
		}
	}
#ifdef WIN32
	// finally we should clean up the winsock.dll
	WSACleanup();
#endif
	return UA_SUCCESS;
}
#endif /* MULTITASKING */


/** the tcp reader function */
void* NL_TCP_reader(NL_Connection *c) {

	UA_ByteString readBuffer;

	TL_Buffer localBuffers;
	UA_Int32 connectionState;

	UA_TL_Connection_getLocalConfig(c->connection, &localBuffers);
	UA_alloc((void**)&(readBuffer.data),localBuffers.recvBufferSize);

	UA_TL_Connection_getState(c->connection, &connectionState);
	if (connectionState  != CONNECTIONSTATE_CLOSE) {
		DBG_VERBOSE(printf("NL_TCP_reader - enter read\n"));

#ifdef WIN32
		readBuffer.length = recv(c->connectionHandle, (char *)readBuffer.data, localBuffers.recvBufferSize, 0);
#else
		readBuffer.length = read(c->connectionHandle, readBuffer.data, localBuffers.recvBufferSize);
#endif
		DBG_VERBOSE(printf("NL_TCP_reader - leave read\n"));

		DBG_VERBOSE(printf("NL_TCP_reader - src={%*.s}, ",c->connection.remoteEndpointUrl.length,c->connection.remoteEndpointUrl.data));
		DBG(UA_ByteString_printx("NL_TCP_reader - received=",&readBuffer));

		if (errno != 0) {
			perror("NL_TCP_reader - ERROR reading from socket1");
			UA_TL_Connection_setState(c->connection, CONNECTIONSTATE_CLOSE);
		} else if (readBuffer.length  > 0) {
#ifdef DEBUG
#include "ua_transport_binary_secure.h"
			UA_UInt32 pos = 0;
			UA_OPCUATcpMessageHeader header;
			UA_OPCUATcpMessageHeader_decodeBinary(&readBuffer, &pos, &header);
			pos = 24;
			if(header.messageType == UA_MESSAGETYPE_MSG)
			{
				UA_NodeId serviceRequestType;
				UA_NodeId_decodeBinary(&readBuffer, &pos,&serviceRequestType);
				UA_NodeId_printf("NL_TCP_reader - Service Type\n",&serviceRequestType);
			}
#endif
			TL_Process((c->connection),&readBuffer);
		} else {
			perror("NL_TCP_reader - ERROR reading from socket");
			UA_TL_Connection_setState(c->connection, CONNECTIONSTATE_CLOSE);
		}
	}

	UA_TL_Connection_getState(c->connection, &connectionState);
	DBG_VERBOSE(printf("NL_TCP_reader - connectionState=%d\n",connectionState));
	if (connectionState == CONNECTIONSTATE_CLOSE) {
		// set connection's state to CONNECTIONSTATE_CLOSED and call callback to actually close
		UA_TL_Connection_close(c->connection);
#ifndef MULTITHREADING
		DBG_VERBOSE(printf("NL_TCP_reader - search element to remove\n"));
		UA_list_Element* lec = UA_list_search(&(c->networkLayer->connections),NL_ConnectionComparer,c);
		DBG_VERBOSE(printf("NL_TCP_reader - remove connection for handle=%d\n",((NL_Connection*)lec->payload)->connection.connectionHandle));
		UA_list_removeElement(lec,UA_NULL);
		DBG_VERBOSE(UA_list_iteratePayload(&(c->networkLayer->connections),NL_Connection_printf));
		UA_free(c);
#endif
	}
	UA_ByteString_deleteMembers(&readBuffer);
	return UA_NULL;
}

#ifdef MULTITHREADING
/** the tcp reader thread */
void* NL_TCP_readerThread(NL_Connection *c) {
	// just loop, NL_TCP_Reader will call the stack
	UA_Int32 connectionState;
	do {
		NL_TCP_reader(c);
		UA_TL_Connection_getState(c->connection, &connectionState);
	} while (connectionState != CONNECTIONSTATE_CLOSED);
	// clean up
	UA_free(c);
	pthread_exit(UA_NULL);
}
#endif

/** write message provided in the gather buffers to a tcp transport layer connection */
UA_Int32 NL_TCP_writer(UA_Int32 connectionHandle, UA_ByteString const * const * gather_buf, UA_UInt32 gather_len) {

	UA_UInt32 total_len = 0;
#ifdef WIN32
	WSABUF *buf = malloc(gather_len * sizeof(WSABUF));
	int result = 0;
	for (UA_UInt32 i = 0; i<gather_len; i++) {
		buf[i].buf = (char*)gather_buf[i]->data;
		buf[i].len = gather_buf[i]->length;
		total_len += gather_buf[i]->length;
		//		DBG(printf("NL_TCP_writer - gather_buf[%i]",i));
		//		DBG(UA_ByteString_printx("=", gather_buf[i]));
	}
#else
	struct iovec iov[gather_len];
	for(UA_UInt32 i=0;i<gather_len;i++) {
		iov[i].iov_base = gather_buf[i]->data;
		iov[i].iov_len = gather_buf[i]->length;
		total_len += gather_buf[i]->length;
//		DBG(printf("NL_TCP_writer - gather_buf[%i]",i));
//		DBG(UA_ByteString_printx("=", gather_buf[i]));
	}
	struct msghdr message;
	message.msg_name = UA_NULL;
	message.msg_namelen = 0;
	message.msg_iov = iov;
	message.msg_iovlen = gather_len;
	message.msg_control = UA_NULL;
	message.msg_controllen = 0;
	message.msg_flags = 0;
#endif


	UA_UInt32 nWritten = 0;
#ifdef WIN32
	while (nWritten < total_len) {
		UA_UInt32 n=0;
		do {
			DBG_VERBOSE(printf("NL_TCP_writer - enter write with %d bytes to write\n",total_len));

			//result = WSASendMsg(connectionHandle,&message,0,&n,UA_NULL,UA_NULL);
			result = WSASend(connectionHandle, buf, gather_len , (LPDWORD)&n, 0, NULL, NULL);
			if(result != 0)
				printf("NL_TCP_Writer - Error WSASend, code: %d \n", WSAGetLastError());
			DBG_VERBOSE(printf("NL_TCP_writer - leave write with n=%d,errno={%d,%s}\n",n,(n>0)?0:errno,(n>0)?"":strerror(errno)));
		} while (errno == EINTR);
		nWritten += n;
#else
	while (nWritten < total_len) {
		UA_Int32 n=0;
		do {
			DBG_VERBOSE(printf("NL_TCP_writer - enter write with %d bytes to write\n",total_len));
			n = sendmsg(connectionHandle, &message, 0);
			DBG_VERBOSE(printf("NL_TCP_writer - leave write with n=%d,errno={%d,%s}\n",n,(n>0)?0:errno,(n>0)?"":strerror(errno)));
		} while (n == -1L && errno == EINTR);

		if (n >= 0) {
			nWritten += n;
			break;
			// TODO: handle incompletely send messages
		} else {
			// TODO: error handling
			break;
		}
#endif
	}
#ifdef WIN32
	free(buf);
#endif
	return UA_SUCCESS;
}
//callback function which is called when the UA_TL_Connection_close() function is initiated
UA_Int32 NL_Connection_close(UA_TL_Connection *connection)
{
	NL_Connection *networkLayerData = UA_NULL;
	UA_TL_Connection_getNetworkLayerData(connection, (void**)&networkLayerData);
	if(networkLayerData != UA_NULL){
		DBG_VERBOSE(printf("NL_Connection_close - enter shutdown\n"));
		shutdown(networkLayerData->connectionHandle,2);
		DBG_VERBOSE(printf("NL_Connection_close - enter close\n"));
		CLOSESOCKET(networkLayerData->connectionHandle);
		FD_CLR(networkLayerData->connectionHandle, &networkLayerData->networkLayer->readerHandles);
		DBG_VERBOSE(printf("NL_Connection_close - leave close\n"));
		return UA_SUCCESS;
	}
    DBG_VERBOSE(printf("NL_Connection_close - ERROR: connection object invalid \n"));
	return UA_ERROR;
}
void* NL_Connection_init(NL_Connection* c, NL_data* tld, UA_Int32 connectionHandle, NL_Reader reader, TL_Writer writer)
{
	UA_TL_Connection *connection = UA_NULL;
	//create new connection object
	UA_TL_Connection_new(&connection, tld->tld->localConf, writer, NL_Connection_close,connectionHandle,c);

	c->connection = connection;
	c->connectionHandle = connectionHandle;
	// network layer
	c->reader = reader;
#ifdef MULTITHREADING
	c->readerThreadHandle = -1;
#endif
	c->networkLayer = tld;
	return UA_NULL;
}

/** the tcp accept routine */
void* NL_TCP_accept(NL_Connection* c) {
	NL_data* tld = c->networkLayer;

	if (tld->tld->maxConnections == -1 || tld->connections.size < tld->tld->maxConnections) {
		// accept only if not max number of connections exceeded
		struct sockaddr_in cli_addr;
		socklen_t cli_len = sizeof(cli_addr);
		DBG_VERBOSE(printf("NL_TCP_listen - enter accept\n"));
		int newsockfd = accept(c->connectionHandle, (struct sockaddr *) &cli_addr, &cli_len);
		DBG_VERBOSE(printf("NL_TCP_listen - leave accept\n"));
		if (newsockfd < 0) {
			DBG_ERR(printf("TL_TCP_listen - accept returns errno={%d,%s}\n",errno,strerror(errno)));
			perror("ERROR on accept");
		} else {
			DBG_VERBOSE(printf("NL_TCP_listen - new connection on %d\n",newsockfd));
			NL_Connection* cclient;
			UA_Int32 retval = UA_SUCCESS;
			retval |= UA_alloc((void**)&cclient,sizeof(NL_Connection));
			NL_Connection_init(cclient, tld, newsockfd, NL_TCP_reader, (TL_Writer) NL_TCP_writer);
#ifdef MULTITHREADING
			pthread_create( &(cclient->readerThreadHandle), NULL, (void*(*)(void*)) NL_TCP_readerThread, (void*) cclient);
#else
			UA_list_addPayloadToBack(&(tld->connections),cclient);
			NL_TCP_SetNonBlocking(cclient->connectionHandle);
#endif
		}
	} else {
		// no action necessary to reject connection
	}
	return UA_NULL;
}

#ifdef MULTITHREADING
void* NL_TCP_listenThread(NL_Connection* c) {
	NL_data* tld = c->networkLayer;

	DBG_VERBOSE(printf("NL_TCP_listenThread - enter listen\n"));
	int retval = listen(c->connectionHandle, tld->tld->maxConnections);
	DBG_VERBOSE(printf("NL_TCP_listenThread - leave listen, retval=%d\n", retval));
	if (retval < 0) {
		// TODO: Error handling
		perror("NL_TCP_listen");
		DBG_ERR(printf("NL_TCP_listen retval=%d, errno={%d,%s}\n", retval, errno, strerror(errno)));
	} else {
		do {
			NL_TCP_accept(c);
		}
	} while (UA_TRUE);
	UA_free(c);
	pthread_exit(UA_NULL);
}
#endif


UA_Int32 NL_TCP_init(NL_data* tld, UA_Int32 port) {
	UA_Int32 retval = UA_SUCCESS;
	// socket variables
#ifdef WIN32
	unsigned int newsockfd;
#else
	int newsockfd;
#endif

	struct sockaddr_in serv_addr;


	// create socket for listening to incoming connections
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	WSAStartup(wVersionRequested, &wsaData);
	newsockfd = socket(PF_INET, SOCK_STREAM,0);
	if (newsockfd == INVALID_SOCKET){
		//UA_Int32 lasterror = WSAGetLastError();
		printf("ERROR opening socket, code: %d\n",WSAGetLastError());
#else
	newsockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (newsockfd < 0) {
#endif
		perror("ERROR opening socket");
		retval = UA_ERROR;
	}
	else {
		// set port number, options and bind
		memset((void *)&serv_addr, sizeof(serv_addr), 1);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);

#ifdef WIN32
		char optval = 1;
		if (setsockopt(newsockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1) {
			perror("setsockopt");
			retval = UA_ERROR;
		}
		else {
#endif
			// bind to port
			if (bind(newsockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
				perror("ERROR on binding");
				retval = UA_ERROR;
			}
			else {
				UA_String_copyprintf("opc.tcp://localhost:%d/", &(tld->endpointUrl), port);
			}
#ifdef WIN32
		}
#endif
	}
	// finally
	if (retval == UA_SUCCESS) {
		DBG_VERBOSE(printf("NL_TCP_init - new listener on %d\n",newsockfd));
		NL_Connection* c;
		UA_Int32 retval = UA_SUCCESS;
		retval |= UA_alloc((void**)&c,sizeof(NL_Connection));
		NL_Connection_init(c, tld, newsockfd, NL_TCP_accept, (TL_Writer) NL_TCP_writer);
#ifdef MULTITHREADING
		pthread_create( &(c->readerThreadHandle), NULL, (void*(*)(void*)) NL_TCP_listenThread, (void*) c);
#else
		UA_list_addPayloadToBack(&(tld->connections),c);
		NL_TCP_SetNonBlocking(c->connectionHandle);
		listen(c->connectionHandle, tld->tld->maxConnections);
#endif
	}
	return retval;
}


/** checks arguments and dispatches to worker or refuses to init */
NL_data* NL_init(NL_Description* tlDesc, UA_Int32 port) {
	NL_data* nl = UA_NULL;
	if (tlDesc->connectionType == NL_CONNECTIONTYPE_TCPV4 && tlDesc->encoding == NL_UA_ENCODING_BINARY) {
		UA_alloc((void**)&nl, sizeof(NL_data));
		nl->tld = tlDesc;
		FD_ZERO(&(nl->readerHandles));
		UA_list_init(&(nl->connections));
		NL_TCP_init(nl, port);
	}
	return nl;
}
