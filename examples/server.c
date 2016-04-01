/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
//to compile with single file releases:
// * single-threaded: gcc -std=c99 server.c open62541.c -o server
// * multi-threaded: gcc -std=c99 server.c open62541.c -o server -lurcu-cds -lurcu -lurcu-common -lpthread

#ifdef UA_NO_AMALGAMATION
# include <time.h>
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

#include <signal.h>
#include <errno.h> // errno, EINTR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
# include <io.h> //access
#else
# include <unistd.h> //access
#endif

#ifdef UA_ENABLE_MULTITHREADING
# ifdef UA_NO_AMALGAMATION
#  ifndef __USE_XOPEN2K
#   define __USE_XOPEN2K
#  endif
# endif
#include <pthread.h>
#endif

/****************************/
/* Server-related variables */
/****************************/

UA_Boolean running = 1;
UA_Logger logger = Logger_Stdout;

/*************************/
/* Read-only data source */
/*************************/
static UA_StatusCode
readTimeData(void *handle, const UA_NodeId nodeId, UA_Boolean sourceTimeStamp,
             const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
	UA_DateTime currentTime = UA_DateTime_now();
    UA_Variant_setScalarCopy(&value->value, &currentTime, &UA_TYPES[UA_TYPES_DATETIME]);
	value->hasValue = true;
	if(sourceTimeStamp) {
		value->hasSourceTimestamp = true;
		value->sourceTimestamp = currentTime;
	}
	return UA_STATUSCODE_GOOD;
}

/*****************************/
/* Read-only CPU temperature */
/*      Only on Linux        */
/*****************************/
FILE* temperatureFile = NULL;
static UA_StatusCode
readTemperature(void *handle, const UA_NodeId nodeId, UA_Boolean sourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }

	rewind(temperatureFile);
	fflush(temperatureFile);

	UA_Double currentTemperature;
	if(fscanf(temperatureFile, "%lf", &currentTemperature) != 1){
		UA_LOG_WARNING(logger, UA_LOGCATEGORY_USERLAND, "Can not parse temperature");
		exit(1);
	}

	currentTemperature /= 1000.0;

	value->sourceTimestamp = UA_DateTime_now();
	value->hasSourceTimestamp = true;
    UA_Variant_setScalarCopy(&value->value, &currentTemperature, &UA_TYPES[UA_TYPES_DOUBLE]);
	value->hasValue = true;
	return UA_STATUSCODE_GOOD;
}

/*************************/
/* Read-write status led */
/*************************/
#ifdef UA_ENABLE_MULTITHREADING
pthread_rwlock_t writeLock;
#endif
FILE* triggerFile = NULL;
FILE* ledFile = NULL;
UA_Boolean ledStatus = 0;

