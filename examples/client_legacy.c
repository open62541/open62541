/*
 C ECHO client example using sockets

 This is an example client for internal benchmarks. It works, but is not ready
 for serious use. We do not really check any of the returns from the server.
 */
#include <stdio.h> //printf
#include <string.h> //strlen
#include <sys/socket.h> //socket
#include <arpa/inet.h> //inet_addr
#include <unistd.h> // for close
#include <stdlib.h> // pulls in declaration of malloc, free

#ifdef NOT_AMALGATED
    #include "ua_transport_generated.h"
    #include "ua_types_encoding_binary.h"
    #include "ua_util.h"
#else
    #include "open62541.h"
#endif

typedef struct ConnectionInfo {
	UA_Int32 socket;
	UA_UInt32 channelId;
	UA_SequenceHeader sequenceHdr;
	UA_NodeId authenticationToken;
	UA_UInt32 tokenId;
} ConnectionInfo;

static UA_Int32 sendHello(UA_Int32 sock, UA_String *endpointURL) {

	UA_TcpMessageHeader messageHeader;
	messageHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_HELF;

	UA_TcpHelloMessage hello;
	UA_String_copy(endpointURL, &hello.endpointUrl);
	hello.maxChunkCount = 1;
	hello.maxMessageSize = 16777216;
	hello.protocolVersion = 0;
	hello.receiveBufferSize = 65536;
	hello.sendBufferSize = 65536;

	messageHeader.messageSize = UA_TcpHelloMessage_calcSizeBinary((UA_TcpHelloMessage const*) &hello) +
                                UA_TcpMessageHeader_calcSizeBinary((UA_TcpMessageHeader const*) &messageHeader);
	UA_ByteString message;
	UA_ByteString_newMembers(&message, messageHeader.messageSize);

	size_t offset = 0;
	UA_TcpMessageHeader_encodeBinary((UA_TcpMessageHeader const*) &messageHeader, &message, &offset);
	UA_TcpHelloMessage_encodeBinary((UA_TcpHelloMessage const*) &hello, &message, &offset);

	UA_Int32 sendret = send(sock, message.data, offset, 0);

	UA_ByteString_deleteMembers(&message);
	free(hello.endpointUrl.data);
	if (sendret < 0)
		return 1;
	return 0;
}

static int sendOpenSecureChannel(UA_Int32 sock) {
	UA_TcpMessageHeader msghdr;
	msghdr.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_OPNF;

	UA_UInt32 secureChannelId = 0;
	UA_String securityPolicy;
	securityPolicy = UA_STRING("http://opcfoundation.org/UA/SecurityPolicy#None");

	UA_String senderCert;
	senderCert.data = UA_NULL;
	senderCert.length = -1;

	UA_String receiverCertThumb;
	receiverCertThumb.data = UA_NULL;
	receiverCertThumb.length = -1;

	UA_UInt32 sequenceNumber = 51;

	UA_UInt32 requestId = 1;

	UA_NodeId type;
	type.identifier.numeric = 446; // id of opensecurechannelrequest
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

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

	msghdr.messageSize = 135; // todo: compute the message size from the actual content

	UA_ByteString message;
	UA_ByteString_newMembers(&message, 1000);
	size_t offset = 0;
	UA_TcpMessageHeader_encodeBinary(&msghdr, &message, &offset);
	UA_UInt32_encodeBinary(&secureChannelId, &message, &offset);
	UA_String_encodeBinary(&securityPolicy, &message, &offset);
	UA_String_encodeBinary(&senderCert, &message, &offset);
	UA_String_encodeBinary(&receiverCertThumb, &message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, &message, &offset);
	UA_UInt32_encodeBinary(&requestId, &message, &offset);
	UA_NodeId_encodeBinary(&type, &message, &offset);
	UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq, &message, &offset);

    UA_OpenSecureChannelRequest_deleteMembers(&opnSecRq);
	UA_String_deleteMembers(&securityPolicy);

	UA_Int32 sendret = send(sock, message.data, offset, 0);
	UA_ByteString_deleteMembers(&message);
	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;
}

