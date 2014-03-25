/*
 * opcua_secureChannelLayer.h
 *
 *  Created on: Dec 19, 2013
 *      Author: opcua
 */
#ifndef OPCUA_SECURECHANNELLAYER_H_
#define OPCUA_SECURECHANNELLAYER_H_
#include "opcua.h"
#include "UA_connection.h"
#include "../include/UA_config.h"
#include "UA_stackInternalTypes.h"
/*
*
* @param connection
* @return
*/
UA_Int32 SL_initConnectionObject(UA_connection *connection);

/**
*
* @param connection
* @param response
* @param sizeInOut
* @return
*/
UA_Int32 SL_openSecureChannel_responseMessage_get(UA_connection *connection,
UA_SL_Response *response, UA_Int32* sizeInOut);

void SL_receive(UA_connection *connection, UA_ByteString *serviceMessage);

#endif /* OPCUA_SECURECHANNELLAYER_H_ */
