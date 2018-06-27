/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_client.h"
#include "ua_client_internal.h"
#include "ua_transport_generated.h"
#include "ua_transport_generated_handling.h"
#include "ua_transport_generated_encoding_binary.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"

#define UA_MINMESSAGESIZE                8192
#define UA_SESSION_LOCALNONCELENGTH      32
#define MAX_DATA_SIZE 4096

/* Asynchronous client connection
 * To prepare an async connection, UA_Client_connectAsync() is called, which does not connect the
 * client directly. UA_Client_run_iterate() takes care of actually connecting the client:
 * if client is disconnected:
 *      send hello msg and set the client state to be WAITING_FOR_ACK
 *      (see UA_Client_connect_iterate())
 * if client is waiting for the ACK:
 *      call the non-blocking receiving function and register processACKResponseAsync() as its callback
 *      (see receivePacketAsync())
 * if ACK is processed (callback called):
 *      processACKResponseAsync() calls openSecureChannelAsync() at the end, which prepares the request
 *      to open secure channel and the client is connected
 * if client is connected:
 *      call the non-blocking receiving function and register processOPNResponse() as its callback
 *      (see receivePacketAsync())
 * if OPN-request processed (callback called)
 *      send session request, where the session response is put into a normal AsyncServiceCall, and when
 *      called, request to activate session is sent, where its response is again put into an AsyncServiceCall
 * in the very last step responseActivateSession():
 *      the user defined callback that is passed into UA_Client_connectAsync() is called and the
 *      async connection finalized.
 * */

/***********************/
/* Open the Connection */
/***********************/
static UA_StatusCode
openSecureChannelAsync(UA_Client *client/*, UA_Boolean renew*/);

static UA_StatusCode
requestSession(UA_Client *client, UA_UInt32 *requestId);

static UA_StatusCode
requestGetEndpoints(UA_Client *client, UA_UInt32 *requestId);

/*receives hello ack, opens secure channel*/
static UA_StatusCode
processACKResponseAsync(void *application, UA_Connection *connection,
                         UA_ByteString *chunk) {
    UA_Client *client = (UA_Client*)application;

    /* Decode the message */
    size_t offset = 0;
    UA_TcpMessageHeader messageHeader;
    UA_TcpAcknowledgeMessage ackMessage;
    client->connectStatus = UA_TcpMessageHeader_decodeBinary (chunk, &offset,
                                                              &messageHeader);
    client->connectStatus |= UA_TcpAcknowledgeMessage_decodeBinary(
            chunk, &offset, &ackMessage);
    if (client->connectStatus != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Decoding ACK message failed");
        return client->connectStatus;
    }

    /* Store remote connection settings and adjust local configuration to not
     * exceed the limits */
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Received ACK message");
    connection->remoteConf.maxChunkCount = ackMessage.maxChunkCount; /* may be zero -> unlimited */
    connection->remoteConf.maxMessageSize = ackMessage.maxMessageSize; /* may be zero -> unlimited */
    connection->remoteConf.protocolVersion = ackMessage.protocolVersion;
    connection->remoteConf.sendBufferSize = ackMessage.sendBufferSize;
    connection->remoteConf.recvBufferSize = ackMessage.receiveBufferSize;
    if (connection->remoteConf.recvBufferSize
            < connection->localConf.sendBufferSize)
        connection->localConf.sendBufferSize =
                connection->remoteConf.recvBufferSize;
    if (connection->remoteConf.sendBufferSize
            < connection->localConf.recvBufferSize)
        connection->localConf.recvBufferSize =
                connection->remoteConf.sendBufferSize;
    connection->state = UA_CONNECTION_ESTABLISHED;

    client->state = UA_CLIENTSTATE_CONNECTED;

    /* Open a SecureChannel. TODO: Select with endpoint  */
    client->channel.connection = &client->connection;
    client->connectStatus = openSecureChannelAsync(client/*, false*/);
    return client->connectStatus;
}

