#include "ua_util.h"
#include "ua_client.h"
#include "ua_types_generated.h"
#include "ua_nodeids.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"
#include "ua_types_generated_encoding_binary.h"
#include "ua_transport_generated_encoding_binary.h"
#include "ua_client_internal.h"

typedef enum {
       UA_CLIENTSTATE_READY,
       UA_CLIENTSTATE_CONNECTED,
       UA_CLIENTSTATE_ERRORED
} UA_Client_State;

struct UA_Client {
    /* State */ //maybe it should be visible to user
    UA_Client_State state;

    /* Connection */
    UA_Connection connection;
    UA_SecureChannel channel;
    UA_String endpointUrl;
    UA_UInt32 requestId;

    /* Session */
    UA_UserTokenPolicy token;
    UA_NodeId sessionId;
    UA_NodeId authenticationToken;
    UA_UInt32 requestHandle;
    
#ifdef ENABLE_SUBSCRIPTIONS
    UA_Int32 monitoredItemHandles;
    LIST_HEAD(UA_ListOfUnacknowledgedNotificationNumbers, UA_Client_NotificationsAckNumber_s) pendingNotificationsAcks;
    LIST_HEAD(UA_ListOfClientSubscriptionItems, UA_Client_Subscription_s) subscriptions;
#endif
    
    /* Config */
    UA_Logger logger;
    UA_ClientConfig config;
    UA_DateTime scExpiresAt;
};

const UA_EXPORT UA_ClientConfig UA_ClientConfig_standard =
    { .timeout = 500 /* ms receive timout */, .secureChannelLifeTime = 30000, .timeToRenewSecureChannel = 2000,
      {.protocolVersion = 0, .sendBufferSize = 65536, .recvBufferSize  = 65536,
       .maxMessageSize = 65536, .maxChunkCount = 1}};

UA_Client * UA_Client_new(UA_ClientConfig config, UA_Logger logger) {
    UA_Client *client = UA_calloc(1, sizeof(UA_Client));
    if(!client)
        return UA_NULL;

    UA_Client_init(client, config, logger);
    return client;
}

void UA_Client_reset(UA_Client* client){
    UA_Client_deleteMembers(client);
    UA_Client_init(client, client->config, client->logger);
}

void UA_Client_init(UA_Client* client, UA_ClientConfig config, UA_Logger logger){
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

#ifdef ENABLE_SUBSCRIPTIONS
    client->monitoredItemHandles = 0;
    LIST_INIT(&client->pendingNotificationsAcks);
    LIST_INIT(&client->subscriptions);
#endif
}

void UA_Client_deleteMembers(UA_Client* client){
    if(client->state == UA_CLIENTSTATE_READY) //initialized client has no dynamic memory allocated
        return;
    UA_Connection_deleteMembers(&client->connection);
    UA_SecureChannel_deleteMembersCleanup(&client->channel);
    if(client->endpointUrl.data)
        UA_String_deleteMembers(&client->endpointUrl);
    UA_UserTokenPolicy_deleteMembers(&client->token);
}
void UA_Client_delete(UA_Client* client){
    if(client->state != UA_CLIENTSTATE_READY)
        UA_Client_deleteMembers(client);
    UA_free(client);
}

static UA_StatusCode
HelAckHandshake(UA_Client *c) {
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
        UA_LOG_DEBUG(c->logger, UA_LOGCATEGORY_NETWORK, "Sending HEL failed");
        return retval;
    }
    UA_LOG_DEBUG(c->logger, UA_LOGCATEGORY_NETWORK, "Sent HEL message");

    UA_ByteString reply;
    UA_ByteString_init(&reply);
    do {
        retval = c->connection.recv(&c->connection, &reply, c->config.timeout);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG(c->logger, UA_LOGCATEGORY_NETWORK, "Receiving ACK message failed");
            return retval;
        }
    } while(!reply.data);

    offset = 0;
    UA_TcpMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    UA_TcpAcknowledgeMessage ackMessage;
    retval = UA_TcpAcknowledgeMessage_decodeBinary(&reply, &offset, &ackMessage);
    UA_ByteString_deleteMembers(&reply);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(c->logger, UA_LOGCATEGORY_NETWORK, "Decoding ACK message failed");
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
    if(renew && client->scExpiresAt - UA_DateTime_now() > client->config.timeToRenewSecureChannel * 10000 ){
        return UA_STATUSCODE_GOOD;
    }

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
    UA_Connection *c = &client->connection;
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
    do {
        retval = client->connection.recv(&client->connection, &reply, client->config.timeout);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL, "Receiving OpenSecureChannelResponse failed");
            return retval;
        }
    } while(!reply.data);

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
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_SECURECHANNEL,
                     "Decoding OpenSecureChannelResponse failed");
        UA_ByteString_deleteMembers(&reply);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_OpenSecureChannelResponse_init(&response);
        response.responseHeader.serviceResult = retval;
        return retval;
    }
    client->scExpiresAt = UA_DateTime_now() + response.securityToken.revisedLifetime * 10000;
    UA_ByteString_deleteMembers(&reply);
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

