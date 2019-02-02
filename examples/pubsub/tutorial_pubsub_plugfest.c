/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

/**
 * .. _pubsub-tutorial:
 *
 * Working with Publish/Subscribe
 * ------------------------------
 *
 * Work in progress: This Tutorial will be continuously extended during the next
 * PubSub batches. More details about the PubSub extension and corresponding
 * open62541 API are located here: :ref:`pubsub`.
 *
 * Publishing Fields
 * ^^^^^^^^^^^^^^^^^
 * The PubSub publish example demonstrate the simplest way to publish
 * informations from the information model over UDP multicast using the UADP
 * encoding.
 *
 * **Connection handling**
 *
 * PubSubConnections can be created and deleted on runtime. More details about
 * the system preconfiguration and connection can be found in
 * ``tutorial_pubsub_connection.c``.
 */

#include <ua_server.h>
#include <ua_config_default.h>
#include <ua_log_stdout.h>
#include <ua_network_pubsub_udp.h>
#include <ua_network_pubsub_ethernet.h>

#include <signal.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>

UA_NodeId connectionIdent, publishedDataSetIdent, writerGroupIdent;

static void
addPubSubConnection(UA_Server *server, UA_String *transportProfile,
                    UA_NetworkAddressUrlDataType *networkAddressUrl){
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection 1");
    connectionConfig.transportProfileUri = *transportProfile;
    connectionConfig.enabled = UA_TRUE;
    UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.numeric = UA_UInt32_random();
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
}

/**
 * **PublishedDataSet handling**
 *
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and
 * can exist alone. The PDS contains the collection of the published fields. All
 * other PubSub elements are directly or indirectly linked with the PDS or
 * connection. */
static void
addPublishedDataSet(UA_Server *server) {
    /* The PublishedDataSetConfig contains all necessary public
    * informations for the creation of a new PublishedDataSet */
    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Demo PDS");
    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(server, &publishedDataSetConfig, &publishedDataSetIdent);
}

/**
 * **DataSetField handling**
 *
 * The DataSetField (DSF) is part of the PDS and describes exactly one published
 * field. */
static void
addDataSetField(UA_Server *server) {
    /* Add a field to the previous created PublishedDataSet */
    UA_NodeId dataSetFieldIdent;
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
    UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_Server_addDataSetField(server, publishedDataSetIdent,
                              &dataSetFieldConfig, &dataSetFieldIdent);
}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static void
addWriterGroup(UA_Server *server) {
    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Demo WriterGroup");
    writerGroupConfig.publishingInterval = 100;
    writerGroupConfig.enabled = UA_FALSE;
    writerGroupConfig.writerGroupId = 100;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    /* The configuration flags for the messages are encapsulated inside the
     * message- and transport settings extension objects. These extension
     * objects are defined by the standard. e.g.
     * UadpWriterGroupMessageDataType */
    UA_Server_addWriterGroup(server, connectionIdent, &writerGroupConfig, &writerGroupIdent);
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation. */
static void
addDataSetWriter(UA_Server *server) {
    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_NodeId dataSetWriterIdent;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Demo DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = 62541;
    dataSetWriterConfig.keyFrameCount = 10;
    UA_Server_addDataSetWriter(server, writerGroupIdent, publishedDataSetIdent,
                               &dataSetWriterConfig, &dataSetWriterIdent);
}

static void
any10MsecHandler (UA_Server *server, void *data){
    UA_Variant value;
    //part of DS1
    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 1003), &value);
    UA_Int32 int32DS1 = *((UA_Int32 *)value.data);
    if(int32DS1 < 10000)
        int32DS1++;
    else
        int32DS1 = 0;
    UA_Variant_setScalar(&value, &int32DS1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1003), value);
}