static UA_StatusCode
sendHELMessage(UA_Client *client) {
    /* Get a buffer */
    UA_ByteString message;
    UA_Connection *conn = &client->connection;
    client->connectStatus = conn->getSendBuffer(conn, UA_MINMESSAGESIZE,
                                                &message);

    if (client->connectStatus != UA_STATUSCODE_GOOD)
        return client->connectStatus;

    /* Prepare the HEL message and encode at offset 8 */
    UA_TcpHelloMessage hello;
    UA_String_copy(&client->endpointUrl, &hello.endpointUrl); /* must be less than 4096 bytes */
    hello.maxChunkCount = conn->localConf.maxChunkCount;
    hello.maxMessageSize = conn->localConf.maxMessageSize;
    hello.protocolVersion = conn->localConf.protocolVersion;
    hello.receiveBufferSize = conn->localConf.recvBufferSize;
    hello.sendBufferSize = conn->localConf.sendBufferSize;

    UA_Byte *bufPos = &message.data[8]; /* skip the header */
    const UA_Byte *bufEnd = &message.data[message.length];
    client->connectStatus = UA_TcpHelloMessage_encodeBinary(&hello, &bufPos,
                                                            bufEnd);
    UA_TcpHelloMessage_deleteMembers (&hello);

    /* Encode the message header at offset 0 */
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_CHUNKTYPE_FINAL
            + UA_MESSAGETYPE_HEL;
    messageHeader.messageSize = (UA_UInt32) ((uintptr_t)bufPos
            - (uintptr_t)message.data);
    bufPos = message.data;
    client->connectStatus |= UA_TcpMessageHeader_encodeBinary(&messageHeader,
                                                              &bufPos,
                                                              bufEnd);
    if (client->connectStatus != UA_STATUSCODE_GOOD) {
        conn->releaseSendBuffer(conn, &message);
        return client->connectStatus;
    }

    /* Send the HEL message */
    message.length = messageHeader.messageSize;
    client->connectStatus = conn->send (conn, &message);

    if (client->connectStatus != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Sending HEL failed");
        return client->connectStatus;
    }
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Sent HEL message");
    setClientState(client, UA_CLIENTSTATE_WAITING_FOR_ACK);
    return client->connectStatus;
}

