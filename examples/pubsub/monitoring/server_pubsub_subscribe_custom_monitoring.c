/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * This example can be used to receive and display values that are published
 * by tutorial_pubsub_publish example in the TargetVariables of Subscriber
 * Information Model.
 *
 * Additionally this example shows the usage of the PubSub monitoring implementation.
 * The application can provide the pubsubStateChangeCallback to get notifications
 * about specific PubSub state changes or timeouts. Currently only the
 * MessageReceiveTimeout handling of a DataSetReader is implemented.
 * Stop the tutorial_pubsub_publish example during operation to trigger the
 * MessageReceiveTimeout and check the callback invocation.
 *
 * Per default the monitoring backend is provided by the open62541 sdk.
 * An application can provide it's own monitoring backend (e.g. linux timers),
 * by setting the monitoring callbacks. This option can be tested with
 * program argument '-UseCustomMonitoring'
 * ./server_pubsub_subscribe_monitoring_linux opc.udp://224.0.0.22:4840/ -UseCustomMonitoring
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types_generated.h>

#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

UA_NodeId connectionIdentifier;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;

UA_DataSetReaderConfig readerConfig;

#define MILLI_AS_NANO_SECONDS    (1000 * 1000)
#define SECONDS_AS_NANO_SECONDS  (1000 * 1000 * 1000)

/* custom monitoring backend data structure */
typedef struct MonitoringParams {
    timer_t timer;
    struct sigevent event;
    UA_Server *server;
    void *componentData;
    UA_ServerCallback callback;
} TMonitoringParams;

/* In this example we only configure 1 DataSetReader and therefore only provide 1
data structure for monitoring params. This example only works with 1 DataSetReader for simplicity's sake.
If more DataSetReaders shall be configured one can create a new data structure at every pubSubComponent_createMonitoring() call and
administer them with a list/map/vector ....  */
TMonitoringParams monitoringParams;

static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData);

/* Add new connection to the server */
static UA_StatusCode
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    if((server == NULL) || (transportProfile == NULL) ||
        (networkAddressUrl == NULL)) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Configuration creation for the connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset (&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UDPMC Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random ();
    retval |= UA_Server_addPubSubConnection (server, &connectionConfig, &connectionIdentifier);
    if (retval != UA_STATUSCODE_GOOD) {
        return retval;
    }
    retval |= UA_PubSubConnection_regist(server, &connectionIdentifier);
    return retval;
}

/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroup(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ReaderGroupConfig readerGroupConfig;
    memset (&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = UA_STRING("ReaderGroup1");
    retval |= UA_Server_addReaderGroup(server, connectionIdentifier, &readerGroupConfig,
                                       &readerGroupIdentifier);
    UA_Server_setReaderGroupOperational(server, readerGroupIdentifier);
    return retval;
}

/* Add DataSetReader to the ReaderGroup */
static UA_StatusCode
addDataSetReader(UA_Server *server) {
    if(server == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    memset (&readerConfig, 0, sizeof(UA_DataSetReaderConfig));
    readerConfig.name = UA_STRING("DataSet Reader 1");
    /* Parameters to filter which DataSetMessage has to be processed
     * by the DataSetReader */
    /* The following parameters are used to show that the data published by
     * tutorial_pubsub_publish.c is being subscribed and is being updated in
     * the information model */
    UA_UInt16 publisherIdentifier      = 2234;
    readerConfig.publisherId.type      = &UA_TYPES[UA_TYPES_UINT16];
    readerConfig.publisherId.data      = &publisherIdentifier;
    readerConfig.writerGroupId         = 100;
    readerConfig.dataSetWriterId       = 62541;
    readerConfig.messageReceiveTimeout = 200.0; /* Timeout must be greater than publishing interval of corresponding WriterGroup */

    /* Setting up Meta data configuration in DataSetReader */
    fillTestDataSetMetaData(&readerConfig.dataSetMetaData);

    retval |= UA_Server_addDataSetReader(server, readerGroupIdentifier, &readerConfig,
                                         &readerIdentifier);
    return retval;
}

/* Set SubscribedDataSet type to TargetVariables data type
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables (UA_Server *server, UA_NodeId dataSetReaderId) {
    if(server == NULL)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId folderId;
    UA_String folderName = readerConfig.dataSetMetaData.name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING ("en-US");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    }
    else {
        oAttr.displayName = UA_LOCALIZEDTEXT ("en-US", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME (1, "Subscribed Variables");
    }

    UA_Server_addObjectNode (server, UA_NODEID_NULL,
                             UA_NODEID_NUMERIC (0, UA_NS0ID_OBJECTSFOLDER),
                             UA_NODEID_NUMERIC (0, UA_NS0ID_ORGANIZES),
                             folderBrowseName, UA_NODEID_NUMERIC (0,
                             UA_NS0ID_BASEOBJECTTYPE), oAttr, NULL, &folderId);

    /* Create the TargetVariables with respect to DataSetMetaData fields */
    UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *)
            UA_calloc(readerConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++) {
        /* Variable to subscribe data */
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        UA_LocalizedText_copy(&readerConfig.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        vAttr.displayName.locale = UA_STRING("en-US");
        vAttr.displayName.text = readerConfig.dataSetMetaData.fields[i].name;
        vAttr.dataType = readerConfig.dataSetMetaData.fields[i].dataType;

        UA_NodeId newNode;
        retval |= UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32)i + 50000),
                                           folderId,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           UA_QUALIFIEDNAME(1, (char *)readerConfig.dataSetMetaData.fields[i].name.data),
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newNode);

        /* For creating Targetvariables */
        UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
        targetVars[i].targetVariable.attributeId  = UA_ATTRIBUTEID_VALUE;
        targetVars[i].targetVariable.targetNodeId = newNode;
    }

    retval = UA_Server_DataSetReader_createTargetVariables(server, dataSetReaderId,
                                                           readerConfig.dataSetMetaData.fieldsSize, targetVars);
    for(size_t i = 0; i < readerConfig.dataSetMetaData.fieldsSize; i++)
        UA_FieldTargetDataType_clear(&targetVars[i].targetVariable);

    UA_free(targetVars);
    UA_free(readerConfig.dataSetMetaData.fields);
    return retval;
}

