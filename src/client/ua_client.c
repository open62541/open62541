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
 *    Copyright 2022 (c) Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/transport_generated.h>

#include "ua_client_internal.h"
#include "ua_types_encoding_binary.h"

static void
clientHouseKeeping(UA_Client *client, void *_);

/********************/
/* Client Lifecycle */
/********************/

UA_StatusCode
UA_ClientConfig_copy(UA_ClientConfig const *src, UA_ClientConfig *dst){
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    retval = UA_ApplicationDescription_copy(&src->clientDescription, &dst->clientDescription);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_ExtensionObject_copy(&src->userIdentityToken, &dst->userIdentityToken);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_String_copy(&src->securityPolicyUri, &dst->securityPolicyUri);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_EndpointDescription_copy(&src->endpoint, &dst->endpoint);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_UserTokenPolicy_copy(&src->userTokenPolicy, &dst->userTokenPolicy);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    retval = UA_Array_copy(src->sessionLocaleIds, src->sessionLocaleIdsSize,
                           (void **)&dst->sessionLocaleIds, &UA_TYPES[UA_TYPES_LOCALEID]);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    dst->sessionLocaleIdsSize = src->sessionLocaleIdsSize;
    dst->connectivityCheckInterval = src->connectivityCheckInterval;
    dst->certificateVerification = src->certificateVerification;
    dst->clientContext = src->clientContext;
    dst->customDataTypes = src->customDataTypes;
    dst->eventLoop = src->eventLoop;
    dst->externalEventLoop = src->externalEventLoop;
    dst->inactivityCallback = src->inactivityCallback;
    dst->localConnectionConfig = src->localConnectionConfig;
    dst->logging = src->logging;
    if(src->certificateVerification.logging == NULL)
        dst->certificateVerification.logging = dst->logging;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    dst->outStandingPublishRequests = src->outStandingPublishRequests;
#endif
    dst->requestedSessionTimeout = src->requestedSessionTimeout;
    dst->secureChannelLifeTime = src->secureChannelLifeTime;
    dst->securityMode = src->securityMode;
    dst->stateCallback = src->stateCallback;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    dst->subscriptionInactivityCallback = src->subscriptionInactivityCallback;
#endif
    dst->timeout = src->timeout;
    dst->userTokenPolicy = src->userTokenPolicy;
    dst->securityPolicies = src->securityPolicies;
    dst->securityPoliciesSize = src->securityPoliciesSize;
    dst->authSecurityPolicies = src->authSecurityPolicies;
    dst->authSecurityPoliciesSize = src->authSecurityPoliciesSize;

cleanup:
    if(retval != UA_STATUSCODE_GOOD) {
        /* _clear will free the plugins in dst that are a shallow copy from src. */
        dst->authSecurityPolicies = NULL;
        dst->certificateVerification.context = NULL;
        dst->eventLoop = NULL;
        dst->logging = NULL;
        dst->securityPolicies = NULL;
        UA_ClientConfig_clear(dst);
    }
    return retval;
}

UA_Client *
UA_Client_newWithConfig(const UA_ClientConfig *config) {
    if(!config)
        return NULL;
    UA_Client *client = (UA_Client*)UA_malloc(sizeof(UA_Client));
    if(!client)
        return NULL;
    memset(client, 0, sizeof(UA_Client));
    client->config = *config;

    UA_SecureChannel_init(&client->channel);
    client->channel.config = client->config.localConnectionConfig;
    client->connectStatus = UA_STATUSCODE_GOOD;

#if UA_MULTITHREADING >= 100
    UA_LOCK_INIT(&client->clientMutex);
#endif

    return client;
}

