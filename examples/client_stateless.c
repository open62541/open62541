/*
 * C ECHO client example using sockets
 */
#include <stdio.h>	//printf
#include <string.h>	//strlen
#include <sys/socket.h>	//socket
#include <arpa/inet.h>	//inet_addr
#include <unistd.h> // for close
#include <stdlib.h> // pulls in declaration of malloc, free

#include "ua_util.h"
#include "ua_types_encoding_binary.h"
#include "ua_transport_generated.h"

int main(int argc , char *argv[])
{
	int sock;
	struct sockaddr_in server;
	UA_ByteString message;
	message.data = (UA_Byte*)malloc(1000*sizeof(UA_Byte));
	message.length = 1000;
	UA_UInt32 messageEncodedLength = 0;
	UA_Byte server_reply[2000];
	unsigned int messagepos = 0;

	//Create socket
#ifdef EXTENSION_UDP
	sock = socket(AF_INET , SOCK_DGRAM , 0);
#else
	sock = socket(AF_INET , SOCK_STREAM , 0);
#endif
	if (sock == -1)
	{
		printf("Could not create socket");
	}
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons( 16664 );

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		perror("connect failed. Error");
		return 1;
	}


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


	UA_SequenceHeader_init(&reqSequenceHeader);
	reqSequenceHeader.sequenceNumber = 42;

	UA_ReadRequest_init(&req);
	req.requestHeader = reqHeader;
	UA_RequestHeader_init(&(req.requestHeader));
	req.requestHeader.authenticationToken = reqHeaderAuthToken;
	UA_NodeId_init(&(req.requestHeader.authenticationToken));
	req.requestHeader.additionalHeader = reqHeaderAdditionalHeader;
	UA_ExtensionObject_init(&(req.requestHeader.additionalHeader));

	req.nodesToRead= UA_Array_new(&UA_TYPES[UA_TYPES_READVALUEID], 1);
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
	reqTcpHeader.messageTypeAndFinal = UA_MESSAGETYPEANDFINAL_MSGF;
	reqTcpHeader.messageSize = messageEncodedLength;

	UA_TcpMessageHeader_encodeBinary(&reqTcpHeader, &message, &messagepos);
	UA_UInt32_encodeBinary(&reqSecureChannelId, &message, &messagepos);
	UA_UInt32_encodeBinary(&reqTokenId, &message, &messagepos);
	UA_SequenceHeader_encodeBinary(&reqSequenceHeader, &message, &messagepos);
	UA_NodeId_encodeBinary(&reqRequestType, &message, &messagepos);
	UA_ReadRequest_encodeBinary(&req, &message, &messagepos);


	//Send some data
	if( send(sock , message.data, messagepos , 0) < 0)
	{
		puts("Send failed");
		return 1;
	}

	//Receive a reply from the server
	int received = recv(sock , server_reply , 2000 , 0);
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
