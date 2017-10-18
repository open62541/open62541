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

#define UA_MINMESSAGESIZE 8192


 /***********************/
 /* Open the Connection */
 /***********************/

static UA_StatusCode
processACKResponse(void *application, UA_Connection *connection, UA_ByteString *chunk) {
    UA_Client *client = (UA_Client*)application;

    /* Decode the message */
    size_t offset = 0;
    UA_StatusCode retval;
    UA_TcpMessageHeader messageHeader;
    UA_TcpAcknowledgeMessage ackMessage;
    retval = UA_TcpMessageHeader_decodeBinary(chunk, &offset, &messageHeader);
    retval |= UA_TcpAcknowledgeMessage_decodeBinary(chunk, &offset, &ackMessage);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Decoding ACK message failed");
        return retval;
    }

    /* Store remote connection settings and adjust local configuration to not
     * exceed the limits */
    client->connectState = HEL_ACK;
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK, "Received ACK message");
    connection->remoteConf.maxChunkCount = ackMessage.maxChunkCount; /* may be zero -> unlimited */
    connection->remoteConf.maxMessageSize = ackMessage.maxMessageSize; /* may be zero -> unlimited */
    connection->remoteConf.protocolVersion = ackMessage.protocolVersion;
    connection->remoteConf.sendBufferSize = ackMessage.sendBufferSize;
    connection->remoteConf.recvBufferSize = ackMessage.receiveBufferSize;
    if(connection->remoteConf.recvBufferSize < connection->localConf.sendBufferSize)
        connection->localConf.sendBufferSize = connection->remoteConf.recvBufferSize;
    if(connection->remoteConf.sendBufferSize < connection->localConf.recvBufferSize)
        connection->localConf.recvBufferSize = connection->remoteConf.sendBufferSize;
    connection->state = UA_CONNECTION_ESTABLISHED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
