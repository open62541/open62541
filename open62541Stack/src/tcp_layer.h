/*
 * tcp_layer.h
 *
 *  Created on: Jan 10, 2014
 *      Author: opcua
 */

#ifndef TCP_LAYER_H_
#define TCP_LAYER_H_
#include "opcua_connectionHelper.h"
/*
 * returns the length of read bytes
 */
UInt32 receive(UA_connection *connection, AD_RawMessage *message,UInt32 bufferLength);

#endif /* TCP_LAYER_H_ */
