/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h> 
#include <signal.h>
#include <errno.h> // errno, EINTR
#include <string.h>

#ifdef NOT_AMALGATED
    #include "ua_types.h"
    #include "ua_server.h"
#else
    #include "open62541.h"
#endif

// provided by the user, implementations available in the /examples folder
#include "logger_stdout.h"
#include "networklayer_tcp.h"

UA_Boolean running = 1;
UA_Logger logger;

static void stopHandler(int sign) {
    printf("Received Ctrl-C\n");
	running = 0;
}

static UA_ByteString loadCertificate(void) {
    UA_ByteString certificate = UA_STRING_NULL;
	FILE *fp = NULL;
	//FIXME: a potiential bug of locating the certificate, we need to get the path from the server's config
	fp=fopen("server_cert.der", "rb");

	if(!fp) {
        errno = 0; // we read errno also from the tcp layer...
        return certificate;
    }

    fseek(fp, 0, SEEK_END);
    certificate.length = ftell(fp);
    certificate.data = malloc(certificate.length*sizeof(UA_Byte));
	if(!certificate.data)
		return certificate;

    fseek(fp, 0, SEEK_SET);
    if(fread(certificate.data, sizeof(UA_Byte), certificate.length, fp) < (size_t)certificate.length)
        UA_ByteString_deleteMembers(&certificate); // error reading the cert
    fclose(fp);

    return certificate;
}

static void testCallback(UA_Server *server, void *data) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "testcallback");
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */

	UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    logger = Logger_Stdout_new();
    UA_Server_setLogger(server, logger);
    UA_ByteString certificate = loadCertificate();
    UA_Server_setServerCertificate(server, certificate);
    UA_ByteString_deleteMembers(&certificate);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    UA_WorkItem work = {.type = UA_WORKITEMTYPE_METHODCALL,
                        .work.methodCall = {.method = testCallback, .data = NULL} };
    UA_Server_addRepeatedWorkItem(server, &work, 2000, NULL); // call every 2 sec

	// add a variable node to the adresspace
    UA_Variant *myIntegerVariant = UA_Variant_new();
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(myIntegerVariant, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer"); /* UA_NODEID_NULL would assign a random free nodeid */
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerVariant, myIntegerName,
                              myIntegerNodeId, parentNodeId, parentReferenceNodeId);

/*
#ifdef BENCHMARK
    UA_UInt32 nodeCount = 50;
    char str[32];
    for(UA_UInt32 i = 0;i<nodeCount;i++) {
        // scalar
        void *data = UA_new(&UA_TYPES[i]);
        UA_Variant *variant = UA_Variant_new();
        UA_Variant_setScalar(variant, data, &UA_TYPES[i]);
        sprintf(str,"%d",i);
        UA_QualifiedName nodeName = UA_QUALIFIEDNAME(1, str);
        UA_NodeId id = UA_NODEID_NUMERIC(1, 100 + i);
        UA_Server_addVariableNode(server, variant, nodeName, id,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));

        // array
        data = UA_Array_new(&UA_TYPES[i], 10);
        variant = UA_Variant_new();
        UA_Variant_setArray(variant, data, 10, &UA_TYPES[i]);
        sprintf(str,"array of %d",i);
        nodeName = UA_QUALIFIEDNAME(1, str);
        id = UA_NODEID_NUMERIC(1, 200 + i);
        UA_Server_addVariableNode(server, variant, nodeName, id,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
    }
#endif
*/
    UA_StatusCode retval = UA_Server_run(server, 1, &running);
	UA_Server_delete(server);

	return retval;
}
