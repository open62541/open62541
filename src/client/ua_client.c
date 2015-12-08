#include "ua_util.h"
#include "ua_client.h"
#include "ua_client_highlevel.h"
#include "ua_client_internal.h"
#include "ua_types_generated.h"
#include "ua_nodeids.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated_encoding_binary.h"

const UA_EXPORT UA_ClientConfig UA_ClientConfig_standard =
    { .timeout = 5000 /* ms receive timout */, .secureChannelLifeTime = 30000,
      .timeToRenewSecureChannel = 2000,
      {.protocolVersion = 0, .sendBufferSize = 65536, .recvBufferSize  = 65536,
       .maxMessageSize = 65536, .maxChunkCount = 1}};

/*********************/
/* Create and Delete */
/*********************/

static void UA_Client_init(UA_Client* client, UA_ClientConfig config,
                           UA_Logger logger) {
    client->state = UA_CLIENTSTATE_READY;
    UA_Connection_init(&client->connection);
    UA_SecureChannel_init(&client->channel);
    client->channel.connection = &client->connection;
    UA_String_init(&client->endpointUrl);
    client->requestId = 0;

    UA_NodeId_init(&client->authenticationToken);
    client->requestHandle = 0;

    client->logger = logger;
    client->config = config;
    client->scExpiresAt = 0;

#ifdef UA_ENABLE_SUBSCRIPTIONS
    client->monitoredItemHandles = 0;
    LIST_INIT(&client->pendingNotificationsAcks);
    LIST_INIT(&client->subscriptions);
#endif
}

UA_Client * UA_Client_new(UA_ClientConfig config, UA_Logger logger) {
    UA_Client *client = UA_calloc(1, sizeof(UA_Client));
    if(!client)
        return NULL;

    UA_Client_init(client, config, logger);
    return client;
}

static void UA_Client_deleteMembers(UA_Client* client) {
    UA_Client_disconnect(client);
    UA_Connection_deleteMembers(&client->connection);
    UA_SecureChannel_deleteMembersCleanup(&client->channel);
    if(client->endpointUrl.data)
        UA_String_deleteMembers(&client->endpointUrl);
    UA_UserTokenPolicy_deleteMembers(&client->token);
#ifdef UA_ENABLE_SUBSCRIPTIONS
    UA_Client_NotificationsAckNumber *n, *tmp;
    LIST_FOREACH_SAFE(n, &client->pendingNotificationsAcks, listEntry, tmp) {
        LIST_REMOVE(n, listEntry);
        free(n);
    }
    UA_Client_Subscription *sub, *tmps;
    LIST_FOREACH_SAFE(sub, &client->subscriptions, listEntry, tmps) {
        LIST_REMOVE(sub, listEntry);
        UA_Client_MonitoredItem *mon, *tmpmon;
        LIST_FOREACH_SAFE(mon, &sub->MonitoredItems, listEntry, tmpmon) {
            UA_Client_Subscriptions_removeMonitoredItem(client, sub->SubscriptionID,
                                                        mon->MonitoredItemId);
        }
        free(sub);
    }
#endif
}

void UA_Client_reset(UA_Client* client){
    UA_Client_deleteMembers(client);
    UA_Client_init(client, client->config, client->logger);
}

void UA_Client_delete(UA_Client* client){
    if(client->state != UA_CLIENTSTATE_READY)
        UA_Client_deleteMembers(client);
    UA_free(client);
}

/*************************/
/* Manage the Connection */
/*************************/

