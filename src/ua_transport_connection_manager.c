/*
 * ua_connection_manager.c
 *
 *  Created on: 11.05.2014
 *      Author: open62541
 */

#include "ua_transport_connection_manager.h"
#include "ua_indexedList.h"

typedef struct UA_TL_ConnectionManager {
	UA_list_List connections;
	UA_UInt32 maxConnectionCount;
	UA_UInt32 currentConnectionCount;
} UA_TL_ConnectionManager;

static UA_TL_ConnectionManager *connectionManager = UA_NULL;

UA_Int32 UA_TL_ConnectionManager_init(UA_UInt32 maxConnectionCount) {
	UA_Int32 retval = UA_SUCCESS;
	if(connectionManager != UA_NULL)
		return UA_ERROR; //connection Manager already exists

	retval |= UA_alloc((void**)&connectionManager,sizeof(UA_TL_ConnectionManager));
	connectionManager->maxConnectionCount = maxConnectionCount;
	connectionManager->currentConnectionCount = 0;
	retval |= UA_indexedList_init(&connectionManager->connections);
	return UA_SUCCESS;
}

UA_Int32 UA_TL_ConnectionManager_addConnection(UA_TL_Connection *connection) {
	UA_UInt32 connectionId;
	UA_TL_Connection_getHandle(connection, &connectionId);
	printf("UA_TL_ConnectionManager_addConnection - added connection with handle = %d \n", connectionId);
	return UA_list_addPayloadToBack(&(connectionManager->connections), (void*)connection);
}

UA_Int32 UA_TL_ConnectionManager_removeConnection(UA_TL_Connection *connection) {
	UA_list_Element *element =  UA_list_find(&connectionManager->connections, (UA_list_PayloadMatcher)UA_TL_Connection_compare);
	if(element) {
		UA_list_removeElement(element, (UA_list_PayloadVisitor)UA_TL_Connection_delete);
	}
	return UA_SUCCESS;
}

UA_Int32 UA_TL_ConnectionManager_getConnectionByHandle(UA_UInt32 connectionId, UA_TL_Connection **connection) {
	UA_UInt32 tmpConnectionHandle;
	if(connectionManager) {
		UA_list_Element* current = connectionManager->connections.first;
		while (current) {
			if (current->payload) {
				UA_list_Element* elem = (UA_list_Element*) current;
				*connection = ((UA_TL_Connection*) (elem->payload));
				UA_TL_Connection_getHandle(*connection, &tmpConnectionHandle);

				if(tmpConnectionHandle == connectionId)
					return UA_SUCCESS;
			}
			current = current->next;
		}
	}

	*connection = UA_NULL;
	return UA_ERROR;
}
