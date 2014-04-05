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

#include "UA_stack.h"


int main(void) {
	UA_Stack_init(&UA_TransportLayerDescriptorTcpBinary,16664);
	while (UA_TRUE) {
		sleep(10);
	}
	return EXIT_SUCCESS;
}
