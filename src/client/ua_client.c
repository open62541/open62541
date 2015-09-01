#include "ua_types_generated.h"
#include "ua_client.h"
#include "ua_nodeids.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"
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
    { 5 /* ms receive timout */, 30000, 2000,
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
    UA_StatusCode retval = c->connection.getBuffer(&c->connection, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t offset = 8;
    retval |= UA_TcpHelloMessage_encodeBinary(&hello, &message, &offset);
    messageHeader.messageSize = offset;
    offset = 0;
    retval |= UA_TcpMessageHeader_encodeBinary(&messageHeader, &message, &offset);
    UA_TcpHelloMessage_deleteMembers(&hello);
    if(retval != UA_STATUSCODE_GOOD) {
        c->connection.releaseBuffer(&c->connection, &message);
        return retval;
    }

    retval = c->connection.write(&c->connection, &message, messageHeader.messageSize);
    if(retval != UA_STATUSCODE_GOOD) {
        c->connection.releaseBuffer(&c->connection, &message);
        return retval;
    }

    UA_ByteString reply;
    UA_ByteString_init(&reply);
    do {
        retval = c->connection.recv(&c->connection, &reply, c->config.timeout);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    } while(!reply.data);

    offset = 0;
    UA_TcpMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    UA_TcpAcknowledgeMessage ackMessage;
    retval = UA_TcpAcknowledgeMessage_decodeBinary(&reply, &offset, &ackMessage);
    UA_ByteString_deleteMembers(&reply);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    conn->remoteConf.maxChunkCount = ackMessage.maxChunkCount;
    conn->remoteConf.maxMessageSize = ackMessage.maxMessageSize;
    conn->remoteConf.protocolVersion = ackMessage.protocolVersion;
    conn->remoteConf.recvBufferSize = ackMessage.receiveBufferSize;
    conn->remoteConf.sendBufferSize = ackMessage.sendBufferSize;
    conn->state = UA_CONNECTION_ESTABLISHED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode SecureChannelHandshake(UA_Client *client, UA_Boolean renew) {
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
    } else {
        opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
        UA_SecureChannel_generateNonce(&client->channel.clientNonce);
        UA_ByteString_copy(&client->channel.clientNonce, &opnSecRq.clientNonce);
        opnSecRq.securityMode = UA_MESSAGESECURITYMODE_NONE;
    }

    UA_ByteString message;
    UA_StatusCode retval = client->connection.getBuffer(&client->connection, &message);
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
        client->connection.releaseBuffer(&client->connection, &message);
        return retval;
    }

    retval = client->connection.write(&client->connection, &message, messageHeader.messageHeader.messageSize);
    if(retval != UA_STATUSCODE_GOOD) {
        client->connection.releaseBuffer(&client->connection, &message);
        return retval;
    }

    UA_ByteString reply;
    UA_ByteString_init(&reply);
    do {
        retval = client->connection.recv(&client->connection, &reply, client->config.timeout);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
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
        UA_LOG_ERROR(client->logger, UA_LOGCATEGORY_CLIENT,
                     "Reply answers the wrong request. Expected OpenSecureChannelResponse.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_OpenSecureChannelResponse response;
    UA_OpenSecureChannelResponse_decodeBinary(&reply, &offset, &response);
    client->scExpiresAt = UA_DateTime_now() + response.securityToken.revisedLifetime * 10000;
    UA_ByteString_deleteMembers(&reply);
    retval = response.responseHeader.serviceResult;

    if(!renew && retval == UA_STATUSCODE_GOOD) {
        UA_ChannelSecurityToken_copy(&response.securityToken, &client->channel.securityToken);
        UA_ByteString_deleteMembers(&client->channel.serverNonce); // if the handshake is repeated
        UA_ByteString_copy(&response.serverNonce, &client->channel.serverNonce);
    }

    UA_OpenSecureChannelResponse_deleteMembers(&response);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    return retval;
}

/** If the request fails, then the response is cast to UA_ResponseHeader (at the beginning of every
    response) and filled with the appropriate error code */
static void synchronousRequest(UA_Client *client, void *request, const UA_DataType *requestType,
                               void *response, const UA_DataType *responseType) {
    /* Check if sc needs to be renewed */
    if(client->scExpiresAt - UA_DateTime_now() <= client->config.timeToRenewSecureChannel * 10000 )
        UA_Client_renewSecureChannel(client);

    /* Copy authenticationToken token to request header */
    typedef struct {
        UA_RequestHeader requestHeader;
    } headerOnlyRequest;
    /* The cast is valid, since all requests start with a requestHeader */
    UA_NodeId_copy(&client->authenticationToken, &((headerOnlyRequest*)request)->requestHeader.authenticationToken);

    if(!response)
        return;
    UA_init(response, responseType);

    /* Send the request */
    UA_UInt32 requestId = ++client->requestId;
    UA_StatusCode retval = UA_SecureChannel_sendBinaryMessage(&client->channel, requestId,
                                                              request, requestType);
    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;
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
    UA_ByteString_deleteMembers(&reply);
    if(retval != UA_STATUSCODE_GOOD){
        client->state = UA_CLIENTSTATE_ERRORED;
        respHeader->serviceResult = retval;
    }
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

    UA_ByteString_newMembers(&request.userIdentityToken.body, identityToken.policyId.length+4);
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
    for(UA_Int32 i=0; i<response.endpointsSize; ++i){
        UA_EndpointDescription* endpoint = &response.endpoints[i];
        /* look out for an endpoint without security */
        if(!UA_String_equal(&endpoint->securityPolicyUri,
                            &UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None")))
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
    UA_CreateSessionResponse response;
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
    UA_StatusCode retval = client->connection.getBuffer(&client->connection, &message);
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
        client->connection.releaseBuffer(&client->connection, &message);
        return retval;
    }
        
    retval = client->connection.write(&client->connection, &message, msgHeader.messageHeader.messageSize);
    if(retval != UA_STATUSCODE_GOOD)
        client->connection.releaseBuffer(&client->connection, &message);
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

    client->connection = connectFunc(UA_ConnectionConfig_standard, endpointUrl, &client->logger);
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
        if (response.results[i] == UA_STATUSCODE_GOOD) {
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
        tmpAck = (UA_Client_NotificationsAckNumber *) malloc(sizeof(UA_Client_NotificationsAckNumber));
        tmpAck->subAck.sequenceNumber = msg.sequenceNumber;
        tmpAck->subAck.subscriptionId = sub->SubscriptionID;
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
            ack = client->pendingNotificationsAcks.lh_first;
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
    synchronousRequest(client, request, &UA_TYPES[UA_TYPES_CALLREQUEST],
                       &response, &UA_TYPES[UA_TYPES_CALLRESPONSE]);
    return response;
}

UA_StatusCode UA_Client_callServerMethod(UA_Client *client, UA_NodeId objectNodeId, UA_NodeId methodNodeId,
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
    
    if(retval == UA_STATUSCODE_GOOD) {
        retval |= response.results[0].statusCode;
        *output = response.results[0].outputArguments;
        *outputSize = response.results[0].outputArgumentsSize;
        response.results[0].outputArguments = UA_NULL;
        response.results[0].outputArgumentsSize = -1;
    }
    
    UA_CallResponse_deleteMembers(&response);
    return retval;
}
#endif

/* Delete Node */
#define UA_CLIENT_DELETENODETYPEALIAS(TYPE) \
UA_StatusCode UA_Client_delete##TYPE##Node(UA_Client *client, UA_NodeId nodeId) { \
  return UA_Client_deleteNode(client, nodeId);\
}

UA_StatusCode UA_Client_deleteNode(UA_Client *client, UA_NodeId nodeId) {
  UA_DeleteNodesRequest *drq = UA_DeleteNodesRequest_new();
  UA_DeleteNodesResponse drs;
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  
  drq->nodesToDeleteSize = 1;
  drq->nodesToDelete = (UA_DeleteNodesItem *) UA_malloc(sizeof(UA_DeleteNodesItem));
  drq->nodesToDelete[0].deleteTargetReferences = UA_TRUE;
  UA_NodeId_copy(&nodeId, &drq->nodesToDelete[0].nodeId);
  drs = UA_Client_deleteNodes(client, drq);
  
  if (drs.responseHeader.serviceResult != UA_STATUSCODE_GOOD || drs.resultsSize < 1)
    return drs.responseHeader.serviceResult;
  
  retval = drs.results[0];
    
  UA_DeleteNodesRequest_delete(drq);
  UA_DeleteNodesResponse_deleteMembers(&drs);
  return retval;
}

UA_CLIENT_DELETENODETYPEALIAS(Object)

UA_CLIENT_DELETENODETYPEALIAS(Variable)

UA_CLIENT_DELETENODETYPEALIAS(ObjectType)

UA_CLIENT_DELETENODETYPEALIAS(VariableType)

UA_CLIENT_DELETENODETYPEALIAS(DataType)

UA_CLIENT_DELETENODETYPEALIAS(Method)

UA_CLIENT_DELETENODETYPEALIAS(View)

#define ADDNODES_COPYDEFAULTATTRIBUTES(REQUEST,ATTRIBUTES) do {                           \
    ATTRIBUTES.specifiedAttributes = 0;                                                   \
    if(! UA_LocalizedText_copy(&description, &(ATTRIBUTES.description)))                  \
        ATTRIBUTES.specifiedAttributes |=  UA_NODEATTRIBUTESMASK_DESCRIPTION;             \
    if(! UA_LocalizedText_copy(&displayName, &(ATTRIBUTES.displayName)))                  \
        ATTRIBUTES.specifiedAttributes |= UA_NODEATTRIBUTESMASK_DISPLAYNAME;              \
    ATTRIBUTES.userWriteMask       = userWriteMask;                                       \
    ATTRIBUTES.specifiedAttributes |= UA_NODEATTRIBUTESMASK_USERWRITEMASK;                \
    ATTRIBUTES.writeMask           = writeMask;                                           \
    ATTRIBUTES.specifiedAttributes |= UA_NODEATTRIBUTESMASK_WRITEMASK;                    \
    UA_QualifiedName_copy(&browseName, &(REQUEST.nodesToAdd[0].browseName));              \
    UA_ExpandedNodeId_copy(&parentNodeId, &(REQUEST.nodesToAdd[0].parentNodeId));         \
    UA_NodeId_copy(&referenceTypeId, &(REQUEST.nodesToAdd[0].referenceTypeId));           \
    UA_ExpandedNodeId_copy(&typeDefinition, &(REQUEST.nodesToAdd[0].typeDefinition));     \
    UA_ExpandedNodeId reqExpNodeId;                                                       \
    UA_ExpandedNodeId_init(&reqExpNodeId);                                                \
    UA_NodeId_copy(&reqId, &reqExpNodeId.nodeId);                                         \
    UA_ExpandedNodeId_copy(&reqExpNodeId, &(REQUEST.nodesToAdd[0].requestedNewNodeId ));  \
    REQUEST.nodesToAddSize = 1;                                                           \
    } while(0)
    
#define ADDNODES_PACK_AND_SEND(PREQUEST,PATTRIBUTES,PNODETYPE) do {                                                                       \
    PREQUEST.nodesToAdd[0].nodeAttributes.encoding = UA_EXTENSIONOBJECT_ENCODINGMASK_BODYISBYTESTRING;                                    \
    PREQUEST.nodesToAdd[0].nodeAttributes.typeId   = UA_NODEID_NUMERIC(0, UA_NS0ID_##PNODETYPE##ATTRIBUTES + UA_ENCODINGOFFSET_BINARY);   \
    size_t encOffset = 0;                                                                                                                 \
    UA_ByteString_newMembers(&PREQUEST.nodesToAdd[0].nodeAttributes.body, client->connection.remoteConf.maxMessageSize);                  \
    UA_encodeBinary(&PATTRIBUTES,&UA_TYPES[UA_TYPES_##PNODETYPE##ATTRIBUTES], &(PREQUEST.nodesToAdd[0].nodeAttributes.body), &encOffset); \
    PREQUEST.nodesToAdd[0].nodeAttributes.body.length = encOffset;                                                                                 \
    *(adRes) = UA_Client_addNodes(client, &PREQUEST);                                                                                     \
    UA_AddNodesRequest_deleteMembers(&PREQUEST);                                                                                          \
} while(0)
    
/* NodeManagement */
UA_StatusCode UA_Client_addObjectNode(UA_Client *client, UA_NodeId reqId, UA_QualifiedName browseName, UA_LocalizedText displayName, 
                                      UA_LocalizedText description, UA_ExpandedNodeId parentNodeId, UA_NodeId referenceTypeId,
                                      UA_UInt32 userWriteMask, UA_UInt32 writeMask, UA_ExpandedNodeId typeDefinition, UA_NodeId *createdNodeId) {
    UA_AddNodesRequest adReq;
    UA_StatusCode retval;
    UA_AddNodesRequest_init(&adReq);

    UA_AddNodesResponse *adRes;
    adRes = UA_AddNodesResponse_new();
    UA_AddNodesResponse_init(adRes);

    UA_ObjectAttributes vAtt;
    UA_ObjectAttributes_init(&vAtt);
    adReq.nodesToAdd = (UA_AddNodesItem *) UA_AddNodesItem_new();
    UA_AddNodesItem_init(adReq.nodesToAdd);
    
    // Default node properties and attributes
    ADDNODES_COPYDEFAULTATTRIBUTES(adReq, vAtt);
    
    // Specific to objects
    adReq.nodesToAdd[0].nodeClass = UA_NODECLASS_OBJECT;
    vAtt.eventNotifier       = 0;
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_EVENTNOTIFIER;

    ADDNODES_PACK_AND_SEND(adReq,vAtt,OBJECT); 
    
    if(adRes->responseHeader.serviceResult != UA_STATUSCODE_GOOD || adRes->resultsSize == 0)
      retval = adRes->responseHeader.serviceResult;
    retval = adRes->results[0].statusCode;
    if(createdNodeId != NULL)
      UA_NodeId_copy(&adRes->results[0].addedNodeId, createdNodeId);
    UA_AddNodesResponse_deleteMembers(adRes);
    return retval;
}

UA_StatusCode UA_Client_addVariableNode(UA_Client *client, UA_NodeId reqId, UA_QualifiedName browseName, UA_LocalizedText displayName, 
                                        UA_LocalizedText description, UA_ExpandedNodeId parentNodeId, UA_NodeId referenceTypeId,
                                        UA_UInt32 userWriteMask, UA_UInt32 writeMask, UA_Variant *value, UA_NodeId *createdNodeId) {
    UA_AddNodesRequest adReq;
    UA_StatusCode retval;
    UA_AddNodesRequest_init(&adReq);
    
    UA_AddNodesResponse *adRes;
    adRes = UA_AddNodesResponse_new();
    UA_AddNodesResponse_init(adRes);
    
    UA_VariableAttributes vAtt;
    UA_VariableAttributes_init(&vAtt);
    adReq.nodesToAdd = (UA_AddNodesItem *) UA_AddNodesItem_new();
    UA_AddNodesItem_init(adReq.nodesToAdd);
    
    UA_ExpandedNodeId typeDefinition;
    UA_ExpandedNodeId_init(&typeDefinition);
    if (value != UA_NULL)
      UA_NodeId_copy(&value->type->typeId, &typeDefinition.nodeId);
    // Default node properties and attributes
    ADDNODES_COPYDEFAULTATTRIBUTES(adReq, vAtt);
    
    // Specific to variables
    adReq.nodesToAdd[0].nodeClass = UA_NODECLASS_VARIABLE;
    vAtt.accessLevel              = 0;
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_ACCESSLEVEL;
    vAtt.userAccessLevel          = 0;
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_USERACCESSLEVEL;
    vAtt.minimumSamplingInterval  = 100;
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_MINIMUMSAMPLINGINTERVAL;
    vAtt.historizing              = UA_FALSE;
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_HISTORIZING;
    
    if (value != NULL) {
        UA_Variant_copy(value, &(vAtt.value));
        vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_VALUE;
        vAtt.valueRank            = -2;
        vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_VALUERANK;
        // These are defined by the variant
        //vAtt.arrayDimensionsSize  = value->arrayDimensionsSize;
        //vAtt.arrayDimensions      = NULL;
    }
    UA_NodeId_copy(&value->type->typeId, &(vAtt.dataType));
    
    ADDNODES_PACK_AND_SEND(adReq,vAtt,VARIABLE);
    
    if(adRes->responseHeader.serviceResult != UA_STATUSCODE_GOOD || adRes->resultsSize == 0)
      retval = adRes->responseHeader.serviceResult;
    retval = adRes->results[0].statusCode;
    if(createdNodeId != NULL)
      UA_NodeId_copy(&adRes->results[0].addedNodeId, createdNodeId);
    UA_AddNodesResponse_deleteMembers(adRes);
    return retval;
}

UA_StatusCode UA_Client_addReferenceTypeNode( UA_Client *client, UA_NodeId reqId, UA_QualifiedName browseName, UA_LocalizedText displayName, 
                                              UA_LocalizedText description, UA_ExpandedNodeId parentNodeId, UA_NodeId referenceTypeId,
                                              UA_UInt32 userWriteMask, UA_UInt32 writeMask, UA_ExpandedNodeId typeDefinition,
                                              UA_LocalizedText inverseName, UA_NodeId *createdNodeId ) {
    UA_AddNodesRequest adReq;
    UA_StatusCode retval;
    UA_AddNodesRequest_init(&adReq);
    
    UA_AddNodesResponse *adRes;
    adRes = UA_AddNodesResponse_new();
    UA_AddNodesResponse_init(adRes);
    
    UA_ReferenceTypeAttributes vAtt;
    UA_ReferenceTypeAttributes_init(&vAtt);
    adReq.nodesToAdd = (UA_AddNodesItem *) UA_AddNodesItem_new();
    UA_AddNodesItem_init(adReq.nodesToAdd);
    
    // Default node properties and attributes
    ADDNODES_COPYDEFAULTATTRIBUTES(adReq, vAtt);

    // Specific to referencetypes
    adReq.nodesToAdd[0].nodeClass = UA_NODECLASS_REFERENCETYPE;
    UA_LocalizedText_copy(&inverseName, &(vAtt.inverseName));
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_INVERSENAME;
    vAtt.symmetric = UA_FALSE;
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_SYMMETRIC;
    vAtt.isAbstract = UA_FALSE;
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_ISABSTRACT;
    
    
    ADDNODES_PACK_AND_SEND(adReq,vAtt,REFERENCETYPE);
    
    if(adRes->responseHeader.serviceResult != UA_STATUSCODE_GOOD || adRes->resultsSize == 0)
      retval = adRes->responseHeader.serviceResult;
    retval = adRes->results[0].statusCode;
    if(createdNodeId != NULL)
      UA_NodeId_copy(&adRes->results[0].addedNodeId, createdNodeId);
    UA_AddNodesResponse_deleteMembers(adRes);
    return retval;
}

UA_StatusCode UA_Client_addObjectTypeNode(UA_Client *client, UA_NodeId reqId, UA_QualifiedName browseName, UA_LocalizedText displayName, 
                                          UA_LocalizedText description, UA_ExpandedNodeId parentNodeId, UA_NodeId referenceTypeId,
                                          UA_UInt32 userWriteMask, UA_UInt32 writeMask, UA_ExpandedNodeId typeDefinition, UA_Boolean isAbstract,
                                          UA_NodeId *createdNodeId) {
    UA_AddNodesRequest adReq;
    UA_StatusCode retval;
    UA_AddNodesRequest_init(&adReq);
    
    UA_AddNodesResponse *adRes;
    adRes = UA_AddNodesResponse_new();
    UA_AddNodesResponse_init(adRes);
    
    UA_ObjectTypeAttributes vAtt;
    UA_ObjectTypeAttributes_init(&vAtt);
    adReq.nodesToAdd = (UA_AddNodesItem *) UA_AddNodesItem_new();
    UA_AddNodesItem_init(adReq.nodesToAdd);
    
    // Default node properties and attributes
    ADDNODES_COPYDEFAULTATTRIBUTES(adReq, vAtt);
    
    // Specific to referencetypes
    adReq.nodesToAdd[0].nodeClass = UA_NODECLASS_OBJECTTYPE;
    vAtt.isAbstract = isAbstract;
    vAtt.specifiedAttributes |= UA_NODEATTRIBUTESMASK_ISABSTRACT;
    
    
    ADDNODES_PACK_AND_SEND(adReq,vAtt,OBJECTTYPE);
    
    if(adRes->responseHeader.serviceResult != UA_STATUSCODE_GOOD || adRes->resultsSize == 0)
      retval = adRes->responseHeader.serviceResult;
    retval = adRes->results[0].statusCode;
    if(createdNodeId != NULL)
      UA_NodeId_copy(&adRes->results[0].addedNodeId, createdNodeId);
    UA_AddNodesResponse_deleteMembers(adRes);
    return retval;
}

UA_StatusCode 
UA_Client_forEachChildNodeCall(UA_Client *client, UA_NodeId parentNodeId, UA_NodeIteratorCallback callback, void *handle) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  
  UA_BrowseRequest  brq;
  UA_BrowseRequest_init(&brq);
  UA_BrowseResponse brs;
  UA_BrowseResponse_init(&brs);
  brq.nodesToBrowseSize = 1;
  brq.requestedMaxReferencesPerNode = 0;
  brq.nodesToBrowse = UA_BrowseDescription_new();
  brq.nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_BOTH;
  brq.nodesToBrowse[0].includeSubtypes = UA_TRUE;
  UA_NodeId_copy(&parentNodeId, &brq.nodesToBrowse[0].nodeId);
  brq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
  brs = UA_Client_browse(client, &brq);
  
  UA_BrowseRequest_deleteMembers(&brq);
  if (brs.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
    UA_BrowseResponse_deleteMembers(&brq);
    return brs.responseHeader.serviceResult;
  }
  if (brs.resultsSize < 1) {
    UA_BrowseResponse_deleteMembers(&brq);
    return UA_STATUSCODE_GOOD;
  }
  if (brs.results[0].statusCode != UA_STATUSCODE_GOOD) {
    UA_BrowseResponse_deleteMembers(&brq);
    return brs.results[0].statusCode;
  }
  
  UA_Boolean isInverse;
  UA_NodeId *childId = UA_NodeId_new();
  UA_NodeId *refTypeId = UA_NodeId_new();
  for (int i = 0; i < brs.results[0].referencesSize; i++) {
    isInverse = UA_FALSE;
    if (brs.results[0].references[i].isForward == UA_FALSE)
      isInverse = UA_TRUE;
    UA_NodeId_copy(&brs.results[0].references[i].nodeId.nodeId, childId);
    UA_NodeId_copy(&brs.results[0].references[i].referenceTypeId, refTypeId);
    //UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId
    callback(*childId, isInverse, *refTypeId, handle);
    
    UA_NodeId_deleteMembers(childId);
    UA_NodeId_deleteMembers(refTypeId);
  }
  UA_NodeId_delete(childId);
  UA_NodeId_delete(refTypeId);
  
  UA_BrowseResponse_deleteMembers(&brs);
  return retval;
}

#include "server/ua_nodes.h"

UA_StatusCode 
UA_Client_copyBaseAttributes(UA_Client *client, UA_ReadResponse *readResponseSrc, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_Node *target = (UA_Node *) dst;
  if (readResponseSrc->results[0].value.data != UA_NULL)
    retval |= UA_NodeId_copy((UA_NodeId *) readResponseSrc->results[0].value.data, &target->nodeId);
  if (readResponseSrc->results[1].value.data != UA_NULL)
    retval |= UA_NodeClass_copy((UA_NodeClass *) readResponseSrc->results[1].value.data, &target->nodeClass);
  if (readResponseSrc->results[2].value.data != UA_NULL)
    retval |= UA_QualifiedName_copy((UA_QualifiedName *) readResponseSrc->results[2].value.data, &target->browseName);
  if (readResponseSrc->results[3].value.data != UA_NULL)
    retval |= UA_LocalizedText_copy((UA_LocalizedText *) readResponseSrc->results[3].value.data, &target->displayName);
  if (readResponseSrc->results[4].value.data != UA_NULL)
    retval |= UA_LocalizedText_copy((UA_LocalizedText *) readResponseSrc->results[4].value.data, &target->description);
  if (readResponseSrc->results[5].value.data != UA_NULL)  
    retval |= UA_UInt32_copy((UA_UInt32 *) readResponseSrc->results[5].value.data, &target->writeMask);
  if (readResponseSrc->results[6].value.data != UA_NULL)  
    retval |= UA_UInt32_copy((UA_UInt32 *) readResponseSrc->results[6].value.data, &target->userWriteMask);
  
  target->referencesSize = 0;
  target->references = UA_NULL;
  
  UA_BrowseRequest *brq = UA_BrowseRequest_new();
  UA_BrowseResponse brs;
  UA_BrowseResult_init(&brs);
  brq->nodesToBrowseSize = 1;
  brq->requestedMaxReferencesPerNode = 0;
  brq->nodesToBrowse = UA_BrowseDescription_new();
  brq->nodesToBrowse[0].browseDirection = UA_BROWSEDIRECTION_BOTH;
  brq->nodesToBrowse[0].includeSubtypes = UA_TRUE;
  UA_NodeId_copy((UA_NodeId *) readResponseSrc->results[0].value.data, &brq->nodesToBrowse[0].nodeId);
  brq->nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
  brs = UA_Client_browse(client, brq);
  UA_BrowseRequest_delete(brq);
  
  if (brs.responseHeader.serviceResult!= UA_STATUSCODE_GOOD || brs.resultsSize != 1) {
    UA_BrowseResponse_deleteMembers(&brs);
    return retval;
  }
  if (brs.results[0].statusCode != UA_STATUSCODE_GOOD) {
    UA_BrowseResponse_deleteMembers(&brs);
    return retval;
  }
  
  /* typedef struct {
   U A_NodeId referenceTypeId; *
   UA_Boolean isInverse;
   UA_ExpandedNodeId targetId; */
  target->referencesSize = brs.results[0].referencesSize;
  target->references = (UA_ReferenceNode *) UA_malloc(sizeof(UA_ReferenceNode) *  brs.results[0].referencesSize);
  for (int i=0; i<brs.results[0].referencesSize; i++) {
    target->references[i].isInverse = UA_FALSE;
    if (brs.results[0].references->isForward == UA_FALSE)
      target->references[i].isInverse = UA_TRUE;
    UA_NodeId_copy(&brs.results[0].references->nodeId.nodeId, &target->references[i].referenceTypeId); 
    UA_ExpandedNodeId_init(&target->references[i].targetId);
    UA_NodeId_copy(&brs.results[0].references->referenceTypeId, &target->references[i].targetId.nodeId);
  }
  
  UA_BrowseResponse_deleteMembers(&brs);
  return retval;
}

UA_StatusCode 
UA_Client_appendObjectNodeAttributes(UA_Client *client, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_ObjectNode *target = (UA_ObjectNode *) dst;
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  
  /* Read node attributes:
      UA_ATTRIBUTEID_EVENTNOTIFIER           = 12
   */
  rrq->nodesToReadSize = 1;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+12;
    UA_NodeId_copy(&target->nodeId, &rrq->nodesToRead[i].nodeId);
  }
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;
  
  if (rrs.results[0].value.data != NULL)
    UA_Byte_copy((UA_Byte *) rrs.results[0].value.data, &target->eventNotifier);
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
}

UA_StatusCode 
UA_Client_appendObjectTypeNodeAttributes(UA_Client *client, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_ObjectTypeNode *target = (UA_ObjectTypeNode *) dst;
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  
  /* Read node attributes:
   * UA_ATTRIBUTEID_ISABSTRACT              = 8,
   */
  rrq->nodesToReadSize = 1;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+8;
    UA_NodeId_copy(&target->nodeId, &rrq->nodesToRead[i].nodeId);
  }
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;
  
  if (rrs.results[0].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[0].value.data, &target->isAbstract);
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
}

UA_StatusCode 
UA_Client_appendVariableNodeAttributes(UA_Client *client, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_VariableNode *target = (UA_VariableNode *) dst;
  
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  
   /* Read node attributes:    
      UA_ATTRIBUTEID_VALUE                   = 13,
      UA_ATTRIBUTEID_DATATYPE                = 14, // Req. but not used (is in variant)
      UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16, // Req. but not used (is in variant)
      
      UA_ATTRIBUTEID_VALUERANK               = 15,
      UA_ATTRIBUTEID_ACCESSLEVEL             = 17,
      UA_ATTRIBUTEID_USERACCESSLEVEL         = 18,
      UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL = 19,
      UA_ATTRIBUTEID_HISTORIZING             = 20,
   */
  rrq->nodesToReadSize = 8;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+13;
    UA_NodeId_copy(&target->nodeId, &rrq->nodesToRead[i].nodeId);
  }
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;

  if (rrs.results[0].value.data != NULL)
    UA_Variant_copy((UA_Variant *) &rrs.results[0].value, &target->value.variant);
  if (rrs.results[3].value.data != NULL)
    UA_Int32_copy((UA_Int32 *) rrs.results[3].value.data, &target->valueRank);
  if (rrs.results[4].value.data != NULL)
    UA_Byte_copy((UA_Byte *) rrs.results[4].value.data, &target->accessLevel);
  if (rrs.results[5].value.data != NULL)
    UA_Byte_copy((UA_Byte *) rrs.results[5].value.data, &target->userAccessLevel);
  if (rrs.results[6].value.data != NULL)
    UA_Double_copy((UA_Double *) rrs.results[6].value.data, &target->minimumSamplingInterval);
  if (rrs.results[7].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[7].value.data, &target->historizing);
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
}

UA_StatusCode 
UA_Client_appendVariableTypeNodeAttributes(UA_Client *client, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_VariableTypeNode *target = (UA_VariableTypeNode *) dst;
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  
  /* Read node attributes:
      UA_ATTRIBUTEID_ISABSTRACT              = 8,
      UA_ATTRIBUTEID_VALUE                   = 13,
      UA_ATTRIBUTEID_DATATYPE                = 14, // Req. but not used (is in variant)
      UA_ATTRIBUTEID_ARRAYDIMENSIONS         = 16, // Req. but not used (is in variant)
      
      UA_ATTRIBUTEID_VALUERANK               = 15,
      
   */
  rrq->nodesToReadSize = 8;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+13;
    UA_NodeId_copy(&target->nodeId, &rrq->nodesToRead[i].nodeId);
  }
  rrq->nodesToRead[0].attributeId = UA_ATTRIBUTEID_ISABSTRACT;
  
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;
  
  if (rrs.results[0].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[0].value.data, &target->isAbstract);
  if (rrs.results[1].value.data != NULL)
    UA_Variant_copy((UA_Variant *) &rrs.results[1].value, &target->value.variant);
  if (rrs.results[2].value.data != NULL)
    UA_Int32_copy((UA_Int32 *) rrs.results[2].value.data, &target->valueRank);
  
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
}

