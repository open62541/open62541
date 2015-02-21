/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
#include <time.h>
#include "ua_types.h"

#include <stdio.h>
#include <stdlib.h> 
#include <signal.h>
#include <errno.h> // errno, EINTR

// provided by the open62541 lib
#include "ua_server.h"

// provided by the user, implementations available in the /examples folder
#include "logger_stdout.h"
#include "networklayer_tcp.h"

// data source
static UA_StatusCode readTimeData(const void *handle, UA_VariantData* data) {
    UA_DateTime *time = UA_DateTime_new();
    if(!time)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *time = UA_DateTime_now();
    data->arrayLength = 1;
    data->dataPtr = time;
    data->arrayDimensionsSize = -1;
    data->arrayDimensions = UA_NULL;
    return UA_STATUSCODE_GOOD;
}

static void releaseTimeData(const void *handle, UA_VariantData* data) {
    UA_DateTime_delete((UA_DateTime*)data->dataPtr);
}

static void destroyTimeDataSource(const void *handle) {
    return;
}

UA_Boolean running = 1;

static void stopHandler(int sign) {
    printf("Received Ctrl-C\n");
	running = 0;
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */

	UA_Server *server = UA_Server_new();
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    // add node with a callback to the userspace
    UA_Variant *myDateTimeVariant = UA_Variant_new();
    myDateTimeVariant->storageType = UA_VARIANT_DATASOURCE;
    myDateTimeVariant->storage.datasource = (UA_VariantDataSource)
        {.handle = UA_NULL, .read = readTimeData, .release = releaseTimeData,
         .write = (UA_StatusCode (*)(const void*, const UA_VariantData*))UA_NULL,
         .destroy = destroyTimeDataSource};
    myDateTimeVariant->type = &UA_TYPES[UA_TYPES_DATETIME];
    myDateTimeVariant->typeId = UA_NODEID_STATIC(UA_TYPES_IDS[UA_TYPES_DATETIME],0);
    UA_QualifiedName myDateTimeName;
    UA_QUALIFIEDNAME_ASSIGN(myDateTimeName, "the time");
    UA_Server_addVariableNode(server, myDateTimeVariant, &UA_NODEID_NULL, &myDateTimeName,
                              &UA_NODEID_STATIC(UA_NS0ID_OBJECTSFOLDER,0),
                              &UA_NODEID_STATIC(UA_NS0ID_ORGANIZES,0));

    UA_StatusCode retval = UA_Server_run(server, 1, &running);
	UA_Server_delete(server);

	return retval;
}
