/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
    UA_SecurityPolicy_None(&client->securityPolicy, UA_BYTESTRING_NULL, config.logger);
    client->channel.securityPolicy = &client->securityPolicy;
    client->channel.securityMode = UA_MESSAGESECURITYMODE_NONE;
    client->config = config;
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
    AsyncServiceCall *ac, *ac_tmp;
    LIST_FOREACH_SAFE(ac, &client->asyncServiceCalls, pointers, ac_tmp) {
        LIST_REMOVE(ac, pointers);
        UA_free(ac);
    }

    /* Delete the subscriptions */
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_NotificationsAckNumber *n, *tmp;
    LIST_FOREACH_SAFE(n, &client->pendingNotificationsAcks, listEntry, tmp) {
        LIST_REMOVE(n, listEntry);
        UA_free(n);
    }
    UA_Client_Subscription *sub, *tmps;
    LIST_FOREACH_SAFE(sub, &client->subscriptions, listEntry, tmps)
        UA_Client_Subscriptions_forceDelete(client, sub); /* force local removal */
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
    UA_StatusCode retval = UA_Client_manuallyRenewSecureChannel(client);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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
    retval = UA_SecureChannel_sendSymmetricMessage(&client->channel, rqId, UA_MESSAGETYPE_MSG,
                                                   rr, requestType);
    UA_NodeId_init(&rr->authenticationToken); /* Do not return the token to the user */
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    *requestId = rqId;
    return UA_STATUSCODE_GOOD;
}

/* Look for the async callback in the linked list, execute and delete it */
static UA_StatusCode
processAsyncResponse(UA_Client *client, UA_UInt32 requestId, UA_NodeId *responseTypeId,
                     const UA_ByteString *responseMessage, size_t *offset) {
    /* Find the callback */
    AsyncServiceCall *ac;
    LIST_FOREACH(ac, &client->asyncServiceCalls, pointers) {
        if(ac->requestId == requestId)
            break;
    }
    if(!ac)
        return UA_STATUSCODE_BADREQUESTHEADERINVALID;

    /* Decode the response */
    void *response = UA_alloca(ac->responseType->memSize);
    UA_StatusCode retval = UA_decodeBinary(responseMessage, offset, response,
                                           ac->responseType, 0, NULL);

    /* Call the callback */
    if(retval == UA_STATUSCODE_GOOD) {
        ac->callback(client, ac->userdata, requestId, response);
        UA_deleteMembers(response, ac->responseType);
    } else {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Could not decodee the response with Id %u", requestId);
    }

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
        (channel->securityToken.revisedLifetime * UA_MSEC_TO_DATETIME))
       < UA_DateTime_nowMonotonic())
        return UA_STATUSCODE_BADSECURECHANNELCLOSED;

    /* Forward declaration for the goto */
    UA_NodeId expectedNodeId;
    const UA_NodeId serviceFaultNodeId =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_SERVICEFAULT].binaryEncodingId);

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
    if(UA_NodeId_equal(&responseId, &expectedNodeId)) {
        /* Decode the response */
        retval = UA_decodeBinary(message, &offset, rd->response, rd->responseType,
                                 rd->client->config.customDataTypesSize,
                                 rd->client->config.customDataTypes);
    } else {
        UA_LOG_ERROR(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Reply contains the wrong service response");
        if(UA_NodeId_equal(&responseId, &serviceFaultNodeId)) {
            /* Decode only the message header with the servicefault */
            retval = UA_decodeBinary(message, &offset, rd->response,
                                     &UA_TYPES[UA_TYPES_SERVICEFAULT], 0, NULL);
        } else {
            /* Close the connection */
            retval = UA_STATUSCODE_BADCOMMUNICATIONERROR;
        }
    }


finish:
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Received a response of type %i", responseId.identifier.numeric);
    } else {
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
    UA_NodeId_deleteMembers(&responseId);

    return retval;
}

/* Forward complete chunks directly to the securechannel */
static UA_StatusCode
client_processChunk(void *application, UA_Connection *connection, UA_ByteString *chunk) {
    SyncResponseDescription *rd = (SyncResponseDescription*)application;
    return UA_SecureChannel_processChunk(&rd->client->channel, chunk,
                                         processServiceResponse,
                                         rd);
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
         * if (maxDate - now) < (UA_MSEC_TO_DATETIME/2) */
        UA_UInt32 timeout = (UA_UInt32)(((maxDate - now) + (UA_MSEC_TO_DATETIME - 1)) / UA_MSEC_TO_DATETIME);

        retval = UA_Connection_receiveChunksBlocking(&client->connection, &rd,
                                                     client_processChunk, timeout);

        if (retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT)
            break;

        if(retval != UA_STATUSCODE_GOOD) {
            if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
                client->state = UA_CLIENTSTATE_DISCONNECTED;
            else
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
        (client->config.timeout * UA_MSEC_TO_DATETIME);
    retval = receiveServiceResponse(client, response, responseType, maxDate, &requestId);
    if (retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT){
        /* In synchronous service, if we have don't have a reply we need to close the connection */
        UA_Client_disconnect(client);
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    if(retval != UA_STATUSCODE_GOOD)
        respHeader->serviceResult = retval;
}

UA_StatusCode
__UA_Client_AsyncService(UA_Client *client, const void *request,
                         const UA_DataType *requestType,
                         UA_ClientAsyncServiceCallback callback,
                         const UA_DataType *responseType,
                         void *userdata, UA_UInt32 *requestId) {
    /* Prepare the entry for the linked list */
    AsyncServiceCall *ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
    if(!ac)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ac->callback = callback;
    ac->responseType = responseType;
    ac->userdata = userdata;

    /* Call the service and set the requestId */
    UA_StatusCode retval = sendSymmetricServiceRequest(client, request, requestType, &ac->requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(ac);
        return retval;
    }

    /* Store the entry for async processing */
    LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);
    if(requestId)
        *requestId = ac->requestId;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Client_runAsync(UA_Client *client, UA_UInt16 timeout) {
    /* TODO: Call repeated jobs that are scheduled */
    UA_DateTime maxDate = UA_DateTime_nowMonotonic() +
        (timeout * UA_MSEC_TO_DATETIME);
    UA_StatusCode retval = receiveServiceResponse(client, NULL, NULL, maxDate, NULL);
    if(retval == UA_STATUSCODE_GOODNONCRITICALTIMEOUT)
        retval = UA_STATUSCODE_GOOD;
    return retval;
}