UA_StatusCode 
UA_Client_appendReferenceTypeNodeAttributes(UA_Client *client, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_ReferenceTypeNode *target = (UA_ReferenceTypeNode *) dst;
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  
  /* Read node attributes:
      UA_ATTRIBUTEID_SYMMETRIC               = 9,
      UA_ATTRIBUTEID_INVERSENAME             = 10,
   *  UA_ATTRIBUTEID_ISABSTRACT              = 8,
   */
  rrq->nodesToReadSize = 3;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+8;
    UA_NodeId_copy(&target->nodeId, &rrq->nodesToRead[i].nodeId);
  }
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;
  
  if (rrs.results[0].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[0].value.data, &target->isAbstract);
  if (rrs.results[1].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[1].value.data, &target->symmetric);
  if (rrs.results[2].value.data != NULL)
    UA_LocalizedText_copy((UA_LocalizedText *) rrs.results[2].value.data, &target->inverseName);
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
  return retval;
}

UA_StatusCode 
UA_Client_appendViewNodeAttributes(UA_Client *client, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_ViewNode *target = (UA_ViewNode *) dst;
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  
  /* Read node attributes:
      UA_ATTRIBUTEID_CONTAINSNOLOOPS         = 11,
      UA_ATTRIBUTEID_EVENTNOTIFIER           = 12,
   */
  rrq->nodesToReadSize = 2;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+11;
    UA_NodeId_copy(&target->nodeId, &rrq->nodesToRead[i].nodeId);
  }
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;
  
  if (rrs.results[0].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[0].value.data, &target->containsNoLoops);
  if (rrs.results[1].value.data != NULL)
    UA_Byte_copy((UA_Byte *) rrs.results[1].value.data, &target->eventNotifier);
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
  return retval;
}

