/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#include "custom_datatype.h"

UA_Boolean running = true;
const UA_NodeId pointVariableTypeId = {
    1, UA_NODEIDTYPE_NUMERIC, {4243}};
const UA_NodeId measurementVariableTypeId = {
    1, UA_NODEIDTYPE_NUMERIC, {4444}};
const UA_NodeId optstructVariableTypeId = {
    1, UA_NODEIDTYPE_NUMERIC, {4645}};
const UA_NodeId unionVariableTypeId = {
    1, UA_NODEIDTYPE_NUMERIC, {4846}};

static void stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

static void add3DPointDataType(UA_Server* server)
{
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "3D Point Type");

    UA_Server_addDataTypeNode(
        server, PointType.typeId, UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), UA_QUALIFIEDNAME(1, "3D.Point"), attr, NULL, NULL);
}

static void
add3DPointVariableType(UA_Server *server) {
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

    UA_Server_addVariableTypeNode(server, pointVariableTypeId,
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
                              pointVariableTypeId, vattr, NULL, NULL);
}

static void addMeasurementSeriesDataType(UA_Server* server)
{
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Measurement Series (Array) Type");

    UA_Server_addDataTypeNode(
        server, MeasurementType.typeId, UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), UA_QUALIFIEDNAME(1, "Measurement Series"), attr, NULL, NULL);
}

static void
addMeasurementSeriesVariableType(UA_Server *server) {
    UA_VariableTypeAttributes dattr = UA_VariableTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "Measurement Series");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "Measurement Series");
    dattr.dataType = MeasurementType.typeId;
    dattr.valueRank = UA_VALUERANK_ANY;

    Measurements m;
    memset(&m, 0, sizeof(Measurements));
    m.description = UA_STRING("");
    m.measurementSize = 0;
    m.measurement = (UA_Float *) UA_Array_new(m.measurementSize, &MeasurementType);
    UA_Variant_setScalar(&dattr.value, &m, &MeasurementType);

    UA_Server_addVariableTypeNode(server, measurementVariableTypeId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                  UA_QUALIFIEDNAME(1, "Measurement Series"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  dattr, NULL, NULL);

}

static void
addMeasurementSeriesVariable(UA_Server *server) {
    Measurements m;
    m.description = UA_STRING_ALLOC("TestDesc");
    m.measurementSize = 3;
    m.measurement = (UA_Float *) UA_Array_new(m.measurementSize, &MeasurementType);
    m.measurement[0] = (UA_Float) 19.1;
    m.measurement[1] = (UA_Float) 20.2;
    m.measurement[2] = (UA_Float) 19.7;

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description = UA_LOCALIZEDTEXT("en-US", "Temp Measurement (Array Example)");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Temp Measurement (Array Example)");
    vattr.dataType = MeasurementType.typeId;
    vattr.valueRank = UA_VALUERANK_ANY;
    UA_Variant_setScalar(&vattr.value, &m, &MeasurementType);

    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Temp.Measurement"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "Temp Measurement (Array Example)"),
                              measurementVariableTypeId, vattr, NULL, NULL);
    UA_clear(&m, &MeasurementType);
}

static void addOptStructDataType(UA_Server* server)
{
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "OptStruct Example Type");

    UA_Server_addDataTypeNode(
        server, OptType.typeId, UA_NODEID_NUMERIC(0, UA_NS0ID_STRUCTURE),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), UA_QUALIFIEDNAME(1, "OptStruct Example"), attr, NULL, NULL);
}

static void
addOptStructVariableType(UA_Server *server) {
    UA_VariableTypeAttributes dattr = UA_VariableTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "OptStruct Example");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "OptStruct Example");
    dattr.dataType = OptType.typeId;
    dattr.valueRank = UA_VALUERANK_SCALAR;

    Opt o;
    memset(&o, 0, sizeof(Opt));
    UA_Variant_setScalar(&dattr.value, &o, &OptType);

    UA_Server_addVariableTypeNode(server, optstructVariableTypeId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                  UA_QUALIFIEDNAME(1, "OptStruct Example"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  dattr, NULL, NULL);
}

