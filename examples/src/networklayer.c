#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>

#include <unistd.h> // read, write, close
#include <stdlib.h> // exit
#include <errno.h> // errno, EINTR

#include <memory.h> // memset
#include <fcntl.h> // fcntl

#define VERBOSE 1
#include "networklayer.h"

NL_Description NL_Description_TcpBinary  = {
		NL_THREADINGTYPE_SINGLE,
		NL_CONNECTIONTYPE_TCPV4,
		NL_MAXCONNECTIONS_DEFAULT,
		{-1,8192,8192,16384,1}
};

_Bool NL_connectionComparer(void *p1, void* p2) {
	NL_connection* c1 = (NL_connection*) p1;
	NL_connection* c2 = (NL_connection*) p2;
	return (c1->connection.connectionHandle == c2->connection.connectionHandle);
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

void NL_connection_printf(void* payload) {
  NL_connection* c = (NL_connection*) payload;
  printf("ListElement connectionHandle = %d\n",c->connection.connectionHandle);
}

/** the tcp reader thread - single shot if single-threaded, looping until CLOSE if multi-threaded
 */
void* NL_TCP_reader(NL_connection *c) {

	UA_ByteString readBuffer;
	UA_alloc((void**)&(readBuffer.data),c->connection.localConf.recvBufferSize);

	if (c->connection.connectionState != connectionState_CLOSE) {
		do {
			DBG_VERBOSE(printf("NL_TCP_reader - enter read\n"));
			readBuffer.length = read(c->connection.connectionHandle, readBuffer.data, c->connection.localConf.recvBufferSize);
			DBG_VERBOSE(printf("NL_TCP_reader - leave read\n"));

			DBG_VERBOSE(printf("NL_TCP_reader - src={%*.s}, ",c->connection.remoteEndpointUrl.length,c->connection.remoteEndpointUrl.data));
			UA_ByteString_printx("received=",&readBuffer);

			if (readBuffer.length  > 0) {
				TL_process(&(c->connection),&readBuffer);
			} else {
				c->connection.connectionState = connectionState_CLOSE;
				perror("ERROR reading from socket1");
			}
		} while (c->connection.connectionState != connectionState_CLOSE && c->networkLayer->threaded == NL_THREADINGTYPE_PTHREAD);
	}
	if (c->connection.connectionState == connectionState_CLOSE) {
		DBG_VERBOSE(printf("NL_TCP_reader - enter shutdown\n"));
		shutdown(c->connection.connectionHandle,2);
		DBG_VERBOSE(printf("NL_TCP_reader - enter close\n"));
		close(c->connection.connectionHandle);
		DBG_VERBOSE(printf("NL_TCP_reader - leave close\n"));
		c->connection.connectionState = connectionState_CLOSED;

		UA_ByteString_deleteMembers(&readBuffer);
		DBG_VERBOSE(printf("NL_TCP_reader - search element to remove\n"));
		UA_list_Element* lec = UA_list_search(&(c->networkLayer->connections),NL_connectionComparer,c);
		DBG_VERBOSE(printf("NL_TCP_reader - remove connection for handle=%d\n",((NL_connection*)lec->payload)->connection.connectionHandle));
		UA_list_removeElement(lec,UA_NULL);
		DBG_VERBOSE(UA_list_iteratePayload(&(c->networkLayer->connections),NL_connection_printf));
		UA_free(c);
	}
	return UA_NULL;
}

/** write to a tcp transport layer connection */
UA_Int32 NL_TCP_writer(UA_TL_connection* c, UA_ByteString* msg) {
	DBG_VERBOSE(UA_ByteString_printx("NL_TCP_writer - msg=", msg));
	int nWritten = 0;
	while (nWritten < msg->length) {
		int n=0;
		do {
			DBG_VERBOSE(printf("NL_TCP_writer - enter write\n"));
			n = write(c->connectionHandle, &(msg->data[nWritten]), msg->length-nWritten);
			DBG_VERBOSE(printf("NL_TCP_writer - leave write with n=%d,errno={%d,%s}\n",n,errno,strerror(errno)));
		} while (n == -1L && errno == EINTR);
		if (n >= 0) {
			nWritten += n;
		} else {
			// TODO: error handling
		}
	}
	return UA_SUCCESS;
}

void* NL_Connection_init(NL_connection* c, NL_data* tld, UA_Int32 connectionHandle, NL_reader reader, UA_TL_writer writer)
{
	// connection layer of UA stack
	c->connection.connectionHandle = connectionHandle;
	c->connection.connectionState = connectionState_CLOSED;
	c->connection.writerCallback = writer;
	memcpy(&(c->connection.localConf),&(tld->tld->localConf),sizeof(TL_buffer));
	memset(&(c->connection.remoteConf),0,sizeof(TL_buffer));
	UA_String_copy(&(tld->endpointUrl), &(c->connection.localEndpointUrl));

	// network layer
	c->reader = reader;
	c->readerThreadHandle = -1;
	c->networkLayer = tld;
	return UA_NULL;
}


/** the tcp listener routine.
 *  does a single shot if single threaded, runs forever if multithreaded
 */
void* NL_TCP_listen(NL_connection* c) {
	NL_data* tld = c->networkLayer;

	// UA_String_printf("open62541-server listening at endpoint ",&(tld->endpointUrl));
	do {
		DBG_VERBOSE(printf("NL_TCP_listen - enter listen\n"));
		int retval = listen(c->connection.connectionHandle, tld->tld->maxConnections);
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
			int newsockfd = accept(c->connection.connectionHandle, (struct sockaddr *) &cli_addr, &cli_len);
			DBG_VERBOSE(printf("NL_TCP_listen - leave accept\n"));
			if (newsockfd < 0) {
				DBG_ERR(printf("UA_TL_TCP_listen - accept returns errno={%d,%s}\n",errno,strerror(errno)));
				perror("ERROR on accept");
			} else {
				DBG_VERBOSE(printf("NL_TCP_listen - new connection on %d\n",newsockfd));
				NL_connection* cclient;
				UA_Int32 retval = UA_SUCCESS;
				retval |= UA_alloc((void**)&cclient,sizeof(NL_connection));
				NL_Connection_init(cclient, tld, newsockfd, NL_TCP_reader, NL_TCP_writer);
				UA_list_addPayloadToBack(&(tld->connections),cclient);
				if (tld->threaded == NL_THREADINGTYPE_PTHREAD) {
					// TODO: handle retval of pthread_create
					pthread_create( &(cclient->readerThreadHandle), NULL, (void*(*)(void*)) NL_TCP_reader, (void*) cclient);
				} else {
					NL_TCP_SetNonBlocking(cclient->connection.connectionHandle);
				}
			}
		} else {
			// no action necessary to reject connection
		}
	} while (tld->threaded == NL_THREADINGTYPE_PTHREAD);
	return UA_NULL;
}