/** If the request fails, then the response is cast to UA_ResponseHeader (at the beginning of every
    response) and filled with theaappropriate error code */
static void synchronousRequest(UA_Client *client, void *r, const UA_DataType *requestType,
                               void *response, const UA_DataType *responseType) {
    /* Requests always begin witih a RequestHeader, therefore we can cast. */
    UA_RequestHeader *request = r;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!response)
        return;
    UA_init(response, responseType);
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;
    //make sure we have a valid session
    retval = UA_Client_renewSecureChannel(client);
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
    if(retval) {
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
    do {
        retval = client->connection.recv(&client->connection, &reply, client->config.timeout);
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

    if(retval != UA_STATUSCODE_GOOD) {
        goto finish;
    }

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
    UA_ByteString_deleteMembers(&reply);
    if(retval != UA_STATUSCODE_GOOD){
        UA_LOG_INFO(client->logger, UA_LOGCATEGORY_CLIENT, "Error receiving the response");
        client->state = UA_CLIENTSTATE_ERRORED;
        respHeader->serviceResult = retval;
    }
    UA_LOG_DEBUG(client->logger, UA_LOGCATEGORY_CLIENT, "Received a response of type %i", responseId.identifier.numeric);
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
    request.userIdentityToken.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
    request.userIdentityToken.typeId = UA_TYPES[UA_TYPES_ANONYMOUSIDENTITYTOKEN].typeId;
    request.userIdentityToken.typeId.identifier.numeric+=UA_ENCODINGOFFSET_BINARY;
    
    if (identityToken.policyId.length >= 0)
      UA_ByteString_newMembers(&request.userIdentityToken.body, identityToken.policyId.length+4);
    else {
      identityToken.policyId.length = -1;
      UA_ByteString_newMembers(&request.userIdentityToken.body, 4);
    }
    
    size_t offset = 0;
    UA_ByteString_encodeBinary(&identityToken.policyId,&request.userIdentityToken.body,&offset);

    UA_ActivateSessionResponse response;
    synchronousRequest(client, &request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                       &response, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE]);

    UA_AnonymousIdentityToken_deleteMembers(&identityToken);
    UA_ActivateSessionRequest_deleteMembers(&request);
    UA_ActivateSessionResponse_deleteMembers(&response);
    return response.responseHeader.serviceResult; // not deleted
}

