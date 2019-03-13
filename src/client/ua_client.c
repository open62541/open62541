/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
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

#include "ua_client_internal.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"

#define STATUS_CODE_BAD_POINTER 0x01

/********************/
/* Client Lifecycle */
/********************/

static void
UA_Client_init(UA_Client* client) {
    memset(client, 0, sizeof(UA_Client));
    UA_SecureChannel_init(&client->channel);
    if(client->config.stateCallback)
        client->config.stateCallback(client, client->state);
    /* Catch error during async connection */
    client->connectStatus = UA_STATUSCODE_GOOD;

    UA_Timer_init(&client->timer);
    UA_WorkQueue_init(&client->workQueue);
}

UA_Client *
UA_Client_new() {
    UA_Client *client = (UA_Client*)UA_malloc(sizeof(UA_Client));
    if(!client)
        return NULL;
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

    if(config->certificateVerification.deleteMembers)
        config->certificateVerification.deleteMembers(&config->certificateVerification);

    /* Delete the SecurityPolicies */
    if(config->securityPolicies == 0)
        return;
    for(size_t i = 0; i < config->securityPoliciesSize; i++)
        config->securityPolicies[i].deleteMembers(&config->securityPolicies[i]);
    UA_free(config->securityPolicies);
    config->securityPolicies = 0;
}

static void
UA_Client_deleteMembers(UA_Client *client) {
    UA_Client_disconnect(client);
    /* Commented as UA_SecureChannel_deleteMembers already done
     * in UA_Client_disconnect function */
    //UA_SecureChannel_deleteMembersCleanup(&client->channel);
    if (client->connection.free)
        client->connection.free(&client->connection);
    UA_Connection_deleteMembers(&client->connection);
    UA_NodeId_deleteMembers(&client->authenticationToken);
    UA_String_deleteMembers(&client->endpointUrl);

    /* Delete the async service calls */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

    /* Delete the subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_Subscriptions_clean(client);
#endif

    /* Delete the timed work */
    UA_Timer_deleteMembers(&client->timer);

    /* Clean up the work queue */
    UA_WorkQueue_cleanup(&client->workQueue);

    UA_ClientConfig_deleteMembers(&client->config);
}

void
UA_Client_reset(UA_Client* client) {
    UA_Client_deleteMembers(client);
    UA_Client_init(client);
}

void
UA_Client_delete(UA_Client* client) {
    UA_Client_deleteMembers(client);
    UA_free(client);
}

UA_ClientState
UA_Client_getState(UA_Client *client) {
    return client->state;
}

