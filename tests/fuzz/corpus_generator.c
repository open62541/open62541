/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * This code is used to generate a binary file for every request type
 * which can be sent from a client to the server.
 * These files form the basic corpus for fuzzing the server.
 * This script is intended to be executed manually and then commit the new
 * corpus to the repository.
 */

#ifndef UA_DEBUG_DUMP_PKGS_FILE
#error UA_DEBUG_DUMP_PKGS_FILE must be defined
#endif

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "client/ua_client_internal.h"
#include <server/ua_server_internal.h>
#include "test_helpers.h"

#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>

UA_Server *server;
UA_Boolean running;
pthread_t server_thread;

static void * serverloop(void *_) {
    while(running)
        UA_Server_run_iterate(server, true);
    return NULL;
}

static void start_server(void) {
    running = true;

    /* less log output */
    UA_ServerConfig initialConfig;
    memset(&initialConfig, 0, sizeof(UA_ServerConfig));
    UA_ServerConfig_setDefault(&initialConfig);
    initialConfig.allowEmptyVariables = UA_RULEHANDLING_ACCEPT;
    server = UA_Server_newWithConfig(&initialConfig);

    UA_ServerConfig *config = UA_Server_getConfig(server);
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_SERVER;
    config->mdnsEnabled = true;
    config->mdnsConfig.mdnsServerName = UA_String_fromChars("Sample Multicast Server");

    UA_Server_run_startup(server);
    pthread_create(&server_thread, NULL, serverloop, NULL);
}