static UA_StatusCode EndpointsHandshake(UA_Client *client) {
    UA_GetEndpointsRequest request;
    UA_GetEndpointsRequest_init(&request);
    UA_NodeId_copy(&client->authenticationToken, &request.requestHeader.authenticationToken);
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    UA_String_copy(&client->endpointUrl, &request.endpointUrl);
    request.profileUrisSize = 1;
    request.profileUris = UA_Array_new(&UA_TYPES[UA_TYPES_STRING], request.profileUrisSize);
    *request.profileUris = UA_STRING_ALLOC("http://opcfoundation.org/UA-Profile/Transport/uatcp-uasc-uabinary");

    UA_GetEndpointsResponse response;
    UA_GetEndpointsResponse_init(&response);
    synchronousRequest(client, &request, &UA_TYPES[UA_TYPES_GETENDPOINTSREQUEST],
                       &response, &UA_TYPES[UA_TYPES_GETENDPOINTSRESPONSE]);

    UA_Boolean endpointFound = UA_FALSE;
    UA_Boolean tokenFound = UA_FALSE;
    UA_String securityNone = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");
    for(UA_Int32 i=0; i<response.endpointsSize; ++i){
        UA_EndpointDescription* endpoint = &response.endpoints[i];
        /* look out for an endpoint without security */
        if(!UA_String_equal(&endpoint->securityPolicyUri,
                            &securityNone))
            continue;
        endpointFound = UA_TRUE;
        /* endpoint with no security found */
        /* look for a user token policy with an anonymous token */
        for(UA_Int32 j=0; j<endpoint->userIdentityTokensSize; ++j) {
            UA_UserTokenPolicy* userToken = &endpoint->userIdentityTokens[j];
            if(userToken->tokenType != UA_USERTOKENTYPE_ANONYMOUS)
                continue;
            tokenFound = UA_TRUE;
            UA_UserTokenPolicy_copy(userToken, &client->token);
            break;
        }
    }

    UA_GetEndpointsRequest_deleteMembers(&request);
    UA_GetEndpointsResponse_deleteMembers(&response);

    if(!endpointFound){
        UA_LOG_ERROR(client->logger, UA_LOGCATEGORY_CLIENT, "No suitable endpoint found");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(!tokenFound){
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
    synchronousRequest(client, &request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
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
    synchronousRequest(client, &request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
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

/*************************/
/* User-Facing Functions */
/*************************/

UA_StatusCode UA_Client_connect(UA_Client *client, UA_ConnectClientConnection connectFunc, char *endpointUrl) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /** make the function more convenient to the end-user **/
    if(client->state == UA_CLIENTSTATE_CONNECTED){
        UA_Client_disconnect(client);
    }
    if(client->state == UA_CLIENTSTATE_ERRORED){
        UA_Client_reset(client);
    }

    client->connection = connectFunc(UA_ConnectionConfig_standard, endpointUrl, client->logger);
    if(client->connection.state != UA_CONNECTION_OPENING){
        retval = UA_STATUSCODE_BADCONNECTIONCLOSED;
        goto cleanup;
    }

    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);
    if(client->endpointUrl.length < 0){
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
    if(retval == UA_STATUSCODE_GOOD){
        client->connection.state = UA_CONNECTION_ESTABLISHED;
        client->state = UA_CLIENTSTATE_CONNECTED;
    }else{
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
    UA_Client_reset(client);
    return retval;
}

UA_StatusCode UA_Client_renewSecureChannel(UA_Client *client) {
    return SecureChannelHandshake(client, UA_TRUE);
}

UA_ReadResponse UA_Client_read(UA_Client *client, UA_ReadRequest *request) {
    UA_ReadResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_READREQUEST], &response,
                       &UA_TYPES[UA_TYPES_READRESPONSE]);
    return response;
}

UA_WriteResponse UA_Client_write(UA_Client *client, UA_WriteRequest *request) {
    UA_WriteResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_WRITEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_WRITERESPONSE]);
    return response;
}

UA_BrowseResponse UA_Client_browse(UA_Client *client, UA_BrowseRequest *request) {
    UA_BrowseResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_BROWSEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_BROWSERESPONSE]);
    return response;
}

UA_BrowseNextResponse UA_Client_browseNext(UA_Client *client, UA_BrowseNextRequest *request) {
    UA_BrowseNextResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_BROWSENEXTREQUEST], &response,
                       &UA_TYPES[UA_TYPES_BROWSENEXTRESPONSE]);
    return response;
}

UA_TranslateBrowsePathsToNodeIdsResponse
    UA_Client_translateTranslateBrowsePathsToNodeIds(UA_Client *client,
                                                     UA_TranslateBrowsePathsToNodeIdsRequest *request) {
    UA_TranslateBrowsePathsToNodeIdsResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSREQUEST],
                       &response, &UA_TYPES[UA_TYPES_TRANSLATEBROWSEPATHSTONODEIDSRESPONSE]);
    return response;
}

UA_AddNodesResponse UA_Client_addNodes(UA_Client *client, UA_AddNodesRequest *request) {
    UA_AddNodesResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_ADDNODESREQUEST],
                       &response, &UA_TYPES[UA_TYPES_ADDNODESRESPONSE]);
    return response;
}

