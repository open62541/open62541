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
    //if(UA_Client_connect(client, UA_ConnectionConfig_standard, nl, "opc.tcp://localhost:48020") != UA_STATUSCODE_GOOD)
	if(UA_Client_connect(client, UA_ConnectionConfig_standard, nl, "opc.tcp://localhost:16664") != UA_STATUSCODE_GOOD)
    	return 0;

    UA_NodeId node;
    //node.namespaceIndex = 4;
    //node.identifierType = UA_NODEIDTYPE_STRING;
    //UA_String_copycstring("Demo.Static.Scalar.Int32", &node.identifier.string);
    node.namespaceIndex = 1;
    node.identifierType = UA_NODEIDTYPE_NUMERIC;
    node.identifier.numeric = 76;

    UA_ReadRequest read_req;
    UA_ReadRequest_init(&read_req);

    read_req.nodesToRead = UA_ReadValueId_new();
    read_req.nodesToReadSize = 1;
    read_req.nodesToRead[0].nodeId = node;
    read_req.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;
    UA_ReadResponse read_resp = UA_Client_read(client, &read_req);
    printf("the answer is: %i\n", *(UA_Int32*)read_resp.results[0].value.dataPtr);
    UA_ReadRequest_deleteMembers(&read_req);
    UA_ReadResponse_deleteMembers(&read_resp);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
}
