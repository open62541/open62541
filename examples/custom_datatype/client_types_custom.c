/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <stdlib.h>
#include <stdio.h>

#define STRING_BUFFER_SIZE 20

int main(void) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    /* Get the description of the unknown types from the server */
    UA_DataTypeArray *newTypes;
    UA_Client_getRemoteDataTypes(client, 0, NULL, &newTypes);
    cc->customDataTypes = newTypes;

    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

    UA_NodeId nodeId = UA_NODEID_STRING(1, "3D.Point");
    retval = UA_Client_readValueAttribute(client, nodeId, &value);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_String out = UA_STRING_NULL;;
        UA_print(value.data, value.type, &out);
        printf("Point = %*s\n", (int)out.length, out.data);
        UA_String_clear(&out);
    }

    UA_Variant_clear(&value);
    UA_Variant_init(&value);

    nodeId = UA_NODEID_STRING(1, "Temp.Measurement");
    retval = UA_Client_readValueAttribute(client, nodeId, &value);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_String out = UA_STRING_NULL;;
        UA_print(value.data, value.type, &out);
        printf("Measurement = %*s\n", (int)out.length, out.data);
        UA_String_clear(&out);
    }
    UA_Variant_clear(&value);
    UA_Variant_init(&value);

    /* nodeId = UA_NODEID_STRING(1, "Optstruct.Value"); */
    /* retval = UA_Client_readValueAttribute(client, nodeId, &value); */
    /* if(retval == UA_STATUSCODE_GOOD) { */
    /*     UA_String out = UA_STRING_NULL;; */
    /*     UA_print(value.data, value.type, &out); */
    /*     printf("OptStruct = %s\n", out.data); */
    /*     UA_String_clear(&out); */
    /* } */
    /* UA_Variant_clear(&value); */
    /* UA_Variant_init(&value); */

    /* nodeId = UA_NODEID_STRING(1, "Union.Value"); */
    /* retval = UA_Client_readValueAttribute(client, nodeId, &value); */
    /* if(retval == UA_STATUSCODE_GOOD) { */
    /*     UA_String out = UA_STRING_NULL;; */
    /*     UA_print(value.data, value.type, &out); */
    /*     printf("Union = %s\n", out.data); */
    /*     UA_String_clear(&out); */
    /* } */

    /* Clean up */
    UA_Variant_clear(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
