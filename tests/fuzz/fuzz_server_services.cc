/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "ua_server_internal.h"
#include "ua_services.h"

typedef enum {
    SERVICE_FINDSERVERS = 0,
    SERVICE_GETENDPOINTS,
    SERVICE_REGISTERSERVER,
    SERVICE_REGISTERSERVER2,
    SERVICE_FINDSERVERSONNETWORK,
    SERVICE_CREATESESSION,
    SERVICE_ACTIVATESESSION,
    SERVICE_CLOSESESSION,
    SERVICE_CANCEL,
    SERVICE_CREATESUBSCRIPTION,
    SERVICE_MODIFYSUBSCRIPTION,
    SERVICE_SETPUBLISHINGMODE,
    SERVICE_DELETESUBSCRIPTIONS,
    SERVICE_CREATEMONITOREDITEMS,
    SERVICE_MODIFYMONITOREDITEMS,
    SERVICE_SETMONITORINGMODE,
    SERVICE_DELETEMONITOREDITEMS,
    SERVICE_COUNT
} ServiceType;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if(size < 2)
        return 0;

    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode retval = UA_ServerConfig_setDefault(&config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_clean(&config);
        return 0;
    }

    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server) {
        UA_ServerConfig_clean(&config);
        return 0;
    }

    UA_SecureChannel channel;
    UA_SecureChannel_init(&channel);
    channel.state = UA_SECURECHANNELSTATE_OPEN;

    UA_Session session;
    UA_Session_init(&session);
    session.activated = true;
    UA_NodeId_init(&session.sessionId);
    session.sessionId.identifierType = UA_NODEIDTYPE_NUMERIC;
    session.sessionId.identifier.numeric = 1;

    // Use the first byte to decide which service to call
    uint8_t serviceChoice = data[0] % SERVICE_COUNT;
    data++;
    size--;

    UA_ByteString msg = {size, (UA_Byte *) (void *) data};

    switch((ServiceType)serviceChoice) {
        case SERVICE_FINDSERVERS: {
            UA_FindServersRequest request;
            UA_FindServersResponse response;
            UA_FindServersResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_FindServers(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_FindServersRequest_clear(&request);
            }
            UA_FindServersResponse_clear(&response);
            break;
        }
        case SERVICE_GETENDPOINTS: {
            UA_GetEndpointsRequest request;
            UA_GetEndpointsResponse response;
            UA_GetEndpointsResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_GetEndpoints(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_GetEndpointsRequest_clear(&request);
            }
            UA_GetEndpointsResponse_clear(&response);
            break;
        }
#ifdef UA_ENABLE_DISCOVERY
        case SERVICE_REGISTERSERVER: {
            UA_RegisterServerRequest request;
            UA_RegisterServerResponse response;
            UA_RegisterServerResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_RegisterServer(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_RegisterServerRequest_clear(&request);
            }
            UA_RegisterServerResponse_clear(&response);
            break;
        }
        case SERVICE_REGISTERSERVER2: {
            UA_RegisterServer2Request request;
            UA_RegisterServer2Response response;
            UA_RegisterServer2Response_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_RegisterServer2(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_RegisterServer2Request_clear(&request);
            }
            UA_RegisterServer2Response_clear(&response);
            break;
        }
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
        case SERVICE_FINDSERVERSONNETWORK: {
            UA_FindServersOnNetworkRequest request;
            UA_FindServersOnNetworkResponse response;
            UA_FindServersOnNetworkResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_FindServersOnNetwork(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_FindServersOnNetworkRequest_clear(&request);
            }
            UA_FindServersOnNetworkResponse_clear(&response);
            break;
        }
# endif
#endif
        case SERVICE_CREATESESSION: {
            UA_CreateSessionRequest request;
            UA_CreateSessionResponse response;
            UA_CreateSessionResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_CreateSession(server, &channel, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_CreateSessionRequest_clear(&request);
            }
            UA_CreateSessionResponse_clear(&response);
            break;
        }
        case SERVICE_ACTIVATESESSION: {
            UA_ActivateSessionRequest request;
            UA_ActivateSessionResponse response;
            UA_ActivateSessionResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_ActivateSession(server, &channel, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_ActivateSessionRequest_clear(&request);
            }
            UA_ActivateSessionResponse_clear(&response);
            break;
        }
        case SERVICE_CLOSESESSION: {
            UA_CloseSessionRequest request;
            UA_CloseSessionResponse response;
            UA_CloseSessionResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_CloseSession(server, &channel, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_CloseSessionRequest_clear(&request);
            }
            UA_CloseSessionResponse_clear(&response);
            break;
        }
        case SERVICE_CANCEL: {
            UA_CancelRequest request;
            UA_CancelResponse response;
            UA_CancelResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_CANCELREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_Cancel(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_CancelRequest_clear(&request);
            }
            UA_CancelResponse_clear(&response);
            break;
        }
#ifdef UA_ENABLE_SUBSCRIPTIONS
        case SERVICE_CREATESUBSCRIPTION: {
            UA_CreateSubscriptionRequest request;
            UA_CreateSubscriptionResponse response;
            UA_CreateSubscriptionResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_CreateSubscription(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_CreateSubscriptionRequest_clear(&request);
            }
            UA_CreateSubscriptionResponse_clear(&response);
            break;
        }
        case SERVICE_MODIFYSUBSCRIPTION: {
            UA_ModifySubscriptionRequest request;
            UA_ModifySubscriptionResponse response;
            UA_ModifySubscriptionResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_ModifySubscription(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_ModifySubscriptionRequest_clear(&request);
            }
            UA_ModifySubscriptionResponse_clear(&response);
            break;
        }
        case SERVICE_SETPUBLISHINGMODE: {
            UA_SetPublishingModeRequest request;
            UA_SetPublishingModeResponse response;
            UA_SetPublishingModeResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_SetPublishingMode(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_SetPublishingModeRequest_clear(&request);
            }
            UA_SetPublishingModeResponse_clear(&response);
            break;
        }
        case SERVICE_DELETESUBSCRIPTIONS: {
            UA_DeleteSubscriptionsRequest request;
            UA_DeleteSubscriptionsResponse response;
            UA_DeleteSubscriptionsResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_DeleteSubscriptions(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_DeleteSubscriptionsRequest_clear(&request);
            }
            UA_DeleteSubscriptionsResponse_clear(&response);
            break;
        }
        case SERVICE_CREATEMONITOREDITEMS: {
            UA_CreateMonitoredItemsRequest request;
            UA_CreateMonitoredItemsResponse response;
            UA_CreateMonitoredItemsResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_CreateMonitoredItems(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_CreateMonitoredItemsRequest_clear(&request);
            }
            UA_CreateMonitoredItemsResponse_clear(&response);
            break;
        }
        case SERVICE_MODIFYMONITOREDITEMS: {
            UA_ModifyMonitoredItemsRequest request;
            UA_ModifyMonitoredItemsResponse response;
            UA_ModifyMonitoredItemsResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_ModifyMonitoredItems(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_ModifyMonitoredItemsRequest_clear(&request);
            }
            UA_ModifyMonitoredItemsResponse_clear(&response);
            break;
        }
        case SERVICE_SETMONITORINGMODE: {
            UA_SetMonitoringModeRequest request;
            UA_SetMonitoringModeResponse response;
            UA_SetMonitoringModeResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_SetMonitoringMode(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_SetMonitoringModeRequest_clear(&request);
            }
            UA_SetMonitoringModeResponse_clear(&response);
            break;
        }
        case SERVICE_DELETEMONITOREDITEMS: {
            UA_DeleteMonitoredItemsRequest request;
            UA_DeleteMonitoredItemsResponse response;
            UA_DeleteMonitoredItemsResponse_init(&response);
            if(UA_decodeBinary(&msg, &request, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST], NULL) == UA_STATUSCODE_GOOD) {
                UA_LOCK(&server->serviceMutex);
                Service_DeleteMonitoredItems(server, &session, &request, &response);
                UA_UNLOCK(&server->serviceMutex);
                UA_DeleteMonitoredItemsRequest_clear(&request);
            }
            UA_DeleteMonitoredItemsResponse_clear(&response);
            break;
        }
#endif
        default:
            break;
    }

    UA_LOCK(&server->serviceMutex);
    UA_SecureChannel_clear(&channel);
    UA_Session_clear(&session, server);
    UA_UNLOCK(&server->serviceMutex);
    UA_Server_delete(server);
    return 0;
}
