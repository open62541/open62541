/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Working with Meta Type
 * ---------------------------
 *
 * The MetaType structure is a versatile construct, designed to dynamically process variables
 * whose data types are unknown at compile time.
 * Instead of relying on pre-generated C structures for specific data types,
 * the MetaType structure enables runtime handling by describing the type metadata,
 * including fields, their types, sizes, and any nested data types.
 *
 * In the example in this tutorial, we represent a motor that contains a further structure
 * in addition to simple fields.
 * The following functions add the appropriate DataTypeNode to the data type hierarchy.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

#include "open62541/server_config_default.h"

const UA_NodeId diagnosticVariableTypeId = {
    1, UA_NODEIDTYPE_NUMERIC, {4246}};

/* Define the Motor structure */
typedef struct {
    UA_Int32 errorCode;
    UA_Int64 timestamp;
} DiagnosticInformation;

/* Define padding between structure members */
#define DiagnosticInformation_padding_timestamp offsetof(DiagnosticInformation, timestamp) - offsetof(DiagnosticInformation, errorCode) - sizeof(UA_Int32)

/* Define the binary encoding IDs */
#define DiagnosticInformation_binary_encoding_id  1002

/* Define the members for the Motor */
static UA_DataTypeMember DiagnosticInformation_members[2] = {
    /* errorCode */
    {
        UA_TYPENAME("errorCode")   /* .memberName */
        &UA_TYPES[UA_TYPES_INT32],  /* .memberType */
        0,                             /* .padding */
        false,                          /* .isArray */
        false                         /* .isOptional */
    },
    /* timestamp */
    {
        UA_TYPENAME("timestamp")             /* .memberName */
        &UA_TYPES[UA_TYPES_INT64],            /* .memberType */
        DiagnosticInformation_padding_timestamp, /* .padding */
        false,                                    /* .isArray */
        false                                   /* .isOptional */
    }
};

/* Define the Motor DataType */
static const UA_DataType DiagnosticInformationType = {
    UA_TYPENAME("DiagnosticInformationType") /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {4245}}, /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {DiagnosticInformation_binary_encoding_id}}, /* .binaryEncodingId */
    sizeof(DiagnosticInformation),            /* .memSize */
    UA_DATATYPEKIND_STRUCTURE,                /* .typeKind */
    true,                                   /* .pointerFree */
    false,                                  /* .overlayable */
    sizeof(DiagnosticInformation_members) / sizeof(UA_DataTypeMember), /* .membersSize */
    DiagnosticInformation_members             /* .members */
};


static void addDiagnosticInformationDataType(UA_Server* server) {
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "DiagnosticInformation Type");

    UA_Server_addDataTypeNode(server,DiagnosticInformationType.typeId, UA_NS0ID(STRUCTURE),
                              UA_NS0ID(HASSUBTYPE), UA_QUALIFIEDNAME(1, "DiagnosticInformationDataType"),
                              attr, NULL, NULL);
}

static void
addDiagnosticInformationVariableType(UA_Server *server) {
    UA_VariableTypeAttributes dattr = UA_VariableTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "DiagnosticInformation");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "DiagnosticInformation");
    dattr.dataType = DiagnosticInformationType.typeId;
    dattr.valueRank = UA_VALUERANK_SCALAR;

    DiagnosticInformation m;
    m.errorCode = 0;
    m.timestamp = 0;
    UA_Variant_setScalar(&dattr.value, &m, &DiagnosticInformationType);

    UA_Server_addVariableTypeNode(server, diagnosticVariableTypeId,
                                  UA_NS0ID(BASEDATAVARIABLETYPE),
                                  UA_NS0ID(HASSUBTYPE),
                                  UA_QUALIFIEDNAME(1, "DiagnosticInformationVariableType"),
                                  UA_NS0ID(BASEDATAVARIABLETYPE),
                                  dattr, NULL, NULL);
}

const UA_NodeId motorVariableTypeId = {
    1, UA_NODEIDTYPE_NUMERIC, {4244}};

/* Define the Motor structure */
typedef struct {
    UA_Float speed;
    UA_Float torque;
    UA_UInt32 power;
    DiagnosticInformation diagnosticInformation;
    UA_Float temperatures[4];
} Motor;

/* Define padding between structure members */
#define Motor_padding_torque offsetof(Motor, torque) - offsetof(Motor, speed) - sizeof(UA_Float)
#define Motor_padding_power offsetof(Motor, power) - offsetof(Motor, torque) - sizeof(UA_Float)
#define Motor_padding_diagnosticInformation offsetof(Motor, diagnosticInformation) - offsetof(Motor, power) - sizeof(UA_UInt32)
#define Motor_padding_temperatures offsetof(Motor, temperatures) - offsetof(Motor, diagnosticInformation) - sizeof(DiagnosticInformation)

/* Define the binary encoding IDs */
#define Motor_binary_encoding_id  1001

