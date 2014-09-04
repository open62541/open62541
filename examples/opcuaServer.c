#include <stdio.h>

#ifndef WIN32
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>

#include "server/ua_server.h"
#include "logger_stdout.h"
#include "networklayer_tcp.h"

UA_Boolean running = UA_TRUE;

void stopHandler(int sign) {
	running = UA_FALSE;
}

void serverCallback(UA_Server *server) {
	printf("does whatever servers do\n");
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */

	UA_Server server;
	UA_String endpointUrl;
	UA_String_copycstring("no endpoint url",&endpointUrl);
	UA_Server_init(&server, &endpointUrl);
	Logger_Stdout_init(&server.logger);
	
	NetworklayerTCP* nl;
	NetworklayerTCP_new(&nl, UA_ConnectionConfig_standard, 16664);
	struct timeval callback_interval = {1, 0}; // 1 second
	UA_Int32 retval = NetworkLayerTCP_run(nl, &server, callback_interval,
										  serverCallback, &running);
	NetworklayerTCP_delete(nl);
	UA_Server_deleteMembers(&server);
	return retval == UA_SUCCESS ? 0 : retval;
}
