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
#include "ua_services.h"

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
/* store the authentication token and session ID so we can help fuzzing by
 * setting these values in the next request automatically */
UA_NodeId unsafe_fuzz_authenticationToken = {0, UA_NODEIDTYPE_NUMERIC, {0}};
#endif

/* The counterOffset is the offset of the UA_ServiceCounterDataType for the
 * service in the UA_ SessionDiagnosticsDataType. */
#ifdef UA_ENABLE_DIAGNOSTICS
# define UA_SERVICECOUNTER_OFFSET_NONE(requiresSession) 0, requiresSession
# define UA_SERVICECOUNTER_OFFSET(X, requiresSession) \
    offsetof(UA_SessionDiagnosticsDataType, X), requiresSession
#else
# define UA_SERVICECOUNTER_OFFSET_NONE(requiresSession) requiresSession
# define UA_SERVICECOUNTER_OFFSET(X, requiresSession) requiresSession
#endif

UA_ServiceDescription serviceDescriptions[] = {
    {UA_NS0ID_GETENDPOINTSREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET_NONE(false), (UA_Service)Service_GetEndpoints,
     &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST], &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]},
    {UA_NS0ID_FINDSERVERSREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET_NONE(false), (UA_Service)Service_FindServers,
     &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST], &UA_TYPES[UA_TYPES_FINDSERVERSRESPONSE]},
#ifdef UA_ENABLE_DISCOVERY
    {UA_NS0ID_REGISTERSERVERREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET_NONE(false), (UA_Service)Service_RegisterServer,
     &UA_TYPES[UA_TYPES_REGISTERSERVERREQUEST], &UA_TYPES[UA_TYPES_REGISTERSERVERRESPONSE]},
    {UA_NS0ID_REGISTERSERVER2REQUEST_ENCODING_DEFAULTBINARY,
    UA_SERVICECOUNTER_OFFSET_NONE(false), (UA_Service)Service_RegisterServer2,
    &UA_TYPES[UA_TYPES_REGISTERSERVER2REQUEST], &UA_TYPES[UA_TYPES_REGISTERSERVER2RESPONSE]},
# ifdef UA_ENABLE_DISCOVERY_MULTICAST
    {UA_NS0ID_FINDSERVERSONNETWORKREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET_NONE(false), (UA_Service)Service_FindServersOnNetwork,
     &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST], &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKRESPONSE]},
# endif
#endif
    {UA_NS0ID_CREATESESSIONREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET_NONE(false), (UA_Service)Service_CreateSession,
     &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST], &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]},
    {UA_NS0ID_ACTIVATESESSIONREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET_NONE(false), (UA_Service)Service_ActivateSession,
     &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],  &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]},
    {UA_NS0ID_CLOSESESSIONREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET_NONE(true), (UA_Service)Service_CloseSession,
     &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST], &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]},
    {UA_NS0ID_CANCELREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET_NONE(true), (UA_Service)Service_Cancel,
     &UA_TYPES[UA_TYPES_CANCELREQUEST], &UA_TYPES[UA_TYPES_CANCELRESPONSE]},
    {UA_NS0ID_READREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(readCount, true), (UA_Service)Service_Read,
     &UA_TYPES[UA_TYPES_READREQUEST], &UA_TYPES[UA_TYPES_READRESPONSE]},
    {UA_NS0ID_WRITEREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(writeCount, true), (UA_Service)Service_Write,
     &UA_TYPES[UA_TYPES_WRITEREQUEST], &UA_TYPES[UA_TYPES_WRITERESPONSE]},
    {UA_NS0ID_BROWSEREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(browseCount, true), (UA_Service)Service_Browse,
     &UA_TYPES[UA_TYPES_BROWSEREQUEST], &UA_TYPES[UA_TYPES_BROWSERESPONSE]},
    {UA_NS0ID_BROWSENEXTREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(browseNextCount, true), (UA_Service)Service_BrowseNext,
     &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST], &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE]},
    {UA_NS0ID_REGISTERNODESREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(registerNodesCount, true), (UA_Service)Service_RegisterNodes,
     &UA_TYPES[UA_TYPES_REGISTERNODESREQUEST], &UA_TYPES[UA_TYPES_REGISTERNODESRESPONSE]},
    {UA_NS0ID_UNREGISTERNODESREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(unregisterNodesCount, true), (UA_Service)Service_UnregisterNodes,
     &UA_TYPES[UA_TYPES_UNREGISTERNODESREQUEST], &UA_TYPES[UA_TYPES_UNREGISTERNODESRESPONSE]},
    {UA_NS0ID_TRANSLATEBROWSEPATHSTONODEIDSREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(translateBrowsePathsToNodeIdsCount, true), (UA_Service)Service_TranslateBrowsePathsToNodeIds,
     &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST], &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE]},
