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
#include <open62541/server.h>
#include <open62541/plugin/log.h>

static void
asyncCall(UA_Server *server, void *data) {
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_USERLAND, "call");

    UA_Variant *out = (UA_Variant*)data;
    UA_Server_setAsyncCallMethodResult(server, UA_STATUSCODE_GOOD, out);
}

static UA_StatusCode
helloWorldMethodCallback1(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output) {
    UA_LOG_INFO(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_USERLAND, "async");

    /* Prepare the output */
    UA_String *inputStr = (UA_String*)input->data;
    UA_String out = UA_STRING_NULL;
    UA_String_format(&out, "Hello %S", *inputStr);
    UA_Variant_setScalarCopy(output, &out, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&out);

    /* Return the output with a five second delay */
    UA_DateTime callTime = UA_DateTime_nowMonotonic() + (2 * UA_DATETIME_SEC);
    UA_Server_addTimedCallback(server, asyncCall, output, callTime, NULL);
    return UA_STATUSCODE_GOODCOMPLETESASYNCHRONOUSLY;
}

int main(void) {
    UA_Server *server = UA_Server_new();

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
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &helloWorldMethodCallback1,
                            1, &inputArgument, 1, &outputArgument, NULL, NULL);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
