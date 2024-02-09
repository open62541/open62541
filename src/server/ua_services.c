/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2014-2024 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2014-2016 (c) Sten Grüner
 *    Copyright 2014-2015, 2017 (c) Florian Palm
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015-2016 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) Joakim L. Gilje
 *    Copyright 2016-2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) TorbenD
 *    Copyright 2017 (c) frax2222
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2019 (c) Kalycito Infotech Private Limited
 *    Copyright 2023 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Phuong Nguyen)
 */

/* This file contains the service invocation logic that is called from all
 * communication backends */

#include "ua_server_internal.h"

/* The counterOffset is the offset of the UA_ServiceCounterDataType for the
 * service in the UA_ SessionDiagnosticsDataType. */
#ifdef UA_ENABLE_DIAGNOSTICS
#define UA_SERVICECOUNTER_OFFSET(X)                             \
    *counterOffset = offsetof(UA_SessionDiagnosticsDataType, X)
#else
#define UA_SERVICECOUNTER_OFFSET(X)
#endif

void
getServicePointers(UA_UInt32 requestTypeId, const UA_DataType **requestType,
                   const UA_DataType **responseType, UA_Service *service,
                   UA_Boolean *requiresSession, size_t *counterOffset) {
    switch(requestTypeId) {
    case UA_NS0ID_GETENDPOINTSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_GetEndpoints;
        *requestType = &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_FINDSERVERSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_FindServers;
        *requestType = &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE];
        *requiresSession = false;
        break;
#ifdef UA_ENABLE_DISCOVERY
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    case UA_NS0ID_FINDSERVERSONNETWORKREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_FindServersOnNetwork;
        *requestType = &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKRESPONSE];
        *requiresSession = false;
        break;
# endif
    case UA_NS0ID_REGISTERSERVERREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_RegisterServer;
        *requestType = &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_REGISTERSERVER2REQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_RegisterServer2;
        *requestType = &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REGISTERSERVER2RESPONSE];
        *requiresSession = false;
        break;
#endif
    case UA_NS0ID_CREATESESSIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CreateSession;
        *requestType = &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_ACTIVATESESSIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_ActivateSession;
        *requestType = &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE];
        *requiresSession = false;
        break;
    case UA_NS0ID_CLOSESESSIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CloseSession;
        *requestType = &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE];
        break;
    case UA_NS0ID_CANCELREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Cancel;
        *requestType = &UA_TYPES[UA_TYPES_CANCELREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CANCELRESPONSE];
        break;
    case UA_NS0ID_READREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Read;
        *requestType = &UA_TYPES[UA_TYPES_READREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_READRESPONSE];
        UA_SERVICECOUNTER_OFFSET(readCount);
        break;
    case UA_NS0ID_WRITEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Write;
        *requestType = &UA_TYPES[UA_TYPES_WRITEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_WRITERESPONSE];
        UA_SERVICECOUNTER_OFFSET(writeCount);
        break;
    case UA_NS0ID_BROWSEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Browse;
        *requestType = &UA_TYPES[UA_TYPES_BROWSEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_BROWSERESPONSE];
        UA_SERVICECOUNTER_OFFSET(browseCount);
        break;
    case UA_NS0ID_BROWSENEXTREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_BrowseNext;
        *requestType = &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE];
        UA_SERVICECOUNTER_OFFSET(browseNextCount);
        break;
    case UA_NS0ID_REGISTERNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_RegisterNodes;
        *requestType = &UA_TYPES[UA_TYPES_REGISTERNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REGISTERNODESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(registerNodesCount);
        break;
    case UA_NS0ID_UNREGISTERNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_UnregisterNodes;
        *requestType = &UA_TYPES[UA_TYPES_UNREGISTERNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_UNREGISTERNODESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(unregisterNodesCount);
        break;
    case UA_NS0ID_TRANSLATEBROWSEPATHSTONODEIDSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_TranslateBrowsePathsToNodeIds;
        *requestType = &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(translateBrowsePathsToNodeIdsCount);
        break;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    case UA_NS0ID_CREATESUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CreateSubscription;
        *requestType = &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE];
        UA_SERVICECOUNTER_OFFSET(createSubscriptionCount);
        break;
    case UA_NS0ID_PUBLISHREQUEST_ENCODING_DEFAULTBINARY:
        *requestType = &UA_TYPES[UA_TYPES_PUBLISHREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_PUBLISHRESPONSE];
        UA_SERVICECOUNTER_OFFSET(publishCount);
        break;
    case UA_NS0ID_REPUBLISHREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Republish;
        *requestType = &UA_TYPES[UA_TYPES_REPUBLISHREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_REPUBLISHRESPONSE];
        UA_SERVICECOUNTER_OFFSET(republishCount);
        break;
    case UA_NS0ID_MODIFYSUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_ModifySubscription;
        *requestType = &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE];
        UA_SERVICECOUNTER_OFFSET(modifySubscriptionCount);
        break;
    case UA_NS0ID_SETPUBLISHINGMODEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_SetPublishingMode;
        *requestType = &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETPUBLISHINGMODERESPONSE];
        UA_SERVICECOUNTER_OFFSET(setPublishingModeCount);
        break;
    case UA_NS0ID_DELETESUBSCRIPTIONSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteSubscriptions;
        *requestType = &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(deleteSubscriptionsCount);
        break;
    case UA_NS0ID_TRANSFERSUBSCRIPTIONSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_TransferSubscriptions;
        *requestType = &UA_TYPES[UA_TYPES_TRANSFERSUBSCRIPTIONSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_TRANSFERSUBSCRIPTIONSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(transferSubscriptionsCount);
        break;
    case UA_NS0ID_CREATEMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_CreateMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(createMonitoredItemsCount);
        break;
    case UA_NS0ID_DELETEMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(deleteMonitoredItemsCount);
        break;
    case UA_NS0ID_MODIFYMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_ModifyMonitoredItems;
        *requestType = &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSRESPONSE];
        UA_SERVICECOUNTER_OFFSET(modifyMonitoredItemsCount);
        break;
    case UA_NS0ID_SETMONITORINGMODEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_SetMonitoringMode;
        *requestType = &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE];
        UA_SERVICECOUNTER_OFFSET(setMonitoringModeCount);
        break;
    case UA_NS0ID_SETTRIGGERINGREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_SetTriggering;
        *requestType = &UA_TYPES[UA_TYPES_SETTRIGGERINGREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_SETTRIGGERINGRESPONSE];
        UA_SERVICECOUNTER_OFFSET(setTriggeringCount);
        break;
