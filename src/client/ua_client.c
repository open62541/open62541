/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2015-2016 (c) Sten Grüner
 *    Copyright 2015-2016 (c) Chris Iatrou
 *    Copyright 2015 (c) hfaham
 *    Copyright 2015-2017 (c) Florian Palm
 *    Copyright 2017-2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2015 (c) Holger Jeromin
 *    Copyright 2015 (c) Oleksiy Vasylyev
 *    Copyright 2016 (c) TorbenD
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2016 (c) Lykurg
 *    Copyright 2017 (c) Mark Giraud, Fraunhofer IOSB
 *    Copyright 2018 (c) Kalycito Infotech Private Limited
 */

#include <open62541/types_generated_encoding_binary.h>
#include "open62541/transport_generated.h"

#include "ua_client_internal.h"
#include "ua_types_encoding_binary.h"

/********************/
/* Client Lifecycle */
/********************/

static void
UA_Client_init(UA_Client* client) {
    UA_SecureChannel_init(&client->channel, &client->config.secureChannelConfig);
    client->connectStatus = UA_STATUSCODE_GOOD;
    client->socketConnectPending = false;
    UA_Timer_init(&client->timer);
    notifyClientState(client);
}

UA_Client UA_EXPORT *
UA_Client_newWithConfig(const UA_ClientConfig *config) {
    if(!config)
        return NULL;
    UA_Client *client = (UA_Client*)UA_malloc(sizeof(UA_Client));
    if(!client)
        return NULL;
    memset(client, 0, sizeof(UA_Client));
    client->config = *config;
    UA_Client_init(client);
    return client;
}

static void
UA_ClientConfig_deleteMembers(UA_ClientConfig *config) {
    UA_ApplicationDescription_deleteMembers(&config->clientDescription);

    UA_ExtensionObject_deleteMembers(&config->userIdentityToken);
    UA_String_deleteMembers(&config->securityPolicyUri);

    UA_EndpointDescription_deleteMembers(&config->endpoint);
    UA_UserTokenPolicy_deleteMembers(&config->userTokenPolicy);

    if(config->certificateVerification.clear)
        config->certificateVerification.clear(&config->certificateVerification);

    if(config->deleteNetworkManagerOnShutdown) {
        config->networkManager->shutdown(config->networkManager);
        config->networkManager->free(config->networkManager);
    }

    /* Delete the SecurityPolicies */
    if(config->securityPolicies == 0)
        return;
    for(size_t i = 0; i < config->securityPoliciesSize; i++)
        config->securityPolicies[i].clear(&config->securityPolicies[i]);
    UA_free(config->securityPolicies);
    config->securityPolicies = 0;

    /* Logger */
    if(config->logger.clear)
        config->logger.clear(config->logger.context);
    config->logger.log = NULL;
    config->logger.clear = NULL;
}

static void
UA_Client_deleteMembers(UA_Client *client) {
    /* Delete the async service calls with BADHSUTDOWN */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

    UA_Client_disconnect(client);
    UA_String_clear(&client->endpointUrl);

    UA_String_clear(&client->remoteNonce);
    UA_String_clear(&client->localNonce);

    /* Delete the subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_Subscriptions_clean(client);
#endif

    /* Delete the timed work */
    UA_Timer_deleteMembers(&client->timer);
}

void
UA_Client_delete(UA_Client* client) {
    UA_Client_deleteMembers(client);
    UA_ClientConfig_deleteMembers(&client->config);
    UA_free(client);
}

void
UA_Client_getState(UA_Client *client, UA_SecureChannelState *channelState,
                   UA_SessionState *sessionState, UA_StatusCode *connectStatus) {
    if(channelState)
        *channelState = client->channel.state;
    if(sessionState)
        *sessionState = client->sessionState;
    if(connectStatus)
        *connectStatus = client->connectStatus;
}

UA_ClientConfig *
UA_Client_getConfig(UA_Client *client) {
    if(!client)
        return NULL;
    return &client->config;
}

#if UA_LOGLEVEL <= 300
static const char *channelStateTexts[8] = {
    "Closed", "HELSent", "HELReceived", "ACKSent",
    "AckReceived", "OPNSent", "Open", "Closing"};
static const char *sessionStateTexts[6] =
    {"Closed", "CreateRequested", "Created",
     "ActivateRequested", "Activated", "Closing"};
#endif

