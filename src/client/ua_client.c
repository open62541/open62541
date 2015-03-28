#include "ua_client.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"

struct UA_Client {
    UA_ClientNetworkLayer networkLayer;
    UA_String endpointUrl;
    UA_Connection connection;
	/* UA_UInt32 channelId; */
	/* UA_SequenceHeader sequenceHdr; */
	/* UA_NodeId authenticationToken; */
	/* UA_UInt32 tokenId; */
    /* UA_Connection *connection; */
};

UA_Client * UA_Client_new(void) {
    UA_Client *c = UA_malloc(sizeof(UA_Client));
    if(!c)
        return UA_NULL;
    UA_String_init(&c->endpointUrl);
    c->connection.state = UA_CONNECTION_OPENING;
    return c;
}

void UA_Client_delete(UA_Client* client){
	if(client){
		client->networkLayer.delete(client->networkLayer.nlHandle);
		UA_String_deleteMembers(&client->endpointUrl);
		free(client);
	}
}

static UA_StatusCode HelAckHandshake(UA_Client *c);
static UA_StatusCode SecureChannelHandshake(UA_Client *c);
static UA_StatusCode SessionHandshake(UA_Client *c);
static UA_StatusCode CloseSession(UA_Client *c);
static UA_StatusCode CloseSecureChannel(UA_Client *c);

UA_StatusCode UA_Client_connect(UA_Client *c, UA_ConnectionConfig conf, UA_ClientNetworkLayer networkLayer,
                                char *endpointUrl) {
    UA_StatusCode retval = UA_String_copycstring(endpointUrl, &c->endpointUrl);
    if(retval != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    c->networkLayer = networkLayer;
    c->connection.localConf = conf;
    retval = networkLayer.connect(c->endpointUrl, c->networkLayer.nlHandle);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    HelAckHandshake(c);
    // securechannel
    // session
    
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT UA_Client_disconnect(UA_Client *c) {
    return UA_STATUSCODE_GOOD;
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

static UA_StatusCode SecureChannelHandshake(UA_Client *c) {
    UA_SecureConversationMessageHeader msghdr;
    msghdr.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_OPNF;
    msghdr.secureChannelId = 0;

    UA_AsymmetricAlgorithmSecurityHeader asymHeader;
    UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
	UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None", &asymHeader.securityPolicyUri);

    UA_SequenceHeader seqHeader;
    seqHeader.sequenceNumber = 51; // why is that???
    seqHeader.requestId = 1;
    
    UA_NodeId requestType = {.identifierType = UA_NODEIDTYPE_NUMERIC, .namespaceIndex = 0,
                             .identifier.numeric = 446}; // id of opensecurechannelrequest

	UA_OpenSecureChannelRequest opnSecRq;
	UA_OpenSecureChannelRequest_init(&opnSecRq);
	opnSecRq.requestHeader.timestamp = UA_DateTime_now();
	UA_ByteString_newMembers(&opnSecRq.clientNonce, 1);
	opnSecRq.clientNonce.data[0] = 0;
	opnSecRq.clientProtocolVersion = 0;
	opnSecRq.requestedLifetime = 30000;
	opnSecRq.securityMode = UA_MESSAGESECURITYMODE_NONE;
	opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;
	opnSecRq.requestHeader.authenticationToken.identifier.numeric = 10;
	opnSecRq.requestHeader.authenticationToken.identifierType = UA_NODEIDTYPE_NUMERIC;
	opnSecRq.requestHeader.authenticationToken.namespaceIndex = 10;

	msghdr.messageHeader.messageSize = UA_SecureConversationMessageHeader_calcSizeBinary(&msghdr) +
        UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(&asymHeader) +
        UA_SequenceHeader_calcSizeBinary(&seqHeader) +
        UA_NodeId_calcSizeBinary(&requestType) +
        UA_OpenSecureChannelRequest_calcSizeBinary(&opnSecRq);

	UA_ByteString message;
	UA_ByteString_newMembers(&message, msghdr.messageHeader.messageSize);
	size_t offset = 0;
    UA_SecureConversationMessageHeader_encodeBinary(&msghdr, &message, &offset);
    UA_AsymmetricAlgorithmSecurityHeader_encodeBinary(&asymHeader, &message, &offset);
    UA_SequenceHeader_encodeBinary(&seqHeader, &message, &offset);
    UA_NodeId_encodeBinary(&requestType, &message, &offset);
    UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, &message, &offset);

    UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);
    UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);

    UA_ByteStringArray buf = {.stringsSize = 1, .strings = &message};
    UA_StatusCode retval = c->networkLayer.send(c->networkLayer.nlHandle, buf);
	UA_ByteString_deleteMembers(&message);

    // parse the response
    UA_ByteString response;
    UA_ByteString_newMembers(&response, c->connection.localConf.recvBufferSize);
    retval = c->networkLayer.awaitResponse(c->networkLayer.nlHandle, &response, 1000);
    
    /* UA_SecureConversationMessageHeader_init(&msghdr); */
    /* UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader); */
    /* UA_SequenceHeader_init(&seqHeader); */
	/* UA_OpenSecureChannelResponse opnSecRq; */
    
        /* UA_SecureConversationMessageHeader_calcSizeBinary(&respHeader) */
        /* + UA_AsymmetricAlgorithmSecurityHeader_calcSizeBinary(&asymHeader) */
        /* + UA_SequenceHeader_calcSizeBinary(&seqHeader) */
        /* + UA_NodeId_calcSizeBinary(&responseType) */
        /* + UA_OpenSecureChannelResponse_calcSizeBinary(&p); */

    //UA_SecureConversationMessageHeader respHeader;
    //UA_NodeId responseType = UA_NODEIDS[UA_OPENSECURECHANNELRESPONSE];
    
    return retval;
}
