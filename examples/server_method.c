/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <signal.h>
#include <stdlib.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

UA_Boolean running = UA_TRUE;
UA_Logger logger;

static UA_StatusCode helloWorldMethod(const UA_NodeId objectId, const UA_Variant *input, UA_Variant *output) {
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

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = 0;
}

int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    /* initialize the server */
    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    logger = Logger_Stdout_new();
    UA_Server_setLogger(server, logger);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    /* add the method node with the callback */
    UA_Argument inputArguments;
    UA_Argument_init(&inputArguments);
    inputArguments.arrayDimensionsSize = -1;
    inputArguments.arrayDimensions = NULL;
    inputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArguments.description = UA_LOCALIZEDTEXT("en_US", "A String");
    inputArguments.name = UA_STRING("MyInput");
    inputArguments.valueRank = -1;

    UA_Argument outputArguments;
    UA_Argument_init(&outputArguments);
    outputArguments.arrayDimensionsSize = -1;
    outputArguments.arrayDimensions = NULL;
    outputArguments.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArguments.description = UA_LOCALIZEDTEXT("en_US", "A String");
    outputArguments.name = UA_STRING("MyOutput");
    outputArguments.valueRank = -1;
        
    UA_Server_addMethodNode(server, UA_QUALIFIEDNAME(1, "hello world"), UA_NODEID_NUMERIC(1,62541),
                            UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            &helloWorldMethod, 1, &inputArguments, 1, &outputArguments);

    /* start server */
    UA_StatusCode retval = UA_Server_run(server, 1, &running); //blocks until running=false

    /* ctrl-c received -> clean up */
    UA_Server_delete(server);
    return retval;
}