static UA_StatusCode HelAckHandshake(UA_Client *c) {
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_HELF;

    UA_TcpHelloMessage hello;
    UA_String_copy(&c->endpointUrl, &hello.endpointUrl); /* must be less than 4096 bytes */

    UA_Connection *conn = &c->connection;
    hello.maxChunkCount = conn->localConf.maxChunkCount;
    hello.maxMessageSize = conn->localConf.maxMessageSize;
    hello.protocolVersion = conn->localConf.protocolVersion;
    hello.receiveBufferSize = conn->localConf.recvBufferSize;
    hello.sendBufferSize = conn->localConf.sendBufferSize;

    UA_ByteString message;
    UA_StatusCode retval;
    retval = c->connection.getSendBuffer(&c->connection, c->connection.remoteConf.recvBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t offset = 8;
    retval |= UA_TcpHelloMessage_encodeBinary(&hello, &message, &offset);
    messageHeader.messageSize = offset;
    offset = 0;
    retval |= UA_TcpMessageHeader_encodeBinary(&messageHeader, &message, &offset);
    UA_TcpHelloMessage_deleteMembers(&hello);
    if(retval != UA_STATUSCODE_GOOD) {
        c->connection.releaseSendBuffer(&c->connection, &message);
        return retval;
    }

    message.length = messageHeader.messageSize;
    retval = c->connection.send(&c->connection, &message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(c->logger, UA_LOGCATEGORY_NETWORK, "Sending HEL failed");
        return retval;
    }
    UA_LOG_DEBUG(c->logger, UA_LOGCATEGORY_NETWORK, "Sent HEL message");

    UA_ByteString reply;
    UA_ByteString_init(&reply);
    UA_Boolean realloced = UA_FALSE;
    do {
        retval = c->connection.recv(&c->connection, &reply, c->config.timeout);
        retval |= UA_Connection_completeMessages(&c->connection, &reply, &realloced);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(c->logger, UA_LOGCATEGORY_NETWORK, "Receiving ACK message failed");
            return retval;
        }
    } while(reply.length == 0);

    offset = 0;
    UA_TcpMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    UA_TcpAcknowledgeMessage ackMessage;
    retval = UA_TcpAcknowledgeMessage_decodeBinary(&reply, &offset, &ackMessage);
    if(!realloced)
        c->connection.releaseRecvBuffer(&c->connection, &reply);
    else
        UA_ByteString_deleteMembers(&reply);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(c->logger, UA_LOGCATEGORY_NETWORK, "Decoding ACK message failed");
        return retval;
    }
    UA_LOG_DEBUG(c->logger, UA_LOGCATEGORY_NETWORK, "Received ACK message");

    conn->remoteConf.maxChunkCount = ackMessage.maxChunkCount;
    conn->remoteConf.maxMessageSize = ackMessage.maxMessageSize;
    conn->remoteConf.protocolVersion = ackMessage.protocolVersion;
    conn->remoteConf.recvBufferSize = ackMessage.receiveBufferSize;
    conn->remoteConf.sendBufferSize = ackMessage.sendBufferSize;
    conn->state = UA_CONNECTION_ESTABLISHED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode SecureChannelHandshake(UA_Client *client, UA_Boolean renew) {
    /* Check if sc is still valid */
    if(renew && client->scExpiresAt - UA_DateTime_now() > client->config.timeToRenewSecureChannel * 10000)
        return UA_STATUSCODE_GOOD;

    UA_Connection *c = &client->connection;
    if(c->state != UA_CONNECTION_ESTABLISHED)
        return UA_STATUSCODE_BADSERVERNOTCONNECTED;

    UA_SecureConversationMessageHeader messageHeader;
    messageHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_OPNF;
    messageHeader.secureChannelId = 0;

    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = ++client->channel.sequenceNumber;
    seqHeader.requestId = ++client->requestId;

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
    asymHeader.securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#None");

    /* id of opensecurechannelrequest */
    UA_NodeId requestType = UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELREQUEST + UA_ENCODINGOFFSET_BINARY);

    UA_OpenSecureChannelRequest opnSecRq;
    UA_OpenSecureChannelRequest_init(&opnSecRq);
    opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    opnSecRq.requestHeader.authenticationToken = client->authenticationToken;
    opnSecRq.requestedLifetime = client->config.secureChannelLifeTime;
    if(renew) {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_RENEW;
        UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL, "Requesting to renew the SecureChannel");
    } else {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_ByteString_init(&client->channel.clientNonce);
        UA_ByteString_copy(&client->channel.clientNonce, &opnSecRq.clientNonce);
        opnSecRq.securityMode = UA_MESSAGESECURITYMODE_NONE;
        UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL, "Requesting to open a SecureChannel");
    }

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
    messageHeader.messageHeader.messageSize = offset;
    offset = 0;
    retval |= UA_SecureConversationMessageHeader_encodeBinary(&messageHeader, &message, &offset);

    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);
    if(retval != UA_STATUSCODE_GOOD) {
        client->connection.releaseSendBuffer(&client->connection, &message);
        return retval;
    }

    message.length = messageHeader.messageHeader.messageSize;
    retval = client->connection.send(&client->connection, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_ByteString reply;
    UA_ByteString_init(&reply);
    UA_Boolean realloced = UA_FALSE;
    do {
        retval = c->recv(c, &reply, client->config.timeout);
        retval |= UA_Connection_completeMessages(c, &reply, &realloced);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL,
                         "Receiving OpenSecureChannelResponse failed");
            return retval;
        }
    } while(reply.length == 0);

    offset = 0;
    UA_SecureConversationMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&reply, &offset, &asymHeader);
    UA_SequenceHeader_decodeBinary(&reply, &offset, &seqHeader);
    UA_NodeId_decodeBinary(&reply, &offset, &requestType);
    UA_NodeId expectedRequest = UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELRESPONSE +
                                                  UA_ENCODINGOFFSET_BINARY);
    if(!UA_NodeId_equal(&requestType, &expectedRequest)) {
        UA_ByteString_deleteMembers(&reply);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_NodeId_deleteMembers(&requestType);
        UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_CLIENT,
                     "Reply answers the wrong request. Expected OpenSecureChannelResponse.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_OpenSecureChannelResponse response;
    UA_OpenSecureChannelResponse_init(&response);
    retval = UA_OpenSecureChannelResponse_decodeBinary(&reply, &offset, &response);
    if(!realloced)
        c->releaseRecvBuffer(c, &reply);
    else
        UA_ByteString_deleteMembers(&reply);
        
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Decoding OpenSecureChannelResponse failed");
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_OpenSecureChannelResponse_init(&response);
        response.responseHeader.serviceResult = retval;
        return retval;
    }

    client->scExpiresAt = UA_DateTime_now() + response.securityToken.revisedLifetime * 10000;
    retval = response.responseHeader.serviceResult;

    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "SecureChannel could not be opened / renewed");
    else if(!renew) {
        UA_ChannelSecurityToken_copy(&response.securityToken, &client->channel.securityToken);
        /* if the handshake is repeated, replace the old nonce */
        UA_ByteString_deleteMembers(&client->channel.serverNonce);
        UA_ByteString_copy(&response.serverNonce, &client->channel.serverNonce);
        UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL, "SecureChannel opened");
    } else
        UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL, "SecureChannel renewed");

    UA_OpenSecureChannelResponse_deleteMembers(&response);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    return retval;
}