#endif
#ifdef UA_ENABLE_HISTORIZING
        /* For History read */
    case UA_NS0ID_HISTORYREADREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_HistoryRead;
        *requestType = &UA_TYPES[UA_TYPES_HISTORYREADREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_HISTORYREADRESPONSE];
        UA_SERVICECOUNTER_OFFSET(historyReadCount);
        break;
        /* For History update */
    case UA_NS0ID_HISTORYUPDATEREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_HistoryUpdate;
        *requestType = &UA_TYPES[UA_TYPES_HISTORYUPDATEREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_HISTORYUPDATERESPONSE];
        UA_SERVICECOUNTER_OFFSET(historyUpdateCount);
        break;
#endif

#ifdef UA_ENABLE_METHODCALLS
    case UA_NS0ID_CALLREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_Call;
        *requestType = &UA_TYPES[UA_TYPES_CALLREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_CALLRESPONSE];
        UA_SERVICECOUNTER_OFFSET(callCount);
        break;
#endif

#ifdef UA_ENABLE_NODEMANAGEMENT
    case UA_NS0ID_ADDNODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_AddNodes;
        *requestType = &UA_TYPES[UA_TYPES_ADDNODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ADDNODESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(addNodesCount);
        break;
    case UA_NS0ID_ADDREFERENCESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_AddReferences;
        *requestType = &UA_TYPES[UA_TYPES_ADDREFERENCESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_ADDREFERENCESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(addReferencesCount);
        break;
    case UA_NS0ID_DELETENODESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteNodes;
        *requestType = &UA_TYPES[UA_TYPES_DELETENODESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETENODESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(deleteNodesCount);
        break;
    case UA_NS0ID_DELETEREFERENCESREQUEST_ENCODING_DEFAULTBINARY:
        *service = (UA_Service)Service_DeleteReferences;
        *requestType = &UA_TYPES[UA_TYPES_DELETEREFERENCESREQUEST];
        *responseType = &UA_TYPES[UA_TYPES_DELETEREFERENCESRESPONSE];
        UA_SERVICECOUNTER_OFFSET(deleteReferencesCount);
        break;
#endif

    default:
        break;
    }
}

static const UA_String securityPolicyNone =
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#None");

/* Returns a status of the SecureChannel. The detailed service status (usually
 * part of the response) is set in the serviceResult argument. */
UA_StatusCode
processService(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
               UA_UInt32 requestId, UA_Service service, const UA_Request *request,
               const UA_DataType *requestType, UA_Response *response,
               const UA_DataType *responseType, UA_Boolean sessionRequired,
               size_t counterOffset) {
    UA_Session anonymousSession;
    UA_StatusCode channelRes = UA_STATUSCODE_GOOD;
    UA_ResponseHeader *rh = &response->responseHeader;

    /* If it is an unencrypted (#None) channel, only allow the discovery services */
    if(server->config.securityPolicyNoneDiscoveryOnly &&
       UA_String_equal(&channel->securityPolicy->policyUri, &securityPolicyNone ) &&
       requestType != &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST] &&
       requestType != &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]
#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)
       && requestType != &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST]
