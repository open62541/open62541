/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include "open62541.h"


static
void valueWritten(UA_Client *client, void *userdata,
                    UA_UInt32 requestId, const void *response){
    printf("value written \n");
}
static
void valueRead(UA_Client *client, void *userdata,
                UA_UInt32 requestId, const void *response){

    printf("value Read \n");
}


static
void methodCalled(UA_Client *client, void *userdata,
                    UA_UInt32 requestId, const void *response) {
    printf("Method Called \n");
}

#define OPCUA_SERVER_URI "opc.tcp://localhost:4840"

int main(int argc, char *argv[]) {
    UA_ClientConfig clientConfig = UA_ClientConfig_default;
    clientConfig.localConnectionConfig.useBlockingSocket = UA_FALSE;
    UA_Client *client = UA_Client_new(clientConfig);
    UA_StatusCode retval;
    /* Connect to a server */
    /* anonymous connect would be: retval = UA_Client_connect(client, OPCUA_SERVER_URI); */
    retval = UA_Client_connect_username(client, OPCUA_SERVER_URI, "user1", "password");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }
    UA_String sValue;
    sValue.data = (UA_Byte*) malloc(90000);
    memset(sValue.data,'a',90000);
    sValue.length = 90000;

    printf("\nWriting a value of node (1, \"the.answer\"):\n");
    
    //Construct write request
    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = UA_WriteValue_new();
    wReq.nodesToWriteSize = 1;
    wReq.nodesToWrite[0].nodeId = UA_NODEID_NUMERIC(1, 51034);
    wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
    wReq.nodesToWrite[0].value.hasValue = true;
    wReq.nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_STRING];
    wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE; /* do not free the integer on deletion */
    wReq.nodesToWrite[0].value.value.data = &sValue;

    //Construct read request
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_NUMERIC(1, 51034);
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    // Construct call request
    UA_CallRequest callRequ;
    UA_CallRequest_init(&callRequ);

    UA_CallMethodRequest methodRequest;
    UA_CallMethodRequest_init(&methodRequest);
    methodRequest.objectId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    methodRequest.methodId = UA_NODEID_NUMERIC(1, 62541);
    UA_Variant input;
    UA_String argString = UA_STRING("input_Argument");
    UA_Variant_init(&input);
    UA_Variant_setScalarCopy(&input, &argString, &UA_TYPES[UA_TYPES_STRING]);
    methodRequest.inputArguments = &input;
    methodRequest.inputArgumentsSize = 1;

    callRequ.methodsToCall = &methodRequest;
    callRequ.methodsToCallSize = 1;


    UA_UInt32 reqId;
    UA_Int32 loopCount = 0;
    while(true){
        printf("--- Send Requests %d ---\n", ++loopCount);
        retval = UA_Client_AsyncService_write(client, wReq, valueWritten, NULL, &reqId);
        retval = UA_Client_AsyncService_read(client, rReq, valueRead, NULL, &reqId);
        retval = UA_Client_AsyncService_call(client, callRequ, methodCalled, NULL, &reqId);
		if(retval != UA_STATUSCODE_GOOD) {
			break;
		}
        printf("Requests Send \n");
        UA_Client_runAsync(client, 1500);
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) retval;
}