UA_ClientConfig *
UA_Client_getConfig(UA_Client *client) {
    if(!client)
        return NULL;
    return &client->config;
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
sendSymmetricServiceRequest(UA_Client *client, const void *request,
                            const UA_DataType *requestType, UA_UInt32 *requestId) {
    /* Make sure we have a valid session */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* FIXME: this is just a dirty workaround. We need to rework some of the sync and async processing
     * FIXME: in the client. Currently a lot of stuff is semi broken and in dire need of cleaning up.*/
    /*UA_StatusCode retval = openSecureChannel(client, true);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;*/

    /* Adjusting the request header. The const attribute is violated, but we
     * only touch the following members: */
    UA_RequestHeader *rr = (UA_RequestHeader*)(uintptr_t)request;
    rr->authenticationToken = client->authenticationToken; /* cleaned up at the end */
    rr->timestamp = UA_DateTime_now();
    rr->requestHandle = ++client->requestHandle;

    /* Send the request */
    UA_UInt32 rqId = ++client->requestId;
    UA_LOG_DEBUG(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Sending a request of type %i", requestType->typeId.identifier.numeric);

    if (client->channel.nextSecurityToken.tokenId != 0) // Change to the new security token if the secure channel has been renewed.
        UA_SecureChannel_revolveTokens(&client->channel);
    retval = UA_SecureChannel_sendSymmetricMessage(&client->channel, rqId, UA_MESSAGETYPE_MSG,
                                                   rr, requestType);
    UA_NodeId_init(&rr->authenticationToken); /* Do not return the token to the user */
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    *requestId = rqId;
    return UA_STATUSCODE_GOOD;
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
    if(!ac)
        return UA_STATUSCODE_BADREQUESTHEADERINVALID;

    /* Allocate the response */
    UA_STACKARRAY(UA_Byte, responseBuf, ac->responseType->memSize);
    void *response = (void*)(uintptr_t)&responseBuf[0]; /* workaround aliasing rules */

    /* Verify the type of the response */
    const UA_DataType *responseType = ac->responseType;
    const UA_NodeId expectedNodeId = UA_NODEID_NUMERIC(0, ac->responseType->binaryEncodingId);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!UA_NodeId_equal(responseTypeId, &expectedNodeId)) {
        UA_init(response, ac->responseType);
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
    retval = UA_decodeBinary(responseMessage, offset, response, responseType, client->config.customDataTypes);

 process:
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not decode the response with id %u due to %s",
                    requestId, UA_StatusCode_name(retval));
        ((UA_ResponseHeader*)response)->serviceResult = retval;
    } else if(((UA_ResponseHeader*)response)->serviceResult != UA_STATUSCODE_GOOD) {
        /* Decode as a ServiceFault, i.e. only the response header */
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "The ServiceResult has the StatusCode %s",
                    UA_StatusCode_name(((UA_ResponseHeader*)response)->serviceResult));
    }

    /* Call the callback */
    if(ac->callback)
        ac->callback(client, ac->userdata, requestId, response);
    UA_deleteMembers(response, ac->responseType);

    /* Remove the callback */
    LIST_REMOVE(ac, pointers);
    UA_free(ac);
    return retval;
}

/* Processes the received service response. Either with an async callback or by
 * decoding the message and returning it "upwards" in the
 * SyncResponseDescription. */
