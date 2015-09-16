/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <signal.h>
#include <stdio.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "logger_stdout.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif

UA_Boolean running = 1;
UA_Logger logger;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = 0;
}

static UA_StatusCode readInteger(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue) {
    dataValue->hasValue = UA_TRUE;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)handle, &UA_TYPES[UA_TYPES_INT32]);
    //note that this is only possible if the identifier is a string - but we are sure to have one here
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node read %.*s",nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "read value %i", *(UA_UInt32*)handle);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeInteger(void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range){
    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_INT32] && data->data){
        *(UA_UInt32*)handle = *(UA_Int32*)data->data;
    }
    //note that this is only possible if the identifier is a string - but we are sure to have one here
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node written %.*s",nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "written value %i", *(UA_UInt32*)handle);
    return UA_STATUSCODE_GOOD;
}


int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    logger = Logger_Stdout_new();
    UA_Server_setLogger(server, logger);
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));

    UA_Int32 myInteger = 42;

    /* add a variable node to the address space */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer"); /* UA_NODEID_NULL would assign a random free nodeid */
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_LocalizedText myIntegerBrowseName = UA_LOCALIZEDTEXT("en_US","the answer");

    UA_DataSource dateDataSource = (UA_DataSource) {.handle = &myInteger, .read = readInteger, .write = writeInteger};

    UA_Server_addDataSourceVariableNode(server, myIntegerNodeId, myIntegerName, myIntegerBrowseName, myIntegerBrowseName, 0, 0,

                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),

                                    dateDataSource,

                                    NULL);

    UA_StatusCode retval = UA_Server_run(server, 1, &running);
    UA_Server_delete(server);

    return retval;
}