static UA_Int32 sendCreateSession(UA_Int32 sock, UA_UInt32 channelId, UA_UInt32 tokenId, UA_UInt32 sequenceNumber,
                                  UA_UInt32 requestId, UA_String *endpointUrl) {
    UA_ByteString message;
	UA_ByteString_newMembers(&message, 65536);
	UA_UInt32 tmpChannelId = channelId;
	size_t offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;

	UA_NodeId type;
	type.identifier.numeric = 461;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

	UA_CreateSessionRequest rq;
    UA_CreateSessionRequest_init(&rq);
	rq.requestHeader.requestHandle = 1;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.timeoutHint = 10000;
	rq.requestHeader.authenticationToken.identifier.numeric = 10;
	rq.requestHeader.authenticationToken.identifierType = UA_NODEIDTYPE_NUMERIC;
	rq.requestHeader.authenticationToken.namespaceIndex = 10;
	UA_String_copy(endpointUrl, &rq.endpointUrl);
	rq.sessionName = UA_STRING("mysession");
	rq.clientCertificate = UA_STRING("abcd");
	UA_ByteString_newMembers(&rq.clientNonce, 1);
	rq.clientNonce.data[0] = 0;
	rq.requestedSessionTimeout = 1200000;
	rq.maxResponseMessageSize = UA_INT32_MAX;

	msghdr.messageSize = 16 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) +
                         UA_CreateSessionRequest_calcSizeBinary(&rq);

	UA_TcpMessageHeader_encodeBinary(&msghdr, &message, &offset);
	UA_UInt32_encodeBinary(&tmpChannelId, &message, &offset);
	UA_UInt32_encodeBinary(&tokenId, &message, &offset);
	UA_UInt32_encodeBinary(&sequenceNumber, &message, &offset);
	UA_UInt32_encodeBinary(&requestId, &message, &offset);
	UA_NodeId_encodeBinary(&type, &message, &offset);
	UA_CreateSessionRequest_encodeBinary(&rq, &message, &offset);

	UA_Int32 sendret = send(sock, message.data, offset, 0);
	UA_ByteString_deleteMembers(&message);
	UA_CreateSessionRequest_deleteMembers(&rq);
	if (sendret < 0) {
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;
}

static UA_Int32 closeSession(ConnectionInfo *connectionInfo) {
	size_t offset = 0;

	UA_ByteString message;
	UA_ByteString_newMembers(&message, 65536);

	UA_CloseSessionRequest rq;
    UA_CloseSessionRequest_init(&rq);

	rq.requestHeader.requestHandle = 1;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.timeoutHint = 10000;
	rq.requestHeader.authenticationToken.identifier.numeric = 10;
	rq.requestHeader.authenticationToken.identifierType = UA_NODEIDTYPE_NUMERIC;
	rq.requestHeader.authenticationToken.namespaceIndex = 10;
    rq.deleteSubscriptions = UA_TRUE;

	UA_TcpMessageHeader msghdr;
	msghdr.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;

	UA_NodeId type;
	type.identifier.numeric = 473;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

	msghdr.messageSize = 16 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) +
                         UA_CloseSessionRequest_calcSizeBinary(&rq);

	UA_TcpMessageHeader_encodeBinary(&msghdr, &message, &offset);
	UA_UInt32_encodeBinary(&connectionInfo->channelId, &message, &offset);
	UA_UInt32_encodeBinary(&connectionInfo->tokenId, &message, &offset);
	UA_UInt32_encodeBinary(&connectionInfo->sequenceHdr.sequenceNumber, &message, &offset);
	UA_UInt32_encodeBinary(&connectionInfo->sequenceHdr.requestId, &message, &offset);
	UA_NodeId_encodeBinary(&type, &message, &offset);
	UA_CloseSessionRequest_encodeBinary(&rq, &message, &offset);

	UA_Int32 sendret = send(connectionInfo->socket, message.data, offset, 0);
	UA_ByteString_deleteMembers(&message);
	UA_CloseSessionRequest_deleteMembers(&rq);
	if(sendret < 0) {
		printf("send closesessionrequest failed");
		return 1;
	}

    return 0;
}