HelAckHandshake(UA_Client *client) {
    /* Get a buffer */
    UA_ByteString message;
    UA_Connection *conn = &client->connection;
    UA_StatusCode retval = conn->getSendBuffer(conn, UA_MINMESSAGESIZE, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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
    retval = UA_TcpHelloMessage_encodeBinary(&hello, &bufPos, &bufEnd);
    UA_TcpHelloMessage_deleteMembers(&hello);

    /* Encode the message header at offset 0 */
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_CHUNKTYPE_FINAL + UA_MESSAGETYPE_HEL;
    messageHeader.messageSize = (UA_UInt32)((uintptr_t)bufPos - (uintptr_t)message.data);
    bufPos = message.data;
    retval |= UA_TcpMessageHeader_encodeBinary(&messageHeader, &bufPos, &bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        conn->releaseSendBuffer(conn, &message);
        return retval;
    }

    /* Send the HEL message */
    message.length = messageHeader.messageSize;
    retval = conn->send(conn, &message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Sending HEL failed");
        return retval;
    }
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Sent HEL message");

    /* Loop until we have a complete chunk */
    retval = UA_Connection_receiveChunksBlocking(conn, client, processACKResponse,
                                                 client->config.timeout);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Receiving ACK message failed");
    return retval;
}

static UA_StatusCode
processDecodedOPNResponse(void *application, UA_SecureChannel *channel,
                          UA_MessageType messageType, UA_UInt32 requestId,
                          const UA_ByteString *message) {
    /* Does the request id match? */
    UA_Client *client = (UA_Client*)application;
    if(requestId != client->requestId)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;

    /* Is the content of the expected type? */
    size_t offset = 0;
    UA_NodeId responseId;
    UA_NodeId expectedId =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
    UA_StatusCode retval = UA_NodeId_decodeBinary(message, &offset, &responseId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    if(!UA_NodeId_equal(&responseId, &expectedId)) {
        UA_NodeId_deleteMembers(&responseId);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    UA_NodeId_deleteMembers(&responseId);

    /* Decode the response */
    UA_OpenSecureChannelResponse response;
    retval = UA_OpenSecureChannelResponse_decodeBinary(message, &offset, &response);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Response.securityToken.revisedLifetime is UInt32 we need to cast it to
     * DateTime=Int64 we take 75% of lifetime to start renewing as described in
     * standard */
    client->connectState = SECURECHANNEL_ACK;
    client->nextChannelRenewal = UA_DateTime_nowMonotonic() +
        (UA_DateTime)(response.securityToken.revisedLifetime * (UA_Double)UA_MSEC_TO_DATETIME * 0.75);

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

static UA_StatusCode
processOPNResponse(void *application, UA_Connection *connection, UA_ByteString *chunk) {
    UA_Client *client = (UA_Client*)application;
    return UA_SecureChannel_processChunk(&client->channel, chunk,
                                         processDecodedOPNResponse,
                                         client);
}

/* OPN messges to renew the channel are sent asynchronous */
static UA_StatusCode
openSecureChannel(UA_Client *client, UA_Boolean renew) {
    /* Check if sc is still valid */
    if(renew && client->nextChannelRenewal - UA_DateTime_nowMonotonic() > 0)
        return UA_STATUSCODE_GOOD;

    UA_Connection *conn = &client->connection;
    if(conn->state != UA_CONNECTION_ESTABLISHED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    /* Prepare the OpenSecureChannelRequest */
    UA_OpenSecureChannelRequest opnSecRq;
    UA_OpenSecureChannelRequest_init(&opnSecRq);
    opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    opnSecRq.requestHeader.authenticationToken = client->authenticationToken;
    if(renew) {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to renew the SecureChannel");
    } else {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to open a SecureChannel");
    }
    opnSecRq.securityMode = UA_MESSAGESECURITYMODE_NONE;
    opnSecRq.clientNonce = client->channel.localNonce;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;

    /* Prepare the entry for the linked list */
    UA_UInt32 requestId = ++client->requestId;
    AsyncServiceCall *ac = NULL;
    if(renew) {
        ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
        if(!ac)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        ac->callback = (UA_ClientAsyncServiceCallback)processDecodedOPNResponse;
        ac->responseType = &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE];
        ac->requestId = requestId;
        ac->userdata = NULL;
    }

    /* Send the OPN message */
    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&client->channel, requestId,
                                                  &opnSecRq, &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Sending OPN message failed with error %s", UA_StatusCode_name(retval));
        UA_Client_disconnect(client);
        if(renew)
            UA_free(ac);
        return retval;
    }

    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL, "OPN message sent");

    /* Store the entry for async processing and return */
    if(renew) {
        LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);
        return retval;
    }

    return UA_Connection_receiveChunksBlocking(&client->connection, client,
                                               processOPNResponse, client->config.timeout);
}

static UA_StatusCode
activateSession(UA_Client *client) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 600000;

    //manual ExtensionObject encoding of the identityToken
    if(client->authenticationMethod == UA_CLIENTAUTHENTICATION_NONE) {
        UA_AnonymousIdentityToken* identityToken = UA_AnonymousIdentityToken_new();
        UA_AnonymousIdentityToken_init(identityToken);
        UA_String_copy(&client->token.policyId, &identityToken->policyId);
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
        request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
        request.userIdentityToken.content.decoded.data = identityToken;
    } else {
        UA_UserNameIdentityToken* identityToken = UA_UserNameIdentityToken_new();
        UA_UserNameIdentityToken_init(identityToken);
        UA_String_copy(&client->token.policyId, &identityToken->policyId);
        UA_String_copy(&client->username, &identityToken->userName);
        UA_String_copy(&client->password, &identityToken->password);
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
        request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
        request.userIdentityToken.content.decoded.data = identityToken;
    }

    UA_ActivateSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);

    if(response.responseHeader.serviceResult) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "ActivateSession failed with error code %s",
                     UA_StatusCode_name(response.responseHeader.serviceResult));
    }

    UA_StatusCode retval = response.responseHeader.serviceResult;
    UA_ActivateSessionRequest_deleteMembers(&request);
    UA_ActivateSessionResponse_deleteMembers(&response);
    return retval;
}