/* Define MetaData for TargetVariables */
static void fillTestDataSetMetaData(UA_DataSetMetaDataType *pMetaData) {
    if(pMetaData == NULL) {
        return;
    }

    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name = UA_STRING ("DataSet 1");

    /* Static definition of number of fields size to 4 to create four different
     * targetVariables of distinct datatype
     * Currently the publisher sends only DateTime data type */
    pMetaData->fieldsSize = 4;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                         &UA_TYPES[UA_TYPES_FIELDMETADATA]);

    /* DateTime DataType */
    UA_FieldMetaData_init (&pMetaData->fields[0]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_DATETIME].typeId,
                    &pMetaData->fields[0].dataType);
    pMetaData->fields[0].builtInType = UA_NS0ID_DATETIME;
    pMetaData->fields[0].name =  UA_STRING ("DateTime");
    pMetaData->fields[0].valueRank = -1; /* scalar */

    /* Int32 DataType */
    UA_FieldMetaData_init (&pMetaData->fields[1]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId,
                   &pMetaData->fields[1].dataType);
    pMetaData->fields[1].builtInType = UA_NS0ID_INT32;
    pMetaData->fields[1].name =  UA_STRING ("Int32");
    pMetaData->fields[1].valueRank = -1; /* scalar */

    /* Int64 DataType */
    UA_FieldMetaData_init (&pMetaData->fields[2]);
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT64].typeId,
                   &pMetaData->fields[2].dataType);
    pMetaData->fields[2].builtInType = UA_NS0ID_INT64;
    pMetaData->fields[2].name =  UA_STRING ("Int64");
    pMetaData->fields[2].valueRank = -1; /* scalar */

    /* Boolean DataType */
    UA_FieldMetaData_init (&pMetaData->fields[3]);
    UA_NodeId_copy (&UA_TYPES[UA_TYPES_BOOLEAN].typeId,
                    &pMetaData->fields[3].dataType);
    pMetaData->fields[3].builtInType = UA_NS0ID_BOOLEAN;
    pMetaData->fields[3].name =  UA_STRING ("BoolToggle");
    pMetaData->fields[3].valueRank = -1; /* scalar */
}

UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

/* Provide a callback to get notifications about specific PubSub state changes or timeouts.
    Currently only the MessageReceiveTimeout of the subscriber is supported.
    Stop the tutorial_pubsub_publish example during operation to trigger the MessageReceiveTimeout and
    check the callback invocation here */
