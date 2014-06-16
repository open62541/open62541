/*
 * ua_connection_manager.h
 *
 *  Created on: 11.05.2014
 *      Author: open62541
 */

#ifndef UA_CONNECTION_MANAGER_H_
#define UA_CONNECTION_MANAGER_H_
#include "stdio.h"
#include "ua_transport_connection.h"



UA_Int32 UA_TL_ConnectionManager_addConnection(UA_TL_Connection1 *connection);
UA_Int32 UA_TL_ConnectionManager_removeConnection(UA_TL_Connection1 connection);
//getter
UA_Int32 UA_TL_ConnectionManager_getConnectionByHandle(UA_UInt32 connectionId, UA_TL_Connection1 *connection);

#endif /* UA_CONNECTION_MANAGER_H_ */