UA_AddReferencesResponse UA_Client_addReferences(UA_Client *client, UA_AddReferencesRequest *request) {
    UA_AddReferencesResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_ADDREFERENCESREQUEST],
                       &response, &UA_TYPES[UA_TYPES_ADDREFERENCESRESPONSE]);
    return response;
}

UA_DeleteNodesResponse UA_Client_deleteNodes(UA_Client *client, UA_DeleteNodesRequest *request) {
    UA_DeleteNodesResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_DELETENODESREQUEST],
                       &response, &UA_TYPES[UA_TYPES_DELETENODESRESPONSE]);
    return response;
}

UA_DeleteReferencesResponse UA_Client_deleteReferences(UA_Client *client, UA_DeleteReferencesRequest *request) {
    UA_DeleteReferencesResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_DELETEREFERENCESREQUEST],
                       &response, &UA_TYPES[UA_TYPES_DELETEREFERENCESRESPONSE]);
    return response;
}

#ifdef ENABLE_SUBSCRIPTIONS
UA_CreateSubscriptionResponse UA_Client_createSubscription(UA_Client *client, UA_CreateSubscriptionRequest *request) {
    UA_CreateSubscriptionResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONREQUEST],
                       &response, &UA_TYPES[UA_TYPES_CREATESUBSCRIPTIONRESPONSE]);
    return response;
}

UA_DeleteSubscriptionsResponse UA_Client_deleteSubscriptions(UA_Client *client, UA_DeleteSubscriptionsRequest *request) {
    UA_DeleteSubscriptionsResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSREQUEST],
                       &response, &UA_TYPES[UA_TYPES_DELETESUBSCRIPTIONSRESPONSE]);
    return response;
}

UA_ModifySubscriptionResponse UA_Client_modifySubscription(UA_Client *client, UA_ModifySubscriptionRequest *request) {
    UA_ModifySubscriptionResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONREQUEST],
                       &response, &UA_TYPES[UA_TYPES_MODIFYSUBSCRIPTIONRESPONSE]);
    return response;
}

UA_CreateMonitoredItemsResponse UA_Client_createMonitoredItems(UA_Client *client, UA_CreateMonitoredItemsRequest *request) {
    UA_CreateMonitoredItemsResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSREQUEST],
                       &response, &UA_TYPES[UA_TYPES_CREATEMONITOREDITEMSRESPONSE]);
    return response;
}

UA_DeleteMonitoredItemsResponse UA_Client_deleteMonitoredItems(UA_Client *client, UA_DeleteMonitoredItemsRequest *request) {
    UA_DeleteMonitoredItemsResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSREQUEST],
                       &response, &UA_TYPES[UA_TYPES_DELETEMONITOREDITEMSRESPONSE]);
    return response;
}

UA_PublishResponse UA_Client_publish(UA_Client *client, UA_PublishRequest *request) {
    UA_PublishResponse response;
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_PUBLISHREQUEST],
                       &response, &UA_TYPES[UA_TYPES_PUBLISHRESPONSE]);
    return response;
}

UA_Int32 UA_Client_newSubscription(UA_Client *client, UA_Int32 publishInterval) {
    UA_Int32 retval;
    UA_CreateSubscriptionRequest aReq;
    UA_CreateSubscriptionResponse aRes;
    UA_CreateSubscriptionRequest_init(&aReq);
    UA_CreateSubscriptionResponse_init(&aRes);
    
    aReq.maxNotificationsPerPublish = 10;
    aReq.priority = 0;
    aReq.publishingEnabled = UA_TRUE;
    aReq.requestedLifetimeCount = 100;
    aReq.requestedMaxKeepAliveCount = 10;
    aReq.requestedPublishingInterval = publishInterval;
    
    aRes = UA_Client_createSubscription(client, &aReq);
    
    if (aRes.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        UA_Client_Subscription *newSub = UA_malloc(sizeof(UA_Client_Subscription));
        LIST_INIT(&newSub->MonitoredItems);
        
        newSub->LifeTime = aRes.revisedLifetimeCount;
        newSub->KeepAliveCount = aRes.revisedMaxKeepAliveCount;
        newSub->PublishingInterval = aRes.revisedPublishingInterval;
        newSub->SubscriptionID = aRes.subscriptionId;
        newSub->NotificationsPerPublish = aReq.maxNotificationsPerPublish;
        newSub->Priority = aReq.priority;
        retval = newSub->SubscriptionID;
        LIST_INSERT_HEAD(&(client->subscriptions), newSub, listEntry);
    } else
        retval = 0;
    
    UA_CreateSubscriptionResponse_deleteMembers(&aRes);
    UA_CreateSubscriptionRequest_deleteMembers(&aReq);
    return retval;
}

