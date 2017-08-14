/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_client.h"
#include "ua_client_internal.h"
#include "ua_connection_internal.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated.h"
#include "ua_transport_generated_handling.h"
#include "ua_transport_generated_encoding_binary.h"
#include "ua_util.h"
#include<stdio.h>
/*********************/
/* Create and Delete */
/*********************/

static UA_StatusCode
SecureChannelHandshake(UA_Client *client, UA_Boolean renew);
static UA_StatusCode
requestActivateSession(UA_Client *client, size_t *requestId);

static void UA_Client_init(UA_Client* client, UA_ClientConfig config) {
    memset(client, 0, sizeof(UA_Client));
    client->channel.connection = &client->connection;
    client->config = config;
    UA_Timer_init(&client->timer);

    /* Retrieve complete chunks */
    client->reply = UA_BYTESTRING_NULL;
    client->realloced = false;

    #ifndef UA_ENABLE_MULTITHREADING
        SLIST_INIT(&client->delayedCallbacks);
    #endif
}

UA_Client * UA_Client_new(UA_ClientConfig config) {
    UA_Client *client = (UA_Client*)UA_calloc(1, sizeof(UA_Client));
    if(!client)
        return NULL;
    UA_Client_init(client, config);
    return client;
}

static void UA_Client_deleteMembers(UA_Client* client) {
    UA_Client_disconnect(client);
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

void UA_Client_reset(UA_Client* client){
    UA_Client_deleteMembers(client);
    UA_Client_init(client, client->config);
}

void UA_Client_delete(UA_Client* client){
    UA_Client_deleteMembers(client);
    UA_free(client);
}

UA_ClientState UA_Client_getState(UA_Client *client) {
    if(!client)
        return UA_CLIENTSTATE_ERRORED;
    return client->state;
}

/*************************/
/* Manage the Connection */
/*************************/

#define UA_MINMESSAGESIZE 8192
/*functions for async connection
 * hello and open secure channel dsyncronized with client.ConnectState
 * following requests and responses dsyncronized with callbacks*/

typedef struct Endpoints{
	UA_EndpointDescription** description;
	size_t* size;
}endpoints;

static void
responseEndpoints(UA_Client *client, endpoints *userdata,
        UA_UInt32 requestId, const void *response0) {
    UA_EndpointDescription* endpointArray = NULL; //description
    size_t endpointArraySize = 0;//descriptionsSize
    UA_GetEndpointsResponse response;
    UA_GetEndpointsResponse_init(&response);

    UA_Client_run_iterate(client, true);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_StatusCode retval = response.responseHeader.serviceResult;
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "GetEndpointRequest failed with error code %s",
                     UA_StatusCode_name(retval));
        UA_GetEndpointsResponse_deleteMembers(&response);
       // return retval;
    }
    endpointArray = response.endpoints;
    endpointArraySize = response.endpointsSize;
    response.endpoints = NULL;
    response.endpointsSize = 0;
    UA_GetEndpointsResponse_deleteMembers(&response);
    UA_Boolean endpointFound = false;
    UA_Boolean tokenFound = false;
    UA_String securityNone = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_String binaryTransport = UA_STRING("http://opcfoundation.org/UA-Profile/"
                                          "Transport/uatcp-uasc-uabinary");

    //TODO: compare endpoint information with client->endpointUri
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
            UA_UserTokenPolicy_copy(userToken, &client->token);
            break;
        }
    }

    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(!endpointFound) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No suitable endpoint found");
    } else if(!tokenFound) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No suitable UserTokenPolicy found for the possible endpoints");
    }

    userdata->description = &endpointArray;
    userdata->size = &endpointArraySize;
}

