//This file contains source-code that is discussed in a tutorial located here:
//http://open62541.org/doc/sphinx/tutorial_firstStepsClient.html

#include <stdio.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

int main(void) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard, Logger_Stdout);
    UA_StatusCode retval = UA_Client_connect(client, ClientNetworkLayerTCP_connect, "opc.tcp://localhost:16664");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    //variables to store data
    UA_DateTime raw_date = 0;
    UA_String* string_date = UA_String_new();

    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_Array_new(&UA_TYPES[UA_TYPES_READVALUEID], 1);
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_NUMERIC(0, 2258);
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    UA_ReadResponse rResp = UA_Client_read(client, &rReq);
    if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
            rResp.resultsSize > 0 && rResp.results[0].hasValue &&
            UA_Variant_isScalar(&rResp.results[0].value) &&
            rResp.results[0].value.type == &UA_TYPES[UA_TYPES_DATETIME]) {
        raw_date = *(UA_DateTime*)rResp.results[0].value.data;
        printf("raw date is: %" PRId64 "\n", raw_date);
        UA_DateTime_toString(raw_date, string_date);
        printf("string date is: %.*s\n", string_date->length, string_date->data);
    }

    UA_ReadRequest_deleteMembers(&rReq);
    UA_ReadResponse_deleteMembers(&rResp);
    UA_String_delete(string_date);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}