static void
processServiceResponse(void *application, UA_SecureChannel *channel,
                       UA_MessageType messageType, UA_UInt32 requestId,
                       const UA_ByteString *message) {
    SyncResponseDescription *rd = (SyncResponseDescription*)application;

    /* Must be OPN or MSG */
    if(messageType != UA_MESSAGETYPE_OPN &&
       messageType != UA_MESSAGETYPE_MSG) {
        UA_LOG_TRACE_CHANNEL(&rd->client->config.logger, channel,
                             "Invalid message type");
        return;
    }

    /* Forward declaration for the goto */
    UA_NodeId expectedNodeId = UA_NODEID_NULL;

    /* Decode the data type identifier of the response */
    size_t offset = 0;
    UA_NodeId responseId;
    UA_StatusCode retval = UA_NodeId_decodeBinary(message, &offset, &responseId);
    if(retval != UA_STATUSCODE_GOOD)
        goto finish;

    /* Got an asynchronous response. Don't expected a synchronous response
     * (responseType NULL) or the id does not match. */
    if(!rd->responseType || requestId != rd->requestId) {
        retval = processAsyncResponse(rd->client, requestId, &responseId, message, &offset);
        goto finish;
    }

    /* Got the synchronous response */
    rd->received = true;

    /* Check that the response type matches */
    expectedNodeId = UA_NODEID_NUMERIC(0, rd->responseType->binaryEncodingId);
    if(!UA_NodeId_equal(&responseId, &expectedNodeId)) {
        if(UA_NodeId_equal(&responseId, &serviceFaultId)) {
            UA_init(rd->response, rd->responseType);
            retval = UA_decodeBinary(message, &offset, rd->response,
                                     &UA_TYPES[UA_TYPES_SERVICEFAULT],
                                     rd->client->config.customDataTypes);
            if(retval != UA_STATUSCODE_GOOD)
                ((UA_ResponseHeader*)rd->response)->serviceResult = retval;
            UA_LOG_INFO(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                        "Received a ServiceFault response with StatusCode %s",
                        UA_StatusCode_name(((UA_ResponseHeader*)rd->response)->serviceResult));
        } else {
            /* Close the connection */
            UA_LOG_ERROR(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Reply contains the wrong service response");
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
        }
        goto finish;
    }

#ifdef UA_ENABLE_TYPENAMES
    UA_LOG_DEBUG(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %s", rd->responseType->typeName);
#else
    UA_LOG_DEBUG(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %u", responseId.identifier.numeric);
#endif

    /* Decode the response */
    retval = UA_decodeBinary(message, &offset, rd->response, rd->responseType,
                             rd->client->config.customDataTypes);

finish:
    UA_NodeId_deleteMembers(&responseId);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            retval = UA_STATUSCODE_BADRESPONSETOOLARGE;
        UA_LOG_INFO(&rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Error receiving the response with status code %s",
                    UA_StatusCode_name(retval));

        if(rd->response) {
            UA_ResponseHeader *respHeader = (UA_ResponseHeader*)rd->response;
            respHeader->serviceResult = retval;
        }
    }
}

/* Forward complete chunks directly to the securechannel */
static UA_StatusCode
client_processChunk(void *application, UA_Connection *connection, UA_ByteString *chunk) {
    SyncResponseDescription *rd = (SyncResponseDescription*)application;
    UA_StatusCode retval = UA_SecureChannel_decryptAddChunk(&rd->client->channel, chunk, true);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    return UA_SecureChannel_persistIncompleteMessages(&rd->client->channel);
}

/* Receive and process messages until a synchronous message arrives or the
 * timout finishes */
UA_StatusCode
receiveServiceResponse(UA_Client *client, void *response, const UA_DataType *responseType,
                       UA_DateTime maxDate, const UA_UInt32 *synchronousRequestId) {
    /* Prepare the response and the structure we give into processServiceResponse */
    SyncResponseDescription rd = { client, false, 0, response, responseType };

    /* Return upon receiving the synchronized response. All other responses are
     * processed with a callback "in the background". */
    if(synchronousRequestId)
        rd.requestId = *synchronousRequestId;

    UA_StatusCode retval;
    do {
        UA_DateTime now = UA_DateTime_nowMonotonic();

        /* >= avoid timeout to be set to 0 */
        if(now >= maxDate)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;

        /* round always to upper value to avoid timeout to be set to 0
         * if(maxDate - now) < (UA_DATETIME_MSEC/2) */
        UA_UInt32 timeout = (UA_UInt32)(((maxDate - now) + (UA_DATETIME_MSEC - 1)) / UA_DATETIME_MSEC);

        retval = UA_Connection_receiveChunksBlocking(&client->connection, &rd, client_processChunk, timeout);
        UA_SecureChannel_processCompleteMessages(&client->channel, &rd, processServiceResponse);

        if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
            if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
                setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
            UA_Client_disconnect(client);
            break;
        }
    } while(!rd.received);
    return retval;
}

void
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType) {
    UA_init(response, responseType);
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;

    /* Send the request */
    UA_UInt32 requestId;
    UA_StatusCode retval = sendSymmetricServiceRequest(client, request, requestType, &requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            respHeader->serviceResult = UA_STATUSCODE_BADREQUESTTOOLARGE;
        else
            respHeader->serviceResult = retval;
        UA_Client_disconnect(client);
        return;
    }

    /* Retrieve the response */
    UA_DateTime maxDate = UA_DateTime_nowMonotonic() +
        (client->config.timeout * UA_DATETIME_MSEC);
    retval = receiveServiceResponse(client, response, responseType, maxDate, &requestId);
    if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
        /* In synchronous service, if we have don't have a reply we need to close the connection */
        UA_Client_disconnect(client);
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    if(retval != UA_STATUSCODE_GOOD)
        respHeader->serviceResult = retval;
}

UA_StatusCode
receiveServiceResponseAsync(UA_Client *client, void *response,
                             const UA_DataType *responseType) {
    SyncResponseDescription rd = { client, false, 0, response, responseType };

    UA_StatusCode retval = UA_Connection_receiveChunksNonBlocking(
            &client->connection, &rd, client_processChunk);
    UA_SecureChannel_processCompleteMessages(&client->channel, &rd, processServiceResponse);
    /*let client run when non critical timeout*/
    if(retval != UA_STATUSCODE_GOOD
            && retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
            setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
        }
        UA_Client_disconnect(client);
    }
    return retval;
}