UA_StatusCode 
UA_Client_appendDataTypeNodeAttributes(UA_Client *client, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_DataTypeNode *target = (UA_DataTypeNode *) dst;
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  
  /* Read node attributes:
   * UA_ATTRIBUTEID_ISABSTRACT              = 8,
   */
  rrq->nodesToReadSize = 1;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+8;
    UA_NodeId_copy(&target->nodeId, &rrq->nodesToRead[i].nodeId);
  }
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;
  
  if (rrs.results[0].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[0].value.data, &target->isAbstract);
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
}

UA_StatusCode UA_Client_appendMethodNodeAttributes(UA_Client *client, void *dst) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_MethodNode *target = (UA_MethodNode *) dst;
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  
  /* Read node attributes:
   * UA_ATTRIBUTEID_USEREXECUTABLE          = 22
   * UA_ATTRIBUTEID_EXECUTABLE              = 21,
   */
  rrq->nodesToReadSize = 2;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+21;
    UA_NodeId_copy(&target->nodeId, &rrq->nodesToRead[i].nodeId);
  }
  
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;
  
  if (rrs.results[0].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[0].value.data, &target->executable);
  if (rrs.results[1].value.data != NULL)
    UA_Boolean_copy((UA_Boolean *) rrs.results[1].value.data, &target->userExecutable);
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
}

