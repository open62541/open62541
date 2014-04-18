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
#include "ua_application.h"

UA_Int32 serverCallback(void * arg) {
	char *name = (char *) arg;
	printf("%s does whatever servers do\n",name);
	return UA_SUCCESS;
}

int main(int argc, char** argv) {
	appMockup_init();
	NL_data* nl = NL_init(&NL_Description_TcpBinary,16664);

	struct timeval tv = {20, 0}; // 20 seconds
	NL_msgLoop(nl, &tv,serverCallback,argv[0]);
}
