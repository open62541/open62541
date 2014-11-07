/*
 * ua_namespace.h
 *
 *  Created on: Nov 6, 2014
 *      Author: opcua
 */

#ifndef UA_NAMESPACE_H_
#define UA_NAMESPACE_H_
#include "ua_server.h"
#include "ua_util.h"
#include "ua_statuscodes.h"

typedef struct UA_Namespace
{
	UA_UInt16 index;
	UA_String url;
	UA_NodeStoreInterface *nodeStore;
}UA_Namespace;

UA_Namespace *UA_Namespace_new();
void UA_Namespace_init(UA_Namespace *namespace);
void UA_Namespace_delete(UA_Namespace *namespace);
void UA_Namespace_deleteMembers(UA_Namespace *namespace);

#endif /* UA_NAMESPACE_H_ */