void
notifyClientState(UA_Client *client) {
    if(client->connectStatus == client->oldConnectStatus &&
       client->channel.state == client->oldChannelState &&
       client->sessionState == client->oldSessionState)
        return;

#if UA_LOGLEVEL <= 300
    UA_Boolean info = (client->connectStatus != UA_STATUSCODE_GOOD);
    if(client->oldChannelState != client->channel.state)
        info |= (client->channel.state == UA_SECURECHANNELSTATE_OPEN ||
                 client->channel.state == UA_SECURECHANNELSTATE_CLOSED);
    if(client->oldSessionState != client->sessionState)
        info |= (client->sessionState == UA_SESSIONSTATE_CREATED ||
                 client->sessionState == UA_SESSIONSTATE_ACTIVATED ||
                 client->sessionState == UA_SESSIONSTATE_CLOSED);

    const char *channelStateText = channelStateTexts[client->channel.state];
    const char *sessionStateText = sessionStateTexts[client->sessionState];
    const char *connectStatusText = UA_StatusCode_name(client->connectStatus);

    if(info)
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Client Status: ChannelState: %s, SessionState: %s, ConnectStatus: %s",
                    channelStateText, sessionStateText, connectStatusText);
    else
        UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Client Status: ChannelState: %s, SessionState: %s, ConnectStatus: %s",
                     channelStateText, sessionStateText, connectStatusText);
#endif

    client->oldConnectStatus = client->connectStatus;
    client->oldChannelState = client->channel.state;
    client->oldSessionState = client->sessionState;

    if(client->config.stateCallback)
        client->config.stateCallback(client, client->channel.state,
                                     client->sessionState, client->connectStatus);
}

/****************/
/* Raw Services */
/****************/

/* For synchronous service calls. Execute async responses with a callback. When
 * the response with the correct requestId turns up, return it via the
 * SyncResponseDescription pointer. */
typedef struct {
    UA_Client *client;
    UA_Boolean received;
    UA_UInt32 requestId;
    void *response;
    const UA_DataType *responseType;
} SyncResponseDescription;

/* For both synchronous and asynchronous service calls */
static UA_StatusCode
sendServiceRequest(UA_Client *client, const void *request,
                   const UA_DataType *requestType, UA_UInt32 *requestId) {
    /* Renew SecureChannel if necessary */
    UA_Client_renewSecureChannel(client);
    if(client->connectStatus != UA_STATUSCODE_GOOD)
        return client->connectStatus;

    /* Adjusting the request header. The const attribute is violated, but we
     * only touch the following members: */
    UA_RequestHeader *rr = (UA_RequestHeader *) (uintptr_t) request;
    UA_NodeId oldToken = rr->authenticationToken; /* Put back in place later */
    rr->authenticationToken = client->authenticationToken;
    rr->timestamp = UA_DateTime_now();
    rr->requestHandle = ++client->requestHandle;
    UA_UInt32 rqId = ++client->requestId;

#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_LOG_DEBUG_CHANNEL(&client->config.logger, &client->channel,
                         "Sending request with RequestId %u of type %s",
                         (unsigned)rqId, requestType->typeName);
#else
    UA_LOG_DEBUG_CHANNEL(&client->config.logger, &client->channel,
                         "Sending request with RequestId %u of type %" PRIi16,
                         (unsigned)rqId, requestType->binaryEncodingId);
#endif

    /* Send the message */
    UA_StatusCode retval =
        UA_SecureChannel_sendSymmetricMessage(&client->channel, rqId,
                                              UA_MESSAGETYPE_MSG, rr, requestType);
    rr->authenticationToken = oldToken; /* Set the original token */

    *requestId = rqId;
    return retval;
}

static const UA_NodeId
serviceFaultId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SERVICEFAULT_ENCODING_DEFAULTBINARY}};