UA_StatusCode
__UA_Client_getEndpoints_async(UA_Client *client, size_t *requestId, size_t* endpointDescriptionsSize,
		UA_EndpointDescription** endpointDescriptions) {

	endpoints ep = {.size = endpointDescriptionsSize, .description = endpointDescriptions};

    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    // assume the endpointurl outlives the service call
    request.endpointUrl = client->endpointUrl;

    //void callback and userdata
    UA_StatusCode retval = UA_Client_addAsyncRequest(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
    		(UA_ClientAsyncServiceCallback)responseEndpoints, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE], &ep, requestId);


    return retval;
	}

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

	/* Loop until we have a complete chunk */
	UA_ByteString reply = UA_BYTESTRING_NULL;
	UA_Boolean realloced = false;
	UA_Connection *conn = &client->connection;
	UA_StatusCode retval = UA_Connection_receiveChunksNonBlocking(conn, &reply, &realloced);
	if(retval != UA_STATUSCODE_GOOD) {
		UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
					"Receiving ACK message failed");
		return retval;
	}

	/* Decode the message */
	size_t offset = 0;
	UA_TcpAcknowledgeMessage ackMessage;
	retval = UA_TcpMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
	retval |= UA_TcpAcknowledgeMessage_decodeBinary(&reply, &offset, &ackMessage);

	/* Free the message buffer */
	if(!realloced)
		conn->releaseRecvBuffer(conn, &reply);
	else
		UA_ByteString_deleteMembers(&reply);

	/* Store remote connection settings and adjust local configuration to not
	   exceed the limits */
	if(retval == UA_STATUSCODE_GOOD) {
		client->connectState = HEL_ACK;
		UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK, "Received ACK message");
		conn->remoteConf.maxChunkCount = ackMessage.maxChunkCount; /* may be zero -> unlimited */
		conn->remoteConf.maxMessageSize = ackMessage.maxMessageSize; /* may be zero -> unlimited */
		conn->remoteConf.protocolVersion = ackMessage.protocolVersion;
		conn->remoteConf.sendBufferSize = ackMessage.sendBufferSize;
		conn->remoteConf.recvBufferSize = ackMessage.receiveBufferSize;
		if(conn->remoteConf.recvBufferSize < conn->localConf.sendBufferSize)
			conn->localConf.sendBufferSize = conn->remoteConf.recvBufferSize;
		if(conn->remoteConf.sendBufferSize < conn->localConf.recvBufferSize)
			conn->localConf.recvBufferSize = conn->remoteConf.sendBufferSize;
		conn->state = UA_CONNECTION_ESTABLISHED;
	} else {
		UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK, "Decoding ACK message failed");
	}
	UA_TcpAcknowledgeMessage_deleteMembers(&ackMessage);

	return retval;
}

static UA_StatusCode
sendOpenSecRequest(UA_Client *client, UA_Boolean renew, UA_SecureConversationMessageHeader *messageHeader,
		UA_AsymmetricAlgorithmSecurityHeader *asymHeader, UA_SequenceHeader *seqHeader, UA_NodeId *requestType){
	/* Check if sc is still valid */
	    if(renew && client->nextChannelRenewal - UA_DateTime_nowMonotonic() > 0)
	        return UA_STATUSCODE_GOOD;

	    UA_Connection *conn = &client->connection;
	    if(conn->state != UA_CONNECTION_ESTABLISHED)
	        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

	    UA_ByteString message;
	    UA_StatusCode retval = conn->getSendBuffer(conn, conn->remoteConf.recvBufferSize, &message);
	    if(retval != UA_STATUSCODE_GOOD)
	        return retval;

	    /* Jump over the messageHeader that will be encoded last */
	    UA_Byte *bufPos = &message.data[12];
	    const UA_Byte *bufEnd = &message.data[message.length];

	    /* Encode the Asymmetric Security Header */
	    UA_AsymmetricAlgorithmSecurityHeader_init(asymHeader);
	    asymHeader->securityPolicyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
	    retval = UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(asymHeader, &bufPos, &bufEnd);

	    /* Encode the sequence header */
	    seqHeader->sequenceNumber = ++client->channel.sendSequenceNumber;
	    seqHeader->requestId = ++client->requestId;
	    retval |= UA_SequenceHeader_encodeBinary(seqHeader, &bufPos, &bufEnd);

	    /* Encode the NodeId of the OpenSecureChannel Service */

	    retval |= UA_NodeId_encodeBinary(requestType, &bufPos, &bufEnd);

	    /* Encode the OpenSecureChannelRequest */
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
	    opnSecRq.clientNonce = client->channel.clientNonce;
	    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;
	    retval |= UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, &bufPos, &bufEnd);

	    /* Encode the message header at the beginning */
	    size_t length = (uintptr_t)(bufPos - message.data);
	    bufPos = message.data;
	    messageHeader->messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
	    messageHeader->messageHeader.messageSize = (UA_UInt32)length;
	    if(renew)
	        messageHeader->secureChannelId = client->channel.securityToken.channelId;
	    else
	        messageHeader->secureChannelId = 0;
	    retval |= UA_SecureConversationMessageHeader_encodeBinary(messageHeader, &bufPos, &bufEnd);

	    /* Clean up and return if encoding the message failed */
	    if(retval != UA_STATUSCODE_GOOD) {
	        client->connection.releaseSendBuffer(&client->connection, &message);
	        return retval;
	    }

	    /* Send the message */
	    message.length = length;
	    retval = conn->send(conn, &message);
	    if(retval != UA_STATUSCODE_GOOD)
		UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
								 "Opening SecureChannel failed");
		return retval;


}

