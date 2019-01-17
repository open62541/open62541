/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#ifdef UA_ENABLE_AMALGAMATION
#include "open62541.h"
#else
#include "ua_server.h"
#include "ua_config_default.h"
#include "ua_server_config.h"
#include "ua_log_stdout.h"
#endif

#include "datatypes_generated.h"

#include <signal.h>


UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

static void
addDataType(UA_Server *server) {
    UA_VariableTypeAttributes dattr = UA_VariableTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "OptionObject");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "OptionObject");
    dattr.dataType = DATATYPES[0].typeId;
    dattr.valueRank = UA_VALUERANK_SCALAR;

    UA_OptionObject o;
    o.optionOne = false;
    o.optionTwo = true;
    o.optionalByteOne = 0x12;
    o.optionalByteTwo = 0x34;
    UA_Variant_setScalar(&dattr.value, &o, &DATATYPES[0]);

    UA_Server_addVariableTypeNode(server, DATATYPES[0].typeId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                  UA_QUALIFIEDNAME(1, "OptionObject"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  dattr, NULL, NULL);

}

static void
addVariable(UA_Server *server) {
    UA_OptionObject o;
    o.optionOne = true;
    o.optionTwo = false;
    o.optionalByteOne = 0x56;
    o.optionalByteTwo = 0x78;

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description = UA_LOCALIZEDTEXT("en-US", "OptionObject Var");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "OptionObject Var");
    vattr.dataType = DATATYPES[0].typeId;
    vattr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&vattr.value, &o, &DATATYPES[0]);

    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "OptionObject"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "OptionObject"),
                              DATATYPES[0].typeId, vattr, NULL, NULL);
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    /* Make your custom datatype known to the stack
     * Attention! Here the custom datatypes are allocated on the stack. So they
     * cannot be accessed from parallel (worker) threads. */
    UA_DataTypeArray customDataTypes = {config->customDataTypes, 1, DATATYPES};

    config->customDataTypes = &customDataTypes;

    UA_Server *server = UA_Server_new(config);

    addDataType(server);
    addVariable(server);

    UA_Server_run(server, &running);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return 0;
}