/* Look for the async callback in the linked list, execute and delete it */
static UA_StatusCode
processAsyncResponse(UA_Client *client, UA_UInt32 requestId, const UA_NodeId *responseTypeId,
                     const UA_ByteString *responseMessage, size_t *offset) {
    /* Find the callback */
    AsyncServiceCall *ac;
    LIST_FOREACH(ac, &client->asyncServiceCalls, pointers) {
        if(ac->requestId == requestId)
            break;
    }

    /* Part 6, 6.7.6: After the security validation is complete the receiver
     * shall verify the RequestId and the SequenceNumber. If these checks fail a
     * Bad_SecurityChecksFailed error is reported. The RequestId only needs to
     * be verified by the Client since only the Client knows if it is valid or
     * not.*/
    if(!ac)
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;

    /* Dequeue ac. We might disconnect (remove all ac) in the callback. */
    LIST_REMOVE(ac, pointers);

    /* Verify the type of the response */
    UA_Response response;
    const UA_DataType *responseType = ac->responseType;
    const UA_NodeId expectedNodeId = UA_NODEID_NUMERIC(0, ac->responseType->binaryEncodingId);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!UA_NodeId_equal(responseTypeId, &expectedNodeId)) {
        UA_init(&response, ac->responseType);
        if(UA_NodeId_equal(responseTypeId, &serviceFaultId)) {
            /* Decode as a ServiceFault, i.e. only the response header */
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Received a ServiceFault response");
            responseType = &UA_TYPES[UA_TYPES_SERVICEFAULT];
        } else {
            /* Close the connection */
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Reply contains the wrong service response");
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
            goto process;
        }
    }

    /* Decode the response */
    retval = UA_decodeBinary(responseMessage, offset, &response, responseType,
                             client->config.customDataTypes);

 process:
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not decode the response with id %u due to %s",
                    requestId, UA_StatusCode_name(retval));
        response.responseHeader.serviceResult = retval;
    } else if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        /* Decode as a ServiceFault, i.e. only the response header */
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "The ServiceResult has the StatusCode %s",
                    UA_StatusCode_name(response.responseHeader.serviceResult));
    }

    /* Call the callback */
    if(ac->callback)
        ac->callback(client, ac->userdata, requestId, &response);
    UA_clear(&response, ac->responseType);

    /* Remove the callback */
    UA_free(ac);
    return retval;
}

/* Processes the received service response. Either with an async callback or by
 * decoding the message and returning it "upwards" in the
 * SyncResponseDescription. */
UA_StatusCode
UA_Client_processServiceResponse(UA_SecureChannel *channel, UA_MessageType messageType,
                                 UA_UInt32 requestId, UA_ByteString *message) {
    // TODO: Use client here. What to do with response desc?
    UA_Client *client = (UA_Client *) channel->application;

    /* Process ACK response */
    switch(messageType) {
    case UA_MESSAGETYPE_ACK:
        processACKResponse(client, message);
        return UA_STATUSCODE_GOOD;
    case UA_MESSAGETYPE_OPN:
        processOPNResponse(client, message);
        return UA_STATUSCODE_GOOD;
    case UA_MESSAGETYPE_ERR:
        processERRResponse(client, message);
        return UA_STATUSCODE_GOOD;
    case UA_MESSAGETYPE_MSG:
        /* Continue below */
        break;
    default:
        UA_LOG_TRACE_CHANNEL(&client->config.logger, channel, "Invalid message type");
        channel->state = UA_SECURECHANNELSTATE_CLOSING;
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    /* Decode the data type identifier of the response */
    size_t offset = 0;
    UA_NodeId responseId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(message, &offset, &responseId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Got an asynchronous response. Don't expected a synchronous response
     * (responseType NULL) or the id does not match. */
    processAsyncResponse(client, requestId, &responseId, message, &offset);
    return UA_STATUSCODE_GOOD;
}

static void
sync_response_callback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                       void *response) {
    SyncResponseDescription *const rd = (SyncResponseDescription *const) userdata;
    /* Got the synchronous response */
    rd->received = true;

    UA_ResponseHeader *const responseHeader = (UA_ResponseHeader *const )response;
    if(responseHeader->serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Synchronous response failed with error code %s",
                     UA_StatusCode_name(responseHeader->serviceResult));
        UA_copy(response, rd->response, &UA_TYPES[UA_TYPES_RESPONSEHEADER]);
        return;
    }

    UA_copy(response, rd->response, rd->responseType);
}

