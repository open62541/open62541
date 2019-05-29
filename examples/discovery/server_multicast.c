/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/*
 * A simple server instance which registers with the discovery server.
 * Compared to server_register.c this example waits until the LDS server announces
 * itself through mDNS. Therefore the LDS server needs to support multicast extension
 * (i.e., LDS-ME).
 */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

const UA_ByteString UA_SECURITY_POLICY_BASIC128_URI =
    {56, (UA_Byte *)"http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15"};

UA_Boolean running = true;

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

char *discovery_url = NULL;

static void
serverOnNetworkCallback(const UA_ServerOnNetwork *serverOnNetwork, UA_Boolean isServerAnnounce,
                        UA_Boolean isTxtReceived, void *data) {

    if(discovery_url != NULL || !isServerAnnounce) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "serverOnNetworkCallback called, but discovery URL "
                     "already initialized or is not announcing. Ignoring.");
        return; // we already have everything we need or we only want server announces
    }

    if(!isTxtReceived)
        return; // we wait until the corresponding TXT record is announced.
                // Problem: how to handle if a Server does not announce the
                // optional TXT?

    // here you can filter for a specific LDS server, e.g. call FindServers on
    // the serverOnNetwork to make sure you are registering with the correct
    // LDS. We will ignore this for now
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Another server announced itself on %.*s",
                (int)serverOnNetwork->discoveryUrl.length, serverOnNetwork->discoveryUrl.data);

    if(discovery_url != NULL)
        UA_free(discovery_url);
    discovery_url = (char*)UA_malloc(serverOnNetwork->discoveryUrl.length + 1);
    memcpy(discovery_url, serverOnNetwork->discoveryUrl.data, serverOnNetwork->discoveryUrl.length);
    discovery_url[serverOnNetwork->discoveryUrl.length] = 0;
}

/*
 * Get the endpoint from the server, where we can call RegisterServer2 (or RegisterServer).
 * This is normally the endpoint with highest supported encryption mode.
 *
 * @param discoveryServerUrl The discovery url from the remote server
 * @return The endpoint description (which needs to be freed) or NULL
 */
static
UA_EndpointDescription *getRegisterEndpointFromServer(const char *discoveryServerUrl) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_EndpointDescription *endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, discoveryServerUrl,
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "GetEndpoints failed with %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return NULL;
    }

    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Server has %lu endpoints", (unsigned long)endpointArraySize);
    UA_EndpointDescription *foundEndpoint = NULL;
    for(size_t i = 0; i < endpointArraySize; i++) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "\tURL = %.*s, SecurityMode = %s",
                     (int) endpointArray[i].endpointUrl.length,
                     endpointArray[i].endpointUrl.data,
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_NONE ? "None" :
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_SIGN ? "Sign" :
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ? "SignAndEncrypt" :
                     "Invalid"
        );
        // find the endpoint with highest supported security mode
        if((UA_String_equal(&endpointArray[i].securityPolicyUri, &UA_SECURITY_POLICY_NONE_URI) ||
            UA_String_equal(&endpointArray[i].securityPolicyUri, &UA_SECURITY_POLICY_BASIC128_URI)) && (
            foundEndpoint == NULL || foundEndpoint->securityMode < endpointArray[i].securityMode))
            foundEndpoint = &endpointArray[i];
    }
    UA_EndpointDescription *returnEndpoint = NULL;
    if(foundEndpoint != NULL) {
        returnEndpoint = UA_EndpointDescription_new();
        UA_EndpointDescription_copy(foundEndpoint, returnEndpoint);
    }
    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
    return returnEndpoint;
}

#ifdef UA_ENABLE_ENCRYPTION
/* loadFile parses the certificate file.
 *
 * @param  path               specifies the file name given in argv[]
 * @return Returns the file content after parsing */
static UA_ByteString loadFile(const char *const path) {
    UA_ByteString fileContents = UA_BYTESTRING_NULL;
    if(path == NULL)
        return fileContents;

    /* Open the file */
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; /* We read errno also from the tcp layer */
        return fileContents;
    }

    /* Get the file length, allocate the data and read */
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t) ftell(fp);
    fileContents.data = (UA_Byte *) UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }

    fclose(fp);
    return fileContents;
}
#endif

/**
 * Initialize a client instance which is used for calling the registerServer service.
 * If the given endpoint has securityMode NONE, a client with default configuration
 * is returned.
 * If it is using SignAndEncrypt, the client certificates must be provided as a
 * command line argument and then the client is initialized using these certificates.
 * @param endpointRegister The remote endpoint where this server should register
 * @param argc from the main method
 * @param argv from the main method
 * @return NULL or the initialized non-connected client
 */
static
UA_Client *getRegisterClient(UA_EndpointDescription *endpointRegister, int argc, char **argv) {
    if(endpointRegister->securityMode == UA_MESSAGESECURITYMODE_NONE) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Using LDS endpoint with security None");
        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));
        return client;
    }
