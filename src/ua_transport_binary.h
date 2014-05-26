#ifndef OPCUA_TRANSPORT_BINARY_H_
#define OPCUA_TRANSPORT_BINARY_H_
#include <stdio.h>

#include "opcua.h"

//transport errors begin at 1000
#define UA_ERROR_MULTIPLE_HEL 1000
#define UA_ERROR_RCV_ERROR 1001

//variables which belong to layer
#define TL_SERVER_PROTOCOL_VERSION  0
#define TL_SERVER_MAX_CHUNK_COUNT 1
#define TL_SERVER_MAX_MESSAGE_SIZE  8192

typedef struct TL_Buffer {
	UA_UInt32 protocolVersion;
	UA_UInt32 sendBufferSize;
	UA_UInt32 recvBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
} TL_Buffer;

/* Transport Layer Connection */
struct TL_Connection; // forward declaration
typedef UA_Int32 (*TL_Writer)(struct TL_Connection* connection, const UA_ByteString** gather_bufs, UA_Int32 gather_len); // send mutiple buffer concatenated into one msg (zero copy)

typedef struct TL_Connection {
	UA_Int32 connectionHandle;
	UA_UInt32 connectionState;
	TL_Buffer localConf;
	TL_Buffer remoteConf;
	TL_Writer writerCallback;
	UA_String localEndpointUrl;
	UA_String remoteEndpointUrl;
	struct SL_Channel* secureChannel;
} TL_Connection;

UA_Int32 TL_Send(TL_Connection* connection, const UA_ByteString** gather_buf, UA_UInt32 gather_len);
UA_Int32 TL_Process(TL_Connection *connection, const UA_ByteString *packet);

#endif /* OPCUA_TRANSPORT_BINARY_H_ */
