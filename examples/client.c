#ifdef NOT_AMALGATED
    #include "ua_types.h"
    #include "ua_client.h"
#else
    #include "open62541.h"
#endif

#include <stdio.h>
#include "networklayer_tcp.h"

int main(int argc, char *argv[]) {
	UA_Client *client = UA_Client_new();
	UA_ClientNetworkLayer nl = ClientNetworkLayerTCP_new(UA_ConnectionConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, UA_ConnectionConfig_standard, nl,
                                             "opc.tcp://localhost:16664");
	if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
    	return retval;
    }

    UA_ReadRequest req;
    UA_ReadRequest_init(&req);
    req.nodesToRead = UA_ReadValueId_new();
    req.nodesToReadSize = 1;
    req.nodesToRead[0].nodeId = UA_NODEID_STRING(1, "the.answer"); /* assume this node exists */
    req.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadResponse resp = UA_Client_read(client, &req);
    if(resp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
       resp.resultsSize > 0 && resp.results[0].hasValue &&
       resp.results[0].value.data /* an empty array returns a null-ptr */ &&
       resp.results[0].value.type == &UA_TYPES[UA_TYPES_INT32])
        printf("the answer is: %i\n", *(UA_Int32*)resp.results[0].value.data);

    UA_ReadRequest_deleteMembers(&req);
    UA_ReadResponse_deleteMembers(&resp);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return UA_STATUSCODE_GOOD;
}

