/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Async Read Write of Variables
 * -------------------------
*/

#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include "open62541/server_config_default.h"
#include <open62541/server.h>

#include <signal.h>
#include <malloc.h>
#include "common.h"

#ifndef _WIN32
#include <pthread.h>
#define THREAD_HANDLE pthread_t
#define THREAD_CREATE(handle, callback) do {            \
        sigset_t set;                                   \
        sigemptyset(&set);                              \
        sigaddset(&set, SIGINT);                        \
        pthread_sigmask(SIG_BLOCK, &set, NULL);         \
        pthread_create(&handle, NULL, callback, &set);  \
    } while(0)
#define THREAD_JOIN(handle) pthread_join(handle, NULL)
#define THREAD_CALLBACK(name) static void * name(void *_)
#else
#include <windows.h>
#define THREAD_HANDLE HANDLE
#define THREAD_CREATE(handle, callback) { handle = CreateThread( NULL, 0, callback, NULL, 0, NULL); }
#define THREAD_JOIN(handle) WaitForSingleObject(handle, INFINITE)
#define THREAD_CALLBACK(name) static DWORD WINAPI name( LPVOID lpParam )
#endif

static UA_Server* globalServer;
static volatile UA_Boolean running = true;

static UA_StatusCode
helloWorldMethodCallback1(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_String *inputStr = (UA_String*)input->data;
    UA_String tmp = UA_STRING_ALLOC("Hello ");
    if(inputStr->length > 0) {
        tmp.data = (UA_Byte *)UA_realloc(tmp.data, tmp.length + inputStr->length);
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
	UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    char* test = (char*)calloc(1,tmp.length+1);
    memcpy(test, tmp.data, tmp.length);
    UA_String_clear(&tmp);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "'Hello World 1 (async)' was called and will take 3 seconds");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "    Data 1: %s", test);
    free(test);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "'Hello World 1 (async)' has ended");
    return UA_STATUSCODE_GOOD;
}

static void
addVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer async");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableNodeId;
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, &variableNodeId);
    UA_Server_setVariableNodeAsync(server, variableNodeId, UA_TRUE);

    //add non async variable for comparison
    attr.displayName = UA_LOCALIZEDTEXT("en-US","the answer non async");
    UA_Server_addVariableNode(server, UA_NODEID_NULL, parentNodeId,
                              parentReferenceNodeId, UA_QUALIFIEDNAME(0, "the answer 2"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), attr, NULL, NULL);
}

static void
addHelloWorldMethod1(UA_Server *server) {
    UA_Argument inputArgument;
    UA_Argument_init(&inputArgument);
    inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    inputArgument.name = UA_STRING("MyInput");
    inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_Argument outputArgument;
    UA_Argument_init(&outputArgument);
    outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
    outputArgument.name = UA_STRING("MyOutput");
    outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument.valueRank = UA_VALUERANK_SCALAR;

    UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
    helloAttr.description = UA_LOCALIZEDTEXT("en-US","Say `Hello World` async");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en-US","Hello World async");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &helloWorldMethodCallback1,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
	/* Get the method node */
	UA_NodeId id = UA_NODEID_NUMERIC(1, 62541);
	UA_Server_setMethodNodeAsync(server, id, UA_TRUE);
}

static UA_StatusCode
helloWorldMethodCallback2(UA_Server *server,
	const UA_NodeId *sessionId, void *sessionHandle,
	const UA_NodeId *methodId, void *methodContext,
	const UA_NodeId *objectId, void *objectContext,
	size_t inputSize, const UA_Variant *input,
	size_t outputSize, UA_Variant *output) {
	UA_String *inputStr = (UA_String*)input->data;
	UA_String tmp = UA_STRING_ALLOC("Hello ");
	if (inputStr->length > 0) {
		tmp.data = (UA_Byte *)UA_realloc(tmp.data, tmp.length + inputStr->length);
		memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
		tmp.length += inputStr->length;
	}
	UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    char* test = (char*)calloc(1, tmp.length + 1);
    memcpy(test, tmp.data, tmp.length);
	UA_String_clear(&tmp);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "'Hello World 2 (async)' was called and will take 1 seconds");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "    Data 2: %s", test);
    free(test);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "'Hello World 2 (async)' has ended");
	return UA_STATUSCODE_GOOD;
}