static UA_StatusCode
readLedStatus(void *handle, UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
              const UA_NumericRange *range, UA_DataValue *value) {
    if(range)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

    value->hasValue = true;
    UA_StatusCode retval = UA_Variant_setScalarCopy(&value->value, &ledStatus,
                                                    &UA_TYPES[UA_TYPES_BOOLEAN]);

    if(retval != UA_STATUSCODE_GOOD)
        return retval;
  
    if(sourceTimeStamp) {
        value->sourceTimestamp = UA_DateTime_now();
        value->hasSourceTimestamp = true;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeLedStatus(void *handle, const UA_NodeId nodeid,
               const UA_Variant *data, const UA_NumericRange *range) {
    if(range)
        return UA_STATUSCODE_BADINDEXRANGEINVALID;

#ifdef UA_ENABLE_MULTITHREADING
	pthread_rwlock_wrlock(&writeLock);
#endif
	if(data->data)
		ledStatus = *(UA_Boolean*)data->data;

	if(triggerFile)
		fseek(triggerFile, 0, SEEK_SET);

	if(ledFile) {
		if(ledStatus == 1)
			fprintf(ledFile, "%s", "1");
		else
			fprintf(ledFile, "%s", "0");
		fflush(ledFile);
	}
#ifdef UA_ENABLE_MULTITHREADING
	pthread_rwlock_unlock(&writeLock);
#endif
	return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_METHODCALLS
static UA_StatusCode
getMonitoredItems(void *methodHandle, const UA_NodeId objectId,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output) {
    UA_String tmp = UA_STRING("Hello World");
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    printf("getMonitoredItems was called\n");
    return UA_STATUSCODE_GOOD;
}
#endif

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Received Ctrl-C");
	running = 0;
}

static UA_ByteString loadCertificate(void) {
	UA_ByteString certificate = UA_STRING_NULL;
	FILE *fp = NULL;
	if(!(fp=fopen("server_cert.der", "rb"))) {
        errno = 0; // we read errno also from the tcp layer...
        return certificate;
    }

    fseek(fp, 0, SEEK_END);
    certificate.length = (size_t)ftell(fp);
    certificate.data = malloc(certificate.length*sizeof(UA_Byte));
	if(!certificate.data){
		fclose(fp);
		return certificate;
	}

    fseek(fp, 0, SEEK_SET);
    if(fread(certificate.data, sizeof(UA_Byte), certificate.length, fp) < (size_t)certificate.length)
        UA_ByteString_deleteMembers(&certificate); // error reading the cert
    fclose(fp);

    return certificate;
}

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {  
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
monitoredHandler(void *handle, const UA_NodeId *nodeid, const bool removed)
{
    // This handler can help managing the DataSources, e.g. activating them, etc..

    if (removed)
        printf("Stop monitoring Node ns=%d; id=%d\n", nodeid->namespaceIndex, nodeid->identifier.numeric);
    else
        printf("Start monitoring Node ns=%d; id=%d\n", nodeid->namespaceIndex, nodeid->identifier.numeric);
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
instantiationHandle(UA_NodeId newNodeId, UA_NodeId templateId, void *handle ) {
    printf("Instantiated Node ns=%d; id=%d from ns=%d; id=%d\n", newNodeId.namespaceIndex, newNodeId.identifier.numeric, templateId.namespaceIndex, templateId.identifier.numeric);
    return UA_STATUSCODE_GOOD;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
#ifdef UA_ENABLE_MULTITHREADING
    pthread_rwlock_init(&writeLock, 0);
#endif

    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    UA_ServerConfig config = UA_ServerConfig_standard;
    config.logger = Logger_Stdout;
    config.serverCertificate = loadCertificate();
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    // add node with the datetime data source
    UA_DataSource dateDataSource = (UA_DataSource) {.handle = NULL, .read = readTimeData,
        .write = NULL, .monitored = monitoredHandler};
    UA_VariableAttributes v_attr;
    UA_VariableAttributes_init(&v_attr);
    v_attr.description = UA_LOCALIZEDTEXT("en_US","current time");
    v_attr.displayName = UA_LOCALIZEDTEXT("en_US","current time");
    const UA_QualifiedName dateName = UA_QUALIFIEDNAME(1, "current time");
    UA_NodeId dataSourceId;
    UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), dateName,
                                        UA_NODEID_NULL, v_attr, dateDataSource, &dataSourceId);

#ifndef _WIN32
    /* cpu temperature monitoring for linux machines */
    const char *temperatureFileName = "/sys/class/thermal/thermal_zone0/temp"; // RaspberryPi
    // const char *temperatureFileName = "/sys/class/hwmon/hwmon0/device/temp1_input"; // Beaglebone
    // const char *temperatureFileName = "/sys/class/thermal/thermal_zone3/temp"; // Intel Edison Alternative 1
    // const char *temperatureFileName = "/sys/class/thermal/thermal_zone4/temp"; // Intel Edison Alternative 2
    if((temperatureFile = fopen(temperatureFileName, "r"))) {
        // add node with the data source
        UA_DataSource temperatureDataSource = (UA_DataSource) {
            .handle = NULL, .read = readTemperature, .write = NULL};
        const UA_QualifiedName tempName = UA_QUALIFIEDNAME(1, "cpu temperature");
        UA_VariableAttributes_init(&v_attr);
        v_attr.description = UA_LOCALIZEDTEXT("en_US","temperature");
        v_attr.displayName = UA_LOCALIZEDTEXT("en_US","temperature");
        UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), tempName,
                                            UA_NODEID_NULL, v_attr, temperatureDataSource, NULL);
    }

    /* LED control for rpi */
    if(access("/sys/class/leds/led0/trigger", F_OK ) != -1 ||
       access("/sys/class/leds/led0/brightness", F_OK ) != -1) {
        if((triggerFile = fopen("/sys/class/leds/led0/trigger", "w")) &&
           (ledFile = fopen("/sys/class/leds/led0/brightness", "w"))) {
            //setting led mode to manual
            fprintf(triggerFile, "%s", "none");
            fflush(triggerFile);

            //turning off led initially
            fprintf(ledFile, "%s", "1");
            fflush(ledFile);

            // add node with the LED status data source
            UA_DataSource ledStatusDataSource = (UA_DataSource) {
                .handle = NULL, .read = readLedStatus, .write = writeLedStatus};
            UA_VariableAttributes_init(&v_attr);
            v_attr.description = UA_LOCALIZEDTEXT("en_US","status LED");
            v_attr.displayName = UA_LOCALIZEDTEXT("en_US","status LED");
            const UA_QualifiedName statusName = UA_QUALIFIEDNAME(0, "status LED");
            UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), statusName,
                                                UA_NODEID_NULL, v_attr, ledStatusDataSource, NULL);
        } else
            UA_LOG_WARNING(logger, UA_LOGCATEGORY_USERLAND,
                           "[Raspberry Pi] LED file exist, but is not accessible (try to run server with sudo)");
    }
