/*
 C ECHO client example using sockets
 */
#include <stdio.h> //printf
#include <string.h> //strlen
#include <sys/socket.h> //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h> // for close
#include <stdlib.h> // pulls in declaration of malloc, free
#include "ua_transport_generated.h"
#include "ua_namespace_0.h"

UA_Int32 sendHello(UA_Int32 sock, UA_String *endpointURL) {
	UA_ByteString *message;
	UA_ByteString_new(&message);
	UA_ByteString_newMembers(message, 1000);

	UA_UInt32 offset = 0;



	UA_TcpMessageHeader messageHeader;
	UA_TcpHelloMessage hello;
	messageHeader.isFinal = 'F';
	messageHeader.messageType = UA_MESSAGETYPE_HEL;

	UA_String_copy(endpointURL, &hello.endpointUrl);

	hello.maxChunkCount = 1;
	hello.maxMessageSize = 16777216;
	hello.protocolVersion = 0;
	hello.receiveBufferSize = 65536;
	hello.sendBufferSize = 65536;

	messageHeader.messageSize = UA_TcpHelloMessage_calcSizeBinary(
			(UA_TcpHelloMessage const*) &hello)
			+ UA_TcpMessageHeader_calcSizeBinary(
					(UA_TcpMessageHeader const*) &messageHeader);

	UA_TcpMessageHeader_encodeBinary(
			(UA_TcpMessageHeader const*) &messageHeader, message, &offset);
	UA_TcpHelloMessage_encodeBinary((UA_TcpHelloMessage const*) &hello, message,
			&offset);

	UA_Int32 sendret = send(sock, message->data, offset, 0);

	UA_ByteString_delete(message);

	free(hello.endpointUrl.data);
	if (sendret < 0) {
		return 1;
	}
	return 0;

}
int sendOpenSecureChannel(UA_Int32 sock) {
	UA_ByteString *message;
	UA_ByteString_new(&message);
	UA_ByteString_newMembers(message, 1000);

	UA_UInt32 offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_OPN;
	msghdr.messageSize = 135;

	UA_TcpMessageHeader_encodeBinary(&msghdr, message, &offset);
	UA_UInt32 secureChannelId = 0;
	UA_UInt32_encodeBinary(&secureChannelId, message, &offset);
	UA_String securityPolicy;

	UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None",
			&securityPolicy);
	UA_String_encodeBinary(&securityPolicy, message, &offset);

	UA_String senderCert;
	senderCert.data = UA_NULL;
	senderCert.length = -1;
	UA_String_encodeBinary(&senderCert, message, &offset);

	UA_String receiverCertThumb;
	receiverCertThumb.data = UA_NULL;
	receiverCertThumb.length = -1;
	UA_String_encodeBinary(&receiverCertThumb, message, &offset);

	UA_UInt32 sequenceNumber = 51;
	UA_UInt32_encodeBinary(&sequenceNumber, message, &offset);

	UA_UInt32 requestId = 1;
	UA_UInt32_encodeBinary(&requestId, message, &offset);

	UA_NodeId type;
	type.identifier.numeric = 446;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;
	UA_NodeId_encodeBinary(&type, message, &offset);

	UA_OpenSecureChannelRequest opnSecRq;
	UA_OpenSecureChannelRequest_init(&opnSecRq);

	opnSecRq.requestHeader.timestamp = UA_DateTime_now();

	UA_ByteString_newMembers(&opnSecRq.clientNonce, 1);
	opnSecRq.clientNonce.data[0] = 0;

	opnSecRq.clientProtocolVersion = 0;
	opnSecRq.requestedLifetime = 30000;
	opnSecRq.securityMode = UA_SECURITYMODE_NONE;
	opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;

	opnSecRq.requestHeader.authenticationToken.identifier.numeric = 10;
	opnSecRq.requestHeader.authenticationToken.identifierType =
			UA_NODEIDTYPE_NUMERIC;
	opnSecRq.requestHeader.authenticationToken.namespaceIndex = 10;

	UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, message, &offset);
	UA_Int32 sendret = send(sock, message->data, offset, 0);
	UA_ByteString_delete(message);
	free(securityPolicy.data);
	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;
}

