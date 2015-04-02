#include "ua_client.h"
#include "ua_nodeids.h"
#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"

struct UA_Client {
    UA_ClientNetworkLayer networkLayer;
    UA_String endpointUrl;
    UA_Connection connection;

    UA_UInt32 sequenceNumber;
    UA_UInt32 requestId;

    /* Secure Channel */
    UA_ChannelSecurityToken securityToken;
    UA_ByteString clientNonce;
    UA_ByteString serverNonce;
	/* UA_SequenceHeader sequenceHdr; */
	/* UA_NodeId authenticationToken; */

    /* Session */
    UA_NodeId sessionId;
    UA_NodeId authenticationToken;
};

UA_Client * UA_Client_new(void) {
    UA_Client *client = UA_malloc(sizeof(UA_Client));
    if(!client)
        return UA_NULL;
    UA_String_init(&client->endpointUrl);
    client->connection.state = UA_CONNECTION_OPENING;

    client->sequenceNumber = 0;
    client->requestId = 0;

    /* Secure Channel */
    UA_ChannelSecurityToken_deleteMembers(&client->securityToken);
    UA_ByteString_init(&client->clientNonce);
    UA_ByteString_init(&client->serverNonce);
    
    UA_NodeId_init(&client->authenticationToken);
    return client;
}

void UA_Client_delete(UA_Client* client){
    client->networkLayer.delete(client->networkLayer.nlHandle);
    UA_String_deleteMembers(&client->endpointUrl);
    // client->connection

    /* Secure Channel */
    UA_ByteString_deleteMembers(&client->clientNonce);
    UA_ByteString_deleteMembers(&client->serverNonce);

    free(client);
}