void
UA_ClientConfig_clear(UA_ClientConfig *config) {
    UA_ApplicationDescription_clear(&config->clientDescription);
    UA_String_clear(&config->endpointUrl);
    UA_ExtensionObject_clear(&config->userIdentityToken);

    /* Delete the SecurityPolicies for Authentication */
    if(config->authSecurityPolicies != 0) {
        for(size_t i = 0; i < config->authSecurityPoliciesSize; i++)
            config->authSecurityPolicies[i].clear(&config->authSecurityPolicies[i]);
        UA_free(config->authSecurityPolicies);
        config->authSecurityPolicies = 0;
    }
    UA_String_clear(&config->securityPolicyUri);
    UA_String_clear(&config->authSecurityPolicyUri);

    UA_EndpointDescription_clear(&config->endpoint);
    UA_UserTokenPolicy_clear(&config->userTokenPolicy);

    UA_String_clear(&config->applicationUri);

    if(config->certificateVerification.clear)
        config->certificateVerification.clear(&config->certificateVerification);

    /* Delete the SecurityPolicies */
    if(config->securityPolicies != 0) {
        for(size_t i = 0; i < config->securityPoliciesSize; i++)
            config->securityPolicies[i].clear(&config->securityPolicies[i]);
        UA_free(config->securityPolicies);
        config->securityPolicies = 0;
    }

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

    /* Logging */
    if(config->logging != NULL && config->logging->clear != NULL)
        config->logging->clear(config->logging);
    config->logging = NULL;

    UA_String_clear(&config->sessionName);
    if(config->sessionLocaleIdsSize > 0 && config->sessionLocaleIds) {
        UA_Array_delete(config->sessionLocaleIds,
                        config->sessionLocaleIdsSize, &UA_TYPES[UA_TYPES_LOCALEID]);
    }
    config->sessionLocaleIds = NULL;
    config->sessionLocaleIdsSize = 0;

    /* Custom Data Types */
    UA_cleanupDataTypeWithCustom(config->customDataTypes);

#ifdef UA_ENABLE_ENCRYPTION
    config->privateKeyPasswordCallback = NULL;
#endif
}

void
UA_ClientConfig_delete(UA_ClientConfig *config){
    UA_assert(config != NULL);
    UA_ClientConfig_clear(config);
    UA_free(config);
}

static void
UA_Client_clear(UA_Client *client) {
    /* Prevent new async service calls in UA_Client_AsyncService_removeAll */
    UA_SessionState oldState = client->sessionState;
    client->sessionState = UA_SESSIONSTATE_CLOSING;

    /* Delete the async service calls with BADHSUTDOWN */
    __Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

    /* Reset to the old state to properly close the session */
    client->sessionState = oldState;

    UA_Client_disconnect(client);
    UA_String_clear(&client->discoveryUrl);
    UA_ApplicationDescription_clear(&client->serverDescription);

    UA_String_clear(&client->serverSessionNonce);
    UA_String_clear(&client->clientSessionNonce);

    /* Delete the subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    __Client_Subscriptions_clean(client);
#endif

    /* Remove the internal regular callback */
    UA_Client_removeCallback(client, client->houseKeepingCallbackId);
    client->houseKeepingCallbackId = 0;

    UA_SecureChannel_clear(&client->channel);

#if UA_MULTITHREADING >= 100
    UA_LOCK_DESTROY(&client->clientMutex);
#endif
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
    UA_LOCK(&client->clientMutex);
    if(channelState)
        *channelState = client->channel.state;
    if(sessionState)
        *sessionState = client->sessionState;
    if(connectStatus)
        *connectStatus = client->connectStatus;
    UA_UNLOCK(&client->clientMutex);
}

UA_ClientConfig *
UA_Client_getConfig(UA_Client *client) {
    if(!client)
        return NULL;
    return &client->config;
}

#if UA_LOGLEVEL <= 300
static const char *channelStateTexts[14] = {
    "Fresh", "ReverseListening", "Connecting", "Connected", "ReverseConnected", "RHESent", "HELSent", "HELReceived", "ACKSent",
    "AckReceived", "OPNSent", "Open", "Closing", "Closed"};
static const char *sessionStateTexts[6] =
    {"Closed", "CreateRequested", "Created",
     "ActivateRequested", "Activated", "Closing"};
#endif

void
notifyClientState(UA_Client *client) {
    UA_LOCK_ASSERT(&client->clientMutex, 1);

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
        UA_LOG_INFO(client->config.logging, UA_LOGCATEGORY_CLIENT,
                    "Client Status: ChannelState: %s, SessionState: %s, ConnectStatus: %s",
                    channelStateText, sessionStateText, connectStatusText);
    else
        UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "Client Status: ChannelState: %s, SessionState: %s, ConnectStatus: %s",
                     channelStateText, sessionStateText, connectStatusText);
#endif

    client->oldConnectStatus = client->connectStatus;
    client->oldChannelState = client->channel.state;
    client->oldSessionState = client->sessionState;

    UA_UNLOCK(&client->clientMutex);
    if(client->config.stateCallback)
        client->config.stateCallback(client, client->channel.state,
                                     client->sessionState, client->connectStatus);
    UA_LOCK(&client->clientMutex);
}

/****************/
/* Raw Services */
/****************/