static void
any1secHandler (UA_Server *server, void *data){
    UA_Variant value;
    //part of DS1
    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 1002), &value);
    UA_Int32 int32DS1 = *((UA_Int32 *)value.data);
    if(int32DS1 < 10000)
        int32DS1++;
    else
        int32DS1 = 0;
    UA_Variant_setScalar(&value, &int32DS1, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1002), value);

    //part of DS2
    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2001), &value);
    UA_Boolean boolToggle = *((UA_Boolean *)value.data);
    if(boolToggle)
        boolToggle = UA_FALSE;
    else
        boolToggle = UA_TRUE;
    UA_Variant_setScalar(&value, &boolToggle, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2001), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2002), &value);
    UA_Byte byteDS2 = *((UA_Byte *)value.data);
    if(byteDS2 < UA_BYTE_MAX)
        byteDS2++;
    else
        byteDS2 = 0;
    UA_Variant_setScalar(&value, &byteDS2, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2002), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2003), &value);
    UA_Int16 int16DS2 = *((UA_Int16 *)value.data);
    if(int16DS2 < UA_INT16_MAX)
        int16DS2++;
    else
        int16DS2 = 0;
    UA_Variant_setScalar(&value, &int16DS2, &UA_TYPES[UA_TYPES_INT16]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2003), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2004), &value);
    UA_Int32 int32DS2 = *((UA_Int32 *)value.data);
    if(int32DS2 < UA_INT32_MAX)
        int32DS2++;
    else
        int32DS2 = 0;
    UA_Variant_setScalar(&value, &int32DS2, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2004), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2005), &value);
    UA_SByte sByteDS2 = *((UA_SByte *)value.data);
    if(sByteDS2 < UA_SBYTE_MAX)
        sByteDS2++;
    else
        sByteDS2 = 0;
    UA_Variant_setScalar(&value, &sByteDS2, &UA_TYPES[UA_TYPES_SBYTE]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2005), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2006), &value);
    UA_UInt16 uint16DS2 = *((UA_UInt16 *)value.data);
    if(uint16DS2 < UA_UINT16_MAX)
        uint16DS2++;
    else
        uint16DS2 = 0;
    UA_Variant_setScalar(&value, &uint16DS2, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2006), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2007), &value);
    UA_UInt32 uint32DS2 = *((UA_UInt32 *)value.data);
    if(uint32DS2 < UA_UINT32_MAX)
        uint32DS2++;
    else
        uint32DS2 = 0;
    UA_Variant_setScalar(&value, &uint32DS2, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2007), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2008), &value);
    UA_Float floatDS2 = *((UA_Float *)value.data);
    if(floatDS2 < FLT_MAX)
        floatDS2++;
    else
        floatDS2 = 0;
    UA_Variant_setScalar(&value, &floatDS2, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2008), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 2009), &value);
    UA_Double doubleDS2 = *((UA_Double *)value.data);
    if(doubleDS2 < DBL_MAX)
        doubleDS2++;
    else
        doubleDS2 = 0;
    UA_Variant_setScalar(&value, &doubleDS2, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 2009), value);

    //part of DS3
    for (int i = 0; i < 100; ++i) {
        UA_Server_readValue(server, UA_NODEID_NUMERIC(1, (UA_UInt32) (3001 + i)), &value);
        UA_UInt32 uint32DS3 = *((UA_UInt32 *)value.data);
        if(uint32DS3 < UA_UINT32_MAX)
            uint32DS3++;
        else
            uint32DS3 = (UA_UInt32) (i * 100);
        UA_Variant_setScalar(&value, &uint32DS3, &UA_TYPES[UA_TYPES_UINT32]);
        UA_Server_writeValue(server, UA_NODEID_NUMERIC(1,(UA_UInt32) (3001 + i)), value);
    }

    //part of DS4
    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4001), &value);
    UA_Boolean boolToggleDS4 = *((UA_Boolean *)value.data);
    if(boolToggleDS4)
        boolToggleDS4 = UA_FALSE;
    else
        boolToggleDS4 = UA_TRUE;
    UA_Variant_setScalar(&value, &boolToggleDS4, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4001), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4002), &value);
    UA_Byte byteDS4 = *((UA_Byte *)value.data);
    if(byteDS4 < UA_BYTE_MAX)
        byteDS4++;
    else
        byteDS4 = 0;
    UA_Variant_setScalar(&value, &byteDS4, &UA_TYPES[UA_TYPES_BYTE]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4002), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4003), &value);
    UA_Int16 int16DS4 = *((UA_Int16 *)value.data);
    if(int16DS4 < UA_INT16_MAX)
        int16DS4++;
    else
        int16DS4 = 0;
    UA_Variant_setScalar(&value, &int16DS4, &UA_TYPES[UA_TYPES_INT16]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4003), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4004), &value);
    UA_Int32 int32DS4 = *((UA_Int32 *)value.data);
    if(int32DS4 < UA_INT32_MAX)
        int32DS4++;
    else
        int32DS4 = 0;
    UA_Variant_setScalar(&value, &int32DS4, &UA_TYPES[UA_TYPES_INT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4004), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4005), &value);
    UA_SByte sByteDS4 = *((UA_SByte *)value.data);
    if(sByteDS4 < UA_SBYTE_MAX)
        sByteDS4++;
    else
        sByteDS4 = 0;
    UA_Variant_setScalar(&value, &sByteDS4, &UA_TYPES[UA_TYPES_SBYTE]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4005), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4006), &value);
    UA_UInt16 uint16DS4 = *((UA_UInt16 *)value.data);
    if(uint16DS4 < UA_UINT16_MAX)
        uint16DS4++;
    else
        uint16DS4 = 0;
    UA_Variant_setScalar(&value, &uint16DS4, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4006), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4007), &value);
    UA_UInt32 uint32DS4 = *((UA_UInt32 *)value.data);
    if(uint32DS4 < UA_UINT32_MAX)
        uint32DS4++;
    else
        uint32DS4 = 0;
    UA_Variant_setScalar(&value, &uint32DS4, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4007), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4008), &value);
    UA_Float floatDS4 = *((UA_Float *)value.data);
    if(floatDS4 < FLT_MAX)
        floatDS4++;
    else
        floatDS4 = 0;
    UA_Variant_setScalar(&value, &floatDS4, &UA_TYPES[UA_TYPES_FLOAT]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4008), value);

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4009), &value);
    UA_Double doubleDS4 = *((UA_Double *)value.data);
    if(doubleDS4 < DBL_MAX)
        doubleDS4++;
    else
        doubleDS4 = 0;
    UA_Variant_setScalar(&value, &doubleDS4, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4009), value);

    char natoAlphabet[26][20] = {"Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrott", "Golf",
    "Hotel", "India", "Juliet", "Kilo", "Lima", "Mike", "November",
    "Oscar", "Papa", "Quebec", "Romeo", "Sierra", "Tango", "Uniform",
    "Victor", "Whiskey", "X-Ray", "Yankie", "Zulu"};

    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4004), &value);
    int32DS4 = *((UA_Int32 *)value.data);
    UA_String stringDS4 = UA_STRING(natoAlphabet[int32DS4 % 26]);
    UA_Variant_setScalar(&value, &stringDS4, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4010), value);

    char alphabets[26] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};
    char rString[20];
    srand((unsigned int) time(NULL));
    int i=0;
    while(i<19) {
        int temp = rand() % 26;
        rString[i] = alphabets[temp];
        i++;
    }
    rString[19] = '\0';
    UA_ByteString byteStringDS4 = UA_BYTESTRING(rString);
    UA_Variant_setScalar(&value, &byteStringDS4, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4011), value);

    UA_Guid guidDS4 = UA_Guid_random();
    UA_Variant_setScalar(&value, &guidDS4, &UA_TYPES[UA_TYPES_GUID]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4012), value);

    UA_Variant uint32ArrayVariant;
    UA_UInt32 *uint32Array;
    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 4014), &value);
    uint32Array = (UA_UInt32 *) value.data;
    for (int j = 0; j < 10; ++j) {
        if(uint32Array[j] < UA_UINT32_MAX)
            uint32Array[j]++;
        else
            uint32Array[j] = (UA_UInt32) (j * 10);
    }
    UA_Variant_setArray(&uint32ArrayVariant, uint32Array, 10, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 4014), uint32ArrayVariant);

}

