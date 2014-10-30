/*
 * open62541_nodestore.c
 *
 *  Created on: Oct 27, 2014
 *      Author: opcua
 */

#include "open62541_nodestore.h"
static UA_NodeStoreExample *open62541_nodestore;

void Nodestore_set(UA_NodeStoreExample *nodestore){
	open62541_nodestore = nodestore;
}

UA_NodeStoreExample *Nodestore_get(){
	return open62541_nodestore;
}