#ifdef UA_ENABLE_SUBSCRIPTIONS
    {UA_NS0ID_CREATESUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(createSubscriptionCount, true), (UA_Service)Service_CreateSubscription,
     &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST], &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE]},
    {UA_NS0ID_PUBLISHREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(publishCount, true), NULL,
     &UA_TYPES[UA_TYPES_PUBLISHREQUEST], &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]},
    {UA_NS0ID_REPUBLISHREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(republishCount, true), (UA_Service)Service_Republish,
     &UA_TYPES[UA_TYPES_REPUBLISHREQUEST], &UA_TYPES[UA_TYPES_REPUBLISHRESPONSE]},
    {UA_NS0ID_MODIFYSUBSCRIPTIONREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(modifySubscriptionCount, true), (UA_Service)Service_ModifySubscription,
     &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST], &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE]},
    {UA_NS0ID_SETPUBLISHINGMODEREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(setPublishingModeCount, true), (UA_Service)Service_SetPublishingMode,
     &UA_TYPES[UA_TYPES_SETPUBLISHINGMODEREQUEST], &UA_TYPES[UA_TYPES_SETPUBLISHINGMODERESPONSE]},
    {UA_NS0ID_DELETESUBSCRIPTIONSREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(deleteSubscriptionsCount, true), (UA_Service)Service_DeleteSubscriptions,
     &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST], &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE]},
    {UA_NS0ID_TRANSFERSUBSCRIPTIONSREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(transferSubscriptionsCount, true), (UA_Service)Service_TransferSubscriptions,
     &UA_TYPES[UA_TYPES_TRANSFERSUBSCRIPTIONSREQUEST], &UA_TYPES[UA_TYPES_TRANSFERSUBSCRIPTIONSRESPONSE]},
    {UA_NS0ID_CREATEMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(createMonitoredItemsCount, true), (UA_Service)Service_CreateMonitoredItems,
     &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST], &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE]},
    {UA_NS0ID_DELETEMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(deleteMonitoredItemsCount, true), (UA_Service)Service_DeleteMonitoredItems,
     &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST], &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE]},
    {UA_NS0ID_MODIFYMONITOREDITEMSREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(modifyMonitoredItemsCount, true), (UA_Service)Service_ModifyMonitoredItems,
     &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSREQUEST], &UA_TYPES[UA_TYPES_MODIFYMONITOREDITEMSRESPONSE]},
    {UA_NS0ID_SETMONITORINGMODEREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(setMonitoringModeCount, true), (UA_Service)Service_SetMonitoringMode,
     &UA_TYPES[UA_TYPES_SETMONITORINGMODEREQUEST], &UA_TYPES[UA_TYPES_SETMONITORINGMODERESPONSE]},
    {UA_NS0ID_SETTRIGGERINGREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(setTriggeringCount, true), (UA_Service)Service_SetTriggering,
     &UA_TYPES[UA_TYPES_SETTRIGGERINGREQUEST], &UA_TYPES[UA_TYPES_SETTRIGGERINGRESPONSE]},
#endif
#ifdef UA_ENABLE_HISTORIZING
    {UA_NS0ID_HISTORYREADREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(historyReadCount, true), (UA_Service)Service_HistoryRead,
     &UA_TYPES[UA_TYPES_HISTORYREADREQUEST], &UA_TYPES[UA_TYPES_HISTORYREADRESPONSE]},
    {UA_NS0ID_HISTORYUPDATEREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(historyUpdateCount, true), (UA_Service)Service_HistoryUpdate,
     &UA_TYPES[UA_TYPES_HISTORYUPDATEREQUEST], &UA_TYPES[UA_TYPES_HISTORYUPDATERESPONSE]},
