#ifdef NOT_AMALGATED
    #include "ua_types.h"
    #include "ua_client.h"
#else
    #include "open62541.h"
#endif

int main(int argc, char *argv[]) {
	UA_Client *client = UA_Client_new();
	free(client);
}
