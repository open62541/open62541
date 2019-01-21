/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifdef UA_ENABLE_AMALGAMATION
#include "open62541.h"
#else
#include "ua_client.h"
#include "ua_client_highlevel.h"
#include "ua_config_default.h"
#include "ua_log_stdout.h"
#endif

#include "datatypes_generated.h"

int main(void) {
    UA_ClientConfig config = UA_ClientConfig_default;

    /* Attention! Here the custom datatypes are allocated on the stack. So they
     * cannot be accessed from parallel (worker) threads. */
    UA_DataTypeArray customDataTypes = {config.customDataTypes, 1, DATATYPES};
    config.customDataTypes = &customDataTypes;

    UA_Client *client = UA_Client_new(config);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

    UA_NodeId nodeId = UA_NODEID_STRING(1, "OptionObject");

    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_OptionObject *o = (UA_OptionObject *)value.data;
        printf("Type with optional fields: (optionOne: %d, optionTwo: %d, byteOne: %X, byteTwo: %X) \n",
            o->optionOne,
            o->optionTwo,
            o->optionalByteOne,
            o->optionalByteTwo);
    } else {
        printf("Value attribute read failed with status code: %d\n", retval);
    }

    /* Clean up */
    UA_Variant_clear(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return UA_STATUSCODE_GOOD;
}