static void
pubsubStateChangeCallback(UA_Server *server,
                                    UA_NodeId *pubsubComponentId,
                                    UA_PubSubState state,
                                    UA_StatusCode code) {
    /* Use the application context kept by the server.
       In this case, count up the number of times the state changed */
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_UInt32 *stateChangeCnt = (UA_UInt32 *)config->context;
    (*stateChangeCnt)++;

    if (pubsubComponentId == 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "pubsubStateChangeCallback(): Null pointer error. Internal error");
        return;
    }

    UA_String strId;
    UA_String_init(&strId);
    UA_NodeId_print(pubsubComponentId, &strId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "pubsubStateChangeCallback(): State of component '%.*s' changed to '%i'. "
        "Status code '0x%08x' '%s'", (UA_Int32) strId.length, strId.data, state, code, UA_StatusCode_name(code));
    UA_String_clear(&strId);
}

static void
monitoringCallbackHandler(union sigval val)
{
    /* execute the monitoring callback */
    TMonitoringParams *param = (TMonitoringParams *) val.sival_ptr;
    param->callback(param->server, param->componentData);
}

/* Custom monitoring backend implementation: only 1 DataSetReader is supported */
static UA_StatusCode
pubSubComponent_createMonitoring(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType, void *data, UA_ServerCallback callback) {
    if ((!server) || (!data)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if ((eComponentType == UA_PUBSUB_COMPONENT_DATASETREADER) && (eMonitoringType == UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT)) {

        memset(&monitoringParams, 0, sizeof(monitoringParams));
        monitoringParams.server = server;
        monitoringParams.componentData = data;
        monitoringParams.callback = callback;

        /* create the timer */
        memset(&monitoringParams.timer, 0, sizeof(monitoringParams.timer));
        monitoringParams.event.sigev_notify = SIGEV_THREAD;
        monitoringParams.event.sigev_signo = SIGUSR1;
        monitoringParams.event.sigev_value.sival_ptr = &monitoringParams;
        monitoringParams.event.sigev_notify_function = monitoringCallbackHandler;
        if (timer_create(CLOCK_MONOTONIC, &monitoringParams.event, &monitoringParams.timer) != 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Error PubSubComponent_createMonitoring(): timer_create() failed with code %s", strerror(errno));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    } else {
        ret = UA_STATUSCODE_BADNOTSUPPORTED;
    }
    return ret;
}

static UA_StatusCode
pubSubComponent_startMonitoring(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType, void *data) {
    if ((!server) || (!data)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if ((eComponentType == UA_PUBSUB_COMPONENT_DATASETREADER) && (eMonitoringType == UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT)) {
        /* start custom monitoring/timer */
        struct itimerspec its;
        long int timeout_ns = ((long int) readerConfig.messageReceiveTimeout) * MILLI_AS_NANO_SECONDS;
        its.it_value.tv_sec = timeout_ns / SECONDS_AS_NANO_SECONDS;
        its.it_value.tv_nsec = timeout_ns % SECONDS_AS_NANO_SECONDS;
        its.it_interval.tv_sec = 0; // execute only once, we don't need a cyclic MessageReceiveTimeout notification
        its.it_interval.tv_nsec = 0;
        if (timer_settime(monitoringParams.timer, 0, &its, NULL) != 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Error pubSubComponent_startMonitoring(): timer_settime() failed with code %s", strerror(errno));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    } else {
        ret = UA_STATUSCODE_BADNOTSUPPORTED;
    }
    return ret;
}

static UA_StatusCode
pubSubComponent_stopMonitoring(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType, void *data) {
    if ((!server) || (!data)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if ((eComponentType == UA_PUBSUB_COMPONENT_DATASETREADER) && (eMonitoringType == UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT)) {
        /* stop custom monitoring/timer */
        struct itimerspec its;
        its.it_value.tv_sec = 0;
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        if (timer_settime(monitoringParams.timer, 0, &its, NULL) != 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Error pubSubComponent_stopMonitoring(): timer_settime() failed with code %s", strerror(errno));
            return UA_STATUSCODE_BADINTERNALERROR;
        }

    } else {
        ret = UA_STATUSCODE_BADNOTSUPPORTED;
    }
    return ret;
}

static UA_StatusCode
pubSubComponent_updateMonitoringInterval(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                            UA_PubSubMonitoringType eMonitoringType, void *data)
{
    if ((!server) || (!data)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if ((eComponentType == UA_PUBSUB_COMPONENT_DATASETREADER) && (eMonitoringType == UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT)) {
        /* change interval of custom monitoring/timer */
        UA_DataSetReaderConfig dsReaderConfig;
        if (UA_Server_DataSetReader_getConfig(server, Id, &dsReaderConfig) == UA_STATUSCODE_GOOD) {
            struct itimerspec its;
            long int timeout_ns = ((long int) dsReaderConfig.messageReceiveTimeout) * MILLI_AS_NANO_SECONDS;
            its.it_value.tv_sec = timeout_ns / SECONDS_AS_NANO_SECONDS;
            its.it_value.tv_nsec = timeout_ns % SECONDS_AS_NANO_SECONDS;
            its.it_interval.tv_sec = 0; // execute only once, we don't need a cyclic MessageReceiveTimeout notification
            its.it_interval.tv_nsec = 0;
            if (timer_settime(monitoringParams.timer, 0, &its, NULL) != 0) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Error pubSubComponent_updateMonitoringInterval(): timer_settime() failed with code %s", strerror(errno));
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            UA_DataSetReaderConfig_clear(&dsReaderConfig);
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Error pubSubComponent_updateMonitoringInterval(): UA_Server_DataSetReader_getConfig() failed");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    } else {
        ret = UA_STATUSCODE_BADNOTSUPPORTED;
    }
    return ret;
}

static UA_StatusCode
pubSubComponent_deleteMonitoring(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType, void *data) {
    if ((!server) || (!data)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if ((eComponentType == UA_PUBSUB_COMPONENT_DATASETREADER) && (eMonitoringType == UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT)) {
        /* delete custom monitoring/timer */
        if (timer_delete(monitoringParams.timer) != 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Error pubSubComponent_deleteMonitoring(): timer_delete() failed with code %s", strerror(errno));
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    return ret;
}

static int
run(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl, UA_Boolean *useCustomMonitoring) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    /* Return value initialized to Status Good */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, 4801, NULL);

    /* Add the PubSub network layer implementation to the server config.
     * The TransportLayer is acting as factory to create new connections
     * on runtime. Details about the PubSubTransportLayer can be found inside the
     * tutorial_pubsub_connection */
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

    /* Provide a callback to get notifications of specific PubSub state changes or timeouts (e.g. subscriber MessageReceiveTimeout) */
    config->pubSubConfig.stateChangeCallback = pubsubStateChangeCallback;

    /* Provide some application context that can be accessed from the above callback.
       e.g. count the number of times the state changed. */
    UA_UInt32 stateChangeCnt = 0;
    config->context = &stateChangeCnt;

    if (*useCustomMonitoring == UA_TRUE) {
        /* provide own backend by setting the monitoring callbacks */
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Use custom monitoring callback implementation");
        UA_PubSubMonitoringInterface monitoringInterface;
        monitoringInterface.createMonitoring = pubSubComponent_createMonitoring;
        monitoringInterface.startMonitoring = pubSubComponent_startMonitoring;
        monitoringInterface.stopMonitoring = pubSubComponent_stopMonitoring;
        monitoringInterface.updateMonitoringInterval = pubSubComponent_updateMonitoringInterval;
        monitoringInterface.deleteMonitoring = pubSubComponent_deleteMonitoring;
        config->pubSubConfig.monitoringInterface = monitoringInterface;
    }

    /* API calls */
    /* Add PubSubConnection */
    retval |= addPubSubConnection(server, transportProfile, networkAddressUrl);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add ReaderGroup to the created PubSubConnection */
    retval |= addReaderGroup(server);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add DataSetReader to the created ReaderGroup */
    retval |= addDataSetReader(server);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    /* Add SubscribedVariables to the created DataSetReader */
    retval |= addSubscribedVariables(server, readerIdentifier);
    if (retval != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;

    retval = UA_Server_run(server, &running);
    UA_Server_delete(server);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "PubSub state changed %d time(s).", stateChangeCnt);

    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s uri [device] [-UseCustomMonitoring]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Boolean useCustomMonitoring = UA_FALSE;

    if(argc > 1) {
        if(strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        } else if(strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if(strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if(argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else {
            printf ("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
        // last argument is optional [-UseCustomMonitoring]
        if (strcmp(argv[argc-1], "-UseCustomMonitoring") == 0) {
            useCustomMonitoring = UA_TRUE;
        }
    }
    return run(&transportProfile, &networkAddressUrl, &useCustomMonitoring);
}