static UA_Int32 closeSecureChannel(ConnectionInfo *connectionInfo) {
	size_t offset = 0;

	UA_ByteString message;
	UA_ByteString_newMembers(&message, 65536);

	UA_CloseSecureChannelRequest rq;
    UA_CloseSecureChannelRequest_init(&rq);

	rq.requestHeader.requestHandle = 1;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.timeoutHint = 10000;
	rq.requestHeader.authenticationToken.identifier.numeric = 10;
	rq.requestHeader.authenticationToken.identifierType = UA_NODEIDTYPE_NUMERIC;
	rq.requestHeader.authenticationToken.namespaceIndex = 10;

	UA_TcpMessageHeader msghdr;
	msghdr.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_CLOF;

	msghdr.messageSize = 4 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) +
                         UA_CloseSecureChannelRequest_calcSizeBinary(&rq);

	UA_TcpMessageHeader_encodeBinary(&msghdr, &message, &offset);
	UA_UInt32_encodeBinary(&connectionInfo->channelId, &message, &offset);
	UA_CloseSecureChannelRequest_encodeBinary(&rq, &message, &offset);

	UA_Int32 sendret = send(connectionInfo->socket, message.data, offset, 0);
	UA_ByteString_deleteMembers(&message);
	UA_CloseSecureChannelRequest_deleteMembers(&rq);
	if(sendret < 0) {
		printf("send CloseSecureChannelRequest failed");
		return 1;
	}

    return 0;
}

static UA_Int32 sendActivateSession(UA_Int32 sock, UA_UInt32 channelId, UA_UInt32 tokenId, UA_UInt32 sequenceNumber,
                                    UA_UInt32 requestId, UA_NodeId authenticationToken) {
	UA_ByteString *message = UA_ByteString_new();
	UA_ByteString_newMembers(message, 65536);
	UA_UInt32 tmpChannelId = channelId;
	size_t offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;
	msghdr.messageSize = 86;

	UA_NodeId type;
	type.identifier.numeric = 467;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

	UA_ActivateSessionRequest rq;
	UA_ActivateSessionRequest_init(&rq);
	rq.requestHeader.requestHandle = 2;
	rq.requestHeader.authenticationToken = authenticationToken;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.timeoutHint = 10000;
    
	msghdr.messageSize  = 16 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) +
                          UA_ActivateSessionRequest_calcSizeBinary(&rq);

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

