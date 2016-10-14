#include "ua_util.h"
#include "ua_client.h"
#include "ua_client_highlevel.h"
#include "ua_client_internal.h"
#include "ua_connection_internal.h"
#include "ua_types_generated.h"
#include "ua_types_encoding_binary.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_nodeids.h"
#include "ua_transport_generated.h"
#include "ua_transport_generated_handling.h"
#include "ua_transport_generated_encoding_binary.h"

/*********************/
/* Create and Delete */
/*********************/

static void UA_Client_init(UA_Client* client, UA_ClientConfig config) {
    client->state = UA_CLIENTSTATE_READY;
    client->connection = UA_malloc(sizeof(UA_Connection));
    UA_Connection_init(client->connection);
    client->channel = UA_malloc(sizeof(UA_SecureChannel));
    UA_SecureChannel_init(client->channel);
    client->channel->connection = client->connection;
    UA_String_init(&client->endpointUrl);
    client->requestId = 0;

    client->authenticationMethod = UA_CLIENTAUTHENTICATION_NONE;
    UA_String_init(&client->username);
    UA_String_init(&client->password);

    UA_NodeId_init(&client->authenticationToken);
    client->requestHandle = 0;

    client->config = config;
    client->scRenewAt = 0;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    client->monitoredItemHandles = 0;
    LIST_INIT(&client->pendingNotificationsAcks);
    LIST_INIT(&client->subscriptions);
#endif
}

UA_Client * UA_Client_new(UA_ClientConfig config) {
    UA_Client *client = UA_calloc(1, sizeof(UA_Client));
    if(!client)
        return NULL;

    UA_Client_init(client, config);
    return client;
}

static void UA_Client_deleteMembers(UA_Client* client) {
    UA_Client_disconnect(client);
    UA_SecureChannel_deleteMembersCleanup(client->channel);
    UA_free(client->channel);
    UA_Connection_deleteMembers(client->connection);
    UA_free(client->connection);
    if(client->endpointUrl.data)
        UA_String_deleteMembers(&client->endpointUrl);
    UA_UserTokenPolicy_deleteMembers(&client->token);
    UA_NodeId_deleteMembers(&client->authenticationToken);
    if(client->username.data)
        UA_String_deleteMembers(&client->username);
    if(client->password.data)
           UA_String_deleteMembers(&client->password);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_NotificationsAckNumber *n, *tmp;
    LIST_FOREACH_SAFE(n, &client->pendingNotificationsAcks, listEntry, tmp) {
        LIST_REMOVE(n, listEntry);
        free(n);
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

UA_ClientState UA_EXPORT UA_Client_getState(UA_Client *client) {
    if(!client)
        return UA_CLIENTSTATE_ERRORED;
    return client->state;
}

/*************************/
/* Manage the Connection */
/*************************/

#define UA_MINMESSAGESIZE 8192

static UA_StatusCode HelAckHandshake(UA_Client *client) {
    UA_Connection *conn = client->connection;

    /* Get a buffer */
    UA_ByteString message;
    UA_StatusCode retval =
        client->connection->getSendBuffer(client->connection,
                                          UA_MINMESSAGESIZE, &message);
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

    size_t offset = 8;
    retval = UA_TcpHelloMessage_encodeBinary(&hello, &message, &offset);
    UA_TcpHelloMessage_deleteMembers(&hello);

    /* Encode the message header at offset 0 */
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndChunkType = UA_CHUNKTYPE_FINAL + UA_MESSAGETYPE_HEL;
    messageHeader.messageSize = (UA_UInt32)offset;
    offset = 0;
    retval |= UA_TcpMessageHeader_encodeBinary(&messageHeader, &message, &offset);
    if(retval != UA_STATUSCODE_GOOD) {
        client->connection->releaseSendBuffer(client->connection, &message);
        return retval;
    }

    /* Send the HEL message */
    message.length = messageHeader.messageSize;
    retval = client->connection->send(client->connection, &message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Sending HEL failed");
        return retval;
    }
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK,
                 "Sent HEL message");

    /* Loop until we have a complete chunk */
    /* TODO: quit after a total wait time of client->config.timeout */
    UA_ByteString reply = UA_BYTESTRING_NULL;
    UA_Boolean realloced = false;
    do {
        retval = client->connection->recv(client->connection, &reply, client->config.timeout);
        retval |= UA_Connection_completeMessages(client->connection, &reply, &realloced);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                        "Receiving ACK message failed");
            return retval;
        }
    } while(reply.length == 0);

    /* Decode the message */
    offset = 0;
    UA_TcpAcknowledgeMessage ackMessage;
    retval = UA_TcpMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    retval |= UA_TcpAcknowledgeMessage_decodeBinary(&reply, &offset, &ackMessage);

    /* Free the message buffer */
    if(!realloced)
        client->connection->releaseRecvBuffer(client->connection, &reply);
    else
        UA_ByteString_deleteMembers(&reply);

    /* Store remote connection settings and adjust local configuration to not
       exceed the limits */
    if(retval == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_NETWORK,
                     "Received ACK message");
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
        UA_LOG_INFO(client->config.logger, UA_LOGCATEGORY_NETWORK,
                    "Decoding ACK message failed");
    }
    UA_TcpAcknowledgeMessage_deleteMembers(&ackMessage);

    return retval;
}