UA_Int32 sendCreateSession(UA_Int32 sock, UA_UInt32 channelId,
		UA_UInt32 tokenId, UA_UInt32 sequenceNumber, UA_UInt32 requestId,
		UA_String *endpointUrl) {
	UA_ByteString *message;
	UA_ByteString_new(&message);
	UA_ByteString_newMembers(message, 65536);
	UA_UInt32 tmpChannelId = channelId;
	UA_UInt32 offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_MSG;




	UA_NodeId type;
	type.identifier.numeric = 461;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;


	UA_CreateSessionRequest rq;
	UA_RequestHeader_init(&rq.requestHeader);

	rq.requestHeader.requestHandle = 1;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.timeoutHint = 10000;
	rq.requestHeader.auditEntryId.length = -1;
	rq.requestHeader.authenticationToken.identifier.numeric = 10;
	rq.requestHeader.authenticationToken.identifierType = UA_NODEIDTYPE_NUMERIC;
	rq.requestHeader.authenticationToken.namespaceIndex = 10;
	UA_String_copy(endpointUrl, &rq.endpointUrl);
	rq.clientDescription.applicationName.locale.length = -1;
	rq.clientDescription.applicationName.text.length = -1;

	rq.clientDescription.applicationUri.length = -1;
	rq.clientDescription.discoveryProfileUri.length = -1;
	rq.clientDescription.discoveryUrls = UA_NULL;
	rq.clientDescription.discoveryUrlsSize = -1;
	rq.clientDescription.gatewayServerUri.length = -1;
	rq.clientDescription.productUri.length = -1;

	UA_String_copycstring("mysession", &rq.sessionName);

	UA_String_copycstring("abcd", &rq.clientCertificate);

	UA_ByteString_newMembers(&rq.clientNonce, 1);
	rq.clientNonce.data[0] = 0;

	rq.requestedSessionTimeout = 1200000;
	rq.maxResponseMessageSize = UA_INT32_MAX;



	msghdr.messageSize = 16 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) + UA_CreateSessionRequest_calcSizeBinary(&rq);
	UA_TcpMessageHeader_encodeBinary(&msghdr, message, &offset);

	UA_UInt32_encodeBinary(&tmpChannelId, message, &offset);
	UA_UInt32_encodeBinary(&tokenId, message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, message, &offset);
	UA_UInt32_encodeBinary(&requestId, message, &offset);
	UA_NodeId_encodeBinary(&type, message, &offset);
	UA_CreateSessionRequest_encodeBinary(&rq, message, &offset);

	UA_Int32 sendret = send(sock, message->data, offset, 0);
	UA_ByteString_delete(message);
	free(rq.sessionName.data);
	free(rq.clientCertificate.data);
	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;
}
UA_Int32 sendActivateSession(UA_Int32 sock, UA_UInt32 channelId,
		UA_UInt32 tokenId, UA_UInt32 sequenceNumber, UA_UInt32 requestId, UA_NodeId *authenticationToken) {
	UA_ByteString *message;
	UA_ByteString_new(&message);
	UA_ByteString_newMembers(message, 65536);
	UA_UInt32 tmpChannelId = channelId;
	UA_UInt32 offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_MSG;
	msghdr.messageSize = 86;



	UA_NodeId type;
	type.identifier.numeric = 467;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;


	UA_ActivateSessionRequest rq;
	UA_ActivateSessionRequest_init(&rq);
	rq.requestHeader.requestHandle = 2;

	rq.requestHeader.authenticationToken.identifier.numeric = authenticationToken->identifier.numeric;
	rq.requestHeader.authenticationToken.identifierType = authenticationToken->identifierType;
	rq.requestHeader.authenticationToken.namespaceIndex = authenticationToken->namespaceIndex;

	msghdr.messageSize  = 16 +UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) + UA_ActivateSessionRequest_calcSizeBinary(&rq);

	UA_TcpMessageHeader_encodeBinary(&msghdr, message, &offset);

	UA_UInt32_encodeBinary(&tmpChannelId, message, &offset);
	UA_UInt32_encodeBinary(&tokenId, message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, message, &offset);
	UA_UInt32_encodeBinary(&requestId, message, &offset);
	UA_NodeId_encodeBinary(&type, message, &offset);
	UA_ActivateSessionRequest_encodeBinary(&rq, message, &offset);

	UA_Int32 sendret = send(sock, message->data, offset, 0);
	UA_ByteString_delete(message);

	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;

}

