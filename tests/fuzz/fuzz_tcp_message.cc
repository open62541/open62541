/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "custom_memory_manager.h"

#include <pthread.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "ua_server_internal.h"

#include "testing_networklayers.h"

#define RECEIVE_BUFFER_SIZE 65535


volatile bool running = true;

static void *serverLoop(void *server_ptr) {
    UA_Server *server = (UA_Server*) server_ptr;

    while (running) {
        UA_Server_run_iterate(server, true);
    }
    return NULL;
}

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    if (!UA_memoryManager_setLimitFromLast4Bytes(data, size))
        return 0;
    size -= 4;

    UA_Server *server = UA_Server_new();
    if(!server) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create server instance using UA_Server_new");
        return 0;
    }

    UA_ServerConfig *config = UA_Server_getConfig(server);
    if (UA_ServerConfig_setMinimal(config, 4840, NULL) != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return 0;
    }

    // Enable the mDNS announce and response functionality
    config->discovery.mdnsEnable = true;

    config->discovery.mdns.mdnsServerName = UA_String_fromChars("Sample Multicast Server");

    UA_StatusCode retval = UA_Server_run_startup(server);
    if(retval != UA_STATUSCODE_GOOD)
        return 0;

    // Iterate once to initialize the TCP connection. Otherwise the connect below may come before the server is up.
    UA_Server_run_iterate(server, true);

    pthread_t serverThread;
    int rc = pthread_create(&serverThread, NULL, serverLoop, (void *)server);
    if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    int sockfd = 0;
    {
        // create a client and write to localhost TCP server

        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Error : Could not create socket \n");
            return 1;
        }

        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(4840);
        serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        int status = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if( status >= 0)
        {
            write(sockfd, data, size);
        } else {
            printf("Could not connect to server");
            return 1;
        }


    }
    running = false;
    void *status;
    pthread_join(serverThread, &status);

    // Process any remaining data
    UA_Server_run_iterate(server, true);
    close(sockfd);


    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    return 0;
}
