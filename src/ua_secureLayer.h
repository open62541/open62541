#ifndef OPCUA_SECURECHANNELLAYER_H_
#define OPCUA_SECURECHANNELLAYER_H_
#include "opcua.h"
#include "ua_connection.h"
#include "ua_stackInternalTypes.h"

UA_Int32 SL_initConnectionObject(UA_SL_Channel *connection);
UA_Int32 SL_openSecureChannel_responseMessage_get(UA_SL_Channel *connection,
UA_SL_Response *response, UA_Int32* sizeInOut);
UA_Int32 UA_SL_process(UA_SL_Channel* channel, UA_ByteString* msg, UA_Int32* pos);
UA_Int32 UA_SL_Channel_new(UA_TL_connection *connection, UA_ByteString* msg, UA_Int32* pos);

#endif /* OPCUA_SECURECHANNELLAYER_H_ */
