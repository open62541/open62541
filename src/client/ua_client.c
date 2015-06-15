#include "ua_types_generated.h"
#include "ua_client.h"
#include "ua_nodeids.h"
#include "ua_securechannel.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"

struct UA_Client {
    /* Connection */
    UA_Connection connection;
    UA_SecureChannel channel;
    UA_String endpointUrl;
    UA_UInt32 requestId;

    /* Session */
    UA_UserTokenPolicy token;
    UA_NodeId sessionId;
    UA_NodeId authenticationToken;

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

    UA_Connection_init(&client->connection);
    UA_SecureChannel_init(&client->channel);
    client->channel.connection = &client->connection;
    UA_String_init(&client->endpointUrl);
    client->requestId = 0;

    UA_NodeId_init(&client->authenticationToken);

    client->logger = logger;
    client->config = config;
    client->scExpiresAt = 0;
    
    return client;
}

void UA_Client_delete(UA_Client* client){
    UA_Connection_deleteMembers(&client->connection);
    UA_SecureChannel_deleteMembersCleanup(&client->channel);
    UA_String_deleteMembers(&client->endpointUrl);
    UA_UserTokenPolicy_deleteMembers(&client->token);
    free(client);
}

static UA_StatusCode HelAckHandshake(UA_Client *c) {
    UA_TcpMessageHeader messageHeader;
    messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_HELF;

    UA_TcpHelloMessage hello;
    UA_String_copy(&c->endpointUrl, &hello.endpointUrl);

    UA_Connection *conn = &c->connection;
    hello.maxChunkCount = conn->localConf.maxChunkCount;
    hello.maxMessageSize = conn->localConf.maxMessageSize;
    hello.protocolVersion = conn->localConf.protocolVersion;
    hello.receiveBufferSize = conn->localConf.recvBufferSize;
    hello.sendBufferSize = conn->localConf.sendBufferSize;

    messageHeader.messageSize = UA_TcpHelloMessage_calcSizeBinary((UA_TcpHelloMessage const*)&hello) +
                                UA_TcpMessageHeader_calcSizeBinary((UA_TcpMessageHeader const*)&messageHeader);
    UA_ByteString message;
    UA_StatusCode retval = c->connection.getBuffer(&c->connection, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t offset = 0;
    UA_TcpMessageHeader_encodeBinary(&messageHeader, &message, &offset);
    UA_TcpHelloMessage_encodeBinary(&hello, &message, &offset);
    UA_TcpHelloMessage_deleteMembers(&hello);

    retval = c->connection.write(&c->connection, &message, messageHeader.messageSize);
    if(retval != UA_STATUSCODE_GOOD)
        c->connection.releaseBuffer(&c->connection, &message);
    if(retval)
        return retval;

    UA_ByteString reply;
    UA_ByteString_init(&reply);
    do {
        retval = c->connection.recv(&c->connection, &reply, c->config.timeout);
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
            return retval;
    } while(retval != UA_STATUSCODE_GOOD);

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

    messageHeader.messageHeader.messageSize =
        UA_SecureConversationMessageHeader_calcSizeBinary(&messageHeader) +
        UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(&asymHeader) +
        UA_SequenceHeader_calcSizeBinary(&seqHeader) +
        UA_NodeId_calcSizeBinary(&requestType) +
        UA_OpenSecureChannelRequest_calcSizeBinary(&opnSecRq);

    UA_ByteString message;
    UA_StatusCode retval = client->connection.getBuffer(&client->connection, &message);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);
        return retval;
    }

    size_t offset = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&messageHeader, &message, &offset);
    UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &message, &offset);
    UA_SequenceHeader_encodeBinary(&seqHeader, &message, &offset);
    UA_NodeId_encodeBinary(&requestType, &message, &offset);
    UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, &message, &offset);

    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);

    retval = client->connection.write(&client->connection, &message, messageHeader.messageHeader.messageSize);
    if(retval != UA_STATUSCODE_GOOD)
        client->connection.releaseBuffer(&client->connection, &message);
    if(retval)
        return retval;

    // parse the response
    UA_ByteString reply;
    UA_ByteString_init(&reply);
    do {
        retval = client->connection.recv(&client->connection, &reply, client->config.timeout);
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED)
            return retval;
    } while(retval != UA_STATUSCODE_GOOD);

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
        respHeader->serviceResult = retval;
        return;
    }

    /* Retrieve the response */
    // Todo: push this into the generic securechannel implementation for client and server
    UA_ByteString reply;
    UA_ByteString_init(&reply);
    do {
        retval = client->connection.recv(&client->connection, &reply, client->config.timeout);
        if(retval == UA_STATUSCODE_BADCONNECTIONCLOSED) {
            respHeader->serviceResult = retval;
            return;
        }
    } while(retval != UA_STATUSCODE_GOOD);

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
    if(!UA_NodeId_equal(&responseId, &expectedNodeId) || seqHeader.requestId != requestId) {
        // Todo: we need to demux responses since a publish responses may come at any time
        UA_ByteString_deleteMembers(&reply);
        UA_SymmetricAlgorithmSecurityHeader_deleteMembers(&symHeader);
        respHeader->serviceResult = UA_STATUSCODE_BADINTERNALERROR;
        return;
    }

    retval = UA_decodeBinary(&reply, &offset, response, responseType);
    UA_ByteString_deleteMembers(&reply);
    if(retval != UA_STATUSCODE_GOOD)
        respHeader->serviceResult = retval;
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

    msgHeader.messageHeader.messageSize =
        UA_SecureConversationMessageHeader_calcSizeBinary(&msgHeader) +
        UA_SymmetricAlgorithmSecurityHeader_calcSizeBinary(&symHeader) +
        UA_SequenceHeader_calcSizeBinary(&seqHeader) +
        UA_NodeId_calcSizeBinary(&typeId) +
        UA_calcSizeBinary(&request, &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST]);

    UA_ByteString message;
    UA_StatusCode retval = client->connection.getBuffer(&client->connection, &message);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    size_t offset = 0;
    retval |= UA_SecureConversationMessageHeader_encodeBinary(&msgHeader, &message, &offset);
    retval |= UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symHeader, &message, &offset);
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &message, &offset);
    retval |= UA_NodeId_encodeBinary(&typeId, &message, &offset);
    retval |= UA_encodeBinary(&request, &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST], &message, &offset);
    if(retval == UA_STATUSCODE_GOOD) {
        retval = client->connection.write(&client->connection, &message, msgHeader.messageHeader.messageSize);
        if(retval != UA_STATUSCODE_GOOD)
            client->connection.releaseBuffer(&client->connection, &message);
    } else
        client->connection.releaseBuffer(&client->connection, &message);
    return retval;
}

/*************************/
/* User-Facing Functions */
/*************************/

UA_StatusCode UA_Client_connect(UA_Client *client, UA_ConnectClientConnection connectFunc, char *endpointUrl) {
    client->connection = connectFunc(UA_ConnectionConfig_standard, endpointUrl, &client->logger);
    if(client->connection.state != UA_CONNECTION_OPENING)
        return UA_STATUSCODE_BADCONNECTIONCLOSED;

    client->endpointUrl = UA_STRING_ALLOC(endpointUrl);
    if(client->endpointUrl.length < 0)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    client->connection.localConf = client->config.localConnectionConfig;
    UA_StatusCode retval = HelAckHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SecureChannelHandshake(client, UA_FALSE);
    if(retval == UA_STATUSCODE_GOOD)
        retval = EndpointsHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SessionHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = ActivateSession(client);
        
    return retval;
}

UA_StatusCode UA_Client_disconnect(UA_Client *client) {
    UA_StatusCode retval;
    retval = CloseSession(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = CloseSecureChannel(client);
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