static UA_StatusCode ActivateSession(UA_Client *client) {
    UA_ActivateSessionRequest request;
    UA_ActivateSessionRequest_init(&request);

    request.requestHeader.requestHandle = 2; //TODO: is it a magic number?
    request.requestHeader.authenticationToken = client->authenticationToken;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    UA_AnonymousIdentityToken identityToken;
    UA_AnonymousIdentityToken_init(&identityToken);
    UA_String_copy(&client->token.policyId, &identityToken.policyId);

    //manual ExtensionObject encoding of the identityToken
    request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    request.userIdentityToken.content.decoded.type = &UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN];
    request.userIdentityToken.content.decoded.data = &identityToken;

    UA_ActivateSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);

    UA_AnonymousIdentityToken_deleteMembers(&identityToken);
    UA_ActivateSessionRequest_deleteMembers(&request);
    UA_ActivateSessionResponse_deleteMembers(&response);
    return response.responseHeader.serviceResult; // not deleted
}

static UA_StatusCode EndpointsHandshake(UA_Client *client) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    request.requestHeader.authenticationToken = client->authenticationToken;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.endpointUrl = client->endpointUrl;
    UA_String profileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");
    request.profileUris = &profileUri;
    request.profileUrisSize = 1;

    UA_GetEndpointsResponse response;
    UA_GetEndpointsResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                        &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(client->logger, UA_LOGCATEGORY_CLIENT, "GetEndpointRequest failed");
        return response.responseHeader.serviceResult;
    }

    UA_Boolean endpointFound = UA_FALSE;
    UA_Boolean tokenFound = UA_FALSE;
    UA_String securityNone = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    for(size_t i = 0; i < response.endpointsSize; i++) {
        UA_EndpointDescription* endpoint = &response.endpoints[i];
        /* look out for an endpoint without security */
        if(!UA_String_equal(&endpoint->securityPolicyUri, &securityNone))
            continue;
        endpointFound = UA_TRUE;
        /* endpoint with no security found */
        /* look for a user token policy with an anonymous token */
        for(size_t j = 0; j < endpoint->userIdentityTokensSize; ++j) {
            UA_UserTokenPolicy* userToken = &endpoint->userIdentityTokens[j];
            if(userToken->tokenType != UA_USERTOKENTYPE_ANONYMOUS)
                continue;
            tokenFound = UA_TRUE;
            UA_UserTokenPolicy_copy(userToken, &client->token);
            break;
        }
    }

    UA_GetEndpointsResponse_deleteMembers(&response);
    if(!endpointFound) {
        UA_LOG_ERROR(client->logger, UA_LOGCATEGORY_CLIENT, "No suitable endpoint found");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(!tokenFound) {
        UA_LOG_ERROR(client->logger, UA_LOGCATEGORY_CLIENT, "No anonymous token found");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return response.responseHeader.serviceResult;
}

static UA_StatusCode SessionHandshake(UA_Client *client) {
    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);

    // todo: is this needed for all requests?
    UA_NodeId_copy(&client->authenticationToken, &request.requestHeader.authenticationToken);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->channel.clientNonce, &request.clientNonce);
    request.requestedSessionTimeout = 1200000;
    request.maxResponseMessageSize = UA_INT32_MAX;

    UA_CreateSessionResponse response;
    UA_CreateSessionResponse_init(&response);
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE]);

    UA_NodeId_copy(&response.authenticationToken, &client->authenticationToken);

    UA_CreateSessionRequest_deleteMembers(&request);
    UA_CreateSessionResponse_deleteMembers(&response);
    return response.responseHeader.serviceResult; // not deleted
}

