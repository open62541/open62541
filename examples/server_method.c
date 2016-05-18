/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <signal.h>
#include <stdlib.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "ua_config_standard.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

UA_Boolean running = true;
UA_Logger logger = Logger_Stdout;

static UA_StatusCode
helloWorldMethod(void *handle, const UA_NodeId objectId, size_t inputSize, const UA_Variant *input,
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

static UA_StatusCode
fooBarMethod(void *handle, const UA_NodeId objectId, size_t inputSize, const UA_Variant *input,
                 size_t outputSize, UA_Variant *output) {
	// Exactly the same as helloWorld, but returns foobar
        UA_String *inputStr = (UA_String*)input->data;
        UA_String tmp = UA_STRING_ALLOC("FooBar! ");
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

static UA_StatusCode
IncInt32ArrayValuesMethod(void *handle, const UA_NodeId objectId, size_t inputSize,
                          const UA_Variant *input, size_t outputSize, UA_Variant *output) {
	UA_Variant_setArrayCopy(output, input->data, 5, &UA_TYPES[UA_TYPES_INT32]);
	for(size_t i = 0; i< input->arrayLength; i++)
		((UA_Int32*)output->data)[i] = ((UA_Int32*)input->data)[i] + 1;
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
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    //EXAMPLE 1
    /* add the method node with the callback */
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

    //END OF EXAMPLE 1

    //EXAMPLE 2
    /* add another method node: output argument as 1d Int32 array*/
    // define input arguments
    UA_Argument_init(&inputArguments);
    inputArguments.arrayDimensionsSize = 1;
    UA_UInt32 * pInputDimensions = UA_UInt32_new();
    pInputDimensions[0] = 5;
    inputArguments.arrayDimensions = pInputDimensions;
    inputArguments.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en_US",
                    "input an array with 5 elements, type int32");
    inputArguments.name = UA_STRING("int32 value");
    inputArguments.valueRank = 1;

    // define output arguments
    UA_Argument_init(&outputArguments);
    outputArguments.arrayDimensionsSize = 1;
    UA_UInt32 * pOutputDimensions = UA_UInt32_new();
    pOutputDimensions[0] = 5;
    outputArguments.arrayDimensions = pOutputDimensions;
    outputArguments.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    outputArguments.description = UA_LOCALIZEDTEXT("en_US", "increment each array index");
    outputArguments.name = UA_STRING("output is the array, each index is incremented by one");
    outputArguments.valueRank = 1;

    
    UA_MethodAttributes incAttr;
    UA_MethodAttributes_init(&incAttr);
    incAttr.description = UA_LOCALIZEDTEXT("en_US","1dArrayExample");
    incAttr.displayName = UA_LOCALIZEDTEXT("en_US","1dArrayExample");
    incAttr.executable = true;
    incAttr.userExecutable = true;
    UA_Server_addMethodNode(server, UA_NODEID_STRING(1, "IncInt32ArrayValues"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), 
                            UA_QUALIFIEDNAME(1, "IncInt32ArrayValues"),
                            incAttr, &IncInt32ArrayValuesMethod, NULL,
                            1, &inputArguments, 1, &outputArguments, NULL);
    //END OF EXAMPLE 2

    /* If out methodnode is part of an instantiated object, we never had
       the opportunity to define the callback... we could do that now
    */
    UA_Server_setMethodNode_callback(server, UA_NODEID_NUMERIC(1,62541), &fooBarMethod, NULL);
    
    //END OF EXAMPLE 3
    /* start server */
    UA_StatusCode retval = UA_Server_run(server, &running);

    /* ctrl-c received -> clean up */
    UA_UInt32_delete(pInputDimensions);
    UA_UInt32_delete(pOutputDimensions);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);

    return (int)retval;
}


