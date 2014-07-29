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
<<<<<<< HEAD
#include "ua_stack_channel_manager.h"
#include "ua_stack_session_manager.h"

#ifdef RASPI
	#include "raspberrypi_io.h"
#endif
=======
>>>>>>> origin/master


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

	return UA_SUCCESS;
}


int main(int argc, char** argv) {
	appMockup_init();
	NL_data* nl = NL_init(&NL_Description_TcpBinary,16664);

	struct timeval tv = {1, 0}; // 1 second
<<<<<<< HEAD
#ifdef RASPI
  	Namespace* ns0 = (Namespace*)UA_indexedList_find(appMockup.namespaces, 0)->payload;
		const UA_Node *foundNode1 = UA_NULL;
		const UA_Node *foundNode2 = UA_NULL;
		const UA_Node *foundNode3 = UA_NULL;
		Namespace_Entry_Lock *lock;
		//node which should be filled with data (float value)
		UA_NodeId tmpNodeId1;
		UA_NodeId tmpNodeId2;
		UA_NodeId tmpNodeId3;

		tmpNodeId1.encodingByte = UA_NODEIDTYPE_TWOBYTE;
		tmpNodeId1.identifier.numeric = 108;
		tmpNodeId1.namespace =  0;

		tmpNodeId2.encodingByte = UA_NODEIDTYPE_TWOBYTE;
		tmpNodeId2.identifier.numeric = 109;
		tmpNodeId2.namespace =  0;

		tmpNodeId3.encodingByte = UA_NODEIDTYPE_TWOBYTE;
		tmpNodeId3.identifier.numeric = 110;
		tmpNodeId3.namespace =  0;
            	
			if(Namespace_get(ns0,&tmpNodeId1, &foundNode1,&lock) != UA_SUCCESS){
				_exit(1);
			}

			if(Namespace_get(ns0,&tmpNodeId2, &foundNode2,&lock) != UA_SUCCESS){
				_exit(1);
			}

			if(Namespace_get(ns0,&tmpNodeId3, &foundNode3,&lock) != UA_SUCCESS){
				_exit(1);
			}

		((UA_VariableNode *)foundNode1)->value.data = &sharedTemperature;
		((UA_VariableNode *)foundNode2)->value.data = &sharedLED1;
		((UA_VariableNode *)foundNode3)->value.data = &sharedLED2;

		pthread_t t;
		pthread_create(&t, NULL, &ioloop, UA_NULL);

	printf("raspi enabled \n");	
  	//pid_t i = fork();
	//printf("process id i=%d \n",i);
#endif    

	SL_ChannelManager_init(2,3600000, 873, 23, &nl->endpointUrl);
	UA_SessionManager_init(2,300000,5);

=======
>>>>>>> origin/master
  	NL_msgLoop(nl, &tv, serverCallback, argv[0]);

}