UA_StatusCode UA_Client_removeSubscription(UA_Client *client, UA_UInt32 subscriptionId) {
    UA_Client_Subscription *sub;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    LIST_FOREACH(sub, &(client->subscriptions), listEntry) {
        if (sub->SubscriptionID == subscriptionId)
            break;
    }
    
    // Problem? We do not have this subscription registeres. Maybe the server should
    // be consulted at this point?
    if (sub == NULL)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    UA_DeleteSubscriptionsRequest  request;
    UA_DeleteSubscriptionsResponse response;
    UA_DeleteSubscriptionsRequest_init(&request);
    UA_DeleteSubscriptionsResponse_init(&response);
    
    request.subscriptionIdsSize=1;
    request.subscriptionIds = (UA_UInt32 *) UA_malloc(sizeof(UA_UInt32));
    *(request.subscriptionIds) = sub->SubscriptionID;
    
    UA_Client_MonitoredItem *mon, *tmpmon;
    LIST_FOREACH_SAFE(mon, &(sub->MonitoredItems), listEntry, tmpmon) {
        retval |= UA_Client_unMonitorItemChanges(client, sub->SubscriptionID, mon->MonitoredItemId);
    }
    if (retval != UA_STATUSCODE_GOOD){
	    UA_DeleteSubscriptionsRequest_deleteMembers(&request);
        return retval;
    }
    
    response = UA_Client_deleteSubscriptions(client, &request);
    
    if (response.resultsSize > 0)
        retval = response.results[0];
    else
        retval = response.responseHeader.serviceResult;
    
    if (retval == UA_STATUSCODE_GOOD) {
        LIST_REMOVE(sub, listEntry);
        UA_free(sub);
    }
    UA_DeleteSubscriptionsRequest_deleteMembers(&request);
    UA_DeleteSubscriptionsResponse_deleteMembers(&response);
    return retval;
}

UA_UInt32 UA_Client_monitorItemChanges(UA_Client *client, UA_UInt32 subscriptionId,
                                       UA_NodeId nodeId, UA_UInt32 attributeID, void *handlingFunction) {
    UA_Client_Subscription *sub;
    UA_StatusCode retval = 0;
    
    LIST_FOREACH(sub, &(client->subscriptions), listEntry) {
        if (sub->SubscriptionID == subscriptionId)
            break;
    }
    
    // Maybe the same problem as in DeleteSubscription... ask the server?
    if (sub == NULL)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    UA_CreateMonitoredItemsRequest request;
    UA_CreateMonitoredItemsResponse response;
    UA_CreateMonitoredItemsRequest_init(&request);
    UA_CreateMonitoredItemsResponse_init(&response);
    request.subscriptionId = subscriptionId;
    request.itemsToCreateSize = 1;
    request.itemsToCreate = UA_MonitoredItemCreateRequest_new();
    UA_NodeId_copy(&nodeId, &((request.itemsToCreate[0]).itemToMonitor.nodeId));
    (request.itemsToCreate[0]).itemToMonitor.attributeId = attributeID;
    (request.itemsToCreate[0]).monitoringMode = UA_MONITORINGMODE_REPORTING;
    (request.itemsToCreate[0]).requestedParameters.clientHandle = ++(client->monitoredItemHandles);
    (request.itemsToCreate[0]).requestedParameters.samplingInterval = sub->PublishingInterval;
    (request.itemsToCreate[0]).requestedParameters.discardOldest = UA_TRUE;
    (request.itemsToCreate[0]).requestedParameters.queueSize = 1;
    // Filter can be left void for now, only changes are supported (UA_Expert does the same with changeItems)
    
    response = UA_Client_createMonitoredItems(client, &request);
    
    // slight misuse of retval here to check if the deletion was successfull.
    if (response.resultsSize == 0)
        retval = response.responseHeader.serviceResult;
    else
        retval = response.results[0].statusCode;
    
    if (retval == UA_STATUSCODE_GOOD) {
        UA_Client_MonitoredItem *newMon = (UA_Client_MonitoredItem *) UA_malloc(sizeof(UA_Client_MonitoredItem));
        newMon->MonitoringMode = UA_MONITORINGMODE_REPORTING;
        UA_NodeId_copy(&nodeId, &(newMon->monitoredNodeId)); 
        newMon->AttributeID = attributeID;
        newMon->ClientHandle = client->monitoredItemHandles;
        newMon->SamplingInterval = sub->PublishingInterval;
        newMon->QueueSize = 1;
        newMon->DiscardOldest = UA_TRUE;
        newMon->handler = handlingFunction;
        newMon->MonitoredItemId = response.results[0].monitoredItemId;
        
        LIST_INSERT_HEAD(&(sub->MonitoredItems), newMon, listEntry);
        retval = newMon->MonitoredItemId ;
    }
    else {
        retval = 0;
    }
    
    UA_CreateMonitoredItemsRequest_deleteMembers(&request);
    UA_CreateMonitoredItemsResponse_deleteMembers(&response);
    
    return retval;
}

