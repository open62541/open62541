/*
 ============================================================================
 Name        : opcuaServer.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : lala
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

int main(int argc, char** argv) {
	NL_data* nl = NL_init(&NL_Description_TcpBinary,16664);
	// NL_data* nl = NL_init(&NL_Description_TcpBinary,16664,NL_THREADINGTYPE_SINGLE);
	appMockup_init();

	struct timeval tv = {2, 0}; // 2 seconds
	NL_msgLoop(nl, &tv,serverCallback,argv[0]);
}