#endif
#ifdef UA_ENABLE_METHODCALLS
    {UA_NS0ID_CALLREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(callCount, true), (UA_Service)Service_Call,
     &UA_TYPES[UA_TYPES_CALLREQUEST], &UA_TYPES[UA_TYPES_CALLRESPONSE]},
#endif
#ifdef UA_ENABLE_NODEMANAGEMENT
    {UA_NS0ID_ADDNODESREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(addNodesCount, true), (UA_Service)Service_AddNodes,
     &UA_TYPES[UA_TYPES_ADDNODESREQUEST], &UA_TYPES[UA_TYPES_ADDNODESRESPONSE]},
    {UA_NS0ID_ADDREFERENCESREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(addReferencesCount, true), (UA_Service)Service_AddReferences,
     &UA_TYPES[UA_TYPES_ADDREFERENCESREQUEST], &UA_TYPES[UA_TYPES_ADDREFERENCESRESPONSE]},
    {UA_NS0ID_DELETENODESREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(deleteNodesCount, true), (UA_Service)Service_DeleteNodes,
     &UA_TYPES[UA_TYPES_DELETENODESREQUEST], &UA_TYPES[UA_TYPES_DELETENODESRESPONSE]},
    {UA_NS0ID_DELETEREFERENCESREQUEST_ENCODING_DEFAULTBINARY,
     UA_SERVICECOUNTER_OFFSET(deleteReferencesCount, true), (UA_Service)Service_DeleteReferences,
     &UA_TYPES[UA_TYPES_DELETEREFERENCESREQUEST], &UA_TYPES[UA_TYPES_DELETEREFERENCESRESPONSE]},
#endif
    {0, UA_SERVICECOUNTER_OFFSET_NONE(false), NULL, NULL, NULL}
};

UA_ServiceDescription *
getServiceDescription(UA_UInt32 requestTypeId) {
    for(size_t i = 0; serviceDescriptions[i].requestTypeId > 0; i++) {
        if(serviceDescriptions[i].requestTypeId == requestTypeId)
            return &serviceDescriptions[i];
    }
    return NULL;
}

static const UA_String securityPolicyNone =
    UA_STRING_STATIC("http://opcfoundation.org/UA/SecurityPolicy#None");

static UA_Boolean
processServiceInternal(UA_Server *server, UA_SecureChannel *channel, UA_Session *session,
                       UA_UInt32 requestId, UA_ServiceDescription *sd,
                       const UA_Request *request, UA_Response *response) {
    UA_ResponseHeader *rh = &response->responseHeader;

    /* Check timestamp in the request header */
    if(request->requestHeader.timestamp == 0 &&
       server->config.verifyRequestTimestamp <= UA_RULEHANDLING_WARN) {
        UA_LOG_WARNING_CHANNEL(server->config.logging, channel,
                               "The server sends no timestamp in the request header. "
                               "See the 'verifyRequestTimestamp' setting.");
        if(server->config.verifyRequestTimestamp <= UA_RULEHANDLING_ABORT) {
            rh->serviceResult = UA_STATUSCODE_BADINVALIDTIMESTAMP;
            return false;
        }
    }

    /* If it is an unencrypted (#None) channel, only allow the discovery services */
    if(server->config.securityPolicyNoneDiscoveryOnly &&
       UA_String_equal(&channel->securityPolicy->policyUri, &securityPolicyNone ) &&
       sd->requestType != &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST] &&
       sd->requestType != &UA_TYPES[UA_TYPES_FINDSERVERSREQUEST]
#if defined(UA_ENABLE_DISCOVERY) && defined(UA_ENABLE_DISCOVERY_MULTICAST)
       && sd->requestType != &UA_TYPES[UA_TYPES_FINDSERVERSONNETWORKREQUEST]
#endif
       ) {
        rh->serviceResult = UA_STATUSCODE_BADSECURITYPOLICYREJECTED;
        return false;
    }

    /* Session lifecycle services */
    if(sd->requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST] ||
       sd->requestType == &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST] ||
       sd->requestType == &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST]) {
        ((UA_ChannelService)sd->serviceCallback)(server, channel, request, response);
        /* Store the authentication token created during CreateSession to help
         * fuzzing cover more lines */
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        if(sd->requestType == &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST]) {
            UA_CreateSessionResponse *res = &response->createSessionResponse;
            UA_NodeId_clear(&unsafe_fuzz_authenticationToken);
            UA_NodeId_copy(&res->authenticationToken, &unsafe_fuzz_authenticationToken);
        }
