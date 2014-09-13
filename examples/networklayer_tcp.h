/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifndef NETWORKLAYERTCP_H_
#define NETWORKLAYERTCP_H_

#include "ua_server.h"

struct NetworklayerTCP;
typedef struct NetworklayerTCP NetworklayerTCP;

#define BINARYCONNECTION_PROTOCOL_VERSION  0
#define BINARYCONNECTION_MAX_CHUNK_COUNT 1
#define BINARYCONNECTION_MAX_MESSAGE_SIZE 8192

UA_Int32 NetworklayerTCP_new(NetworklayerTCP **newlayer, UA_ConnectionConfig localConf,
							 UA_UInt32 port);
void NetworklayerTCP_delete(NetworklayerTCP *layer);
UA_Int32 NetworkLayerTCP_run(NetworklayerTCP *layer, UA_Server *server, struct timeval tv,
							   void(*worker)(UA_Server*), UA_Boolean *running);

#endif /* NETWORKLAYERTCP_H_ */
