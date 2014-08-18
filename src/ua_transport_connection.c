#include "ua_transport_connection.h"
#include "ua_transport.h"

struct UA_TL_Connection {
	UA_Int32 connectionHandle;
	UA_UInt32 state;
	TL_Buffer localConf;
	TL_Buffer remoteConf;
	TL_Writer writer;
	UA_String localEndpointUrl;
	UA_String remoteEndpointUrl;
	TL_Closer closeCallback;
	void *networkLayerData;
};

UA_Int32 UA_TL_Connection_new(UA_TL_Connection **connection, TL_Buffer localBuffers,TL_Writer writer,
		                      TL_Closer closeCallback,UA_Int32 handle, void* networkLayerData) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_alloc((void**)connection,sizeof(UA_TL_Connection));
	if(retval == UA_SUCCESS) {
		(*connection)->connectionHandle = handle;
		(*connection)->localConf = localBuffers;
		(*connection)->writer = writer;
		(*connection)->closeCallback = closeCallback;
		(*connection)->state = CONNECTIONSTATE_CLOSED;
		(*connection)->networkLayerData = networkLayerData;
	}
	return retval;
}

UA_Int32 UA_TL_Connection_delete(UA_TL_Connection *connection) {
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_free((void*)connection);
	return retval;
}

UA_Int32 UA_TL_Connection_close(UA_TL_Connection *connection) {
	connection->state = CONNECTIONSTATE_CLOSED;
	connection->closeCallback(connection);
	return UA_SUCCESS;
}

UA_Boolean UA_TL_Connection_compare(UA_TL_Connection *connection1, UA_TL_Connection *connection2) {
	if(!connection1 || !connection2)
		return UA_FALSE;
	return ((*(UA_TL_Connection**)connection1)->connectionHandle == (*(UA_TL_Connection**)connection2)->connectionHandle);
}


UA_Int32 UA_TL_Connection_configByHello(UA_TL_Connection *connection, UA_OPCUATcpHelloMessage *helloMessage) {
	UA_Int32 retval = UA_SUCCESS;
	connection->remoteConf.maxChunkCount = helloMessage->maxChunkCount;
	connection->remoteConf.maxMessageSize = helloMessage->maxMessageSize;
	connection->remoteConf.protocolVersion = helloMessage->protocolVersion;
	connection->remoteConf.recvBufferSize = helloMessage->receiveBufferSize;
	connection->remoteConf.sendBufferSize = helloMessage->sendBufferSize;
	connection->state = CONNECTIONSTATE_ESTABLISHED;
	retval |= UA_String_copy(&helloMessage->endpointUrl,&connection->remoteEndpointUrl);

	return UA_SUCCESS;
}

UA_Int32 UA_TL_Connection_callWriter(UA_TL_Connection *connection, const UA_ByteString** gather_bufs, UA_Int32 gather_len) {
	return connection->writer(connection->connectionHandle,gather_bufs, gather_len);
}

//setters
UA_Int32 UA_TL_Connection_setWriter(UA_TL_Connection *connection, TL_Writer writer) {
	connection->writer = writer;
	return UA_SUCCESS;
}

/* UA_Int32 UA_TL_Connection_setConnectionHandle(UA_TL_Connection *connection, UA_Int32 connectionHandle)
{
	connection->connectionHandle = connectionHandle;
	return UA_SUCCESS;
} */

UA_Int32 UA_TL_Connection_setState(UA_TL_Connection *connection, UA_Int32 connectionState) {
	if(!connection)
		return UA_ERROR;

	connection->state = connectionState;
	return UA_SUCCESS;
}

//getters
UA_Int32 UA_TL_Connection_getState(UA_TL_Connection *connection, UA_Int32 *connectionState) {
	if(!connection) {
		*connectionState = -1;
		return UA_ERROR;
	}

	*connectionState = connection->state;
	return UA_SUCCESS;
}

UA_Int32 UA_TL_Connection_getNetworkLayerData(UA_TL_Connection *connection,void** networkLayerData) {
	if(!connection) {
		*networkLayerData = UA_NULL;
		return UA_ERROR;
	}

	*networkLayerData = connection->networkLayerData;
	return UA_SUCCESS;
}

UA_Int32 UA_TL_Connection_getProtocolVersion(UA_TL_Connection *connection, UA_UInt32 *protocolVersion) {
	if(!connection) {
		*protocolVersion = 0xFF;
		return UA_ERROR;
	}

	*protocolVersion = connection->localConf.protocolVersion;
	return UA_SUCCESS;
}

UA_Int32 UA_TL_Connection_getLocalConfig(UA_TL_Connection *connection, TL_Buffer *localConfiguration) {
	if(!connection) {
		localConfiguration = UA_NULL;
		return UA_ERROR;
	}

	return UA_memcpy(localConfiguration,&connection->localConf, sizeof(TL_Buffer));
}

UA_Int32 UA_TL_Connection_getHandle(UA_TL_Connection *connection, UA_UInt32 *connectionHandle) {
	if(!connection) {
		connectionHandle = 0;
		return UA_ERROR;
	}

	*connectionHandle = connection->connectionHandle;
	return UA_SUCCESS;
}

UA_Int32 UA_TL_Connection_bind(UA_TL_Connection *connection, UA_Int32 handle) {
	if(!connection)
		return UA_ERROR;

	connection->connectionHandle = handle;
	return UA_SUCCESS;
}
