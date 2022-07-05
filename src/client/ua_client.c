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
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Jan Hermes)
 */

#include <open62541/transport_generated.h>

#include "ua_client_internal.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"

static void
clientHouseKeeping(UA_Client *client, void *_);

/********************/
/* Client Lifecycle */
/********************/

UA_Client *
UA_Client_newWithConfig(const UA_ClientConfig *config) {
    if(!config)
        return NULL;
    UA_Client *client = (UA_Client*)UA_malloc(sizeof(UA_Client));
    if(!client)
        return NULL;
    memset(client, 0, sizeof(UA_Client));
    client->config = *config;

    UA_SecureChannel_init(&client->channel, &client->config.localConnectionConfig);
    client->connectStatus = UA_STATUSCODE_GOOD;

    /* Set up the regular callback for checking the internal state */
    UA_StatusCode res =
        UA_Client_addRepeatedCallback(client,
                                      (UA_ClientCallback)clientHouseKeeping,
                                      NULL, 1000.0, &client->houseKeepingCallbackId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return NULL;
    }
    return client;
}

static void
UA_ClientConfig_clear(UA_ClientConfig *config) {
    UA_ApplicationDescription_clear(&config->clientDescription);

    UA_ExtensionObject_clear(&config->userIdentityToken);
    UA_String_clear(&config->securityPolicyUri);

    UA_EndpointDescription_clear(&config->endpoint);
    UA_UserTokenPolicy_clear(&config->userTokenPolicy);

    if(config->certificateVerification.clear)
        config->certificateVerification.clear(&config->certificateVerification);

    /* Delete the SecurityPolicies */
    if(config->securityPolicies == 0)
        return;
    for(size_t i = 0; i < config->securityPoliciesSize; i++)
        config->securityPolicies[i].clear(&config->securityPolicies[i]);
    UA_free(config->securityPolicies);
    config->securityPolicies = 0;

    /* Stop and delete the EventLoop */
    UA_EventLoop *el = config->eventLoop;
    if(el && !config->externalEventLoop) {
        if(el->state != UA_EVENTLOOPSTATE_FRESH &&
           el->state != UA_EVENTLOOPSTATE_STOPPED) {
            el->stop(el);
            while(el->state != UA_EVENTLOOPSTATE_STOPPED) {
                el->run(el, 100);
            }
        }
        el->free(el);
        config->eventLoop = NULL;
    }

    /* Logger */
    if(config->logger.clear)
        config->logger.clear(config->logger.context);
    config->logger.log = NULL;
    config->logger.clear = NULL;

    if(config->sessionLocaleIdsSize > 0 && config->sessionLocaleIds) {
        UA_Array_delete(config->sessionLocaleIds,
                        config->sessionLocaleIdsSize, &UA_TYPES[UA_TYPES_LOCALEID]);
    }
    config->sessionLocaleIds = NULL;
    config->sessionLocaleIdsSize = 0;
}

static void
UA_Client_clear(UA_Client *client) {
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

    /* Remove the internal regular callback */
    UA_Client_removeCallback(client, client->houseKeepingCallbackId);
    client->houseKeepingCallbackId = 0;
}