static UA_StatusCode
recvOpenSecResponse(UA_Client *client, UA_Boolean renew, UA_SecureConversationMessageHeader messageHeader,
		UA_AsymmetricAlgorithmSecurityHeader asymHeader, UA_SequenceHeader seqHeader, UA_NodeId requestType){
	UA_Connection *conn = &client->connection;

	    /* Receive the response */
	    UA_ByteString reply = UA_BYTESTRING_NULL;
	    UA_Boolean realloced = false;
	    UA_StatusCode retval = UA_Connection_receiveChunksNonBlocking(conn, &reply, &realloced);
	    //UA_StatusCode retval = UA_Connection_receiveChunksBlocking(conn, &reply, &realloced, client->config.timeout);
	    if(retval != UA_STATUSCODE_GOOD) {
	        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
	                     "Receiving OpenSecureChannelResponse failed");
	        return retval;
	    }

	    /* Decode the header */
	    size_t offset = 0;
	    retval = UA_SecureConversationMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
	    retval |= UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&reply, &offset, &asymHeader);
	    retval |= UA_SequenceHeader_decodeBinary(&reply, &offset, &seqHeader);
	    retval |= UA_NodeId_decodeBinary(&reply, &offset, &requestType);
	    UA_NodeId expectedRequest =
	        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
	    if(retval != UA_STATUSCODE_GOOD || !UA_NodeId_equal(&requestType, &expectedRequest)) {
	        UA_ByteString_deleteMembers(&reply);
	        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
	        UA_NodeId_deleteMembers(&requestType);
	        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
	                     "Reply answers the wrong request. Expected OpenSecureChannelResponse.");
	        return UA_STATUSCODE_BADINTERNALERROR;
	    }

	    /* Save the sequence number from server */
	    client->channel.receiveSequenceNumber = seqHeader.sequenceNumber;

	    /* Decode the response */
	    UA_OpenSecureChannelResponse response;
	    retval = UA_OpenSecureChannelResponse_decodeBinary(&reply, &offset, &response);

	    /* Free the message */
	    if(!realloced)
	        conn->releaseRecvBuffer(conn, &reply);
	    else
	        UA_ByteString_deleteMembers(&reply);

	    /* Results in either the StatusCode of decoding or the service */
	    retval |= response.responseHeader.serviceResult;

	    if(retval == UA_STATUSCODE_GOOD) {
	        /* Response.securityToken.revisedLifetime is UInt32 we need to cast it
	         * to DateTime=Int64 we take 75% of lifetime to start renewing as
	         *  described in standard */
	    	client->connectState = SECURECHANNEL_ACK;
	        client->nextChannelRenewal = UA_DateTime_nowMonotonic() +
	            (UA_DateTime)(response.securityToken.revisedLifetime * (UA_Double)UA_MSEC_TO_DATETIME * 0.75);

	        /* Replace the old nonce */
	        UA_ChannelSecurityToken_deleteMembers(&client->channel.securityToken);
	        UA_ChannelSecurityToken_copy(&response.securityToken, &client->channel.securityToken);
	        UA_ByteString_deleteMembers(&client->channel.serverNonce);
	        UA_ByteString_copy(&response.serverNonce, &client->channel.serverNonce);

	        if(renew)
	            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
	                         "SecureChannel renewed");
	        else
	            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
	                         "SecureChannel opened");
	    } else {
	        if(renew)
	            UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
	                        "SecureChannel could not be renewed "
	                        "with error code %s", UA_StatusCode_name(retval));
	        else
	            UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
	                        "SecureChannel could not be opened "
	                        "with error code %s", UA_StatusCode_name(retval));
	    }

	    /* Clean up */
	    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
	    UA_OpenSecureChannelResponse_deleteMembers(&response);
	return retval;
}

static void
responseSessionCallback(UA_Client *client, void *userdata,
        UA_UInt32 requestId, void *response){
	UA_CreateSessionResponse *sessionResponse = response;
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
	UA_ByteString_copy(&client->channel.clientNonce, &request.clientNonce);
	request.requestedSessionTimeout = 1200000;
	request.maxResponseMessageSize = UA_INT32_MAX;
	UA_String_copy(&client->endpointUrl, &request.endpointUrl);

	UA_StatusCode retval = UA_Client_addAsyncRequest(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
			(UA_ClientAsyncServiceCallback)responseSessionCallback, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE], NULL, requestId);
	UA_CreateSessionRequest_deleteMembers(&request);
	client->connectState = SESSION_ACK;
	return retval;
}

