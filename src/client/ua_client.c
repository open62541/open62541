/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2015-2017 (c) Julius Pfrommer, Fraunhofer IOSB
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
 */

#include "ua_client.h"
#include "ua_client_internal.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_util.h"
#include "ua_securitypolicy_none.h"

/********************/
/* Client Lifecycle */
/********************/

static void
UA_Client_init(UA_Client* client, UA_ClientConfig config) {
    memset(client, 0, sizeof(UA_Client));
    /* TODO: Select policy according to the endpoint */
    UA_SecurityPolicy_None(&client->securityPolicy, NULL, UA_BYTESTRING_NULL, config.logger);
    client->channel.securityPolicy = &client->securityPolicy;
    client->channel.securityMode = UA_MESSAGESECURITYMODE_NONE;
    client->config = config;
    if(client->config.stateCallback)
        client->config.stateCallback(client, client->state);
}

UA_Client *
UA_Client_new(UA_ClientConfig config) {
    UA_Client *client = (UA_Client*)UA_malloc(sizeof(UA_Client));
    if(!client)
        return NULL;
    UA_Client_init(client, config);
    return client;
}

static void
UA_Client_deleteMembers(UA_Client* client) {
    UA_Client_disconnect(client);
    client->securityPolicy.deleteMembers(&client->securityPolicy);
    UA_SecureChannel_deleteMembersCleanup(&client->channel);
    UA_Connection_deleteMembers(&client->connection);
    if(client->endpointUrl.data)
        UA_String_deleteMembers(&client->endpointUrl);
    UA_UserTokenPolicy_deleteMembers(&client->token);
    UA_NodeId_deleteMembers(&client->authenticationToken);
    if(client->username.data)
        UA_String_deleteMembers(&client->username);
    if(client->password.data)
        UA_String_deleteMembers(&client->password);

    /* Delete the async service calls */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

    /* Delete the subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_Subscriptions_clean(client);
#endif
}

void
UA_Client_reset(UA_Client* client) {
    UA_Client_deleteMembers(client);
    UA_Client_init(client, client->config);
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

void *
UA_Client_getContext(UA_Client *client) {
    if(!client)
        return NULL;
    return client->config.clientContext;
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
    UA_StatusCode retval;

    /* If a message is pending in the chunk don't call UA_Client_manuallyRenewSecureChannel
    * to prevent incomming message desynchronization */
    if(!client->connection.pendingMessage) {
        /* Make sure we have a valid session */
        retval = UA_Client_manuallyRenewSecureChannel(client);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Adjusting the request header. The const attribute is violated, but we
     * only touch the following members: */
    UA_RequestHeader *rr = (UA_RequestHeader*)(uintptr_t)request;
    rr->authenticationToken = client->authenticationToken; /* cleaned up at the end */
    rr->timestamp = UA_DateTime_now();
    rr->requestHandle = ++client->requestHandle;

    /* Send the request */
    UA_UInt32 rqId = ++client->requestId;
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
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
            UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Received a ServiceFault response");
            responseType = &UA_TYPES[UA_TYPES_SERVICEFAULT];
        } else {
            /* Close the connection */
            UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Reply contains the wrong service response");
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
            goto process;
        }
    }

    /* Decode the response */
    retval = UA_decodeBinary(responseMessage, offset, response,
                             responseType, 0, NULL);

 process:
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not decode the response with id %u due to %s",
                    requestId, UA_StatusCode_name(retval));
        ((UA_ResponseHeader*)response)->serviceResult = retval;
    }

    /* Call the callback */
    if (ac->callback)
        ac->callback(client, ac->userdata, requestId, response, ac->responseType);
    UA_deleteMembers(response, ac->responseType);

    /* Remove the callback */
    LIST_REMOVE(ac, pointers);
    UA_free(ac);
    return retval;
}

/* Processes the received service response. Either with an async callback or by
 * decoding the message and returning it "upwards" in the
 * SyncResponseDescription. */
