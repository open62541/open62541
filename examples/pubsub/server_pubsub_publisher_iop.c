/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include <signal.h>
#include <stdlib.h>

#define Publisher_ID 60

static void addPublisher1(UA_Server *server, UA_NodeId publishedDataSetId);
static void addPublisher2(UA_Server *server, UA_NodeId publishedDataSetId);

static UA_NodeId ds1BoolToggleId;
static int ds1BoolToggleCount = 0;
static UA_Boolean ds1BoolToggleVal = false;
static UA_NodeId ds1Int32Id;
static UA_Int32 ds1Int32Val = 0;
static UA_NodeId ds1Int32FastId;
static UA_Int32 ds1Int32FastVal = 0;
static UA_NodeId ds1DateTimeId;

static UA_NodeId ds2BoolToggleId;
static UA_Boolean ds2BoolToggleVal = false;
static UA_NodeId ds2ByteId;
static UA_Byte ds2ByteVal = 0;
static UA_NodeId ds2Int16Id;
static UA_Int16 ds2Int16Val = 0;
static UA_NodeId ds2Int32Id;
static UA_Int32 ds2Int32Val = 0;
static UA_NodeId ds2Int64Id;
static UA_Int64 ds2Int64Val = 0;
static UA_NodeId ds2SByteId;
static UA_SByte ds2SByteVal = 0;
static UA_NodeId ds2UInt16Id;
static UA_UInt16 ds2UInt16Val = 0;
static UA_NodeId ds2UInt32Id;
static UA_UInt32 ds2UInt32Val = 0;
static UA_NodeId ds2UInt64Id;
static UA_UInt64 ds2UInt64Val = 0;
static UA_NodeId ds2FloatId;
static UA_Float ds2FloatVal = 0;
static UA_NodeId ds2DoubleId;
static UA_Double ds2DoubleVal = 0;
static UA_String *ds2StringArray = NULL;
static size_t ds2StringArrayLen = 0;
static size_t ds2StringIndex = 0;
static UA_NodeId ds2StringId;
static UA_NodeId ds2ByteStringId;
static UA_NodeId ds2GuidId;
static UA_NodeId ds2DateTimeId;
static UA_NodeId ds2UInt32ArrId;
static UA_UInt32 ds2UInt32ArrValue[10] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 };

UA_NodeId connectionIdent;

void
timerCallback(UA_Server *server, void *data);