static void
any3secHandler (UA_Server *server, void *data){
    UA_Variant value;
    //part of DS1
    UA_Server_readValue(server, UA_NODEID_NUMERIC(1, 1001), &value);
    UA_Boolean boolToggle = *((UA_Boolean *)value.data);
    if(boolToggle)
        boolToggle = UA_FALSE;
    else
        boolToggle = UA_TRUE;
    UA_Variant_setScalar(&value, &boolToggle, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_Server_writeValue(server, UA_NODEID_NUMERIC(1, 1001), value);
}

static void
addPlugfestNodes(UA_Server *server) {
    UA_NodeId dataSet1ParamtersId, dataSet2ParamtersId, dataSet3ParamtersId, dataSet4ParamtersId;
    UA_ObjectAttributes oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en-US", "Plugfest_DataSet1Parameters");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1000), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
            UA_QUALIFIEDNAME(0, "Plugfest_DataSet1Parameters"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oattr, NULL, &dataSet1ParamtersId);
    oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en-US", "Plugfest_DataSet2Parameters");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 2000), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(0, "Plugfest_DataSet2Parameters"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oattr, NULL, &dataSet2ParamtersId);
    oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en-US", "Plugfest_DataSet3Parameters");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 3000), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(0, "Plugfest_DataSet3Parameters"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oattr, NULL, &dataSet3ParamtersId);
    oattr = UA_ObjectAttributes_default;
    oattr.displayName = UA_LOCALIZEDTEXT("en-US", "Plugfest_DataSet4Parameters");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 4000), UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                            UA_QUALIFIEDNAME(0, "Plugfest_DataSet4Parameters"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE), oattr, NULL, &dataSet4ParamtersId);
    //adding variables DataSet 1
    UA_VariableAttributes vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "BoolToggle");
    UA_Variant value;
    UA_Boolean boolToggle;
    UA_Variant_setScalar(&value, &boolToggle, &UA_TYPES[UA_TYPES_BOOLEAN]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1001), dataSet1ParamtersId,
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "BoolToggle"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Int32");
    UA_Int32 int32;
    UA_Variant_setScalar(&value, &int32, &UA_TYPES[UA_TYPES_INT32]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1002), dataSet1ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Int32"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Int32Fast");
    UA_Int32 int32Fast;
    UA_Variant_setScalar(&value, &int32Fast, &UA_TYPES[UA_TYPES_INT32]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1003), dataSet1ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Int32Fast"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "DateTime");
    UA_DateTime dateTime;
    UA_Variant_setScalar(&value, &dateTime, &UA_TYPES[UA_TYPES_DATETIME]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 1004), dataSet1ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "DateTime"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    //adding variables DataSet 2
    UA_Boolean boolToggleDS2 = 0;
    UA_Byte byteDS2 = 0;
    UA_Int16 int16DS2 = 0;
    UA_Int32 int32DS2 = 0;
    UA_SByte sbyteDS2 = 0;
    UA_UInt16 uint16DS2 = 0;
    UA_UInt32 uInt32DS2 = 0;
    UA_Float floatDS2 = 0;
    UA_Double doubleDS2 = 0;
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "BoolToggle");
    UA_Variant_setScalar(&value, &boolToggleDS2, &UA_TYPES[UA_TYPES_BOOLEAN]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2001), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "BoolToggle"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Byte");
    UA_Variant_setScalar(&value, &byteDS2, &UA_TYPES[UA_TYPES_BYTE]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2002), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Byte"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Int16");
    UA_Variant_setScalar(&value, &int16DS2, &UA_TYPES[UA_TYPES_INT16]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2003), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Int16"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Int32");
    UA_Variant_setScalar(&value, &int32DS2, &UA_TYPES[UA_TYPES_INT32]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2004), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Int32"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "SByte");
    UA_Variant_setScalar(&value, &sbyteDS2, &UA_TYPES[UA_TYPES_SBYTE]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2005), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "SByte"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);

    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt16");
    UA_Variant_setScalar(&value, &uint16DS2, &UA_TYPES[UA_TYPES_UINT16]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2006), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "UInt16"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt32");
    UA_Variant_setScalar(&value, &uInt32DS2, &UA_TYPES[UA_TYPES_UINT32]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2007), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "UInt32"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Float");
    UA_Variant_setScalar(&value, &floatDS2, &UA_TYPES[UA_TYPES_FLOAT]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2008), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Float"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Double");
    UA_Variant_setScalar(&value, &doubleDS2, &UA_TYPES[UA_TYPES_DOUBLE]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 2009), dataSet2ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Double"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    //adding variables DataSet 3
    UA_UInt32 * uint32ArrayDS3 = (UA_UInt32 *) UA_calloc(100, sizeof(UA_UInt32));
    for (int i = 0; i < 100; ++i) {
        char s[8];
        sprintf(s,"MASS_%i", i);
        vattr = UA_VariableAttributes_default;
        vattr.displayName = UA_LOCALIZEDTEXT("en-US", s);
        uint32ArrayDS3[i] = (UA_UInt32) (i * 100);
        UA_Variant_setScalar(&value, &uint32ArrayDS3[i], &UA_TYPES[UA_TYPES_UINT32]);
        vattr.value = value;
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, (UA_UInt32) (3001 + i)), dataSet3ParamtersId,
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, s),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    }
    //adding variables DataSet 4
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "BoolToggle");
    UA_Variant_setScalar(&value, &boolToggleDS2, &UA_TYPES[UA_TYPES_BOOLEAN]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4001), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "BoolToggle"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Byte");
    UA_Variant_setScalar(&value, &byteDS2, &UA_TYPES[UA_TYPES_BYTE]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4002), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Byte"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Int16");
    UA_Variant_setScalar(&value, &int16DS2, &UA_TYPES[UA_TYPES_INT16]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4003), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Int16"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Int32");
    UA_Variant_setScalar(&value, &int32DS2, &UA_TYPES[UA_TYPES_INT32]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4004), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Int32"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "SByte");
    UA_Variant_setScalar(&value, &sbyteDS2, &UA_TYPES[UA_TYPES_SBYTE]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4005), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "SByte"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);

    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt16");
    UA_Variant_setScalar(&value, &uint16DS2, &UA_TYPES[UA_TYPES_UINT16]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4006), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "UInt16"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt32");
    UA_Variant_setScalar(&value, &uInt32DS2, &UA_TYPES[UA_TYPES_UINT32]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4007), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "UInt32"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Float");
    UA_Variant_setScalar(&value, &floatDS2, &UA_TYPES[UA_TYPES_FLOAT]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4008), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Float"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Double");
    UA_Variant_setScalar(&value, &doubleDS2, &UA_TYPES[UA_TYPES_DOUBLE]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4009), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Double"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    UA_String stringDS4 = UA_STRING("Alpha");
    UA_ByteString byteStringDS4;
    UA_Guid guidDS4;
    UA_DateTime dateTimeDS4;

    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "String");
    UA_Variant_setScalar(&value, &stringDS4, &UA_TYPES[UA_TYPES_STRING]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4010), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "String"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);

    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "ByteString");
    UA_Variant_setScalar(&value, &byteStringDS4, &UA_TYPES[UA_TYPES_BYTESTRING]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4011), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "ByteString"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);

    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "Guid");
    UA_Variant_setScalar(&value, &guidDS4, &UA_TYPES[UA_TYPES_GUID]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4012), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "Guid"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);
    vattr = UA_VariableAttributes_default;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "DateTime");
    UA_Variant_setScalar(&value, &dateTimeDS4, &UA_TYPES[UA_TYPES_DATETIME]);
    vattr.value = value;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4013), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "DateTime"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);

    UA_UInt32 uint32ArrayDS4[10] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90};
    vattr = UA_VariableAttributes_default;
    UA_Variant valueArray;
    vattr.displayName = UA_LOCALIZEDTEXT("en-US", "UInt32Array");
    UA_Variant_setArray(&valueArray, uint32ArrayDS4, 10, &UA_TYPES[UA_TYPES_UINT32]);
    vattr.value = valueArray;
    UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(1, 4014), dataSet4ParamtersId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(0, "UInt32Array"),
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE), vattr, NULL, NULL);

    //add variable behavior
    UA_UInt64 repeatedCallback10MS, repeatedCallback1S, repeatedCallback3S;;
    UA_Server_addRepeatedCallback(server, any10MsecHandler, NULL, 10, &repeatedCallback10MS);
    UA_Server_addRepeatedCallback(server, any1secHandler, NULL, 1000, &repeatedCallback1S);
    UA_Server_addRepeatedCallback(server, any3secHandler, NULL, 3000, &repeatedCallback3S);

}