static UA_StatusCode
processDecodedOPNResponseAsync(void *application, UA_SecureChannel *channel,
                                UA_MessageType messageType,
                                UA_UInt32 requestId,
                                const UA_ByteString *message) {
    /* Does the request id match? */
    UA_Client *client = (UA_Client*)application;
    if (requestId != client->requestId)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;

    /* Is the content of the expected type? */
    size_t offset = 0;
    UA_NodeId responseId;
    UA_NodeId expectedId = UA_NODEID_NUMERIC(
            0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
    UA_StatusCode retval = UA_NodeId_decodeBinary(message, &offset,
                                                  &responseId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(!UA_NodeId_equal(&responseId, &expectedId)) {
        UA_NodeId_deleteMembers(&responseId);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    UA_NodeId_deleteMembers (&responseId);

    /* Decode the response */
    UA_OpenSecureChannelResponse response;
    retval = UA_OpenSecureChannelResponse_decodeBinary(message, &offset,
                                                       &response);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Response.securityToken.revisedLifetime is UInt32 we need to cast it to
     * DateTime=Int64 we take 75% of lifetime to start renewing as described in
     * standard */
    client->nextChannelRenewal = UA_DateTime_nowMonotonic()
            + (UA_DateTime) (response.securityToken.revisedLifetime
                    * (UA_Double) UA_DATETIME_MSEC * 0.75);

    /* Replace the token and nonce */
    UA_ChannelSecurityToken_deleteMembers(&client->channel.securityToken);
    UA_ByteString_deleteMembers(&client->channel.remoteNonce);
    client->channel.securityToken = response.securityToken;
    client->channel.remoteNonce = response.serverNonce;
    UA_ResponseHeader_deleteMembers(&response.responseHeader); /* the other members were moved */
    if(client->channel.state == UA_SECURECHANNELSTATE_OPEN)
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "SecureChannel renewed");
    else
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "SecureChannel opened");
    client->channel.state = UA_SECURECHANNELSTATE_OPEN;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode processOPNResponse
    (void *application, UA_Connection *connection,
                    UA_ByteString *chunk) {
    UA_Client *client = (UA_Client*) application;
    UA_StatusCode retval = UA_SecureChannel_processChunk (
            &client->channel, chunk, processDecodedOPNResponseAsync, client);
    client->connectStatus = retval;
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }
    setClientState(client, UA_CLIENTSTATE_SECURECHANNEL);
    retval |= UA_SecureChannel_generateNewKeys(&client->channel);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    /* Following requests and responses */
    UA_UInt32 reqId;
    if(client->endpointsHandshake)
        retval = requestGetEndpoints (client, &reqId);
    else
        retval = requestSession (client, &reqId);

    client->connectStatus = retval;
    return retval;

}

/* OPN messges to renew the channel are sent asynchronous */
static UA_StatusCode
openSecureChannelAsync(UA_Client *client/*, UA_Boolean renew*/) {
    /* Check if sc is still valid */
    /*if(renew && client->nextChannelRenewal - UA_DateTime_nowMonotonic () > 0)
        return UA_STATUSCODE_GOOD;*/

    UA_Connection *conn = &client->connection;
    if(conn->state != UA_CONNECTION_ESTABLISHED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    /* Prepare the OpenSecureChannelRequest */
    UA_OpenSecureChannelRequest opnSecRq;
    UA_OpenSecureChannelRequest_init(&opnSecRq);
    opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    opnSecRq.requestHeader.authenticationToken = client->authenticationToken;
    /*if(renew) {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to renew the SecureChannel");
    } else {*/
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to open a SecureChannel");
    //}
    opnSecRq.securityMode = client->channel.securityMode;

    opnSecRq.clientNonce = client->channel.localNonce;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;

    /* Prepare the entry for the linked list */
    UA_UInt32 requestId = ++client->requestId;
    /*AsyncServiceCall *ac = NULL;
    if(renew) {
        ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
        if (!ac)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        ac->callback =
                (UA_ClientAsyncServiceCallback) processDecodedOPNResponseAsync;
        ac->responseType = &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE];
        ac->requestId = requestId;
        ac->userdata = NULL;
    }*/

    /* Send the OPN message */
    UA_StatusCode retval = UA_SecureChannel_sendAsymmetricOPNMessage (
            &client->channel, requestId, &opnSecRq,
            &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST]);
    client->connectStatus = retval;

    if(retval != UA_STATUSCODE_GOOD) {
        client->connectStatus = retval;
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                      "Sending OPN message failed with error %s",
                      UA_StatusCode_name(retval));
        UA_Client_close(client);
        //if(renew)
        //    UA_free(ac);
        return retval;
    }

    UA_LOG_DEBUG (client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                  "OPN message sent");

    /* Store the entry for async processing and return */
    /*if(renew) {
        LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);
        return retval;
    }*/
    return retval;
}

static void
responseActivateSession(UA_Client *client, void *userdata, UA_UInt32 requestId,
                        void *response) {
    UA_ActivateSessionResponse *activateResponse =
            (UA_ActivateSessionResponse *) response;
    if(activateResponse->responseHeader.serviceResult) {
        UA_LOG_ERROR(
                client->config.logger,
                UA_LOGCATEGORY_CLIENT,
                "ActivateSession failed with error code %s",
                UA_StatusCode_name(activateResponse->responseHeader.serviceResult));
    }
    client->connection.state = UA_CONNECTION_ESTABLISHED;
    setClientState(client, UA_CLIENTSTATE_SESSION);

#ifdef UA_ENABLE_SUBSCRIPTIONS
        /* A new session has been created. We need to clean up the subscriptions */
        UA_Client_Subscriptions_clean(client);
#endif

     /* call onConnect (client_async.c) callback */
    AsyncServiceCall ac = client->asyncConnectCall;

    ac.callback(client, ac.userdata, requestId + 1,
                &activateResponse->responseHeader.serviceResult);
}

