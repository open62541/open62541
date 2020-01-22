/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * A simple server instance which registers with the discovery server (see server_lds.c).
 * Before shutdown it has to unregister itself.
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#define DISCOVERY_SERVER_ENDPOINT "opc.tcp://localhost:4840"

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static UA_StatusCode
readInteger(UA_Server *server, const UA_NodeId *sessionId,
            void *sessionContext, const UA_NodeId *nodeId,
            void *nodeContext, UA_Boolean includeSourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *value) {
    UA_Int32 *myInteger = (UA_Int32*)nodeContext;
    value->hasValue = true;
    UA_Variant_setScalarCopy(&value->value, myInteger, &UA_TYPES[UA_TYPES_INT32]);

    // we know the nodeid is a string
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node read %.*s",
                (int)nodeId->identifier.string.length,
                nodeId->identifier.string.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "read value %i", *(UA_UInt32 *)myInteger);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeInteger(UA_Server *server, const UA_NodeId *sessionId,
             void *sessionContext, const UA_NodeId *nodeId,
             void *nodeContext, const UA_NumericRange *range,
             const UA_DataValue *value) {
    UA_Int32 *myInteger = (UA_Int32*)nodeContext;
    if(value->hasValue && UA_Variant_isScalar(&value->value) &&
       value->value.type == &UA_TYPES[UA_TYPES_INT32] && value->value.data)
        *myInteger = *(UA_Int32 *)value->value.data;

    // we know the nodeid is a string
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node written %.*s",
                (int)nodeId->identifier.string.length,
                nodeId->identifier.string.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "written value %i", *(UA_UInt32 *)myInteger);
    return UA_STATUSCODE_GOOD;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    // use port 0 to dynamically assign port
    UA_ServerConfig_setMinimal(config, 0, NULL);

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.example.server_register");
    config->discovery.mdns.mdnsServerName = UA_String_fromChars("Sample Server");
    // See http://www.opcfoundation.org/UA/schemas/1.03/ServerCapabilities.csv
    //config.serverCapabilitiesSize = 1;
    //UA_String caps = UA_String_fromChars("LDS");
    //config.serverCapabilities = &caps;

    /* add a variable node to the address space */
    UA_Int32 myInteger = 42;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_DataSource dateDataSource;
    dateDataSource.read = readInteger;
    dateDataSource.write = writeInteger;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");

    UA_Server_addDataSourceVariableNode(server, myIntegerNodeId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        myIntegerName, UA_NODEID_NULL, attr, dateDataSource,
                                        &myInteger, NULL);

    UA_Client *clientRegister = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(clientRegister));

    // periodic server register after 10 Minutes, delay first register for 500ms
    UA_StatusCode retval =
        UA_Server_addPeriodicServerRegisterCallback(server, clientRegister, DISCOVERY_SERVER_ENDPOINT,
                                                    10 * 60 * 1000, 500, NULL);
    // UA_StatusCode retval = UA_Server_addPeriodicServerRegisterJob(server,
    // "opc.tcp://localhost:4840", 10*60*1000, 500, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create periodic job for server register. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Client_disconnect(clientRegister);
        UA_Client_delete(clientRegister);
        UA_Server_delete(server);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    retval = UA_Server_run(server, &running);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not start the server. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Client_disconnect(clientRegister);
        UA_Client_delete(clientRegister);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    // Unregister the server from the discovery server.
    retval = UA_Server_unregister_discovery(server, clientRegister);
    //retval = UA_Server_unregister_discovery(server, "opc.tcp://localhost:4840" );
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not unregister server from discovery server. StatusCode %s",
                     UA_StatusCode_name(retval));

    UA_Client_disconnect(clientRegister);
    UA_Client_delete(clientRegister);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;;
}
