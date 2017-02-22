/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */
/**
 * This client requests all the available servers from the discovery server (see server_lds.c)
 * and then calls GetEndpoints on the returned list of servers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <open62541.h>

UA_Logger logger = UA_Log_Stdout;

#define DISCOVERY_SERVER_ENDPOINT "opc.tcp://localhost:4840"

static UA_StatusCode
FindServers(const char *discoveryServerUrl, size_t *registeredServerSize,
            UA_ApplicationDescription **registeredServers) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect_no_session(client, discoveryServerUrl);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }

    UA_FindServersRequest request;
    UA_FindServersRequest_init(&request);

    /* If you want to find specific servers, you can also include the server
     * URIs in the request: */
    //request.serverUrisSize = 1;
    //request.serverUris = UA_malloc(sizeof(UA_String));
    //request.serverUris[0] = UA_String_fromChars("open62541.example.server_register");

    //request.localeIdsSize = 1;
    //request.localeIds = UA_malloc(sizeof(UA_String));
    //request.localeIds[0] = UA_String_fromChars("en");

    // now send the request
    UA_FindServersResponse response;
    UA_FindServersResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE]);

    //UA_Array_delete(request.serverUris, request.serverUrisSize, &UA_TYPES[UA_TYPES_STRING]);
    //UA_Array_delete(request.localeIds, request.localeIdsSize, &UA_TYPES[UA_TYPES_STRING]);

    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_CLIENT,
                     "FindServers failed with statuscode %s", UA_StatusCode_name(response.responseHeader.serviceResult));
        UA_FindServersResponse_deleteMembers(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return response.responseHeader.serviceResult;
    }

    *registeredServerSize = response.serversSize;
    *registeredServers = (UA_ApplicationDescription *) UA_Array_new(response.serversSize,
                                                                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    for (size_t i = 0; i < response.serversSize; i++)
        UA_ApplicationDescription_copy(&response.servers[i], &(*registeredServers)[i]);
    UA_FindServersResponse_deleteMembers(&response);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}


static UA_StatusCode
FindServersOnNetwork(const char *discoveryServerUrl, size_t *serverOnNetworkSize, UA_ServerOnNetwork **serverOnNetwork) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect_no_session(client, discoveryServerUrl);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return retval;
    }


    UA_FindServersOnNetworkRequest request;
    UA_FindServersOnNetworkRequest_init(&request);

    request.startingRecordId = 0;
    request.maxRecordsToReturn = 0; // get all
    /*
     * Here you can define some filtering rules:
     */
    //request.serverCapabilityFilterSize = 1;
    //request.serverCapabilityFilter[0] = UA_String_fromChars("LDS");

    // now send the request
    UA_FindServersOnNetworkResponse response;
    UA_FindServersOnNetworkResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST],
                        &response, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKRESPONSE]);

    //UA_Array_delete(request.serverCapabilityFilter, request.serverCapabilityFilterSize, &UA_TYPES[UA_TYPES_STRING]);

    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_CLIENT,
                     "FindServersOnNetwork failed with statuscode %s", UA_StatusCode_name(response.responseHeader.serviceResult));
		UA_StatusCode retVal = response.responseHeader.serviceResult;
        UA_FindServersOnNetworkResponse_deleteMembers(&response);
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return retVal;
    }

    *serverOnNetworkSize = response.serversSize;
    *serverOnNetwork = (UA_ServerOnNetwork *) UA_Array_new(response.serversSize, &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    for (size_t i = 0; i < response.serversSize; i++)
        UA_ServerOnNetwork_copy(&response.servers[i], &(*serverOnNetwork)[i]);
    UA_FindServersOnNetworkResponse_deleteMembers(&response);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}