static UA_Int64 sendReadRequest(ConnectionInfo *connectionInfo, UA_Int32 nodeIds_size,UA_NodeId* nodeIds){
		/*UA_Int32 sock, UA_UInt32 channelId, UA_UInt32 tokenId, UA_UInt32 sequenceNumber, UA_UInt32 requestId,
                         UA_NodeId authenticationToken, UA_Int32 nodeIds_size,UA_NodeId* nodeIds) {
                         */
	UA_ByteString *message = UA_ByteString_new();
	UA_ByteString_newMembers(message, 65536);
	UA_UInt32 tmpChannelId = connectionInfo->channelId;
	size_t offset = 0;

	UA_TcpMessageHeader msghdr;
	msghdr.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;

	UA_NodeId type;
	type.identifier.numeric = 631;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;

	UA_ReadRequest rq;
	UA_ReadRequest_init(&rq);
	rq.maxAge = 0;
	rq.nodesToRead = UA_Array_new(&UA_TYPES[UA_TYPES_READVALUEID], nodeIds_size);
	rq.nodesToReadSize = 1;
	for(UA_Int32 i=0;i<nodeIds_size;i++) {
		UA_ReadValueId_init(&(rq.nodesToRead[i]));
		rq.nodesToRead[i].attributeId = 6; //WriteMask
		UA_NodeId_init(&(rq.nodesToRead[i].nodeId));
		rq.nodesToRead[i].nodeId = nodeIds[i];
		UA_QualifiedName_init(&(rq.nodesToRead[0].dataEncoding));
	}
	rq.requestHeader.timeoutHint = 10000;
	rq.requestHeader.timestamp = UA_DateTime_now();
	rq.requestHeader.authenticationToken = connectionInfo->authenticationToken;
	rq.timestampsToReturn = 0x03;
	rq.requestHeader.requestHandle = 1 + connectionInfo->sequenceHdr.requestId;

	msghdr.messageSize = 16 + UA_TcpMessageHeader_calcSizeBinary(&msghdr) + UA_NodeId_calcSizeBinary(&type) +
                         UA_ReadRequest_calcSizeBinary(&rq);

	UA_TcpMessageHeader_encodeBinary(&msghdr,message,&offset);
	UA_UInt32_encodeBinary(&tmpChannelId, message, &offset);
	UA_UInt32_encodeBinary(&connectionInfo->tokenId, message, &offset);
	UA_UInt32_encodeBinary(&connectionInfo->sequenceHdr.sequenceNumber, message, &offset);
	UA_UInt32_encodeBinary(&connectionInfo->sequenceHdr.requestId, message, &offset);
	UA_NodeId_encodeBinary(&type,message,&offset);
	UA_ReadRequest_encodeBinary(&rq, message, &offset);

	UA_DateTime tic = UA_DateTime_now();
	UA_Int32 sendret = send(connectionInfo->socket, message->data, offset, 0);
	UA_Array_delete(rq.nodesToRead, &UA_TYPES[UA_TYPES_READVALUEID], nodeIds_size);
	UA_ByteString_delete(message);

	if (sendret < 0) {
		printf("send readrequest failed");
		return 1;
	}
	return tic;
}

static int ua_client_connectUA(char* ipaddress,int port, UA_String *endpointUrl, ConnectionInfo *connectionInfo,
                               UA_Boolean stateless, UA_Boolean udp) {
	UA_ByteString reply;
	UA_ByteString_newMembers(&reply, 65536);
	int sock;
	struct sockaddr_in server;
	//Create socket
	if(udp==UA_TRUE){
		sock = socket(AF_INET, SOCK_DGRAM, 0);
	}else{
		sock = socket(AF_INET, SOCK_STREAM, 0);
	}
	if(sock == -1) {
		printf("Could not create socket");
        return 1;
    }
	server.sin_addr.s_addr = inet_addr(ipaddress);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if(connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
			perror("connect failed. Error");
			return 1;
		}
		connectionInfo->socket = sock;

		if(stateless){
			UA_NodeId_init(&connectionInfo->authenticationToken);
			connectionInfo->channelId=0;
			UA_SequenceHeader_init(&connectionInfo->sequenceHdr);
			connectionInfo->tokenId=0;
			return 0;
		}else{
			sendHello(sock, endpointUrl);
			recv(sock, reply.data, reply.length, 0);
			sendOpenSecureChannel(sock);
			recv(sock, reply.data, reply.length, 0);

			size_t recvOffset = 0;
			UA_TcpMessageHeader msghdr;
			UA_TcpMessageHeader_decodeBinary(&reply, &recvOffset, &msghdr);

			UA_AsymmetricAlgorithmSecurityHeader asymHeader;
			UA_NodeId rspType;
			UA_OpenSecureChannelResponse openSecChannelRsp;
			UA_UInt32_decodeBinary(&reply, &recvOffset, &connectionInfo->channelId);
			UA_AsymmetricAlgorithmSecurityHeader_decodeBinary(&reply,&recvOffset,&asymHeader);
			UA_AsymmetricAlgorithmSecurityHeader_deleteMembers(&asymHeader);
			UA_SequenceHeader_decodeBinary(&reply,&recvOffset,&connectionInfo->sequenceHdr);
			UA_NodeId_decodeBinary(&reply,&recvOffset,&rspType);
			UA_OpenSecureChannelResponse_decodeBinary(&reply,&recvOffset,&openSecChannelRsp);
			connectionInfo->tokenId = openSecChannelRsp.securityToken.tokenId;

			sendCreateSession(sock, connectionInfo->channelId, openSecChannelRsp.securityToken.tokenId, 52, 2, endpointUrl);
			recv(sock, reply.data, reply.length, 0);

			UA_NodeId messageType;
			recvOffset = 24;
			UA_NodeId_decodeBinary(&reply,&recvOffset,&messageType);
			UA_CreateSessionResponse createSessionResponse;
			UA_CreateSessionResponse_decodeBinary(&reply,&recvOffset,&createSessionResponse);
			connectionInfo->authenticationToken = createSessionResponse.authenticationToken;
			sendActivateSession(sock, connectionInfo->channelId, connectionInfo->tokenId, 53, 3,
					connectionInfo->authenticationToken);
			recv(sock, reply.data, reply.length, 0);

			UA_OpenSecureChannelResponse_deleteMembers(&openSecChannelRsp);

			UA_String_deleteMembers(&reply);
			UA_CreateSessionResponse_deleteMembers(&createSessionResponse);
			return 0;
		}
}