void
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType) {
    /* Initialize. Response is valied in case of aborting. */
    UA_init(response, responseType);
    /* Renew SecureChannel if necessary */
    UA_Client_renewSecureChannel(client);
    if(client->connectStatus != UA_STATUSCODE_GOOD)
        return;

    if(client->channel.state != UA_SECURECHANNELSTATE_OPEN) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "SecureChannel must be connected before sending requests");
        UA_ResponseHeader *respHeader = (UA_ResponseHeader *) response;
        respHeader->serviceResult = UA_STATUSCODE_BADCONNECTIONCLOSED;
        return;
    }

    SyncResponseDescription rd = {client, false, 0, response, responseType};

    /* Send the request */
    UA_StatusCode retval = __UA_Client_AsyncServiceEx(client, request, requestType,
                                                      sync_response_callback, responseType, &rd,
                                                      &rd.requestId, 0);
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime maxDate = now + ((UA_DateTime)client->config.timeout * UA_DATETIME_MSEC);

    UA_ResponseHeader *respHeader = (UA_ResponseHeader *) response;
    if(retval == UA_STATUSCODE_GOOD) {
        while(!rd.received) {
            if(now > maxDate)
                retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
            if(retval == UA_STATUSCODE_GOOD) {
                retval = client->config.networkManager->process(client->config.networkManager, 0);
            }
            if(client->channel.socket == NULL && retval == UA_STATUSCODE_GOOD)
                retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
            if(retval != UA_STATUSCODE_GOOD) {
                // We need to clean up the async call here, since it was not processed yet,
                // and the rd is not valid anymore as soon as we return.
                AsyncServiceCall *ac = NULL;
                LIST_FOREACH(ac, &client->asyncServiceCalls, pointers)if(ac->requestId == rd.requestId)
                        break;
                if(ac != NULL) {
                    LIST_REMOVE(ac, pointers);
                    UA_free(ac);
                }
                closeSecureChannel(client);
                respHeader->serviceResult = retval;
                return;
            }
            now = UA_DateTime_nowMonotonic();
        }
    } else if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED) {
        respHeader->serviceResult = UA_STATUSCODE_BADREQUESTTOOLARGE;
        return;
    }

    /* In synchronous service, if we have don't have a reply we need to close
     * the connection. For all other error cases, receiveResponse has already
     * closed the channel. */
    if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT ||
       client->channel.state == UA_SECURECHANNELSTATE_CLOSING) {
        closeSecureChannel(client);
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    if(retval != UA_STATUSCODE_GOOD)
        respHeader->serviceResult = retval;

    notifyClientState(client);
}

void
UA_Client_AsyncService_cancel(UA_Client *client, AsyncServiceCall *ac,
                              UA_StatusCode statusCode) {
    /* Create an empty response with the statuscode */
    UA_Response response;
    UA_init(&response, ac->responseType);
    response.responseHeader.serviceResult = statusCode;

    if(ac->callback)
        ac->callback(client, ac->userdata, ac->requestId, &response);

    /* Clean up the response. Users might move data into it. For whatever reasons. */
    UA_clear(&response, ac->responseType);
}

void UA_Client_AsyncService_removeAll(UA_Client *client, UA_StatusCode statusCode) {
    AsyncServiceCall *ac, *ac_tmp;
    LIST_FOREACH_SAFE(ac, &client->asyncServiceCalls, pointers, ac_tmp) {
        LIST_REMOVE(ac, pointers);
        UA_Client_AsyncService_cancel(client, ac, statusCode);
        UA_free(ac);
    }
}

UA_StatusCode
__UA_Client_AsyncServiceEx(UA_Client *client, const void *request,
                           const UA_DataType *requestType,
                           UA_ClientAsyncServiceCallback callback,
                           const UA_DataType *responseType,
                           void *userdata, UA_UInt32 *requestId,
                           UA_UInt32 timeout) {
    if(client->channel.state != UA_SECURECHANNELSTATE_OPEN) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "SecureChannel must be connected before sending requests");
		return UA_STATUSCODE_BADSERVERNOTCONNECTED;
    }

    /* Prepare the entry for the linked list */
    AsyncServiceCall *ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
    if(!ac)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ac->callback = callback;
    ac->responseType = responseType;
    ac->userdata = userdata;
    ac->timeout = timeout;

    /* Call the service and set the requestId */
    UA_StatusCode retval = sendServiceRequest(client, request, requestType, &ac->requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(ac);
        closeSecureChannel(client);
        notifyClientState(client);
        return retval;
    }

    ac->start = UA_DateTime_nowMonotonic();

    /* Store the entry for async processing */
    LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);
    if(requestId)
        *requestId = ac->requestId;

    notifyClientState(client);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
__UA_Client_AsyncService(UA_Client *client, const void *request,
                         const UA_DataType *requestType,
                         UA_ClientAsyncServiceCallback callback,
                         const UA_DataType *responseType,
                         void *userdata, UA_UInt32 *requestId) {
    return __UA_Client_AsyncServiceEx(client, request, requestType, callback, responseType,
                                      userdata, requestId, client->config.timeout);
}

UA_StatusCode
UA_Client_sendAsyncRequest(UA_Client *client, const void *request,
                           const UA_DataType *requestType,
                           UA_ClientAsyncServiceCallback callback,
                           const UA_DataType *responseType, void *userdata,
                           UA_UInt32 *requestId) {
    return __UA_Client_AsyncService(client, request, requestType, callback,
                                    responseType, userdata, requestId);
}

UA_StatusCode UA_EXPORT
UA_Client_addTimedCallback(UA_Client *client, UA_ClientCallback callback,
                           void *data, UA_DateTime date, UA_UInt64 *callbackId) {
    return UA_Timer_addTimedCallback(&client->timer, (UA_ApplicationCallback)callback,
                                     client, data, date, callbackId);
}