void
UA_Client_delete(UA_Client* client) {
    UA_Client_disconnect(client);
    UA_Client_clear(client);
    UA_ClientConfig_clear(&client->config);
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
static const char *channelStateTexts[9] = {
    "Fresh", "HELSent", "HELReceived", "ACKSent",
    "AckReceived", "OPNSent", "Open", "Closing", "Closed"};
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

/* For both synchronous and asynchronous service calls */
static UA_StatusCode
sendRequest(UA_Client *client, const void *request,
            const UA_DataType *requestType, UA_UInt32 *requestId) {
    /* Renew SecureChannel if necessary */
    UA_Client_renewSecureChannel(client);
    if(client->connectStatus != UA_STATUSCODE_GOOD)
        return client->connectStatus;

    /* Adjusting the request header. The const attribute is violated, but we
     * only touch the following members: */
    UA_RequestHeader *rr = (UA_RequestHeader*)(uintptr_t)request;
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
                         "Sending request with RequestId %u of type %" PRIu32,
                         (unsigned)rqId, requestType->binaryEncodingId.identifier.numeric);
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
processMSGResponse(UA_Client *client, UA_UInt32 requestId,
                   const UA_ByteString *msg) {
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
    if(!ac) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Request with unknown RequestId %u", requestId);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

    UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                   "Processing request with RequestId %u", requestId);

    UA_Response asyncResponse;
    UA_Response *response = (ac->syncResponse) ? ac->syncResponse : &asyncResponse;
    const UA_DataType *responseType = ac->responseType;

    /* Dequeue ac. We might disconnect the client (remove all ac) in the callback. */
    LIST_REMOVE(ac, pointers);

    /* Decode the response type */
    size_t offset = 0;
    UA_NodeId responseTypeId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(msg, &offset, &responseTypeId);
    if(retval != UA_STATUSCODE_GOOD)
        goto process;

    /* Verify the type of the response */
    if(!UA_NodeId_equal(&responseTypeId, &ac->responseType->binaryEncodingId)) {
        /* Initialize before switching the responseType to ServiceFault.
         * Otherwise the decoding will leave fields from the original response
         * type uninitialized. */
        UA_init(response, ac->responseType);
        if(UA_NodeId_equal(&responseTypeId, &serviceFaultId)) {
            /* Decode as a ServiceFault, i.e. only the response header */
            UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Received a ServiceFault response");
            responseType = &UA_TYPES[UA_TYPES_SERVICEFAULT];
        } else {
            UA_LOG_ERROR(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Service response type does not match");
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
            goto process; /* Do not decode */
        }
    }

    /* Decode the response */
#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %s", responseType->typeName);
#else
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %" PRIu32,
                 responseTypeId.identifier.numeric);
#endif
    retval = UA_decodeBinaryInternal(msg, &offset, response, responseType,
                                     client->config.customDataTypes);

 process:
    /* Process the received MSG response */
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                       "Could not decode the response with RequestId %u with status %s",
                       (unsigned)requestId, UA_StatusCode_name(retval));
        response->responseHeader.serviceResult = retval;
    }

    /* Call the async callback */
    if(ac->callback)
        ac->callback(client, ac->userdata, requestId, response);

    /* Clean up */
    UA_NodeId_clear(&responseTypeId);
    if(!ac->syncResponse) {
        UA_clear(response, ac->responseType);
        UA_free(ac);
    } else {
        ac->syncResponse = NULL; /* Indicate that response was received */
    }
    return retval;
}

UA_StatusCode
processServiceResponse(void *application, UA_SecureChannel *channel,
                       UA_MessageType messageType, UA_UInt32 requestId,
                       UA_ByteString *message) {
    UA_Client *client = (UA_Client*)application;

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
        return processMSGResponse(client, requestId, message);
    default:
        UA_LOG_TRACE_CHANNEL(&client->config.logger, channel,
                             "Invalid message type");
        channel->state = UA_SECURECHANNELSTATE_CLOSING;
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }
}