/* For both synchronous and asynchronous service calls */
static UA_StatusCode
sendRequest(UA_Client *client, const void *request,
            const UA_DataType *requestType, UA_UInt32 *requestId) {
    UA_LOCK_ASSERT(&client->clientMutex, 1);

    /* Renew SecureChannel if necessary */
    __Client_renewSecureChannel(client);
    if(client->connectStatus != UA_STATUSCODE_GOOD)
        return client->connectStatus;

    UA_EventLoop *el = client->config.eventLoop;

    /* Adjusting the request header. The const attribute is violated, but we
     * only touch the following members: */
    UA_RequestHeader *rr = (UA_RequestHeader*)(uintptr_t)request;
    UA_NodeId oldToken = rr->authenticationToken; /* Put back in place later */
    rr->authenticationToken = client->authenticationToken;
    rr->timestamp = el->dateTime_now(el);

    /* Create a unique handle >100,000 if not manually defined. The handle is
     * not necessarily unique when manually defined and used to cancel async
     * service requests. */
    if(rr->requestHandle == 0) {
        if(UA_UNLIKELY(client->requestHandle < 100000))
            client->requestHandle = 100000;
        rr->requestHandle = ++client->requestHandle;
    }

    /* Set the timeout hint if not manually defined */
    if(rr->timeoutHint == 0)
        rr->timeoutHint = client->config.timeout;

    /* Generate the request id */
    UA_UInt32 rqId = ++client->requestId;

#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_LOG_DEBUG_CHANNEL(client->config.logging, &client->channel,
                         "Sending request with RequestId %u of type %s",
                         (unsigned)rqId, requestType->typeName);
#else
    UA_LOG_DEBUG_CHANNEL(client->config.logging, &client->channel,
                         "Sending request with RequestId %u of type %" PRIu32,
                         (unsigned)rqId, requestType->binaryEncodingId.identifier.numeric);
#endif

    /* Send the message */
    UA_StatusCode retval =
        UA_SecureChannel_sendSymmetricMessage(&client->channel, rqId,
                                              UA_MESSAGETYPE_MSG, rr, requestType);

    rr->authenticationToken = oldToken; /* Set back to the original token */

    /* Sending failed. The SecureChannel cannot recover from that. Call
     * closeSecureChannel to a) close from our end and b) set the session to
     * non-activated. */
    if(retval != UA_STATUSCODE_GOOD)
        closeSecureChannel(client);

    /* Return the request id */
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
        UA_LOG_WARNING(client->config.logging, UA_LOGCATEGORY_CLIENT,
                       "Request with unknown RequestId %u", requestId);
        return UA_STATUSCODE_BADSECURITYCHECKSFAILED;
    }

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
            UA_LOG_INFO(client->config.logging, UA_LOGCATEGORY_CLIENT,
                        "Received a ServiceFault response");
            responseType = &UA_TYPES[UA_TYPES_SERVICEFAULT];
        } else {
            UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                         "Service response type does not match");
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
            goto process; /* Do not decode */
        }
    }

    /* Decode the response */
#ifdef UA_ENABLE_TYPEDESCRIPTION
    UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %s", responseType->typeName);
#else
    UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %" PRIu32,
                 responseTypeId.identifier.numeric);
