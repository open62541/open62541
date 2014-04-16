#ifndef NETWORKLAYER_H_
#define NETWORKLAYER_H_

#include "opcua.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"
#include "ua_list.h"

#ifdef MULTITHREADING
#include <pthread.h> // pthreadcreate, pthread_t
#endif
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

typedef struct T_NL_data {
	NL_Description* tld;
	UA_String endpointUrl;
	int listenerHandle;
	pthread_t listenerThreadHandle;
	UA_list_List connections;
	fd_set readerHandles;
	int maxReaderHandle;
} NL_data;

struct T_NL_connection;
typedef void* (*NL_reader)(struct T_NL_connection *c);
typedef struct T_NL_connection {
	TL_connection connection;
	NL_reader reader;
	pthread_t readerThreadHandle;
	NL_data* networkLayer;
} NL_connection;

NL_data* NL_init(NL_Description* tlDesc, UA_Int32 port);
UA_Int32 NL_msgLoop(NL_data* nl, struct timeval* tv,UA_Int32 (*timeoutCallBack)(void*),void *arg);

#endif /* NETWORKLAYER_H_ */