void
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType) {
    /* Initialize. Response is valied in case of aborting. */
    UA_init(response, responseType);

    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;

    /* Verify that the EventLoop is running */
    UA_EventLoop *el = client->config.eventLoop;
    if(!el || el->state != UA_EVENTLOOPSTATE_STARTED) {
        respHeader->serviceResult = UA_STATUSCODE_BADINTERNALERROR;
		return;
    }

    /* Store the time until which the request has to be answered */
    UA_DateTime maxDate = UA_DateTime_nowMonotonic() +
        ((UA_DateTime)client->config.timeout * UA_DATETIME_MSEC);

    /* Check that the SecureChannel is open. Otherwise reopen. */
    if((client->sessionState != UA_SESSIONSTATE_ACTIVATED && !client->noSession) ||
       client->channel.state != UA_SECURECHANNELSTATE_OPEN) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Re-establish the connction for the synchronous service call");
        connectSync(client);
        if(client->connectStatus != UA_STATUSCODE_GOOD) {
            respHeader->serviceResult = client->connectStatus;
            return;
        }
    }

    AsyncServiceCall ac;
    ac.callback = NULL;
    ac.userdata = NULL;
    ac.responseType = responseType;
    ac.timeout = client->config.timeout;
    ac.syncResponse = (UA_Response*)response;

    /* Call the service and set the requestId */
    UA_StatusCode retval = sendRequest(client, request, requestType, &ac.requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        closeSecureChannel(client);
        notifyClientState(client);
        respHeader->serviceResult = retval;
        return;
    }

    /* Begin counting for the timeout after sending */
    ac.start = UA_DateTime_nowMonotonic();

    /* Temporarily insert to the async service list */
    LIST_INSERT_HEAD(&client->asyncServiceCalls, &ac, pointers);

    /* Run the EventLoop until the request was processed, the request has timed
     * out or the client connection fails */
    while(true) {
        /* Compute the remaining timeout and run the EventLoop */
        UA_DateTime now = UA_DateTime_nowMonotonic();
        if(now > maxDate) {
            retval = UA_STATUSCODE_BADTIMEOUT;
            break;
        }
        retval = el->run(el, (UA_UInt32)((maxDate - now) / UA_DATETIME_MSEC));

        /* Was the response received? In that case we can directly return. The
         * ac was already removed from the internal linked list. */
        if(ac.syncResponse == NULL)
            return;

        /* Check the status. Do not try to resend if the connection breaks.
         * Leave this to the application-level user. For example, we do not want
         * to call a method twice is the connection broke after sending the
         * request. */
        if(retval != UA_STATUSCODE_GOOD)
            break;
        retval = client->connectStatus;
        if(retval != UA_STATUSCODE_GOOD)
            break;
    }

    /* Detach from the internal async service list */
    LIST_REMOVE(&ac, pointers);

    /* Return the status code */
    respHeader->serviceResult = retval;
}

/***********************************/
/* Handling of Async Service Calls */
/***********************************/

static void
UA_Client_AsyncService_cancel(UA_Client *client, AsyncServiceCall *ac,
                              UA_StatusCode statusCode) {
    /* Set the status for the synchronous service call. Don't free the ac. */
    if(ac->syncResponse) {
        ac->syncResponse->responseHeader.serviceResult = statusCode;
        ac->syncResponse = NULL; /* Indicate the async service call was processed */
        return;
    }

    if(ac->callback) {
        /* Create an empty response with the statuscode and call the callback */
        UA_Response response;
        UA_init(&response, ac->responseType);
        response.responseHeader.serviceResult = statusCode;
        ac->callback(client, ac->userdata, ac->requestId, &response);

        /* Clean up the response. The user callback might move data into it. For
         * whatever reasons. */
        UA_clear(&response, ac->responseType);
    }

    UA_free(ac);
}

void
UA_Client_AsyncService_removeAll(UA_Client *client, UA_StatusCode statusCode) {
    /* Make this function reentrant. One of the async callbacks could indirectly
     * operate on the list. Moving all elements to a local list before iterating
     * that. */
    UA_AsyncServiceList asyncServiceCalls = client->asyncServiceCalls;
    LIST_INIT(&client->asyncServiceCalls);
    if(asyncServiceCalls.lh_first)
        asyncServiceCalls.lh_first->pointers.le_prev = &asyncServiceCalls.lh_first;

    /* Cancel and remove the elements from the local list */
    AsyncServiceCall *ac, *ac_tmp;
    LIST_FOREACH_SAFE(ac, &asyncServiceCalls, pointers, ac_tmp) {
        LIST_REMOVE(ac, pointers);
        UA_Client_AsyncService_cancel(client, ac, statusCode);
    }
}

