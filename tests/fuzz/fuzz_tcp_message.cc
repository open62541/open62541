/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2019 (c) fortiss (Author: Stefan Profanter)
 */


#include "custom_memory_manager.h"

#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "ua_server_internal.h"

#include "testing_networklayers.h"

#define RECEIVE_BUFFER_SIZE 65535
#define SERVER_PORT 4840

volatile bool running = true;

static void *serverLoop(void *server_ptr) {
    UA_Server *server = (UA_Server*) server_ptr;

    while (running) {
        UA_Server_run_iterate(server, false);
    }
    return NULL;
}

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    // Allow the fuzzer to at least create all the necessary structs before limiting memory.
    // Otherwise fuzzing is useless
    UA_memoryManager_setLimit((unsigned long long) -1);

    /* less debug output */
    UA_ServerConfig initialConfig;
    memset(&initialConfig, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setMinimal(&initialConfig, SERVER_PORT, NULL);
    initialConfig.allowEmptyVariables = UA_RULEHANDLING_ACCEPT;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(&initialConfig);
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not generate the server config");
        return EXIT_FAILURE;
    }

    UA_Server *server = UA_Server_newWithConfig(&initialConfig);
    if(!server) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create server instance using UA_Server_new");
        return EXIT_FAILURE;
    }

    UA_ServerConfig *config = UA_Server_getConfig(server);

    // Enable the mDNS announce and response functionality
    config->mdnsEnabled = true;
    config->mdnsConfig.mdnsServerName = UA_String_fromChars("Sample Multicast Server");

    retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not run UA_Server_run_startup. %s", UA_StatusCode_name(retval));
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    if (!UA_memoryManager_setLimitFromLast4Bytes(data, size)) {
        UA_Server_run_shutdown(server);
        UA_Server_delete(server);
        return EXIT_SUCCESS;
    }
    size -= 4;

    // Iterate once to initialize the TCP connection. Otherwise the connect below may come before the server is up.
    UA_Server_run_iterate(server, true);

    pthread_t serverThread;
    int rc = pthread_create(&serverThread, NULL, serverLoop, (void *)server);
    if (rc){

        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "return code from pthread_create() is %d", rc);

        UA_Server_run_shutdown(server);
        UA_Server_delete(server);
        return -1;
    }

    int retCode = EXIT_SUCCESS;

    int sockfd = 0;
    {
        // create a client and write to localhost TCP server

        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,
                         "Could not create socket");
            retCode = EXIT_FAILURE;
        } else {

            struct sockaddr_in serv_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(SERVER_PORT);
            serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

            int status = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
            if (status >= 0) {
                if (write(sockfd, data, size) != size) {
                    UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,
                                 "Did not write %lu bytes", (long unsigned)size);
                    retCode = EXIT_FAILURE;
                }
            } else {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,
                             "Could not connect to server: %s", strerror(errno));
                retCode = EXIT_FAILURE;
            }
        }

    }
    running = false;
    void *status;
    pthread_join(serverThread, &status);

    // Process any remaining data. Just repeat a few times to empty all the buffered bytes
    for (size_t i=0; i<5; i++) {
        UA_Server_run_iterate(server, false);
    }
    close(sockfd);


    UA_Server_run_shutdown(server);
    UA_Server_delete(server);

    return retCode;
}