/* Gets a list of endpoints. Memory is allocated for endpointDescription array */
UA_StatusCode
UA_Client_getEndpointsInternal(UA_Client *client, size_t* endpointDescriptionsSize,
                               UA_EndpointDescription** endpointDescriptions) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    // assume the endpointurl outlives the service call
    request.endpointUrl = client->endpointUrl;

    UA_GetEndpointsResponse response;
    UA_GetEndpointsResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_StatusCode retval = response.responseHeader.serviceResult;
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "GetEndpointRequest failed with error code %s",
                     UA_StatusCode_name(retval));
        UA_GetEndpointsResponse_deleteMembers(&response);
        return retval;
    }
    *endpointDescriptions = response.endpoints;
    *endpointDescriptionsSize = response.endpointsSize;
    response.endpoints = NULL;
    response.endpointsSize = 0;
    UA_GetEndpointsResponse_deleteMembers(&response);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getEndpoints(UA_Client *client) {
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval =
        UA_Client_getEndpointsInternal(client, &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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
        if(endpoint->transportProfileUri.length != 0 &&
           !UA_String_equal(&endpoint->transportProfileUri, &binaryTransport))
            continue;
        /* look out for an endpoint without security */
        if(!UA_String_equal(&endpoint->securityPolicyUri, &securityNone))
            continue;

        /* endpoint with no security found */
        endpointFound = true;

        /* look for a user token policy with an anonymous token */
        for(size_t j = 0; j < endpoint->userIdentityTokensSize; ++j) {
            UA_UserTokenPolicy* userToken = &endpoint->userIdentityTokens[j];

            /* Usertokens also have a security policy... */
            if(userToken->securityPolicyUri.length > 0 &&
               !UA_String_equal(&userToken->securityPolicyUri, &securityNone))
                continue;

            /* UA_CLIENTAUTHENTICATION_NONE == UA_USERTOKENTYPE_ANONYMOUS
             * UA_CLIENTAUTHENTICATION_USERNAME == UA_USERTOKENTYPE_USERNAME
             * TODO: Check equivalence for other types when adding the support */
            if((int)client->authenticationMethod != (int)userToken->tokenType)
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
        retval = UA_STATUSCODE_BADINTERNALERROR;
    } else if(!tokenFound) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No suitable UserTokenPolicy found for the possible endpoints");
        retval = UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode
createSession(UA_Client *client) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->channel.localNonce, &request.clientNonce);
    request.requestedSessionTimeout = 1200000;
    request.maxResponseMessageSize = UA_INT32_MAX;
    UA_String_copy(&client->endpointUrl, &request.endpointUrl);

    UA_CreateSessionResponse response;
    UA_CreateSessionResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    UA_NodeId_copy(&response.authenticationToken, &client->authenticationToken);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    UA_CreateSessionRequest_deleteMembers(&request);
    UA_CreateSessionResponse_deleteMembers(&response);
    return retval;
}



/*functions for async connection
 * hello and open secure channel dsyncronized with client.ConnectState
 * following requests and responses dsyncronized with callbacks*/

typedef struct Endpoints{
	UA_EndpointDescription** description;
	size_t* size;
}endpoints;

static UA_StatusCode
sendHelHandshake(UA_Client *client, UA_TcpMessageHeader *messageHeader) {
    /* Get a buffer */
    UA_ByteString message;
    UA_Connection *conn = &client->connection;
    UA_StatusCode retval = conn->getSendBuffer(conn, UA_MINMESSAGESIZE, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

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
    retval = UA_TcpHelloMessage_encodeBinary(&hello, &bufPos, &bufEnd);
    UA_TcpHelloMessage_deleteMembers(&hello);

    /* Encode the message header at offset 0 */
    //UA_TcpMessageHeader messageHeader;
    messageHeader->messageTypeAndChunkType = UA_CHUNKTYPE_FINAL + UA_MESSAGETYPE_HEL;
    messageHeader->messageSize = (UA_UInt32)((uintptr_t)bufPos - (uintptr_t)message.data);
    bufPos = message.data;
    retval |= UA_TcpMessageHeader_encodeBinary(messageHeader, &bufPos, &bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        conn->releaseSendBuffer(conn, &message);
        return retval;
    }

    /* Send the HEL message */
    message.length = messageHeader->messageSize;
    retval = conn->send(conn, &message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Sending HEL failed");
        return retval;
    }
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Sent HEL message");
    client->connectState = HEL_SENT;
	return retval;
}

static UA_StatusCode
recvHelAck(UA_Client *client, UA_TcpMessageHeader messageHeader) {

	UA_Connection *conn = &client->connection;
	UA_StatusCode retval = UA_Connection_receiveChunksNonBlocking(conn, client, processACKResponse);
	if(retval != UA_STATUSCODE_GOOD) {
		UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
					"Receiving ACK message failed");
	}
	return retval;
}