/**
 * That's it! You're now publishing the selected fields. Open a packet
 * inspection tool of trust e.g. wireshark and take a look on the outgoing
 * packages. The following graphic figures out the packages created by this
 * tutorial.
 *
 * .. figure:: ua-wireshark-pubsub.png
 *     :figwidth: 100 %
 *     :alt: OPC UA PubSub communication in wireshark
 *
 * The open62541 subscriber API will be released later. If you want to process
 * the the datagrams, take a look on the ua_network_pubsub_networkmessage.c
 * which already contains the decoding code for UADP messages.
 *
 * It follows the main server code, making use of the above definitions. */
UA_Boolean running = true;
static void stopHandler(int sign) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
}

static int run(UA_String *transportProfile,
               UA_NetworkAddressUrlDataType *networkAddressUrl) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    /* Details about the connection configuration and handling are located in
     * the pubsub connection tutorial */
    config->pubsubTransportLayers =
        (UA_PubSubTransportLayer *) UA_calloc(2, sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
        return -1;
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
    config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
    config->pubsubTransportLayersSize++;
#endif
    UA_Server *server = UA_Server_new(config);

    //add plugfest test nodes
    addPlugfestNodes(server);

    addPubSubConnection(server, transportProfile, networkAddressUrl);
    addPublishedDataSet(server);
    addDataSetField(server);
    addWriterGroup(server);
    addDataSetWriter(server);

    retval |= UA_Server_run(server, &running);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    return (int)retval;
}

static void
usage(char *progname) {
    printf("usage: %s <uri> [device]\n", progname);
}

int main(int argc, char **argv) {
    UA_String transportProfile =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL , UA_STRING("opc.udp://239.0.0.1:4840/")};

    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strncmp(argv[1], "opc.udp://", 10) == 0) {
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else if (strncmp(argv[1], "opc.eth://", 10) == 0) {
            transportProfile =
                UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
            if (argc < 3) {
                printf("Error: UADP/ETH needs an interface name\n");
                return 1;
            }
            networkAddressUrl.networkInterface = UA_STRING(argv[2]);
            networkAddressUrl.url = UA_STRING(argv[1]);
        } else {
            printf("Error: unknown URI\n");
            return 1;
        }
    }

    return run(&transportProfile, &networkAddressUrl);
}