static void
addHelloWorldMethod2(UA_Server *server) {
	UA_Argument inputArgument;
	UA_Argument_init(&inputArgument);
	inputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
	inputArgument.name = UA_STRING("MyInput");
	inputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	inputArgument.valueRank = UA_VALUERANK_SCALAR;

	UA_Argument outputArgument;
	UA_Argument_init(&outputArgument);
	outputArgument.description = UA_LOCALIZEDTEXT("en-US", "A String");
	outputArgument.name = UA_STRING("MyOutput");
	outputArgument.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
	outputArgument.valueRank = UA_VALUERANK_SCALAR;

	UA_MethodAttributes helloAttr = UA_MethodAttributes_default;
	helloAttr.description = UA_LOCALIZEDTEXT("en-US", "Say `Hello World` sync");
	helloAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Hello World sync");
	helloAttr.executable = true;
	helloAttr.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1, 62542),
		UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
		UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
		UA_QUALIFIEDNAME(1, "hello world 2"),
		helloAttr, &helloWorldMethodCallback2,
		1, &inputArgument, 1, &outputArgument, NULL, NULL);
	/* Get the method node */
	UA_NodeId id = UA_NODEID_NUMERIC(1, 62542);
	UA_Server_setMethodNodeAsync(server, id, UA_TRUE);
}

THREAD_CALLBACK(ThreadWorker) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Started Async worker Thread. Try do dequeue operations.");
    while(running) {
        const UA_AsyncOperationRequest* request = NULL;
        void *context = NULL;
        UA_AsyncOperationType type; UA_NodeId sessionId;
        size_t opIndex;
        if(UA_Server_getAsyncOperationNonBlocking(globalServer, &type, &request, &context, NULL, &sessionId, &opIndex) == true) {
            switch(type) {
                case UA_ASYNCOPERATIONTYPE_CALL:
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AsyncMethod_Testing: Got entry: OKAY");
                    UA_CallMethodResult response = UA_Server_callWithSession(globalServer, &request->callMethodRequest, &sessionId);
                    UA_Server_setAsyncOperationResult(globalServer, (UA_AsyncOperationResponse*)&response,
                                                      context);
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AsyncMethod_Testing: Call done: OKAY");
                    UA_CallMethodResult_clear(&response);
                    break;
                case UA_ASYNCOPERATIONTYPE_READ:
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AsyncRead_Testing: Got entry: OKAY");
                    UA_DataValue readResponse;
                    readResponse = UA_Server_readWithSession(globalServer, &sessionId, &request->readRequest.nodesToRead[opIndex], request->readRequest.timestampsToReturn);
                    UA_Server_setAsyncOperationResult(globalServer, (UA_AsyncOperationResponse*) &readResponse, context);
                    UA_DataValue_clear(&readResponse);
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AsyncRead_Testing: Read done: OKAY");
                    break;
                case UA_ASYNCOPERATIONTYPE_WRITE:
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AsyncWrite_Testing: Got entry: OKAY");
                    UA_StatusCode result;
                    result = UA_Server_writeWithSession(globalServer, &request->writeValue, &sessionId);
                    UA_Server_setAsyncOperationResult(globalServer, (UA_AsyncOperationResponse*) &result, context);
                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AsyncWrite_Testing: Write done: OKAY");
                    break;
                default:
                    UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Invalid or unknown AsyncOperationType");
            }
        } else {
            /* not a good style, but done for simplicity :-) */
            sleep_ms(1000);
        }
    }
    return 0;
}

/* This callback will be called when a new entry is added to the Callrequest queue */
static void
TestCallback(UA_Server *server) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Dispatched an async operation");
}

int main(void) {
    globalServer = UA_Server_new();

    /* Set the NotifyCallback */
    UA_ServerConfig *config = UA_Server_getConfig(globalServer);
    UA_ServerConfig_setMinimal(config, 4990, NULL);
    config->asyncOperationNotifyCallback = TestCallback;

    /* Start the Worker-Thread */
    THREAD_HANDLE hThread;
    THREAD_CREATE(hThread, ThreadWorker);

    /* Add methods */
    addHelloWorldMethod1(globalServer);
	addHelloWorldMethod2(globalServer);
    addVariable(globalServer);

    UA_StatusCode retval = UA_Server_runUntilInterrupt(globalServer);

    /* Shutdown the thread */
    running = false;
    THREAD_JOIN(hThread);

    UA_Server_delete(globalServer);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
