#ifndef OPCUA_TRANSPORT_BINARY_H_
#define OPCUA_TRANSPORT_BINARY_H_
#include <stdio.h>


#include "ua_transport_binary.h"
#include "ua_transport_connection.h"
#include "ua_server.h"

//transport errors begin at 1000
#define UA_ERROR_MULTIPLE_HEL 1000
#define UA_ERROR_RCV_ERROR 1001

//variables which belong to layer
#define TL_SERVER_PROTOCOL_VERSION  0
#define TL_SERVER_MAX_CHUNK_COUNT 1
#define TL_SERVER_MAX_MESSAGE_SIZE  8192



/* Transport Layer Connection */
//struct TL_Connection_T; // forward declaration

//typedef UA_Int32 (*TL_Writer)(struct TL_Connection_T* connection, const UA_ByteString** gather_bufs, UA_Int32 gather_len); // send mutiple buffer concatenated into one msg (zero copy)
/*
typedef struct TL_Connection_T {
	UA_Int32 connectionHandle;
	UA_UInt32 connectionState;
	TL_Buffer localConf;
	TL_Buffer remoteConf;
	TL_Writer writerCallback;
	UA_String localEndpointUrl;
	UA_String remoteEndpointUrl;
	struct SL_Channel_T* secureChannel;
} TL_Connection;
*/

UA_Int32 TL_Send(UA_TL_Connection *connection, const UA_ByteString** gather_buf, UA_UInt32 gather_len);
UA_Int32 TL_Process(UA_TL_Connection *connection, UA_Server *server, const UA_ByteString* msg);

#endif /* OPCUA_TRANSPORT_BINARY_H_ */