#endif
    retval = UA_decodeBinaryInternal(msg, &offset, response, responseType,
                                     client->config.customDataTypes);

 process:
    /* Process the received MSG response */
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(client->config.logging, UA_LOGCATEGORY_CLIENT,
                       "Could not decode the response with RequestId %u with status %s",
                       (unsigned)requestId, UA_StatusCode_name(retval));
        response->responseHeader.serviceResult = retval;
    }

    /* The Session closed. The current response is processed with the return code.
     * The next request first recreates a session. */
    if(responseType != &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE] &&
       (response->responseHeader.serviceResult == UA_STATUSCODE_BADSESSIONIDINVALID ||
        response->responseHeader.serviceResult == UA_STATUSCODE_BADSESSIONCLOSED)) {
        /* Clean up the session information and reset the state */
        cleanupSession(client);

        if(client->config.noNewSession) {
            /* Configuration option to not create a new Session. Disconnect the
             * client. */
            client->connectStatus = response->responseHeader.serviceResult;
            UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                         "Session cannot be activated with StatusCode %s. "
                         "The client is configured not to create a new Session.",
                         UA_StatusCode_name(client->connectStatus));
            closeSecureChannel(client);
        } else {
            UA_LOG_WARNING(client->config.logging, UA_LOGCATEGORY_CLIENT,
                           "Session no longer valid. A new Session is created for the next "
                           "Service request but we do not re-send the current request.");
        }
    }

    /* Call the async callback. This is the only thread with access to ac. So we
     * can just unlock for the callback into userland. */
    UA_UNLOCK(&client->clientMutex);
    if(ac->callback)
        ac->callback(client, ac->userdata, requestId, response);
    UA_LOCK(&client->clientMutex);

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

    if(!UA_SecureChannel_isConnected(channel)) {
        if(messageType == UA_MESSAGETYPE_MSG) {
            UA_LOG_DEBUG_CHANNEL(client->config.logging, channel, "Discard MSG message "
                                 "with RequestId %u as the SecureChannel is not connected",
                                 requestId);
        } else {
            UA_LOG_DEBUG_CHANNEL(client->config.logging, channel, "Discard message "
                                 "as the SecureChannel is not connected");
        }
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    switch(messageType) {
    case UA_MESSAGETYPE_RHE:
        UA_LOG_DEBUG_CHANNEL(client->config.logging, channel, "Process RHE message");
        processRHEMessage(client, message);
        return UA_STATUSCODE_GOOD;
    case UA_MESSAGETYPE_ACK:
        UA_LOG_DEBUG_CHANNEL(client->config.logging, channel, "Process ACK message");
        processACKResponse(client, message);
        return UA_STATUSCODE_GOOD;
    case UA_MESSAGETYPE_OPN:
        UA_LOG_DEBUG_CHANNEL(client->config.logging, channel, "Process OPN message");
        processOPNResponse(client, message);
        return UA_STATUSCODE_GOOD;
    case UA_MESSAGETYPE_ERR:
        UA_LOG_DEBUG_CHANNEL(client->config.logging, channel, "Process ERR message");
        processERRResponse(client, message);
        return UA_STATUSCODE_GOOD;
    case UA_MESSAGETYPE_MSG:
        UA_LOG_DEBUG_CHANNEL(client->config.logging, channel, "Process MSG message "
                             "with RequestId %u", requestId);
        return processMSGResponse(client, requestId, message);
    default:
        UA_LOG_TRACE_CHANNEL(client->config.logging, channel,
                             "Invalid message type");
        channel->state = UA_SECURECHANNELSTATE_CLOSING;
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }
}

void
__Client_Service(UA_Client *client, const void *request,
                 const UA_DataType *requestType, void *response,
                 const UA_DataType *responseType) {
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;

    /* Initialize. Response is valied in case of aborting. */
    UA_init(response, responseType);

    /* Verify that the EventLoop is running */
    UA_EventLoop *el = client->config.eventLoop;
    if(!el || el->state != UA_EVENTLOOPSTATE_STARTED) {
        respHeader->serviceResult = UA_STATUSCODE_BADINTERNALERROR;
		return;
    }

    /* Check that the SecureChannel is open and also a Session active (if we
     * want a Session). Otherwise reopen. */
    if(!isFullyConnected(client)) {
        UA_LOG_INFO(client->config.logging, UA_LOGCATEGORY_CLIENT,
                    "Re-establish the connction for the synchronous service call");
        connectSync(client);
        if(client->connectStatus != UA_STATUSCODE_GOOD) {
            respHeader->serviceResult = client->connectStatus;
            return;
        }
    }

    /* Store the channelId to detect if the channel was changed by a
     * reconnection within the EventLoop run method. */
    UA_UInt32 channelId = client->channel.securityToken.channelId;

    /* Send the request */
    UA_UInt32 requestId = 0;
    UA_StatusCode retval = sendRequest(client, request, requestType, &requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        /* If sending failed, the status is set to closing. The SecureChannel is
         * the actually closed in the next iteration of the EventLoop. */
        UA_assert(client->channel.state == UA_SECURECHANNELSTATE_CLOSING ||
                  client->channel.state == UA_SECURECHANNELSTATE_CLOSED);
        UA_LOG_WARNING(client->config.logging, UA_LOGCATEGORY_CLIENT,
                       "Sending the request failed with status %s",
                       UA_StatusCode_name(retval));
        notifyClientState(client);
        respHeader->serviceResult = retval;
        return;
    }

    /* Temporarily insert an AsyncServiceCall */
    const UA_RequestHeader *rh = (const UA_RequestHeader*)request;
    AsyncServiceCall ac;
    ac.callback = NULL;
    ac.userdata = NULL;
    ac.responseType = responseType;
    ac.syncResponse = (UA_Response*)response;
    ac.requestId = requestId;
    ac.start = el->dateTime_nowMonotonic(el); /* Start timeout after sending */
    ac.timeout = rh->timeoutHint;
    ac.requestHandle = rh->requestHandle;
    if(ac.timeout == 0)
        ac.timeout = UA_UINT32_MAX; /* 0 -> unlimited */

    LIST_INSERT_HEAD(&client->asyncServiceCalls, &ac, pointers);

    /* Time until which the request has to be answered */
    UA_DateTime maxDate = ac.start + ((UA_DateTime)ac.timeout * UA_DATETIME_MSEC);

    /* Run the EventLoop until the request was processed, the request has timed
     * out or the client connection fails */
    UA_UInt32 timeout_remaining = ac.timeout;
    while(true) {
        /* Unlock before dropping into the EventLoop. The client lock is
         * re-taken in the network callback if an event occurs. */
        UA_UNLOCK(&client->clientMutex);
        retval = el->run(el, timeout_remaining);
        UA_LOCK(&client->clientMutex);

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

        /* The connection was lost */
        retval = client->connectStatus;
        if(retval != UA_STATUSCODE_GOOD)
            break;

        /* The channel is no longer the same or was closed */
        if(channelId != client->channel.securityToken.channelId) {
            retval = UA_STATUSCODE_BADSECURECHANNELCLOSED;
            break;
        }

        /* Update the remaining timeout or break */
        UA_DateTime now = ac.start = el->dateTime_nowMonotonic(el);
        if(now > maxDate) {
            retval = UA_STATUSCODE_BADTIMEOUT;
            break;
        }
        timeout_remaining = (UA_UInt32)((maxDate - now) / UA_DATETIME_MSEC);
    }

    /* Detach from the internal async service list */
    LIST_REMOVE(&ac, pointers);

    /* Return the status code */
    respHeader->serviceResult = retval;
}