UA_StatusCode 
UA_Client_getNodeCopy(UA_Client *client, UA_NodeId nodeId, void **copyInto) {
  UA_ReadRequest  *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  *copyInto = UA_NULL;
  /* Read default node attributes:
      UA_ATTRIBUTEID_NODEID                  = 1,
      UA_ATTRIBUTEID_NODECLASS               = 2,
      UA_ATTRIBUTEID_BROWSENAME              = 3,
      UA_ATTRIBUTEID_DISPLAYNAME             = 4,
      UA_ATTRIBUTEID_DESCRIPTION             = 5,
      UA_ATTRIBUTEID_WRITEMASK               = 6,
      UA_ATTRIBUTEID_USERWRITEMASK           = 7,
   */
  rrq->nodesToReadSize = 7;
  rrq->nodesToRead = (UA_ReadValueId*) UA_malloc(sizeof(UA_ReadValueId) * rrq->nodesToReadSize);
  for(int i = 0; i < rrq->nodesToReadSize; i++) {
    UA_ReadValueId_init(&rrq->nodesToRead[i]);
    rrq->nodesToRead[i].attributeId = (UA_UInt32 ) i+1;
    UA_NodeId_copy(&nodeId, &rrq->nodesToRead[i].nodeId);
  }
  rrs = UA_Client_read(client, rrq);
  if (rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if (rrs.resultsSize != rrq->nodesToReadSize)
    return rrs.responseHeader.serviceResult;
  UA_ReadRequest_delete(rrq);
  
  UA_UInt32 *nodeClass = rrs.results[1].value.data;
  switch(*nodeClass) {
    case UA_NODECLASS_OBJECT:
      *copyInto = UA_ObjectNode_new();
      retval |= UA_Client_copyBaseAttributes(client, &rrs, *copyInto);
      retval |= UA_Client_appendObjectNodeAttributes(client, *copyInto);
      break;
    case UA_NODECLASS_OBJECTTYPE:
      *copyInto = UA_ObjectTypeNode_new();
      retval |= UA_Client_copyBaseAttributes(client, &rrs, *copyInto);
      retval |= UA_Client_appendObjectTypeNodeAttributes(client, *copyInto);
      break;
    case UA_NODECLASS_VARIABLE:
      *copyInto = UA_VariableNode_new();
      retval |= UA_Client_copyBaseAttributes(client, &rrs, *copyInto);
      retval |= UA_Client_appendVariableNodeAttributes(client, *copyInto);
      break;
    case UA_NODECLASS_VARIABLETYPE:
      *copyInto = UA_VariableTypeNode_new();
      retval |= UA_Client_copyBaseAttributes(client, &rrs, *copyInto);
      retval |= UA_Client_appendVariableTypeNodeAttributes(client, *copyInto);
      break;
    case UA_NODECLASS_REFERENCETYPE:
      *copyInto = UA_ReferenceTypeNode_new();
      retval |= UA_Client_copyBaseAttributes(client, &rrs, *copyInto);
      retval |= UA_Client_appendReferenceTypeNodeAttributes(client, *copyInto);
      break;
    case UA_NODECLASS_METHOD:
      *copyInto = UA_MethodNode_new();
      retval |= UA_Client_copyBaseAttributes(client, &rrs, *copyInto);
      retval |= UA_Client_appendMethodNodeAttributes(client, *copyInto);
      break;
    case UA_NODECLASS_VIEW:
      *copyInto = UA_ViewNode_new();
      retval |= UA_Client_copyBaseAttributes(client, &rrs, *copyInto);
      retval |= UA_Client_appendViewNodeAttributes(client, *copyInto);
      break;
    case UA_NODECLASS_DATATYPE:
      *copyInto = UA_DataTypeNode_new();
      retval |= UA_Client_copyBaseAttributes(client, &rrs, *copyInto);
      retval |= UA_Client_appendDataTypeNodeAttributes(client, *copyInto);
      break;
    default:
      UA_ReadRequest_delete(rrq);
      UA_ReadResponse_deleteMembers(&rrs);
      return UA_STATUSCODE_BADNODECLASSINVALID;
  }
 
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
}

UA_StatusCode UA_Client_deleteNodeCopy(UA_Client *client, void **node) {
  return UA_Server_deleteNodeCopy(UA_NULL, node);
}

#define SETATTRIBUTE_COPYTYPEVALUE(TYPE)                                                       \
wrq->nodesToWrite[0].value.value.data = UA_##TYPE##_new();                                     \
UA_##TYPE##_copy((UA_##TYPE *) value, (UA_##TYPE *) wrq->nodesToWrite[0].value.value.data);

UA_StatusCode 
UA_Client_setAttributeValue(UA_Client *client, UA_NodeId nodeId, UA_AttributeId attributeId, void *value) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  
  UA_WriteRequest *wrq = UA_WriteRequest_new();
  UA_WriteResponse wrs;
  UA_WriteResponse_init(&wrs);
  wrq->nodesToWriteSize = 1;
  wrq->nodesToWrite = UA_WriteValue_new();
  UA_NodeId_copy(&nodeId, &wrq->nodesToWrite[0].nodeId);
  wrq->nodesToWrite[0].attributeId = attributeId;
  wrq->nodesToWrite[0].value.hasValue = UA_TRUE;
  wrq->nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA;
  
  switch(attributeId) {
    case UA_ATTRIBUTEID_NODEID:
      UA_WriteRequest_delete(wrq);
      UA_WriteResponse_deleteMembers(&wrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_NODECLASS:
      UA_WriteRequest_delete(wrq);
      UA_WriteResponse_deleteMembers(&wrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_BROWSENAME:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_QUALIFIEDNAME];
      SETATTRIBUTE_COPYTYPEVALUE(QualifiedName);
      break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
      SETATTRIBUTE_COPYTYPEVALUE(LocalizedText);
      break;
    case UA_ATTRIBUTEID_DESCRIPTION:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
      SETATTRIBUTE_COPYTYPEVALUE(LocalizedText);
      break;
    case UA_ATTRIBUTEID_WRITEMASK:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_UINT32];
      SETATTRIBUTE_COPYTYPEVALUE(UInt32);
      break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_UINT32];
      SETATTRIBUTE_COPYTYPEVALUE(UInt32);
      break;    
    case UA_ATTRIBUTEID_ISABSTRACT:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
      SETATTRIBUTE_COPYTYPEVALUE(Boolean);
      break;
    case UA_ATTRIBUTEID_SYMMETRIC:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
      SETATTRIBUTE_COPYTYPEVALUE(Boolean);
      break;
    case UA_ATTRIBUTEID_INVERSENAME:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_LOCALIZEDTEXT];
      SETATTRIBUTE_COPYTYPEVALUE(LocalizedText);
      break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
      SETATTRIBUTE_COPYTYPEVALUE(Boolean);
      break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_BYTE];
      SETATTRIBUTE_COPYTYPEVALUE(Byte); 
      break;
    case UA_ATTRIBUTEID_VALUE:
      UA_Variant_copy((UA_Variant *) value, (UA_Variant *) &wrq->nodesToWrite[0].value.value);
      break;
    case UA_ATTRIBUTEID_DATATYPE:
      UA_WriteRequest_delete(wrq);
      UA_WriteResponse_deleteMembers(&wrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_VALUERANK:
      UA_WriteRequest_delete(wrq);
      UA_WriteResponse_deleteMembers(&wrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
      UA_WriteRequest_delete(wrq);
      UA_WriteResponse_deleteMembers(&wrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_UINT32];
      SETATTRIBUTE_COPYTYPEVALUE(Byte);
      break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_UINT32];
      SETATTRIBUTE_COPYTYPEVALUE(Byte);
      break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_DOUBLE];
      SETATTRIBUTE_COPYTYPEVALUE(Double);
      break;
    case UA_ATTRIBUTEID_HISTORIZING:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
      SETATTRIBUTE_COPYTYPEVALUE(Boolean);
      break;
    case UA_ATTRIBUTEID_EXECUTABLE:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
      SETATTRIBUTE_COPYTYPEVALUE(Boolean);
      break;
    case UA_ATTRIBUTEID_USEREXECUTABLE:
      wrq->nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_BOOLEAN];
      SETATTRIBUTE_COPYTYPEVALUE(Boolean);
      break;
    default:
      UA_WriteRequest_delete(wrq);
      UA_WriteResponse_deleteMembers(&wrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
  }
  
  wrs = UA_Client_write(client, wrq);
  UA_WriteRequest_delete(wrq);
  
  if (wrs.responseHeader.serviceResult)
    return wrs.responseHeader.serviceResult;
  if (wrs.resultsSize != 1)
    return wrs.responseHeader.serviceResult;
  
  retval = wrs.results[0];
  
  UA_WriteResponse_deleteMembers(&wrs);
  return retval;
}

