/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#include <stdlib.h>

#include "custom_datatype.h"

#define STRING_BUFFER_SIZE 20

int main(void) {
    /* Make your custom datatype known to the stack */
    UA_DataType types[4];
    types[0] = PointType;
    types[1] = MeasurementType;
    types[2] = OptType;
    types[3] = UniType;

    /* Attention! Here the custom datatypes are allocated on the stack. So they
     * cannot be accessed from parallel (worker) threads. */
    UA_DataTypeArray customDataTypes = {NULL, 4, types};

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(cc);
    cc->customDataTypes = &customDataTypes;

    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }

    UA_Variant value; /* Variants can hold scalar values and arrays of any type */
    UA_Variant_init(&value);

     UA_NodeId nodeId =
        UA_NODEID_STRING(1, "3D.Point");

    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if(retval == UA_STATUSCODE_GOOD) {
        Point *p = (Point *)value.data;
        printf("Point = %f, %f, %f \n", p->x, p->y, p->z);
    }

    UA_Variant_clear(&value);
    UA_Variant_init(&value);

    nodeId =
        UA_NODEID_STRING(1, "Temp.Measurement");

    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if(retval == UA_STATUSCODE_GOOD) {
        Measurements *m = (Measurements *) value.data;
        char description[STRING_BUFFER_SIZE];
        memcpy(description, m->description.data, m->description.length);
        description[m->description.length] = '\0';
        printf("Description of Series: %s\n", description);
        for(size_t i = 0; i < m->measurementSize; ++i) {
            printf("Value %i : %f\n", (UA_Int32) i, m->measurement[i]);
        }
    }
    UA_Variant_clear(&value);
    UA_Variant_init(&value);

    nodeId =
        UA_NODEID_STRING(1, "Optstruct.Value");

    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if(retval == UA_STATUSCODE_GOOD) {
        Opt *o = (Opt *) value.data;
        printf("Mandatory member 'a': %hd, Not contained optional member (ptr) 'b': %p, Contained optional member 'c': %f\n", o->a,  (void *) o->b, *o->c);
    }
    UA_Variant_clear(&value);
    UA_Variant_init(&value);

    nodeId =
        UA_NODEID_STRING(1, "Union.Value");

    retval = UA_Client_readValueAttribute(client, nodeId, &value);

    if(retval == UA_STATUSCODE_GOOD) {
        Uni *u = (Uni *) value.data;
        char message[STRING_BUFFER_SIZE];
        memcpy(message, u->fields.optionB.data, u->fields.optionB.length);
        message[u->fields.optionB.length] = '\0';
        printf("Union member selection: %i , member content: %s \n", u->switchField, message);
    }

    /* Clean up */
    UA_Variant_clear(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return EXIT_SUCCESS;
}
