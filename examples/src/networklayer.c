#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>

#include <unistd.h> // read, write, close
#include <stdlib.h> // exit
#include <errno.h> // errno, EINTR

#include <memory.h> // memset
#include <fcntl.h> // fcntl

#include "networklayer.h"

NL_Description NL_Description_TcpBinary  = {
	NL_UA_ENCODING_BINARY,
	NL_CONNECTIONTYPE_TCPV4,
	NL_MAXCONNECTIONS_DEFAULT,
	{-1,8192,8192,16384,1}
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
	return 0;
}

void NL_Connection_printf(void* payload) {
  UA_UInt32 id;
  NL_Connection* c = (NL_Connection*) payload;
  UA_TL_Connection_getId(c->connection,&id);
  printf("ListElement connectionHandle = %d\n",id);
}
void NL_addHandleToSet(UA_Int32 handle, NL_data* nl) {
	FD_SET(handle, &(nl->readerHandles));
	nl->maxReaderHandle = (handle > nl->maxReaderHandle) ? handle : nl->maxReaderHandle;
}
void NL_setFdSet(void* payload) {
  UA_UInt32 id;
  NL_Connection* c = (NL_Connection*) payload;
  UA_TL_Connection_getId(c->connection,&id);
  NL_addHandleToSet(id, c->networkLayer);
}
void NL_checkFdSet(void* payload) {
	  UA_UInt32 id;
  NL_Connection* c = (NL_Connection*) payload;
  UA_TL_Connection_getId(c->connection,&id);
  if (FD_ISSET(id, &(c->networkLayer->readerHandles))) {
	  c->reader((void*)c);
  }
}
UA_Int32 NL_msgLoop(NL_data* nl, struct timeval *tv, UA_Int32(*worker)(void*), void *arg)  {
	UA_Int32 result;
	while (UA_TRUE) {
		// determine the largest handle
		nl->maxReaderHandle = 0;
		UA_list_iteratePayload(&(nl->connections),NL_setFdSet);
		DBG_VERBOSE(printf("\n------------\nUA_Stack_msgLoop - maxHandle=%d\n", nl->maxReaderHandle));

		// copy tv, some unixes do overwrite and return the remaining time
		struct timeval tmptv;
		memcpy(&tmptv,tv,sizeof(struct timeval));

		// and wait
		DBG_VERBOSE(printf("UA_Stack_msgLoop - enter select sec=%d,usec=%d\n",(UA_Int32) tmptv.tv_sec, (UA_Int32) tmptv.tv_usec));
		result = select(nl->maxReaderHandle + 1, &(nl->readerHandles), UA_NULL, UA_NULL,&tmptv);
		DBG_VERBOSE(printf("UA_Stack_msgLoop - leave select result=%d,sec=%d,usec=%d\n",result, (UA_Int32) tmptv.tv_sec, (UA_Int32) tmptv.tv_usec));
		if (result == 0) {
			int err = errno;
			switch (err) {
			case EBADF:
			case EINTR:
			case EINVAL:
				//FIXME: handle errors
				DBG_ERR(printf("UA_Stack_msgLoop - errno={%d,%s}\n", errno, strerror(errno)));
				break;
			case EAGAIN:
			default:
				DBG_VERBOSE(printf("UA_Stack_msgLoop - errno={%d,%s}\n", errno, strerror(errno)));
				DBG_VERBOSE(printf("UA_Stack_msgLoop - call worker\n"));
				worker(arg);
				DBG_VERBOSE(printf("UA_Stack_msgLoop - return from worker\n"));
			}
		} else { // activity on listener or client ports
			DBG_VERBOSE(printf("UA_Stack_msgLoop - activities on %d handles\n",result));
			UA_list_iteratePayload(&(nl->connections),NL_checkFdSet);
		}
	}
	return UA_SUCCESS;
}
#endif


