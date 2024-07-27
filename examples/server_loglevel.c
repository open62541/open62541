/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __linux__
#include <getopt.h>
#endif

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    UA_LogLevel log_level = UA_LOGLEVEL_ERROR;

#ifdef __linux__
    static struct option long_options[] = {
            {"help",     no_argument,       0,  'h' },
            {"loglevel", required_argument, 0,  'l' },
            {0,          0,                 0,  0 }
    };
    int long_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv,"hl",
                   long_options, &long_index )) != -1) {
        switch (opt) {
            // This assumes that the mapping between UA_LogLevel and int is known
             case 'l' : log_level = (UA_LogLevel)atoi(optarg);
                 break;
             case 'h' : printf( "Usage server_loglevel --loglevel=level\n" );
                        exit(1);
                 break;
             default: fprintf(stderr,"Illegal argument '%c'\n", opt);
                 exit(EXIT_FAILURE);
        }
    }
#endif


    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    /* Exchange the logger */
    UA_Logger logger = UA_Log_Stdout_withLevel( log_level );
    logger.clear = config->logging->clear;
    *config->logging = logger;

    /* Some data */
    UA_StatusCode retval;
    UA_ObjectTypeAttributes otAttr = UA_ObjectTypeAttributes_default;
    otAttr.description = UA_LOCALIZEDTEXT("en-US", "Some Data");
    otAttr.displayName = UA_LOCALIZEDTEXT("en-US", "data");
    UA_Server_addObjectTypeNode(server, UA_NODEID_NUMERIC(1, 10000),
                                UA_NS0ID(BASEOBJECTTYPE), UA_NS0ID(HASSUBTYPE),
                                UA_QUALIFIEDNAME(1, "data"), otAttr, NULL, NULL);

    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.description =  UA_LOCALIZEDTEXT("en-US", "Some Data");
    vAttr.displayName =  UA_LOCALIZEDTEXT("en-US", "data");
    UA_UInt32 ageVar = 0;
    UA_Variant_setScalar(&vAttr.value, &ageVar, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 10001),
                              UA_NODEID_NUMERIC(1, 10000), UA_NS0ID(HASPROPERTY),
                              UA_QUALIFIEDNAME(1, "data"), UA_NS0ID(BASEDATAVARIABLETYPE),
                              vAttr, NULL, NULL);

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
