/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "nodeset_loader_file.h"

#include <signal.h>
#include <stdio.h>

static volatile UA_Boolean running = true;

static void
stopHandler(int sign) {
    (void)sign;
    running = false;
}

int
main(int argc, const char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    if(!server)
        return EXIT_FAILURE;

    UA_StatusCode res = UA_ServerConfig_setDefault(UA_Server_getConfig(server));
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    for(int cnt = 1; cnt < argc; cnt++) {
        res = loadNodesetFile(server, argv[cnt]);
        if(res != UA_STATUSCODE_GOOD) {
            printf("Nodeset %s could not be loaded, exit\n", argv[cnt]);
            goto cleanup;
        }
    }

    res = UA_Server_run(server, &running);

cleanup:
    UA_Server_delete(server);
    return res == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
