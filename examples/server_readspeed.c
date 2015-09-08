/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <signal.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

#include <time.h>
#include "ua_types_generated.h"
#include "server/ua_services.h"
#include "ua_types_encoding_binary.h"

UA_Boolean running = 1;
UA_Logger logger;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    logger = Logger_Stdout_new();
    UA_Server_setLogger(server, logger);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    /* add a variable node to the address space */
    UA_Variant *myIntegerVariant = UA_Variant_new();
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(myIntegerVariant, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    //NOTE: the link between myInteger and the value of the node is lost here, you can safely reuse myInteger
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer"); /* UA_NODEID_NULL would assign a random free nodeid */
    UA_LocalizedText myIntegerBrowseName = UA_LOCALIZEDTEXT("en_US","the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);

    UA_Server_addVariableNode(server, myIntegerNodeId, myIntegerName, myIntegerBrowseName, myIntegerBrowseName, 0, 0,
                              parentNodeId, parentReferenceNodeId, myIntegerVariant, NULL);

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId rvi;
    rvi.nodeId = myIntegerNodeId;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.indexRange = UA_STRING_NULL;
    rvi.dataEncoding = UA_QUALIFIEDNAME(0, "DefaultBinary");
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToReadSize = 1;
    request.nodesToRead = &rvi;

    UA_ByteString request_msg;
    UA_ByteString response_msg;
    UA_ByteString_newMembers(&request_msg, 1000);
    UA_ByteString_newMembers(&response_msg, 1000);
    size_t offset = 0;
    UA_encodeBinary(&request, &UA_TYPES[UA_TYPES_READREQUEST], &request_msg, &offset);
    
    clock_t begin, end;
    begin = clock();

    UA_ReadRequest rq;
    UA_ReadResponse rr;

    for(int i = 0; i < 600000; i++) {
        UA_ReadRequest_init(&rq);
        UA_ReadResponse_init(&rr);

        offset = 0;
        UA_decodeBinary(&request_msg, &offset, &rq, &UA_TYPES[UA_TYPES_READREQUEST]);

        Service_Read(server, &adminSession, &rq, &rr);

        offset = 0;
        UA_encodeBinary(&rr, &UA_TYPES[UA_TYPES_READRESPONSE], &response_msg, &offset);

        UA_ReadRequest_deleteMembers(&rq);
        UA_ReadResponse_deleteMembers(&rr);
    }

    end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);

    UA_ByteString_deleteMembers(&request_msg);
    UA_ByteString_deleteMembers(&response_msg);

    UA_StatusCode retval = UA_Server_run(server, 1, &running);
    UA_Server_delete(server);

    return retval;
}
