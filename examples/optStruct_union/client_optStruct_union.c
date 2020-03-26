/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/server.h>
#include <open62541/types.h>
#include "ua_types_encoding_binary.h"

#include "optStruct_union.h"
#include <stdlib.h>


int main(void) {
    /* Make your custom datatype known to the stack */
    UA_DataType types[2];
    types[0] = OptType;
    types[1] = UniType;

    /* Attention! Here the custom datatypes are allocated on the stack. So they
     * cannot be accessed from parallel (worker) threads. */
    UA_DataTypeArray customDataTypes = {NULL, 2, types};

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
    cc->customDataTypes = &customDataTypes;

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    UA_Variant optVar; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&optVar);

    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(1, "OptVar"), &optVar);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_ExtensionObject *oEo = (UA_ExtensionObject *) optVar.data;
        Opt o;
        memset(&o, 0, sizeof(Opt));
        size_t oOffset = 0;
        UA_ByteString oBs = oEo->content.encoded.body;
        UA_StatusCode st = UA_decodeBinary(&oBs, &oOffset, &o, &OptType, NULL);
        if(st == UA_STATUSCODE_GOOD){
            printf("Opt o: a = %d", o.a);
            if(o.hasB){
                printf(", b = %f", o.b);
            }
            printf("\n");
        }
    }

    UA_Variant uniVar; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&uniVar);

    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(1, "UniVar"), &uniVar);

    if(retval == UA_STATUSCODE_GOOD) {
        UA_ExtensionObject *uEo = (UA_ExtensionObject *) uniVar.data;
        Uni u;
        memset(&u, 0, sizeof(Uni));
        size_t uOffset = 0;
        UA_ByteString uBs = uEo->content.encoded.body;
        UA_StatusCode st = UA_decodeBinary(&uBs, &uOffset, &u, &UniType, NULL);
        if(st == UA_STATUSCODE_GOOD) {
            switch (u.switchField) {
                case 1:
                    printf("Uni u: x = %f\n", u.x);
                    break;
                case 2:
                    printf("Uni u: y = %s\n", u.y.data);
                    break;
            }
        }
    }

    /* Clean up */
    UA_Variant_clear(&optVar);
    UA_Variant_clear(&uniVar);
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