static void teardown_server(void) {
    running = false;
    pthread_join(server_thread, NULL);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void emptyCorpusDir(void) {
    DIR *theFolder = opendir(UA_CORPUS_OUTPUT_DIR);
    struct dirent *next_file;
    char filepath[400];

    while ( (next_file = readdir(theFolder)) != NULL ) {
        // build the path for each file in the folder
        sprintf(filepath, "%s/%s", UA_CORPUS_OUTPUT_DIR, next_file->d_name);
        remove(filepath);
    }
    closedir(theFolder);
}

#define ASSERT_GOOD(X) if (X != UA_STATUSCODE_GOOD) return X;

/*************************************************
 * The following list of client requests is based
 * on ua_server_binary.c:getServicePointers to
 * cover all possible services and their inputs
 ************************************************/

static UA_StatusCode
findServersRequest(UA_Client *client) {
    UA_ApplicationDescription* applicationDescriptionArray = NULL;
    size_t applicationDescriptionArraySize = 0;

    size_t serverUrisSize = 1;
    UA_String *serverUris = UA_String_new();
    serverUris[0] = UA_String_fromChars("urn:some:server:uri");

    size_t localeIdsSize = 1;
    UA_String *localeIds = UA_String_new();
    localeIds[0] = UA_String_fromChars("en");

    ASSERT_GOOD(UA_Client_findServers(client, "opc.tcp://localhost:4840",
                                  serverUrisSize, serverUris, localeIdsSize, localeIds,
                                  &applicationDescriptionArraySize, &applicationDescriptionArray));

    UA_Array_delete(serverUris, serverUrisSize, &UA_TYPES[UA_TYPES_STRING]);
    UA_Array_delete(localeIds, localeIdsSize, &UA_TYPES[UA_TYPES_STRING]);
    UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
findServersOnNetworkRequest(UA_Client *client) {
    UA_ServerOnNetwork* serverOnNetwork = NULL;
    size_t serverOnNetworkSize = 0;


    size_t  serverCapabilityFilterSize = 2;
    UA_String *serverCapabilityFilter = (UA_String*)UA_malloc(sizeof(UA_String) * serverCapabilityFilterSize);
    serverCapabilityFilter[0] = UA_String_fromChars("LDS");
    serverCapabilityFilter[1] = UA_String_fromChars("NA");


    ASSERT_GOOD(UA_Client_findServersOnNetwork(client, "opc.tcp://localhost:4840", 0, 0,
                                           serverCapabilityFilterSize, serverCapabilityFilter,
                                           &serverOnNetworkSize, &serverOnNetwork));

    UA_Array_delete(serverCapabilityFilter, serverCapabilityFilterSize,
                        &UA_TYPES[UA_TYPES_STRING]);
    UA_Array_delete(serverOnNetwork, serverOnNetworkSize, &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    return UA_STATUSCODE_GOOD;
}

static void
initUaRegisterServer(UA_RegisteredServer *requestServer) {
    requestServer->isOnline = UA_TRUE;
    requestServer->serverUri = server->config.applicationDescription.applicationUri;
    requestServer->productUri = server->config.applicationDescription.productUri;
    requestServer->serverType = server->config.applicationDescription.applicationType;
    requestServer->gatewayServerUri = server->config.applicationDescription.gatewayServerUri;

    // create the semaphore
    int fd = open("/tmp/open62541-corpus-semaphore", O_RDWR|O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    close(fd);
    requestServer->semaphoreFilePath = UA_STRING("/tmp/open62541-corpus-semaphore");

    requestServer->serverNames = &server->config.applicationDescription.applicationName;
    requestServer->serverNamesSize = 1;

    size_t nl_discurls = server->config.serverUrlsSize;
    requestServer->discoveryUrls = (UA_String*)UA_malloc(sizeof(UA_String) * nl_discurls);
    requestServer->discoveryUrlsSize = nl_discurls;
    for(size_t i = 0; i < nl_discurls; ++i) {
        requestServer->discoveryUrls[i] = server->config.serverUrls[i];
    }
}

static UA_StatusCode
registerServerRequest(UA_Client *client) {
    /* Prepare the request. Do not cleanup the request after the service call,
     * as the members are stack-allocated or point into the server config. */
    UA_RegisterServerRequest request;
    UA_RegisterServerRequest_init(&request);
    /* Copy from RegisterServer2 request */
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    initUaRegisterServer(&request.server);


    UA_RegisterServerResponse response;
    UA_RegisterServerResponse_init(&response);

    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST],
                        &response, &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE]);

    UA_free(request.server.discoveryUrls);
    ASSERT_GOOD(response.responseHeader.serviceResult);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
registerServer2Request(UA_Client *client) {
    /* Prepare the request. Do not cleanup the request after the service call,
     * as the members are stack-allocated or point into the server config. */
    UA_RegisterServer2Request request;
    UA_RegisterServer2Request_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    initUaRegisterServer(&request.server);

    request.discoveryConfigurationSize = 1;
    request.discoveryConfiguration = UA_ExtensionObject_new();
    UA_ExtensionObject_init(&request.discoveryConfiguration[0]);
    // Set to NODELETE so that we can just use a pointer to the mdns config
    request.discoveryConfiguration[0].encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    request.discoveryConfiguration[0].content.decoded.type = &UA_TYPES[UA_TYPES_MDNSDISCOVERYCONFIGURATION];
    request.discoveryConfiguration[0].content.decoded.data = &server->config.mdnsConfig;

    // First try with RegisterServer2, if that isn't implemented, use RegisterServer
    UA_RegisterServer2Response response;
    UA_RegisterServer2Response_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST],
                        &response, &UA_TYPES[UA_TYPES_REGISTERSERVER2RESPONSE]);

    ASSERT_GOOD(response.responseHeader.serviceResult);
    UA_free(request.server.discoveryUrls);
    UA_ExtensionObject_delete(request.discoveryConfiguration);

    UA_RegisterServer2Response_clear(&response);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readValueRequest(UA_Client *client) {
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME);
    rvi.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataValue resp = UA_Server_read(server, &rvi, UA_TIMESTAMPSTORETURN_BOTH);
    ASSERT_GOOD(resp.status);

    UA_DataValue_clear(&resp);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeValueRequest(UA_Client *client) {
    UA_WriteValue wValue;
    UA_WriteValue_init(&wValue);
    UA_LocalizedText testValue = UA_LOCALIZEDTEXT("en-EN", "MyServer");
    UA_Variant_setScalar(&wValue.value.value, &testValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    wValue.value.hasValue = true;
    wValue.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    wValue.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
    ASSERT_GOOD(UA_Server_write(server, &wValue));

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
browseAndBrowseNextRequest(UA_Client *client) {
    // Browse node in server folder
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    // normally is set to 0, to get all the nodes, but we want to test browse next
    bReq.requestedMaxReferencesPerNode = 1;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;

    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    ASSERT_GOOD(bResp.responseHeader.serviceResult);

    // browse next
    UA_BrowseNextRequest bNextReq;
    UA_BrowseNextRequest_init(&bNextReq);
    // normally is set to 0, to get all the nodes, but we want to test browse next
    bNextReq.releaseContinuationPoints = UA_FALSE;
    bNextReq.continuationPoints = &bResp.results[0].continuationPoint;
    bNextReq.continuationPointsSize = 1;

    UA_BrowseNextResponse bNextResp = UA_Client_Service_browseNext(client, bNextReq);
    ASSERT_GOOD(bNextResp.responseHeader.serviceResult);

    UA_BrowseNextResponse_clear(&bNextResp);

    bNextResp = UA_Client_Service_browseNext(client, bNextReq);
    ASSERT_GOOD(bNextResp.responseHeader.serviceResult);

    UA_BrowseNextResponse_clear(&bNextResp);

    // release continuation point. Result is then empty
    bNextReq.releaseContinuationPoints = UA_TRUE;
    bNextResp = UA_Client_Service_browseNext(client, bNextReq);
    UA_BrowseNextResponse_clear(&bNextResp);
    ASSERT_GOOD(bNextResp.responseHeader.serviceResult);

    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
    // already deleted by browse request
    bNextReq.continuationPoints = NULL;
    bNextReq.continuationPointsSize = 0;
    UA_BrowseNextRequest_clear(&bNextReq);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
registerUnregisterNodesRequest(UA_Client *client) {
    UA_RegisterNodesRequest req;
    UA_RegisterNodesRequest_init(&req);

    req.nodesToRegister = UA_NodeId_new();
    req.nodesToRegister[0] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    req.nodesToRegisterSize = 1;

    UA_RegisterNodesResponse res = UA_Client_Service_registerNodes(client, req);
    ASSERT_GOOD(res.responseHeader.serviceResult);

    UA_UnregisterNodesRequest reqUn;
    UA_UnregisterNodesRequest_init(&reqUn);

    reqUn.nodesToUnregister = UA_NodeId_new();
    reqUn.nodesToUnregister[0] = res.registeredNodeIds[0];
    reqUn.nodesToUnregisterSize = 1;

    UA_UnregisterNodesResponse resUn = UA_Client_Service_unregisterNodes(client, reqUn);
    ASSERT_GOOD(resUn.responseHeader.serviceResult);

    UA_UnregisterNodesRequest_clear(&reqUn);
    UA_UnregisterNodesResponse_clear(&resUn);
    UA_RegisterNodesRequest_clear(&req);
    UA_RegisterNodesResponse_clear(&res);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
translateBrowsePathsToNodeIdsRequest(UA_Client *client) {
    // Just for testing we want to translate the following path to its corresponding node id
    // /Objects/Server/ServerStatus/State
    // Equals the following node IDs:
    // /85/2253/2256/2259

    #define BROWSE_PATHS_SIZE 3
    char *paths[BROWSE_PATHS_SIZE] = {"Server", "ServerStatus", "State"};
    UA_UInt32 ids[BROWSE_PATHS_SIZE] = {UA_NS0ID_ORGANIZES, UA_NS0ID_HASCOMPONENT, UA_NS0ID_HASCOMPONENT};
    UA_BrowsePath browsePath;
    UA_BrowsePath_init(&browsePath);
    browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browsePath.relativePath.elements = (UA_RelativePathElement*)UA_Array_new(BROWSE_PATHS_SIZE, &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
    browsePath.relativePath.elementsSize = BROWSE_PATHS_SIZE;

    for(size_t i = 0; i < BROWSE_PATHS_SIZE; i++) {
        UA_RelativePathElement *elem = &browsePath.relativePath.elements[i];
        elem->referenceTypeId = UA_NODEID_NUMERIC(0, ids[i]);
        elem->targetName = UA_QUALIFIEDNAME_ALLOC(0, paths[i]);
    }

    UA_TranslateBrowsePathsToNodeIdsRequest request;
    UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
    request.browsePaths = &browsePath;
    request.browsePathsSize = 1;

    UA_TranslateBrowsePathsToNodeIdsResponse response = UA_Client_Service_translateBrowsePathsToNodeIds(client, request);
    ASSERT_GOOD(response.responseHeader.serviceResult);

    UA_BrowsePath_clear(&browsePath);
    UA_TranslateBrowsePathsToNodeIdsResponse_clear(&response);

    return UA_STATUSCODE_GOOD;
}


static void
monitoredItemHandler(UA_Client *client, UA_UInt32 subId, void *subContext,
                     UA_UInt32 monId, void *monContext, UA_DataValue *value) {
}

static UA_StatusCode
subscriptionRequests(UA_Client *client) {
    UA_UInt32 subId;
    // createSubscriptionRequest
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);

    ASSERT_GOOD(response.responseHeader.serviceResult);
    subId = response.subscriptionId;

    // modifySubscription
    UA_ModifySubscriptionRequest modifySubscriptionRequest;
    UA_ModifySubscriptionRequest_init(&modifySubscriptionRequest);
    modifySubscriptionRequest.subscriptionId = subId;
    modifySubscriptionRequest.maxNotificationsPerPublish = request.maxNotificationsPerPublish;
    modifySubscriptionRequest.priority = request.priority;
    modifySubscriptionRequest.requestedLifetimeCount = request.requestedLifetimeCount;
    modifySubscriptionRequest.requestedMaxKeepAliveCount = request.requestedMaxKeepAliveCount;
    modifySubscriptionRequest.requestedPublishingInterval = request.requestedPublishingInterval;
    UA_ModifySubscriptionResponse modifySubscriptionResponse;
    __UA_Client_Service(client, &modifySubscriptionRequest, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST],
                        &modifySubscriptionResponse, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE]);
    ASSERT_GOOD(modifySubscriptionResponse.responseHeader.serviceResult);
    UA_ModifySubscriptionRequest_clear(&modifySubscriptionRequest);
    UA_ModifySubscriptionResponse_clear(&modifySubscriptionResponse);

    // setPublishingMode
    UA_SetPublishingModeRequest setPublishingModeRequest;
    UA_SetPublishingModeRequest_init(&setPublishingModeRequest);
    setPublishingModeRequest.subscriptionIdsSize = 1;
    setPublishingModeRequest.subscriptionIds = UA_UInt32_new();
    setPublishingModeRequest.subscriptionIds[0] = subId;
    setPublishingModeRequest.publishingEnabled = UA_TRUE;
    UA_SetPublishingModeResponse setPublishingModeResponse;
    __UA_Client_Service(client, &setPublishingModeRequest, &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST],
                        &setPublishingModeResponse, &UA_TYPES[UA_TYPES_SETPUBLISHINGMODERESPONSE]);
    ASSERT_GOOD(setPublishingModeResponse.responseHeader.serviceResult);
    UA_SetPublishingModeRequest_clear(&setPublishingModeRequest);
    UA_SetPublishingModeResponse_clear(&setPublishingModeResponse);


    // createMonitoredItemsRequest
    UA_UInt32 monId;
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE));

    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, NULL, monitoredItemHandler, NULL);

    ASSERT_GOOD(monResponse.statusCode);
    monId = monResponse.monitoredItemId;

    // publishRequest
    UA_PublishRequest publishRequest;
    UA_PublishRequest_init(&publishRequest);
    ASSERT_GOOD(UA_Client_preparePublishRequest(client, &publishRequest));
    __UA_Client_AsyncService(client, &publishRequest,
                             &UA_TYPES[UA_TYPES_PUBLISHREQUEST], NULL,
                             &UA_TYPES[UA_TYPES_PUBLISHRESPONSE], NULL, NULL);
    // here we don't care about the return value since it may be UA_STATUSCODE_BADMESSAGENOTAVAILABLE
    // ASSERT_GOOD(publishResponse.responseHeader.serviceResult);
    UA_PublishRequest_clear(&publishRequest);

    // republishRequest
    UA_RepublishRequest republishRequest;
    UA_RepublishRequest_init(&republishRequest);
    republishRequest.retransmitSequenceNumber = 0;
    republishRequest.subscriptionId = subId;
    UA_RepublishResponse republishResponse;
    __UA_Client_Service(client, &republishRequest, &UA_TYPES[UA_TYPES_REPUBLISHREQUEST],
                        &republishResponse, &UA_TYPES[UA_TYPES_REPUBLISHRESPONSE]);
    // here we don't care about the return value since it may be UA_STATUSCODE_BADMESSAGENOTAVAILABLE
    // ASSERT_GOOD(republishResponse.responseHeader.serviceResult);
    UA_RepublishRequest_clear(&republishRequest);
    UA_RepublishResponse_clear(&republishResponse);

    // modifyMonitoredItems
    UA_ModifyMonitoredItemsRequest modifyMonitoredItemsRequest;
    UA_ModifyMonitoredItemsRequest_init(&modifyMonitoredItemsRequest);
    modifyMonitoredItemsRequest.subscriptionId = subId;
    modifyMonitoredItemsRequest.itemsToModifySize = 1;
    modifyMonitoredItemsRequest.itemsToModify = UA_MonitoredItemModifyRequest_new();
    modifyMonitoredItemsRequest.itemsToModify[0].monitoredItemId = monId;
    UA_MonitoringParameters_init(&modifyMonitoredItemsRequest.itemsToModify[0].requestedParameters);
    UA_ModifyMonitoredItemsResponse modifyMonitoredItemsResponse;
    __UA_Client_Service(client, &modifyMonitoredItemsRequest, &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST],
                        &modifyMonitoredItemsResponse, &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSRESPONSE]);
    ASSERT_GOOD(modifyMonitoredItemsResponse.responseHeader.serviceResult);
    UA_ModifyMonitoredItemsRequest_clear(&modifyMonitoredItemsRequest);
    UA_ModifyMonitoredItemsResponse_clear(&modifyMonitoredItemsResponse);

    // setMonitoringMode
    UA_SetMonitoringModeRequest setMonitoringModeRequest;
    UA_SetMonitoringModeRequest_init(&setMonitoringModeRequest);
    setMonitoringModeRequest.subscriptionId = subId;
    setMonitoringModeRequest.monitoredItemIdsSize = 1;
    setMonitoringModeRequest.monitoredItemIds = UA_UInt32_new();
    setMonitoringModeRequest.monitoredItemIds[0] = monId;
    setMonitoringModeRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_SetMonitoringModeResponse setMonitoringModeResponse;
    __UA_Client_Service(client, &setMonitoringModeRequest, &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST],
                        &setMonitoringModeResponse, &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE]);
    ASSERT_GOOD(setMonitoringModeResponse.responseHeader.serviceResult);
    UA_SetMonitoringModeRequest_clear(&setMonitoringModeRequest);
    UA_SetMonitoringModeResponse_clear(&setMonitoringModeResponse);

    // deleteMonitoredItemsRequest
    ASSERT_GOOD(UA_Client_MonitoredItems_deleteSingle(client, subId, monId));


    // deleteSubscriptionRequest
    ASSERT_GOOD(UA_Client_Subscriptions_deleteSingle(client, subId));

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
callRequest(UA_Client *client) {
    /* Set up the request */
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    UA_CallMethodRequest item;
    UA_CallMethodRequest_init(&item);
    item.methodId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_GETMONITOREDITEMS);
    item.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    UA_Variant input;
    UA_UInt32 subId = 12345;
    UA_Variant_init(&input);
    UA_Variant_setScalarCopy(&input, &subId, &UA_TYPES[UA_TYPES_UINT32]);
    item.inputArgumentsSize = 1;
    item.inputArguments = &input;

    request.methodsToCall = &item;
    request.methodsToCallSize = 1;

    /* Call the service */
    UA_CallResponse response = UA_Client_Service_call(client, request);
    ASSERT_GOOD(response.responseHeader.serviceResult);

    UA_CallResponse_clear(&response);
    UA_Variant_clear(&input);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
