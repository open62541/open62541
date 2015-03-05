/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
#include <time.h>
#include "ua_types.h"

#include <stdio.h>
#include <stdlib.h> 
#include <signal.h>
#define __USE_XOPEN2K
#include <pthread.h>

// provided by the open62541 lib
#include "ua_server.h"

// provided by the user, implementations available in the /examples folder
#include "logger_stdout.h"
#include "networklayer_tcp.h"

FILE* temperatureFile;
UA_Boolean running = 1;
UA_Logger logger;

/*************************/
/* Read-only temperature */
/*************************/
static UA_StatusCode readTemperature(const void *handle, UA_DataValue *value) {
    UA_Double* currentTemperature = UA_Double_new();

    if(!currentTemperature)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if (fseek(temperatureFile, 0, SEEK_SET))
      {
        puts("Error seeking to start of file");
        exit(1);
      }

    if(fscanf(temperatureFile, "%lf", currentTemperature) != 1){
    	printf("Can not open parse temperature!\n");
    	exit(1);
    }

    *currentTemperature /= 1000.0;

    value->value.type = &UA_TYPES[UA_TYPES_DOUBLE];
    value->value.arrayLength = 1;
    value->value.dataPtr = currentTemperature;
    value->value.arrayDimensionsSize = -1;
    value->value.arrayDimensions = NULL;
    value->hasVariant = UA_TRUE;
    return UA_STATUSCODE_GOOD;
}

static void releaseTemperature(const void *handle, UA_DataValue *value) {
    UA_Double_delete((UA_Double*)value->value.dataPtr);
}

static void stopHandler(int sign) {
    printf("Received Ctrl-C\n");
	running = 0;
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */

	UA_Server *server = UA_Server_new();
    logger = Logger_Stdout_new();
    UA_Server_setLogger(server, logger);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    if(!(temperatureFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r"))){
    	printf("Can not open temperature file!\n");
    	exit(1);
    }

    // add node with the datetime data source
    UA_DataSource temperatureDataSource = (UA_DataSource)
        {.handle = NULL,
         .read = readTemperature,
         .release = releaseTemperature,
         .write = NULL};
    UA_QualifiedName dateName;
    UA_QUALIFIEDNAME_ASSIGN(dateName, "cpu temperature");
    UA_Server_addDataSourceVariableNode(server, temperatureDataSource, &UA_NODEID_NULL, &dateName,
                                        &UA_NODEID_STATIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        &UA_NODEID_STATIC(0, UA_NS0ID_ORGANIZES));

    UA_StatusCode retval = UA_Server_run(server, 1, &running);
	UA_Server_delete(server);

    fclose(temperatureFile);


	return retval;
}