#endif
       ) {
        rh->serviceResult = UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
        goto send_response;
    }

    /* Session lifecycle services. */
    if(requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST] ||
       requestType == &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST] ||
       requestType == &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST]) {
        ((UA_ChannelService)service)(server, channel, request, response);
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        /* Store the authentication token so we can help fuzzing by setting
         * these values in the next request automatically */
        if(requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]) {
            UA_CreateSessionResponse *res = &response->createSessionResponse;
            UA_NodeId_copy(&res->authenticationToken, &unsafe_fuzz_authenticationToken);
        }
#endif
        goto send_response;
    }

    /* Set an anonymous, inactive session for services that need no session */
    if(!session) {
        if(sessionRequired) {
#ifdef UA_ENABLE_TYPEDESCRIPTION
            UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                                   "%s refused without a valid session",
                                   requestType->typeName);
#else
            UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                                   "Service %" PRIu32 " refused without a valid session",
                                   requestType->binaryEncodingId.identifier.numeric);
#endif
            rh->serviceResult = UA_STATUSCODE_BADSESSIONIDINVALID;
            goto send_response;
        }

        UA_Session_init(&anonymousSession);
        anonymousSession.sessionId = UA_NODEID_GUID(0, UA_GUID_NULL);
        anonymousSession.channel = channel;
        session = &anonymousSession;
    }

    UA_assert(session != NULL);

    /* Trying to use a non-activated session? */
    if(sessionRequired && !session->activated) {
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "%s refused on a non-activated session",
                               requestType->typeName);
#else
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "Service %" PRIu32 " refused on a non-activated session",
                               requestType->binaryEncodingId.identifier.numeric);
#endif
        if(session != &anonymousSession) {
            UA_Server_removeSessionByToken(server, &session->authenticationToken,
                                           UA_SHUTDOWNREASON_ABORT);
        }
        rh->serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
        goto send_response;
    }

    /* Update the session lifetime */
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    UA_DateTime now = el->dateTime_now(el);
    UA_Session_updateLifetime(session, now, nowMonotonic);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The publish request is not answered immediately */
    if(requestType == &UA_TYPES[UA_TYPES_PUBLISHREQUEST]) {
        rh->serviceResult =
            Service_Publish(server, session, &request->publishRequest, requestId);

        /* Don't send a response */
        goto update_statistics;
    }
#endif

#if UA_MULTITHREADING >= 100 && defined(UA_ENABLE_METHODCALLS)
    /* The call request might not be answered immediately */
    if(requestType == &UA_TYPES[UA_TYPES_CALLREQUEST]) {
        UA_Boolean finished = true;
        Service_CallAsync(server, session, requestId, &request->callRequest,
                          &response->callResponse, &finished);

        /* Async method calls remain. Don't send a response now. In case we have
         * an async call, count as a "good" request for the diagnostics
         * statistic. */
        if(UA_LIKELY(finished))
            goto send_response;
        goto update_statistics;
    }
#endif

    /* Execute the synchronous service call */
    service(server, session, request, response);

    /* Upon success, send the response. Otherwise a ServiceFault. */
 send_response:
    channelRes = sendResponse(server, session, channel,
                              requestId, response, responseType);
    goto update_statistics; /* pacify warnings if no other goto to
                             * update_statistics is enabled */

    /* Update the diagnostics statistics */
 update_statistics:
#ifdef UA_ENABLE_DIAGNOSTICS
    if(session && session != &server->adminSession) {
        session->diagnostics.totalRequestCount.totalCount++;
        if(rh->serviceResult != UA_STATUSCODE_GOOD)
            session->diagnostics.totalRequestCount.errorCount++;
        if(counterOffset != 0) {
            UA_ServiceCounterDataType *serviceCounter = (UA_ServiceCounterDataType*)
                (((uintptr_t)&session->diagnostics) + counterOffset);
            serviceCounter->totalCount++;
            if(rh->serviceResult != UA_STATUSCODE_GOOD)
                serviceCounter->errorCount++;
        }
    }
#endif

    return channelRes;
}