// The tcp connection is established. Now do the handshake
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

	messageHeader.messageSize = UA_TcpHelloMessage_calcSizeBinary((UA_TcpHelloMessage const*) &hello) +
                                UA_TcpMessageHeader_calcSizeBinary((UA_TcpMessageHeader const*) &messageHeader);
	UA_ByteString message;
    message.data = UA_alloca(messageHeader.messageSize);
    message.length = messageHeader.messageSize;

	size_t offset = 0;
	UA_TcpMessageHeader_encodeBinary(&messageHeader, &message, &offset);
	UA_TcpHelloMessage_encodeBinary(&hello, &message, &offset);
    UA_TcpHelloMessage_deleteMembers(&hello);

    UA_ByteStringArray buf = {.stringsSize = 1, .strings = &message};
    UA_StatusCode retval = c->networkLayer.send(c->networkLayer.nlHandle, buf);
    if(retval)
        return retval;

    UA_Byte replybuf[1024];
    UA_ByteString reply = {.data = replybuf, .length = 1024};
    retval = c->networkLayer.awaitResponse(c->networkLayer.nlHandle, &reply, 0);
	if (retval)
		return retval;

    offset = 0;
	UA_TcpMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
    UA_TcpAcknowledgeMessage ackMessage;
    retval = UA_TcpAcknowledgeMessage_decodeBinary(&reply, &offset, &ackMessage);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_TcpAcknowledgeMessage_deleteMembers(&ackMessage);
        return retval;
    }
    conn->remoteConf.maxChunkCount = ackMessage.maxChunkCount;
    conn->remoteConf.maxMessageSize = ackMessage.maxMessageSize;
    conn->remoteConf.protocolVersion = ackMessage.protocolVersion;
    conn->remoteConf.recvBufferSize = ackMessage.receiveBufferSize;
    conn->remoteConf.sendBufferSize = ackMessage.sendBufferSize;
    conn->state = UA_CONNECTION_ESTABLISHED;

    UA_TcpAcknowledgeMessage_deleteMembers(&ackMessage);
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode SecureChannelHandshake(UA_Client *client) {
    UA_ByteString_deleteMembers(&client->clientNonce); // if the handshake is repeated
	UA_ByteString_newMembers(&client->clientNonce, 1);
	client->clientNonce.data[0] = 0;

	UA_SecureConversationMessageHeader messageHeader;
	messageHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_OPNF;
	messageHeader.secureChannelId = 0;

	UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = ++client->sequenceNumber;
    seqHeader.requestId = ++client->requestId;

	UA_AsymmetricAlgorithmSecurityHeader asymHeader;
	UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
	asymHeader.securityPolicyUri = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");

    /* id of opensecurechannelrequest */
	UA_NodeId requestType = UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELREQUEST + UA_ENCODINGOFFSET_BINARY);

	UA_OpenSecureChannelRequest opnSecRq;
	UA_OpenSecureChannelRequest_init(&opnSecRq);
	opnSecRq.requestHeader.timestamp = UA_DateTime_now();
    UA_ByteString_copy(&client->clientNonce, &opnSecRq.clientNonce);
	opnSecRq.requestedLifetime = 30000;
	opnSecRq.securityMode = UA_MESSAGESECURITYMODE_NONE;
	opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
	opnSecRq.requestHeader.authenticationToken.identifier.numeric = 10;
	opnSecRq.requestHeader.authenticationToken.identifierType = UA_NODEIDTYPE_NUMERIC;
	opnSecRq.requestHeader.authenticationToken.namespaceIndex = 10;

	messageHeader.messageHeader.messageSize =
        UA_SecureConversationMessageHeader_calcSizeBinary(&messageHeader) +
        UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(&asymHeader) +
        UA_SequenceHeader_calcSizeBinary(&seqHeader) +
        UA_NodeId_calcSizeBinary(&requestType) +
        UA_OpenSecureChannelRequest_calcSizeBinary(&opnSecRq);

	UA_ByteString message;
    message.data = UA_alloca(messageHeader.messageHeader.messageSize);
    message.length = messageHeader.messageHeader.messageSize;

	size_t offset = 0;
	UA_SecureConversationMessageHeader_encodeBinary(&messageHeader, &message, &offset);
	UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &message, &offset);
	UA_SequenceHeader_encodeBinary(&seqHeader, &message, &offset);
	UA_NodeId_encodeBinary(&requestType, &message, &offset);
	UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, &message, &offset);

	UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);

    UA_ByteStringArray buf = {.stringsSize = 1, .strings = &message};
    UA_StatusCode retval = client->networkLayer.send(client->networkLayer.nlHandle, buf);
    if(retval)
        return retval;

    // parse the response
    UA_ByteString reply;
    UA_ByteString_newMembers(&reply, client->connection.localConf.recvBufferSize);
    retval = client->networkLayer.awaitResponse(client->networkLayer.nlHandle, &reply, 1000);
    if(retval) {
        UA_ByteString_deleteMembers(&reply);
        return retval;
    }

	offset = 0;

	UA_SecureConversationMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&reply, &offset, &asymHeader);
	UA_SequenceHeader_decodeBinary(&reply, &offset, &seqHeader);
	UA_NodeId_decodeBinary(&reply, &offset, &requestType);

	if(!UA_NodeId_equal(&requestType, &UA_NODEID_NUMERIC(0, UA_NS0ID_OPENSECURECHANNELRESPONSE +
                                                        UA_ENCODINGOFFSET_BINARY))) {
        UA_ByteString_deleteMembers(&reply);
        UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

	UA_OpenSecureChannelResponse response;
	UA_OpenSecureChannelResponse_decodeBinary(&reply, &offset, &response);
    UA_ByteString_deleteMembers(&reply);
    retval = response.responseHeader.serviceResult;

    if(retval == UA_STATUSCODE_GOOD) {
        UA_ChannelSecurityToken_copy(&response.securityToken, &client->securityToken);
        UA_ByteString_deleteMembers(&client->serverNonce); // if the handshake is repeated
        UA_ByteString_copy(&response.serverNonce, &client->serverNonce);
    }

	UA_OpenSecureChannelResponse_deleteMembers(&response);
	UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    return retval;

}

/** If the request fails, then the response is cast to UA_ResponseHeader (at the beginning of every
    response) and filled with the appropriate error code */
