/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include "open62541.h"
#include "custom_datatype.h"

int main(void) {
    UA_ClientConfig config = UA_ClientConfig_default;

    /* Make your custom datatype known to the stack */
    UA_DataType types[1];
    types[0] = PointType;
    config.customDataTypes = types;
    config.customDataTypesSize = 1;

    UA_Client *client = UA_Client_new(config);
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

    /* Clean up */
    UA_Variant_deleteMembers(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return UA_STATUSCODE_GOOD;
}
