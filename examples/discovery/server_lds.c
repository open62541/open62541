/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * Server representing a local discovery server as a central instance.
 * Any other server can register with this server (see server_register.c). Clients can then call the
 * find servers service to get all registered servers (see client_find_servers.c).
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
            UA_String_fromChars("urn:open62541.example.local_discovery_server");
    config->mdnsServerName = UA_String_fromChars("LDS");
    // See http://www.opcfoundation.org/UA/schemas/1.03/ServerCapabilities.csv
    config->serverCapabilitiesSize = 1;
    UA_String *caps = UA_String_new();
    *caps = UA_String_fromChars("LDS");
    config->serverCapabilities = caps;
    /* timeout in seconds when to automatically remove a registered server from
     * the list, if it doesn't re-register within the given time frame. A value
     * of 0 disables automatic removal. Default is 60 Minutes (60*60). Must be
     * bigger than 10 seconds, because cleanup is only triggered approximately
     * every 10 seconds. The server will still be removed depending on the
     * state of the semaphore file. */
    // config.discoveryCleanupTimeout = 60*60;
    UA_Server *server = UA_Server_new(config);

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