void
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType) {
    UA_LOCK(&client->clientMutex);
    __Client_Service(client, request, requestType, response, responseType);
    UA_UNLOCK(&client->clientMutex);
}

/***********************************/
/* Handling of Async Service Calls */
/***********************************/

static void
__Client_AsyncService_cancel(UA_Client *client, AsyncServiceCall *ac,
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
        UA_UNLOCK(&client->clientMutex);
        ac->callback(client, ac->userdata, ac->requestId, &response);
        UA_LOCK(&client->clientMutex);

        /* Clean up the response. The user callback might move data into it. For
         * whatever reasons. */
        UA_clear(&response, ac->responseType);
    }

    UA_free(ac);
}

void
__Client_AsyncService_removeAll(UA_Client *client, UA_StatusCode statusCode) {
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
        __Client_AsyncService_cancel(client, ac, statusCode);
    }
}

UA_StatusCode
UA_Client_modifyAsyncCallback(UA_Client *client, UA_UInt32 requestId,
                              void *userdata, UA_ClientAsyncServiceCallback callback) {
    UA_LOCK(&client->clientMutex);
    AsyncServiceCall *ac;
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    LIST_FOREACH(ac, &client->asyncServiceCalls, pointers) {
        if(ac->requestId == requestId) {
            ac->callback = callback;
            ac->userdata = userdata;
            res = UA_STATUSCODE_GOOD;
            break;
        }
    }
    UA_UNLOCK(&client->clientMutex);
    return res;
}

UA_StatusCode
__Client_AsyncService(UA_Client *client, const void *request,
                      const UA_DataType *requestType,
                      UA_ClientAsyncServiceCallback callback,
                      const UA_DataType *responseType,
                      void *userdata, UA_UInt32 *requestId) {
    UA_LOCK_ASSERT(&client->clientMutex, 1);

    /* Is the SecureChannel connected? */
    if(client->channel.state != UA_SECURECHANNELSTATE_OPEN) {
        UA_LOG_ERROR(client->config.logging, UA_LOGCATEGORY_CLIENT,
                     "SecureChannel must be connected to send request");
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;
    }

    /* Prepare the entry for the linked list */
    AsyncServiceCall *ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
    if(!ac)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Call the service and set the requestId */
    UA_StatusCode retval = sendRequest(client, request, requestType, &ac->requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        /* If sending failed, the status is set to closing. The SecureChannel is
         * the actually closed in the next iteration of the EventLoop. */
        UA_assert(client->channel.state == UA_SECURECHANNELSTATE_CLOSING ||
                  client->channel.state == UA_SECURECHANNELSTATE_CLOSED);
        UA_free(ac);
        notifyClientState(client);
        return retval;
    }

    /* Set up the AsyncServiceCall for processing the response */
    UA_EventLoop *el = client->config.eventLoop;
    const UA_RequestHeader *rh = (const UA_RequestHeader*)request;
    ac->callback = callback;
    ac->responseType = responseType;
    ac->userdata = userdata;
    ac->syncResponse = NULL;
    ac->start = el->dateTime_nowMonotonic(el);
    ac->timeout = rh->timeoutHint;
    ac->requestHandle = rh->requestHandle;
    if(ac->timeout == 0)
        ac->timeout = UA_UINT32_MAX; /* 0 -> unlimited */

    LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);

    /* Return the generated request id */
    if(requestId)
        *requestId = ac->requestId;

    /* Notify the userland if a change happened */
    notifyClientState(client);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