static UA_StatusCode
requestActivateSession (UA_Client *client, UA_UInt32 *requestId) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now ();
    request.requestHeader.timeoutHint = 600000;

    /* Manual ExtensionObject encoding of the identityToken */
    if (client->authenticationMethod == UA_CLIENTAUTHENTICATION_NONE) {
        UA_AnonymousIdentityToken* identityToken =
                UA_AnonymousIdentityToken_new();
        UA_AnonymousIdentityToken_init (identityToken);
        UA_String_copy(&client->token.policyId, &identityToken->policyId);
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
        request.userIdentityToken.content.decoded.type =
                &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
        request.userIdentityToken.content.decoded.data = identityToken;
    } else {
        UA_UserNameIdentityToken* identityToken =
                UA_UserNameIdentityToken_new();
        UA_UserNameIdentityToken_init (identityToken);
        UA_String_copy(&client->token.policyId, &identityToken->policyId);
        UA_String_copy(&client->username, &identityToken->userName);
        UA_String_copy(&client->password, &identityToken->password);
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
        request.userIdentityToken.content.decoded.type =
                &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
        request.userIdentityToken.content.decoded.data = identityToken;
    }
    /* This function call is to prepare a client signature */
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        signActivateSessionRequest(&client->channel, &request);
    }

    UA_StatusCode retval = UA_Client_sendAsyncRequest (
            client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
            (UA_ClientAsyncServiceCallback) responseActivateSession,
            &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], NULL, requestId);
    UA_ActivateSessionRequest_deleteMembers(&request);
    client->connectStatus = retval;
    return retval;
}

/* Combination of UA_Client_getEndpointsInternal and getEndpoints */
static void
responseGetEndpoints(UA_Client *client, void *userdata, UA_UInt32 requestId,
                     void *response) {
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_GetEndpointsResponse* resp;
    resp = (UA_GetEndpointsResponse*)response;

    if (resp->responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        client->connectStatus = resp->responseHeader.serviceResult;
        UA_LOG_ERROR (client->config.logger, UA_LOGCATEGORY_CLIENT,
                      "GetEndpointRequest failed with error code %s",
                      UA_StatusCode_name (client->connectStatus));
        UA_GetEndpointsResponse_deleteMembers(resp);
        return;
    }
    endpointArray = resp->endpoints;
    endpointArraySize = resp->endpointsSize;
    resp->endpoints = NULL;
    resp->endpointsSize = 0;

    UA_Boolean endpointFound = false;
    UA_Boolean tokenFound = false;
    UA_String securityNone = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_String binaryTransport = UA_STRING("http://opcfoundation.org/UA-Profile/"
                                          "Transport/uatcp-uasc-uabinary");

    // TODO: compare endpoint information with client->endpointUri
    for(size_t i = 0; i < endpointArraySize; ++i) {
        UA_EndpointDescription* endpoint = &endpointArray[i];
        /* look out for binary transport endpoints */
        /* Note: Siemens returns empty ProfileUrl, we will accept it as binary */
        if(endpoint->transportProfileUri.length != 0
                && !UA_String_equal (&endpoint->transportProfileUri,
                                     &binaryTransport))
            continue;

        /* Look for an endpoint corresponding to the client security policy */
        if(!UA_String_equal(&endpoint->securityPolicyUri, &client->securityPolicy.policyUri))
            continue;

        endpointFound = true;

        /* Look for a user token policy with an anonymous token */
        for(size_t j = 0; j < endpoint->userIdentityTokensSize; ++j) {
            UA_UserTokenPolicy* userToken = &endpoint->userIdentityTokens[j];

            /* Usertokens also have a security policy... */
            if(userToken->securityPolicyUri.length > 0
                    && !UA_String_equal(&userToken->securityPolicyUri,
                                         &securityNone))
                continue;

            /* UA_CLIENTAUTHENTICATION_NONE == UA_USERTOKENTYPE_ANONYMOUS
             * UA_CLIENTAUTHENTICATION_USERNAME == UA_USERTOKENTYPE_USERNAME
             * TODO: Check equivalence for other types when adding the support */
            if((int)client->authenticationMethod
                    != (int)userToken->tokenType)
                continue;

            /* Endpoint with matching usertokenpolicy found */
            tokenFound = true;
            UA_UserTokenPolicy_deleteMembers(&client->token);
            UA_UserTokenPolicy_copy(userToken, &client->token);
            break;
        }
    }

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    if(!endpointFound) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                      "No suitable endpoint found");
        client->connectStatus = UA_STATUSCODE_BADINTERNALERROR;
    } else if(!tokenFound) {
        UA_LOG_ERROR(
                client->config.logger, UA_LOGCATEGORY_CLIENT,
                "No suitable UserTokenPolicy found for the possible endpoints");
        client->connectStatus = UA_STATUSCODE_BADINTERNALERROR;
    }
    requestSession(client, &requestId);
}

