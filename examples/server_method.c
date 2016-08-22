/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <signal.h>
#include <stdlib.h>

#ifdef UA_NO_AMALGAMATION
#include "ua_types.h"
#include "ua_server.h"
#include "ua_config_standard.h"
#include "ua_network_tcp.h"
#include "ua_log_stdout.h"
#else
#include "open62541.h"
#endif

UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;


/* Example 1 */
static UA_StatusCode
helloWorldMethod(void *handle, const UA_NodeId objectId,
                 size_t inputSize, const UA_Variant *input,
                 size_t outputSize, UA_Variant *output) {
    UA_String *inputStr = (UA_String*)input->data;
    UA_String tmp = UA_STRING_ALLOC("Hello ");
    if(inputStr->length > 0) {
        tmp.data = realloc(tmp.data, tmp.length + inputStr->length);
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_deleteMembers(&tmp);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "Hello World was called");
    return UA_STATUSCODE_GOOD;
}

/* Example 2 */
static UA_StatusCode
IncInt32ArrayValuesMethod(void *handle, const UA_NodeId objectId,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_Variant_setArrayCopy(output, input->data, 5, &UA_TYPES[UA_TYPES_INT32]);
    for(size_t i = 0; i< input->arrayLength; i++)
        ((UA_Int32*)output->data)[i] = ((UA_Int32*)input->data)[i] + 1;
    return UA_STATUSCODE_GOOD;
}


/* Example 3 */
static UA_StatusCode
fooBarMethod(void *handle, const UA_NodeId objectId,
             size_t inputSize, const UA_Variant *input,
             size_t outputSize, UA_Variant *output) {
    /* the same as helloWorld, but returns foobar */
    UA_String *inputStr = (UA_String*)input->data;
    UA_String tmp = UA_STRING_ALLOC("FooBar! ");
    if(inputStr->length > 0) {
        tmp.data = realloc(tmp.data, tmp.length + inputStr->length);
        memcpy(&tmp.data[tmp.length], inputStr->data, inputStr->length);
        tmp.length += inputStr->length;
    }
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_deleteMembers(&tmp);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "FooBar was called");
    return UA_STATUSCODE_GOOD;
}

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    /* initialize the server */
    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl;
    nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    /* Example 1 */
    /* add the method node with the callback */
    UA_Argument inputArguments1;
    UA_Argument_init(&inputArguments1);
    inputArguments1.arrayDimensionsSize = 0;
    inputArguments1.arrayDimensions = NULL;
    inputArguments1.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments1.description = UA_LOCALIZEDTEXT("en_US", "A String");
    inputArguments1.name = UA_STRING("MyInput");
    inputArguments1.valueRank = -1;

    UA_Argument outputArguments1;
    UA_Argument_init(&outputArguments1);
    outputArguments1.arrayDimensionsSize = 0;
    outputArguments1.arrayDimensions = NULL;
    outputArguments1.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArguments1.description = UA_LOCALIZEDTEXT("en_US", "A String");
    outputArguments1.name = UA_STRING("MyOutput");
    outputArguments1.valueRank = -1;

    UA_MethodAttributes helloAttr;
    UA_MethodAttributes_init(&helloAttr);
    helloAttr.description = UA_LOCALIZEDTEXT("en_US","Say `Hello World`");
    helloAttr.displayName = UA_LOCALIZEDTEXT("en_US","Hello World");
    helloAttr.executable = true;
    helloAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_NUMERIC(1,62541),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, "hello world"),
                            helloAttr, &helloWorldMethod, NULL,
                            1, &inputArguments1, 1, &outputArguments1, NULL);

    /* Example 2 */
    /* add another method node: output argument as 1d Int32 array*/
    UA_Argument inputArguments2;
    UA_Argument_init(&inputArguments2);
    inputArguments2.arrayDimensionsSize = 1;
    UA_UInt32 * pInputDimensions = UA_UInt32_new();
    pInputDimensions[0] = 5;
    inputArguments2.arrayDimensions = pInputDimensions;
    inputArguments2.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArguments2.description = UA_LOCALIZEDTEXT("en_US",
                    "input an array with 5 elements, type int32");
    inputArguments2.name = UA_STRING("int32 value");
    inputArguments2.valueRank = 1;

    UA_Argument outputArguments2;
    UA_Argument_init(&outputArguments2);
    outputArguments2.arrayDimensionsSize = 1;
    UA_UInt32 * pOutputDimensions = UA_UInt32_new();
    pOutputDimensions[0] = 5;
    outputArguments2.arrayDimensions = pOutputDimensions;
    outputArguments2.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArguments2.description = UA_LOCALIZEDTEXT("en_US",
                                                    "increment each array index");
    outputArguments2.name = UA_STRING("output is the array, "
                                      "each index is incremented by one");
    outputArguments2.valueRank = 1;

    UA_MethodAttributes incAttr;
    UA_MethodAttributes_init(&incAttr);
    incAttr.description = UA_LOCALIZEDTEXT("en_US", "1dArrayExample");
    incAttr.displayName = UA_LOCALIZEDTEXT("en_US", "1dArrayExample");
    incAttr.executable = true;
    incAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "IncInt32ArrayValues"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "IncInt32ArrayValues"),
                            incAttr, &IncInt32ArrayValuesMethod, NULL,
                            1, &inputArguments2, 1, &outputArguments2, NULL);

    /* Example 3 */
    UA_MethodAttributes method3Attr;
    UA_MethodAttributes_init(&method3Attr);
    method3Attr.description = UA_LOCALIZEDTEXT("en_US", "FooBar");
    method3Attr.displayName = UA_LOCALIZEDTEXT("en_US", "FooBar");
    method3Attr.executable = true;
    method3Attr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "FooBar"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(1, "FooBar"),
                            method3Attr, NULL, NULL,
                            1, &inputArguments1, 1, &outputArguments1, NULL);
    /* If the method node has no callback (because it was instantiated without
     * one) or if we just want to change it, this can be done
     * UA_Server_setMethodNode_callback() */
    UA_Server_setMethodNode_callback(server,  UA_NODEID_NUMERIC(1,62542),
                                     &fooBarMethod, NULL);

    /* start server */
    UA_StatusCode retval = UA_Server_run(server, &running);

    /* ctrl-c received -> clean up */
    UA_UInt32_delete(pInputDimensions);
    UA_UInt32_delete(pOutputDimensions);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);

    return (int)retval;
}