__UA_Client_AsyncService(UA_Client *client, const void *request,
                         const UA_DataType *requestType,
                         UA_ClientAsyncServiceCallback callback,
                         const UA_DataType *responseType,
                         void *userdata, UA_UInt32 *requestId) {
    UA_LOCK(&client->clientMutex);
    UA_StatusCode res =
        __Client_AsyncService(client, request, requestType, callback, responseType,
                              userdata, requestId);
    UA_UNLOCK(&client->clientMutex);
    return res;
}

static UA_StatusCode
cancelByRequestHandle(UA_Client *client, UA_UInt32 requestHandle, UA_UInt32 *cancelCount) {
    UA_CancelRequest creq;
    UA_CancelRequest_init(&creq);
    creq.requestHandle = requestHandle;
    UA_CancelResponse cresp;
    UA_CancelResponse_init(&cresp);
    __Client_Service(client, &creq, &UA_TYPES[UA_TYPES_CANCELREQUEST],
                     &cresp, &UA_TYPES[UA_TYPES_CANCELRESPONSE]);
    if(cancelCount)
        *cancelCount = cresp.cancelCount;
    UA_StatusCode res = cresp.responseHeader.serviceResult;
    UA_CancelResponse_clear(&cresp);
    return res;
}

UA_StatusCode
UA_Client_cancelByRequestHandle(UA_Client *client, UA_UInt32 requestHandle,
                                UA_UInt32 *cancelCount) {
    UA_LOCK(&client->clientMutex);
    UA_StatusCode res = cancelByRequestHandle(client, requestHandle, cancelCount);
    UA_UNLOCK(&client->clientMutex);
    return res;
}