UA_Int64 sendReadRequest(UA_Int32 sock, UA_UInt32 channelId, UA_UInt32 tokenId,
		UA_UInt32 sequenceNumber, UA_UInt32 requestId,UA_NodeId *authenticationToken, UA_Int32 nodeIds_size,UA_NodeId* nodeIds) {
	UA_ByteString *message;
	UA_ByteString_new(&message);
	UA_ByteString_newMembers(message, 65536);
	UA_UInt32 tmpChannelId = channelId;
	UA_UInt32 offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_MSG;


	UA_NodeId type;
	type.identifier.numeric = 631;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;


	UA_ReadRequest rq;
	UA_ReadRequest_init(&rq);

	rq.maxAge = 0;

	UA_Array_new((void **) &rq.nodesToRead, nodeIds_size, &UA_[UA_READVALUEID]);
	rq.nodesToReadSize = nodeIds_size;
	for(UA_Int32 i=0;i<nodeIds_size;i++)
	{
		UA_ReadValueId_init(&(rq.nodesToRead[0]));
		rq.nodesToRead[i].attributeId = 13; //UA_ATTRIBUTEID_VALUE
		UA_NodeId_init(&(rq.nodesToRead[i].nodeId));
		rq.nodesToRead[i].nodeId = nodeIds[i];
		UA_QualifiedName_init(&(rq.nodesToRead[0].dataEncoding));
	}
	rq.requestHeader.timeoutHint = 10000;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.authenticationToken.identifier.numeric = authenticationToken->identifier.numeric;
	rq.requestHeader.authenticationToken.identifierType = authenticationToken->identifierType;
	rq.requestHeader.authenticationToken.namespaceIndex = authenticationToken->namespaceIndex;

	rq.timestampsToReturn = 0x03;
	msghdr.messageSize = 16 +UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) + UA_ReadRequest_calcSizeBinary(&rq);
	UA_TcpMessageHeader_encodeBinary(&msghdr,message,&offset);
	UA_UInt32_encodeBinary(&tmpChannelId, message, &offset);
	UA_UInt32_encodeBinary(&tokenId, message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, message, &offset);
	UA_UInt32_encodeBinary(&requestId, message, &offset);
	UA_NodeId_encodeBinary(&type,message,&offset);
	UA_ReadRequest_encodeBinary(&rq, message, &offset);

	UA_DateTime tic = UA_DateTime_now();
	UA_Int32 sendret = send(sock, message->data, offset, 0);
	UA_Array_delete(rq.nodesToRead,nodeIds_size,&UA_[UA_READVALUEID]);
	UA_ByteString_delete(message);



	rq.requestHeader.requestHandle = 3;
	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return tic;
}
int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in server;

	UA_ByteString *reply;
	UA_ByteString_new(&reply);
	UA_ByteString_newMembers(reply, 65536);

	//start parameters
	if(argc < 5)
	{
		printf("1st parameter: number of nodes to read \n");
		printf("2nd parameter: number of read-tries \n");
		printf("3rd parameter: name of the file to save measurement data \n");
		printf("4th parameter: 1 = read same node, 0 = read different nodes \n");
		return 0;
	}


	UA_UInt32 nodesToReadSize;
	UA_UInt32 tries;
	UA_Boolean alwaysSameNode;
	if(argv[1] == UA_NULL){
		nodesToReadSize = 1;
	}else{
		nodesToReadSize = atoi(argv[1]);
	}

	if(argv[2] == UA_NULL){
		tries= 2;
	}else{
		tries = (UA_UInt32) atoi(argv[2]);
	}
	if(atoi(argv[4]) != 0)
	{
		alwaysSameNode = UA_TRUE;
	}
	else
	{
		alwaysSameNode = UA_FALSE;
	}



	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printf("Could not create socket");
	}
	server.sin_addr.s_addr = inet_addr("134.130.125.48");
	server.sin_family = AF_INET;
	server.sin_port = htons(16663);
