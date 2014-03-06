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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>

void server_init();
void server_run();

#endif

#define PORT    16664
#define MAXMSG  512
#define BUFFER_SIZE 8192

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
	UA_connection connection;
	UA_ByteString slMessage;
	TL_initConnectionObject(&connection);
	char optval;
	int sockfd, newsockfd, portno, clilen;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in serv_addr, cli_addr;
	int  n;

	/* First call to socket() function */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("ERROR opening socket");
		exit(1);
	}

	/* Initialize socket structure */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = PORT;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	optval = 1;

	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int)) == -1) {
	    perror("setsockopt");
	    exit(1);
	}
	/* Now bind the host address using bind() call.*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
						  sizeof(serv_addr)) < 0)
	{
		 perror("ERROR on binding");
		 exit(1);
	}

	/* Now start listening for the clients, here process will
	* go in sleep mode and will wait for the incoming connection
	*/
	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	/* Accept actual connection from the client */
	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
								&clilen);
	if (newsockfd < 0)
	{
		perror("ERROR on accept");
		exit(1);
	}
	printf("One connection accepted");
	while(1)
	{
		/* If connection is established then start communicating */
		bzero(buffer,BUFFER_SIZE);

		n = read( newsockfd,buffer,BUFFER_SIZE);

		if (n > 0)
		{
			printf("received: %s\n",buffer);
			connection.readData.Data = buffer;
			connection.readData.Length = n;
			connection.newDataToRead = 1;

			//TL_receive(&connection, &slMessage);
			SL_receive(&connection, &slMessage);
		}
		else if (n < 0)
		{
			perror("ERROR reading from socket1");
			exit(1);
		}

		if(connection.newDataToWrite)
		{
			printf("data will be sent \n");
			n = write(newsockfd,connection.writeData.Data,connection.writeData.Length);
			printf("sent data \n");
			connection.newDataToWrite = 0;
			opcua_free(connection.writeData.Data);
			connection.writeData.Data = NULL;
			connection.writeData.Length = 0;
		}

		connection.readData.Data = NULL;
		connection.readData.Length = 0;
		connection.newDataToRead = 0;



	}
  }



#endif
