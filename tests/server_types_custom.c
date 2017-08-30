// CustomDataTypeServer.cpp : Defines the entry point for the console application.
//
#include <signal.h>
#include "stdafx.h"
#include"CustomDataTypes.h"
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

static UA_DataTypeMember members1[1] = {
	{ "Day",UA_TYPES_INT32,0,true,false },
};
static const UA_DataType customType[2] = {
	{ "Point",{ 1, UA_NODEIDTYPE_NUMERIC,{ 1 } }, sizeof(Point), 0, 3, false, true, false, 0, members
	},
	{ "WeekDays",{ 1,UA_NODEIDTYPE_NUMERIC,{ 2 } },sizeof(WeekDays),UA_TYPES_INT32,1,false,true, UA_BINARY_OVERLAYABLE_INTEGER,0,
	members1
	}
};
UA_Boolean running = true;
static void stopHandler(int sig) {
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
	running = false;
}
static void add3PointDataType(UA_Server *server)
{
	UA_DataTypeAttributes dattr;
	UA_DataTypeAttributes_init(&dattr);
	dattr.description = UA_LOCALIZEDTEXT("en_US", "3D Point");
	dattr.displayName = UA_LOCALIZEDTEXT("en_US", "3D Point");

	UA_Server_addDataTypeNode(server, customType[0].typeId,
		UA_NODEID_NUMERIC(0,UA_NS0ID_STRUCTURE),
		UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
		UA_QUALIFIEDNAME(1, "3D.Point"),
		dattr,
		NULL, NULL
		);

}
static void
add3DPointVariable(UA_Server *server)
{
	Point p;
	p.x = 3.0;
	p.y = 3.0;
	p.z = 3.0;
	UA_VariableAttributes  vattr;
	UA_VariableAttributes_init(&vattr);
	vattr.description = UA_LOCALIZEDTEXT("en_US", "3D Point");
	vattr.displayName = UA_LOCALIZEDTEXT("en_US", "3D Point");
	vattr.dataType = customType[0].typeId;
	vattr.valueRank = -1;
	UA_Variant_setScalar(&vattr.value, &p, &customType[0]);

	UA_Server_addVariableNode(server, UA_NODEID_STRING(1,"3D.Point"),
		UA_NODEID_NUMERIC(0,UA_NS0ID_OBJECTSFOLDER),
		UA_NODEID_NUMERIC(0,UA_NS0ID_ORGANIZES),
		UA_QUALIFIEDNAME(1,"3D.Point"),
		UA_NODEID_NULL,
		vattr,
		NULL,
		NULL
	);
}
int main()
{
	signal(SIGINT, stopHandler);
	signal(SIGTERM, stopHandler);

	UA_ServerConfig config = UA_ServerConfig_standard;
	config.customDataTypes =customType;
	config.customDataTypesSize = 2;
  
	UA_ServerNetworkLayer nl =
		UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4840);
	config.networkLayers = &nl;
	config.networkLayersSize = 1;
	UA_Server *server = UA_Server_new(config);
	
	add3PointDataType(server);
	add3DPointVariable(server);
	
  UA_Server_run(server, &running);

	UA_Server_delete(server);
	nl.deleteMembers(&nl);
	
    return 0;
}
