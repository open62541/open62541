/*
 ============================================================================
 Name        : opcuaServer.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : lala
 ============================================================================
 */
#define _XOPEN_SOURCE 1 //TODO HACK
#include <stdio.h>
#include <stdlib.h>

#include "networklayer.h"
#include "ua_application.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef RASPI
	#include "raspberrypi_io.h"
#endif
#include <sys/ipc.h>
#include <sys/shm.h>
#define MAXMYMEM 4



UA_Int32 serverCallback(void * arg) {
	char *name = (char *) arg;
	printf("%s does whatever servers do\n",name);

	Namespace* ns0 = (Namespace*)UA_indexedList_find(appMockup.namespaces, 0)->payload;
	UA_Int32 retval;
	UA_Node const * node;
	UA_ExpandedNodeId serverStatusNodeId = NS0EXPANDEDNODEID(2256);
	retval = Namespace_get(ns0, &(serverStatusNodeId.nodeId),&node, UA_NULL);
	if(retval == UA_SUCCESS){
		((UA_ServerStatusDataType*)(((UA_VariableNode*)node)->value.data))->currentTime = UA_DateTime_now();
	}

	const UA_Node *foundNode = UA_NULL;
	Namespace_Entry_Lock *lock;
	//node which should be filled with data (float value)
	UA_NodeId tmpNodeId;

	tmpNodeId.encodingByte = UA_NODEIDTYPE_TWOBYTE;
	tmpNodeId.identifier.numeric = 110;
	tmpNodeId.namespace =  0;

	if(Namespace_get(ns0,&tmpNodeId, &foundNode,&lock) == UA_SUCCESS){
	#ifdef RASPI
		readTemp((float*)((UA_VariableNode *)foundNode)->value.data);
	#else
		*((float*)((UA_VariableNode *)foundNode)->value.data) = *((float*)((UA_VariableNode *)foundNode)->value.data) + 0.2f;
	#endif

	}

	return UA_SUCCESS;
}

int main(int argc, char** argv) {
	appMockup_init();
	NL_data* nl = NL_init(&NL_Description_TcpBinary,16664);

	struct timeval tv = {1, 0}; // 1 second
	NL_msgLoop(nl, &tv, serverCallback,argv[0]);
}