nodemanagementRequests(UA_Client *client) {
    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "Some Coordinates");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Coordinates");

    UA_NodeId newObjectId;
    ASSERT_GOOD(UA_Client_addObjectNode(client, UA_NODEID_NULL,
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                     UA_QUALIFIEDNAME(1, "Coordinates"),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), attr, &newObjectId));

    UA_ExpandedNodeId target = UA_EXPANDEDNODEID_NULL;
    target.nodeId = newObjectId;
    ASSERT_GOOD(UA_Client_addReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_TRUE, UA_STRING_NULL, target, UA_NODECLASS_OBJECT));

    ASSERT_GOOD(UA_Client_deleteReference(client, UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER),
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                          true, target, true));

    ASSERT_GOOD(UA_Client_deleteNode(client, newObjectId, UA_TRUE));

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
executeClientServices(UA_Client *client) {
    ASSERT_GOOD(findServersRequest(client));
    ASSERT_GOOD(findServersOnNetworkRequest(client));
    ASSERT_GOOD(registerServerRequest(client));
    ASSERT_GOOD(registerServer2Request(client));
    ASSERT_GOOD(readValueRequest(client));
    ASSERT_GOOD(writeValueRequest(client));
    ASSERT_GOOD(browseAndBrowseNextRequest(client));
    ASSERT_GOOD(registerUnregisterNodesRequest(client));
    ASSERT_GOOD(translateBrowsePathsToNodeIdsRequest(client));
    ASSERT_GOOD(subscriptionRequests(client));
    ASSERT_GOOD(callRequest(client));
    ASSERT_GOOD(nodemanagementRequests(client));

    return UA_STATUSCODE_GOOD;
}

int main(void) {
    emptyCorpusDir();
    start_server();

    UA_Client *client = UA_Client_newForUnitTest();

    // this will also call getEndpointsRequest
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://localhost:4840");
    if(retval == UA_STATUSCODE_GOOD)
        retval = executeClientServices(client);
    UA_Client_disconnect(client);
    UA_Client_delete(client);

    if(retval == UA_STATUSCODE_GOOD) {
        // now also connect with user/pass so that fuzzer also knows how to do that
        client = UA_Client_newForUnitTest();
        retval = UA_Client_connectUsername(client, "opc.tcp://localhost:4840", "user1", "password");
        retval = retval == UA_STATUSCODE_BADUSERACCESSDENIED ? UA_STATUSCODE_GOOD : retval;
        UA_Client_disconnect(client);
        UA_Client_delete(client);
    }

    teardown_server();

    if(retval != UA_STATUSCODE_GOOD) {
        printf("\n--------- AN ERROR OCCURRED ----------\nStatus = %s\n", UA_StatusCode_name(retval));
        exit(1);
    } else {
        printf("\n--------- SUCCESS -------\nThe corpus is stored in %s", UA_CORPUS_OUTPUT_DIR);
    }
    return 0;
}