UA_StatusCode
UA_Client_addRepeatedCallback(UA_Client *client, UA_ClientCallback callback,
                              void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&client->timer, (UA_ApplicationCallback)callback,
                                        client, data, interval_ms, callbackId);
}

UA_StatusCode
UA_Client_changeRepeatedCallbackInterval(UA_Client *client, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    return UA_Timer_changeRepeatedCallbackInterval(&client->timer, callbackId, interval_ms);
}

void
UA_Client_removeCallback(UA_Client *client, UA_UInt64 callbackId) {
    UA_Timer_removeCallback(&client->timer, callbackId);
}

static void
asyncServiceTimeoutCheck(UA_Client *client) {
    AsyncServiceCall *ac, *ac_tmp;
    UA_DateTime now = UA_DateTime_nowMonotonic();
    LIST_FOREACH_SAFE(ac, &client->asyncServiceCalls, pointers, ac_tmp) {
        if(!ac->timeout)
           continue;
        if(ac->start + (UA_DateTime)(ac->timeout * UA_DATETIME_MSEC) <= now) {
            LIST_REMOVE(ac, pointers);
            UA_Client_AsyncService_cancel(client, ac, UA_STATUSCODE_BADTIMEOUT);
            UA_free(ac);
        }
    }
}

static void
backgroundConnectivityCallback(UA_Client *client, void *userdata,
                               UA_UInt32 requestId, const UA_ReadResponse *response) {
    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADTIMEOUT) {
        if(client->config.inactivityCallback)
            client->config.inactivityCallback(client);
    }
    client->pendingConnectivityCheck = false;
    client->lastConnectivityCheck = UA_DateTime_nowMonotonic();
}

static void
UA_Client_backgroundConnectivity(UA_Client *client) {
    if(!client->config.connectivityCheckInterval)
        return;

    if(client->pendingConnectivityCheck)
        return;

    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime nextDate = client->lastConnectivityCheck +
        (UA_DateTime)(client->config.connectivityCheckInterval * UA_DATETIME_MSEC);
    if(now <= nextDate)
        return;

    /* Prepare the request */
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    rvid.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    UA_ReadRequest request;
    UA_ReadRequest_init(&request);
    request.nodesToRead = &rvid;
    request.nodesToReadSize = 1;
    UA_StatusCode retval =
        __UA_Client_AsyncService(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                                 (UA_ClientAsyncServiceCallback)backgroundConnectivityCallback,
                                 &UA_TYPES[UA_TYPES_READRESPONSE], NULL, NULL);
    if(retval == UA_STATUSCODE_GOOD)
        client->pendingConnectivityCheck = true;
}

static void
clientExecuteRepeatedCallback(void *executionApplication, UA_ApplicationCallback cb,
                              void *callbackApplication, void *data) {
    cb(callbackApplication, data);
}

UA_StatusCode
UA_Client_run_iterate(UA_Client *client, UA_UInt32 timeout) {
    /* Process timed (repeated) jobs */
    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_Timer_process(&client->timer, now,
                     (UA_TimerExecutionCallback)clientExecuteRepeatedCallback, client);

    /* Make sure we have an open channel */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if((client->noSession && client->channel.state != UA_SECURECHANNELSTATE_OPEN) ||
       client->sessionState < UA_SESSIONSTATE_ACTIVATED) {
        retval = connectIterate(client, timeout);
        notifyClientState(client);
        return retval;
    }

    /* Renew Secure Channel */
    UA_Client_renewSecureChannel(client);
    if(client->connectStatus != UA_STATUSCODE_GOOD)
        return client->connectStatus;

    /* Feed the server PublishRequests for the Subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_Subscriptions_backgroundPublish(client);
#endif

    /* Send read requests from time to time to test the connectivity */
    UA_Client_backgroundConnectivity(client);

    /* Listen on the network for the given timeout */
    retval = client->config.networkManager->process(client->config.networkManager, timeout);
    if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT)
        retval = UA_STATUSCODE_GOOD;
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Could not process network manager with StatusCode %s",
                       UA_StatusCode_name(retval));
        closeSecureChannel(client);
        client->connectStatus = retval;
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The inactivity check must be done after receiveServiceResponse*/
    UA_Client_Subscriptions_backgroundPublishInactivityCheck(client);
#endif

    /* Did async services time out? Process callbacks with an error code */
    asyncServiceTimeoutCheck(client);

    /* Log and notify user if the client state has changed */
    notifyClientState(client);

    return client->connectStatus;
}