//Connect to remote server
	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}
	UA_String *endpointUrl;
	UA_String_new(&endpointUrl);

	UA_String_copycstring("opc.tcp://blablablub.com:16664", endpointUrl);
	sendHello(sock, endpointUrl);
	int received = recv(sock, reply->data, reply->length, 0);
	sendOpenSecureChannel(sock);
	received = recv(sock, reply->data, reply->length, 0);


	UA_UInt32 recvOffset = 0;
	UA_TcpMessageHeader msghdr;
	UA_TcpMessageHeader_decodeBinary(reply, &recvOffset, &msghdr);
	UA_UInt32 secureChannelId;
	UA_UInt32_decodeBinary(reply, &recvOffset, &secureChannelId);
	sendCreateSession(sock, secureChannelId, 1, 52, 2, endpointUrl);
	received = recv(sock, reply->data, reply->length, 0);


	UA_NodeId messageType;
	recvOffset = 24;
	UA_NodeId_decodeBinary(reply,&recvOffset,&messageType);
	UA_CreateSessionResponse createSessionResponse;
	UA_CreateSessionResponse_decodeBinary(reply,&recvOffset,&createSessionResponse);

	sendActivateSession(sock, secureChannelId, 1, 53, 3,&createSessionResponse.authenticationToken);
	received = recv(sock, reply->data, reply->length, 0);

    UA_NodeId *nodesToRead;

    UA_Array_new((void**)&nodesToRead,nodesToReadSize,&UA_[UA_NODEID]);
	for(UA_UInt32 i = 0; i<nodesToReadSize; i++){
		UA_NodeId_new((UA_NodeId**)&nodesToRead[i]);
		if(alwaysSameNode){
			nodesToRead[i].identifier.numeric = 2255; //ask always the same node
		}
		else{
			nodesToRead[i].identifier.numeric = 19000 +i;
		}
		nodesToRead[i].identifierType = UA_NODEIDTYPE_NUMERIC;
		nodesToRead[i].namespaceIndex = 0;
	}

	UA_DateTime tic, toc;
	UA_Double *timeDiffs;

	UA_Array_new((void**)&timeDiffs,tries,&UA_[UA_DOUBLE]);
	UA_Double sum = 0;

	for (UA_UInt32 i = 0; i < tries; i++) {

		tic = sendReadRequest(sock, secureChannelId, 1+i, 54+i, 4+i,&createSessionResponse.authenticationToken,nodesToReadSize,nodesToRead);

		received = recv(sock, reply->data, 2000, 0);
		toc = UA_DateTime_now() - tic;

		timeDiffs[i] = (UA_Double)toc/(UA_Double)10e3;
		sum = sum + timeDiffs[i];
		printf("read request took: %16.10f ms \n",timeDiffs[i]);
	}

	UA_Double mean = sum / tries;
	printf("mean time for handling request: %16.10f ms \n",mean);

	if(received>0)//dummy
	{
		printf("%i",received);
	}
	//save to file
	char data[100];
	const char flag = 'a';
	FILE* fHandle =  fopen("measurement",&flag);
	//header

	UA_Int32 bytesToWrite = sprintf(data,"measurement %s, nodesToRead %d \n",argv[3],nodesToReadSize);

	fwrite(data,1,bytesToWrite,fHandle);

	for(UA_UInt32 i=0;i<tries;i++){
		bytesToWrite = sprintf(data,"t%d,%16.10f \n",i,timeDiffs[i]);
		fwrite(data,1,bytesToWrite,fHandle);
	}
	fclose(fHandle);
	UA_String_delete(endpointUrl);
	UA_Array_delete(nodesToRead,nodesToReadSize,&UA_[UA_NODEID]);
	close(sock);
	return 0;

}