UA_StatusCode
UA_Client_modifyAsyncCallback(UA_Client *client, UA_UInt32 requestId,
                              void *userdata, UA_ClientAsyncServiceCallback callback) {
    AsyncServiceCall *ac;
    LIST_FOREACH(ac, &client->asyncServiceCalls, pointers) {
        if(ac->requestId == requestId) {
            ac->callback = callback;
            ac->userdata = userdata;
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADNOTFOUND;
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
    ac->syncResponse = NULL;

    /* Call the service and set the requestId */
    UA_StatusCode retval = sendRequest(client, request, requestType, &ac->requestId);
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

/*******************/
/* Timed Callbacks */
/*******************/

UA_StatusCode
UA_Client_addTimedCallback(UA_Client *client, UA_ClientCallback callback,
                           void *data, UA_DateTime date, UA_UInt64 *callbackId) {
    if(!client->config.eventLoop)
        return UA_STATUSCODE_BADINTERNALERROR;
    return client->config.eventLoop->
        addTimedCallback(client->config.eventLoop, (UA_Callback)callback,
                         client, data, date, callbackId);
}

UA_StatusCode
UA_Client_addRepeatedCallback(UA_Client *client, UA_ClientCallback callback,
                              void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    if(!client->config.eventLoop)
        return UA_STATUSCODE_BADINTERNALERROR;
    return client->config.eventLoop->
        addCyclicCallback(client->config.eventLoop, (UA_Callback)callback,
                          client, data, interval_ms, NULL,
                          UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME, callbackId);
}

UA_StatusCode
UA_Client_changeRepeatedCallbackInterval(UA_Client *client, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    if(!client->config.eventLoop)
        return UA_STATUSCODE_BADINTERNALERROR;
    return client->config.eventLoop->
        modifyCyclicCallback(client->config.eventLoop, callbackId, interval_ms,
                             NULL, UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME);
}

void
UA_Client_removeCallback(UA_Client *client, UA_UInt64 callbackId) {
    if(!client->config.eventLoop)
        return;
    client->config.eventLoop->
        removeCyclicCallback(client->config.eventLoop, callbackId);
}

/**********************/
/* Housekeeping Tasks */
/**********************/

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

/* Regular housekeeping activities in the client -- called via a cyclic callback */
static void
clientHouseKeeping(UA_Client *client, void *_) {
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Internally check the the client state and "
                 "required activities");

    /* Renew Secure Channel */
    UA_Client_renewSecureChannel(client);

    /* Send read requests from time to time to test the connectivity */
    UA_Client_backgroundConnectivity(client);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Feed the server PublishRequests for the Subscriptions */
    UA_Client_Subscriptions_backgroundPublish(client);

    /* Check for inactive Subscriptions */
    UA_Client_Subscriptions_backgroundPublishInactivityCheck(client);
#endif

    /* Did async services time out? Process callbacks with an error code */
    asyncServiceTimeoutCheck(client);

    /* Log and notify user if the client state has changed */
    notifyClientState(client);
}

UA_StatusCode
UA_Client_run_iterate(UA_Client *client, UA_UInt32 timeout) {
    UA_ClientConfig *cc = UA_Client_getConfig(client);
    UA_EventLoop *el = cc->eventLoop;
    UA_CHECK_ERROR(el != NULL, return UA_STATUSCODE_BADINTERNALERROR,
                   &client->config.logger, UA_LOGCATEGORY_CLIENT,
                   "No EventLoop configured");

    /* Start the EventLoop? */
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if(el->state == UA_EVENTLOOPSTATE_FRESH) {
        rv = el->start(el);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* Process timed and network events in the EventLoop */
    rv = el->run(el, timeout);
    UA_CHECK_STATUS(rv, return rv);

    return client->connectStatus;
}

const UA_DataType *
UA_Client_findDataType(UA_Client *client, const UA_NodeId *typeId) {
    return UA_findDataTypeWithCustom(typeId, client->config.customDataTypes);
}
