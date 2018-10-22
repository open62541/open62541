/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <signal.h>
#include <open62541.h>
#include "custom_datatype.h"

UA_Boolean running = true;

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

static void
add3PointDataType(UA_Server *server) {
    UA_VariableTypeAttributes dattr = UA_VariableTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "3D Point");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "3D Point");
    dattr.dataType = PointType.typeId;
    dattr.valueRank = UA_VALUERANK_SCALAR;

    Point p;
    p.x = 0.0;
    p.y = 0.0;
    p.z = 0.0;
    UA_Variant_setScalar(&dattr.value, &p, &PointType);

    UA_Server_addVariableTypeNode(server, PointType.typeId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                  UA_QUALIFIEDNAME(1, "3D.Point"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  dattr, NULL, NULL);

}

static void
add3DPointVariable(UA_Server *server) {
    Point p;
    p.x = 3.0;
    p.y = 4.0;
    p.z = 5.0;
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description = UA_LOCALIZEDTEXT("en-US", "3D Point");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "3D Point");
    vattr.dataType = PointType.typeId;
    vattr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&vattr.value, &p, &PointType);

    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "3D.Point"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "3D.Point"),
                              PointType.typeId, vattr, NULL, NULL);
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_ServerConfig *config = UA_ServerConfig_new_default();
    /* Make your custom datatype known to the stack */
    UA_DataType types[1];
    types[0] = PointType;
    config->customDataTypes = types;
    config->customDataTypesSize = 1;

    UA_Server *server = UA_Server_new(config);

    add3PointDataType(server);
    add3DPointVariable(server);

    UA_Server_run(server, &running);

    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return 0;
}
