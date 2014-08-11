#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "ua_types.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"
#include "networklayer.h"
#include "ua_stack_channel_manager.h"
#include "ua_transport_connection.h"
#include "ua_transport_connection_manager.h"
#include "ua_stack_session_manager.h"

#ifdef LINUX

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>
#include <unistd.h>

void server_run();

#endif

#define PORT 16664
#define MAXMSG 512
#define BUFFER_SIZE 8192



int main(void) {

#ifdef LINUX
	printf("Starting open62541 demo server on port %d\n", PORT);
	server_run();
#endif

	return EXIT_SUCCESS;
}

#ifdef LINUX
void tmpTestFunction()
{

}
void server_run() {
	//just for debugging
#ifdef DEBUG


	tmpTestFunction();
#endif
	UA_TL_Connection connection;// = UA_NULL;
	TL_Buffer localBuffers;
	UA_Int32 connectionState;
	//connection.connectionState = CONNECTIONSTATE_CLOSED;
	//connection.writerCallback = (TL_Writer) NL_TCP_writer;
	localBuffers.maxChunkCount = 1;
	localBuffers.maxMessageSize = BUFFER_SIZE;
	localBuffers.protocolVersion = 0;
	localBuffers.recvBufferSize = BUFFER_SIZE;
	localBuffers.recvBufferSize = BUFFER_SIZE;

	/*init secure Channel manager, which handles more than one channel */
	UA_String endpointUrl;
	UA_String_copycstring("open62541.org",&endpointUrl);
	SL_ChannelManager_init(2,3600000, 873, 23, &endpointUrl);
	UA_SessionManager_init(2,300000,5);

	UA_ByteString slMessage = { -1, UA_NULL };

	int optval = 1;
	int sockfd, newsockfd, portno, clilen;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	/* First call to socket() function */
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	/* Initialize socket structure */
    memset((void *) &serv_addr, 0, sizeof(serv_addr));
	portno = PORT;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) == -1) {
		perror("setsockopt");
		exit(1);
	}

	/* Now bind the host address using bind() call.*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	/* Now start listening for the clients, here process will
	 * go in sleep mode and will wait for the incoming connection
	 */
	UA_TL_Connection tmpConnection;

	while (listen(sockfd, 5) != -1) {
		clilen = sizeof(cli_addr);
		/* Accept actual connection from the client */
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen);
		if (newsockfd < 0) {
			perror("ERROR on accept");
			exit(1);
		}

		UA_TL_ConnectionManager_getConnectionByHandle(newsockfd, &tmpConnection);
		if(tmpConnection == UA_NULL)
		{
			UA_TL_Connection_new(&connection, localBuffers, (TL_Writer)NL_TCP_writer,NL_Connection_close,newsockfd,UA_NULL);
		}
		UA_TL_Connection_getState(connection, &connectionState);

		printf("server_run - connection accepted: %i, state: %i\n", newsockfd, connectionState);

		UA_TL_Connection_bind(connection, newsockfd);
		//connection.connectionHandle = newsockfd;
		do {
            memset(buffer, 0, BUFFER_SIZE);
			n = read(newsockfd, buffer, BUFFER_SIZE);
			if (n > 0) {
                slMessage.data = (UA_Byte*) buffer;
				slMessage.length = n;
#ifdef DEBUG
				UA_ByteString_printx("server_run - received=",&slMessage);
#endif
				TL_Process(connection, &slMessage);
			} else if (n <= 0) {
				perror("ERROR reading from socket1");
		//		exit(1);
			}
			UA_TL_Connection_getState(connection, &connectionState);
		} while(connectionState != CONNECTIONSTATE_CLOSE);
		shutdown(newsockfd,2);
		close(newsockfd);
		UA_TL_ConnectionManager_getConnectionByHandle(newsockfd, &tmpConnection);
		UA_TL_ConnectionManager_removeConnection(tmpConnection);
	}
	shutdown(sockfd,2);
	close(sockfd);
}


#endif