static void sendReceiveRequest(UA_RequestHeader *request, const UA_DataType *requestType,
                               void *response, const UA_DataType *responseType, UA_Client *client,
                               UA_Boolean sendOnly)
{
    UA_NodeId_copy(&client->authenticationToken, &request->authenticationToken);

    UA_SecureConversationMessageHeader msgHeader;
    if(sendOnly)
    	msgHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_CLOF;
    else
    	msgHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;
    msgHeader.secureChannelId = client->securityToken.channelId;

    UA_SymmetricAlgorithmSecurityHeader symHeader;
    symHeader.tokenId = client->securityToken.tokenId;
    
	UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = ++client->sequenceNumber;
    seqHeader.requestId = ++client->requestId;

	UA_NodeId requestId = UA_NODEID_NUMERIC(0, requestType->typeId.identifier.numeric +
                                           UA_ENCODINGOFFSET_BINARY);

	msgHeader.messageHeader.messageSize =
        UA_SecureConversationMessageHeader_calcSizeBinary(&msgHeader) +
        UA_SymmetricAlgorithmSecurityHeader_calcSizeBinary(&symHeader) +
        UA_SequenceHeader_calcSizeBinary(&seqHeader) +
        UA_NodeId_calcSizeBinary(&requestId) +
        UA_calcSizeBinary(request, requestType);

	UA_ByteString message;
    UA_StatusCode retval = UA_ByteString_newMembers(&message, msgHeader.messageHeader.messageSize);
    if(retval != UA_STATUSCODE_GOOD) {
        // todo
    }

    size_t offset = 0;
    retval |= UA_SecureConversationMessageHeader_encodeBinary(&msgHeader, &message, &offset);
    retval |= UA_SymmetricAlgorithmSecurityHeader_encodeBinary(&symHeader, &message, &offset);
    retval |= UA_SequenceHeader_encodeBinary(&seqHeader, &message, &offset);

    retval |= UA_NodeId_encodeBinary(&requestId, &message, &offset);
    retval |= UA_encodeBinary(request, requestType, &message, &offset);

    UA_ByteStringArray buf = {.stringsSize = 1, .strings = &message};
    retval = client->networkLayer.send(client->networkLayer.nlHandle, buf);
    UA_ByteString_deleteMembers(&message);

    //TODO: rework to get return value
    if(sendOnly)
    	return;

    UA_ResponseHeader *respHeader = (UA_ResponseHeader*)response;
    if(retval != UA_STATUSCODE_GOOD) {
        respHeader->serviceResult = retval;
        return;
    }

    /* Response */
    UA_ByteString reply;
    UA_ByteString_newMembers(&reply, client->connection.localConf.recvBufferSize);
    retval = client->networkLayer.awaitResponse(client->networkLayer.nlHandle, &reply, 1000);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(&reply);
        respHeader->serviceResult = retval;
        return;
    }

	offset = 0;
	retval |= UA_SecureConversationMessageHeader_decodeBinary(&reply, &offset, &msgHeader);
	retval |= UA_SymmetricAlgorithmSecurityHeader_decodeBinary(&reply, &offset, &symHeader);
	retval |= UA_SequenceHeader_decodeBinary(&reply, &offset, &seqHeader);
    UA_NodeId responseId;
	retval |= UA_NodeId_decodeBinary(&reply, &offset, &responseId);

	if(!UA_NodeId_equal(&responseId, &UA_NODEID_NUMERIC(0, responseType->typeId.identifier.numeric +
                                                       UA_ENCODINGOFFSET_BINARY))) {
        UA_ByteString_deleteMembers(&reply);
        UA_SymmetricAlgorithmSecurityHeader_deleteMembers(&symHeader);
        respHeader->serviceResult = retval;
        return;
    }

	retval = UA_decodeBinary(&reply, &offset, response, responseType);
    UA_ByteString_deleteMembers(&reply);
    if(retval != UA_STATUSCODE_GOOD)
        respHeader->serviceResult = retval;
}

static void synchronousRequest(void *request, const UA_DataType *requestType,
                               void *response, const UA_DataType *responseType, UA_Client *client){
	sendReceiveRequest(request, requestType, response, responseType, client, UA_FALSE);
}

static UA_StatusCode ActivateSession(UA_Client *client) {
	UA_ActivateSessionRequest request;
	UA_ActivateSessionRequest_init(&request);

    request.requestHeader.requestHandle = 2; //TODO: is it a magic number?
    request.requestHeader.authenticationToken = client->authenticationToken;
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;

    UA_ActivateSessionResponse response;
    synchronousRequest(&request, &UA_TYPES[UA_TYPES_ACTIVATESESSIONREQUEST],
                       &response, &UA_TYPES[UA_TYPES_ACTIVATESESSIONRESPONSE],
                       client);

    UA_ActivateSessionRequest_deleteMembers(&request);
    UA_ActivateSessionResponse_deleteMembers(&response);
    return response.responseHeader.serviceResult; // not deleted
}

static UA_StatusCode SessionHandshake(UA_Client *client) {

    UA_CreateSessionRequest request;
    UA_CreateSessionRequest_init(&request);

    // todo: is this needed for all requests?
    UA_NodeId_copy(&client->authenticationToken, &request.requestHeader.authenticationToken);

    request.requestHeader.timestamp = UA_DateTime_now();
	request.requestHeader.timeoutHint = 10000;
    UA_ByteString_copy(&client->clientNonce, &request.clientNonce);
	request.requestedSessionTimeout = 1200000;
	request.maxResponseMessageSize = UA_INT32_MAX;

	/* UA_String_copy(endpointUrl, &rq.endpointUrl); */
	/* UA_String_copycstring("mysession", &rq.sessionName); */
	/* UA_String_copycstring("abcd", &rq.clientCertificate); */

    UA_CreateSessionResponse response;
    synchronousRequest(&request, &UA_TYPES[UA_TYPES_CREATESESSIONREQUEST],
                       &response, &UA_TYPES[UA_TYPES_CREATESESSIONRESPONSE],
                       client);

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
    synchronousRequest(&request, &UA_TYPES[UA_TYPES_CLOSESESSIONREQUEST],
                       &response, &UA_TYPES[UA_TYPES_CLOSESESSIONRESPONSE],
                       client);

    UA_CloseSessionRequest_deleteMembers(&request);
    UA_CloseSessionResponse_deleteMembers(&response);
    return response.responseHeader.serviceResult; // not deleted
}

