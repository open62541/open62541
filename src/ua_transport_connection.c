/*
 * ua_transport_connection.c
 *
 *  Created on: 10.05.2014
 *      Author: open62541
 */

#include "ua_transport_connection.h"
#include "ua_transport.h"
typedef struct TL_Connection{
	UA_Int32 connectionHandle;
	UA_UInt32 state;
	TL_Buffer localConf;
	TL_Buffer remoteConf;
	TL_Writer writer;
	UA_String localEndpointUrl;
	UA_String remoteEndpointUrl;
} TL_Connection;


UA_Int32 UA_TL_Connection_new(UA_TL_Connection1 *connection, TL_Buffer localBuffers,TL_Writer writer)
{
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_alloc((void**)connection,sizeof(TL_Connection));
	if(retval == UA_SUCCESS)
	{
		(*((TL_Connection**)connection))->localConf = localBuffers;
		(*((TL_Connection**)connection))->writer = writer;

	//	((TL_Connection*)connection)->localConf.maxChunkCount = localBuffers->maxChunkCount;
	//	((TL_Connection*)connection)->localConf.maxMessageSize = localBuffers->maxMessageSize;
	//	((TL_Connection*)connection)->localConf.protocolVersion = localBuffers->protocolVersion;
	//	((TL_Connection*)connection)->localConf.recvBufferSize = localBuffers->recvBufferSize;
	//	((TL_Connection*)connection)->localConf.sendBufferSize = localBuffers->sendBufferSize;
	}
	return retval;
}

UA_Int32 UA_TL_Connection_delete(UA_TL_Connection1 connection)
{
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_free((void*)connection);
	return retval;
}
UA_Int32 UA_TL_Connection_close(UA_TL_Connection1 connection)
{
	((TL_Connection*)connection)->state = CONNECTIONSTATE_CLOSED;
	return UA_SUCCESS;
}
UA_Int32 UA_TL_Connection_configByHello(UA_TL_Connection1 connection, UA_OPCUATcpHelloMessage *helloMessage)
{
	UA_Int32 retval = UA_SUCCESS;
	((TL_Connection*)connection)->remoteConf.maxChunkCount = helloMessage->maxChunkCount;
	((TL_Connection*)connection)->remoteConf.maxMessageSize = helloMessage->maxMessageSize;
	((TL_Connection*)connection)->remoteConf.protocolVersion = helloMessage->protocolVersion;
	((TL_Connection*)connection)->remoteConf.recvBufferSize = helloMessage->receiveBufferSize;
	((TL_Connection*)connection)->remoteConf.sendBufferSize = helloMessage->sendBufferSize;
	((TL_Connection*)connection)->state = CONNECTIONSTATE_ESTABLISHED;
	retval |= UA_String_copy(&helloMessage->endpointUrl,&connection->remoteEndpointUrl);

	return UA_SUCCESS;
}

UA_Int32 UA_TL_Connection_callWriter(UA_TL_Connection1 connection, const UA_ByteString** gather_bufs, UA_Int32 gather_len)
{
	return ((TL_Connection*)connection)->writer(((TL_Connection*)connection)->connectionHandle,gather_bufs, gather_len);
}

//setters
UA_Int32 UA_TL_Connection_setWriter(UA_TL_Connection1 connection, TL_Writer writer)
{
	((TL_Connection*)connection)->writer = writer;
	return UA_SUCCESS;
}

//getters
UA_Int32 UA_TL_Connection_getState(UA_TL_Connection1 connection, UA_Int32 *connectionState)
{
	if(connection)
	{
		*connectionState = ((TL_Connection*)connection)->state;
		return UA_SUCCESS;
	}else{
		*connectionState = -1;
		return UA_ERROR;
	}
}

UA_Int32 UA_TL_Connection_getProtocolVersion(UA_TL_Connection1 connection, UA_UInt32 *protocolVersion)
{
	if(connection)
	{
		*protocolVersion = ((TL_Connection*)connection)->localConf.protocolVersion;
		return UA_SUCCESS;
	}else{
		*protocolVersion = 0xFF;
		return UA_ERROR;
	}
}
UA_Int32 UA_TL_Connection_getLocalConfiguration(UA_TL_Connection1 connection, TL_Buffer *localConfiguration)
{
	if(connection)
	{
		return UA_memcpy(localConfiguration,&((TL_Connection*)connection)->localConf, sizeof(TL_Buffer));

	}else{
		localConfiguration = UA_NULL;
		return UA_ERROR;
	}
}
UA_Int32 UA_TL_Connection_getId(UA_TL_Connection1 connection, UA_UInt32 *connectionId)
{
	if(connection)
	{
		*connectionId = ((TL_Connection*)connection)->connectionHandle;
		return UA_SUCCESS;
	}else{
			connectionId = 0;
			return UA_ERROR;
		}
}

UA_Int32 UA_TL_Connection_bind(UA_TL_Connection1 connection, UA_Int32 handle)
{
	if(connection)
	{

		((TL_Connection*)connection)->connectionHandle = handle;
		return UA_SUCCESS;
	}else{

		return UA_ERROR;
	}

}