int main(int argc, char *argv[]) {
	int defaultParams = argc < 8;

	//start parameters
	if(defaultParams) {
		printf("1st parameter: number of nodes to read \n");
		printf("2nd parameter: number of read-tries \n");
		printf("3rd parameter: name of the file to save measurement data \n");
		printf("4th parameter: 1 = read same node, 0 = read different nodes \n");
		printf("5th parameter: ip adress \n");
		printf("6th parameter: port \n");
		printf("7th parameter: 0=stateful, 1=stateless\n");
		printf("8th parameter: 0=tcp, 1=udp (only with stateless calls)\n");
		printf("\nUsing default parameters. \n");
	}

	UA_UInt32 nodesToReadSize;
	UA_UInt32 tries;
	UA_Boolean alwaysSameNode;
	UA_ByteString reply;
	UA_ByteString_newMembers(&reply, 65536);
	UA_Boolean stateless;
	UA_Boolean udp;

	if(defaultParams)
		nodesToReadSize = 1;
	else
		nodesToReadSize = atoi(argv[1]);

	if(defaultParams)
		tries= 2;
	else
		tries = (UA_UInt32) atoi(argv[2]);

	if(defaultParams){
		alwaysSameNode = UA_TRUE;
	}else{
		if(atoi(argv[4]) != 0)
			alwaysSameNode = UA_TRUE;
		else
			alwaysSameNode = UA_FALSE;
	}

	if(defaultParams){
		stateless = UA_FALSE;
	}else{
		if(atoi(argv[7]) != 0)
			stateless = UA_TRUE;
		else
			stateless = UA_FALSE;
	}

	if(defaultParams){
		udp = UA_FALSE;
	}else{
		if(atoi(argv[8]) != 0)
			udp = UA_TRUE;
		else
			udp = UA_FALSE;
	}



    //Connect to remote server
	UA_String endpoint;
	endpoint = UA_STRING("none");
	ConnectionInfo connectionInfo;


/* REQUEST START*/
    UA_NodeId *nodesToRead;
    nodesToRead = UA_Array_new(&UA_TYPES[UA_TYPES_NODEID], 1);

	for(UA_UInt32 i = 0; i<1; i++) {
		if(alwaysSameNode)
			nodesToRead[i].identifier.numeric = 2253; //ask always the same node
		else
			nodesToRead[i].identifier.numeric = 19000 +i;
		nodesToRead[i].identifierType = UA_NODEIDTYPE_NUMERIC;
		nodesToRead[i].namespaceIndex = 0;
	}

	UA_DateTime tic, toc;
	UA_Double *timeDiffs;
	UA_Int32 received = 0;
	timeDiffs = UA_Array_new(&UA_TYPES[UA_TYPES_DOUBLE], tries);
	UA_Double sum = 0;

	tic = UA_DateTime_now();

	/**
	UA_Double duration;

	UA_UInt32 count = 0;
	UA_Double start = 0, stop = 0;

	UA_UInt32 timeToRun = 30;
	UA_UInt32 timeToStart = 8;
	UA_UInt32 timeToStop = 22;

	do{
		toc = UA_DateTime_now();
		duration = ((UA_Double)toc-(UA_Double)tic)/(UA_Double)1e4;
		if(duration>=timeToStart*1000 && duration <= timeToStop*1000){
			if(start==0.0){
				start=UA_DateTime_now();
			}
		}
			//if(stateless || (!stateless && i==0)){
				if(defaultParams){
					if(ua_client_connectUA("127.0.0.1",atoi("16664"),&endpoint,&connectionInfo,stateless,udp) != 0){
						return 0;
					}
				}else{
					if(ua_client_connectUA(argv[5],atoi(argv[6]),&endpoint,&connectionInfo,stateless,udp) != 0){
						return 0;
					}
				}
			//}
			sendReadRequest(&connectionInfo,1,nodesToRead);
			received = recv(connectionInfo.socket, reply.data, 2000, 0);
			if(duration>=timeToStart*1000 && duration <= timeToStop*1000){
				count++;
			}

			if(!stateless){
			closeSession(&connectionInfo);
			recv(connectionInfo.socket, reply.data, 2000, 0);

			closeSecureChannel(&connectionInfo);
			}
			//if(stateless || (!stateless && i==tries-1)){
				close(connectionInfo.socket);
			//}
		if(duration >= timeToStop*1000 && stop==0){
			stop=UA_DateTime_now();
			printf("%i messages in %f secs, rate %f m/s\n", count, (stop-start)/(UA_Double)1e7, (UA_Double)count/((stop-start)/(UA_Double)1e7));
		}

	}while(duration<timeToRun*1000);

	exit(0);
	**/

	for(UA_UInt32 i = 0; i < tries; i++) {
		//if(stateless || (!stateless && i==0)){
		tic = UA_DateTime_now();
			if(defaultParams){
				if(ua_client_connectUA("127.0.0.1",atoi("16664"),&endpoint,&connectionInfo,stateless,udp) != 0){
					return 0;
				}
			}else{
				if(ua_client_connectUA(argv[5],atoi(argv[6]),&endpoint,&connectionInfo,stateless,udp) != 0){
					return 0;
				}
			}
		//}
		for(UA_UInt32 i = 0; i < nodesToReadSize; i++) {
		sendReadRequest(&connectionInfo,1,nodesToRead);
		received = recv(connectionInfo.socket, reply.data, 2000, 0);
		}
			if(!stateless){
		closeSession(&connectionInfo);
		recv(connectionInfo.socket, reply.data, 2000, 0);
			closeSecureChannel(&connectionInfo);
		}
		//if(stateless || (!stateless && i==tries-1)){
			close(connectionInfo.socket);
		//}
		toc = UA_DateTime_now() - tic;
		timeDiffs[i] = (UA_Double)toc/(UA_Double)1e4;
		sum = sum + timeDiffs[i];
	}

/* REQUEST END*/

	UA_Double mean = sum / tries;
	printf("mean time for handling request: %16.10f ms \n",mean);

	if(received>0)
		printf("received: %i\n",received); // dummy


	//save to file
	char data[100];
	const char flag = 'a';
	FILE* fHandle = UA_NULL;
	if (defaultParams) {
		fHandle =  fopen("client.log", &flag);
	}else{
		fHandle =  fopen(argv[3], &flag);
	}
	//header

	UA_Int32 bytesToWrite = sprintf(data, "measurement %s in ms, nodesToRead %d \n", argv[3], 1);
	fwrite(data,1,bytesToWrite,fHandle);
	for(UA_UInt32 i=0;i<tries;i++) {
		bytesToWrite = sprintf(data,"%16.10f \n",timeDiffs[i]);
		fwrite(data,1,bytesToWrite,fHandle);
	}
	fclose(fHandle);

	UA_String_deleteMembers(&reply);
	UA_Array_delete(nodesToRead,&UA_TYPES[UA_TYPES_NODEID], 1);
    UA_free(timeDiffs);

	return 0;
}
