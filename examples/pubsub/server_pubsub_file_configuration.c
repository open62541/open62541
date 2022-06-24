/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/* Includes */
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "ua_pubsub_config.h"

#include <signal.h>

#include "common.h"

/* Global variables */
volatile UA_Boolean g_running = true;

/* Signal handler */
static void stopHandler(int signum) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Exiting...\n");
    g_running = false;
}

/* Function to give user information about correct usage */
static void usage_info(void) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "USAGE: ./server_pubsub_file_configuration [port] [name of UA_Binary_Config_File]");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Alternatively, Bin-files can be loaded via configuration method calls.");
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Boolean loadPubSubFromFile = UA_FALSE;
    UA_UInt16 port = 4840;

    /* 1. Check arguments and set name of PubSub configuration file*/
    switch(argc) {
        case 2:
            port = (unsigned short)atoi(argv[1]);
            break;
        case 3:
            port = (unsigned short)atoi(argv[1]);
            loadPubSubFromFile = UA_TRUE;
            break;

        default:
            usage_info();
    }

    /* 2. Initialize Server */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, port, NULL); /* creates server on default port 4840 */

    /* 3. Add variable nodes to the server */
    UA_VariableAttributes attr;
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId pubSubVariableObjectId = UA_NODEID_STRING(1, "PubSubObject");

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "PubSubVariables");
    UA_Server_addObjectNode(server, pubSubVariableObjectId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            parentReferenceNodeId,
                            UA_QUALIFIEDNAME(1, "PubSubVariables"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, NULL);

    attr = UA_VariableAttributes_default;
    UA_Boolean myBool = UA_TRUE;
    UA_Variant_setScalar(&attr.value, &myBool, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.description = UA_LOCALIZEDTEXT("en-US","BoolToggle");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","BoolToggle");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myBoolNodeId = UA_NODEID_STRING(1, "BoolToggle");
    UA_QualifiedName myBoolName = UA_QUALIFIEDNAME(1, "BoolToggle");
    UA_Server_addVariableNode(server, myBoolNodeId, pubSubVariableObjectId,
                              parentReferenceNodeId, myBoolName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

    attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 0;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","Int32");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","Int32");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "Int32");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "Int32");
    UA_Server_addVariableNode(server, myIntegerNodeId, pubSubVariableObjectId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

    attr = UA_VariableAttributes_default;
    UA_Int32 myIntegerFast = 24;
    UA_Variant_setScalar(&attr.value, &myIntegerFast, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","Int32Fast");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","Int32Fast");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myIntegerFastNodeId = UA_NODEID_STRING(1, "Int32Fast");
    UA_QualifiedName myIntegerFastName = UA_QUALIFIEDNAME(1, "Int32Fast");
    UA_Server_addVariableNode(server, myIntegerFastNodeId, pubSubVariableObjectId,
                              parentReferenceNodeId, myIntegerFastName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);

    attr = UA_VariableAttributes_default;
    UA_DateTime myDate = UA_DateTime_now() + UA_DateTime_localTimeUtcOffset();
    UA_Variant_setScalar(&attr.value, &myDate, &UA_TYPES[UA_TYPES_DATETIME]);
    attr.description = UA_LOCALIZEDTEXT("en-US","DateTime");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","DateTime");
    attr.dataType = UA_TYPES[UA_TYPES_DATETIME].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myDateNodeId = UA_NODEID_STRING(1, "DateTime");
    UA_QualifiedName myDateName = UA_QUALIFIEDNAME(1, "DateTime");
    UA_Server_addVariableNode(server, myDateNodeId, pubSubVariableObjectId,
                              parentReferenceNodeId, myDateName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
    /* 4. load configuration from file */
    if(loadPubSubFromFile) {
        UA_ByteString configuration = loadFile(argv[2]);
        UA_PubSubManager_loadPubSubConfigFromByteString(server, configuration);
    }

    /* 5. start server */
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Starting server...");
    UA_StatusCode statusCode = UA_Server_run(server, &g_running);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Server stopped. Status code: 0x%x\n", statusCode);
        return(-1);
    }

    if(loadPubSubFromFile) {
        /* 6. save current configuration to file */
        UA_ByteString buffer = UA_BYTESTRING_NULL;
        statusCode = UA_PubSubManager_getEncodedPubSubConfiguration(server, &buffer);
        if(statusCode == UA_STATUSCODE_GOOD)
            statusCode = writeFile(argv[2], buffer);

        if(statusCode != UA_STATUSCODE_GOOD)
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Saving PubSub configuration to file failed. StatusCode: 0x%x\n", statusCode);

        UA_ByteString_clear(&buffer);
    }

    UA_Server_delete(server);

    return 0;
}
/******************************************************************************************************/
