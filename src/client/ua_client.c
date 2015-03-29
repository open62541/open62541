#include "ua_client.h"
#include "ua_nodeids.h"
#include "ua_types.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"

struct UA_Client {
    UA_ClientNetworkLayer networkLayer;
    UA_String endpointUrl;
    UA_Connection connection;

    UA_UInt32 requestId;
    UA_UInt32 sequenceId;

	UA_UInt32 channelId;
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
    c->requestId = 1;
    c->sequenceId = 1;
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
    SecureChannelHandshake(c);
    // session
    
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT UA_Client_disconnect(UA_Client *c) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode SecureChannelHandshake(UA_Client *c) {
	UA_SecureConversationMessageHeader messageHeader;
	messageHeader.messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_OPNF;
	messageHeader.secureChannelId = 0;

	UA_SequenceHeader seqHeader;
	seqHeader.requestId = c->requestId;
	seqHeader.sequenceNumber = c->sequenceId;

	UA_AsymmetricAlgorithmSecurityHeader asymHeader;
	UA_AsymmetricAlgorithmSecurityHeader_init(&asymHeader);
	UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None", &asymHeader.securityPolicyUri);

	UA_NodeId requestType = UA_NODEID_STATIC(0, UA_NS0ID_OPENSECURECHANNELREQUEST + UA_ENCODINGOFFSET_BINARY);  // id of opensecurechannelrequest

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
    UA_StatusCode retval = c->networkLayer.send(c->networkLayer.nlHandle, buf);
    if(retval)
        return retval;

    // parse the response
    UA_ByteString reply;
    UA_ByteString_newMembers(&reply, c->connection.localConf.recvBufferSize);
    retval = c->networkLayer.awaitResponse(c->networkLayer.nlHandle, &reply, 1000);
    if(retval) {
        UA_ByteString_deleteMembers(&reply);
        return retval;
    }

	offset = 0;

	UA_SecureConversationMessageHeader_decodeBinary(&reply, &offset, &messageHeader);
	UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&reply, &offset, &asymHeader);
	UA_SequenceHeader_decodeBinary(&reply, &offset, &seqHeader);
	UA_NodeId_decodeBinary(&reply, &offset, &requestType);

	c->channelId = messageHeader.secureChannelId;


	//TODO: save other stuff
	UA_OpenSecureChannelResponse response;
	UA_OpenSecureChannelResponse_decodeBinary(&reply, &offset, &response);
    UA_ByteString_deleteMembers(&reply);

	UA_OpenSecureChannelResponse_deleteMembers(&response);
	UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
    return retval;

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

