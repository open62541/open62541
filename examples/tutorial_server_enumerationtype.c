/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * Working with Custom Enumeration Data Types and DataSourceVariableNodes
 * ----------------------------------------------------------------------
 *
 * Variable types have three functions:
 *
 * In the example of this tutorial, we represent two different types of custom
 * enumerations: EnumValueType and LocalizedTextType.
 *
 * LocalizedTextTypes are only allowed for 0-based values without any gaps,
 * whereas EnumValueTypes can have any value as the key (int32) with gaps.
 */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <stdio.h>

#define ENUMVALUES_LEN 5

/* Just some arbitrary value to start assigning numeric node IDs */
static UA_UInt32 FreeNodeId = 0x80000000;

static UA_DataType DataTypes[2] = {0};
static UA_DataTypeArray DataTypeArray = {NULL, 2, DataTypes, UA_FALSE};
static size_t UsedDataTypes = 0;

static UA_NodeId
addEnumerationDataType(UA_Server *server, UA_UInt16 nsIndex, char *name) {
    UA_DataType customEnumDataType = UA_TYPES[UA_TYPES_ENUMERATION];
    customEnumDataType.typeName = name;
    UA_NodeId enumerationDataTypeNodeId = UA_NODEID_NUMERIC(nsIndex, ++FreeNodeId);
    customEnumDataType.typeId = enumerationDataTypeNodeId;
    customEnumDataType.binaryEncodingId = enumerationDataTypeNodeId;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    if(config->customDataTypes == NULL) {
        config->customDataTypes = &DataTypeArray;
    }
    DataTypes[UsedDataTypes++] = customEnumDataType;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_DataTypeAttributes dAttr = UA_DataTypeAttributes_default;
    dAttr.displayName = UA_LOCALIZEDTEXT("", name);

    retVal = UA_Server_addDataTypeNode(
        server, enumerationDataTypeNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_ENUMERATION),
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE), UA_QUALIFIEDNAME(nsIndex, name), dAttr,
        NULL, NULL);
    assert(retVal == UA_STATUSCODE_GOOD);
    return enumerationDataTypeNodeId;
}

static void
addEnumValues(UA_Server *server, UA_Int16 nsIndex, UA_NodeId parentNodeId) {
    /* Create the variable node holding the enumeration values */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.userAccessLevel = UA_ACCESSLEVELMASK_READ;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
    vAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    vAttr.arrayDimensionsSize = 1;
    UA_UInt32 arrayDimensions[1] = {ENUMVALUES_LEN};
    vAttr.arrayDimensions = arrayDimensions;
    vAttr.displayName = UA_LOCALIZEDTEXT("", "EnumValues");
    vAttr.dataType = UA_TYPES[UA_TYPES_ENUMVALUETYPE].typeId;

    UA_EnumValueType enumValues[ENUMVALUES_LEN];
    for(size_t i = 0; i < ENUMVALUES_LEN; ++i) {
        enumValues[i].value = (UA_Int64)(i);
        char name[] = "EnumValue   ";
        snprintf(name + 10, sizeof(name), "%d", (int)i);
        enumValues[i].displayName = UA_LOCALIZEDTEXT_ALLOC("", name);
        char description[] = "Description   ";
        snprintf(description + 12, sizeof(description), "%d", (int)i);
        enumValues[i].description = UA_LOCALIZEDTEXT_ALLOC("", description);
    }

    UA_Variant_setArray(&vAttr.value, enumValues, 5, &UA_TYPES[UA_TYPES_ENUMVALUETYPE]);

    UA_StatusCode retVal = UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(nsIndex, ++FreeNodeId), parentNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), UA_QUALIFIEDNAME(0, "EnumValues"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), vAttr, NULL, NULL);
    assert(retVal == UA_STATUSCODE_GOOD);
}

