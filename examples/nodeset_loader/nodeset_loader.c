/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "nodeset_loader_file.h"

#include <stdio.h>

int
main(int argc, const char *argv[]) {
    UA_Server *server = UA_Server_new();
    if(!server)
        return EXIT_FAILURE;

    UA_StatusCode res = UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    res = UA_Server_run_startup(server);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    for(int cnt = 1; cnt < argc; cnt++) {
        res = loadNodesetFile(server, argv[cnt]);
        if(res != UA_STATUSCODE_GOOD) {
            printf("Nodeset %s could not be loaded, exit\n", argv[cnt]);
            break;
        }
    }

    UA_StatusCode shutdownRes = UA_Server_run_shutdown(server);
    if(res == UA_STATUSCODE_GOOD)
        res = shutdownRes;

cleanup:
    UA_Server_delete(server);
    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