static void
addOptStructVariable(UA_Server *server) {
    Opt o;
    memset(&o, 0, sizeof(Opt));
    o.a = 3;
    o.b = NULL;
    o.c = UA_Float_new();
    *o.c = (UA_Float) 10.10;

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description = UA_LOCALIZEDTEXT("en-US", "OptStruct Example");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "OptStruct Example");
    vattr.dataType = OptType.typeId;
    vattr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&vattr.value, &o, &OptType);

    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Optstruct.Value"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "OptStruct Example"),
                              optstructVariableTypeId, vattr, NULL, NULL);
    UA_clear(&o, &OptType);
}

static void addUnionExampleDataType(UA_Server* server)
{
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Union Example Type");

    UA_Server_addDataTypeNode(
        server, UniType.typeId, UA_NODEID_NUMERIC(0, UA_NS0ID_UNION),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), UA_QUALIFIEDNAME(1, "Union Example"), attr, NULL, NULL);
}

static void
addUnionExampleVariableType(UA_Server *server) {
    UA_VariableTypeAttributes dattr = UA_VariableTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "Union Example");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "Union Example");
    dattr.dataType = UniType.typeId;
    dattr.valueRank = UA_VALUERANK_SCALAR;

    Uni u;
    memset(&u, 0, sizeof(Uni));
    UA_Variant_setScalar(&dattr.value, &u, &UniType);

    UA_Server_addVariableTypeNode(server, unionVariableTypeId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                  UA_QUALIFIEDNAME(1, "Union Example"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  dattr, NULL, NULL);
}

static void
addUnionExampleVariable(UA_Server *server) {
    Uni u;
    u.switchField = UA_UNISWITCH_OPTIONB;
    u.fields.optionB = UA_STRING("test string");

    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.description = UA_LOCALIZEDTEXT("en-US", "Union Example");
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Union Example");
    vattr.dataType = UniType.typeId;
    vattr.valueRank = UA_VALUERANK_SCALAR;
    UA_Variant_setScalar(&vattr.value, &u, &UniType);

    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Union.Value"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              UA_QUALIFIEDNAME(1, "Union Example"),
                              unionVariableTypeId, vattr, NULL, NULL);
}

int main(void) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    /* Make your custom datatype known to the stack */
    UA_DataType *types = (UA_DataType*)UA_malloc(4 * sizeof(UA_DataType));
    UA_DataTypeMember *pointMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 3);
    pointMembers[0] = Point_members[0];
    pointMembers[1] = Point_members[1];
    pointMembers[2] = Point_members[2];
    types[0] = PointType;
    types[0].members = pointMembers;

    types[1] = MeasurementType;
    UA_DataTypeMember *measurementMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 2);
    measurementMembers[0] = Measurements_members[0];
    measurementMembers[1] = Measurements_members[1];
    types[1].members = measurementMembers;

    types[2] = OptType;
    UA_DataTypeMember *optMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 3);
    optMembers[0] = Opt_members[0];
    optMembers[1] = Opt_members[1];
    optMembers[2] = Opt_members[2];
    types[2].members = optMembers;

    types[3] = UniType;
    UA_DataTypeMember *uniMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 2);
    uniMembers[0] = Uni_members[0];
    uniMembers[1] = Uni_members[1];
    types[3].members = uniMembers;

    /* Attention! Here the custom datatypes are allocated on the stack. So they
     * cannot be accessed from parallel (worker) threads. */
    UA_DataTypeArray customDataTypes = {config->customDataTypes, 4, types};
    config->customDataTypes = &customDataTypes;

    add3DPointDataType(server);
    add3DPointVariableType(server);
    add3DPointVariable(server);

    addMeasurementSeriesDataType(server);
    addMeasurementSeriesVariableType(server);
    addMeasurementSeriesVariable(server);

    addOptStructDataType(server);
    addOptStructVariableType(server);
    addOptStructVariable(server);

    addUnionExampleDataType(server);
    addUnionExampleVariableType(server);
    addUnionExampleVariable(server);

    UA_Server_run(server, &running);

    UA_Server_delete(server);
    UA_free(pointMembers);
    UA_free(measurementMembers);
    UA_free(optMembers);
    UA_free(uniMembers);
    UA_free(types);
    return EXIT_SUCCESS;
}