static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    /* Details about the connection configuration and handling are located
    * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
        &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = Publisher_ID;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

static void addPublisher1(UA_Server *server, UA_NodeId publishedDataSetId) {
    UA_NodeId folderId;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Publisher 1");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Publisher 1"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &folderId);

    UA_NodeId_init(&ds1BoolToggleId);
    UA_VariableAttributes boolToggleAttr = UA_VariableAttributes_default;
    boolToggleAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_BOOLEAN].typeId, &boolToggleAttr.dataType);
    UA_Variant_setScalar(&boolToggleAttr.value, &ds1BoolToggleVal, &UA_TYPES[UA_TYPES_BOOLEAN]);
    boolToggleAttr.displayName = UA_LOCALIZEDTEXT("en-US", "BoolToggle");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher1.BoolToggle"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "BoolToggle"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), boolToggleAttr, NULL, &ds1BoolToggleId);

    UA_NodeId_init(&ds1Int32Id);
    UA_VariableAttributes int32Attr = UA_VariableAttributes_default;
    int32Attr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &int32Attr.dataType);
    int32Attr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&int32Attr.value, &ds1Int32Val, &UA_TYPES[UA_TYPES_INT32]);
    int32Attr.displayName = UA_LOCALIZEDTEXT("en-US", "Int32");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher1.Int32"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Int32"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), int32Attr, NULL, &ds1Int32Id);

    UA_NodeId_init(&ds1Int32FastId);
    UA_VariableAttributes int32FastAttr = UA_VariableAttributes_default;
    int32FastAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &int32FastAttr.dataType);
    int32FastAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&int32FastAttr.value, &ds1Int32FastVal, &UA_TYPES[UA_TYPES_INT32]);
    int32FastAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Int32Fast");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher1.Int32Fast"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Int32Fast"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), int32FastAttr, NULL, &ds1Int32FastId);

    UA_NodeId_init(&ds1DateTimeId);
    UA_VariableAttributes dateTimeAttr = UA_VariableAttributes_default;
    dateTimeAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_DATETIME].typeId, &dateTimeAttr.dataType);
    dateTimeAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_DateTime dateTimeVal = UA_DateTime_now();
    UA_Variant_setScalar(&dateTimeAttr.value, &dateTimeVal, &UA_TYPES[UA_TYPES_DATETIME]);
    dateTimeAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DateTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher1.DateTime"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "DateTime"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), dateTimeAttr, NULL, &ds1DateTimeId);

    if (!UA_NodeId_equal(&publishedDataSetId, &UA_NODEID_NULL))
    {
        // Create and add fields to the PublishedDataSet
        UA_DataSetFieldConfig boolToggleConfig;
        memset(&boolToggleConfig, 0, sizeof(UA_DataSetFieldConfig));
        boolToggleConfig.field.variable.fieldNameAlias = UA_STRING("BoolToggle");
        boolToggleConfig.field.variable.promotedField = false;
        boolToggleConfig.field.variable.publishParameters.publishedVariable = ds1BoolToggleId;
        boolToggleConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig int32Config;
        memset(&int32Config, 0, sizeof(UA_DataSetFieldConfig));
        int32Config.field.variable.fieldNameAlias = UA_STRING("Int32");
        int32Config.field.variable.promotedField = false;
        int32Config.field.variable.publishParameters.publishedVariable = ds1Int32Id;
        int32Config.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig int32FastConfig;
        memset(&int32FastConfig, 0, sizeof(UA_DataSetFieldConfig));
        int32FastConfig.field.variable.fieldNameAlias = UA_STRING("Int32Fast");
        int32FastConfig.field.variable.promotedField = false;
        int32FastConfig.field.variable.publishParameters.publishedVariable = ds1Int32FastId;
        int32FastConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig dateTimeConfig;
        memset(&dateTimeConfig, 0, sizeof(UA_DataSetFieldConfig));
        dateTimeConfig.field.variable.fieldNameAlias = UA_STRING("DateTime");
        dateTimeConfig.field.variable.promotedField = false;
        dateTimeConfig.field.variable.publishParameters.publishedVariable = ds1DateTimeId;
        dateTimeConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_NodeId f1, f2, f3, f4;
        // add fields in reverse order, because all fields are added to the beginning of the list
        UA_Server_addDataSetField(server, publishedDataSetId, &dateTimeConfig, &f4);
        UA_Server_addDataSetField(server, publishedDataSetId, &int32FastConfig, &f3);
        UA_Server_addDataSetField(server, publishedDataSetId, &int32Config, &f2);
        UA_Server_addDataSetField(server, publishedDataSetId, &boolToggleConfig, &f1);
    }
}

static void addPublisher2(UA_Server *server, UA_NodeId publishedDataSetId) {
    UA_NodeId folderId;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Publisher 2");
    UA_Server_addObjectNode(server, UA_NODEID_NULL,
        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
        UA_QUALIFIEDNAME(1, "Publisher 2"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
        oAttr, NULL, &folderId);

    UA_NodeId_init(&ds2BoolToggleId);
    UA_VariableAttributes boolToggleAttr = UA_VariableAttributes_default;
    boolToggleAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_BOOLEAN].typeId, &boolToggleAttr.dataType);
    UA_Variant_setScalar(&boolToggleAttr.value, &ds2BoolToggleVal, &UA_TYPES[UA_TYPES_BOOLEAN]);
    boolToggleAttr.displayName = UA_LOCALIZEDTEXT("en-US", "BoolToggle");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.BoolToggle"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "BoolToggle"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), boolToggleAttr, NULL, &ds2BoolToggleId);

    UA_NodeId_init(&ds2ByteId);
    UA_VariableAttributes byteAttr = UA_VariableAttributes_default;
    byteAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_BYTE].typeId, &byteAttr.dataType);
    byteAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&byteAttr.value, &ds2ByteVal, &UA_TYPES[UA_TYPES_BYTE]);
    byteAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Byte");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.Byte"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Byte"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), byteAttr, NULL, &ds2ByteId);

    UA_NodeId_init(&ds2Int16Id);
    UA_VariableAttributes int16Attr = UA_VariableAttributes_default;
    int16Attr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT16].typeId, &int16Attr.dataType);
    int16Attr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&int16Attr.value, &ds2Int16Val, &UA_TYPES[UA_TYPES_INT16]);
    int16Attr.displayName = UA_LOCALIZEDTEXT("en-US", "Int16");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.Int16"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Int16"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), int16Attr, NULL, &ds2Int16Id);

    UA_NodeId_init(&ds2Int32Id);
    UA_VariableAttributes int32Attr = UA_VariableAttributes_default;
    int32Attr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT32].typeId, &int32Attr.dataType);
    int32Attr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&int32Attr.value, &ds2Int32Val, &UA_TYPES[UA_TYPES_INT32]);
    int32Attr.displayName = UA_LOCALIZEDTEXT("en-US", "Int32");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.Int32"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Int32"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), int32Attr, NULL, &ds2Int32Id);

    UA_NodeId_init(&ds2Int64Id);
    UA_VariableAttributes int64Attr = UA_VariableAttributes_default;
    int64Attr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_INT64].typeId, &int64Attr.dataType);
    int64Attr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&int64Attr.value, &ds2Int64Val, &UA_TYPES[UA_TYPES_INT64]);
    int64Attr.displayName = UA_LOCALIZEDTEXT("en-US", "Int64");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.Int64"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Int64"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), int64Attr, NULL, &ds2Int64Id);

    UA_NodeId_init(&ds2SByteId);
    UA_VariableAttributes sbyteAttr = UA_VariableAttributes_default;
    sbyteAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_SBYTE].typeId, &sbyteAttr.dataType);
    sbyteAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&sbyteAttr.value, &ds2SByteVal, &UA_TYPES[UA_TYPES_SBYTE]);
    sbyteAttr.displayName = UA_LOCALIZEDTEXT("en-US", "SByte");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.SByte"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "SByte"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), sbyteAttr, NULL, &ds2SByteId);

    UA_NodeId_init(&ds2UInt16Id);
    UA_VariableAttributes uint16Attr = UA_VariableAttributes_default;
    uint16Attr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT16].typeId, &uint16Attr.dataType);
    uint16Attr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&uint16Attr.value, &ds2UInt16Val, &UA_TYPES[UA_TYPES_UINT16]);
    uint16Attr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt16");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.UInt16"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "UInt16"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), uint16Attr, NULL, &ds2UInt16Id);

    UA_NodeId_init(&ds2UInt32Id);
    UA_VariableAttributes uint32Attr = UA_VariableAttributes_default;
    uint32Attr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId, &uint32Attr.dataType);
    uint32Attr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&uint32Attr.value, &ds2UInt32Val, &UA_TYPES[UA_TYPES_UINT32]);
    uint32Attr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt32");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.UInt32"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "UInt32"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), uint32Attr, NULL, &ds2UInt32Id);

    UA_NodeId_init(&ds2UInt64Id);
    UA_VariableAttributes uint64Attr = UA_VariableAttributes_default;
    uint64Attr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT64].typeId, &uint64Attr.dataType);
    uint64Attr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&uint64Attr.value, &ds2UInt64Val, &UA_TYPES[UA_TYPES_UINT64]);
    uint64Attr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt64");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.UInt64"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "UInt64"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), uint64Attr, NULL, &ds2UInt64Id);

    UA_NodeId_init(&ds2FloatId);
    UA_VariableAttributes floatAttr = UA_VariableAttributes_default;
    floatAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_FLOAT].typeId, &floatAttr.dataType);
    floatAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&floatAttr.value, &ds2FloatVal, &UA_TYPES[UA_TYPES_FLOAT]);
    floatAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Float");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.Float"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Float"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), floatAttr, NULL, &ds2FloatId);

    UA_NodeId_init(&ds2DoubleId);
    UA_VariableAttributes doubleAttr = UA_VariableAttributes_default;
    doubleAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_DOUBLE].typeId, &doubleAttr.dataType);
    doubleAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&doubleAttr.value, &ds2DoubleVal, &UA_TYPES[UA_TYPES_DOUBLE]);
    doubleAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Double");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.Double"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Double"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), doubleAttr, NULL, &ds2DoubleId);

    ds2StringArrayLen = 26;
    ds2StringArray = (UA_String*)UA_Array_new(ds2StringArrayLen, &UA_TYPES[UA_TYPES_STRING]);
    ds2StringArray[0] = UA_STRING_ALLOC("Alpha");
    ds2StringArray[1] = UA_STRING_ALLOC("Bravo");
    ds2StringArray[2] = UA_STRING_ALLOC("Charlie");
    ds2StringArray[3] = UA_STRING_ALLOC("Delta");
    ds2StringArray[4] = UA_STRING_ALLOC("Echo");
    ds2StringArray[5] = UA_STRING_ALLOC("Foxtrot");
    ds2StringArray[6] = UA_STRING_ALLOC("Golf");
    ds2StringArray[7] = UA_STRING_ALLOC("Hotel");
    ds2StringArray[8] = UA_STRING_ALLOC("India");
    ds2StringArray[9] = UA_STRING_ALLOC("Juliet");
    ds2StringArray[10] = UA_STRING_ALLOC("Kilo");
    ds2StringArray[11] = UA_STRING_ALLOC("Lima");
    ds2StringArray[12] = UA_STRING_ALLOC("Mike");
    ds2StringArray[13] = UA_STRING_ALLOC("November");
    ds2StringArray[14] = UA_STRING_ALLOC("Oscar");
    ds2StringArray[15] = UA_STRING_ALLOC("Papa");
    ds2StringArray[16] = UA_STRING_ALLOC("Quebec");
    ds2StringArray[17] = UA_STRING_ALLOC("Romeo");
    ds2StringArray[18] = UA_STRING_ALLOC("Sierra");
    ds2StringArray[19] = UA_STRING_ALLOC("Tango");
    ds2StringArray[20] = UA_STRING_ALLOC("Uniform");
    ds2StringArray[21] = UA_STRING_ALLOC("Victor");
    ds2StringArray[22] = UA_STRING_ALLOC("Whiskey");
    ds2StringArray[23] = UA_STRING_ALLOC("X-ray");
    ds2StringArray[24] = UA_STRING_ALLOC("Yankee");
    ds2StringArray[25] = UA_STRING_ALLOC("Zulu");

    UA_String stringVal;
    UA_String_init(&stringVal);

    UA_NodeId_init(&ds2StringId);
    UA_VariableAttributes stringAttr = UA_VariableAttributes_default;
    stringAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_STRING].typeId, &stringAttr.dataType);
    stringAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&stringAttr.value, &stringVal, &UA_TYPES[UA_TYPES_STRING]);
    stringAttr.displayName = UA_LOCALIZEDTEXT("en-US", "String");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.String"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "String"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), stringAttr, NULL, &ds2StringId);

    UA_Byte data[] = { 0x00 };
    UA_ByteString byteStringVal = { 1, data };

    UA_NodeId_init(&ds2ByteStringId);
    UA_VariableAttributes byteStringAttr = UA_VariableAttributes_default;
    byteStringAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_BYTESTRING].typeId, &byteStringAttr.dataType);
    byteStringAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&byteStringAttr.value, &byteStringVal, &UA_TYPES[UA_TYPES_BYTESTRING]);
    byteStringAttr.displayName = UA_LOCALIZEDTEXT("en-US", "ByteString");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.ByteString"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "ByteString"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), byteStringAttr, NULL, &ds2ByteStringId);

    UA_Guid guidVal = UA_Guid_random();
    UA_NodeId_init(&ds2GuidId);
    UA_VariableAttributes guidAttr = UA_VariableAttributes_default;
    guidAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_GUID].typeId, &guidAttr.dataType);
    guidAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_Variant_setScalar(&guidAttr.value, &guidVal, &UA_TYPES[UA_TYPES_GUID]);
    guidAttr.displayName = UA_LOCALIZEDTEXT("en-US", "Guid");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.Guid"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "Guid"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), guidAttr, NULL, &ds2GuidId);

    UA_NodeId_init(&ds2DateTimeId);
    UA_VariableAttributes dateTimeAttr = UA_VariableAttributes_default;
    dateTimeAttr.valueRank = -1;
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_DATETIME].typeId, &dateTimeAttr.dataType);
    dateTimeAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;
    UA_DateTime dateTimeVal = UA_DateTime_now();
    UA_Variant_setScalar(&dateTimeAttr.value, &dateTimeVal, &UA_TYPES[UA_TYPES_DATETIME]);
    dateTimeAttr.displayName = UA_LOCALIZEDTEXT("en-US", "DateTime");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.DateTime"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "DateTime"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), dateTimeAttr, NULL, &ds2DateTimeId);

    // UInt32Array
    UA_NodeId_init(&ds2UInt32ArrId);
    UA_VariableAttributes uint32ArrAttr = UA_VariableAttributes_default;
    uint32ArrAttr.valueRank = 1;    // 1-dimensional array
    uint32ArrAttr.arrayDimensionsSize = 1;
    UA_UInt32 arrayDims[1] = { 10 };
    uint32ArrAttr.arrayDimensions = arrayDims;

    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId, &uint32ArrAttr.dataType);
    uint32ArrAttr.accessLevel = UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE;

    UA_Variant_setArray(&uint32ArrAttr.value, ds2UInt32ArrValue, 10, &UA_TYPES[UA_TYPES_UINT32]);
    uint32ArrAttr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt32Array");
    UA_Server_addVariableNode(server, UA_NODEID_STRING(1, "Publisher2.UInt32Array"), folderId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
        UA_QUALIFIEDNAME(1, "UInt32Array"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), uint32ArrAttr, NULL, &ds2UInt32ArrId);

    if (!UA_NodeId_equal(&publishedDataSetId, &UA_NODEID_NULL))
    {
        // Create and add fields to the PublishedDataSet
        UA_DataSetFieldConfig boolToggleConfig;
        memset(&boolToggleConfig, 0, sizeof(UA_DataSetFieldConfig));
        boolToggleConfig.field.variable.fieldNameAlias = UA_STRING("BoolToggle");
        boolToggleConfig.field.variable.promotedField = false;
        boolToggleConfig.field.variable.publishParameters.publishedVariable = ds2BoolToggleId;
        boolToggleConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig byteConfig;
        memset(&byteConfig, 0, sizeof(UA_DataSetFieldConfig));
        byteConfig.field.variable.fieldNameAlias = UA_STRING("Byte");
        byteConfig.field.variable.promotedField = false;
        byteConfig.field.variable.publishParameters.publishedVariable = ds2ByteId;
        byteConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig int16Config;
        memset(&int16Config, 0, sizeof(UA_DataSetFieldConfig));
        int16Config.field.variable.fieldNameAlias = UA_STRING("Int16");
        int16Config.field.variable.promotedField = false;
        int16Config.field.variable.publishParameters.publishedVariable = ds2Int16Id;
        int16Config.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig int32Config;
        memset(&int32Config, 0, sizeof(UA_DataSetFieldConfig));
        int32Config.field.variable.fieldNameAlias = UA_STRING("Int32");
        int32Config.field.variable.promotedField = false;
        int32Config.field.variable.publishParameters.publishedVariable = ds2Int32Id;
        int32Config.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig int64Config;
        memset(&int64Config, 0, sizeof(UA_DataSetFieldConfig));
        int64Config.field.variable.fieldNameAlias = UA_STRING("Int64");
        int64Config.field.variable.promotedField = false;
        int64Config.field.variable.publishParameters.publishedVariable = ds2Int64Id;
        int64Config.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig sbyteConfig;
        memset(&sbyteConfig, 0, sizeof(UA_DataSetFieldConfig));
        sbyteConfig.field.variable.fieldNameAlias = UA_STRING("SByte");
        sbyteConfig.field.variable.promotedField = false;
        sbyteConfig.field.variable.publishParameters.publishedVariable = ds2SByteId;
        sbyteConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig uint16Config;
        memset(&uint16Config, 0, sizeof(UA_DataSetFieldConfig));
        uint16Config.field.variable.fieldNameAlias = UA_STRING("UInt16");
        uint16Config.field.variable.promotedField = false;
        uint16Config.field.variable.publishParameters.publishedVariable = ds2UInt16Id;
        uint16Config.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig uint32Config;
        memset(&uint32Config, 0, sizeof(UA_DataSetFieldConfig));
        uint32Config.field.variable.fieldNameAlias = UA_STRING("UInt32");
        uint32Config.field.variable.promotedField = false;
        uint32Config.field.variable.publishParameters.publishedVariable = ds2UInt32Id;
        uint32Config.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig uint64Config;
        memset(&uint64Config, 0, sizeof(UA_DataSetFieldConfig));
        uint64Config.field.variable.fieldNameAlias = UA_STRING("UInt64");
        uint64Config.field.variable.promotedField = false;
        uint64Config.field.variable.publishParameters.publishedVariable = ds2UInt64Id;
        uint64Config.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig floatConfig;
        memset(&floatConfig, 0, sizeof(UA_DataSetFieldConfig));
        floatConfig.field.variable.fieldNameAlias = UA_STRING("Float");
        floatConfig.field.variable.promotedField = false;
        floatConfig.field.variable.publishParameters.publishedVariable = ds2FloatId;
        floatConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig doubleConfig;
        memset(&doubleConfig, 0, sizeof(UA_DataSetFieldConfig));
        doubleConfig.field.variable.fieldNameAlias = UA_STRING("Double");
        doubleConfig.field.variable.promotedField = false;
        doubleConfig.field.variable.publishParameters.publishedVariable = ds2DoubleId;
        doubleConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig stringConfig;
        memset(&stringConfig, 0, sizeof(UA_DataSetFieldConfig));
        stringConfig.field.variable.fieldNameAlias = UA_STRING("String");
        stringConfig.field.variable.promotedField = false;
        stringConfig.field.variable.publishParameters.publishedVariable = ds2StringId;
        stringConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig byteStringConfig;
        memset(&byteStringConfig, 0, sizeof(UA_DataSetFieldConfig));
        byteStringConfig.field.variable.fieldNameAlias = UA_STRING("ByteString");
        byteStringConfig.field.variable.promotedField = false;
        byteStringConfig.field.variable.publishParameters.publishedVariable = ds2ByteStringId;
        byteStringConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig guidConfig;
        memset(&guidConfig, 0, sizeof(UA_DataSetFieldConfig));
        guidConfig.field.variable.fieldNameAlias = UA_STRING("Guid");
        guidConfig.field.variable.promotedField = false;
        guidConfig.field.variable.publishParameters.publishedVariable = ds2GuidId;
        guidConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig dateTimeConfig;
        memset(&dateTimeConfig, 0, sizeof(UA_DataSetFieldConfig));
        dateTimeConfig.field.variable.fieldNameAlias = UA_STRING("DateTime");
        dateTimeConfig.field.variable.promotedField = false;
        dateTimeConfig.field.variable.publishParameters.publishedVariable = ds2DateTimeId;
        dateTimeConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_DataSetFieldConfig uint32ArrConfig;
        memset(&uint32ArrConfig, 0, sizeof(UA_DataSetFieldConfig));
        uint32ArrConfig.field.variable.fieldNameAlias = UA_STRING("UInt32Array");
        uint32ArrConfig.field.variable.promotedField = false;
        uint32ArrConfig.field.variable.publishParameters.publishedVariable = ds2UInt32ArrId;
        uint32ArrConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

        UA_NodeId f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16;
        // add fields in reverse order, because all fields are added to the beginning of the list
        UA_Server_addDataSetField(server, publishedDataSetId, &uint32ArrConfig, &f16);
        UA_Server_addDataSetField(server, publishedDataSetId, &dateTimeConfig, &f15);
        UA_Server_addDataSetField(server, publishedDataSetId, &guidConfig, &f14);
        UA_Server_addDataSetField(server, publishedDataSetId, &byteStringConfig, &f13);
        UA_Server_addDataSetField(server, publishedDataSetId, &stringConfig, &f12);
        UA_Server_addDataSetField(server, publishedDataSetId, &doubleConfig, &f11);
        UA_Server_addDataSetField(server, publishedDataSetId, &floatConfig, &f10);
        UA_Server_addDataSetField(server, publishedDataSetId, &uint64Config, &f9);
        UA_Server_addDataSetField(server, publishedDataSetId, &uint32Config, &f8);
        UA_Server_addDataSetField(server, publishedDataSetId, &uint16Config, &f7);
        UA_Server_addDataSetField(server, publishedDataSetId, &sbyteConfig, &f6);
        UA_Server_addDataSetField(server, publishedDataSetId, &int64Config, &f5);
        UA_Server_addDataSetField(server, publishedDataSetId, &int32Config, &f4);
        UA_Server_addDataSetField(server, publishedDataSetId, &int16Config, &f3);
        UA_Server_addDataSetField(server, publishedDataSetId, &byteConfig, &f2);
        UA_Server_addDataSetField(server, publishedDataSetId, &boolToggleConfig, &f1);
    }
}

void
timerCallback(UA_Server *server, void *data) {
    // DataSet 1
    // BoolToggle
    ds1BoolToggleCount++;
    UA_Variant tmpVari;
    if (ds1BoolToggleCount >= 3) {
        if (ds1BoolToggleVal) {
            ds1BoolToggleVal = false;
        } else {
            ds1BoolToggleVal = true;
        }

        UA_Variant_init(&tmpVari);
        UA_Variant_setScalar(&tmpVari, &ds1BoolToggleVal, &UA_TYPES[UA_TYPES_BOOLEAN]);
        UA_Server_writeValue(server, ds1BoolToggleId, tmpVari);
        ds1BoolToggleCount = 0;
    }

    // Int32
    UA_Variant_init(&tmpVari);
    ds1Int32Val++;
    if(ds1Int32Val > 10000)
        ds1Int32Val = 0;
    UA_Variant_setScalar(&tmpVari, &ds1Int32Val, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, ds1Int32Id, tmpVari);

    // Int32Fast
    UA_Variant_init(&tmpVari);
    ds1Int32FastVal += 100;
    if(ds1Int32FastVal > 10000)
        ds1Int32FastVal = 0;
    UA_Variant_setScalar(&tmpVari, &ds1Int32FastVal, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, ds1Int32FastId, tmpVari);

    // DateTime
    UA_Variant_init(&tmpVari);
    UA_DateTime tmpTime = UA_DateTime_now();
    UA_Variant_setScalar(&tmpVari, &tmpTime, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Server_writeValue(server, ds1DateTimeId, tmpVari);

    // DataSet 2
    UA_Server_writeValue(server, ds2DateTimeId, tmpVari);

    // BoolToggle
    if (ds2BoolToggleVal)
        ds2BoolToggleVal = false;
    else
        ds2BoolToggleVal = true;

    // Write new value
    UA_Variant_init(&tmpVari);
    UA_Variant_setScalar(&tmpVari, &ds2BoolToggleVal, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, ds2BoolToggleId, tmpVari);

    // Byte
    UA_Variant_init(&tmpVari);
    ds2ByteVal++;
    UA_Variant_setScalar(&tmpVari, &ds2ByteVal, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Server_writeValue(server, ds2ByteId, tmpVari);

    // Int16
    UA_Variant_init(&tmpVari);
    ds2Int16Val++;
    UA_Variant_setScalar(&tmpVari, &ds2Int16Val, &UA_TYPES[UA_TYPES_INT16]);
    UA_Server_writeValue(server, ds2Int16Id, tmpVari);

    // Int32
    UA_Variant_init(&tmpVari);
    ds2Int32Val++;
    UA_Variant_setScalar(&tmpVari, &ds2Int32Val, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, ds2Int32Id, tmpVari);

    // Int64
    UA_Variant_init(&tmpVari);
    ds2Int64Val++;
    UA_Variant_setScalar(&tmpVari, &ds2Int64Val, &UA_TYPES[UA_TYPES_INT64]);
    UA_Server_writeValue(server, ds2Int64Id, tmpVari);

    // SByte
    UA_Variant_init(&tmpVari);
    ds2SByteVal++;
    UA_Variant_setScalar(&tmpVari, &ds2SByteVal, &UA_TYPES[UA_TYPES_SBYTE]);
    UA_Server_writeValue(server, ds2SByteId, tmpVari);

    // UInt16
    UA_Variant_init(&tmpVari);
    ds2UInt16Val++;
    UA_Variant_setScalar(&tmpVari, &ds2UInt16Val, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, ds2UInt16Id, tmpVari);

    // UInt32
    UA_Variant_init(&tmpVari);
    ds2UInt32Val++;
    UA_Variant_setScalar(&tmpVari, &ds2UInt32Val, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, ds2UInt32Id, tmpVari);

    // UInt64
    UA_Variant_init(&tmpVari);
    ds2UInt64Val++;
    UA_Variant_setScalar(&tmpVari, &ds2UInt64Val, &UA_TYPES[UA_TYPES_UINT64]);
    UA_Server_writeValue(server, ds2UInt64Id, tmpVari);

    // Float
    UA_Variant_init(&tmpVari);
    ds2FloatVal++;
    UA_Variant_setScalar(&tmpVari, &ds2FloatVal, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Server_writeValue(server, ds2FloatId, tmpVari);

    // Double
    UA_Variant_init(&tmpVari);
    ds2DoubleVal++;
    UA_Variant_setScalar(&tmpVari, &ds2DoubleVal, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, ds2DoubleId, tmpVari);

    // String
    UA_Variant_init(&tmpVari);
    ds2StringIndex++;
    if(ds2StringIndex >= ds2StringArrayLen)
        ds2StringIndex = 0;

    UA_Variant_setScalar(&tmpVari, &ds2StringArray[ds2StringIndex], &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, ds2StringId, tmpVari);

    // ByteString
    UA_Variant_init(&tmpVari);
    UA_ByteString bs;
    UA_ByteString_init(&bs);
    UA_ByteString_allocBuffer(&bs, 4);
    UA_UInt32 ui2 = UA_UInt32_random();
    memcpy(bs.data, &ui2, 4);

    UA_Variant_setScalar(&tmpVari, &bs, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Server_writeValue(server, ds2ByteStringId, tmpVari);
    UA_ByteString_clear(&bs);

    // Guid
    UA_Variant_init(&tmpVari);
    UA_Guid g = UA_Guid_random();
    UA_Variant_setScalar(&tmpVari, &g, &UA_TYPES[UA_TYPES_GUID]);
    UA_Server_writeValue(server, ds2GuidId, tmpVari);

    // UInt32Array
    for(size_t i = 0; i < 10; i++)
        ds2UInt32ArrValue[i]++;

    UA_Variant_init(&tmpVari);
    UA_Variant_setArray(&tmpVari, ds2UInt32ArrValue, 10, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, ds2UInt32ArrId, tmpVari);
}

UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

static int run(UA_String *transportProfile,
    UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, 4802, NULL);

    /* Details about the connection configuration and handling are located in
    * the pubsub connection tutorial */
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

    addPubSubConnection(server, transportProfile, networkAddressUrl);

    /* Create a PublishedDataSet based on a PublishedDataSetConfig. */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("DataSet 1 (Simple)");

    UA_NodeId publishedDataSetIdent;
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);

    addPublisher1(server, publishedDataSetIdent);

    /* Create a new WriterGroup and configure parameters like the publish interval. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("DataSet WriterGroup");
    writerGroupConfig.publishingInterval = 500;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    writerGroupConfig.maxEncapsulatedDataSetMessageCount = 3;

    /* Add the new WriterGroup to an existing Connection. */
    UA_NodeId writerGroupIdent;
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);

    /* Create a new Writer and connect it with an existing PublishedDataSet */
    // DataSetWriter ID 1 with Variant Encoding
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSet 1 DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 1;
    //The creation of delta messages is configured in the following line. Value
    // 0 -> no delta messages are created.
    dataSetWriterConfig.keyFrameCount = 10;

    UA_NodeId writerIdentifier;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
        &dataSetWriterConfig, &writerIdentifier);

    // Published DataSet 2
    UA_PublishedDataSetConfig publishedDataSetConfig2;
    publishedDataSetConfig2.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig2.name = UA_STRING("DataSet 2 (AllTypes)");

    UA_NodeId publishedDataSetIdent2;
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig2, &publishedDataSetIdent2);

    addPublisher2(server, publishedDataSetIdent2);

    // DataSet Writer 2
    // Create a new Writer and connect it with an existing PublishedDataSet
    UA_DataSetWriterConfig dataSetWriterConfig2;

    memset(&dataSetWriterConfig2, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig2.name = UA_STRING("DataSet 2 DataSetWriter");
    dataSetWriterConfig2.dataSetWriterId = 2;
    //The creation of delta messages is configured in the following line. Value
    // 0 -> no delta messages are created.
    dataSetWriterConfig2.keyFrameCount = 10;

    UA_NodeId writerIdentifier2;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent2,
        &dataSetWriterConfig2, &writerIdentifier2);

    UA_UInt64 timerCallbackId = 0;
    UA_Server_addRepeatedCallback(server, (UA_ServerCallback)timerCallback, NULL, 1000, &timerCallbackId);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
    { UA_STRING_NULL, UA_STRING("opc.udp://239.0.0.1:4840/") };

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        }
        else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return EXIT_FAILURE;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        }
        else {
            printf("Error: unknown URI\n");
            return EXIT_FAILURE;
        }
    }

    return run(&transportProfile, &networkAddressUrl);
}