#define GETATTRIBUTE_COPYTYPEVALUE(TYPE)                                           \
if (rrs.results[0].hasValue == UA_TRUE) {                                          \
  *value = (void *) UA_##TYPE##_new();                                             \
  UA_##TYPE##_copy((UA_##TYPE *) rrs.results[0].value.data, (UA_##TYPE *) *value  ); \
}
  
UA_StatusCode UA_EXPORT 
UA_Client_getAttributeValue(UA_Client *client, UA_NodeId nodeId, UA_AttributeId attributeId, void **value) {
  UA_StatusCode retval = UA_STATUSCODE_GOOD;
  UA_ReadRequest *rrq = UA_ReadRequest_new();
  UA_ReadResponse rrs;
  UA_ReadResponse_init(&rrs);
  *value = UA_NULL;
  
  rrq->timestampsToReturn = UA_TIMESTAMPSTORETURN_NEITHER;
  rrq->nodesToReadSize = 1;
  rrq->nodesToRead = UA_ReadValueId_new();
  rrq->nodesToRead[0].attributeId = attributeId;
  UA_NodeId_copy(&nodeId, &rrq->nodesToRead[0].nodeId);
  
  rrs = UA_Client_read(client, rrq);
  UA_ReadRequest_delete(rrq);
  
  if(rrs.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    return rrs.responseHeader.serviceResult;
  if(rrs.resultsSize < 1)
    return rrs.responseHeader.serviceResult;
  if (rrs.results[0].status != UA_STATUSCODE_GOOD)
    return rrs.results[0].status;
  
  switch(attributeId) {
    case UA_ATTRIBUTEID_NODEID:
      GETATTRIBUTE_COPYTYPEVALUE(NodeId)
      break;
    case UA_ATTRIBUTEID_NODECLASS:
      GETATTRIBUTE_COPYTYPEVALUE(NodeClass)
      break;
    case UA_ATTRIBUTEID_BROWSENAME:
      GETATTRIBUTE_COPYTYPEVALUE(QualifiedName)
      break;
    case UA_ATTRIBUTEID_DISPLAYNAME:
      GETATTRIBUTE_COPYTYPEVALUE(LocalizedText)
      break;
    case UA_ATTRIBUTEID_DESCRIPTION:
      GETATTRIBUTE_COPYTYPEVALUE(LocalizedText)
      break;
    case UA_ATTRIBUTEID_WRITEMASK:
      GETATTRIBUTE_COPYTYPEVALUE(UInt32)
      break;
    case UA_ATTRIBUTEID_USERWRITEMASK:
      GETATTRIBUTE_COPYTYPEVALUE(UInt32)
      break;    
    case UA_ATTRIBUTEID_ISABSTRACT:
      GETATTRIBUTE_COPYTYPEVALUE(Boolean)
      break;
    case UA_ATTRIBUTEID_SYMMETRIC:
      GETATTRIBUTE_COPYTYPEVALUE(Boolean)
      break;
    case UA_ATTRIBUTEID_INVERSENAME:
      GETATTRIBUTE_COPYTYPEVALUE(LocalizedText)
      break;
    case UA_ATTRIBUTEID_CONTAINSNOLOOPS:
      GETATTRIBUTE_COPYTYPEVALUE(Boolean)
      break;
    case UA_ATTRIBUTEID_EVENTNOTIFIER:
      GETATTRIBUTE_COPYTYPEVALUE(Byte)
      break;
    case UA_ATTRIBUTEID_VALUE:
      GETATTRIBUTE_COPYTYPEVALUE(Variant)
      break;
    case UA_ATTRIBUTEID_DATATYPE:
      *value = UA_NULL;
      UA_ReadResponse_deleteMembers(&rrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_VALUERANK:
      *value = UA_NULL;
      UA_ReadResponse_deleteMembers(&rrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_ARRAYDIMENSIONS:
      *value = UA_NULL;
      UA_ReadResponse_deleteMembers(&rrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
      break;
    case UA_ATTRIBUTEID_ACCESSLEVEL:
      GETATTRIBUTE_COPYTYPEVALUE(Byte)
      break;
    case UA_ATTRIBUTEID_USERACCESSLEVEL:
      GETATTRIBUTE_COPYTYPEVALUE(Byte)
      break;
    case UA_ATTRIBUTEID_MINIMUMSAMPLINGINTERVAL:
      GETATTRIBUTE_COPYTYPEVALUE(Double)
      break;
    case UA_ATTRIBUTEID_HISTORIZING:
      GETATTRIBUTE_COPYTYPEVALUE(Boolean)
      break;
    case UA_ATTRIBUTEID_EXECUTABLE:
      GETATTRIBUTE_COPYTYPEVALUE(Boolean)
      break;
    case UA_ATTRIBUTEID_USEREXECUTABLE:
      GETATTRIBUTE_COPYTYPEVALUE(Boolean)
      break;
    default:
      *value = UA_NULL;
      UA_ReadResponse_deleteMembers(&rrs);
      return UA_STATUSCODE_BADATTRIBUTEIDINVALID;
  }
  
  UA_ReadResponse_deleteMembers(&rrs);
  return retval;
}
