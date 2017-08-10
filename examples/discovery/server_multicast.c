/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/*
 * A simple server instance which registers with the discovery server.
 * Compared to server_register.c this example waits until the LDS server announces
 * itself through mDNS. Therefore the LDS server needs to support multicast extension
 * (i.e., LDS-ME).
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "open62541.h"

UA_Logger logger = UA_Log_Stdout;
UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static UA_StatusCode
readInteger(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {
    dataValue->hasValue = true;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32 *) handle, &UA_TYPES[UA_TYPES_INT32]);
    // we know the nodeid is a string
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node read %.*s",
                (int)nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "read value %i", *(UA_UInt32 *) handle);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeInteger(void *handle, const UA_NodeId nodeid,
             const UA_Variant *data, const UA_NumericRange *range) {
    if (UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_INT32] && data->data) {
        *(UA_UInt32 *) handle = *(UA_UInt32 *) data->data;
    }
    // we know the nodeid is a string
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node written %.*s",
                (int)nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "written value %i", *(UA_UInt32 *) handle);
    return UA_STATUSCODE_GOOD;
}

char *discovery_url = NULL;
UA_String *self_discovery_url = NULL;

static void
serverOnNetworkCallback(const UA_ServerOnNetwork *serverOnNetwork, UA_Boolean isServerAnnounce,
                        UA_Boolean isTxtReceived, void *data) {

    if (discovery_url != NULL || !isServerAnnounce) {
        UA_LOG_DEBUG(logger, UA_LOGCATEGORY_SERVER,
                     "serverOnNetworkCallback called, but discovery URL "
                     "already initialized or is not announcing. Ignoring.");
        return; // we already have everything we need or we only want server announces
    }

    if (self_discovery_url != NULL && UA_String_equal(&serverOnNetwork->discoveryUrl, self_discovery_url)) {
        // skip self
        return;
    }

    if (!isTxtReceived)
        return; // we wait until the corresponding TXT record is announced.
                // Problem: how to handle if a Server does not announce the
                // optional TXT?

    // here you can filter for a specific LDS server, e.g. call FindServers on
    // the serverOnNetwork to make sure you are registering with the correct
    // LDS. We will ignore this for now
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Another server announced itself on %.*s",
                (int)serverOnNetwork->discoveryUrl.length, serverOnNetwork->discoveryUrl.data);

    if (discovery_url != NULL)
        UA_free(discovery_url);
    discovery_url = (char*)UA_malloc(serverOnNetwork->discoveryUrl.length + 1);
    memcpy(discovery_url, serverOnNetwork->discoveryUrl.data, serverOnNetwork->discoveryUrl.length);
    discovery_url[serverOnNetwork->discoveryUrl.length] = 0;
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    // To enable mDNS discovery, set application type to discovery server.
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;
    UA_String_deleteMembers(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.example.server_multicast");
    config->mdnsServerName = UA_String_fromChars("Sample Multicast Server");
    // See http://www.opcfoundation.org/UA/schemas/1.03/ServerCapabilities.csv
    //config.serverCapabilitiesSize = 1;
    //UA_String caps = UA_String_fromChars("LDS");
    //config.serverCapabilities = &caps;
    UA_Server *server = UA_Server_new(config);
    self_discovery_url = &config->networkLayers[0].discoveryUrl;

    /* add a variable node to the address space */
    UA_Int32 myInteger = 42;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_DataSource dateDataSource;
    dateDataSource.handle = &myInteger;
    dateDataSource.read = readInteger;
    dateDataSource.write = writeInteger;
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    attr.description = UA_LOCALIZEDTEXT("en_US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en_US", "the answer");

    UA_Server_addDataSourceVariableNode(server, myIntegerNodeId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        myIntegerName, UA_NODEID_NULL, attr, dateDataSource, NULL);

    // callback which is called when a new server is detected through mDNS
    UA_Server_setServerOnNetworkCallback(server, serverOnNetworkCallback, NULL);

    // Start the server and call iterate to wait for the multicast discovery of the LDS
    UA_StatusCode retval = UA_Server_run_startup(server);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "Could not start the server. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Server_delete(server);
        UA_ServerConfig_delete(config);
        UA_free(discovery_url);
        return 1;
    }
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER,
                "Server started. Waiting for announce of LDS Server.");
    while (running && discovery_url == NULL)
        UA_Server_run_iterate(server, true);
    if (!running) {
        UA_Server_delete(server);
        UA_ServerConfig_delete(config);
        UA_free(discovery_url);
        return 1;
    }
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "LDS-ME server found on %s", discovery_url);

    // periodic server register after 10 Minutes, delay first register for 500ms
    retval = UA_Server_addPeriodicServerRegisterCallback(server, discovery_url,
                                                         10 * 60 * 1000, 500, NULL);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "Could not create periodic job for server register. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Server_delete(server);
        UA_ServerConfig_delete(config);
        return 1;
    }

    while (running)
        UA_Server_run_iterate(server, true);

    UA_Server_run_shutdown(server);

    // UNregister the server from the discovery server.
    retval = UA_Server_unregister_discovery(server, discovery_url);
    //retval = UA_Server_unregister_discovery(server, "opc.tcp://localhost:4840" );
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "Could not unregister server from discovery server. "
                     "StatusCode %s", UA_StatusCode_name(retval));

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    UA_free(discovery_url);
    return (int)retval;
}
