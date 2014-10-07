/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <stdio.h>
#include <stdlib.h> 
#ifdef WIN32
  #include "winsock2.h"
#else
  #include <sys/mman.h>
  #include <sys/wait.h>
  #include <unistd.h>
  #include <sys/time.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h> // errno, EINTR
#include "ua_server.h"
#include "logger_stdout.h"
#include "networklayer_tcp.h"
#include "ua_namespace_0.h"

UA_Boolean running = UA_TRUE;

void stopHandler(int sign) {
	running = UA_FALSE;
}

void serverCallback(UA_Server *server) {
    //	printf("does whatever servers do\n");
}

UA_ByteString loadCertificate() {
    UA_ByteString certificate = UA_STRING_NULL;
	FILE *fp = UA_NULL;
	//FIXME: a potiential bug of locating the certificate, we need to get the path from the server's config
	fp=fopen("localhost.der", "rb");

	if(!fp) {
        errno = 0; // otherwise we think sth went wrong on the tcp socket level
        return certificate;
    }

    fseek(fp, 0, SEEK_END);
    certificate.length = ftell(fp);
    certificate.data = malloc(certificate.length*sizeof(UA_Byte));

    fseek(fp, 0, SEEK_SET);
    if(fread(certificate.data, sizeof(UA_Byte), certificate.length, fp) < (size_t)certificate.length)
        UA_ByteString_deleteMembers(&certificate); // error reading the cert
    fclose(fp);

    return certificate;
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */

	UA_Server server;
	UA_String endpointUrl;
	UA_String_copycstring("no endpoint url",&endpointUrl);
	UA_Server_init(&server, &endpointUrl);
	Logger_Stdout_init(&server.logger);
    server.serverCertificate = loadCertificate();

#ifdef BENCHMARK
    UA_UInt32 nodeCount = 500;
    UA_VariableNode *tmpNode;

    UA_ExpandedNodeId objectNodeId;
    UA_ExpandedNodeId_init(&objectNodeId);
    objectNodeId.nodeId.identifier.numeric = 85;

    UA_NodeId hasComponentReferenceId;
    UA_NodeId_init(&hasComponentReferenceId);
    hasComponentReferenceId.identifier.numeric = 47;

    UA_Int32 data = 42;
    char str[15];
    for(UA_UInt32 i = 0;i<nodeCount;i++) {
        UA_VariableNode_new(&tmpNode);
        sprintf(str,"%d",i);
        UA_QualifiedName_copycstring(str,&tmpNode->browseName);
        UA_LocalizedText_copycstring(str,&tmpNode->displayName);
        UA_LocalizedText_copycstring("integer value", &tmpNode->description);
        tmpNode->nodeId.identifier.numeric = 19000+i;
        tmpNode->nodeClass = UA_NODECLASS_VARIABLE;
        //tmpNode->valueRank = -1;
        tmpNode->value.vt = &UA_[UA_INT32];
        tmpNode->value.storage.data.dataPtr = &data;
        tmpNode->value.storageType = UA_VARIANT_DATA_NODELETE;
        tmpNode->value.storage.data.arrayLength = 1;
        UA_Server_addNode(&server, (UA_Node**)&tmpNode, &objectNodeId, &hasComponentReferenceId);
    }
#endif
	
	#define PORT 16664
	NetworklayerTCP* nl;
	NetworklayerTCP_new(&nl, UA_ConnectionConfig_standard, PORT);
	printf("Server started, connect to to opc.tcp://127.0.0.1:%i\n", PORT);
	struct timeval callback_interval = {1, 0}; // 1 second
	UA_Int32 retval = NetworkLayerTCP_run(nl, &server, callback_interval,
										  serverCallback, &running);
	UA_Server_deleteMembers(&server);
	NetworklayerTCP_delete(nl);
    UA_String_deleteMembers(&endpointUrl);
	return retval == UA_SUCCESS ? 0 : retval;
}