/* Define the members for the Motor */
static UA_DataTypeMember Motor_members[5] = {
    /* speed */
    {
        UA_TYPENAME("speed")       /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT],  /* .memberType */
        0,                             /* .padding */
        false,                          /* .isArray */
        false                         /* .isOptional */
    },
    /* torque */
    {
        UA_TYPENAME("torque")       /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT],   /* .memberType */
        Motor_padding_torque,           /* .padding */
        false,                           /* .isArray */
        false                          /* .isOptional */
    },
    /* power */
    {
        UA_TYPENAME("power")        /* .memberName */
        &UA_TYPES[UA_TYPES_UINT32],  /* .memberType */
        Motor_padding_power,            /* .padding */
        false,                           /* .isArray */
        false                          /* .isOptional */
    },
    /* diagnosticInformation */
    {
    UA_TYPENAME("diagnosticInformation") /* .memberName */
    &DiagnosticInformationType,                      /* .memberType */
    Motor_padding_diagnosticInformation,     /* .padding */
    false,                                    /* .isArray */
    false                                   /* .isOptional */
    },
    /* temperatures */
    {
        UA_TYPENAME("temperatures")  /* .memberName */
        &UA_TYPES[UA_TYPES_FLOAT],    /* .memberType */
        Motor_padding_temperatures,      /* .padding */
        true,                             /* .isArray */
        false                           /* .isOptional */
    }
};

/* Define the Motor DataType */
static const UA_DataType MotorType = {
    UA_TYPENAME("MotorType")          /* .typeName */
    {1, UA_NODEIDTYPE_NUMERIC, {4243}}, /* .typeId */
    {1, UA_NODEIDTYPE_NUMERIC, {Motor_binary_encoding_id}}, /* .binaryEncodingId */
    sizeof(Motor),                     /* .memSize */
    UA_DATATYPEKIND_STRUCTURE,         /* .typeKind */
    true,                            /* .pointerFree */
    false,                           /* .overlayable */
    sizeof(Motor_members) / sizeof(UA_DataTypeMember), /* .membersSize */
    Motor_members                      /* .members */
};


static void addMotorDataType(UA_Server* server) {
    UA_DataTypeAttributes attr = UA_DataTypeAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "Motor Type");

    UA_Server_addDataTypeNode(server, MotorType.typeId, UA_NS0ID(STRUCTURE),
                              UA_NS0ID(HASSUBTYPE), UA_QUALIFIEDNAME(1, "MotorDataType"),
                              attr, NULL, NULL);
}

static void
addMotorVariableType(UA_Server *server) {
    UA_VariableTypeAttributes dattr = UA_VariableTypeAttributes_default;
    dattr.description = UA_LOCALIZEDTEXT("en-US", "Motor");
    dattr.displayName = UA_LOCALIZEDTEXT("en-US", "Motor");
    dattr.dataType = MotorType.typeId;
    dattr.valueRank = UA_VALUERANK_SCALAR;

    Motor m;
    m.power = 0;
    m.speed = 0.0;
    m.diagnosticInformation.errorCode = 0;
    m.diagnosticInformation.timestamp = 0;
    m.temperatures[0] = 0.0;
    m.temperatures[1] = 0.0;
    m.temperatures[2] = 0.0;
    m.temperatures[3] = 0.0;
    m.torque = 0.0;
    UA_Variant_setScalar(&dattr.value, &m, &MotorType);

    UA_Server_addVariableTypeNode(server, motorVariableTypeId,
                                  UA_NS0ID(BASEDATAVARIABLETYPE),
                                  UA_NS0ID(HASSUBTYPE),
                                  UA_QUALIFIEDNAME(1, "MotorVariableType"),
                                  UA_NS0ID(BASEDATAVARIABLETYPE),
                                  dattr, NULL, NULL);
}

/**
 * MetaType Workflow
 * -----------------
 *
 * Definition:
 *      Define the MetaType structure, specifying its members, their types, sizes, and properties (e.g., scalar, array, optional).
 *
 * Initialization:
 *      Initialize a MetaType instance using UA_MetaTypeValueDescription_init(), associating it with the data type at runtime.
 *
 *  Read and Write Operations:
 *      Dynamically read data using readValue, leveraging MetaType metadata to interpret variable data.
 *      Write new values by decoding them into the MetaType structure.
 *
 * Encoding/Decoding:
 *      Use UA_MetaTypeValueDescription_encodeToVariant() to serialize a MetaType into a variant.
 *      Decode data from a variant using UA_MetaTypeValueDescription_decodeFromVariant().
 *
 */

