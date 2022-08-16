/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Adding Async Methods to Objects
 * -------------------------
 *
 * An object in an OPC UA information model may contain methods similar to
 * objects in a programming language. Methods are represented by a MethodNode.
 * Note that several objects may reference the same MethodNode. When an object
 * type is instantiated, a reference to the method is added instead of copying
 * the MethodNode. Therefore, the identifier of the context object is always
 * explicitly stated when a method is called.
 *
 * The method callback takes as input a custom data pointer attached to the
 * method node, the identifier of the object from which the method is called,
 * and two arrays for the input and output arguments. The input and output
 * arguments are all of type :ref:`variant`. Each variant may in turn contain a
 * (multi-dimensional) array or scalar of any data type.
 *
 * Constraints for the method arguments are defined in terms of data type, value
 * rank and array dimension (similar to variable definitions). The argument
 * definitions are stored in child VariableNodes of the MethodNode with the
 * respective BrowseNames ``(0, "InputArguments")`` and ``(0,
 * "OutputArguments")``.
 *
 * Example: Hello World Method
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 * The method takes a string scalar and returns a string scalar with "Hello "
 * prepended. The type and length of the input arguments is checked internally
 * by the SDK, so that we don't have to verify the arguments in the callback. */

#include <open62541/client_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#ifndef WIN32
#include <pthread.h>
#define THREAD_HANDLE pthread_t
#define THREAD_CREATE(handle, callback) pthread_create(&handle, NULL, callback, NULL)
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

static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

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
    while(running) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                    "Try to dequeue an async operation");
        const UA_AsyncOperationRequest* request = NULL;
        void *context = NULL;
        UA_AsyncOperationType type;
        if(UA_Server_getAsyncOperationNonBlocking(globalServer, &type, &request, &context, NULL) == true) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AsyncMethod_Testing: Got entry: OKAY");
            UA_CallMethodResult response = UA_Server_call(globalServer, &request->callMethodRequest);
            UA_Server_setAsyncOperationResult(globalServer, (UA_AsyncOperationResponse*)&response,
                                              context);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "AsyncMethod_Testing: Call done: OKAY");
            UA_CallMethodResult_clear(&response);
        } else {
            /* not a good style, but done for simplicity :-) */
            UA_sleep_ms(5000);
        }
    }
    return 0;
}

/* This callback will be called when a new entry is added to the Callrequest queue */
static void
TestCallback(UA_Server *server) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "Dispatched an async method");
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    globalServer = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(globalServer);
    UA_ServerConfig_setDefault(config);

    /* Set the NotifyCallback */
    config->asyncOperationNotifyCallback = TestCallback;

    /* Start the Worker-Thread */
    THREAD_HANDLE hThread;
    THREAD_CREATE(hThread, ThreadWorker);

    /* Add methods */
    addHelloWorldMethod1(globalServer);
	addHelloWorldMethod2(globalServer);

    UA_StatusCode retval = UA_Server_run(globalServer, &running);

    /* Shutdown the thread */
    THREAD_JOIN(hThread);

    UA_Server_delete(globalServer);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
