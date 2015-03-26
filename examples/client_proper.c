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

	free(client);
}