static void
responseActivateSession(UA_Client *client, void *userdata,
        UA_UInt32 requestId, void *response){
	UA_ActivateSessionResponse *activateResponse = response;
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


/*functions for async connection*/


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
    UA_ByteString reply = UA_BYTESTRING_NULL;
    UA_Boolean realloced = false;
    retval = UA_Connection_receiveChunksBlocking(conn, &reply, &realloced,
                                                 client->config.timeout);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Receiving ACK message failed");
        return retval;
    }

    /* Decode the message */
    size_t offset = 0;
    UA_TcpAcknowledgeMessage ackMessage;
    retval = UA_TcpMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    retval |= UA_TcpAcknowledgeMessage_decodeBinary(&reply, &offset, &ackMessage);

    /* Free the message buffer */
    if(!realloced)
        conn->releaseRecvBuffer(conn, &reply);
    else
        UA_ByteString_deleteMembers(&reply);

    /* Store remote connection settings and adjust local configuration to not
       exceed the limits */
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK, "Received ACK message");
        conn->remoteConf.maxChunkCount = ackMessage.maxChunkCount; /* may be zero -> unlimited */
        conn->remoteConf.maxMessageSize = ackMessage.maxMessageSize; /* may be zero -> unlimited */
        conn->remoteConf.protocolVersion = ackMessage.protocolVersion;
        conn->remoteConf.sendBufferSize = ackMessage.sendBufferSize;
        conn->remoteConf.recvBufferSize = ackMessage.receiveBufferSize;
        if(conn->remoteConf.recvBufferSize < conn->localConf.sendBufferSize)
            conn->localConf.sendBufferSize = conn->remoteConf.recvBufferSize;
        if(conn->remoteConf.sendBufferSize < conn->localConf.recvBufferSize)
            conn->localConf.recvBufferSize = conn->remoteConf.sendBufferSize;
        conn->state = UA_CONNECTION_ESTABLISHED;
    } else {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK, "Decoding ACK message failed");
    }
    UA_TcpAcknowledgeMessage_deleteMembers(&ackMessage);

    return retval;
}

static UA_StatusCode
SecureChannelHandshake(UA_Client *client, UA_Boolean renew) {
    /* Check if sc is still valid */
    if(renew && client->nextChannelRenewal - UA_DateTime_nowMonotonic() > 0)
        return UA_STATUSCODE_GOOD;

    UA_Connection *conn = &client->connection;
    if(conn->state != UA_CONNECTION_ESTABLISHED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    UA_ByteString message;
    UA_StatusCode retval = conn->getSendBuffer(conn, conn->remoteConf.recvBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Jump over the messageHeader that will be encoded last */
    UA_Byte *bufPos = &message.data[12];
    const UA_Byte *bufEnd = &message.data[message.length];

    /* Encode the Asymmetric Security Header */
    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    retval = UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &bufPos, &bufEnd);

    /* Encode the sequence header */
    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = ++client->channel.sendSequenceNumber;
    seqHeader.requestId = ++client->requestId;
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &bufPos, &bufEnd);

    /* Encode the NodeId of the OpenSecureChannel Service */
    UA_NodeId requestType =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST].binaryEncodingId);
    retval |= UA_NodeId_encodeBinary(&requestType, &bufPos, &bufEnd);

    /* Encode the OpenSecureChannelRequest */
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
    opnSecRq.clientNonce = client->channel.clientNonce;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;
    retval |= UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, &bufPos, &bufEnd);

    /* Encode the message header at the beginning */
    size_t length = (uintptr_t)(bufPos - message.data);
    bufPos = message.data;
    UA_SecureConversationMessageHeader messageHeader;
    messageHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    messageHeader.messageHeader.messageSize = (UA_UInt32)length;
    if(renew)
        messageHeader.secureChannelId = client->channel.securityToken.channelId;
    else
        messageHeader.secureChannelId = 0;
    retval |= UA_SecureConversationMessageHeader_encodeBinary(&messageHeader, &bufPos, &bufEnd);

    /* Clean up and return if encoding the message failed */
    if(retval != UA_STATUSCODE_GOOD) {
        client->connection.releaseSendBuffer(&client->connection, &message);
        return retval;
    }

    /* Send the message */
    message.length = length;
    retval = conn->send(conn, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Receive the response */
    UA_ByteString reply = UA_BYTESTRING_NULL;
    UA_Boolean realloced = false;
    retval = UA_Connection_receiveChunksBlocking(conn, &reply, &realloced,
                                                 client->config.timeout);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Receiving OpenSecureChannelResponse failed");
        return retval;
    }

    /* Decode the header */
    size_t offset = 0;
    retval = UA_SecureConversationMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    retval |= UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&reply, &offset, &asymHeader);
    retval |= UA_SequenceHeader_decodeBinary(&reply, &offset, &seqHeader);
    retval |= UA_NodeId_decodeBinary(&reply, &offset, &requestType);
    UA_NodeId expectedRequest =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
    if(retval != UA_STATUSCODE_GOOD || !UA_NodeId_equal(&requestType, &expectedRequest)) {
        UA_ByteString_deleteMembers(&reply);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_NodeId_deleteMembers(&requestType);
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Reply answers the wrong request. Expected OpenSecureChannelResponse.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Save the sequence number from server */
    client->channel.receiveSequenceNumber = seqHeader.sequenceNumber;

    /* Decode the response */
    UA_OpenSecureChannelResponse response;
    retval = UA_OpenSecureChannelResponse_decodeBinary(&reply, &offset, &response);

    /* Free the message */
    if(!realloced)
        conn->releaseRecvBuffer(conn, &reply);
    else
        UA_ByteString_deleteMembers(&reply);

    /* Results in either the StatusCode of decoding or the service */
    retval |= response.responseHeader.serviceResult;

    if(retval == UA_STATUSCODE_GOOD) {
    	//commented out because this function is called by getendpoints, changing the state disturbs working flow
    	//client->connectState = SECURECHANNEL_ACK;

        /* Response.securityToken.revisedLifetime is UInt32 we need to cast it
         * to DateTime=Int64 we take 75% of lifetime to start renewing as
         *  described in standard */
        client->nextChannelRenewal = UA_DateTime_nowMonotonic() +
            (UA_DateTime)(response.securityToken.revisedLifetime * (UA_Double)UA_MSEC_TO_DATETIME * 0.75);

        /* Replace the old nonce */
        UA_ChannelSecurityToken_deleteMembers(&client->channel.securityToken);
        UA_ChannelSecurityToken_copy(&response.securityToken, &client->channel.securityToken);
        UA_ByteString_deleteMembers(&client->channel.serverNonce);
        UA_ByteString_copy(&response.serverNonce, &client->channel.serverNonce);

        if(renew)
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "SecureChannel renewed");
        else
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "SecureChannel opened");
    } else {
        if(renew)
            UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                        "SecureChannel could not be renewed "
                        "with error code %s", UA_StatusCode_name(retval));
        else
            UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                        "SecureChannel could not be opened "
                        "with error code %s", UA_StatusCode_name(retval));
    }

    /* Clean up */
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    UA_OpenSecureChannelResponse_deleteMembers(&response);
    return retval;
}