#endif

    // add a static variable node to the adresspace
    UA_VariableAttributes myVar;
    UA_VariableAttributes_init(&myVar);
    myVar.description = UA_LOCALIZEDTEXT("en_US", "the answer");
    myVar.displayName = UA_LOCALIZEDTEXT("en_US", "the answer");
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&myVar.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    /* Instantiation Callback can be used when a typeDefinition creates new nodes under this added one.
     * The method will be called for each created node.
     */
    UA_InstantiationCallback theAnswerCallback = {.method=instantiationHandle, .handle=(void*) server};
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId, parentReferenceNodeId,
                              myIntegerName, UA_NODEID_NULL, myVar, &theAnswerCallback, NULL);
    UA_Variant_deleteMembers(&myVar.value);

    /**************/
    /* Demo Nodes */
    /**************/

#define DEMOID 50000
    UA_ObjectAttributes object_attr;
    UA_ObjectAttributes_init(&object_attr);
    object_attr.description = UA_LOCALIZEDTEXT("en_US","Demo");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Demo");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Demo"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

#define SCALARID 50001
    object_attr.description = UA_LOCALIZEDTEXT("en_US","Scalar");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Scalar");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SCALARID),
                            UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Scalar"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

#define ARRAYID 50002
    object_attr.description = UA_LOCALIZEDTEXT("en_US","Array");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Array");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, ARRAYID),
                            UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Array"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

