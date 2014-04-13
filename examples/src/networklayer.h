/*
 * networklayer.h
 *
 *  Created on: 04.04.2014
 *      Author: mrt
 */

#ifndef NETWORKLAYER_H_
#define NETWORKLAYER_H_

#include "opcua.h"
#include "ua_connection.h"
#include "ua_list.h"

#include <pthread.h> // pthreadcreate, pthread_t
#include <sys/select.h> // FD_ZERO, FD_SET

#define NL_MAXCONNECTIONS_DEFAULT 10

enum NL_UA_ENCODING_enum {
	NL_UA_ENCODING_BINARY = 0,
	NL_UA_ENCODING_XML = 1,
};
enum NL_CONNECTIONTYPE_enum {
	NL_CONNECTIONTYPE_TCPV4 = 0,
	NL_CONNECTIONTYPE_TCPV6 = 1,
};
typedef struct T_NL_Description {
	UA_Int32 encoding;
	UA_Int32 connectionType;
	UA_Int32 maxConnections;
	TL_buffer localConf;
} NL_Description;

extern NL_Description NL_Description_TcpBinary;

enum NL_THREADINGTYPE_enum {
	NL_THREADINGTYPE_SINGLE = 0,
	NL_THREADINGTYPE_PTHREAD = 1,
};

typedef struct T_NL_data {
	NL_Description* tld;
	UA_String endpointUrl;
	int listenerHandle;
	pthread_t listenerThreadHandle;
	UA_list_List connections;
	UA_Int32 threaded;	// NL_THREADINGTYPE_enum
	fd_set readerHandles;
} NL_data;


UA_Int32 NL_init(NL_Description* tlDesc, UA_Int32 port, UA_Int32 threaded);
UA_Int32 NL_msgLoop(struct timeval* tv,UA_Int32 (*timeoutCallBack)(void*),void *arg);
#endif /* NETWORKLAYER_H_ */