static UA_StatusCode CloseSession(UA_Client *client) {
    UA_CloseSessionRequest request;
    UA_CloseSessionRequest_init(&request);

    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.deleteSubscriptions = UA_TRUE;
    UA_NodeId_copy(&client->authenticationToken, &request.requestHeader.authenticationToken);
    UA_CloseSessionResponse response;
    __UA_Client_Service(client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                        &response, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE]);

    UA_CloseSessionRequest_deleteMembers(&request);
    UA_CloseSessionResponse_deleteMembers(&response);
    return response.responseHeader.serviceResult; // not deleted
}

static UA_StatusCode CloseSecureChannel(UA_Client *client) {
    UA_SecureChannel *channel = &client->channel;
    UA_CloseSecureChannelRequest request;
    UA_CloseSecureChannelRequest_init(&request);
    request.requestHeader.requestHandle = 1; //TODO: magic number?
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.requestHeader.authenticationToken = client->authenticationToken;

    UA_SecureConversationMessageHeader msgHeader;
    msgHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_CLOF;
    msgHeader.secureChannelId = client->channel.securityToken.channelId;

    UA_SymmetricAlgorithmSecurityHeader symHeader;
    symHeader.tokenId = channel->securityToken.tokenId;
    
    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = ++channel->sequenceNumber;
    seqHeader.requestId = ++client->requestId;

    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CLOSESECURECHANNELREQUEST + UA_ENCODINGOFFSET_BINARY);

    UA_ByteString message;
    UA_Connection *c = &client->connection;
    UA_StatusCode retval = c->getSendBuffer(c, c->remoteConf.recvBufferSize, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t offset = 12;
    retval |= UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symHeader, &message, &offset);
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &message, &offset);
    retval |= UA_NodeId_encodeBinary(&typeId, &message, &offset);
    retval |= UA_encodeBinary(&request, &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST], &message, &offset);

    msgHeader.messageHeader.messageSize = offset;
    offset = 0;
    retval |= UA_SecureConversationMessageHeader_encodeBinary(&msgHeader, &message, &offset);

    if(retval != UA_STATUSCODE_GOOD) {
        client->connection.releaseSendBuffer(&client->connection, &message);
        return retval;
    }
        
    message.length = msgHeader.messageHeader.messageSize;
    retval = client->connection.send(&client->connection, &message);
    return retval;
}

