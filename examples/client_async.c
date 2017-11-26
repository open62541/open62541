/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <stdio.h>
#include "open62541.h"
#include <unistd.h>

/*total number of unprocessed requests*/
static int reqNo = 0;

static void valueWritten(UA_Client *client, void *userdata, UA_UInt32 requestId,
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
void fileBrowsed(UA_Client *client, void *userdata, UA_UInt32 requestId,
		void *response) {
	reqNo--;
	printf("%-50s%-8i\n", "browseResponse received, pending requests:", reqNo);
}

static
void attrRead(UA_Client *client, void *userdata, UA_UInt32 requestId,
		void *response) {
	reqNo--;
	printf("%-50s%-8i\n", "attribute read, pending requests:", reqNo);
	if (response == NULL) {
		return;
	}
	UA_DataValue *res = ((UA_ReadResponse*) response)->results;
	if (res->hasValue) {
		memcpy(userdata, &res->value, sizeof(UA_Variant));
	}

	/*The following part shall be provided to the client*/
	UA_Variant val = *(UA_Variant*) userdata;
	if (UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_DATETIME])) {
		UA_DateTime raw_date = *(UA_DateTime*) val.data;
		UA_String string_date = UA_DateTime_toString(raw_date);
		printf("string date is: %.*s\n", (int) string_date.length,
				string_date.data);
		UA_String_deleteMembers(&string_date);
	}

	if (UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_INT32])) {
		UA_Int32 int_val = *(UA_Int32*) val.data;
		printf("%-50s%-8i\n", "Reading the value of node (1, \"the.answer\"):",
				int_val);

	}

	/*more case distinctions...*/
	UA_ReadResponse_deleteMembers((UA_ReadResponse*) response);
}

static
void attrWritten(UA_Client *client, void *userdata, UA_UInt32 requestId,
		void *response) {
	reqNo--;
	printf("%-50s%-8i\n", "attribute written, pending requests:", reqNo);
}
//static void testEndpoints(UA_Client *client, void *userdata,
//		UA_UInt32 requestId, void *response) {
//	reqNo--;
//	printf("%-50s\n", "testing getendpoints...");
//	UA_GetEndpointsResponse* resp = (UA_GetEndpointsResponse*) response;
//	size_t endpointArraySize = resp->endpointsSize;
//	UA_EndpointDescription *endpointArray = resp->endpoints;
//	printf("%i endpoints found\n", (int) endpointArraySize);
//	for (size_t i = 0; i < endpointArraySize; i++) {
//		printf("URL of endpoint %i is %.*s\n", (int) i,
//				(int) endpointArray[i].endpointUrl.length,
//				endpointArray[i].endpointUrl.data);
//	}
//}
static void methodCalled(UA_Client *client, void *userdata, UA_UInt32 requestId,
		void *callResponse) {
	reqNo--;
	printf("%-50s%-8i\n", "method called, pending requests:", reqNo);
	size_t outputSize;
	UA_Variant *output;
	UA_CallResponse response = *(UA_CallResponse *) callResponse;
	UA_StatusCode retval = response.responseHeader.serviceResult;
	if (retval == UA_STATUSCODE_GOOD) {
		if (response.resultsSize == 1)
			retval = response.results[0].statusCode;
		else
			retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
	}
	if (retval != UA_STATUSCODE_GOOD) {
		UA_CallResponse_deleteMembers(&response);
	}

	/* Move the output arguments */
	output = response.results[0].outputArguments;
	outputSize = response.results[0].outputArgumentsSize;
	response.results[0].outputArguments = NULL;
	response.results[0].outputArgumentsSize = 0;

	//output
	if (retval == UA_STATUSCODE_GOOD) {
		printf("Method call was successfull with %lu returned values.\n",
				(unsigned long) outputSize);
		UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
	} else {
		printf("Method call was unsuccessfull with %x returned values.\n",
				retval);
	}
}

static void translateCalled(UA_Client *client, void *userdata,
		UA_UInt32 requestId, void *callResponse) {
	reqNo--;
		printf("%-50s%-8i\n", "translation called, pending requests:", reqNo);
	UA_TranslateBrowsePathsToNodeIdsResponse response =
			*(UA_TranslateBrowsePathsToNodeIdsResponse *) callResponse;
	if (response.results[0].targetsSize == 1) {
		return;
	}
	// UA_BrowsePath_deleteMembers(&browsePath);
//	UA_TranslateBrowsePathsToNodeIdsResponse_deleteMembers(&response);
}

