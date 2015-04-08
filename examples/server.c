/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h> 
#define __USE_XOPEN2K
#ifdef UA_MULTITHREADING
# include <pthread.h>
#endif

#ifdef NOT_AMALGATED
# include <time.h>
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

/****************************/
/* Server-related variables */
/****************************/

UA_Boolean running = 1;
UA_Logger logger;

/*************************/
/* Read-only data source */
/*************************/
static UA_StatusCode readTimeData(const void *handle, UA_Boolean sourceTimeStamp, UA_DataValue *value) {
	UA_DateTime *currentTime = UA_DateTime_new();
	if(!currentTime)
		return UA_STATUSCODE_BADOUTOFMEMORY;
	*currentTime = UA_DateTime_now();
	value->value.type = &UA_TYPES[UA_TYPES_DATETIME];
	value->value.arrayLength = -1;
	value->value.data = currentTime;
	value->value.arrayDimensionsSize = -1;
	value->value.arrayDimensions = NULL;
	value->hasValue = UA_TRUE;
	if(sourceTimeStamp) {
		value->hasSourceTimestamp = UA_TRUE;
		value->sourceTimestamp = *currentTime;
	}
	return UA_STATUSCODE_GOOD;
}

static void releaseTimeData(const void *handle, UA_DataValue *value) {
	UA_DateTime_delete((UA_DateTime*)value->value.data);
}

/*****************************/
/* Read-only CPU temperature */
/*      Only on Linux        */
/*****************************/
FILE* temperatureFile = NULL;
static UA_StatusCode readTemperature(const void *handle, UA_Boolean sourceTimeStamp, UA_DataValue *value) {
	UA_Double* currentTemperature = UA_Double_new();

	if(!currentTemperature)
		return UA_STATUSCODE_BADOUTOFMEMORY;

	fseek(temperatureFile, 0, SEEK_SET);

	if(fscanf(temperatureFile, "%lf", currentTemperature) != 1){
		UA_LOG_WARNING(logger, UA_LOGGERCATEGORY_USERLAND, "Can not parse temperature");
		exit(1);
	}

	*currentTemperature /= 1000.0;

	value->value.type = &UA_TYPES[UA_TYPES_DOUBLE];
	value->value.arrayLength = -1;
	value->value.data = currentTemperature;
	value->value.arrayDimensionsSize = -1;
	value->value.arrayDimensions = NULL;
	value->hasValue = UA_TRUE;
	return UA_STATUSCODE_GOOD;
}

static void releaseTemperature(const void *handle, UA_DataValue *value) {
	UA_Double_delete((UA_Double*)value->value.data);
}

/*************************/
/* Read-write status led */
/*************************/
#ifdef UA_MULTITHREADING
pthread_rwlock_t writeLock;
#endif
FILE* triggerFile = NULL;
FILE* ledFile = NULL;
UA_Boolean ledStatus = 0;

static UA_StatusCode readLedStatus(const void *handle, UA_Boolean sourceTimeStamp, UA_DataValue *value) {
	/* In order to reduce blocking time, we could alloc memory for every read
       and return a copy of the data. */
#ifdef UA_MULTITHREADING
	pthread_rwlock_rdlock(&writeLock);
#endif
	value->value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
	value->value.arrayLength = -1;
	value->value.data = &ledStatus;
	value->value.arrayDimensionsSize = -1;
	value->value.arrayDimensions = NULL;
	value->hasValue = UA_TRUE;
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
	value->value.data = NULL;
#ifdef UA_MULTITHREADING
	pthread_rwlock_unlock(&writeLock);
#endif
}

static UA_StatusCode writeLedStatus(const void *handle, const UA_Variant *data) {
#ifdef UA_MULTITHREADING
	pthread_rwlock_wrlock(&writeLock);
#endif
	if(data->data)
		ledStatus = *(UA_Boolean*)data->data;

	if(triggerFile)
		fseek(triggerFile, 0, SEEK_SET);

	if(ledFile){
		if(ledStatus == 1){
			fprintf(ledFile, "%s", "1");
		} else {
			fprintf(ledFile, "%s", "0");
		}
		fflush(ledFile);
	}
#ifdef UA_MULTITHREADING
	pthread_rwlock_unlock(&writeLock);
#endif
	return UA_STATUSCODE_GOOD;
}

