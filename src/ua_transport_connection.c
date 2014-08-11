/*
 * ua_transport_connection.c
 *
 *  Created on: 10.05.2014
 *      Author: open62541
 */

#include "ua_transport_connection.h"
#include "ua_transport.h"
typedef struct TL_ConnectionStruct{
	UA_Int32 connectionHandle;
	UA_UInt32 state;
	TL_Buffer localConf;
	TL_Buffer remoteConf;
	TL_Writer writer;
	UA_String localEndpointUrl;
	UA_String remoteEndpointUrl;
	TL_Closer closeCallback;
	void *networkLayerData;
} TL_ConnectionType;


UA_Int32 UA_TL_Connection_new(UA_TL_Connection *connection, TL_Buffer localBuffers,TL_Writer writer, TL_Closer closeCallback,UA_Int32 handle, void* networkLayerData)
{
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_alloc((void**)connection,sizeof(TL_ConnectionType));
	if(retval == UA_SUCCESS)
	{
		(*((TL_ConnectionType**)connection))->connectionHandle = handle;
		(*((TL_ConnectionType**)connection))->localConf = localBuffers;
		(*((TL_ConnectionType**)connection))->writer = writer;
		(*((TL_ConnectionType**)connection))->closeCallback = closeCallback;
		(*((TL_ConnectionType**)connection))->state = CONNECTIONSTATE_CLOSED;
		(*((TL_ConnectionType**)connection))->networkLayerData = networkLayerData;
	}
	return retval;
}

UA_Int32 UA_TL_Connection_delete(UA_TL_Connection connection)
{
	UA_Int32 retval = UA_SUCCESS;
	retval |= UA_free((void*)connection);
	return retval;
}

UA_Int32 UA_TL_Connection_close(UA_TL_Connection connection)
{
	((TL_ConnectionType*)connection)->state = CONNECTIONSTATE_CLOSE;
	((TL_ConnectionType*)connection)->closeCallback(connection);
	return UA_SUCCESS;
}

UA_Boolean UA_TL_Connection_compare(UA_TL_Connection *connection1, UA_TL_Connection *connection2)
{
	if(connection1 && connection2)
	{
		if ((*(TL_ConnectionType**)connection1)->connectionHandle == (*(TL_ConnectionType**)connection2)->connectionHandle)
		{
			return UA_TRUE;
		}
	}
	return UA_FALSE;
}


UA_Int32 UA_TL_Connection_configByHello(UA_TL_Connection connection, UA_OPCUATcpHelloMessage *helloMessage)
{
	UA_Int32 retval = UA_SUCCESS;
	((TL_ConnectionType*)connection)->remoteConf.maxChunkCount = helloMessage->maxChunkCount;
	((TL_ConnectionType*)connection)->remoteConf.maxMessageSize = helloMessage->maxMessageSize;
	((TL_ConnectionType*)connection)->remoteConf.protocolVersion = helloMessage->protocolVersion;
	((TL_ConnectionType*)connection)->remoteConf.recvBufferSize = helloMessage->receiveBufferSize;
	((TL_ConnectionType*)connection)->remoteConf.sendBufferSize = helloMessage->sendBufferSize;
	((TL_ConnectionType*)connection)->state = CONNECTIONSTATE_ESTABLISHED;
	retval |= UA_String_copy(&helloMessage->endpointUrl,&((TL_ConnectionType*)connection)->remoteEndpointUrl);

	return UA_SUCCESS;
}

UA_Int32 UA_TL_Connection_callWriter(UA_TL_Connection connection, const UA_ByteString** gather_bufs, UA_Int32 gather_len)
{
	return ((TL_ConnectionType*)connection)->writer(((TL_ConnectionType*)connection)->connectionHandle,gather_bufs, gather_len);
}

//setters
UA_Int32 UA_TL_Connection_setWriter(UA_TL_Connection connection, TL_Writer writer)
{
	((TL_ConnectionType*)connection)->writer = writer;
	return UA_SUCCESS;
}
/*
UA_Int32 UA_TL_Connection_setConnectionHandle(UA_TL_Connection connection, UA_Int32 connectionHandle)
{
	((TL_ConnectionType*)connection)->connectionHandle = connectionHandle;
	return UA_SUCCESS;
}
*/
UA_Int32 UA_TL_Connection_setState(UA_TL_Connection connection, UA_Int32 connectionState)
{
	if(connection)
	{
		((TL_ConnectionType*)connection)->state = connectionState;
		return UA_SUCCESS;
	}else{
		return UA_ERROR;
	}
}
//getters
UA_Int32 UA_TL_Connection_getState(UA_TL_Connection connection, UA_Int32 *connectionState)
{
	if(connection)
	{
		*connectionState = ((TL_ConnectionType*)connection)->state;
		return UA_SUCCESS;
	}else{
		*connectionState = -1;
		return UA_ERROR;
	}
}

UA_Int32 UA_TL_Connection_getNetworkLayerData(UA_TL_Connection connection,void** networkLayerData)
{
	if(connection)
	{
		*networkLayerData = ((TL_ConnectionType*)connection)->networkLayerData;
		return UA_SUCCESS;
	}else{
		*networkLayerData = UA_NULL;
		return UA_ERROR;
	}
}

UA_Int32 UA_TL_Connection_getProtocolVersion(UA_TL_Connection connection, UA_UInt32 *protocolVersion)
{
	if(connection)
	{
		*protocolVersion = ((TL_ConnectionType*)connection)->localConf.protocolVersion;
		return UA_SUCCESS;
	}else{
		*protocolVersion = 0xFF;
		return UA_ERROR;
	}
}
UA_Int32 UA_TL_Connection_getLocalConfig(UA_TL_Connection connection, TL_Buffer *localConfiguration)
{
	if(connection)
	{
		return UA_memcpy(localConfiguration,&((TL_ConnectionType*)connection)->localConf, sizeof(TL_Buffer));

	}else{
		localConfiguration = UA_NULL;
		return UA_ERROR;
	}
}
UA_Int32 UA_TL_Connection_getHandle(UA_TL_Connection connection, UA_UInt32 *connectionHandle)
{
	if(connection)
	{
		*connectionHandle = ((TL_ConnectionType*)connection)->connectionHandle;
		return UA_SUCCESS;
	}else{
			connectionHandle = 0;
			return UA_ERROR;
		}
}

UA_Int32 UA_TL_Connection_bind(UA_TL_Connection connection, UA_Int32 handle)
{
	if(connection)
	{
		((TL_ConnectionType*)connection)->connectionHandle = handle;
		return UA_SUCCESS;
	}else{

		return UA_ERROR;
	}

}
