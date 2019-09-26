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

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    // This is an LDS server only. Set the application type to DISCOVERYSERVER.
    // NOTE: This will cause UaExpert to not show this instance in the server list.
    // See also: https://forum.unified-automation.com/topic1987.html
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
            UA_String_fromChars("urn:open62541.example.local_discovery_server");

    // Enable the mDNS announce and response functionality
    config->discovery.mdnsEnable = true;

    config->discovery.mdns.mdnsServerName = UA_String_fromChars("LDS");

    // See http://www.opcfoundation.org/UA/schemas/1.03/ServerCapabilities.csv
    // For a LDS server, you should only indicate the LDS capability.
    // If this instance is an LDS and at the same time a normal OPC UA server, you also have to indicate
    // the additional capabilities.
    // NOTE: UaExpert does not show LDS-only servers in the list.
    // See also: https://forum.unified-automation.com/topic1987.html

    // E.g. here we only set LDS, and you will not see it in UaExpert
    config->discovery.mdns.serverCapabilitiesSize = 1;
    UA_String *caps = (UA_String *) UA_Array_new(1, &UA_TYPES[UA_TYPES_STRING]);
    caps[0] = UA_String_fromChars("LDS");
    config->discovery.mdns.serverCapabilities = caps;

    /* timeout in seconds when to automatically remove a registered server from
     * the list, if it doesn't re-register within the given time frame. A value
     * of 0 disables automatic removal. Default is 60 Minutes (60*60). Must be
     * bigger than 10 seconds, because cleanup is only triggered approximately
     * every 10 seconds. The server will still be removed depending on the
     * state of the semaphore file. */
    // config->discoveryCleanupTimeout = 60*60;

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