static UA_StatusCode
sendOpenSecRequest(UA_Client *client, UA_Boolean renew){
    /* Check if sc is still valid */
    if(renew && client->nextChannelRenewal - UA_DateTime_nowMonotonic() > 0)
        return UA_STATUSCODE_GOOD;

    UA_Connection *conn = &client->connection;
    if(conn->state != UA_CONNECTION_ESTABLISHED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    /* Prepare the OpenSecureChannelRequest */
    UA_OpenSecureChannelRequest opnSecRq;
    UA_OpenSecureChannelRequest_init(&opnSecRq);
    opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    opnSecRq.requestHeader.authenticationToken = client->authenticationToken;
    if(renew) {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to renew the SecureChannel");
    } else {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to open a SecureChannel");
    }
    opnSecRq.securityMode = UA_MESSAGESECURITYMODE_NONE;
    opnSecRq.clientNonce = client->channel.localNonce;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;

    /* Prepare the entry for the linked list */
    UA_UInt32 requestId = ++client->requestId;
    AsyncServiceCall *ac = NULL;
    if(renew) {
        ac = (AsyncServiceCall*)UA_malloc(sizeof(AsyncServiceCall));
        if(!ac)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        ac->callback = (UA_ClientAsyncServiceCallback)processDecodedOPNResponse;
        ac->responseType = &UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE];
        ac->requestId = requestId;
        ac->userdata = NULL;
    }

    /* Send the OPN message */
    UA_StatusCode retval =
        UA_SecureChannel_sendAsymmetricOPNMessage(&client->channel, requestId,
                                                  &opnSecRq, &UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Sending OPN message failed with error %s", UA_StatusCode_name(retval));
        UA_Client_disconnect(client);
        if(renew)
            UA_free(ac);
        return retval;
    }

    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL, "OPN message sent");

    /* Store the entry for async processing and return */
    if(renew) {
        LIST_INSERT_HEAD(&client->asyncServiceCalls, ac, pointers);
        return retval;
    }

	return retval;
}

static UA_StatusCode
recvOpenSecResponse(UA_Client *client){
	UA_Connection *conn = &client->connection;

	/* Receive the response */
	UA_StatusCode retval = UA_Connection_receiveChunksNonBlocking(conn, client, processOPNResponse);
	//UA_StatusCode retval = UA_Connection_receiveChunksBlocking(conn, &reply, &realloced, client->config.timeout);
	if(retval != UA_STATUSCODE_GOOD) {
		UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
					 "Receiving OpenSecureChannelResponse failed");
	}
	return retval;
}

static void
responseActivateSession(UA_Client *client, void *userdata,
        UA_UInt32 requestId, void *response){
	UA_ActivateSessionResponse *activateResponse = (UA_ActivateSessionResponse *) response;
	if(activateResponse->responseHeader.serviceResult) {
		UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
					 "ActivateSession failed with error code %s",
					 UA_StatusCode_name(activateResponse->responseHeader.serviceResult));
	}
	client->connection.state = UA_CONNECTION_ESTABLISHED;
	client->state = UA_CLIENTSTATE_CONNECTED;
}

