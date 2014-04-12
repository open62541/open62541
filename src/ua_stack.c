#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>

#include <unistd.h> // read, write, close
#include <stdlib.h> // exit
#include <errno.h> // errno, EINTR

#include <memory.h> // memset
#include <fcntl.h> // fcntl

#include "ua_stack.h"
#include "ua_transportLayer.h"

UA_TL_Description UA_TransportLayerDescriptorTcpBinary  = {
		UA_TL_ENCODING_BINARY,
		UA_TL_CONNECTIONTYPE_TCPV4,
		UA_TL_MAXCONNECTIONS_DEFAULT,
		{-1,8192,8192,16384,1}
};

// TODO: We currently need a variable global to the module!
UA_TL_data theTL;

_Bool connectionComparer(void *p1, void* p2) {
	UA_TL_connection* c1 = (UA_TL_connection*) p1;
	UA_TL_connection* c2 = (UA_TL_connection*) p2;
	return (c1->connectionHandle == c2->connectionHandle);
}

int UA_TL_TCP_SetNonBlocking(int sock) {
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

/** the tcp reader thread - single shot if single-threaded, looping until CLOSE if multi-threaded
 */
void* UA_TL_TCP_reader(UA_TL_connection *c) {

	UA_ByteString readBuffer;
	UA_alloc((void**)&(readBuffer.data),c->localConf.recvBufferSize);

	if (c->connectionState != connectionState_CLOSE) {
		do {
			printf("UA_TL_TCP_reader - enter read\n");
			readBuffer.length = read(c->connectionHandle, readBuffer.data, c->localConf.recvBufferSize);
			printf("UA_TL_TCP_reader - leave read\n");

			printf("UA_TL_TCP_reader - %*.s ",c->remoteEndpointUrl.length,c->remoteEndpointUrl.data);
			UA_ByteString_printx("received=",&readBuffer);

			if (readBuffer.length  > 0) {
				TL_process(c,&readBuffer);
			} else {
				c->connectionState = connectionState_CLOSE;
				perror("ERROR reading from socket1");
			}
		} while (c->connectionState != connectionState_CLOSE && theTL.threaded == UA_STACK_MULTITHREADED);
	}
	if (c->connectionState == connectionState_CLOSE) {
		// clean up: socket, buffer, connection
		// free resources allocated with socket
		if (theTL.threaded == UA_STACK_SINGLETHREADED) {
			printf("UA_TL_TCP_reader - remove handle=%d from fd_set\n",c->connectionHandle);
			// FD_CLR(c->connectionHandle,&(theTL.readerHandles));
		}
		printf("UA_TL_TCP_reader - shutdown\n");
		printf("UA_TL_TCP_reader - enter shutdown\n");
		shutdown(c->connectionHandle,2);
		printf("UA_TL_TCP_reader - enter close\n");
		close(c->connectionHandle);
		printf("UA_TL_TCP_reader - leave close\n");
		c->connectionState = connectionState_CLOSED;

		UA_ByteString_deleteMembers(&readBuffer);
		printf("UA_TL_TCP_reader - search element to remove\n");
		UA_list_Element* lec = UA_list_search(&theTL.connections,connectionComparer,c);
		printf("UA_TL_TCP_reader - remove handle=%d\n",((UA_TL_connection*)lec->payload)->connectionHandle);
		UA_list_removeElement(lec,UA_NULL);
		UA_free(c);
	}
	return UA_NULL;
}

/** write to a tcp transport layer connection */
UA_Int32 UA_TL_TCP_writer(struct T_TL_connection* c, UA_ByteString* msg) {
	UA_ByteString_printx("write data:", msg);
	int nWritten = 0;
	while (nWritten < msg->length) {
		int n=0;
		do {
			printf("UA_TL_TCP_write - enter write\n");
			n = write(c->connectionHandle, &(msg->data[nWritten]), msg->length-nWritten);
			printf("UA_TL_TCP_write - leave write with n=%d,errno=%d\n",n,errno);
		} while (n == -1L && errno == EINTR);
		if (n >= 0) {
			nWritten += n;
		} else {
			// TODO: error handling
		}
	}
	return UA_SUCCESS;
}

/** the tcp listener routine.
 *  does a single shot if single threaded, runs forever if multithreaded
 */
void* UA_TL_TCP_listen(void *p) {
	UA_TL_data* tld = (UA_TL_data*) p;

	UA_String_printf("open62541-server at ",&(tld->endpointUrl));
	do {
		printf("UA_TL_TCP_listen - enter listen\n");
		int retval = listen(tld->listenerHandle, tld->tld->maxConnections);
		printf("UA_TL_TCP_listen - leave listen, retval=%d\n",retval);

		if (retval < 0) {
			// TODO: Error handling
			printf("UA_TL_TCP_listen retval=%d, errno=%d\n",retval,errno);
		} else if (tld->tld->maxConnections == -1 || tld->connections.size < tld->tld->maxConnections) {
			// accept only if not max number of connections exceede
			struct sockaddr_in cli_addr;
			socklen_t cli_len = sizeof(cli_addr);
			printf("UA_TL_TCP_listen - enter accept\n");
			int newsockfd = accept(tld->listenerHandle, (struct sockaddr *) &cli_addr, &cli_len);
			printf("UA_TL_TCP_listen - leave accept\n");
			if (newsockfd < 0) {
				printf("UA_TL_TCP_listen - accept returns errno=%d\n",errno);
				perror("ERROR on accept");
			} else {
				printf("UA_TL_TCP_listen - new connection on %d\n",newsockfd);
				UA_TL_connection* c;
				UA_Int32 retval = UA_SUCCESS;
				retval |= UA_alloc((void**)&c,sizeof(UA_TL_connection));
				TL_Connection_init(c, tld);
				c->connectionHandle = newsockfd;
				c->writerCallback = UA_TL_TCP_writer;
				c->readerCallback = UA_TL_TCP_reader;
				// add to list
				UA_list_addPayloadToBack(&(tld->connections),c);
				if (tld->threaded == UA_STACK_MULTITHREADED) {
					// TODO: handle retval of pthread_create
					pthread_create( &(c->readerThread), NULL, (void*(*)(void*)) UA_TL_TCP_reader, (void*) c);
				} else {
					UA_TL_TCP_SetNonBlocking(c->connectionHandle);
				}
			}
		} else {
			// no action necessary to reject connection
		}
	} while (tld->threaded == UA_STACK_MULTITHREADED);
	return UA_NULL;
}

void checkFdSet(void* payload) {
  UA_TL_connection* c = (UA_TL_connection*) payload;
  if (FD_ISSET(c->connectionHandle, &(theTL.readerHandles))) {
	  c->readerCallback((void*)c);
  }
}

int maxHandle;
void UA_TL_addHandleToSet(UA_Int32 handle) {
	FD_SET(handle, &(theTL.readerHandles));
	maxHandle = (handle > maxHandle) ? handle : maxHandle;
}
void setFdSet(void* payload) {
  UA_TL_connection* c = (UA_TL_connection*) payload;
  UA_TL_addHandleToSet(c->connectionHandle);
}


UA_Int32 UA_Stack_addReaderHandle(UA_Int32 handle, UA_TL_reader reader) {
	UA_Int32 retval = UA_SUCCESS;
	UA_TL_connection* c;
	retval = UA_alloc((void**)&c,sizeof(UA_TL_connection));
	c->connectionHandle = handle;
	c->connectionState = connectionState_ESTABLISHED;
	c->readerCallback = reader;
	return retval;
}

UA_Int32 UA_Stack_msgLoop(struct timeval *tv, UA_Int32(*worker)(void*), void *arg)  {
	UA_Int32 result;
	while (UA_TRUE) {
		// determine the largest handle
		maxHandle = 0;
		UA_list_iteratePayload(&theTL.connections,setFdSet);
		UA_TL_addHandleToSet(theTL.listenerHandle);
		printf("UA_Stack_msgLoop - maxHandle=%d\n", maxHandle);

		// copy tv, some unixes do overwrite and return the remaining time
		struct timeval tmptv;
		memcpy(&tmptv,tv,sizeof(struct timeval));

		// and wait
		printf("UA_Stack_msgLoop - enter select sec=%d,usec=%d\n",(UA_Int32) tmptv.tv_sec, (UA_Int32) tmptv.tv_usec);
		result = select(maxHandle + 1, &(theTL.readerHandles), UA_NULL, UA_NULL,&tmptv);
		printf("UA_Stack_msgLoop - leave select result=%d,sec=%d,usec=%d\n",result, (UA_Int32) tmptv.tv_sec, (UA_Int32) tmptv.tv_usec);
		if (result == 0) {
			int err = errno;
			switch (err) {
			case EBADF:
				printf("UA_Stack_msgLoop - result=bad file\n"); //FIXME: handle
				break;
			case EINTR:
				printf("UA_Stack_msgLoop - result=interupted\n"); //FIXME: handle
				break;
			case EINVAL:
				printf("UA_Stack_msgLoop - result=bad arguments\n"); //FIXME: handle
				break;
			case EAGAIN:
				printf("UA_Stack_msgLoop - result=do it again\n");
			default:
				printf("UA_Stack_msgLoop - result=%d\n",err);
				worker(arg);
			}
		} else if (FD_ISSET(theTL.listenerHandle,&theTL.readerHandles)) { // activity on listener port
			printf("UA_Stack_msgLoop - connection request\n");
			UA_TL_TCP_listen((void*)&theTL);
		} else { // activity on client ports
			printf("UA_Stack_msgLoop - activities on %d handles\n",result);
			UA_list_iteratePayload(&theTL.connections,checkFdSet);
		}
	}
	return UA_SUCCESS;
}

UA_Int32 UA_TL_TCP_init(UA_TL_data* tld, UA_Int32 port) {
	UA_Int32 retval = UA_SUCCESS;
	// socket variables
	int optval = 1;
	struct sockaddr_in serv_addr;


	// create socket for listening to incoming connections
	tld->listenerHandle = socket(PF_INET, SOCK_STREAM, 0);
	if (tld->listenerHandle < 0) {
		perror("ERROR opening socket");
		retval = UA_ERROR;
	} else {
		// set port number, options and bind
		memset((void *) &serv_addr, sizeof(serv_addr),1);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(port);
		if (setsockopt(tld->listenerHandle, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1 ) {
			perror("setsockopt");
			retval = UA_ERROR;
		} else {
			// bind to port
			if (bind(tld->listenerHandle, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
				perror("ERROR on binding");
				retval = UA_ERROR;
			} else {
				// TODO: implement
				// UA_String_sprintf("opc.tpc://localhost:%d", &(tld->endpointUrl), port);
				UA_String_copycstring("opc.tpc://localhost:16664/", &(tld->endpointUrl));
			}
		}
	}
	// finally
	if (retval == UA_SUCCESS) {
		if (tld->threaded == UA_STACK_MULTITHREADED) {
			// TODO: handle retval of pthread_create
			pthread_create( &tld->listenerThreadHandle, NULL, UA_TL_TCP_listen, (void*) tld);
		} else {
			UA_TL_TCP_SetNonBlocking(tld->listenerHandle);
			FD_ZERO(&(tld->readerHandles));
		}
	}
	return retval;
}

/** checks arguments and dispatches to worker or refuses to init */
UA_Int32 UA_TL_init(UA_TL_Description* tlDesc, UA_Int32 port, UA_Int32 threaded) {
	UA_Int32 retval = UA_SUCCESS;
	if (tlDesc->connectionType == UA_TL_CONNECTIONTYPE_TCPV4 && tlDesc->encoding == UA_TL_ENCODING_BINARY) {
		theTL.tld = tlDesc;
		theTL.threaded = threaded;
		UA_list_init(&theTL.connections);
		retval |= UA_TL_TCP_init(&theTL,port);
	} else {
		retval = UA_ERR_NOT_IMPLEMENTED;
	}
	return retval;
}

/** checks arguments and dispatches to worker or refuses to init */
UA_Int32 UA_Stack_init(UA_TL_Description* tlDesc, UA_Int32 port, UA_Int32 threaded) {
	UA_Int32 retval = UA_SUCCESS;
	retval = UA_TL_init(tlDesc,port,threaded);
	return retval;
}