int main(void) {

    /*
     * Example for calling FindServersOnNetwork
     */

    {
        UA_ServerOnNetwork *serverOnNetwork = NULL;
        size_t serverOnNetworkSize = 0;

        UA_StatusCode retval = FindServersOnNetwork(DISCOVERY_SERVER_ENDPOINT, &serverOnNetworkSize, &serverOnNetwork);
        if (retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not call FindServersOnNetwork service. Is the discovery server started? StatusCode %s",
                         UA_StatusCode_name(retval));
            return (int) retval;
        }

        // output all the returned/registered servers
        for (size_t i = 0; i < serverOnNetworkSize; i++) {
            UA_ServerOnNetwork *server = &serverOnNetwork[i];
            printf("Server[%lu]: %.*s", (unsigned long) i, (int) server->serverName.length, server->serverName.data);
            printf("\n\tRecordID: %d", server->recordId);
            printf("\n\tDiscovery URL: %.*s", (int) server->discoveryUrl.length, server->discoveryUrl.data);
            printf("\n\tCapabilities: ");
            for (size_t j = 0; j < server->serverCapabilitiesSize; j++) {
                printf("%.*s,", (int) server->serverCapabilities[j].length, server->serverCapabilities[j].data);
            }
            printf("\n\n");
        }

        UA_Array_delete(serverOnNetwork, serverOnNetworkSize, &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    }

    /*
     * Example for calling FindServers
     */


    UA_ApplicationDescription *applicationDescriptionArray = NULL;
    size_t applicationDescriptionArraySize = 0;

    UA_StatusCode retval = FindServers(DISCOVERY_SERVER_ENDPOINT,
                                       &applicationDescriptionArraySize,
                                       &applicationDescriptionArray);
    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER, "Could not call FindServers service. "
                "Is the discovery server started? StatusCode %s", UA_StatusCode_name(retval));
        return (int) retval;
    }

    // output all the returned/registered servers
    for (size_t i = 0; i < applicationDescriptionArraySize; i++) {
        UA_ApplicationDescription *description = &applicationDescriptionArray[i];
        printf("Server[%lu]: %.*s", (unsigned long) i, (int) description->applicationUri.length,
               description->applicationUri.data);
        printf("\n\tName: %.*s", (int) description->applicationName.text.length,
               description->applicationName.text.data);
        printf("\n\tApplication URI: %.*s", (int) description->applicationUri.length,
               description->applicationUri.data);
        printf("\n\tProduct URI: %.*s", (int) description->productUri.length,
               description->productUri.data);
        printf("\n\tType: ");
        switch (description->applicationType) {
            case UA_APPLICATIONTYPE_SERVER:
                printf("Server");
                break;
            case UA_APPLICATIONTYPE_CLIENT:
                printf("Client");
                break;
            case UA_APPLICATIONTYPE_CLIENTANDSERVER:
                printf("Client and Server");
                break;
            case UA_APPLICATIONTYPE_DISCOVERYSERVER:
                printf("Discovery Server");
                break;
            default:
                printf("Unknown");
        }
        printf("\n\tDiscovery URLs:");
        for (size_t j = 0; j < description->discoveryUrlsSize; j++) {
            printf("\n\t\t[%lu]: %.*s", (unsigned long) j,
                   (int) description->discoveryUrls[j].length,
                   description->discoveryUrls[j].data);
        }
        printf("\n\n");
    }


    /*
     * Now that we have the list of available servers, call get endpoints on all of them
     */

    printf("-------- Server Endpoints --------\n");

    for (size_t i = 0; i < applicationDescriptionArraySize; i++) {
        UA_ApplicationDescription *description = &applicationDescriptionArray[i];
        if (description->discoveryUrlsSize == 0) {
            UA_LOG_INFO(logger, UA_LOGCATEGORY_CLIENT,
                        "[GetEndpoints] Server %.*s did not provide any discovery urls. Skipping.",
                        description->applicationUri);
            continue;
        }

        printf("\nEndpoints for Server[%lu]: %.*s\n", (unsigned long) i,
               (int) description->applicationUri.length, description->applicationUri.data);

        UA_Client *client = UA_Client_new(UA_ClientConfig_standard);

        char *discoveryUrl = (char *) malloc(sizeof(char) * description->discoveryUrls[0].length + 1);
        memcpy(discoveryUrl, description->discoveryUrls[0].data, description->discoveryUrls[0].length);
        discoveryUrl[description->discoveryUrls[0].length] = '\0';

        UA_EndpointDescription *endpointArray = NULL;
        size_t endpointArraySize = 0;
        retval = UA_Client_getEndpoints(client, discoveryUrl, &endpointArraySize, &endpointArray);
        free(discoveryUrl);
        if (retval != UA_STATUSCODE_GOOD) {
            UA_Client_disconnect(client);
            UA_Client_delete(client);
            break;
        }

        for (size_t j = 0; j < endpointArraySize; j++) {
            UA_EndpointDescription *endpoint = &endpointArray[j];
            printf("\n\tEndpoint[%lu]:", (unsigned long) j);
            printf("\n\t\tEndpoint URL: %.*s", (int) endpoint->endpointUrl.length, endpoint->endpointUrl.data);
            printf("\n\t\tTransport profile URI: %.*s", (int) endpoint->transportProfileUri.length,
                   endpoint->transportProfileUri.data);
            printf("\n\t\tSecurity Mode: ");
            switch (endpoint->securityMode) {
                case UA_MESSAGESECURITYMODE_INVALID:
                    printf("Invalid");
                case UA_MESSAGESECURITYMODE_NONE:
                    printf("None");
                case UA_MESSAGESECURITYMODE_SIGN:
                    printf("Sign");
                case UA_MESSAGESECURITYMODE_SIGNANDENCRYPT:
                    printf("Sign and Encrypt");
            }
            printf("\n\t\tSecurity profile URI: %.*s", (int) endpoint->securityPolicyUri.length,
                   endpoint->securityPolicyUri.data);
            printf("\n\t\tSecurity Level: %d", endpoint->securityLevel);
        }

        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
    }

    printf("\n");

    UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    return (int) UA_STATUSCODE_GOOD;
}