int main(int argc, char *argv[]) {
	UA_Client *client = UA_Client_new(UA_ClientConfig_default);
	UA_UInt32 reqId = 0;

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

	UA_Client_connect(client, "opc.tcp://localhost:4840");

	printf("Testing async requests\n");
	printf("%s\n", "---------------------");
	do {
		if (UA_Client_getState(client) == UA_CLIENTSTATE_SESSION) {
			/*reset the number of total requests sent*/

			UA_Client_addAsyncRequest(client, (void*) &bReq,
					&UA_TYPES[UA_TYPES_BROWSEREQUEST], fileBrowsed,
					&UA_TYPES[UA_TYPES_BROWSERESPONSE], NULL, &reqId);
			reqNo++;
			printf("%-50s%-8i\n", "browseRequest sent, pending requests:",
					reqNo);

			UA_Client_addAsyncRequest(client, (void*) &wReq,
					&UA_TYPES[UA_TYPES_WRITEREQUEST], valueWritten,
					&UA_TYPES[UA_TYPES_WRITERESPONSE], NULL, &reqId);
			reqNo++;
			printf("%-50s%-8i\n", "readRequest sent, pending requests:", reqNo);

			UA_Client_addAsyncRequest(client, (void*) &rReq,
					&UA_TYPES[UA_TYPES_READREQUEST], valueRead,
					&UA_TYPES[UA_TYPES_READRESPONSE], NULL, &reqId);
			reqNo++;
			printf("%-50s%-8i\n", "writeRequest sent, pending requests:",
					reqNo);
		}

		UA_Client_run_iterate(client, 10);
	} while (reqId < 10);

	//the first highlevel function made async, UA_Client_run_iterate capsuled.
	printf("\n");
	printf("Testing async highlevel functions:\n");
	printf("%s\n", "---------------------");
	UA_Variant vals[5];

	UA_Int32 value = 0;
	UA_Variant *myVariant = UA_Variant_new();

	for (UA_UInt16 i = 0; i < 5; i++) {
		UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
		value++;
		/*For write functions NULL is passed as userdata, since no response is needed*/
		UA_Client_writeValueAttribute_async(client,
				UA_NODEID_STRING(1, "the.answer"), myVariant, attrWritten, NULL,
				&reqId);
		reqNo++;

		/*(for the moment) for the callback to work the fourth argument has to be of type UA_Variant*/
		UA_Client_readValueAttribute_async(client,
				UA_NODEID_STRING(1, "the.answer"), &vals[i], attrRead, &vals[i],
				&reqId);
		reqNo++;

		UA_Client_readValueAttribute_async(client,
				UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME),
				&vals[i], attrRead, &vals[i], &reqId);
		reqNo++;

		UA_String stringValue = UA_String_fromChars("World");
		UA_Variant input;
		UA_Variant_init(&input);
		UA_Variant_setScalar(&input, &stringValue, &UA_TYPES[UA_TYPES_STRING]);

		UA_Client_call_async(client,
				UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
				UA_NODEID_NUMERIC(1, 62541), 1, &input, methodCalled, NULL,
				&reqId);
		reqNo++;

#define pathSize 3
		char *paths[pathSize] = { "Server", "ServerStatus", "State" };
		UA_UInt32 ids[pathSize] = { UA_NS0ID_ORGANIZES,
		UA_NS0ID_HASCOMPONENT, UA_NS0ID_HASCOMPONENT };

		UA_Cient_translateBrowsePathsToNodeIds_async(client, paths, ids,
		pathSize, translateCalled, NULL, &reqId);
		reqNo++;

//		UA_Client_getEndpoints_async(client, testEndpoints, NULL, &reqId);
//		reqNo++;
	}

	while (reqNo > 0)
		UA_Client_run_iterate(client, 10);

	UA_Variant_delete(myVariant);
	UA_Client_disconnect(client);
	UA_Client_delete(client);
	return (int) UA_STATUSCODE_GOOD;
}
