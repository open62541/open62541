#include "open62541.h"

struct UA_NetworkLayer;
typedef struct UA_NetworkLayer UA_NetworkLayer;

struct UA_Application;
typedef struct UA_Application UA_Application;

typedef struct UA_ServerConfiguration {
	UA_String certificatePublic;
	UA_String certificatePrivate;
	UA_Networklayer *networklayer;
} UA_ServerConfiguration;

typedef struct UA_Server {
	UA_ServerConfiguration configuration;
	UA_Int32 applicationsSize;
	UA_Application *applications;
} UA_Server;