static UA_StatusCode CloseSecureChannel(UA_Client *client) {
	UA_CloseSecureChannelRequest request;
    UA_CloseSecureChannelRequest_init(&request);

    request.requestHeader.requestHandle = 1; //TODO: magic number?
    request.requestHeader.timestamp = UA_DateTime_now();
    request.requestHeader.timeoutHint = 10000;
    request.requestHeader.authenticationToken = client->authenticationToken;
	sendReceiveRequest(&request.requestHeader, &UA_TYPES[UA_TYPES_CLOSESECURECHANNELREQUEST], UA_NULL, UA_NULL,
                       client, UA_TRUE);

    return UA_STATUSCODE_GOOD;

}

/*************************/
/* User-Facing Functions */
/*************************/


UA_StatusCode UA_Client_connect(UA_Client *client, UA_ConnectionConfig conf,
                                UA_ClientNetworkLayer networkLayer, char *endpointUrl)
{
    client->endpointUrl = UA_STRING(endpointUrl);
    if(client->endpointUrl.length < 0)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    client->networkLayer = networkLayer;
    client->connection.localConf = conf;
    UA_StatusCode retval = networkLayer.connect(client->endpointUrl, client->networkLayer.nlHandle);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = HelAckHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SecureChannelHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = SessionHandshake(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = ActivateSession(client);
    return retval;
}


UA_StatusCode UA_EXPORT UA_Client_disconnect(UA_Client *client) {
	UA_StatusCode retval;
	retval = CloseSession(client);
    if(retval == UA_STATUSCODE_GOOD)
        retval = CloseSecureChannel(client);
    return retval;
}

UA_ReadResponse UA_Client_read(UA_Client *client, UA_ReadRequest *request) {
    UA_ReadResponse response;
    synchronousRequest(request, &UA_TYPES[UA_TYPES_READREQUEST], &response,
                       &UA_TYPES[UA_TYPES_READRESPONSE], client);
    return response;
}

UA_WriteResponse UA_Client_write(UA_Client *client, UA_WriteRequest *request) {
    UA_WriteResponse response;
    synchronousRequest(request, &UA_TYPES[UA_TYPES_WRITEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_WRITERESPONSE], client);
    return response;
}

UA_BrowseResponse UA_Client_browse(UA_Client *client, UA_BrowseRequest *request) {
    UA_BrowseResponse response;
    synchronousRequest(request, &UA_TYPES[UA_TYPES_BROWSEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_BROWSERESPONSE], client);
    return response;
}

UA_TranslateBrowsePathsToNodeIdsResponse
    UA_Client_translateTranslateBrowsePathsToNodeIds(UA_Client *client,
                                                     UA_TranslateBrowsePathsToNodeIdsRequest *request) {
    UA_TranslateBrowsePathsToNodeIdsResponse response;
    synchronousRequest(request, &UA_TYPES[UA_TYPES_BROWSEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_BROWSERESPONSE], client);
    return response;
}

UA_AddNodesResponse UA_Client_addNodes(UA_Client *client, UA_AddNodesRequest *request) {
    UA_AddNodesResponse response;
    synchronousRequest(request, &UA_TYPES[UA_TYPES_BROWSEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_BROWSERESPONSE], client);
    return response;
}

UA_AddReferencesResponse UA_Client_addReferences(UA_Client *client, UA_AddReferencesRequest *request) {
    UA_AddReferencesResponse response;
    synchronousRequest(request, &UA_TYPES[UA_TYPES_BROWSEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_BROWSERESPONSE], client);
    return response;
}

UA_DeleteNodesResponse UA_Client_deleteNodes(UA_Client *client, UA_DeleteNodesRequest *request) {
    UA_DeleteNodesResponse response;
    synchronousRequest(request, &UA_TYPES[UA_TYPES_BROWSEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_BROWSERESPONSE], client);
    return response;
}

UA_DeleteReferencesResponse UA_EXPORT
UA_Client_deleteReferences(UA_Client *client, UA_DeleteReferencesRequest *request) {
    UA_DeleteReferencesResponse response;
    synchronousRequest(request, &UA_TYPES[UA_TYPES_BROWSEREQUEST], &response,
                       &UA_TYPES[UA_TYPES_BROWSERESPONSE], client);
    return response;;
}