static UA_StatusCode
SecureChannelHandshake(UA_Client *client, UA_Boolean renew) {
    /* Check if sc is still valid */
    if(renew && client->scRenewAt - UA_DateTime_now() > 0)
        return UA_STATUSCODE_GOOD;

    UA_Connection *c = client->connection;
    if(c->state != UA_CONNECTION_ESTABLISHED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    UA_SecureConversationMessageHeader messageHeader;
    messageHeader.messageHeader.messageTypeAndChunkType =
        UA_MESSAGETYPE_OPN + UA_CHUNKTYPE_FINAL;
    if(renew)
        messageHeader.secureChannelId = client->channel->securityToken.channelId;
    else
        messageHeader.secureChannelId = 0;

    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = ++client->channel->sendSequenceNumber;
    seqHeader.requestId = ++client->requestId;

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri =
        UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");

    /* id of opensecurechannelrequest */
    UA_NodeId requestType =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELREQUEST].binaryEncodingId);

    UA_OpenSecureChannelRequest opnSecRq;
    UA_OpenSecureChannelRequest_init(&opnSecRq);
    opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    opnSecRq.requestHeader.authenticationToken = client->authenticationToken;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;
    if(renew) {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to renew the SecureChannel");
    } else {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Requesting to open a SecureChannel");
    }

    UA_ByteString_copy(&client->channel->clientNonce, &opnSecRq.clientNonce);
    opnSecRq.securityMode = UA_MESSAGESECURITYMODE_NONE;

    UA_ByteString message;
    UA_StatusCode retval = c->getSendBuffer(c, c->remoteConf.recvBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);
        return retval;
    }

    size_t offset = 12;
    retval = UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &message, &offset);
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &message, &offset);
    retval |= UA_NodeId_encodeBinary(&requestType, &message, &offset);
    retval |= UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, &message, &offset);
    messageHeader.messageHeader.messageSize = (UA_UInt32)offset;
    offset = 0;
    retval |= UA_SecureConversationMessageHeader_encodeBinary(&messageHeader, &message, &offset);

    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);
    if(retval != UA_STATUSCODE_GOOD) {
        client->connection->releaseSendBuffer(client->connection, &message);
        return retval;
    }

    message.length = messageHeader.messageHeader.messageSize;
    retval = client->connection->send(client->connection, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_ByteString reply;
    UA_ByteString_init(&reply);
    UA_Boolean realloced = false;
    do {
        retval = c->recv(c, &reply, client->config.timeout);
        retval |= UA_Connection_completeMessages(c, &reply, &realloced);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Receiving OpenSecureChannelResponse failed");
            return retval;
        }
    } while(reply.length == 0);

    offset = 0;
    UA_SecureConversationMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&reply, &offset, &asymHeader);
    UA_SequenceHeader_decodeBinary(&reply, &offset, &seqHeader);
    UA_NodeId_decodeBinary(&reply, &offset, &requestType);
    UA_NodeId expectedRequest =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_OPENSECURECHANNELRESPONSE].binaryEncodingId);
    if(!UA_NodeId_equal(&requestType, &expectedRequest)) {
        UA_ByteString_deleteMembers(&reply);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_NodeId_deleteMembers(&requestType);
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Reply answers the wrong request. Expected OpenSecureChannelResponse.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_OpenSecureChannelResponse response;
    retval = UA_OpenSecureChannelResponse_decodeBinary(&reply, &offset, &response);
    if(!realloced)
        c->releaseRecvBuffer(c, &reply);
    else
        UA_ByteString_deleteMembers(&reply);

    /* save the sequence number from server */
    client->channel->receiveSequenceNumber = seqHeader.sequenceNumber;


    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Decoding OpenSecureChannelResponse failed");
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_OpenSecureChannelResponse_init(&response);
        response.responseHeader.serviceResult = retval;
        return retval;
    }

    retval = response.responseHeader.serviceResult;
    if(retval == UA_STATUSCODE_GOOD) {
        /* Response.securityToken.revisedLifetime is UInt32 we need to cast it
           to DateTime=Int64 we take 75% of lifetime to start renewing as
           described in standard */
        client->scRenewAt = UA_DateTime_now() +
            (UA_DateTime)(response.securityToken.revisedLifetime * (UA_Double)UA_MSEC_TO_DATETIME * 0.75);

        /* Replace the old nonce */
        UA_ChannelSecurityToken_deleteMembers(&client->channel->securityToken);
        UA_ChannelSecurityToken_copy(&response.securityToken, &client->channel->securityToken);
        UA_ByteString_deleteMembers(&client->channel->serverNonce);
        UA_ByteString_copy(&response.serverNonce, &client->channel->serverNonce);

        if(renew)
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "SecureChannel renewed");
        else
            UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "SecureChannel opened");
    } else {
        UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "SecureChannel could not be opened / "
                     "renewed with statuscode %i", retval);
    }
    UA_OpenSecureChannelResponse_deleteMembers(&response);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    return retval;
}

