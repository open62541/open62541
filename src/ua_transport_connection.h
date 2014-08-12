/*
 * ua_transport_connection.h
 *
 *  Created on: 10.05.2014
 *      Author: open62541
 */

#ifndef UA_TRANSPORT_CONNECTION_H_
#define UA_TRANSPORT_CONNECTION_H_


#include "ua_transport.h"

typedef struct TL_Buffer{
	UA_UInt32 protocolVersion;
	UA_UInt32 sendBufferSize;
	UA_UInt32 recvBufferSize;
	UA_UInt32 maxMessageSize;
	UA_UInt32 maxChunkCount;
} TL_Buffer;

typedef struct UA_ConnectionStruct *UA_TL_Connection;

typedef UA_Int32 (*TL_Closer)(UA_TL_Connection);
typedef UA_Int32 (*TL_Writer)(UA_Int32 connectionHandle, const UA_ByteString** gather_bufs, UA_Int32 gather_len); // send mutiple buffer concatenated into one msg (zero copy)


UA_Int32 UA_TL_Connection_configByHello(UA_TL_Connection connection, UA_OPCUATcpHelloMessage *helloMessage);

UA_Int32 UA_TL_Connection_delete(UA_TL_Connection connection);
UA_Int32 UA_TL_Connection_callWriter(UA_TL_Connection connection, const UA_ByteString** gather_bufs, UA_Int32 gather_len);

UA_Int32 UA_TL_Connection_close(UA_TL_Connection connection);
UA_Int32 UA_TL_Connection_new(UA_TL_Connection *connection, TL_Buffer localBuffers,TL_Writer writer, TL_Closer closeCallback,UA_Int32 handle, void* networkLayerData);
UA_Int32 UA_TL_Connection_bind(UA_TL_Connection connection, UA_Int32 handle);
UA_Boolean UA_TL_Connection_compare(UA_TL_Connection *connection1, UA_TL_Connection *connection2);

//setters
UA_Int32 UA_TL_Connection_setState(UA_TL_Connection connection, UA_Int32 connectionState);
UA_Int32 UA_TL_Connection_setWriter(UA_TL_Connection connection, TL_Writer writer);
//UA_Int32 UA_TL_Connection_setConnectionHandle(UA_TL_Connection connection, UA_Int32 connectionHandle);

//getters
UA_Int32 UA_TL_Connection_getHandle(UA_TL_Connection connection, UA_UInt32 *connectionId);
UA_Int32 UA_TL_Connection_getProtocolVersion(UA_TL_Connection connection, UA_UInt32 *protocolVersion);
UA_Int32 UA_TL_Connection_getState(UA_TL_Connection connection, UA_Int32 *connectionState);
UA_Int32 UA_TL_Connection_getLocalConfig(UA_TL_Connection connection, TL_Buffer *localConfiguration);
UA_Int32 UA_TL_Connection_getNetworkLayerData(UA_TL_Connection connection,void** networkLayerData);


#endif /* UA_TRANSPORT_CONNECTION_H_ */