static UA_StatusCode
requestActivateSession(UA_Client *client, size_t *requestId){
	UA_ActivateSessionRequest request;
	UA_ActivateSessionRequest_init(&request);
	request.requestHeader.requestHandle = *requestId;
	request.requestHeader.timestamp = UA_DateTime_now();
	request.requestHeader.timeoutHint = 600000;

	//manual ExtensionObject encoding of the identityToken
	if(client->authenticationMethod == UA_CLIENTAUTHENTICATION_NONE) {
		UA_AnonymousIdentityToken* identityToken = UA_AnonymousIdentityToken_new();
		UA_AnonymousIdentityToken_init(identityToken);
		UA_String_copy(&client->token.policyId, &identityToken->policyId);
		request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
		request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
		request.userIdentityToken.content.decoded.data = identityToken;
	} else {
		UA_UserNameIdentityToken* identityToken = UA_UserNameIdentityToken_new();
		UA_UserNameIdentityToken_init(identityToken);
		UA_String_copy(&client->token.policyId, &identityToken->policyId);
		UA_String_copy(&client->username, &identityToken->userName);
		UA_String_copy(&client->password, &identityToken->password);
		request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
		request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_USERNAMEIDENTITYTOKEN];
		request.userIdentityToken.content.decoded.data = identityToken;
	}
	UA_StatusCode retval = UA_Client_addAsyncRequest(client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
			(UA_ClientAsyncServiceCallback)responseActivateSession, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE], NULL, requestId);
	UA_ActivateSessionRequest_deleteMembers(&request);
	return retval;
}

static void
responseSessionCallback(UA_Client *client, void *userdata,
        UA_UInt32 requestId, void *response){
	UA_CreateSessionResponse *sessionResponse = (UA_CreateSessionResponse *) response;
    UA_NodeId_copy(&sessionResponse->authenticationToken, &client->authenticationToken);
	requestActivateSession(client, &requestId);
}

static UA_StatusCode
requestSession(UA_Client *client, size_t *requestId){
	UA_CreateSessionRequest request;
	UA_CreateSessionRequest_init(&request);
	request.requestHeader.requestHandle = *requestId;
	request.requestHeader.timestamp = UA_DateTime_now();
	request.requestHeader.timeoutHint = 10000;
	UA_ByteString_copy(&client->channel.localNonce, &request.clientNonce);
	request.requestedSessionTimeout = 1200000;
	request.maxResponseMessageSize = UA_INT32_MAX;
	UA_String_copy(&client->endpointUrl, &request.endpointUrl);

	UA_StatusCode retval = UA_Client_addAsyncRequest(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
			(UA_ClientAsyncServiceCallback)responseSessionCallback, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], NULL, requestId);
	UA_CreateSessionRequest_deleteMembers(&request);
	client->connectState = SESSION_ACK;
	return retval;
}

UA_StatusCode
__UA_Client_connect_async(UA_Client *client, const char *endpointUrl, UA_Boolean endpointsHandshake,
		UA_Boolean createSession, ConnectState *last_cs) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /*does state-check only when no ack has been sent*/
    if(client->connectState == NO_ACK){
        if(client->state == UA_CLIENTSTATE_CONNECTED)
            return UA_STATUSCODE_GOOD;
		client->connection =
			client->config.connectionFunc(client->config.localConnectionConfig,
					endpointUrl, client->config.timeout);
		if(client->connection.state != UA_CONNECTION_OPENING) {
			retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
			goto cleanup;
		}
    }

    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);
    if(!client->endpointUrl.data) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    client->connection.localConf = client->config.localConnectionConfig;

    ConnectState cs;
	cs = client->connectState;
	/*for hello handshake*/
	UA_TcpMessageHeader messageHeader;

	/*for get endpoints request and response*/
	size_t reqId;

	/*compare current state with last state, if the state didn't change, try to get ack in the next run
	 * removed getEndpoints, since */
	switch(cs){
	case NO_ACK:
		retval = sendHelHandshake(client, &messageHeader);
		break;

	case HEL_SENT:
		retval = recvHelAck(client, messageHeader);
		*last_cs = HEL_SENT;
		break;

	case HEL_ACK:
		if(*last_cs != HEL_ACK){
			client->channel.connection = &client->connection;
			retval = sendOpenSecRequest(client, false);
			*last_cs = HEL_ACK;
		}
		else{
			retval = recvOpenSecResponse(client);
		}
		break;

	case SECURECHANNEL_ACK:
		if (*last_cs != SECURECHANNEL_ACK) {
			//treated as a normal request
			retval = requestSession(client, &reqId);
			*last_cs = SECURECHANNEL_ACK;
		}
		break;

	default:
		UA_Client_run_iterate(client, true);
		break;
	}

    if(retval != UA_STATUSCODE_GOOD)
    	goto cleanup;

    return retval;

 cleanup:
    UA_Client_reset(client);
    return retval;
}