static UA_StatusCode ActivateSession(UA_Client *client) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);

    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 600000;

    //manual ExtensionObject encoding of the identityToken
    if(client->authenticationMethod == UA_CLIENTAUTHENTICATION_NONE) {
        UA_AnonymousIdentityToken* identityToken = UA_malloc(sizeof(UA_AnonymousIdentityToken));
        UA_AnonymousIdentityToken_init(identityToken);
        UA_String_copy(&client->token.policyId, &identityToken->policyId);
        request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED;
        request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
        request.userIdentityToken.content.decoded.data = identityToken;
    } else {
        UA_UserNameIdentityToken* identityToken = UA_malloc(sizeof(UA_UserNameIdentityToken));
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
                     "ActivateSession failed with statuscode 0x%08x",
                     response.responseHeader.serviceResult);
    }

    UA_StatusCode retval = response.responseHeader.serviceResult;
    UA_ActivateSessionRequest_deleteMembers(&request);
    UA_ActivateSessionResponse_deleteMembers(&response);
    return retval;
}

/* Gets a list of endpoints. Memory is allocated for endpointDescription array */
static UA_StatusCode
GetEndpoints(UA_Client *client, size_t* endpointDescriptionsSize,
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
                     "GetEndpointRequest failed with statuscode 0x%08x", retval);
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

static UA_StatusCode EndpointsHandshake(UA_Client *client) {
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = GetEndpoints(client, &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Boolean endpointFound = false;
    UA_Boolean tokenFound = false;
    UA_String securityNone = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    UA_String binaryTransport = UA_STRING("http://opcfoundation.org/UA-Profile/"
                                          "Transport/uatcp-uasc-uabinary");

    //TODO: compare endpoint information with client->endpointUri
    for(size_t i = 0; i < endpointArraySize; i++) {
        UA_EndpointDescription* endpoint = &endpointArray[i];
        /* look out for binary transport endpoints */
        //NODE: Siemens returns empty ProfileUrl, we will accept it as binary
        if(endpoint->transportProfileUri.length!=0 &&
           !UA_String_equal(&endpoint->transportProfileUri, &binaryTransport))
            continue;
        /* look out for an endpoint without security */
        if(!UA_String_equal(&endpoint->securityPolicyUri, &securityNone))
            continue;
        endpointFound = true;
        /* endpoint with no security found */
        /* look for a user token policy with an anonymous token */
        for(size_t j = 0; j < endpoint->userIdentityTokensSize; ++j) {
            UA_UserTokenPolicy* userToken = &endpoint->userIdentityTokens[j];
            //anonymous authentication
            if(client->authenticationMethod == UA_CLIENTAUTHENTICATION_NONE){
                if(userToken->tokenType != UA_USERTOKENTYPE_ANONYMOUS)
                    continue;
            }else{
            //username authentication
                if(userToken->tokenType != UA_USERTOKENTYPE_USERNAME)
                    continue;
            }
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
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(!tokenFound) {
        UA_LOG_ERROR(client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "No anonymous token found");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return retval;
}

static UA_StatusCode SessionHandshake(UA_Client *client) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->channel->clientNonce, &request.clientNonce);
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

static UA_StatusCode CloseSession(UA_Client *client) {
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

static UA_StatusCode CloseSecureChannel(UA_Client *client) {
    UA_SecureChannel *channel = client->channel;
    UA_CloseSecureChannelRequest request;
    UA_CloseSecureChannelRequest_init(&request);
    request.requestHeader.requestHandle = ++client->requestHandle;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_NodeId_copy(&client->authenticationToken,
                   &request.requestHeader.authenticationToken);

    UA_SecureConversationMessageHeader msgHeader;
    msgHeader.messageHeader.messageTypeAndChunkType = UA_MESSAGETYPE_CLO + UA_CHUNKTYPE_FINAL;
    msgHeader.secureChannelId = client->channel->securityToken.channelId;

    UA_SymmetricAlgorithmSecurityHeader symHeader;
    symHeader.tokenId = channel->securityToken.tokenId;

    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = ++channel->sendSequenceNumber;
    seqHeader.requestId = ++client->requestId;

    UA_NodeId typeId =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST].binaryEncodingId);

    UA_ByteString message;
    UA_Connection *c = client->connection;
    UA_StatusCode retval = c->getSendBuffer(c, c->remoteConf.recvBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD){
        UA_CloseSecureChannelRequest_deleteMembers(&request);
        return retval;
    }

    size_t offset = 12;
    retval |= UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symHeader, &message, &offset);
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &message, &offset);
    retval |= UA_NodeId_encodeBinary(&typeId, &message, &offset);
    retval |= UA_encodeBinary(&request, &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST],
                              NULL, NULL, &message, &offset);

    msgHeader.messageHeader.messageSize = (UA_UInt32)offset;
    offset = 0;
    retval |= UA_SecureConversationMessageHeader_encodeBinary(&msgHeader, &message, &offset);

    if(retval == UA_STATUSCODE_GOOD) {
        message.length = msgHeader.messageHeader.messageSize;
        retval = client->connection->send(client->connection, &message);
    } else {
        client->connection->releaseSendBuffer(client->connection, &message);
    }
    client->connection->close(client->connection);
    UA_CloseSecureChannelRequest_deleteMembers(&request);
    return retval;
}

