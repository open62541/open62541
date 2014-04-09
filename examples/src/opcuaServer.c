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
#include <pthread.h> // to get the type for the

UA_Int32 serverCallback(void * arg) {
	char *name = (char *) arg;
	printf("%s does whatever servers do\n",name);
	return UA_SUCCESS;
}

// necessary for the linker
int pthread_create(pthread_t* newthread, const pthread_attr_t* attr, void *(*start) (void *), void * arg) {
	perror("this routine should be never called in single-threaded mode\n");
	exit(1);
}

int main(int argc, char** argv) {
	struct timeval tv = {2, 0}; // 2 seconds
	UA_Stack_init(&UA_TransportLayerDescriptorTcpBinary,16664,UA_STACK_SINGLETHREADED);
	UA_Stack_msgLoop(&tv,serverCallback,argv[0]);
}