#define MATRIXID 50003
    object_attr.description = UA_LOCALIZEDTEXT("en_US","Matrix");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Matrix");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, MATRIXID), UA_NODEID_NUMERIC(1, DEMOID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "Matrix"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

    UA_UInt32 id = 51000; // running id in namespace 0
    for(UA_UInt32 type = 0; type < UA_TYPES_DIAGNOSTICINFO; type++) {
        if(type == UA_TYPES_VARIANT || type == UA_TYPES_DIAGNOSTICINFO)
            continue;

        UA_VariableAttributes attr;
        UA_VariableAttributes_init(&attr);
        char name[15];
        sprintf(name, "%02d", type);
        attr.displayName = UA_LOCALIZEDTEXT("en_US",name);
        UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(1, name);

        /* add a scalar node for every built-in type */
        void *value = UA_new(&UA_TYPES[type]);
        UA_Variant_setScalar(&attr.value, value, &UA_TYPES[type]);
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id),
                                  UA_NODEID_NUMERIC(1, SCALARID),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  qualifiedName, UA_NODEID_NULL, attr, NULL, NULL);
        UA_Variant_deleteMembers(&attr.value);

        /* add an array node for every built-in type */
        UA_Variant_setArray(&attr.value, UA_Array_new(10, &UA_TYPES[type]),
                            10, &UA_TYPES[type]);
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id),
                                  UA_NODEID_NUMERIC(1, ARRAYID),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  qualifiedName, UA_NODEID_NULL, attr, NULL, NULL);
        UA_Variant_deleteMembers(&attr.value);

        /* add an matrix node for every built-in type */
        void* myMultiArray = UA_Array_new(9, &UA_TYPES[type]);
        attr.value.arrayDimensions = UA_Array_new(2, &UA_TYPES[UA_TYPES_INT32]);
        attr.value.arrayDimensions[0] = 3;
        attr.value.arrayDimensions[1] = 3;
        attr.value.arrayDimensionsSize = 2;
        attr.value.arrayLength = 9;
        attr.value.data = myMultiArray;
        attr.value.type = &UA_TYPES[type];
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, ++id),
                                  UA_NODEID_NUMERIC(1, MATRIXID),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                  qualifiedName, UA_NODEID_NULL, attr, NULL, NULL);
        UA_Variant_deleteMembers(&attr.value);
    }

#ifdef UA_ENABLE_METHODCALLS
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.arrayDimensionsSize = 0;
    inputArguments.arrayDimensions = NULL;
    inputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en_US", "A String");
    inputArguments.name = UA_STRING("Input an integer");
    inputArguments.valueRank = -1;

    UA_Argument outputArguments;
    UA_Argument_init(&outputArguments);
    outputArguments.arrayDimensionsSize = 0;
    outputArguments.arrayDimensions = NULL;
    outputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArguments.description = UA_LOCALIZEDTEXT("en_US", "A String");
    outputArguments.name = UA_STRING("Input an integer");
    outputArguments.valueRank = -1;

    UA_MethodAttributes addmethodattributes;
    UA_MethodAttributes_init(&addmethodattributes);
    addmethodattributes.description = UA_LOCALIZEDTEXT("en_US", "Return a single argument as passed by the caller");
    addmethodattributes.displayName = UA_LOCALIZEDTEXT("en_US", "ping");
    addmethodattributes.executable = true;
    addmethodattributes.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1,"ping"), addmethodattributes,
                            &getMonitoredItems, // Call this method
                            (void *) server,    // Pass our server pointer as a handle to the method
                            1, &inputArguments, 1, &outputArguments, NULL);
#endif
   
    // Example for iterating over all nodes referenced by "Objects":
    //printf("Nodes connected to 'Objects':\n=============================\n");
    UA_Server_forEachChildNodeCall(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), nodeIter, NULL);
  
    // Some easy localization
    UA_LocalizedText objectsName = UA_LOCALIZEDTEXT("de_DE", "Objekte");
    UA_Server_writeDisplayName(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), objectsName);
  
    //start server
    UA_StatusCode retval = UA_Server_run(server, &running); //blocks until running=false

    //ctrl-c received -> clean up
    UA_Server_delete(server);
    nl.deleteMembers(&nl);

    if(temperatureFile)
        fclose(temperatureFile);

    if(triggerFile) {
        fseek(triggerFile, 0, SEEK_SET);
        //setting led mode to default
        fprintf(triggerFile, "%s", "mmc0");
        fclose(triggerFile);
    }
  
    if(ledFile)
        fclose(ledFile);

#ifdef UA_ENABLE_MULTITHREADING
    pthread_rwlock_destroy(&writeLock);
#endif

    UA_ByteString_deleteMembers(&config.serverCertificate);
    return (int)retval;
}
