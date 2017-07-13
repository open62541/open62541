/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include "open62541.h"
#include <unistd.h>
static void
handler_TheAnswerChanged(UA_UInt32 monId, UA_DataValue *value, void *context) {
    printf("The Answer has changed!\n");
}
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
static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    if(isInverse)
        return UA_STATUSCODE_GOOD;
    UA_NodeId *parent = (UA_NodeId *)handle;
    printf("%d, %d --- %d ---> NodeId %d, %d\n",
           parent->namespaceIndex, parent->identifier.numeric,
           referenceTypeId.identifier.numeric, childId.namespaceIndex,
           childId.identifier.numeric);
    return UA_STATUSCODE_GOOD;
}



int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    //UA_StatusCode retval;

    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:16664",
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return (int)retval;
    }
    printf("%i endpoints found\n", (int)endpointArraySize);
    for(size_t i=0;i<endpointArraySize;i++){
        printf("URL of endpoint %i is %.*s\n", (int)i,
               (int)endpointArray[i].endpointUrl.length,
               endpointArray[i].endpointUrl.data);
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    /* Connect to a server */
    /* anonymous connect would be: retval = UA_Client_connect(client, "opc.tcp://localhost:16664"); */
    //retval = UA_Client_connect_username(client, "opc.tcp://localhost:16664", "user1", "password");

    UA_Boolean waiting = false;
    UA_Boolean connected = false;
    while (!connected){
    	retval = UA_Client_connect(client, "opc.tcp://localhost:16664", &waiting, &connected);
    	if (waiting) //do something else
    		sleep(1);
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }
    UA_String sValue;
    sValue.data = malloc(90000);
    memset(sValue.data,'a',90000);
    sValue.length = 90000;

    printf("\nWriting a value of node (1, \"the.answer\"):\n");
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

    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead = UA_ReadValueId_new();
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_NUMERIC(1, 51034);
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    size_t reqId;
    while(true){

        UA_Client_addAsyncRequest(client,(void*)&wReq,&UA_TYPES[UA_TYPES_WRITEREQUEST],valueWritten,&UA_TYPES[UA_TYPES_WRITERESPONSE],NULL,&reqId);
        UA_Client_addAsyncRequest(client,(void*)&rReq,&UA_TYPES[UA_TYPES_READREQUEST],valueRead,&UA_TYPES[UA_TYPES_READRESPONSE],NULL,&reqId);

        UA_Client_run_iterate(client,10);
        sleep(2);
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}
