/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include "open62541.h"
#include <unistd.h>

/*total number of unresponded requests*/
static int reqNo = 0;

static
void valueWritten(UA_Client *client, void *userdata, UA_UInt32 requestId,
        void *response) {
    reqNo--;
    printf("%-50s%-8i\n", "writeResponse received, pending requests:", reqNo);
}

static
void valueRead(UA_Client *client, void *userdata, UA_UInt32 requestId,
        void *response) {
    reqNo--;
    printf("%-50s%-8i\n", "readResponse received, pending requests:", reqNo);
}

static
void attrRead(UA_Client *client, void *userdata, UA_UInt32 requestId,
        void *response) {
    reqNo--;
    UA_DataValue *res = ((UA_ReadResponse*) response)->results;
    if(res->hasValue){
        memcpy(userdata, &res->value, sizeof(UA_Variant));
        }
    UA_Variant val = *(UA_Variant*) userdata;
    if (UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime*) val.data;
        UA_String string_date = UA_DateTime_toString(raw_date);
        printf("string date is: %.*s\n", (int) string_date.length,
                string_date.data);
        UA_String_deleteMembers(&string_date);
    }
    printf("%-50s%-8i\n", "attribute read, pending requests:", reqNo);
}

//static
//void dataTypeRead(UA_Client *client, void *userdata, UA_UInt32 requestId,
//        void *response) {
//
//}


static
void fileBrowsed(UA_Client *client, void *userdata, UA_UInt32 requestId,
        void *response) {
    reqNo--;
    printf("%-50s%-8i\n", "browseResponse received, pending requests:", reqNo);
}


int main(int argc, char *argv[]) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_default);


    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
//     retval = UA_Client_getEndpoints(client, "opc.tcp://localhost:4840",
//                                                      &endpointArraySize, &endpointArray);

    if (retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize,
                &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return (int) retval;
    }
    printf("%i endpoints found\n", (int) endpointArraySize);
    for (size_t i = 0; i < endpointArraySize; i++) {
        printf("URL of endpoint %i is %.*s\n", (int) i,
                (int) endpointArray[i].endpointUrl.length,
                endpointArray[i].endpointUrl.data);
    }
    UA_Array_delete(endpointArray, endpointArraySize,
            &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    if (retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int) retval;
    }
    UA_String sValue;
    sValue.data = (UA_Byte *) malloc(90000);
    memset(sValue.data, 'a', 90000);
    sValue.length = 90000;

    //printf("\nWriting a value of node (1, \"the.answer\"):\n");
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

    /* Browse some objects */
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(1, UA_NS0ID_OBJECTSFOLDER); /* browse objects folder */
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */
    size_t reqId = 0;
    retval = UA_Client_connect(client, "opc.tcp://localhost:4840");

    printf("Testing async requests\n");
    printf("%s\n","---------------------");
    do {
        if (UA_Client_getState(client) == UA_CLIENTSTATE_SESSION) {
            /*reset the number of total requests sent*/

            UA_Client_addAsyncRequest(client, (void*) &bReq,
                    &UA_TYPES[UA_TYPES_BROWSEREQUEST], fileBrowsed,
                    &UA_TYPES[UA_TYPES_BROWSERESPONSE], NULL, &reqId);
            reqNo++;
            printf("%-50s%-8i\n", "browseRequest sent, pending requests:", reqNo);

            UA_Client_addAsyncRequest(client, (void*) &wReq,
                    &UA_TYPES[UA_TYPES_WRITEREQUEST], valueWritten,
                    &UA_TYPES[UA_TYPES_WRITERESPONSE], NULL, &reqId);
            reqNo++;
            printf("%-50s%-8i\n", "readRequest sent, pending requests:", reqNo);

            UA_Client_addAsyncRequest(client, (void*) &rReq,
                    &UA_TYPES[UA_TYPES_READREQUEST], valueRead,
                    &UA_TYPES[UA_TYPES_READRESPONSE], NULL, &reqId);
            reqNo++;
            printf("%-50s%-8i\n", "writeRequest sent, pending requests:", reqNo);
        }

        UA_Client_run_iterate(client, 10);
        if (reqNo < 0)
            break;
    } while (reqId < 10);

    //the first highlevel function made async, UA_Client_run_iterate capsuled.
    printf("Testing async highlevel functions:\n");
    printf("%s\n","---------------------");
    UA_Variant val;
    UA_Variant_init(&val);
    for (int i = 0; i < 5; i++) {
        UA_Client_readValueAttribute_async(client,
                UA_NODEID_STRING(1, "the.answer"),
                NULL, attrRead, &val, &reqId);
        reqNo++;

//        UA_Int32 value = 0;
//        UA_Variant *myVariant = UA_Variant_new();
//            UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
//            UA_Client_writeValueAttribute(client, UA_NODEID_STRING(1, "the.answer"), myVariant);
//        UA_Variant_delete(myVariant);

    }
    UA_Client_run_iterate(client, 10);
    UA_Client_run_iterate(client, 10);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}
