/*
 * ua_connection_manager.c
 *
 *  Created on: 11.05.2014
 *      Author: open62541
 */

#include "ua_transport_connection_manager.h"
#include "ua_indexedList.h"
typedef struct UA_TL_ConnectionManager
{
	UA_indexedList_List connections;
	UA_UInt32 maxConnectionCount;
	UA_UInt32 currentConnectionCount;
}UA_TL_ConnectionManager;


static UA_TL_ConnectionManager *connectionManager = UA_NULL;


UA_Int32 UA_TL_ConnectionManager_init(UA_UInt32 maxConnectionCount)
{
	UA_Int32 retval = UA_SUCCESS;
	if(connectionManager)
	{
		//connectionManager already exists;
	}
	else
	{
		retval |= UA_alloc((void**)connectionManager,sizeof(UA_TL_ConnectionManager));
		connectionManager->maxConnectionCount = maxConnectionCount;
		connectionManager->currentConnectionCount = 0;
		retval |= UA_indexedList_init(&connectionManager->connections);
	}
	return UA_SUCCESS;
}
UA_Int32 UA_TL_ConnectionManager_addConnection(UA_TL_Connection1 *connection)
{
	UA_UInt32 connectionId;
	UA_TL_Connection_getHandle(*connection, &connectionId);
	return UA_indexedList_addValue(&(connectionManager->connections), connectionId, connection);
}


UA_Int32 UA_TL_ConnectionManager_removeConnection(UA_TL_Connection1 connection)
{
	UA_UInt32 connectionId;
	UA_TL_Connection_getHandle(connection, &connectionId);
	UA_TL_Connection1 foundValue = UA_indexedList_findValue(&connectionManager->connections,connectionId);
	if(foundValue)
	{
		UA_TL_Connection_delete(connection);//remove connection
	}
	return UA_SUCCESS;
}


UA_Int32 UA_TL_ConnectionManager_getConnectionById(UA_Int32 connectionId, UA_TL_Connection1 *connection)
{
	return UA_SUCCESS;
}


