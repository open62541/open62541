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
#include <unistd.h>
#include "networklayer.h"

int main(int argc, char** argv) {
	NL_init(&NL_Description_TcpBinary,16664);
	while (UA_TRUE) {
		printf("%s does whatever servers do\n",argv[0]);
		sleep(2);
	}
	return EXIT_SUCCESS;
}