UA_StatusCode
UA_Client_cancelByRequestId(UA_Client *client, UA_UInt32 requestId,
                            UA_UInt32 *cancelCount) {
    UA_LOCK(&client->clientMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    AsyncServiceCall *ac;
    LIST_FOREACH(ac, &client->asyncServiceCalls, pointers) {
        if(ac->requestId != requestId)
            continue;
        res = cancelByRequestHandle(client, ac->requestHandle, cancelCount);
        break;
    }
    UA_UNLOCK(&client->clientMutex);
    return res;
}

/*******************/
/* Timed Callbacks */
/*******************/

UA_StatusCode
UA_Client_addTimedCallback(UA_Client *client, UA_ClientCallback callback,
                           void *data, UA_DateTime date, UA_UInt64 *callbackId) {
    if(!client->config.eventLoop)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_LOCK(&client->clientMutex);
    UA_StatusCode res = client->config.eventLoop->
        addTimedCallback(client->config.eventLoop, (UA_Callback)callback,
                         client, data, date, callbackId);
    UA_UNLOCK(&client->clientMutex);
    return res;
}

UA_StatusCode
UA_Client_addRepeatedCallback(UA_Client *client, UA_ClientCallback callback,
                              void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    if(!client->config.eventLoop)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_LOCK(&client->clientMutex);
    UA_StatusCode res = client->config.eventLoop->
        addCyclicCallback(client->config.eventLoop, (UA_Callback)callback,
                          client, data, interval_ms, NULL,
                          UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME, callbackId);
    UA_UNLOCK(&client->clientMutex);
    return res;
}

UA_StatusCode
UA_Client_changeRepeatedCallbackInterval(UA_Client *client, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    if(!client->config.eventLoop)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_LOCK(&client->clientMutex);
    UA_StatusCode res = client->config.eventLoop->
        modifyCyclicCallback(client->config.eventLoop, callbackId, interval_ms,
                             NULL, UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME);
    UA_UNLOCK(&client->clientMutex);
    return res;
}

void
UA_Client_removeCallback(UA_Client *client, UA_UInt64 callbackId) {
    if(!client->config.eventLoop)
        return;
    UA_LOCK(&client->clientMutex);
    client->config.eventLoop->
        removeCyclicCallback(client->config.eventLoop, callbackId);
    UA_UNLOCK(&client->clientMutex);
}

/**********************/
/* Housekeeping Tasks */
/**********************/

static void
asyncServiceTimeoutCheck(UA_Client *client) {
    /* Make this function reentrant. One of the async callbacks could indirectly
     * operate on the list. Moving all elements to a local list before iterating
     * that. */
    UA_EventLoop *el = client->config.eventLoop;
    UA_DateTime now = el->dateTime_nowMonotonic(el);
    UA_AsyncServiceList asyncServiceCalls;
    AsyncServiceCall *ac, *ac_tmp;
    LIST_INIT(&asyncServiceCalls);
    LIST_FOREACH_SAFE(ac, &client->asyncServiceCalls, pointers, ac_tmp) {
        if(!ac->timeout)
           continue;
        if(ac->start + (UA_DateTime)(ac->timeout * UA_DATETIME_MSEC) <= now) {
            LIST_REMOVE(ac, pointers);
            LIST_INSERT_HEAD(&asyncServiceCalls, ac, pointers);
        }
    }

    /* Cancel and remove the elements from the local list */
    LIST_FOREACH_SAFE(ac, &asyncServiceCalls, pointers, ac_tmp) {
        LIST_REMOVE(ac, pointers);
        __Client_AsyncService_cancel(client, ac, UA_STATUSCODE_BADTIMEOUT);
    }
}

static void
backgroundConnectivityCallback(UA_Client *client, void *userdata,
                               UA_UInt32 requestId, const UA_ReadResponse *response) {
    UA_LOCK(&client->clientMutex);
    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADTIMEOUT) {
        if(client->config.inactivityCallback) {
            UA_UNLOCK(&client->clientMutex);
            client->config.inactivityCallback(client);
            UA_LOCK(&client->clientMutex);
        }
    }
    UA_EventLoop *el = client->config.eventLoop;
    client->pendingConnectivityCheck = false;
    client->lastConnectivityCheck = el->dateTime_nowMonotonic(el);
    UA_UNLOCK(&client->clientMutex);
}

static void
__Client_backgroundConnectivity(UA_Client *client) {
    if(!client->config.connectivityCheckInterval)
        return;

    if(client->pendingConnectivityCheck)
        return;

    UA_EventLoop *el = client->config.eventLoop;
    UA_DateTime now = el->dateTime_nowMonotonic(el);
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
        __Client_AsyncService(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                              (UA_ClientAsyncServiceCallback)backgroundConnectivityCallback,
                              &UA_TYPES[UA_TYPES_READRESPONSE], NULL, NULL);
    if(retval == UA_STATUSCODE_GOOD)
        client->pendingConnectivityCheck = true;
}

/* Regular housekeeping activities in the client -- called via a cyclic callback */
static void
clientHouseKeeping(UA_Client *client, void *_) {
    UA_LOCK(&client->clientMutex);

    UA_LOG_DEBUG(client->config.logging, UA_LOGCATEGORY_CLIENT,
                 "Internally check the the client state and "
                 "required activities");

    /* Renew Secure Channel */
    __Client_renewSecureChannel(client);

    /* Send read requests from time to time to test the connectivity */
    __Client_backgroundConnectivity(client);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Feed the server PublishRequests for the Subscriptions */
    __Client_Subscriptions_backgroundPublish(client);

    /* Check for inactive Subscriptions */
    __Client_Subscriptions_backgroundPublishInactivityCheck(client);
#endif

    /* Did async services time out? Process callbacks with an error code */
    asyncServiceTimeoutCheck(client);

    /* Log and notify user if the client state has changed */
    notifyClientState(client);

    UA_UNLOCK(&client->clientMutex);
}

