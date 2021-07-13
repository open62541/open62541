/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

static UA_StatusCode
addFileInstance(UA_Server *server)
{
    UA_NodeId FileTypeNodeId = UA_NODEID_NUMERIC(1, 100); /* get the nodeid assigned by the server */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Sample File");
    UA_String filePath = UA_STRING("sample.txt"); //create in current directory
    
    UA_StatusCode result = UA_Server_addFile(server, FileTypeNodeId,
                           UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                           UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                           UA_QUALIFIEDNAME(1, "Sample File"), oAttr,
                           filePath, NULL, NULL);
    
    if(result == UA_STATUSCODE_GOOD)
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "File Object Node was created successfully");
        return UA_STATUSCODE_GOOD;
    }
    return result;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    addFileInstance(server);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