UA_StatusCode
UA_Client_connect_async(UA_Client *client, const char *endpointUrl) {
	UA_StatusCode retval = UA_STATUSCODE_GOOD;
	while (UA_Client_getState(client) != UA_CLIENTSTATE_CONNECTED)
		retval = __UA_Client_connect_async(client, endpointUrl, true, true, &(client->lastConnectState));

	return retval;
}
/*functions for async connection*/


UA_StatusCode
UA_Client_connectInternal(UA_Client *client, const char *endpointUrl,
                          UA_Boolean endpointsHandshake, UA_Boolean createNewSession) {
    UA_ChannelSecurityToken_init(&client->channel.securityToken);
    client->channel.state = UA_SECURECHANNELSTATE_FRESH;

    if(client->state >= UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    client->connection =
        client->config.connectionFunc(client->config.localConnectionConfig,
                                      endpointUrl, client->config.timeout);
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

    /* Open a TCP connection */
    client->connection.localConf = client->config.localConnectionConfig;
    retval = HelAckHandshake(client);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    client->state = UA_CLIENTSTATE_CONNECTED;

    /* Open a SecureChannel. TODO: Select with endpoint  */
    client->channel.connection = &client->connection;
    retval = openSecureChannel(client, false);
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;
    client->state = UA_CLIENTSTATE_SECURECHANNEL;

    /* Get Endpoints */
    if(endpointsHandshake) {
        retval = getEndpoints(client);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Open a Session */
    if(createNewSession) {
        retval = createSession(client);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        retval = activateSession(client);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        client->state = UA_CLIENTSTATE_SESSION;
    }
    return retval;

cleanup:
    UA_Client_disconnect(client);
    return retval;
}

UA_StatusCode
UA_Client_connect(UA_Client *client, const char *endpointUrl) {
    return UA_Client_connectInternal(client, endpointUrl, UA_TRUE, UA_TRUE);
}

UA_StatusCode
UA_Client_connect_username(UA_Client *client, const char *endpointUrl,
                           const char *username, const char *password) {
    client->authenticationMethod = UA_CLIENTAUTHENTICATION_USERNAME;
    client->username = UA_STRING_ALLOC(username);
    client->password = UA_STRING_ALLOC(password);
    return UA_Client_connect(client, endpointUrl);
}

UA_StatusCode
UA_Client_manuallyRenewSecureChannel(UA_Client *client) {
    UA_StatusCode retval = openSecureChannel(client, true);
    if(retval != UA_STATUSCODE_GOOD)
        client->state = UA_CLIENTSTATE_DISCONNECTED;
    return retval;
}

/************************/
/* Close the Connection */
/************************/

static void
sendCloseSession(UA_Client *client) {
    UA_CloseSessionRequest request;
    UA_CloseSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.deleteSubscriptions = true;
    UA_CloseSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);
    UA_CloseSessionRequest_deleteMembers(&request);
    UA_CloseSessionResponse_deleteMembers(&response);
}

static void
sendCloseSecureChannel(UA_Client *client) {
    UA_SecureChannel *channel = &client->channel;
    UA_CloseSecureChannelRequest request;
    UA_CloseSecureChannelRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.requestHeader.authenticationToken = client->authenticationToken;
    UA_SecureChannel_sendSymmetricMessage(channel, ++client->requestId,
                                          UA_MESSAGETYPE_CLO, &request,
                                          &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST]);
    UA_SecureChannel_deleteMembersCleanup(&client->channel);
}

UA_StatusCode
UA_Client_disconnect(UA_Client *client) {
    /* Is a session established? */
    if(client->state == UA_CLIENTSTATE_SESSION)
        sendCloseSession(client);

    /* Is a secure channel established? */
    if(client->state >= UA_CLIENTSTATE_SECURECHANNEL)
        sendCloseSecureChannel(client);

    /* Close the TCP connection */
    if(client->state >= UA_CLIENTSTATE_CONNECTED)
        client->connection.close(&client->connection);

    client->state = UA_CLIENTSTATE_DISCONNECTED;
    return UA_STATUSCODE_GOOD;
}
