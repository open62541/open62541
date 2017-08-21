/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* This example is just to see how fast we can process messages. The server does
   not open a TCP port. */

#include <time.h>
#include <stdio.h>

#include "ua_server.h"
#include "ua_config_standard.h"
#include "server/ua_services.h"
#include "ua_types_encoding_binary.h"

int main(int argc, char** argv) {
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    UA_Server *server = UA_Server_new(config);

    /* add a variable node to the address space */
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NULL, attr, NULL, NULL);

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    UA_ReadValueId rvi;
    rvi.nodeId = myIntegerNodeId;
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;
    rvi.indexRange = UA_STRING_NULL;
    rvi.dataEncoding = UA_QUALIFIEDNAME(0, "Default Binary");
    request.timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
    request.nodesToReadSize = 1;
    request.nodesToRead = &rvi;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ByteString request_msg;
    retval |= UA_ByteString_allocBuffer(&request_msg, 1000);
    UA_ByteString response_msg;
    retval |= UA_ByteString_allocBuffer(&response_msg, 1000);

    UA_Byte *pos = request_msg.data;
    const UA_Byte *end = &request_msg.data[request_msg.length];
    retval |= UA_encodeBinary(&request, &UA_TYPES[UA_TYPES_READREQUEST], &pos, &end, NULL, NULL);

    clock_t begin, finish;
    begin = clock();

    UA_ReadRequest rq;
    UA_ReadResponse rr;

    for(int i = 0; i < 1000000; i++) {
        size_t offset = 0;
        retval |= UA_decodeBinary(&request_msg, &offset, &rq, &UA_TYPES[UA_TYPES_READREQUEST], 0, NULL);

        UA_ReadResponse_init(&rr);
        Service_Read(server, &adminSession, &rq, &rr);

        pos = request_msg.data;
        retval |= UA_encodeBinary(&rr, &UA_TYPES[UA_TYPES_READRESPONSE], &pos, &end, NULL, NULL);

        UA_ReadRequest_deleteMembers(&rq);
        UA_ReadResponse_deleteMembers(&rr);
    }

    finish = clock();
    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);
    printf("retval is %i\n", retval);

    UA_ByteString_deleteMembers(&request_msg);
    UA_ByteString_deleteMembers(&response_msg);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}