UA_StatusCode
receivePacketAsync(UA_Client *client) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if (UA_Client_getState(client) == UA_CLIENTSTATE_DISCONNECTED ||
            UA_Client_getState(client) == UA_CLIENTSTATE_WAITING_FOR_ACK) {
        retval = UA_Connection_receiveChunksNonBlocking(&client->connection, client, processACKResponseAsync);
    }
    else if(UA_Client_getState(client) == UA_CLIENTSTATE_CONNECTED) {
        retval = UA_Connection_receiveChunksNonBlocking(&client->connection, client, processOPNResponseAsync);
    }
    if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
            setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
        UA_Client_disconnect(client);
    }
    return retval;
}

void
UA_Client_AsyncService_cancel(UA_Client *client, AsyncServiceCall *ac,
                              UA_StatusCode statusCode) {
    /* Create an empty response with the statuscode */
    UA_STACKARRAY(UA_Byte, responseBuf, ac->responseType->memSize);
    void *resp = (void*)(uintptr_t)&responseBuf[0]; /* workaround aliasing rules */
    UA_init(resp, ac->responseType);
    ((UA_ResponseHeader*)resp)->serviceResult = statusCode;

    if(ac->callback)
        ac->callback(client, ac->userdata, ac->requestId, resp);

    /* Clean up the response. Users might move data into it. For whatever reasons. */
    UA_deleteMembers(resp, ac->responseType);
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
    /* Prepare the entry for the linked list */
    AsyncServiceCall *ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
    if(!ac)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ac->callback = callback;
    ac->responseType = responseType;
    ac->userdata = userdata;
    ac->timeout = timeout;

    /* Call the service and set the requestId */
    UA_StatusCode retval = sendSymmetricServiceRequest(client, request, requestType, &ac->requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(ac);
        return retval;
    }

    ac->start = UA_DateTime_nowMonotonic();

    /* Store the entry for async processing */
    LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);
    if(requestId)
        *requestId = ac->requestId;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
__UA_Client_AsyncService(UA_Client *client, const void *request,
                         const UA_DataType *requestType,
                         UA_ClientAsyncServiceCallback callback,
                         const UA_DataType *responseType,
                         void *userdata, UA_UInt32 *requestId) {
    return __UA_Client_AsyncServiceEx(client, request, requestType, callback,
                                      responseType, userdata, requestId,
                                      client->config.timeout);
}


UA_StatusCode
UA_Client_sendAsyncRequest(UA_Client *client, const void *request,
                           const UA_DataType *requestType,
                           UA_ClientAsyncServiceCallback callback,
                           const UA_DataType *responseType, void *userdata,
                           UA_UInt32 *requestId) {
    if (UA_Client_getState(client) < UA_CLIENTSTATE_SECURECHANNEL) {
        UA_LOG_INFO(&client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Cient must be connected to send high-level requests");
        return UA_STATUSCODE_GOOD;
    }
    return __UA_Client_AsyncService(client, request, requestType, callback,
                                    responseType, userdata, requestId);
}

UA_StatusCode UA_EXPORT
UA_Client_addTimedCallback(UA_Client *client, UA_ClientCallback callback,
                           void *data, UA_DateTime date, UA_UInt64 *callbackId) {
    return UA_Timer_addTimedCallback(&client->timer, (UA_ApplicationCallback) callback,
                                     client, data, date, callbackId);
}

UA_StatusCode
UA_Client_addRepeatedCallback(UA_Client *client, UA_ClientCallback callback,
                              void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&client->timer, (UA_ApplicationCallback) callback,
                                        client, data, interval_ms, callbackId);
}

UA_StatusCode
UA_Client_changeRepeatedCallbackInterval(UA_Client *client, UA_UInt64 callbackId,
                                         UA_Double interval_ms) {
    return UA_Timer_changeRepeatedCallbackInterval(&client->timer, callbackId,
                                                   interval_ms);
}

void
UA_Client_removeCallback(UA_Client *client, UA_UInt64 callbackId) {
    UA_Timer_removeCallback(&client->timer, callbackId);
}
