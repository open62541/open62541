/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2024 (c) Fraunhofer IOSB (Author: Noel Graf)
 */

#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>

#include "common.h"

UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_String storePath = UA_STRING_NULL;

    if(argc >= 3) {
        /* Load certificate and private key */
        certificate = loadFile(argv[1]);
        privateKey = loadFile(argv[2]);
    } else {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Missing arguments. Arguments are "
                     "<server-certificate.der> <private-key.der> [<path/to/pki/folder>]");
        return EXIT_FAILURE;
    }

    if(argc >= 4) {
        storePath = UA_STRING(argv[3]);
    } else {
        /* If no path is specified, the current working directory is used */
        char storePathDir[4096];
        if(!getcwd(storePathDir, sizeof(storePathDir)))
            return UA_STATUSCODE_BADINTERNALERROR;
        storePath = UA_STRING(storePathDir);
    }

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_StatusCode retval =
            UA_ServerConfig_setDefaultWithFilestore(config, 4840, &certificate, &privateKey, storePath);

    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    if(!running)
        goto cleanup; /* received ctrl-c already */

    retval = UA_Server_run(server, &running);

cleanup:
    UA_Server_delete(server);
    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    UA_String_clear(&storePath);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