UA_StatusCode
UA_Client_getEndpoints(UA_Client *client, const char *serverUrl,
                       size_t* endpointDescriptionsSize,
                       UA_EndpointDescription** endpointDescriptions) {
    if(client->state == UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_GOOD;
    if(client->state == UA_CLIENTSTATE_ERRORED)
        UA_Client_reset(client);


    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    *client->connection =
        client->config.connectionFunc(UA_ConnectionConfig_standard, serverUrl,
                                      client->config.logger);
    if(client->connection->state != UA_CONNECTION_OPENING) {
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    client->endpointUrl = UA_STRING_ALLOC(serverUrl);
    if(!client->endpointUrl.data) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    client->connection->localConf = client->config.localConnectionConfig;
    retval = HelAckHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SecureChannelHandshake(client, false);
    if(retval == UA_STATUSCODE_GOOD)
        retval = GetEndpoints(client, endpointDescriptionsSize, endpointDescriptions);

    /* always cleanup */
    cleanup:
    UA_Client_disconnect(client);
    UA_Client_reset(client);
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
UA_Client_connect(UA_Client *client, const char *endpointUrl) {
    if(client->state == UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_GOOD;
    if(client->state == UA_CLIENTSTATE_ERRORED) {
        UA_Client_reset(client);
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    *client->connection =
        client->config.connectionFunc(UA_ConnectionConfig_standard,
                                      endpointUrl, client->config.logger);
    if(client->connection->state != UA_CONNECTION_OPENING) {
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);
    if(!client->endpointUrl.data) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    client->connection->localConf = client->config.localConnectionConfig;
    retval = HelAckHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SecureChannelHandshake(client, false);
    if(retval == UA_STATUSCODE_GOOD)
        retval = EndpointsHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SessionHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = ActivateSession(client);
    if(retval == UA_STATUSCODE_GOOD) {
        client->connection->state = UA_CONNECTION_ESTABLISHED;
        client->state = UA_CLIENTSTATE_CONNECTED;
    } else {
        goto cleanup;
    }
    return retval;

    cleanup:
    UA_Client_reset(client);
    return retval;
}

UA_StatusCode UA_Client_disconnect(UA_Client *client) {
    if(client->state != UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_BADNOTCONNECTED;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    /* Is a session established? */
    if(client->connection->state == UA_CONNECTION_ESTABLISHED &&
       !UA_NodeId_equal(&client->authenticationToken, &UA_NODEID_NULL))
        retval = CloseSession(client);
    /* Is a secure channel established? */
    if(client->connection->state == UA_CONNECTION_ESTABLISHED)
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

struct ResponseDescription {
    UA_Client *client;
    UA_Boolean processed;
    UA_UInt32 requestId;
    void *response;
    const UA_DataType *responseType;
};

static void
processServiceResponse(struct ResponseDescription *rd, UA_SecureChannel *channel,
                       UA_MessageType messageType, UA_UInt32 requestId,
                       UA_ByteString *message) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)rd->response;
    rd->processed = true;

    /* Check that the request id matches */
    /* Todo: we need to demux async responses since a publish responses may come
       at any time */
    if(requestId != rd->requestId) {
        UA_LOG_ERROR(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                     "Reply answers the wrong requestId. "
                     "Async services are not yet implemented.");
        retval = UA_STATUSCODE_BADINTERNALERROR;
        goto finish;
    }

    /* Check that the response type matches */
    const UA_NodeId expectedNodeId =
        UA_NODEID_NUMERIC(0, rd->responseType->binaryEncodingId);
    const UA_NodeId serviceFaultNodeId =
        UA_NODEID_NUMERIC(0, UA_TYPES[UA_TYPES_SERVICEFAULT].binaryEncodingId);
    size_t offset = 0;
    UA_NodeId responseId;
    retval = UA_NodeId_decodeBinary(message, &offset, &responseId);
    if(retval != UA_STATUSCODE_GOOD)
        goto finish;
    if(!UA_NodeId_equal(&responseId, &expectedNodeId)) {
        if(UA_NodeId_equal(&responseId, &serviceFaultNodeId)) {
            /* Take the statuscode from the servicefault */
            retval = UA_decodeBinary(message, &offset, rd->response,
                                     &UA_TYPES[UA_TYPES_SERVICEFAULT]);
        } else {
            UA_LOG_ERROR(rd->client->config.logger, UA_LOGCATEGORY_CLIENT,
                         "Reply answers the wrong request. Expected ns=%i,i=%i."
                         "But retrieved ns=%i,i=%i", expectedNodeId.namespaceIndex,
                         expectedNodeId.identifier.numeric, responseId.namespaceIndex,
                         responseId.identifier.numeric);
            UA_NodeId_deleteMembers(&responseId);
            retval = UA_STATUSCODE_BADINTERNALERROR;
        }
        goto finish;
    }

    /* Decode the response */
    retval = UA_decodeBinary(message, &offset, rd->response, rd->responseType);

 finish:
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

void
__UA_Client_Service(UA_Client *client, const void *r, const UA_DataType *requestType,
                    void *response, const UA_DataType *responseType) {
    UA_init(response, responseType);
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;

    /* Make sure we have a valid session */
    UA_StatusCode retval = UA_Client_manuallyRenewSecureChannel(client);
    if(retval != UA_STATUSCODE_GOOD) {
        respHeader->serviceResult = retval;
        client->state = UA_CLIENTSTATE_ERRORED;
        return;
    }

    /* Handling request parameters */
    //here const *r is 'violated'
    UA_RequestHeader *request = (void*)(uintptr_t)r;
    UA_NodeId_copy(&client->authenticationToken, &request->authenticationToken);
    request->timestamp = UA_DateTime_now();
    request->requestHandle = ++client->requestHandle;

    /* Send the request */
    UA_UInt32 requestId = ++client->requestId;
    UA_LOG_DEBUG(client->config.logger, UA_LOGCATEGORY_CLIENT,
                 "Sending a request of type %i", requestType->typeId.identifier.numeric);
    retval = UA_SecureChannel_sendBinaryMessage(client->channel, requestId, request, requestType);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            respHeader->serviceResult = UA_STATUSCODE_BADREQUESTTOOLARGE;
        else
            respHeader->serviceResult = retval;
        client->state = UA_CLIENTSTATE_ERRORED;
        goto finish;
    }

    /* Requests always begin witih a RequestHeader, therefore we can cast. */
    UA_init(response, responseType);
    struct ResponseDescription rd = (struct ResponseDescription){
        client, false, requestId, response, responseType};

    /* Retrieve the response */
    UA_ByteString reply;
    UA_ByteString_init(&reply);
    UA_Boolean realloced = false;
    do {
        retval = client->connection->recv(client->connection, &reply, client->config.timeout);
        retval |= UA_Connection_completeMessages(client->connection, &reply, &realloced);
        if(retval != UA_STATUSCODE_GOOD) {
            respHeader->serviceResult = retval;
            client->state = UA_CLIENTSTATE_ERRORED;
            goto finish;
        }
        if(!reply.data)
            break;
        UA_SecureChannel_processChunks(client->channel, &reply,
                     (UA_ProcessMessageCallback*)processServiceResponse, &rd);
    } while(!rd.processed);

 finish:
    UA_NodeId_deleteMembers(&request->authenticationToken);
}