UA_StatusCode UA_Client_connect(UA_Client *client, UA_ConnectClientConnection connectFunc,
                                const char *endpointUrl) {
    if(client->state == UA_CLIENTSTATE_CONNECTED)
        return UA_STATUSCODE_GOOD;
    if(client->state == UA_CLIENTSTATE_ERRORED) {
        UA_Client_reset(client);
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    client->connection = connectFunc(UA_ConnectionConfig_standard, endpointUrl, client->logger);
    if(client->connection.state != UA_CONNECTION_OPENING) {
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);
    if(client->endpointUrl.data == NULL) {
        retval = UA_STATUSCODE_BADOUTOFMEMORY;
        goto cleanup;
    }

    client->connection.localConf = client->config.localConnectionConfig;
    retval = HelAckHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SecureChannelHandshake(client, UA_FALSE);
    if(retval == UA_STATUSCODE_GOOD)
        retval = EndpointsHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SessionHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = ActivateSession(client);
    if(retval == UA_STATUSCODE_GOOD) {
        client->connection.state = UA_CONNECTION_ESTABLISHED;
        client->state = UA_CLIENTSTATE_CONNECTED;
    } else {
        goto cleanup;
    }
    return retval;

    cleanup:
    client->state = UA_CLIENTSTATE_ERRORED;
    UA_Client_reset(client);
    return retval;
}

UA_StatusCode UA_Client_disconnect(UA_Client *client) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(client->channel.connection->state == UA_CONNECTION_ESTABLISHED){
        retval = CloseSession(client);
        if(retval == UA_STATUSCODE_GOOD)
            retval = CloseSecureChannel(client);
    }
    return retval;
}

UA_StatusCode UA_Client_manuallyRenewSecureChannel(UA_Client *client) {
    return SecureChannelHandshake(client, UA_TRUE);
}

/****************/
/* Raw Services */
/****************/