static UA_StatusCode
ActivateSession(UA_Client *client) {
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
__UA_Client_getEndpoints(UA_Client *client, size_t* endpointDescriptionsSize,
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
EndpointsHandshake(UA_Client *client) {
    UA_EndpointDescription* endpointArray = NULL; //description
    size_t endpointArraySize = 0;//descriptionsSize
    UA_StatusCode retval;


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
        retval = response.responseHeader.serviceResult;
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "GetEndpointRequest failed with error code %s",
                     UA_StatusCode_name(retval));
        UA_GetEndpointsResponse_deleteMembers(&response);
        return retval;
    }
    endpointArray = response.endpoints;
    endpointArraySize = response.endpointsSize;
    response.endpoints = NULL;
    response.endpointsSize = 0;
    UA_GetEndpointsResponse_deleteMembers(&response);
    retval =  UA_STATUSCODE_GOOD;

    UA_Boolean endpointFound = false;
    UA_Boolean tokenFound = false;
    UA_String securityNone = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_String binaryTransport = UA_STRING("http://opcfoundation.org/UA-Profile/"
                                          "Transport/uatcp-uasc-uabinary");

    //TODO: compare endpoint information with client->endpointUri
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
SessionHandshake(UA_Client *client) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->channel.clientNonce, &request.clientNonce);
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
    //client->connectState = SESSION_ACK;
    return retval;
}

static UA_StatusCode
CloseSession(UA_Client *client) {
    UA_CloseSessionRequest request;
    UA_CloseSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.deleteSubscriptions = true;
    UA_CloseSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);

    UA_StatusCode retval = response.responseHeader.serviceResult;
    UA_CloseSessionRequest_deleteMembers(&request);
    UA_CloseSessionResponse_deleteMembers(&response);
    return retval;
}

