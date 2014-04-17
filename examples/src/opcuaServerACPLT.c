#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "opcua.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"

#ifdef LINUX

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socketvar.h>
#include <unistd.h>

void server_init();
void server_run();

#endif

#define PORT 16664
#define MAXMSG 512
#define BUFFER_SIZE 8192

int main(void) {

#ifdef LINUX
    server_init();
    server_run();
#endif

return EXIT_SUCCESS;

}

#ifdef LINUX

void server_init() {
	printf("Starting open62541 demo server on port %d\n", PORT);
}

typedef struct T_Server {
	int newDataToWrite;
	UA_ByteString writeData;
} Server;
Server server;

UA_Int32 server_writer(TL_Connection* connection, UA_ByteString** gather_buf, UA_UInt32 gather_len) {
	UA_UInt32 total_len = 0;
	for(UA_UInt32 i=0;i<gather_len;i++) {
		total_len += gather_buf[i]->length;
	}
    UA_ByteString msg;
	UA_ByteString_newMembers(&msg, total_len);

	UA_UInt32 pos = 0;
	for(UA_UInt32 i=0;i<gather_len;i++) {
		memcpy(msg.data+pos, gather_buf[i]->data, gather_buf[i]->length);
		pos += gather_buf[i]->length;
	}
	server.writeData.data = msg.data;
	server.writeData.length = msg.length;
	server.newDataToWrite = 1;
	return UA_SUCCESS;
}

void server_run() {
	TL_Connection connection;
	connection.connectionState = CONNECTIONSTATE_CLOSED;
	connection.writerCallback = (TL_Writer)server_writer;
	connection.localConf.maxChunkCount = 1;
	connection.localConf.maxMessageSize = BUFFER_SIZE;
	connection.localConf.protocolVersion = 0;
	connection.localConf.recvBufferSize = BUFFER_SIZE;
	connection.localConf.recvBufferSize = BUFFER_SIZE;

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

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval)
			== -1) {
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
	while (listen(sockfd, 5) != -1) {
		clilen = sizeof(cli_addr);
		/* Accept actual connection from the client */
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen);
		if (newsockfd < 0) {
			perror("ERROR on accept");
			exit(1);
		}

		printf("connection accepted: %i, state: %i\n", newsockfd, connection.connectionState);
		/* communication loop */
		int i = 0;
		do {
			/* If connection is established then start communicating */
            memset(buffer, 0, BUFFER_SIZE);

			n = read(newsockfd, buffer, BUFFER_SIZE);
			if (n > 0) {
                slMessage.data = (UA_Byte*) buffer;
				slMessage.length = n;
				UA_ByteString_printx("server_run - received=",&slMessage);
				TL_Process(&connection, &slMessage);
			} else if (n <= 0) {
				perror("ERROR reading from socket1");
				exit(1);
			}
			if (server.newDataToWrite) {
				UA_ByteString_printx("Send data:", &server.writeData);
				n = write(newsockfd, server.writeData.data, server.writeData.length);
				printf("written %d bytes \n", n);
				server.newDataToWrite = 0;
				UA_ByteString_deleteMembers(&server.writeData);
			}
			i++;
		} while(connection.connectionState != CONNECTIONSTATE_CLOSE);
		shutdown(newsockfd,2);
		close(newsockfd);
		connection.connectionState = CONNECTIONSTATE_CLOSED;
	}
	shutdown(sockfd,2);
	close(sockfd);
}

#endif