static void
addLocalizedText(UA_Server *server, UA_Int16 nsIndex, UA_NodeId parentNodeId) {
    /* Create the variable node holding the enumeration values */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.userAccessLevel = UA_ACCESSLEVELMASK_READ;
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
    vAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
    vAttr.arrayDimensionsSize = 1;
    UA_UInt32 arrayDimensions[1] = {0};
    vAttr.arrayDimensions = arrayDimensions;
    vAttr.displayName = UA_LOCALIZEDTEXT("", "EnumStrings");
    vAttr.dataType = UA_TYPES[UA_TYPES_LOCALIZEDTEXT].typeId;

    UA_LocalizedText enumStrings[ENUMVALUES_LEN];
    for(size_t i = 0; i < ENUMVALUES_LEN; ++i) {
        char text[] = "EnumString   ";
        snprintf(text + 11, sizeof(text), "%d", (int)i);
        enumStrings[i] = UA_LOCALIZEDTEXT_ALLOC("", text);
    }

    UA_Variant_setArray(&vAttr.value, enumStrings, 5, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);

    UA_StatusCode retVal = UA_Server_addVariableNode(
        server, UA_NODEID_NUMERIC(nsIndex, ++FreeNodeId), parentNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), UA_QUALIFIEDNAME(0, "EnumStrings"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE), vAttr, NULL, NULL);
    assert(retVal == UA_STATUSCODE_GOOD);
}

static UA_NodeId
addFolderNode(UA_Server *server, UA_Int16 nsIndex) {
    UA_NodeId folderNodeId = UA_NODEID_NUMERIC(nsIndex, ++FreeNodeId);
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("", "MyFolder");
    UA_StatusCode retVal = UA_Server_addObjectNode(
        server, folderNodeId, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(nsIndex, "MyFolderQualifiedName"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), oAttr, NULL, NULL);
    assert(retVal == UA_STATUSCODE_GOOD);
    return folderNodeId;
}

static UA_StatusCode
readValue(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
          const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
          const UA_NumericRange *range, UA_DataValue *dataValue) {
    (void)server;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;
    (void)sourceTimeStamp;
    (void)range;

    if(dataValue) {
        /* Return different value on each read to test out all enumerations */
        static UA_Int32 value = 0;
        if(++value >= ENUMVALUES_LEN) {
            value = 0;
        }

        UA_StatusCode retVal = UA_Variant_setScalarCopy(&dataValue->value, &value,
                                                        &UA_TYPES[UA_TYPES_INT32]);
        if(retVal == UA_STATUSCODE_GOOD) {
            dataValue->hasValue = UA_TRUE;
            dataValue->sourceTimestamp = UA_DateTime_now();
        }
        return retVal;
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
writeValue(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
           const UA_NodeId *nodeId, void *nodeContext, const UA_NumericRange *range,
           const UA_DataValue *dataValue) {
    (void)server;
    (void)sessionId;
    (void)sessionContext;
    (void)nodeId;
    (void)nodeContext;
    (void)range;
    (void)dataValue;
    return UA_STATUSCODE_BADNOTFOUND;
}

static void
addVariableNode(UA_Server *server, UA_NodeId dataTypeNodeId, UA_UInt16 nsIndex,
                char *name, UA_NodeId parentNodeId) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.dataType = dataTypeNodeId;
    vAttr.valueRank = UA_VALUERANK_SCALAR;
    vAttr.displayName = UA_LOCALIZEDTEXT("", name);
    vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Int32 valueInt = 0;
    UA_Variant_setScalarCopy(&value, &valueInt, &UA_TYPES[UA_TYPES_INT32]);
    vAttr.value = value;

    UA_DataSource dataSource = {readValue, writeValue};
    UA_StatusCode retVal = UA_Server_addDataSourceVariableNode(
        server, UA_NODEID_NUMERIC(nsIndex, ++FreeNodeId), parentNodeId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(nsIndex, "EnumVariableQualifiedName"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vAttr, dataSource, NULL,
        NULL);
    assert(retVal == UA_STATUSCODE_GOOD);
}

/** It follows the main server code, making use of the above definitions. */
int
main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    UA_UInt16 testNs =
        UA_Server_addNamespace(server, "http://yourorganisation.org/test/");

    UA_NodeId parentNodeId = addFolderNode(server, testNs);

    char enumValueName[] = "CustomEnumValueType";
    UA_NodeId enumValueType = addEnumerationDataType(server, testNs, enumValueName);
    addEnumValues(server, testNs, enumValueType);
    addVariableNode(server, enumValueType, testNs, "EnumValueTypeVariable", parentNodeId);

    char enumTextName[] = "CustomLocalizedTextType";
    UA_NodeId enumLocalizedText = addEnumerationDataType(server, testNs, enumTextName);
    addLocalizedText(server, testNs, enumLocalizedText);
    addVariableNode(server, enumLocalizedText, testNs, "LocalizedTextVariable",
                    parentNodeId);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
