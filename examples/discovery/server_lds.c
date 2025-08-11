/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. 
 * Copyright (c) 2025 Construction Future Lab gGmbH (Author: Jianbin Liu)
 */
/*
 * Server representing a local discovery server as a central instance.
 * Any other server can register with this server (see server_register.c). Clients can then call the
 * find servers service to get all registered servers (see client_find_servers.c).
 * NOTE: Requires a local mDNS/Bonjour service (e.g., Avahi on Linux) to be running.
 * Without it, multicast discovery will not work.
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

#include <signal.h>
#include <stdlib.h>

static UA_Boolean running = true;
static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received Ctrl-c");
    running = false;
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_StatusCode retval = UA_ServerConfig_setDefault(config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    // This is an LDS server only. Set the application type to DISCOVERYSERVER.
    // NOTE: This will cause UaExpert to not show this instance in the server list.
    // See also: https://forum.unified-automation.com/topic1987.html
    // NOTE: In production, the ApplicationURI must be globally unique and stable for this application instance.
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
            UA_String_fromChars("urn:open62541.example.local_discovery_server");
    
    UA_LocalizedText_clear(&config->applicationDescription.applicationName);
    config->applicationDescription.applicationName =
        UA_LOCALIZEDTEXT_ALLOC("en-US", "Open62541 Local Discovery Server");

    // Enable the mDNS announce and response functionality
    config->mdnsEnabled = true;

    config->mdnsConfig.mdnsServerName = UA_String_fromChars("LDS");

    // See http://www.opcfoundation.org/UA/schemas/1.03/ServerCapabilities.csv
    // For a LDS server, you should only indicate the LDS capability.
    // If this instance is an LDS and at the same time a normal OPC UA server, you also have to indicate
    // the additional capabilities.
    // NOTE: UaExpert does not show LDS-only servers in the list.
    // See also: https://forum.unified-automation.com/topic1987.html

    // E.g. here we only set LDS, and you will not see it in UaExpert
    config->mdnsConfig.serverCapabilitiesSize = 1;
    UA_String *caps = (UA_String *) UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    if(!caps) {
        /* Allocation failed for server capabilities array.
         * Clean up and exit to avoid running with invalid config. */
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }
    caps[0] = UA_String_fromChars("LDS");
    config->mdnsConfig.serverCapabilities = caps;

    /* timeout in seconds when to automatically remove a registered server from
     * the list, if it doesn't re-register within the given time frame. A value
     * of 0 disables automatic removal. Default is 60 Minutes (60*60). Must be
     * bigger than 10 seconds, because cleanup is only triggered approximately
     * every 10 seconds. The server will still be removed depending on the
     * state of the semaphore file. */
    // config->discoveryCleanupTimeout = 60*60;

    retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
