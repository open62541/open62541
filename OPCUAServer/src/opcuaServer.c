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


//#include "opcua_binaryEncDec.h"
#include "opcua_builtInDatatypes.h"
#include "opcua_transportLayer.h"
#include "opcua_types.h"

#include <sys/socket.h>
#include <netinet/in.h>


int main(void)
{
	puts("OPC ua Stack");
	//struct BED_ApplicationDescription nStruct;
	//UA_String s;
	puts("running tests...");
	TL_getMessageHeader_test();
	//TL_getHELMessage_test();
	puts("...done");

	server_init();
	server_run();

	return EXIT_SUCCESS;

}

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

	while(1)
	{
		//call recv (nonblocking)

		//call TL_getPacketType

		//if newData
		//
		UA_connection connection;
		AD_RawMessage *rawMessage;
		switch(server_state)
		{

			recv_data :
			{
				//call receive function

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
/*				else
				{
					SL_open(connection,rawMessage);

				}
*/
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