UA_StatusCode UA_Client_unMonitorItemChanges(UA_Client *client, UA_UInt32 subscriptionId, UA_UInt32 monitoredItemId ) {
    UA_Client_Subscription *sub;
    UA_StatusCode retval = 0;
    
    LIST_FOREACH(sub, &(client->subscriptions), listEntry) {
        if (sub->SubscriptionID == subscriptionId)
            break;
    }
    // Maybe the same problem as in DeleteSubscription... ask the server?
    if (sub == NULL)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    
    UA_Client_MonitoredItem *mon, *tmpmon;
    LIST_FOREACH_SAFE(mon, &(sub->MonitoredItems), listEntry, tmpmon) {
        if (mon->MonitoredItemId == monitoredItemId)
            break;
    }
    // Also... ask the server?
    if(mon==NULL) {
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
    }
    
    UA_DeleteMonitoredItemsRequest request;
    UA_DeleteMonitoredItemsResponse response;
    UA_DeleteMonitoredItemsRequest_init(&request);
    UA_DeleteMonitoredItemsResponse_init(&response);
    
    request.subscriptionId = sub->SubscriptionID;
    request.monitoredItemIdsSize = 1;
    request.monitoredItemIds = (UA_UInt32 *) UA_malloc(sizeof(UA_UInt32));
    request.monitoredItemIds[0] = mon->MonitoredItemId;
    
    response = UA_Client_deleteMonitoredItems(client, &request);
    if (response.resultsSize > 1)
        retval = response.results[0];
    else
        retval = response.responseHeader.serviceResult;
    
    if (retval == 0) {
        LIST_REMOVE(mon, listEntry);
        UA_NodeId_deleteMembers(&mon->monitoredNodeId);
        UA_free(mon);
    }
    
    UA_DeleteMonitoredItemsRequest_deleteMembers(&request);
    UA_DeleteMonitoredItemsResponse_deleteMembers(&response);
    
    return retval;
}