static UA_StatusCode
processServiceResponse(void *application, UA_SecureChannel *channel,
                       UA_MessageType messageType, UA_UInt32 requestId,
                       const UA_ByteString *message) {
    SyncResponseDescription *rd = (SyncResponseDescription*)application;

    /* Must be OPN or MSG */
    if(messageType != UA_MESSAGETYPE_OPN &&
       messageType != UA_MESSAGETYPE_MSG) {
        UA_LOG_TRACE_CHANNEL(rd->client->config.logger, channel,
                             "Invalid message type");
        return UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
    }

    /* Has the SecureChannel timed out?
     * TODO: Solve this for client and server together */
    if(rd->client->state >= UA_CLIENTSTATE_SECURECHANNEL &&
       (channel->securityToken.createdAt +
        (channel->securityToken.revisedLifetime * UA_DATETIME_MSEC))
       < UA_DateTime_nowMonotonic())
        return UA_STATUSCODE_BADSECURECHANNELCLOSED;

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

    /* Forward declaration for the goto */
    UA_NodeId expectedNodeId = UA_NODEID_NUMERIC(0, rd->responseType->binaryEncodingId);

    /* Check that the response type matches */
    if(!UA_NodeId_equal(&responseId, &expectedNodeId)) {
        if(UA_NodeId_equal(&responseId, &serviceFaultId)) {
            UA_LOG_INFO(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Received a ServiceFault response");
            UA_init(rd->response, rd->responseType);
            retval = UA_decodeBinary(message, &offset, rd->response,
                                     &UA_TYPES[UA_TYPES_SERVICEFAULT], 0, NULL);
        } else {
            /* Close the connection */
            UA_LOG_ERROR(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Reply contains the wrong service response");
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
        }
        goto finish;
    }

    UA_LOG_DEBUG(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Decode a message of type %u", responseId.identifier.numeric);

    /* Decode the response */
    retval = UA_decodeBinary(message, &offset, rd->response, rd->responseType,
                             rd->client->config.customDataTypesSize,
                             rd->client->config.customDataTypes);

finish:
    UA_NodeId_deleteMembers(&responseId);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            retval = UA_STATUSCODE_BADRESPONSETOOLARGE;
        UA_LOG_INFO(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Error receiving the response with status code %s",
                    UA_StatusCode_name(retval));

        if(rd->response) {
            UA_ResponseHeader *respHeader = (UA_ResponseHeader*)rd->response;
            respHeader->serviceResult = retval;
        }
    }
    return retval;
}

/* Forward complete chunks directly to the securechannel */
static UA_StatusCode
client_processChunk(void *application, UA_Connection *connection, UA_ByteString *chunk) {
    SyncResponseDescription *rd = (SyncResponseDescription*)application;
    return UA_SecureChannel_processChunk(&rd->client->channel, chunk,
                                         processServiceResponse,
                                         rd, UA_TRUE);
}

/* Receive and process messages until a synchronous message arrives or the
 * timout finishes */
UA_StatusCode
receiveServiceResponse(UA_Client *client, void *response, const UA_DataType *responseType,
                       UA_DateTime maxDate, UA_UInt32 *synchronousRequestId) {
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

        if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
            if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
                setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
            UA_Client_close(client);
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
        UA_Client_close(client);
        return;
    }

    /* Retrieve the response */
    UA_DateTime maxDate = UA_DateTime_nowMonotonic() +
        (client->config.timeout * UA_DATETIME_MSEC);
    retval = receiveServiceResponse(client, response, responseType, maxDate, &requestId);
    if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT) {
        /* In synchronous service, if we have don't have a reply we need to close the connection */
        UA_Client_close(client);
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    if(retval != UA_STATUSCODE_GOOD)
        respHeader->serviceResult = retval;
}

void
UA_Client_AsyncService_cancel(UA_Client *client, AsyncServiceCall *ac,
                              UA_StatusCode statusCode) {
    /* Create an empty response with the statuscode */
    UA_STACKARRAY(UA_Byte, responseBuf, ac->responseType->memSize);
    void *resp = (void*)(uintptr_t)&responseBuf[0]; /* workaround aliasing rules */
    UA_init(resp, ac->responseType);
    ((UA_ResponseHeader*)resp)->serviceResult = statusCode;

    if (ac->callback)
        ac->callback(client, ac->userdata, ac->requestId, resp, ac->responseType);

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
        ac->requestId = 0;
        UA_Client_AsyncService_cancel(client, ac, UA_STATUSCODE_BADTIMEOUT);
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

static void
backgroundConnectivityCallback(UA_Client *client, void *userdata,
                               UA_UInt32 requestId, const UA_ReadResponse *response,
                               const UA_DataType *responseType) {
    if(response->responseHeader.serviceResult == UA_STATUSCODE_BADTIMEOUT) {
        if (client->config.inactivityCallback)
            client->config.inactivityCallback(client);
    }
    client->pendingConnectivityCheck = false;
    client->lastConnectivityCheck = UA_DateTime_nowMonotonic();
}

static UA_StatusCode
UA_Client_backgroundConnectivity(UA_Client *client) {
    if(!client->config.connectivityCheckInterval)
        return UA_STATUSCODE_GOOD;

    if (client->pendingConnectivityCheck)
        return UA_STATUSCODE_GOOD;

    UA_DateTime now = UA_DateTime_nowMonotonic();
    UA_DateTime nextDate = client->lastConnectivityCheck + (UA_DateTime)(client->config.connectivityCheckInterval * UA_DATETIME_MSEC);

    if(now <= nextDate)
        return UA_STATUSCODE_GOOD;

    UA_ReadRequest request;
    UA_ReadRequest_init(&request);

    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.attributeId = UA_ATTRIBUTEID_VALUE;
    rvid.nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);

    request.nodesToRead = &rvid;
    request.nodesToReadSize = 1;

    UA_StatusCode retval = __UA_Client_AsyncService(client, &request, &UA_TYPES[UA_TYPES_READREQUEST],
                                                    (UA_ClientAsyncServiceCallback)backgroundConnectivityCallback,
                                                    &UA_TYPES[UA_TYPES_READRESPONSE], NULL, NULL);

    client->pendingConnectivityCheck = true;

    return retval;
}

static void
asyncServiceTimeoutCheck(UA_Client *client) {
    UA_DateTime now = UA_DateTime_nowMonotonic();

    /* Timeout occurs, remove the callback */
    AsyncServiceCall *ac, *ac_tmp;
    LIST_FOREACH_SAFE(ac, &client->asyncServiceCalls, pointers, ac_tmp) {
        if (!ac->timeout)
            continue;

        if (ac->start + (UA_DateTime)(ac->timeout * UA_DATETIME_MSEC) <= now) {
            LIST_REMOVE(ac, pointers);
            UA_Client_AsyncService_cancel(client, ac, UA_STATUSCODE_BADTIMEOUT);
            UA_free(ac);
        }
    }
}

UA_StatusCode
UA_Client_runAsync(UA_Client *client, UA_UInt16 timeout) {
    /* TODO: Call repeated jobs that are scheduled */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_StatusCode retvalPublish = UA_Client_Subscriptions_backgroundPublish(client);
    if (retvalPublish != UA_STATUSCODE_GOOD)
        return retvalPublish;
#endif
    UA_StatusCode retval = UA_Client_manuallyRenewSecureChannel(client);
    if (retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_Client_backgroundConnectivity(client);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_DateTime maxDate = UA_DateTime_nowMonotonic() + (timeout * UA_DATETIME_MSEC);
    retval = receiveServiceResponse(client, NULL, NULL, maxDate, NULL);
    if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT)
        retval = UA_STATUSCODE_GOOD;
#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* The inactivity check must be done after receiveServiceResponse */
    UA_Client_Subscriptions_backgroundPublishInactivityCheck(client);
#endif
    asyncServiceTimeoutCheck(client);
    return retval;
}
