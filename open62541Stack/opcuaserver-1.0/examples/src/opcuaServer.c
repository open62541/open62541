/*
 ============================================================================
 Name        : opcuaServer.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>



#include "../../src/opcua_binaryEncDec.h"
#include "../../src/opcua_builtInDatatypes.h"
#include "../../src/opcua_transportLayer.h"
#include "../../src/opcua_types.h"

#ifdef LINUX

#include <sys/socket.h>
#include <netinet/in.h>

void server_init();
void server_run();

#endif

int main(void)
{

#ifdef LINUX
	server_init();
	server_run();
#endif

	return EXIT_SUCCESS;

}

#ifdef LINUX

void server_init()
{
	puts("starting demo Server");
	//call listen

}
void server_run()
{
	int server_state = 0;
	int recv_data = 0;
	int send_data = 1;
	int new_client = 2;
	int new_request = 3;
	char buf[8192];
	struct sockaddr_in self;
	int sockfd;
	int clientfd;

	struct sockaddr_in client_addr;
	int addrlen=sizeof(client_addr);

	//---Create streaming socket---
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
    	puts("socket error");
	}
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(4840);
	self.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(sockfd,(struct sockaddr *)&self , sizeof(self)) < 0)
	{


	   //Fehler bei bind()
	 }

	//---Make it a "listening socket"---
	if ( listen(sockfd, 1) != 0 )
	{
		puts("listen error");
	}
	clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
	server_state = 0;
	while(1)
	{
		//call recv (nonblocking)

		//call TL_getPacketType

		//if newData
		//
		UA_connection connection;
		AD_RawMessage *rawMessage;
		rawMessage->message = buf;
		rawMessage->length = 0;
		switch(server_state)
		{

			recv_data :
			{

				//call receive function
				rawMessage->length = recv(sockfd,buf,8192,0);
				if(rawMessage->length > 0)
				{
					server_state = new_client;
				}
				break;
			}
			send_data :
			{
				//call send function
				break;
			}
			new_client :
			{
				if(connection.transportLayer.connectionState != connectionState_ESTABLISHED)
				{
					TL_open(connection,rawMessage);
				}
//			else
//				{
//					SL_open(connection,rawMessage);
//
//				}

			}
			new_request :
			{


				break;
			}

		}
		//if newClient


		//TL_processHELMessage(&connection,);

		//--------
		//call listen
	}

}

#endif