static UA_StatusCode
requestGetEndpoints(UA_Client *client, UA_UInt32 *requestId) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    /* assume the endpointurl outlives the service call */
    UA_String_copy (&client->endpointUrl, &request.endpointUrl);

    client->connectStatus = UA_Client_sendAsyncRequest(
            client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
            (UA_ClientAsyncServiceCallback) responseGetEndpoints,
            &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE], NULL, requestId);
    UA_GetEndpointsRequest_deleteMembers(&request);
    return client->connectStatus;

}

static void
responseSessionCallback(UA_Client *client, void *userdata, UA_UInt32 requestId,
                        void *response) {
    UA_CreateSessionResponse *sessionResponse =
            (UA_CreateSessionResponse *)response;
    UA_NodeId_copy(&sessionResponse->authenticationToken,
                   &client->authenticationToken);
    requestActivateSession(client, &requestId);
}

static UA_StatusCode
requestSession(UA_Client *client, UA_UInt32 *requestId) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       client->channel.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(client->channel.localNonce.length != UA_SESSION_LOCALNONCELENGTH) {
           UA_ByteString_deleteMembers(&client->channel.localNonce);
            retval = UA_ByteString_allocBuffer(&client->channel.localNonce,
                                               UA_SESSION_LOCALNONCELENGTH);
            if(retval != UA_STATUSCODE_GOOD)
                return retval;
        }

        retval = client->channel.securityPolicy->symmetricModule.
                 generateNonce(client->channel.securityPolicy, &client->channel.localNonce);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->channel.localNonce, &request.clientNonce);
    request.requestedSessionTimeout = 1200000;
    request.maxResponseMessageSize = UA_INT32_MAX;
    UA_String_copy(&client->endpointUrl, &request.endpointUrl);

    retval = UA_Client_sendAsyncRequest (
            client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
            (UA_ClientAsyncServiceCallback) responseSessionCallback,
            &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], NULL, requestId);
    UA_CreateSessionRequest_deleteMembers(&request);
    client->connectStatus = retval;
    return client->connectStatus;
}

