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

UA_Boolean running = 1;
UA_Logger logger;

/*************************/
/* Read-only temperature */
/*************************/
FILE* temperatureFile = NULL;
static UA_StatusCode readTemperature(const void *handle, UA_Boolean sourceTimeStamp, UA_DataValue *value) {
	UA_Double* currentTemperature = UA_Double_new();

	if(!currentTemperature)
		return UA_STATUSCODE_BADOUTOFMEMORY;

	if (fseek(temperatureFile, 0, SEEK_SET))
	{
		puts("Error seeking to start of file");
		exit(1);
	}

	if(fscanf(temperatureFile, "%lf", currentTemperature) != 1){
		printf("Can not parse temperature!\n");
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

/*************************/
/* Read-write status led */
/*************************/
pthread_rwlock_t ledStatusLock;
FILE* triggerFile = NULL;
FILE* ledFile = NULL;
UA_Boolean ledStatus = 0;

static UA_StatusCode readLedStatus(const void *handle, UA_Boolean sourceTimeStamp, UA_DataValue *value) {
    /* In order to reduce blocking time, we could alloc memory for every read
       and return a copy of the data. */
    pthread_rwlock_rdlock(&ledStatusLock);
    value->value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
    value->value.arrayLength = 1;
    value->value.dataPtr = &ledStatus;
    value->value.arrayDimensionsSize = -1;
    value->value.arrayDimensions = NULL;
    value->hasVariant = UA_TRUE;
    if(sourceTimeStamp) {
        value->sourceTimestamp = UA_DateTime_now();
        value->hasSourceTimestamp = UA_TRUE;
    }
    return UA_STATUSCODE_GOOD;
}

static void releaseLedStatus(const void *handle, UA_DataValue *value) {
    /* If we allocated memory for a specific read, free the content of the
       variantdata. */
    value->value.arrayLength = -1;
    value->value.dataPtr = NULL;
    pthread_rwlock_unlock(&ledStatusLock);
}

static UA_StatusCode writeLedStatus(const void *handle, const UA_Variant *data) {
    pthread_rwlock_wrlock(&ledStatusLock);
    if(data->dataPtr)
        ledStatus = *(UA_Boolean*)data->dataPtr;

	if (fseek(triggerFile, 0, SEEK_SET))
	{
		puts("Error seeking to start of led file");
	}
    if(ledStatus == 1){
    	fprintf(ledFile, "%s", "1");
    } else {
    	fprintf(ledFile, "%s", "0");
    }
    pthread_rwlock_unlock(&ledStatusLock);
    return UA_STATUSCODE_GOOD;
}

static void stopHandler(int sign) {
	printf("Received Ctrl-C\n");
	running = 0;
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */
	pthread_rwlock_init(&ledStatusLock, 0);

	UA_Server *server = UA_Server_new();
	logger = Logger_Stdout_new();
	UA_Server_setLogger(server, logger);
	UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

	if(!(temperatureFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r"))){
		printf("Can not open temperature file, no temperature node will be added\n");
	} else {
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
	}

	if (	!(triggerFile = fopen("/sys/class/leds/led0/trigger", "w"))
			|| 	!(ledFile = fopen("/sys/class/leds/led0/brightness", "w"))) {
		printf("Can not open trigger or led file, no led node will be added\n");
	} else {
		//setting led mode to manual
		fprintf(triggerFile, "%s", "none");

		//turning off led initially
		fprintf(ledFile, "%s", "1");

		// add node with the device status data source
		UA_DataSource ledStatusDataSource = (UA_DataSource)
		        		{.handle = NULL,
						.read = readLedStatus,
						.release = releaseLedStatus,
						.write = writeLedStatus};
		UA_QualifiedName statusName;
		UA_QUALIFIEDNAME_ASSIGN(statusName, "status led");
		UA_Server_addDataSourceVariableNode(server, ledStatusDataSource, &UA_NODEID_NULL, &statusName,
				&UA_NODEID_STATIC(0, UA_NS0ID_OBJECTSFOLDER),
				&UA_NODEID_STATIC(0, UA_NS0ID_ORGANIZES));
	}


	UA_StatusCode retval = UA_Server_run(server, 1, &running);
	UA_Server_delete(server);

	if(temperatureFile)
		fclose(temperatureFile);

	if(triggerFile){
		if (fseek(triggerFile, 0, SEEK_SET))
		{
			puts("Error seeking to start of led file");
		}

		//setting led mode to default
		fprintf(triggerFile, "%s", "mmc0");
		fclose(triggerFile);
	}

	if(ledFile){
		fclose(ledFile);
	}

    pthread_rwlock_destroy(&ledStatusLock);

	return retval;
}