void __UA_Client_Service(UA_Client *client, const void *r, const UA_DataType *requestType,
                         void *response, const UA_DataType *responseType) {
    /* Requests always begin witih a RequestHeader, therefore we can cast. */
    UA_RequestHeader *request = (void*)(uintptr_t)r;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_init(response, responseType);
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;
    
    /* make sure we have a valid session */
    retval = UA_Client_manuallyRenewSecureChannel(client);
    if(retval != UA_STATUSCODE_GOOD) {
        respHeader->serviceResult = retval;
        client->state = UA_CLIENTSTATE_ERRORED;
        return;
    }

    /* handling request parameters */
    UA_NodeId_copy(&client->authenticationToken, &request->authenticationToken);
    request->timestamp = UA_DateTime_now();
    request->requestHandle = ++client->requestHandle;

    /* Send the request */
    UA_UInt32 requestId = ++client->requestId;
    UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_CLIENT,
                 "Sending a request of type %i", requestType->typeId.identifier.numeric);
    retval = UA_SecureChannel_sendBinaryMessage(&client->channel, requestId, request, requestType);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
            respHeader->serviceResult = UA_STATUSCODE_BADREQUESTTOOLARGE;
        else
            respHeader->serviceResult = retval;
        client->state = UA_CLIENTSTATE_ERRORED;
        return;
    }

    /* Retrieve the response */
    // Todo: push this into the generic securechannel implementation for client and server
    UA_ByteString reply;
    UA_ByteString_init(&reply);
    UA_Boolean realloced = UA_FALSE;
    do {
        retval = client->connection.recv(&client->connection, &reply, client->config.timeout);
        retval |= UA_Connection_completeMessages(&client->connection, &reply, &realloced);
        if(retval != UA_STATUSCODE_GOOD) {
            respHeader->serviceResult = retval;
            client->state = UA_CLIENTSTATE_ERRORED;
            return;
        }
    } while(!reply.data);

    size_t offset = 0;
    UA_SecureConversationMessageHeader msgHeader;
    retval |= UA_SecureConversationMessageHeader_decodeBinary(&reply, &offset, &msgHeader);
    UA_SymmetricAlgorithmSecurityHeader symHeader;
    retval |= UA_SymmetricAlgorithmSecurityHeader_decodeBinary(&reply, &offset, &symHeader);
    UA_SequenceHeader seqHeader;
    retval |= UA_SequenceHeader_decodeBinary(&reply, &offset, &seqHeader);
    UA_NodeId responseId;
    retval |= UA_NodeId_decodeBinary(&reply, &offset, &responseId);
    UA_NodeId expectedNodeId = UA_NODEID_NUMERIC(0, responseType->typeId.identifier.numeric +
                                                 UA_ENCODINGOFFSET_BINARY);

    if(retval != UA_STATUSCODE_GOOD)
        goto finish;

    /* Todo: we need to demux responses since a publish responses may come at any time */
    if(!UA_NodeId_equal(&responseId, &expectedNodeId) || seqHeader.requestId != requestId) {
        if(responseId.identifier.numeric != UA_NS0ID_SERVICEFAULT + UA_ENCODINGOFFSET_BINARY) {
            UA_LOG_ERROR(client->logger, UA_LOGCATEGORY_CLIENT,
                         "Reply answers the wrong request. Expected ns=%i,i=%i. But retrieved ns=%i,i=%i",
                         expectedNodeId.namespaceIndex, expectedNodeId.identifier.numeric,
                         responseId.namespaceIndex, responseId.identifier.numeric);
            respHeader->serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        } else
            retval = UA_decodeBinary(&reply, &offset, respHeader, &UA_TYPES[UA_TYPES_SERVICEFAULT]);
        goto finish;
    } 
    
    retval = UA_decodeBinary(&reply, &offset, response, responseType);
    if(retval == UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED)
        retval = UA_STATUSCODE_BADRESPONSETOOLARGE;

 finish:
    UA_SymmetricAlgorithmSecurityHeader_deleteMembers(&symHeader);
    if(!realloced)
        client->connection.releaseRecvBuffer(&client->connection, &reply);
    else
        UA_ByteString_deleteMembers(&reply);
    if(retval != UA_STATUSCODE_GOOD){
        UA_LOG_INFO(client->logger, UA_LOGCATEGORY_CLIENT, "Error receiving the response");
        client->state = UA_CLIENTSTATE_ERRORED;
        respHeader->serviceResult = retval;
    }
    UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_CLIENT,
                 "Received a response of type %i", responseId.identifier.numeric);
}
