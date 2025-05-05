/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "open62541/namespace_di_generated.h"
#include "open62541/namespace_plc_generated.h"

#include <signal.h>
#include <limits.h>
#include <stdlib.h>

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    /* create nodes from nodeset */
    size_t idx = LONG_MAX;
    UA_StatusCode retval = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/DI/"), &idx);
    if(retval != UA_STATUSCODE_GOOD) {
        retval = namespace_di_generated(server);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "Adding the DI namespace failed. Please check previous error output.");
            UA_Server_delete(server);
            return EXIT_FAILURE;
        }
    }
    retval = UA_Server_getNamespaceByName(server, UA_STRING("http://PLCopen.org/OpcUa/IEC61131-3/"), &idx);
    if(retval != UA_STATUSCODE_GOOD) {
        retval = namespace_plc_generated(server);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "Adding the PLCopen namespace failed. Please check previous error output.");
            UA_Server_delete(server);
            return EXIT_FAILURE;
        }
    }

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
