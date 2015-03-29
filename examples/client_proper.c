#ifdef NOT_AMALGATED
    #include "ua_types.h"
    #include "ua_client.h"
#else
    #include "open62541.h"
#endif

#include "networklayer_tcp.h"


int main(int argc, char *argv[]) {
	UA_Client *client = UA_Client_new();
	UA_ClientNetworkLayer nl = ClientNetworkLayerTCP_new(UA_ConnectionConfig_standard);
    UA_Client_connect(client, UA_ConnectionConfig_standard, nl, "opc.tcp://localhost:16664");

    UA_ReadRequest read_req;
    UA_ReadRequest_init(&read_req);
    read_req.nodesToRead = UA_ReadValueId_new();
    read_req.nodesToReadSize = 1;
    read_req.nodesToRead[0].nodeId = UA_NODEID_STATIC(1, 73);
    read_req.nodesToRead[0].attributeId = 13;
    UA_ReadResponse read_resp = UA_Client_read(client, &read_req);
    printf("answer statuscode: %i\n", read_resp.responseHeader.serviceResult);
    printf("answer value: %i\n", *(UA_Int32*)read_resp.results[0].value.dataPtr);
    UA_Client_delete(client);
}