#ifdef UA_ENABLE_ENCRYPTION
    if(endpointRegister->securityMode == UA_MESSAGESECURITYMODE_SIGN) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "LDS endpoint which only supports Sign is currently not supported");
        return NULL;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Using LDS endpoint with security SignAndEncrypt");

    UA_ByteString certificate = UA_BYTESTRING_NULL;
    UA_ByteString privateKey = UA_BYTESTRING_NULL;
    UA_ByteString *trustList = NULL;
    size_t trustListSize = 0;
    UA_ByteString *revocationList = NULL;
    size_t revocationListSize = 0;


    if(argc < 3) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "The Certificate and key is missing."
                         "The required arguments are "
                         "<client-certificate.der> <client-private-key.der> "
                         "[<trustlist1.crl>, ...]");
        return NULL;
    }
    certificate = loadFile(argv[1]);
    privateKey = loadFile(argv[2]);

    /* Load the trustList. Load revocationList is not supported now */
    if(argc > 3) {
        trustListSize = (size_t) argc - 3;
        UA_StatusCode retval = UA_ByteString_allocBuffer(trustList, trustListSize);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_ByteString_clear(&certificate);
            UA_ByteString_clear(&privateKey);
            return NULL;
        }

        for(size_t trustListCount = 0; trustListCount < trustListSize; trustListCount++) {
            trustList[trustListCount] = loadFile(argv[trustListCount + 3]);
        }
    }

    /* Secure client initialization */
    UA_Client *clientRegister = UA_Client_new();
    UA_ClientConfig *cc = UA_Client_getConfig(clientRegister);
    UA_ClientConfig_setDefaultEncryption(cc, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);
    cc->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;

    UA_ByteString_clear(&certificate);
    UA_ByteString_clear(&privateKey);
    for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++)
        UA_ByteString_clear(&trustList[deleteCount]);

    return clientRegister;
#else
    return NULL;
#endif
}

int main(int argc, char **argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    // use port 0 to dynamically assign port
    UA_ServerConfig_setMinimal(config, 0, NULL);

    // An LDS server normally has the application type to DISCOVERYSERVER.
    // Since this instance implements LDS and other OPC UA services, we set the type to SERVER.
    // NOTE: Using DISCOVERYSERVER will cause UaExpert to not show this instance in the server list.
    // See also: https://forum.unified-automation.com/topic1987.html

    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_SERVER;
    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.example.server_multicast");

    // Enable the mDNS announce and response functionality
    config->discovery.mdnsEnable = true;

    config->discovery.mdns.mdnsServerName = UA_String_fromChars("Sample Multicast Server");

    // See http://www.opcfoundation.org/UA/schemas/1.03/ServerCapabilities.csv
    // For a LDS server, you should only indicate the LDS capability.
    // If this instance is an LDS and at the same time a normal OPC UA server, you also have to indicate
    // the additional capabilities.
    // NOTE: UaExpert does not show LDS-only servers in the list.
    // See also: https://forum.unified-automation.com/topic1987.html

    config->discovery.mdns.serverCapabilitiesSize = 2;
    UA_String *caps = (UA_String *) UA_Array_new(2, &UA_TYPES[UA_TYPES_STRING]);
    caps[0] = UA_String_fromChars("LDS");
    caps[1] = UA_String_fromChars("NA");
    config->discovery.mdns.serverCapabilities = caps;

    // Start the server and call iterate to wait for the multicast discovery of the LDS
    UA_StatusCode retval = UA_Server_run_startup(server);

    // callback which is called when a new server is detected through mDNS
    // needs to be set after UA_Server_run_startup or UA_Server_run
    UA_Server_setServerOnNetworkCallback(server, serverOnNetworkCallback, NULL);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not start the server. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Server_delete(server);
        UA_free(discovery_url);
        return EXIT_FAILURE;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Server started. Waiting for announce of LDS Server.");
    while (running && discovery_url == NULL)
        UA_Server_run_iterate(server, true);
    if(!running) {
        UA_Server_delete(server);
        UA_free(discovery_url);
        return EXIT_FAILURE;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "LDS-ME server found on %s", discovery_url);

    /* Check if the server supports sign and encrypt. OPC Foundation LDS
     * requires an encrypted session for RegisterServer call, our server
     * currently uses encrpytion optionally */
    UA_EndpointDescription *endpointRegister = getRegisterEndpointFromServer(discovery_url);
    UA_free(discovery_url);
    if(endpointRegister == NULL || endpointRegister->securityMode == UA_MESSAGESECURITYMODE_INVALID) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not find any suitable endpoints on discovery server");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    UA_Client *clientRegister = getRegisterClient(endpointRegister, argc, argv);
    if(!clientRegister) {
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                     "Could not create the client for remote registering");
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* Connect the client */
    char *endpointUrl = (char*)UA_malloc(endpointRegister->endpointUrl.length + 1);
    memcpy(endpointUrl, endpointRegister->endpointUrl.data, endpointRegister->endpointUrl.length);
    endpointUrl[endpointRegister->endpointUrl.length] = 0;
    UA_EndpointDescription_delete(endpointRegister);
    retval = UA_Server_addPeriodicServerRegisterCallback(server, clientRegister, endpointUrl,
                                                         10 * 60 * 1000, 500, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create periodic job for server register. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_free(endpointUrl);
        UA_Client_disconnect(clientRegister);
        UA_Client_delete(clientRegister);
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    while (running)
        UA_Server_run_iterate(server, true);

    UA_Server_run_shutdown(server);

    // UNregister the server from the discovery server.
    retval = UA_Server_unregister_discovery(server, clientRegister);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not unregister server from discovery server. "
                     "StatusCode %s", UA_StatusCode_name(retval));

    UA_free(endpointUrl);
    UA_Client_disconnect(clientRegister);
    UA_Client_delete(clientRegister);
    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