UA_Boolean UA_Client_processPublishRx(UA_Client *client, UA_PublishResponse response) {
    UA_Client_Subscription *sub;
    UA_Client_MonitoredItem *mon;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
        return UA_FALSE;
    
    // Check if the server has acknowledged any of our ACKS
    // Note that a list of serverside status codes may be send without valid publish data, i.e. 
    // during keepalives or no data availability
    UA_Client_NotificationsAckNumber *tmpAck = client->pendingNotificationsAcks.lh_first;
    UA_Client_NotificationsAckNumber *nxtAck = tmpAck;
    for(int i=0; i<response.resultsSize && nxtAck != NULL; i++) {
        tmpAck = nxtAck;
        nxtAck = tmpAck->listEntry.le_next;
        if (response.results[i] == UA_STATUSCODE_GOOD || response.results[i] == UA_STATUSCODE_BADSEQUENCENUMBERINVALID) {
            LIST_REMOVE(tmpAck, listEntry);
            UA_free(tmpAck);
        }
    }
    
    if(response.subscriptionId == 0)
        return UA_FALSE;
    
    LIST_FOREACH(sub, &(client->subscriptions), listEntry) {
        if (sub->SubscriptionID == response.subscriptionId)
            break;
    }
    if (sub == NULL)
        return UA_FALSE;
    
    UA_NotificationMessage msg = response.notificationMessage;
    UA_DataChangeNotification dataChangeNotification;
    size_t decodingOffset = 0;
    for (int k=0; k<msg.notificationDataSize; k++) {
        if (msg.notificationData[k].encoding == UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING) {
            if (msg.notificationData[k].typeId.namespaceIndex == 0 && msg.notificationData[k].typeId.identifier.numeric == 811 ) {
                // This is a dataChangeNotification
                retval |= UA_DataChangeNotification_decodeBinary(&(msg.notificationData[k].body), &decodingOffset, &dataChangeNotification);
                UA_MonitoredItemNotification *mitemNot;
                for(int i=0; i<dataChangeNotification.monitoredItemsSize; i++) {
                    mitemNot = &dataChangeNotification.monitoredItems[i];
                    // find this client handle
                    LIST_FOREACH(mon, &(sub->MonitoredItems), listEntry) {
                        if (mon->ClientHandle == mitemNot->clientHandle) {
                            mon->handler(mitemNot->clientHandle, &(mitemNot->value));
                            break;
                        }
                    }
                }
                UA_DataChangeNotification_deleteMembers(&dataChangeNotification);
            }
            else if (msg.notificationData[k].typeId.namespaceIndex == 0 && msg.notificationData[k].typeId.identifier.numeric == 820 ) {
                //FIXME: This is a statusChangeNotification (not supported yet)
                continue;
            }
            else if (msg.notificationData[k].typeId.namespaceIndex == 0 && msg.notificationData[k].typeId.identifier.numeric == 916 ) {
                //FIXME: This is an EventNotification
                continue;
            }
        }
    }
    
    // We processed this message, add it to the list of pending acks (but make sure it's not in the list first)
    LIST_FOREACH(tmpAck, &(client->pendingNotificationsAcks), listEntry) {
        if (tmpAck->subAck.sequenceNumber == msg.sequenceNumber &&
            tmpAck->subAck.subscriptionId == response.subscriptionId)
            break;
    }
    if (tmpAck == NULL ){
        tmpAck = (UA_Client_NotificationsAckNumber *) UA_malloc(sizeof(UA_Client_NotificationsAckNumber));
        tmpAck->subAck.sequenceNumber = msg.sequenceNumber;
        tmpAck->subAck.subscriptionId = sub->SubscriptionID;
	tmpAck->listEntry.le_next = UA_NULL;
	tmpAck->listEntry.le_prev = UA_NULL;
        LIST_INSERT_HEAD(&(client->pendingNotificationsAcks), tmpAck, listEntry);
    }
    
    return response.moreNotifications;
}

void UA_Client_doPublish(UA_Client *client) {
    UA_PublishRequest request;
    UA_PublishResponse response;
    UA_Client_NotificationsAckNumber *ack;
    UA_Boolean moreNotifications = UA_TRUE;
    int index = 0 ;
    
    do {
        UA_PublishRequest_init(&request);
        UA_PublishResponse_init(&response);
        
        request.subscriptionAcknowledgementsSize = 0;
        LIST_FOREACH(ack, &(client->pendingNotificationsAcks), listEntry) {
            request.subscriptionAcknowledgementsSize++;
        }
        request.subscriptionAcknowledgements = (UA_SubscriptionAcknowledgement *) UA_malloc(sizeof(UA_SubscriptionAcknowledgement)*request.subscriptionAcknowledgementsSize);
        
        index = 0;
        LIST_FOREACH(ack, &(client->pendingNotificationsAcks), listEntry) {
            request.subscriptionAcknowledgements[index].sequenceNumber = ack->subAck.sequenceNumber;
            request.subscriptionAcknowledgements[index].subscriptionId = ack->subAck.subscriptionId;
            index++;
        }
        
        response = UA_Client_publish(client, &request);
        if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            moreNotifications = UA_Client_processPublishRx(client, response);
        else
            moreNotifications = UA_FALSE;
        
        UA_PublishResponse_deleteMembers(&response);
        UA_PublishRequest_deleteMembers(&request);
    }  while(moreNotifications == UA_TRUE);
    return;
}

