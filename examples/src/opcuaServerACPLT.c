#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "opcua.h"
#include "ua_transport.h"
#include "ua_transport_binary.h"
#include "networklayer.h"

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
	// given
/*	UA_Variant *value = UA_NULL;
	UA_Variant *copiedValue = UA_NULL;
	UA_Int32 *dimensions;
	UA_Int32 theta, zeta;
	UA_String **srcArray; UA_Array_new((void***)&srcArray,3,UA_STRING);
	//init
	UA_String_copycstring("open",srcArray[0]);
	UA_String_copycstring("62541",srcArray[1]);
	UA_String_copycstring("opc ua",srcArray[2]);

	UA_alloc((void**)&dimensions,UA_Int32_calcSize(UA_NULL));
	dimensions[0]=1;
	UA_Variant_new(&value);
	UA_Variant_new(&copiedValue);

	value->arrayLength = 3;
	value->data = (void**)srcArray;
	value->arrayDimensionsLength = 1;
	value->arrayDimensions = &dimensions;
	value->encodingMask |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
	value->encodingMask |= UA_STRING_NS0;
	value->encodingMask |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;

	UA_Variant_copy(value,copiedValue);
	theta = *(value->arrayDimensions)[0],
	zeta =  *(copiedValue->arrayDimensions)[0];

	theta = zeta * theta;
*/
	// given
	UA_Variant *value = UA_NULL;
	UA_Variant *copiedValue = UA_NULL;
	UA_Int32 **dimensions;
	UA_Int32 i,i1,i2;
	UA_Int32 **srcArray;
	UA_Array_new((void***)&srcArray,6,UA_INT32);


	*srcArray[0] = 0;
	*srcArray[1] = 1;
	*srcArray[2] = 2;
	*srcArray[3] = 3;
	*srcArray[4] = 4;
	*srcArray[5] = 5;



	UA_Array_new((void***)&dimensions,2,UA_INT32);

	*(dimensions)[0] = 3;
	*(dimensions)[1] = 2;

	UA_Variant_new(&value);
	UA_Variant_new(&copiedValue);

	value->arrayLength = 6;
	value->data = (void**)srcArray;
	value->arrayDimensionsLength = 2;
	value->arrayDimensions = dimensions;
	value->encodingMask |= UA_VARIANT_ENCODINGMASKTYPE_ARRAY;
	value->encodingMask |= UA_INT32_NS0;
	value->encodingMask |= UA_VARIANT_ENCODINGMASKTYPE_DIMENSIONS;

	//when
	UA_Variant_copy(value, copiedValue);

	//then
	//1st dimension
	i1 = *(value->arrayDimensions)[0],
	i2 = *(copiedValue->arrayDimensions)[0];
	//	ck_assert_int_eq(i1,i2);

	//2nd dimension
	i1 = *(value->arrayDimensions)[1];
	i2 = *(copiedValue->arrayDimensions)[1];
	//ck_assert_int_eq(i1,i2);

	for(i=0;i<6;i++){
		i1 = *((UA_Int32*)(value->data[i]));
		i2 = *((UA_Int32*)(copiedValue->data[i]));
	//	ck_assert_int_eq(i1,i2);
	}

}
void server_run() {
	//just for debugging
#ifdef DEBUG
	tmpTestFunction();
#endif
	TL_Connection connection;
	connection.connectionState = CONNECTIONSTATE_CLOSED;
	connection.writerCallback = (TL_Writer) NL_TCP_writer;
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
	while (listen(sockfd, 5) != -1) {
		clilen = sizeof(cli_addr);
		/* Accept actual connection from the client */
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen);
		if (newsockfd < 0) {
			perror("ERROR on accept");
			exit(1);
		}

		printf("server_run - connection accepted: %i, state: %i\n", newsockfd, connection.connectionState);
		connection.connectionHandle = newsockfd;
		do {
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
		} while(connection.connectionState != CONNECTIONSTATE_CLOSE);
		shutdown(newsockfd,2);
		close(newsockfd);
		connection.connectionState = CONNECTIONSTATE_CLOSED;
	}
	shutdown(sockfd,2);
	close(sockfd);
}

#endif
