/*
 ============================================================================
 Name        : opcuaServer.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "ua_stack.h"


int main(int argc, char** argv) {
	UA_Stack_init(&UA_TransportLayerDescriptorTcpBinary,16664,UA_STACK_MULTITHREADED);
	while (UA_TRUE) {
		printf("%s does whatever servers do\n",argv[0]);
		sleep(2);
	}
	return EXIT_SUCCESS;
}
