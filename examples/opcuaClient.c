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


UA_Int32 sendHello(UA_Int32 sock)
{
	UA_ByteString *message;
	UA_ByteString_new(&message);
	UA_ByteString_newMembers(message,1000);

	UA_UInt32 offset = 0;
	UA_String endpointUrl;

	UA_String_copycstring("opc.tcp://134.130.125.48:16663",&endpointUrl);
	UA_TcpMessageHeader messageHeader;
	UA_TcpHelloMessage hello;
	messageHeader.isFinal = 'F';
	messageHeader.messageType = UA_MESSAGETYPE_HEL;

	hello.endpointUrl = endpointUrl;
	hello.maxChunkCount = 1;
	hello.maxMessageSize = 16777216;
	hello.protocolVersion = 0;
	hello.receiveBufferSize = 65536;
	hello.sendBufferSize = 65536;


	messageHeader.messageSize = UA_TcpHelloMessage_calcSizeBinary((UA_TcpHelloMessage const*) &hello) +
				UA_TcpMessageHeader_calcSizeBinary((UA_TcpMessageHeader  const*)&messageHeader);

	UA_TcpMessageHeader_encodeBinary((UA_TcpMessageHeader const*) &messageHeader, message, &offset);
	UA_TcpHelloMessage_encodeBinary((UA_TcpHelloMessage const*) &hello, message, &offset);

	UA_Int32 sendret = send(sock , message->data, offset , 0);

	UA_ByteString_delete(message);

	free(endpointUrl.data);
	if(sendret <0)
	{
		return 1;
	}
	return 0;

}
int sendOpenSecureChannel(UA_Int32 sock)
{
	UA_ByteString *message;
	UA_ByteString_new(&message);
	UA_ByteString_newMembers(message,1000);

	UA_UInt32 offset = 0;


	UA_TcpMessageHeader msghdr;
	msghdr.isFinal = 'F';
	msghdr.messageType = UA_MESSAGETYPE_OPN;
	msghdr.messageSize = 133;


	UA_TcpMessageHeader_encodeBinary(&msghdr,message,&offset);
	UA_UInt32 secureChannelId = 0;
	UA_UInt32_encodeBinary(&secureChannelId,message,&offset);
	UA_String securityPolicy;

	UA_String_copycstring("http://opcfoundation.org/UA/SecurityPolicy#None",&securityPolicy);
	UA_String_encodeBinary(&securityPolicy,message,&offset);

	UA_String senderCert;
	senderCert.data = UA_NULL;
	senderCert.length = -1;
	UA_String_encodeBinary(&senderCert,message,&offset);

	UA_String receiverCertThumb;
	receiverCertThumb.data = UA_NULL;
	receiverCertThumb.length = -1;
	UA_String_encodeBinary(&receiverCertThumb,message,&offset);


	UA_UInt32 sequenceNumber = 51;
	UA_UInt32_encodeBinary(&sequenceNumber,message,&offset);

	UA_UInt32 requestId = 1;
	UA_UInt32_encodeBinary(&requestId,message,&offset);


	UA_NodeId type;
	type.identifier.numeric = 446;
	type.identifierType = UA_NODEIDTYPE_NUMERIC;
	type.namespaceIndex = 0;
	UA_NodeId_encodeBinary(&type,message,&offset);

	UA_OpenSecureChannelRequest opnSecRq;
	UA_OpenSecureChannelRequest_init(&opnSecRq);

	opnSecRq.requestHeader.timestamp = UA_DateTime_now();



	UA_ByteString_newMembers(&opnSecRq.clientNonce,1);
	opnSecRq.clientNonce.data[0] = 0;


	opnSecRq.clientProtocolVersion = 0;
	opnSecRq.requestedLifetime = 30000;
	opnSecRq.securityMode = UA_SECURITYMODE_NONE;
	opnSecRq.requestType = UA_SECURITYTOKENREQUESTTYPE_ISSUE;


	UA_OpenSecureChannelRequest_encodeBinary(&opnSecRq,message,&offset);
	UA_Int32 sendret = send(sock , message->data, offset , 0);
	UA_ByteString_delete(message);
	free(securityPolicy.data);
	if(sendret<0)
	{
		printf("send opensecurechannel failed");
		return 1;
	}
	return 0;
}
int main(int argc , char *argv[])
{
int sock;
struct sockaddr_in server;



UA_Byte server_reply[2000];

//Create socket
sock = socket(AF_INET , SOCK_STREAM , 0);
if (sock == -1)
{
printf("Could not create socket");
}
server.sin_addr.s_addr = inet_addr("134.130.125.48");
server.sin_family = AF_INET;
server.sin_port = htons( 16663 );
//Connect to remote server
if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
{
perror("connect failed. Error");
return 1;
}

sendHello(sock);
int received = recv(sock , server_reply , 2000 , 0);
sendOpenSecureChannel(sock);




/*
UA_TcpMessageHeader reqTcpHeader;
UA_UInt32 reqSecureChannelId = 0;
UA_UInt32 reqTokenId = 0;
UA_SequenceHeader reqSequenceHeader;
UA_NodeId reqRequestType;
UA_ReadRequest req;
UA_RequestHeader reqHeader;
UA_NodeId reqHeaderAuthToken;
UA_ExtensionObject reqHeaderAdditionalHeader;
UA_NodeId_init(&reqRequestType);
reqRequestType.identifierType = UA_NODEIDTYPE_NUMERIC;
reqRequestType.identifier.numeric = 631; //read request
UA_ReadRequest_init(&req);
req.requestHeader = reqHeader;
UA_RequestHeader_init(&(req.requestHeader));
req.requestHeader.authenticationToken = reqHeaderAuthToken;
UA_NodeId_init(&(req.requestHeader.authenticationToken));
req.requestHeader.additionalHeader = reqHeaderAdditionalHeader;
UA_ExtensionObject_init(&(req.requestHeader.additionalHeader));
UA_Array_new((void **)&req.nodesToRead, 1, &UA_.types[UA_READVALUEID]);
req.nodesToReadSize = 1;
UA_ReadValueId_init(&(req.nodesToRead[0]));
req.nodesToRead[0].attributeId = 13; //UA_ATTRIBUTEID_VALUE
UA_NodeId_init(&(req.nodesToRead[0].nodeId));
req.nodesToRead[0].nodeId.identifierType = UA_NODEIDTYPE_NUMERIC;
req.nodesToRead[0].nodeId.identifier.numeric = 2255;
UA_QualifiedName_init(&(req.nodesToRead[0].dataEncoding));
messageEncodedLength = UA_TcpMessageHeader_calcSizeBinary(&reqTcpHeader) +
UA_UInt32_calcSizeBinary(&reqSecureChannelId)+
UA_UInt32_calcSizeBinary(&reqTokenId)+
UA_SequenceHeader_calcSizeBinary(&reqSequenceHeader)+
UA_NodeId_calcSizeBinary(&reqRequestType) +
UA_ReadRequest_calcSizeBinary(&req);
UA_TcpMessageHeader_init(&reqTcpHeader);
reqTcpHeader.messageType = UA_MESSAGETYPE_MSG;
reqTcpHeader.messageSize = messageEncodedLength;
reqTcpHeader.isFinal = 'F';
UA_TcpMessageHeader_encodeBinary(&reqTcpHeader, &message, &messagepos);
UA_UInt32_encodeBinary(&reqSecureChannelId, &message, &messagepos);
UA_UInt32_encodeBinary(&reqTokenId, &message, &messagepos);
UA_SequenceHeader_encodeBinary(&reqSequenceHeader, &message, &messagepos);
UA_NodeId_encodeBinary(&reqRequestType, &message, &messagepos);
UA_ReadRequest_encodeBinary(&req, &message, &messagepos);
*/
//Send some data

//Receive a reply from the server
received = recv(sock , server_reply , 2000 , 0);
if(received < 0)
{
puts("recv failed");
return 1;
}
for(int i=0;i<received;i++){
//show only printable ascii
if(server_reply[i] >= 32 && server_reply[i]<= 126)
printf("%c",server_reply[i]);
}
printf("\n");

close(sock);
return 0;
}
