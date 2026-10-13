/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Adding Async Methods to Objects
 * -------------------------------
 *
 * This Hello World method returns immediately with
 * ``UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY``. A timed callback submits the
 * output two seconds later with ``UA_Server_setAsyncCallMethodResult``. It runs
 * on the server's event loop, so no worker thread or external mutex is needed.
 *
 * Cancellation ends the requester's wait but leaves the timed callback's output
 * valid. This example lets the callback finish normally, including during
 * shutdown. Its single result-setter call returns ownership to the server; a
 * result submitted after cancellation is discarded.
 *
 * A worker thread can use the same ownership pattern with
 * ``UA_MULTITHREADING >= 100``. Shared queues and cancellation flags still need
 * their own synchronization. See :ref:`async-operations` for the API contract. */

#include <open62541/client_config_default.h>
#include <open62541/server.h>
#include <open62541/plugin/log.h>

static void
asyncOperationCancelCallback(UA_Server *server, const void *out) {
    /* Do not touch the worker's output or acknowledge on its behalf. The timed
     * callback will finish normally and acknowledge, even during shutdown. */
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_APPLICATION,
                "Async operation %p canceled; waiting for the worker", out);
}

static void
asyncCall(UA_Server *server, void *data) {
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_APPLICATION, "call");

    /* Return ownership exactly once. Do not access output after this call. */
    UA_Server_setAsyncCallMethodResult(server, (UA_Variant*)data, UA_STATUSCODE_GOOD);
}

static UA_StatusCode
helloWorldMethodCallback1(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_APPLICATION, "async");

    /* Prepare the output */
    UA_String *inputStr = (UA_String*)input->data;
    UA_String out = UA_STRING_NULL;
    UA_StatusCode res = UA_String_format(&out, "Hello %S", *inputStr);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_Variant_setScalarCopy(output, &out, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&out);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Return the output with a two second delay */
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;
    UA_DateTime callTime = el->dateTime_nowMonotonic(el) + (2 * UA_DATETIME_SEC);
    res = UA_Server_addTimedCallback(server, asyncCall, output, callTime, NULL);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Signal async processing to the server. Will be completed later. */
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

static UA_StatusCode
addHelloWorldMethod(UA_Server *server) {
    /* Set the cancel callback */
    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->asyncOperationCancelCallback = asyncOperationCancelCallback;

    /* Add method */
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
    return UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &helloWorldMethodCallback1,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);
}

int main(void) {
    UA_Server *server = UA_Server_new();
    if(!server)
        return EXIT_FAILURE;

    UA_StatusCode res = addHelloWorldMethod(server);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Server_delete(server);
        return EXIT_FAILURE;
    }

    /* Run the server */
    UA_Server_runUntilInterrupt(server);

    /* Clean up */
    UA_Server_delete(server);

    return 0;
}