static UA_StatusCode
readValue(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
    UA_ServerConfig *config = UA_Server_getConfig(server);

    UA_MetaTypeValueDescription motorType;
    UA_MetaTypeValueDescription_init(&motorType, &config->customDataTypes[0].types[0]);
    UA_Float speed = 10.50;
    UA_MetaTypeMember_setScalar(&motorType.members[0], &speed, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_Float torgue = 50.25;
    UA_MetaTypeMember_setScalar(&motorType.members[1], &torgue, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_UInt32 power = 125;
    UA_MetaTypeMember_setScalar(&motorType.members[2], &power, &UA_TYPES[UA_TYPES_UINT32]);


    UA_MetaTypeValueDescription diagnosticInformation;
    UA_MetaTypeValueDescription_init(&diagnosticInformation, &config->customDataTypes[0].types[1]);

    UA_Int32 errorCode = 10;
    UA_MetaTypeMember_setScalar(&diagnosticInformation.members[0], &errorCode, &UA_TYPES[UA_TYPES_INT32]);

    UA_Int64 timestamp = 1720618028;
    UA_MetaTypeMember_setScalar(&diagnosticInformation.members[1], &timestamp, &UA_TYPES[UA_TYPES_INT64]);

    UA_MetaTypeMember_setMetaTypeValue(&motorType.members[3], &diagnosticInformation);
    UA_MetaTypeValueDescription_clear(&diagnosticInformation);

    UA_Float temperatures[4] = {23.5, 30.1, 40.75, 24.0};
    UA_MetaTypeMember_setArray(&motorType.members[4], temperatures, 4, &UA_TYPES[UA_TYPES_FLOAT]);

    UA_MetaTypeValueDescription_encodeToVariant(&motorType, &dataValue->value);
    dataValue->hasValue = true;

    UA_MetaTypeValueDescription_clear(&motorType);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeValue(UA_Server *server,
                 const UA_NodeId *sessionId, void *sessionContext,
                 const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *data) {
    UA_MetaTypeValueDescription metaType;
    UA_MetaTypeValueDescription_init(&metaType, data->value.type);
    UA_MetaTypeValueDescription_decodeFromVariant(&metaType, &data->value);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: %.2f", metaType.members[0].name, *(UA_Float*)metaType.members[0].value);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: %.2f", metaType.members[1].name, *(UA_Float*)metaType.members[1].value);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: %d", metaType.members[2].name, *(UA_UInt32*)metaType.members[2].value);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: %d", metaType.members[3].name, *(UA_Int32*)metaType.members[3].members->members[0].value);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: %d", metaType.members[3].name, *(UA_Int64*)metaType.members[3].members->members[1].value);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: array size: %ld", metaType.members[4].name, metaType.members[4].arraySize);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: data1: %.2f", metaType.members[4].name, (*(UA_Float**)metaType.members[4].value)[0]);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: data2: %.2f", metaType.members[4].name, (*(UA_Float**)metaType.members[4].value)[1]);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: data3: %.2f", metaType.members[4].name, (*(UA_Float**)metaType.members[4].value)[2]);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "%s: data4: %.2f", metaType.members[4].name, (*(UA_Float**)metaType.members[4].value)[3]);

    UA_MetaTypeValueDescription_clear(&metaType);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addMotorVariable(UA_Server *server) {
    /* Define the attribute of the myInteger variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US","motor1");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","motor1");
    attr.dataType = MotorType.typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    /* Add the variable node to the information model */
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "motor1");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "motor1");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = motorVariableTypeId;

    UA_DataSource motorSource;
    motorSource.read = readValue;
    motorSource.write = writeValue;
    return UA_Server_addDataSourceVariableNode(server, myIntegerNodeId, parentNodeId,
                                        parentReferenceNodeId, myIntegerName,
                                        variableTypeNodeId, attr,
                                        motorSource, NULL, NULL);
}

/** It follows the main server code, making use of the above definitions. */

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    /* Make your custom datatype known to the stack */
    UA_DataType *types = (UA_DataType*)UA_malloc(2 * sizeof(UA_DataType));
    UA_DataTypeMember *motorMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 5);
    motorMembers[0] = Motor_members[0];
    motorMembers[1] = Motor_members[1];
    motorMembers[2] = Motor_members[2];
    motorMembers[3] = Motor_members[3];
    motorMembers[4] = Motor_members[4];
    UA_DataTypeMember *diagnosticMembers = (UA_DataTypeMember*)UA_malloc(sizeof(UA_DataTypeMember) * 2);
    diagnosticMembers[0] = DiagnosticInformation_members[0];
    diagnosticMembers[1] = DiagnosticInformation_members[1];

    types[0] = MotorType;
    types[0].members = motorMembers;
    types[1] = DiagnosticInformationType;
    types[1].members = diagnosticMembers;

    /* Attention! Here the custom datatypes are allocated on the stack. So they
     * cannot be accessed from parallel (worker) threads. */
    UA_DataTypeArray customDataTypes = {config->customDataTypes, 2, types, UA_FALSE};
    config->customDataTypes = &customDataTypes;

    addDiagnosticInformationDataType(server);
    addDiagnosticInformationVariableType(server);

    addMotorDataType(server);
    addMotorVariableType(server);
    addMotorVariable(server);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
