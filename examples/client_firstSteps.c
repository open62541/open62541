/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include <inttypes.h>

#ifdef UA_NO_AMALGAMATION
#include "ua_client.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#else
#include "open62541.h"
#endif

int main(void) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:16664");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    /* Read the value attribute of the node. UA_Client_readValueAttribute is a
     * wrapper for the raw read service available as UA_Client_Service_read. */
    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

    #define NS0_CURRENT_TIME 2258
    const UA_NodeId nodeId = UA_NODEID_NUMERIC(0, NS0_CURRENT_TIME);

    retval = UA_Client_readValueAttribute(client, nodeId, &value);
    if(retval == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime*)value.data;
        UA_String string_date = UA_DateTime_toString(raw_date);
        printf("string date is: %.*s\n", (int)string_date.length, string_date.data);
        UA_String_deleteMembers(&string_date);
    }

    /* Clean up */
    UA_Variant_deleteMembers(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return UA_STATUSCODE_GOOD;
}