UA_StatusCode
UA_Client_connectInternalAsync(UA_Client *client, const char *endpointUrl,
                               UA_ClientAsyncServiceCallback callback,
                               void *userdata, UA_Boolean endpointsHandshake,
                               UA_Boolean createNewSession) {
    if(client->state >= UA_CLIENTSTATE_WAITING_FOR_ACK)
        return UA_STATUSCODE_GOOD;
    UA_ChannelSecurityToken_init(&client->channel.securityToken);
    client->channel.state = UA_SECURECHANNELSTATE_FRESH;
    /* Set up further callback function to handle secure channel and session establishment  */
    client->ackResponseCallback = processACKResponseAsync;
    client->openSecureChannelResponseCallback = processOPNResponse;
    client->endpointsHandshake = endpointsHandshake;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    client->connection = client->config.initConnectionFunc(
            client->config.localConnectionConfig, endpointUrl,
            client->config.timeout, client->config.logger);
    if(client->connection.state != UA_CONNECTION_OPENING) {
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    UA_String_deleteMembers(&client->endpointUrl);
    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);
    if(!client->endpointUrl.data) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    client->asyncConnectCall.callback = callback;
    client->asyncConnectCall.userdata = userdata;

    if(!client->connection.connectCallbackID) {
        retval = UA_Client_addRepeatedCallback(
                     client, client->config.pollConnectionFunc, &client->connection, 100,
                     &client->connection.connectCallbackID);
    }

    retval |= UA_SecureChannel_generateLocalNonce(&client->channel);

    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Delete async service. TODO: Move this from connect to the disconnect/cleanup phase */
    UA_Client_AsyncService_removeAll(client, UA_STATUSCODE_BADSHUTDOWN);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    client->currentlyOutStandingPublishRequests = 0;
#endif

    UA_NodeId_deleteMembers(&client->authenticationToken);

    /* Generate new local and remote key */
    retval = UA_SecureChannel_generateNewKeys(&client->channel);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return retval;

    cleanup: UA_Client_close(client);
        return retval;
}

UA_StatusCode
UA_Client_connect_iterate(UA_Client *client) {
    if (client->connection.state == UA_CONNECTION_ESTABLISHED){
        if (client->state < UA_CLIENTSTATE_WAITING_FOR_ACK)
            return sendHELMessage(client);
    }

    /* If server is not connected */
    if (client->connection.state == UA_CONNECTION_CLOSED) {
        client->connectStatus = UA_STATUSCODE_BADCONNECTIONCLOSED;
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "No connection to server.");
    }

    return client->connectStatus;
}

UA_StatusCode
UA_Client_connect_async(UA_Client *client, const char *endpointUrl,
                        UA_ClientAsyncServiceCallback callback,
                        void *userdata) {
    return UA_Client_connectInternalAsync(client, endpointUrl, callback,
            userdata, UA_TRUE, UA_TRUE);
}

/* Async disconnection */
static void
sendCloseSecureChannelAsync(UA_Client *client, void *userdata,
                             UA_UInt32 requestId, void *response) {
    UA_NodeId_deleteMembers (&client->authenticationToken);
    client->requestHandle = 0;

    UA_SecureChannel *channel = &client->channel;
    UA_CloseSecureChannelRequest request;
    UA_CloseSecureChannelRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.requestHeader.authenticationToken = client->authenticationToken;
    UA_SecureChannel_sendSymmetricMessage(
            channel, ++client->requestId, UA_MESSAGETYPE_CLO, &request,
            &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST]);
    UA_SecureChannel_deleteMembersCleanup(&client->channel);
}

static void
sendCloseSessionAsync(UA_Client *client, UA_UInt32 *requestId) {
    UA_CloseSessionRequest request;
    UA_CloseSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.deleteSubscriptions = true;

    UA_Client_sendAsyncRequest(
            client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
            (UA_ClientAsyncServiceCallback) sendCloseSecureChannelAsync,
            &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE], NULL, requestId);

}

UA_StatusCode
UA_Client_disconnect_async(UA_Client *client, UA_UInt32 *requestId) {
    /* Is a session established? */
    if (client->state == UA_CLIENTSTATE_SESSION) {
        client->state = UA_CLIENTSTATE_SESSION_DISCONNECTED;
        sendCloseSessionAsync(client, requestId);
    }

    /* Close the TCP connection
     * shutdown and close (in tcp.c) are already async*/
    if (client->state >= UA_CLIENTSTATE_CONNECTED)
        client->connection.close(&client->connection);
    else
        UA_Client_removeRepeatedCallback(client, client->connection.connectCallbackID);

#ifdef UA_ENABLE_SUBSCRIPTIONS
// TODO REMOVE WHEN UA_SESSION_RECOVERY IS READY
    /* We need to clean up the subscriptions */
    UA_Client_Subscriptions_clean(client);
#endif

    setClientState(client, UA_CLIENTSTATE_DISCONNECTED);
    return UA_STATUSCODE_GOOD;
}
