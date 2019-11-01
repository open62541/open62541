/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright 2020 (c) Kalycito Infotech Private Limited
 *
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/nodestore_default.h>

#include <signal.h>
#include <stdlib.h>

#ifdef UA_ENABLE_USE_ENCODED_NODES
static void usage(void) {
    printf("Usage: server [-lookupTable <lookup table file>] \n"
           "              [-enocdedBin <encoded binary file>] \n");
}
#endif

static volatile UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

#ifdef UA_ENABLE_USE_ENCODED_NODES
    char *lookupTablePath = NULL;
    char *enocdedBinPath = NULL;

    /* At least three argument is required for the binary nodestore */
    if(argc <= 2) {
        usage();
        return EXIT_FAILURE;
    }

    /* Parse the arguments */
    for(int argpos = 1; argpos < argc; argpos++) {
        if(strcmp(argv[argpos], "--help") == 0 ||
           strcmp(argv[argpos], "-h") == 0) {
            usage();
            return EXIT_SUCCESS;
        }

        if(strcmp(argv[argpos], "-lookupTable") == 0) {
            argpos++;
            lookupTablePath = argv[argpos];
            continue;
        }

        if(strcmp(argv[argpos], "-enocdedBin") == 0) {
            argpos++;
            enocdedBinPath = argv[argpos];
            continue;
        }

        usage();
        return EXIT_FAILURE;
    }


    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));

    UA_Nodestore_BinaryEncoded(&config.nodestore, lookupTablePath, enocdedBinPath);
    UA_ServerConfig_setDefault(&config);

    UA_Server *server = NULL;
    server = UA_Server_newWithConfig(&config);
    if(!server) {
        retval |= UA_STATUSCODE_BADINTERNALERROR;
    }

#endif

#ifdef UA_ENABLE_ENCODE_AND_DUMP
    /* Use the iterate before along with encodeNodeCallback to dump the nodes */
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    config->nodestore.iterate(config->nodestore.context, encodeNodeCallback ,NULL);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Nodes encoded and dumped to encodedNode.bin and lookupTable.bin");
#endif

    retval |= UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