static UA_StatusCode
CloseSecureChannel(UA_Client *client) {
    UA_SecureChannel *channel = &client->channel;
    UA_CloseSecureChannelRequest request;
    UA_CloseSecureChannelRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_NodeId_copy(&client->authenticationToken,
                   &request.requestHeader.authenticationToken);

    UA_SecureConversationMessageHeader msgHeader;
    msgHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_CLO + UA_CHUNKTYPE_FINAL;
    msgHeader.secureChannelId = channel->securityToken.channelId;

    UA_SymmetricAlgorithmSecurityHeader symHeader;
    symHeader.tokenId = channel->securityToken.tokenId;

    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = ++channel->sendSequenceNumber;
    seqHeader.requestId = ++client->requestId;

    UA_NodeId typeId =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST].binaryEncodingId);

    UA_ByteString message;
    UA_Connection *conn = &client->connection;
    UA_StatusCode retval = conn->getSendBuffer(conn, conn->remoteConf.recvBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD){
        UA_CloseSecureChannelRequest_deleteMembers(&request);
        return retval;
    }

    /* Jump over the header */
    UA_Byte *bufPos = &message.data[12];
    const UA_Byte *bufEnd = &message.data[message.length];
    
    /* Encode some more headers and the CloseSecureChannelRequest */
    retval |= UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symHeader, &bufPos, &bufEnd);
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &bufPos, &bufEnd);
    retval |= UA_NodeId_encodeBinary(&typeId, &bufPos, &bufEnd);
    retval |= UA_encodeBinary(&request, &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST],
                              &bufPos, &bufEnd, NULL, NULL);

    /* Encode the header */
    msgHeader.messageHeader.messageSize = (UA_UInt32)((uintptr_t)bufPos - (uintptr_t)message.data);
    bufPos = message.data;
    retval |= UA_SecureConversationMessageHeader_encodeBinary(&msgHeader, &bufPos, &bufEnd);

    if(retval == UA_STATUSCODE_GOOD) {
        message.length = msgHeader.messageHeader.messageSize;
        retval = conn->send(conn, &message);
    } else {
        conn->releaseSendBuffer(conn, &message);
    }
    conn->close(conn);
    UA_CloseSecureChannelRequest_deleteMembers(&request);
    return retval;
}

UA_StatusCode
UA_Client_connect_username(UA_Client *client, const char *endpointUrl,
                           const char *username, const char *password){
    client->authenticationMethod=UA_CLIENTAUTHENTICATION_USERNAME;
    client->username = UA_STRING_ALLOC(username);
    client->password = UA_STRING_ALLOC(password);
    return UA_Client_connect(client, endpointUrl);
}