#endif
        return false;
    }

    /* Set an anonymous, inactive session for services that need no session */
    UA_Session anonymousSession;
    if(!session) {
        UA_assert(!sd->sessionRequired);
        UA_Session_init(&anonymousSession);
        anonymousSession.sessionId = UA_NODEID_GUID(0, UA_GUID_NULL);
        anonymousSession.channel = channel;
        session = &anonymousSession;
    }

    /* Trying to use a non-activated session? */
    if(sd->sessionRequired && !session->activated) {
        UA_assert(session != &anonymousSession); /* because sd->sessionRequired */
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "%s refused on a non-activated session",
                               sd->requestType->typeName);
#else
        UA_LOG_WARNING_SESSION(server->config.logging, session,
                               "Service %" PRIu32 " refused on a non-activated session",
                               sd->requestType->binaryEncodingId.identifier.numeric);
#endif
        UA_Server_removeSessionByToken(server, &session->authenticationToken,
                                       UA_SHUTDOWNREASON_ABORT);
        rh->serviceResult = UA_STATUSCODE_BADSESSIONNOTACTIVATED;
        return false;
    }

    /* Update the session lifetime */
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime nowMonotonic = el->dateTime_nowMonotonic(el);
    UA_DateTime now = el->dateTime_now(el);
    UA_Session_updateLifetime(session, now, nowMonotonic);

    /* The publish request is not answered immediately */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    if(sd->requestType == &UA_TYPES[UA_TYPES_PUBLISHREQUEST]) {
        rh->serviceResult = Service_Publish(server, session, &request->publishRequest, requestId);
        return (rh->serviceResult == UA_STATUSCODE_GOOD);
    }
#endif

    /* An async call request might not be answered immediately */
#if UA_MULTITHREADING >= 100 && defined(UA_ENABLE_METHODCALLS)
    if(sd->requestType == &UA_TYPES[UA_TYPES_CALLREQUEST]) {
        UA_Boolean finished = true;
        Service_CallAsync(server, session, requestId, &request->callRequest,
                          &response->callResponse, &finished);
        return !finished;
    }
#endif

    /* Execute the synchronous service call */
    sd->serviceCallback(server, session, request, response);
    return false;
}

UA_Boolean
UA_Server_processRequest(UA_Server *server, UA_SecureChannel *channel,
                         UA_UInt32 requestId, UA_ServiceDescription *sd,
                         const UA_Request *request, UA_Response *response) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Set the authenticationToken from the create session request to help
     * fuzzing cover more lines */
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    UA_NodeId *authenticationToken = &request->requestHeader.authenticationToken;
    if(!UA_NodeId_isNull(authenticationToken) &&
       !UA_NodeId_isNull(&unsafe_fuzz_authenticationToken)) {
        UA_NodeId_clear(authenticationToken);
        UA_NodeId_copy(&unsafe_fuzz_authenticationToken, authenticationToken);
    }
#endif

    /* Get the session bound to the SecureChannel (not necessarily activated) */
    UA_Session *session = NULL;
    response->responseHeader.serviceResult =
        getBoundSession(server, channel, &request->requestHeader.authenticationToken, &session);
    if(!session && sd->sessionRequired)
        return false;

    /* The session can be NULL if not required */
    response->responseHeader.serviceResult = UA_STATUSCODE_GOOD;

    /* Process the service */
    UA_Boolean async =
        processServiceInternal(server, channel, session, requestId, sd, request, response);

    /* Update the service statistics */
#ifdef UA_ENABLE_DIAGNOSTICS
    if(session) {
        session->diagnostics.totalRequestCount.totalCount++;
        if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
            session->diagnostics.totalRequestCount.errorCount++;
        if(sd->counterOffset != 0) {
            UA_ServiceCounterDataType *serviceCounter = (UA_ServiceCounterDataType*)
                (((uintptr_t)&session->diagnostics) + sd->counterOffset);
            serviceCounter->totalCount++;
            if(response->responseHeader.serviceResult != UA_STATUSCODE_GOOD)
                serviceCounter->errorCount++;
        }
    }
#endif

    return async;
}
