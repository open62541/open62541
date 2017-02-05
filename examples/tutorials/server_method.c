/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Adding a server-side method
 * ---------------------------
 *
 * This tutorial demonstrates how to add method nodes to the server. Use an UA
 * client, e.g., UaExpert to call the method (right-click on the method node ->
 * call).
 *
 * The first example shows how to define input and output arguments (lines 72 -
 * 88), make the method executable (lines 94,95), add the method node (line
 * 96-101) with a specified method callback (lines 10 - 24).
 * 
 * The second example shows that a method can also be applied on an array as
 * input argument and output argument.
 *
 * The last example presents a way to bind a new method callback to an already
 * instantiated method node. */

#include <stdlib.h>
#include <signal.h>
#include "open62541.h"

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
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Hello World was called");
    return UA_STATUSCODE_GOOD;
}

UA_Boolean running = true;
static void stopHandler(int sign) {
    running = 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */
    signal(SIGTERM, stopHandler);

    /* Initialize the server */
    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl =
        UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    /* Add the method node with the callback */
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.arrayDimensionsSize = 0;
    inputArguments.arrayDimensions = NULL;
    inputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en_US", "A String");
    inputArguments.name = UA_STRING("MyInput");
    inputArguments.valueRank = -1;

    UA_Argument outputArguments;
    UA_Argument_init(&outputArguments);
    outputArguments.arrayDimensionsSize = 0;
    outputArguments.arrayDimensions = NULL;
    outputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArguments.description = UA_LOCALIZEDTEXT("en_US", "A String");
    outputArguments.name = UA_STRING("MyOutput");
    outputArguments.valueRank = -1;

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
                            1, &inputArguments, 1, &outputArguments, NULL);

    /* Run server */
    UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return 0;
}