UA_StatusCode
__UA_Client_connect(UA_Client *client, const char *endpointUrl,
                    UA_Boolean endpointsHandshake, UA_Boolean createSession) {
    if(client->state == UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_GOOD;
    if(client->state == UA_CLIENTSTATE_ERRORED) {
        UA_Client_reset(client);
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    client->connection =
        client->config.connectionFunc(client->config.localConnectionConfig, endpointUrl);
    if(client->connection.state != UA_CONNECTION_OPENING) {
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);
    if(!client->endpointUrl.data) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    client->connection.localConf = client->config.localConnectionConfig;
    retval = HelAckHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SecureChannelHandshake(client, false);
    if(endpointsHandshake && retval == UA_STATUSCODE_GOOD)
        retval = EndpointsHandshake(client);
    if(endpointsHandshake && createSession && retval == UA_STATUSCODE_GOOD)
        retval = SessionHandshake(client);
    if(endpointsHandshake && createSession && retval == UA_STATUSCODE_GOOD)
        retval = ActivateSession(client);
    if(retval == UA_STATUSCODE_GOOD) {
        client->connection.state = UA_CONNECTION_ESTABLISHED;
        client->state = UA_CLIENTSTATE_CONNECTED;
    } else {
        goto cleanup;
    }
    return retval;

 cleanup:
    UA_Client_reset(client);
    return retval;
}

UA_StatusCode
UA_Client_connect(UA_Client *client, const char *endpointUrl) {
    return __UA_Client_connect(client, endpointUrl, UA_TRUE, UA_TRUE);
}

UA_StatusCode
__UA_Client_connect_async(UA_Client *client, const char *endpointUrl, UA_Boolean endpointsHandshake,
		UA_Boolean createSession, ConnectState *last_cs) {

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /*does state-check only when no ack has been sent*/
    if(client->connectState == NO_ACK){
        if(client->state == UA_CLIENTSTATE_CONNECTED)
            return UA_STATUSCODE_GOOD;
        if(client->state == UA_CLIENTSTATE_ERRORED) {
            UA_Client_reset(client);
        }
		client->connection =
			client->config.connectionFunc(client->config.localConnectionConfig, endpointUrl);
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
	/*for secure channel handshake*/
	UA_SecureConversationMessageHeader secMessageHeader;
	UA_AsymmetricAlgorithmSecurityHeader asymHeader;
	UA_SequenceHeader seqHeader;
	UA_NodeId requestType = UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST].binaryEncodingId);
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
			retval = sendOpenSecRequest(client, false, &secMessageHeader, &asymHeader, &seqHeader, &requestType);
			*last_cs = HEL_ACK;
		}
		else{
			retval = recvOpenSecResponse(client, false, secMessageHeader, asymHeader, seqHeader, requestType);
		}
		break;

	case SECURECHANNEL_ACK:
		if (*last_cs != SECURECHANNEL_ACK) {
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

UA_StatusCode UA_Client_disconnect(UA_Client *client) {
    if(client->state == UA_CLIENTSTATE_READY)
        return UA_STATUSCODE_BADNOTCONNECTED;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Is a session established? */
    if(client->connection.state == UA_CONNECTION_ESTABLISHED &&
       !UA_NodeId_equal(&client->authenticationToken, &UA_NODEID_NULL))
        retval = CloseSession(client);
    /* Is a secure channel established? */
    if(client->connection.state == UA_CONNECTION_ESTABLISHED)
        retval |= CloseSecureChannel(client);
    return retval;
}

UA_StatusCode UA_Client_manuallyRenewSecureChannel(UA_Client *client) {
    UA_StatusCode retval = SecureChannelHandshake(client, true);
    if(retval == UA_STATUSCODE_GOOD)
        client->state = UA_CLIENTSTATE_CONNECTED;
    return retval;
}

/****************/
/* Raw Services */
/****************/

/* For both synchronous and asynchronous service calls */
static UA_StatusCode
sendServiceRequest(UA_Client *client, const void *request,
                   const UA_DataType *requestType, UA_UInt32 *requestId) {
    /* Make sure we have a valid session */
    //UA_StatusCode retval = UA_Client_manuallyRenewSecureChannel(client);
	UA_StatusCode retval = SecureChannelHandshake(client, true);

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
    retval = UA_SecureChannel_sendBinaryMessage(&client->channel, rqId, rr, requestType);
    UA_NodeId_init(&rr->authenticationToken);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    *requestId = rqId;
    return UA_STATUSCODE_GOOD;
}

/* Look for the async callback in the linked list, execute and delete it */
static UA_StatusCode
processAsyncResponse(UA_Client *client, UA_UInt32 requestId,
                  UA_NodeId *responseTypeId, UA_ByteString *responseMessage,
                  size_t *offset) {
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

/* For synchronous service calls. Execute async responses until the response
 * with the correct requestId turns up. Then, the response is decoded into the
 * "response" pointer. If the responseType is NULL, just process responses as
 * async. */
typedef struct {
    UA_Client *client;
    UA_Boolean received;
    UA_UInt32 requestId;
    void *response;
    const UA_DataType *responseType;
} SyncResponseDescription;

static void
processServiceResponse(SyncResponseDescription *rd, UA_SecureChannel *channel,
                       UA_MessageType messageType, UA_UInt32 requestId,
                       UA_ByteString *message) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_NodeId expectedNodeId;
    const UA_NodeId serviceFaultNodeId =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_SERVICEFAULT].binaryEncodingId);

    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)rd->response;

    /* Forward declaration for the goto */
    size_t offset = 0;
    UA_NodeId responseId;
    UA_NodeId_init(&responseId);

    /* Got an error.
     * TODO: Return as part of the response header */
    if(messageType == UA_MESSAGETYPE_ERR) {
        UA_TcpErrorMessage *msg = (UA_TcpErrorMessage*)message;
        UA_LOG_ERROR(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Server replied with an error message: %s %.*s",
                     UA_StatusCode_name(msg->error), msg->reason.length,
                     msg->reason.data);
        retval = msg->error;
        goto finish;
    }

    /* Unexpected response type.
     * TODO: How to process valid OPN responses? */
    if(messageType != UA_MESSAGETYPE_MSG) {
        UA_LOG_ERROR(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Server replied with the wrong message type");
        retval = UA_STATUSCODE_BADTCPMESSAGETYPEINVALID;
        goto finish;
    }

    /* Decode the data type identifier of the response */
    retval = UA_NodeId_decodeBinary(message, &offset, &responseId);
    if(retval != UA_STATUSCODE_GOOD)
        goto finish;

    /* Got an asynchronous response. Didn't expect a synchronous response
     * (responseType NULL) or the id does not match. */
    if(!rd->responseType || requestId != rd->requestId) {
        retval = processAsyncResponse(rd->client, requestId, &responseId, message, &offset);
        goto finish;
    }

    /* Got the synchronous response */
    rd->received= true;

    /* Check that the response type matches */
    expectedNodeId = UA_NODEID_NUMERIC(0, rd->responseType->binaryEncodingId);
    if(!UA_NodeId_equal(&responseId, &expectedNodeId)) {
        if(UA_NodeId_equal(&responseId, &serviceFaultNodeId)) {
            /* Take the statuscode from the servicefault */
            retval = UA_decodeBinary(message, &offset, rd->response,
                                     &UA_TYPES[UA_TYPES_SERVICEFAULT], 0, NULL);
        } else {
            UA_LOG_ERROR(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Reply answers the wrong request. Expected ns=%i,i=%i."
                         "But retrieved ns=%i,i=%i", expectedNodeId.namespaceIndex,
                         expectedNodeId.identifier.numeric, responseId.namespaceIndex,
                         responseId.identifier.numeric);
            retval = UA_STATUSCODE_BADINTERNALERROR;
        }
        goto finish;
    }

    /* Decode the response */
    retval = UA_decodeBinary(message, &offset, rd->response, rd->responseType,
                             rd->client->config.customDataTypesSize,
                             rd->client->config.customDataTypes);

 finish:
    UA_NodeId_deleteMembers(&responseId);
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Received a response of type %i", responseId.identifier.numeric);
    } else {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            retval = UA_STATUSCODE_BADRESPONSETOOLARGE;
        UA_LOG_INFO(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                    "Error receiving the response");
        respHeader->serviceResult = retval;
    }
}
UA_StatusCode
receiveServiceResponse_async(UA_Client *client, void *response,
                       const UA_DataType *responseType) {
    /* Prepare the response and the structure we give into processServiceResponse */
    SyncResponseDescription rd = {client, false, 0, response, responseType};

    /* Return upon receiving the synchronized response. All other responses are
     * processed with a callback "in the background". */

        UA_StatusCode retval =
            UA_Connection_receiveChunksNonBlocking(&client->connection, &client->reply,
                                                &client->realloced);

        if(retval != UA_STATUSCODE_GOOD || client->reply.length > 0){
            /* ProcessChunks and call processServiceResponse for complete messages */
            UA_SecureChannel_processChunks(&client->channel, &client->reply,
                                       (UA_ProcessMessageCallback*)processServiceResponse, &rd);
            /* Free the received buffer */
            if(!client->realloced)
                client->connection.releaseRecvBuffer(&client->connection, &client->reply);
            else
                UA_ByteString_deleteMembers(&client->reply);

            /* Retrieve complete chunks */
            client->reply = UA_BYTESTRING_NULL;
            client->realloced = false;
        }


    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
receiveServiceResponse(UA_Client *client, void *response,
                       const UA_DataType *responseType, UA_DateTime maxDate,
                       UA_UInt32 *synchronousRequestId) {
    /* Prepare the response and the structure we give into processServiceResponse */
    SyncResponseDescription rd = {client, false, 0, response, responseType};

    /* Return upon receiving the synchronized response. All other responses are
     * processed with a callback "in the background". */
    if(synchronousRequestId)
        rd.requestId = *synchronousRequestId;

    do {
        /* Retrieve complete chunks */
        UA_ByteString reply = UA_BYTESTRING_NULL;
        UA_Boolean realloced = false;
        UA_DateTime now = UA_DateTime_nowMonotonic();
        if(now > maxDate)
            return UA_STATUSCODE_GOODNONCRITICALTIMEOUT;
        UA_UInt32 timeout = (UA_UInt32)((maxDate - now) / UA_MSEC_TO_DATETIME);
        UA_StatusCode retval =
            UA_Connection_receiveChunksBlocking(&client->connection, &reply,
                                                &realloced, timeout);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;

        /* ProcessChunks and call processServiceResponse for complete messages */
        UA_SecureChannel_processChunks(&client->channel, &reply,
                                       (UA_ProcessMessageCallback*)processServiceResponse, &rd);
        /* Free the received buffer */
        if(!realloced)
            client->connection.releaseRecvBuffer(&client->connection, &reply);
        else
            UA_ByteString_deleteMembers(&reply);
    } while(!rd.received);

    return UA_STATUSCODE_GOOD;
}

void
__UA_Client_Service(UA_Client *client, const void *request,
                    const UA_DataType *requestType, void *response,
                    const UA_DataType *responseType) {
    UA_init(response, responseType);
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;

    /* Send the request */
    UA_UInt32 requestId;
    UA_StatusCode retval = sendServiceRequest(client, request, requestType, &requestId);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            respHeader->serviceResult = UA_STATUSCODE_BADREQUESTTOOLARGE;
        else
            respHeader->serviceResult = retval;
        client->state = UA_CLIENTSTATE_FAULTED;
        return;
    }

    /* Retrieve the response */
    UA_DateTime maxDate = UA_DateTime_nowMonotonic() +
        (client->config.timeout * UA_MSEC_TO_DATETIME);
    retval = receiveServiceResponse(client, response, responseType, maxDate, &requestId);
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
    UA_StatusCode retval = sendServiceRequest(client, request, requestType, &ac->requestId);
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
UA_Client_addAsyncRequest(UA_Client *client, const void *request,
        const UA_DataType *requestType,
        UA_ClientAsyncServiceCallback callback,
        const UA_DataType *responseType,
        void *userdata, UA_UInt32 *requestId) {
    return __UA_Client_AsyncService(client,request,requestType,callback,responseType,userdata,requestId);
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