/** the tcp reader function */
void* NL_TCP_reader(NL_Connection *c) {

	UA_ByteString readBuffer;

	TL_Buffer localBuffers;
	UA_UInt32 connectionId;
	UA_TL_Connection_getLocalConfiguration(c->connection, &localBuffers);
	UA_TL_Connection_getId(c->connection, &connectionId);
	UA_alloc((void**)&(readBuffer.data),localBuffers.recvBufferSize);


	if (c->state  != CONNECTIONSTATE_CLOSE) {
		DBG_VERBOSE(printf("NL_TCP_reader - enter read\n"));
		readBuffer.length = read(connectionId, readBuffer.data, localBuffers.recvBufferSize);
		DBG_VERBOSE(printf("NL_TCP_reader - leave read\n"));

		DBG_VERBOSE(printf("NL_TCP_reader - src={%*.s}, ",c->connection.remoteEndpointUrl.length,c->connection.remoteEndpointUrl.data));
		DBG(UA_ByteString_printx("NL_TCP_reader - received=",&readBuffer));

		if (readBuffer.length  > 0) {

			TL_Process((c->connection),&readBuffer);
		} else {
//TODO close connection - what does close do?
			c->state = CONNECTIONSTATE_CLOSE;
			//c->connection.connectionState = CONNECTIONSTATE_CLOSE;
			perror("ERROR reading from socket1");
		}
	}

	if (c->state == CONNECTIONSTATE_CLOSE) {
		DBG_VERBOSE(printf("NL_TCP_reader - enter shutdown\n"));
		shutdown(connectionId,2);
		DBG_VERBOSE(printf("NL_TCP_reader - enter close\n"));
		close(connectionId);
		DBG_VERBOSE(printf("NL_TCP_reader - leave close\n"));
		c->state  = CONNECTIONSTATE_CLOSED;

		UA_ByteString_deleteMembers(&readBuffer);

#ifndef MULTITHREADING
		DBG_VERBOSE(printf("NL_TCP_reader - search element to remove\n"));
		UA_list_Element* lec = UA_list_search(&(c->networkLayer->connections),NL_ConnectionComparer,c);
		DBG_VERBOSE(printf("NL_TCP_reader - remove connection for handle=%d\n",((NL_Connection*)lec->payload)->connection.connectionHandle));
		UA_list_removeElement(lec,UA_NULL);
		DBG_VERBOSE(UA_list_iteratePayload(&(c->networkLayer->connections),NL_Connection_printf));
		UA_free(c);
#endif
	}
	return UA_NULL;
}

#ifdef MULTITHREADING
/** the tcp reader thread */
void* NL_TCP_readerThread(NL_Connection *c) {
	// just loop, NL_TCP_Reader will call the stack
	do {
		NL_TCP_reader(c);
	} while (c->connection.connectionState != CONNECTIONSTATE_CLOSED);
	// clean up
	UA_free(c);
	pthread_exit(UA_NULL);
}
#endif
/** write message provided in the gather buffers to a tcp transport layer connection */
UA_Int32 NL_TCP_writer(UA_Int32 connectionHandle, UA_ByteString const * const * gather_buf, UA_UInt32 gather_len) {

	struct iovec iov[gather_len];
	UA_UInt32 total_len = 0;
	for(UA_UInt32 i=0;i<gather_len;i++) {
		iov[i].iov_base = gather_buf[i]->data;
		iov[i].iov_len = gather_buf[i]->length;
		total_len += gather_buf[i]->length;
		DBG(printf("NL_TCP_writer - gather_buf[%i]",i));
		DBG(UA_ByteString_printx("=", gather_buf[i]));
	}

	struct msghdr message;
	message.msg_name = UA_NULL;
	message.msg_namelen = 0;
	message.msg_iov = iov;
	message.msg_iovlen = gather_len;
	message.msg_control = UA_NULL;
	message.msg_controllen = 0;
	message.msg_flags = 0;
	
	UA_UInt32 nWritten = 0;
	while (nWritten < total_len) {
		int n=0;
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
			break;
			// TODO: error handling
		}
	}
	return UA_SUCCESS;
}

