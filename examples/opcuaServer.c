#include <stdio.h>
#include <stdlib.h>

#include "networklayer.h"
#include "ua_application.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <signal.h>
#include "ua_stack_channel_manager.h"
#include "ua_stack_session_manager.h"

UA_Boolean running = UA_TRUE;

void stopHandler(int sign) {
	running = UA_FALSE;
}

UA_Int32 serverCallback(void * arg) {
	char *name = (char *) arg;
	printf("%s does whatever servers do\n",name);

	Namespace* ns0 = (Namespace*)UA_indexedList_find(appMockup.namespaces, 0)->payload;
	UA_Int32 retval;
	const UA_Node * node;
	UA_ExpandedNodeId serverStatusNodeId; NS0EXPANDEDNODEID(serverStatusNodeId, 2256);
	retval = Namespace_get(ns0, &serverStatusNodeId.nodeId, &node);
	if(retval == UA_SUCCESS){
		((UA_ServerStatusDataType*)(((UA_VariableNode*)node)->value.data))->currentTime = UA_DateTime_now();
	}

	return UA_SUCCESS;
}


int main(int argc, char** argv) {

	/* gets called at ctrl-c */
	signal(SIGINT, stopHandler);
	
	appMockup_init();
	NL_data* nl = NL_init(&NL_Description_TcpBinary, 16664);
	UA_String endpointUrl;
	UA_String_copycstring("no endpoint url",&endpointUrl);
	SL_ChannelManager_init(10,36000,244,2,&endpointUrl);
	UA_SessionManager_init(10,3600000,25);
	struct timeval tv = {1, 0}; // 1 second
	NL_msgLoop(nl, &tv, serverCallback, argv[0], &running);

}
