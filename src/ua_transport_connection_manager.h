#ifndef UA_CONNECTION_MANAGER_H_
#define UA_CONNECTION_MANAGER_H_

#include "stdio.h"
#include "ua_transport_connection.h"

UA_Int32 UA_TL_ConnectionManager_init(UA_UInt32 maxConnectionCount);
UA_Int32 UA_TL_ConnectionManager_addConnection(UA_TL_Connection *connection);
UA_Int32 UA_TL_ConnectionManager_removeConnection(UA_TL_Connection *connection);

//getter
UA_Int32 UA_TL_ConnectionManager_getConnectionByHandle(UA_UInt32 connectionId, UA_TL_Connection **connection);

#endif /* UA_CONNECTION_MANAGER_H_ */