UA_StatusCode
__UA_Client_startup(UA_Client *client) {
    UA_LOCK_ASSERT(&client->clientMutex, 1);

    UA_EventLoop *el = client->config.eventLoop;
    UA_CHECK_ERROR(el != NULL,
                   return UA_STATUSCODE_BADINTERNALERROR,
                   client->config.logging, UA_LOGCATEGORY_CLIENT,
                   "No EventLoop configured");

    /* Set up the repeated timer callback for checking the internal state. Like
     * in the public API UA_Client_addRepeatedCallback, but without locking the
     * mutex again */
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    if(!client->houseKeepingCallbackId) {
        rv = el->addCyclicCallback(el, (UA_Callback)clientHouseKeeping,
                                   client, NULL, 1000.0, NULL,
                                   UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
                                   &client->houseKeepingCallbackId);
        UA_CHECK_STATUS(rv, return rv);
    }

    /* Start the EventLoop? */
    if(el->state == UA_EVENTLOOPSTATE_FRESH) {
        rv = el->start(el);
        UA_CHECK_STATUS(rv, return rv);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_run_iterate(UA_Client *client, UA_UInt32 timeout) {
    /* Make sure the EventLoop has been started */
    UA_LOCK(&client->clientMutex);
    UA_StatusCode rv = __UA_Client_startup(client);
    UA_UNLOCK(&client->clientMutex);
    UA_CHECK_STATUS(rv, return rv);

    /* All timers and network events are triggered in the EventLoop. Release the
     * client lock before. The callbacks from the EventLoop take the lock
     * again. */
    UA_EventLoop *el = client->config.eventLoop;
    rv = el->run(el, timeout);
    UA_CHECK_STATUS(rv, return rv);
    return client->connectStatus;
}

const UA_DataType *
UA_Client_findDataType(UA_Client *client, const UA_NodeId *typeId) {
    return UA_findDataTypeWithCustom(typeId, client->config.customDataTypes);
}

/*************************/
/* Connection Attributes */
/*************************/

#define UA_CONNECTIONATTRIBUTESSIZE 3
static const UA_QualifiedName connectionAttributes[UA_CONNECTIONATTRIBUTESSIZE] = {
    {0, UA_STRING_STATIC("serverDescription")},
    {0, UA_STRING_STATIC("securityPolicyUri")},
    {0, UA_STRING_STATIC("securityMode")}
};

static UA_StatusCode
getConnectionttribute(UA_Client *client, const UA_QualifiedName key,
                      UA_Variant *outValue, UA_Boolean copy) {
    if(!outValue)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Variant localAttr;

    if(UA_QualifiedName_equal(&key, &connectionAttributes[0])) {
        /* ServerDescription */
        UA_Variant_setScalar(&localAttr, &client->serverDescription,
                             &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);
    } else if(UA_QualifiedName_equal(&key, &connectionAttributes[1])) {
        /* SecurityPolicyUri */
        const UA_SecurityPolicy *sp = client->channel.securityPolicy;
        if(!sp)
            return UA_STATUSCODE_BADNOTCONNECTED;
        UA_Variant_setScalar(&localAttr, (void*)(uintptr_t)&sp->policyUri,
                             &UA_TYPES[UA_TYPES_STRING]);
    } else if(UA_QualifiedName_equal(&key, &connectionAttributes[2])) {
        /* SecurityMode */
        UA_Variant_setScalar(&localAttr, &client->channel.securityMode,
                             &UA_TYPES[UA_TYPES_MESSAGESECURITYMODE]);
    } else {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(copy)
        return UA_Variant_copy(&localAttr, outValue);

    localAttr.storageType = UA_VARIANT_DATA_NODELETE;
    *outValue = localAttr;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_getConnectionAttribute(UA_Client *client, const UA_QualifiedName key,
                                 UA_Variant *outValue) {
    UA_LOCK(&client->clientMutex);
    UA_StatusCode res = getConnectionttribute(client, key, outValue, false);
    UA_UNLOCK(&client->clientMutex);
    return res;
}

UA_StatusCode
UA_Client_getConnectionAttributeCopy(UA_Client *client, const UA_QualifiedName key,
                                     UA_Variant *outValue) {
    UA_LOCK(&client->clientMutex);
    UA_StatusCode res = getConnectionttribute(client, key, outValue, true);
    UA_UNLOCK(&client->clientMutex);
    return res;
}

UA_StatusCode
UA_Client_getConnectionAttribute_scalar(UA_Client *client,
                                        const UA_QualifiedName key,
                                        const UA_DataType *type,
                                        void *outValue) {
    UA_LOCK(&client->clientMutex);

    UA_Variant attr;
    UA_StatusCode res = getConnectionttribute(client, key, &attr, false);
    if(res != UA_STATUSCODE_GOOD) {
        UA_UNLOCK(&client->clientMutex);
        return res;
    }

    if(!UA_Variant_hasScalarType(&attr, type)) {
        UA_UNLOCK(&client->clientMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    memcpy(outValue, attr.data, type->memSize);

    UA_UNLOCK(&client->clientMutex);
    return UA_STATUSCODE_GOOD;
}