void NL_addHandleToSet(UA_Int32 handle, NL_data* nl) {
	FD_SET(handle, &(nl->readerHandles));
	nl->maxReaderHandle = (handle > nl->maxReaderHandle) ? handle : nl->maxReaderHandle;
}
void NL_setFdSet(void* payload) {
  NL_connection* c = (NL_connection*) payload;
  NL_addHandleToSet(c->connection.connectionHandle, c->networkLayer);
}
void NL_checkFdSet(void* payload) {
  NL_connection* c = (NL_connection*) payload;
  if (FD_ISSET(c->connection.connectionHandle, &(c->networkLayer->readerHandles))) {
	  c->reader((void*)c);
  }
}

UA_Int32 NL_msgLoop(NL_data* nl, struct timeval *tv, UA_Int32(*worker)(void*), void *arg)  {
	UA_Int32 result;
	while (UA_TRUE) {
		// determine the largest handle
		nl->maxReaderHandle = 0;
		UA_list_iteratePayload(&(nl->connections),NL_setFdSet);
		DBG_VERBOSE(printf("UA_Stack_msgLoop - maxHandle=%d\n", nl->maxReaderHandle));

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
		NL_connection* c;
		UA_Int32 retval = UA_SUCCESS;
		retval |= UA_alloc((void**)&c,sizeof(NL_connection));
		NL_Connection_init(c, tld, newsockfd, NL_TCP_listen, (UA_TL_writer) UA_NULL);
		UA_list_addPayloadToBack(&(tld->connections),c);
		if (tld->threaded == NL_THREADINGTYPE_PTHREAD) {
			// TODO: handle retval of pthread_create
			pthread_create( &(c->readerThreadHandle), NULL, (void*(*)(void*)) NL_TCP_listen, (void*) c);
		} else {
			NL_TCP_SetNonBlocking(c->connection.connectionHandle);
		}
	}
	return retval;
}


/** checks arguments and dispatches to worker or refuses to init */
NL_data* NL_init(NL_Description* tlDesc, UA_Int32 port, UA_Int32 threaded) {
	NL_data* nl = UA_NULL;
	if (tlDesc->connectionType == NL_CONNECTIONTYPE_TCPV4 && tlDesc->encoding == NL_UA_ENCODING_BINARY) {
		UA_alloc((void**)&nl,sizeof(NL_data));
		nl->tld = tlDesc;
		nl->threaded = threaded;
		FD_ZERO(&(nl->readerHandles));
		UA_list_init(&(nl->connections));
		NL_TCP_init(nl,port);
	}
	return nl;
}