static void printLedStatus(UA_Server *server, void *data) {
	UA_LOG_INFO(logger, UA_LOGGERCATEGORY_SERVER, ledStatus ? "LED is on" : "LED is off");
}

static void stopHandler(int sign) {
	printf("Received Ctrl-C\n");
	running = 0;
}

int main(int argc, char** argv) {
	signal(SIGINT, stopHandler); /* catches ctrl-c */
#ifdef UA_MULTITHREADING
	pthread_rwlock_init(&writeLock, 0);
#endif

	UA_Server *server = UA_Server_new();
	logger = Logger_Stdout_new();
	UA_Server_setLogger(server, logger);
	UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

	// print the status every 2 sec
	UA_WorkItem work = {.type = UA_WORKITEMTYPE_METHODCALL,
			.work.methodCall = {.method = printLedStatus, .data = NULL} };
	UA_Server_addRepeatedWorkItem(server, &work, 20000000, NULL);

	// add node with the datetime data source
	UA_DataSource dateDataSource = (UA_DataSource)
        {.handle = NULL,
		.read = readTimeData,
		.release = releaseTimeData,
		.write = NULL};
	const UA_QualifiedName dateName = UA_QUALIFIEDNAME(0, "current time");
	UA_Server_addDataSourceVariableNode(server, dateDataSource, dateName, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));

	if(!(temperatureFile = fopen("/sys/class/thermal/thermal_zone0/temp", "r"))){
		UA_LOG_WARNING(logger, UA_LOGGERCATEGORY_USERLAND, "[Linux specific] Can not open temperature file, no temperature node will be added");
	} else {
		// add node with the datetime data source
		UA_DataSource temperatureDataSource = (UA_DataSource)
    	    {.handle = NULL,
			.read = readTemperature,
			.release = releaseTemperature,
			.write = NULL};
		const UA_QualifiedName ledName = UA_QUALIFIEDNAME(0, "cpu temperature");
		UA_Server_addDataSourceVariableNode(server, temperatureDataSource, ledName, UA_NODEID_NULL, 
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));
	}

	if (	!(triggerFile = fopen("/sys/class/leds/led0/trigger", "w"))
		|| 	!(ledFile = fopen("/sys/class/leds/led0/brightness", "w"))) {
		UA_LOG_WARNING(logger, UA_LOGGERCATEGORY_USERLAND, "[Raspberry Pi specific] Can not open trigger or LED file (try to run server with sudo if on a Raspberry PI)");
		UA_LOG_WARNING(logger, UA_LOGGERCATEGORY_USERLAND, "An LED node will be added but no physical LED will be operated");
	} else {
		//setting led mode to manual
		fprintf(triggerFile, "%s", "none");
		fflush(triggerFile);

		//turning off led initially
		fprintf(ledFile, "%s", "1");
		fflush(ledFile);
	}

	// add node with the LED status data source
	UA_DataSource ledStatusDataSource = (UA_DataSource)
   		{.handle = NULL,
		.read = readLedStatus,
		.release = releaseLedStatus,
		.write = writeLedStatus};
	const UA_QualifiedName statusName = UA_QUALIFIEDNAME(0, "status LED");
	UA_Server_addDataSourceVariableNode(server, ledStatusDataSource, statusName, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES));

	// add a static variable node to the adresspace
    UA_Variant *myIntegerVariant = UA_Variant_new();
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(myIntegerVariant, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerVariant, myIntegerName,
                              myIntegerNodeId, parentNodeId, parentReferenceNodeId);

	//start server
	UA_StatusCode retval = UA_Server_run(server, 1, &running); //blocks until running=false

	//ctrl-c received -> clean up
	UA_Server_delete(server);

	if(temperatureFile)
		fclose(temperatureFile);

	if(triggerFile){
		fseek(triggerFile, 0, SEEK_SET);
		//setting led mode to default
		fprintf(triggerFile, "%s", "mmc0");
		fclose(triggerFile);
	}

	if(ledFile){
		fclose(ledFile);
	}

#ifdef UA_MULTITHREADING
	pthread_rwlock_destroy(&writeLock);
#endif

	return retval;
}
