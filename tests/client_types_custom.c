// client_types_custom.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "open62541.h"

typedef struct {
	UA_Float x;
	UA_Float y;
	UA_Float z;
} Point;

/* The datatype description for the Point datatype */
#define padding_y offsetof(Point,y) - offsetof(Point,x) - sizeof(UA_Float)
#define padding_z offsetof(Point,z) - offsetof(Point,y) - sizeof(UA_Float)

static UA_DataTypeMember members[3] = {
	{ "x", UA_TYPES_FLOAT, 0,         true, false },
	{ "y", UA_TYPES_FLOAT, padding_y, true, false },
	{ "z", UA_TYPES_FLOAT, padding_z, true, false },
};

static const UA_DataType customType[1] = {
	{ "Point",{ 1, UA_NODEIDTYPE_NUMERIC,{ 1 } }, sizeof(Point), 0, 3, false, true, false, 0, members
	},
};

int main(void) {
	UA_ClientConfig config = UA_ClientConfig_standard;
	
	config.customDataTypes = customType;
	config.customDataTypesSize = 1;

	UA_Client *client = UA_Client_new(config);
	UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
	if (retval != UA_STATUSCODE_GOOD) {
		UA_Client_delete(client);
		return (int)retval;
	}
	
	UA_Variant value; /* Variants can hold scalar values and arrays of any type */
	UA_Variant_init(&value);
	
	 UA_NodeId nodeId =
		UA_NODEID_STRING(1, "3D.Point");

	retval = UA_Client_readValueAttribute(client, nodeId, &value);
			
	if (retval == UA_STATUSCODE_GOOD) {
		Point *p = (Point *)value.data;
		printf("Point = %f, %f, %f \n", p->x, p->y, p->z);
	}
	/* Clean up */
	UA_Variant_deleteMembers(&value);
	UA_Client_delete(client); /* Disconnects the client internally */
	return UA_STATUSCODE_GOOD;
}
