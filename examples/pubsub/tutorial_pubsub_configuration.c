/* Includes */
#include <stdio.h>
#include <signal.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/pubsub.h>

/* Global variables */
volatile UA_Boolean g_running = true;

/* Initialization of signal handler */
void initSigHandler(void);

/* Function to give user information about correct usage */
void usage_info(void);

/* Add variable nodes */
void addVariables(UA_Server *server);

/***** MAIN FUNCTION **********************************************************************************/
int main(int argc, char** argv)
{
#ifdef UA_ENABLE_PUBSUB_CONFIG
    UA_Boolean loadPubSubFromFile = UA_FALSE;
#endif
    UA_UInt16 port = 4840;

    /* 1. Check arguments and set name of PubSub configuration file*/
    switch(argc)
    {
        case 2:
            port = (unsigned short)atoi(argv[1]);
            break;
        case 3:
            port = (unsigned short)atoi(argv[1]);
#ifdef UA_ENABLE_PUBSUB_CONFIG
            loadPubSubFromFile = UA_TRUE;
#endif
            break;

        default:
            usage_info(); 
    }

    /* Initialize Signal Handler for exiting the process */
    initSigHandler();

    /* 2. Initialize Server */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, port, NULL); /* creates server on default port 4840 */

#ifdef UA_ENABLE_PUBSUB_CONFIG
    if(loadPubSubFromFile) 
    {
        UA_Server_setPubSubConfigFilename(server, argv[2]);
    }
#endif

    /* 3. Add variable nodes to the server */
    addVariables(server);

    printf("Starting server...\n");
    UA_StatusCode statusCode = UA_Server_run(server, &g_running);
    if(statusCode != UA_STATUSCODE_GOOD)
    {
        fprintf(stderr, "Server stopped. Status code: 0x%x\n", statusCode);
        return(-1);
    }
    UA_Server_delete(server);

    printf("\n");
    return 0;
}
/******************************************************************************************************/


/* Signal handler */
static void stopHandler(int signum)
{
    printf("Exiting...\n");
    g_running = false;
}


/* Initialization of signal handler */
void initSigHandler(void)
{
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
}


/* Function to give user information about correct usage */
void usage_info(void)
{
    printf("USAGE: ./example_pubsub_configuration [name of UA_Binary_Config_File]\n");
    printf("Alternatively, Bin-files can be loaded via configuration method calls.\n\n");
}


/* Add object that contains the PubSubVariables (= ParentNode of variables) */
static UA_NodeId 
addObject(UA_Server *server)
{
    UA_NodeId pubSubVariableObjectId = UA_NODEID_STRING(1, "PubSubObject");
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "PubSubVariables");
    UA_Server_addObjectNode(server, pubSubVariableObjectId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "PubSubVariables"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, NULL);
    return pubSubVariableObjectId;
}


/* Add variable */
void
addVariables(UA_Server *server)
{
    UA_VariableAttributes attr;
    UA_NodeId parentNodeId = addObject(server);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);

    attr = UA_VariableAttributes_default;
    UA_Boolean myBool = UA_TRUE;
    UA_Variant_setScalar(&attr.value, &myBool, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.description = UA_LOCALIZEDTEXT("en-US","BoolToggle");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","BoolToggle");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_NodeId myBoolNodeId = UA_NODEID_STRING(1, "BoolToggle");
    UA_QualifiedName myBoolName = UA_QUALIFIEDNAME(1, "BoolToggle");
    UA_Server_addVariableNode(server, myBoolNodeId, parentNodeId,
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
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
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
    UA_Server_addVariableNode(server, myIntegerFastNodeId, parentNodeId,
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
    UA_Server_addVariableNode(server, myDateNodeId, parentNodeId,
                              parentReferenceNodeId, myDateName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}