void* NL_Connection_init(NL_Connection* c, NL_data* tld, UA_Int32 connectionHandle, NL_Reader reader, TL_Writer writer)
{


	UA_TL_Connection1 connection = UA_NULL;
	//create new connection object
	UA_TL_Connection_new(&connection, tld->tld->localConf,writer);
	//add connection object to list, so stack is aware of its connections

	UA_TL_ConnectionManager_addConnection(&connection);

	// connection layer of UA stackwriteLock

	//c->connection.connectionHandle = connectionHandle;
	//c->connection.connectionState = CONNECTIONSTATE_CLOSED;
	//c->connection.writerCallback = writer;
	//memcpy(&(c->connection.localConf),&(tld->tld->localConf),sizeof(TL_Buffer));
	//memset(&(c->connection.remoteConf),0,sizeof(TL_Buffer));
	//UA_String_copy(&(tld->endpointUrl), &(c->connection.localEndpointUrl));

	// network layer
	c->reader = reader;
#ifdef MULTITHREADING
	c->readerThreadHandle = -1;
#endif
	c->networkLayer = tld;
	return UA_NULL;
}

/** the tcp listener routine */
void* NL_TCP_listen(NL_Connection* c) {
	NL_data* tld = c->networkLayer;

	DBG_VERBOSE(printf("NL_TCP_listen - enter listen\n"));
	int retval = listen(c->connectionHandle, tld->tld->maxConnections);
	DBG_VERBOSE(printf("NL_TCP_listen - leave listen, retval=%d\n",retval));

	if (retval < 0) {
		// TODO: Error handling
		perror("NL_TCP_listen");
		DBG_ERR(printf("NL_TCP_listen retval=%d, errno={%d,%s}\n",retval,errno,strerror(errno)));
	} else if (tld->tld->maxConnections == -1 || tld->connections.size < tld->tld->maxConnections) {
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
	do {
		NL_TCP_listen(c);
	} while (UA_TRUE);
	UA_free(c);
	pthread_exit(UA_NULL);
}
#endif


UA_Int32 NL_TCP_init(NL_data* tld, UA_Int32 port) {
	UA_Int32 retval = UA_SUCCESS;
	// socket variables
	int newsockfd;
	int optval = 1;
	struct sockaddr_in serv_addr;


	// create socket for listening to incoming connections
	newsockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (newsockfd < 0) {
		perror("ERROR opening socket");
		retval = UA_ERROR;
	} else {
		// set port number, options and bind
		memset((void *) &serv_addr, sizeof(serv_addr),1);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		if (setsockopt(newsockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1 ) {
			perror("setsockopt");
			retval = UA_ERROR;
		} else {
			// bind to port
			if (bind(newsockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
				perror("ERROR on binding");
				retval = UA_ERROR;
			} else {
				UA_String_copyprintf("opc.tcp://localhost:%d/", &(tld->endpointUrl), port);
			}
		}
	}
	// finally
	if (retval == UA_SUCCESS) {
		DBG_VERBOSE(printf("NL_TCP_init - new listener on %d\n",newsockfd));
		NL_Connection* c;
		UA_Int32 retval = UA_SUCCESS;
		retval |= UA_alloc((void**)&c,sizeof(NL_Connection));
		NL_Connection_init(c, tld, newsockfd, NL_TCP_listen, (TL_Writer) NL_TCP_writer);
#ifdef MULTITHREADING
		pthread_create( &(c->readerThreadHandle), NULL, (void*(*)(void*)) NL_TCP_listenThread, (void*) c);
#else
		UA_list_addPayloadToBack(&(tld->connections),c);
		NL_TCP_SetNonBlocking(c->connectionHandle);
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
