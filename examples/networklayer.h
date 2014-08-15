#ifndef NETWORKLAYER_H_
#define NETWORKLAYER_H_

#include "ua_types.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"
#include "util/ua_list.h"

#ifdef MULTITHREADING
#include <pthread.h> // pthreadcreate, pthread_t
#endif

// fd_set, FD_ZERO, FD_SET
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/select.h> 
#endif

#define NL_MAXCONNECTIONS_DEFAULT 10

enum NL_UA_ENCODING_enum {
	NL_UA_ENCODING_BINARY = 0,
	NL_UA_ENCODING_XML = 1,
};

enum NL_CONNECTIONTYPE_enum {
	NL_CONNECTIONTYPE_TCPV4 = 0,
	NL_CONNECTIONTYPE_TCPV6 = 1,
};

typedef struct NL_Description {
	UA_Int32 encoding;
	UA_Int32 connectionType;
	UA_Int32 maxConnections;
	TL_Buffer localConf;
} NL_Description;

extern NL_Description NL_Description_TcpBinary;

typedef struct NL_data {
	NL_Description* tld;
	UA_String endpointUrl;
	UA_list_List connections;
	fd_set readerHandles;
	int maxReaderHandle;
} NL_data;

struct NL_Connection;
typedef void* (*NL_Reader)(struct NL_Connection *c);
typedef struct NL_Connection {
	UA_TL_Connection *connection;
	UA_Int32 state;
	UA_UInt32 connectionHandle;
	NL_Reader reader;
#ifdef MULTITHREADING
	pthread_t readerThreadHandle;
#endif
	NL_data* networkLayer;
} NL_Connection;

NL_data* NL_init(NL_Description* tlDesc, UA_Int32 port);
UA_Int32 NL_Connection_close(UA_TL_Connection *connection);
UA_Int32 NL_msgLoop(NL_data* nl, struct timeval *tv, UA_Int32(*worker)(void*), void *arg, UA_Boolean *running);
UA_Int32 NL_TCP_writer(UA_Int32 connectionHandle, UA_ByteString const * const * gather_buf, UA_UInt32 gather_len);

#endif /* NETWORKLAYER_H_ */
