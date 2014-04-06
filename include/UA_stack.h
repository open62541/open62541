/*
 * UA_stack.h
 *
 *  Created on: 04.04.2014
 *      Author: mrt
 */

#ifndef UA_STACK_H_
#define UA_STACK_H_

#include "opcua.h"
#include "UA_connection.h"
#include "UA_list.h"

#define UA_TL_MAXCONNECTIONS_DEFAULT 10

enum UA_TRANSPORTLAYERDESCRIPTION_ENCODING_enum {
	UA_TL_ENCODING_BINARY = 0,
	UA_TL_ENCODING_XML = 1,
};
enum UA_TRANSPORTLAYERDESCRIPTION_CONNECTION_enum {
	UA_TL_CONNECTIONTYPE_TCPV4 = 0,
	UA_TL_CONNECTIONTYPE_TCPV6 = 1,
};
typedef struct T_UA_TL_Description {
	UA_Int32 encoding;
	UA_Int32 connectionType;
	UA_Int32 maxConnections;
	TL_buffer localConf;
} UA_TL_Description;

extern UA_TL_Description UA_TransportLayerDescriptorTcpBinary;

typedef struct T_UA_TL_data {
	UA_TL_Description* tld;
	UA_String endpointUrl;
	int listenerHandle;
	pthread_t listenerThreadHandle;
	UA_list_List connections;
} UA_TL_data;

UA_Int32 UA_Stack_init(UA_TL_Description* tlDesc, UA_Int32 port);
#endif /* UA_STACK_H_ */
