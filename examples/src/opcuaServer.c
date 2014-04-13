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

#include "networklayer.h"

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
	NL_data* nl = NL_init(&NL_Description_TcpBinary,16664,NL_THREADINGTYPE_SINGLE);
	struct timeval tv = {2, 0}; // 2 seconds
	NL_msgLoop(nl, &tv,serverCallback,argv[0]);
}