#endif

/**********************************/
/* User-Facing Macros-Function    */
/**********************************/
#ifdef ENABLE_METHODCALLS
UA_CallResponse UA_Client_call(UA_Client *client, UA_CallRequest *request) {
    UA_CallResponse response;
    synchronousRequest(client, (UA_RequestHeader*)request, &UA_TYPES[UA_TYPES_CALLREQUEST],
                       &response, &UA_TYPES[UA_TYPES_CALLRESPONSE]);
    return response;
}

UA_StatusCode UA_Client_CallServerMethod(UA_Client *client, UA_NodeId objectNodeId, UA_NodeId methodNodeId,
                                         UA_Int32 inputSize, const UA_Variant *input,
                                         UA_Int32 *outputSize, UA_Variant **output) {
    UA_CallRequest request;
    UA_CallRequest_init(&request);
    
    request.methodsToCallSize = 1;
    request.methodsToCall = UA_CallMethodRequest_new();
    if(!request.methodsToCall)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_CallMethodRequest *rq = &request.methodsToCall[0];
    UA_NodeId_copy(&methodNodeId, &rq->methodId);
    UA_NodeId_copy(&objectNodeId, &rq->objectId);
    rq->inputArguments = (void*)(uintptr_t)input; // cast const...
    rq->inputArgumentsSize = inputSize;
    
    UA_CallResponse response;
    response = UA_Client_call(client, &request);
    rq->inputArguments = UA_NULL;
    rq->inputArgumentsSize = -1;
    UA_CallRequest_deleteMembers(&request);
    UA_StatusCode retval = response.responseHeader.serviceResult;
    if(response.resultsSize > 0){
        retval |= response.results[0].statusCode;

        if(retval == UA_STATUSCODE_GOOD) {
            *output = response.results[0].outputArguments;
            *outputSize = response.results[0].outputArgumentsSize;
            response.results[0].outputArguments = UA_NULL;
            response.results[0].outputArgumentsSize = -1;
        }
    }
    UA_CallResponse_deleteMembers(&response);
    return retval;
}
#endif

UA_StatusCode __UA_Client_addNode(UA_Client *client, const UA_NodeClass nodeClass,
                                  const UA_NodeId requestedNewNodeId, const UA_NodeId parentNodeId,
                                  const UA_NodeId referenceTypeId, const UA_QualifiedName browseName,
                                  const UA_NodeId typeDefinition, const UA_NodeAttributes *attr,
                                  const UA_DataType *attributeType, UA_NodeId *outNewNodeId) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_AddNodesRequest request;
    UA_AddNodesRequest_init(&request);
    UA_AddNodesItem item;
    UA_AddNodesItem_init(&item);
    item.parentNodeId.nodeId = parentNodeId;
    item.referenceTypeId = referenceTypeId;
    item.requestedNewNodeId.nodeId = requestedNewNodeId;
    item.browseName = browseName;
    item.nodeClass = nodeClass;
    item.typeDefinition.nodeId = typeDefinition;
    size_t attributes_length = UA_calcSizeBinary(attr, attributeType);
    item.nodeAttributes.typeId = attributeType->typeId;
    item.nodeAttributes.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;
    retval = UA_ByteString_newMembers(&item.nodeAttributes.body, attributes_length);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    size_t offset = 0;
    retval = UA_encodeBinary(attr, attributeType, &item.nodeAttributes.body, &offset);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(&item.nodeAttributes.body);
        return retval;
    }
    request.nodesToAdd = &item;
    request.nodesToAddSize = 1;
    UA_AddNodesResponse response = UA_Client_addNodes(client, &request);
    UA_ByteString_deleteMembers(&item.nodeAttributes.body);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        retval = response.responseHeader.serviceResult;
        UA_AddNodesResponse_deleteMembers(&response);
        return retval;
    }
    if(response.resultsSize != 1) {
        UA_AddNodesResponse_deleteMembers(&response);
        return UA_STATUSCODE_BADTOOMANYOPERATIONS;
    }
    if(outNewNodeId && response.results[0].statusCode) {
        *outNewNodeId = response.results[0].addedNodeId;
        UA_NodeId_init(&response.results[0].addedNodeId);
    }
    return response.results[0].statusCode;
}
