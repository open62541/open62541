/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "open62541/namespace_di_generated.h"
#include "open62541/namespace_powerlink_generated.h"

#include <signal.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static volatile UA_Boolean running = true;

static void stopHandler(int sign) {
    (void)sign;
    running = false;
}

int main(int argc, char** argv) {
    /* The bundled POWERLINK model contains array instances of scalar types.
     * CI checks their rejection until the upstream NodeSet is corrected.
     * Normal example runs still report the invalid model as an error. */
    UA_Boolean checkInvalidNodeset =
        argc == 2 && strcmp(argv[1], "--check-invalid-nodeset") == 0;
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    /* create nodes from nodeset */
    size_t idx = LONG_MAX;
    UA_StatusCode retval = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/DI/"), &idx);
    if(retval != UA_STATUSCODE_GOOD) {
        retval = namespace_di_generated(server, true);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "Adding the DI namespace failed. Please check previous error output.");
            UA_Server_delete(server);
            return EXIT_FAILURE;
        }
    }
    retval = UA_Server_getNamespaceByName(server, UA_STRING("http://opcfoundation.org/UA/POWERLINK/"), &idx);
    if(retval != UA_STATUSCODE_GOOD)
        retval = namespace_powerlink_generated(server, true);
    if(checkInvalidNodeset) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "POWERLINK validation returned %s (expected BadTypeMismatch)",
                    UA_StatusCode_name(retval));
        UA_Server_delete(server);
        return retval == UA_STATUSCODE_BADTYPEMISMATCH ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Adding the Powerlink namespace failed. Please check previous error output.");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
