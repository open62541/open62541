/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/types.h>

#include "test_helpers.h"
#include "pubsub_test_helpers.h"
#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

#define PDS_NAME        "Config2 PDS"
#define SSDS_NAME       "Config2 SSDS"
#define CONNECTION_NAME "Config2 Connection"
#define WG_NAME         "Config2 WriterGroup"
#define DSW_NAME        "Config2 DataSetWriter"
#define RG_NAME         "Config2 ReaderGroup"
#define DSR_NAME        "Config2 DataSetReader"
#define DSR2_NAME       "Config2 DataSetReader SSDS"

#define PUBLISHER_ID    2234
#define WG_ID           100
#define DSW_ID          62541
#define DSW2_ID         62542

static UA_Server *server = NULL;

static UA_NodeId pubVarId32, pubVarId64, subVarId32, subVarId64, ssdsVarId;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* Add the variables used as publisher sources and subscriber targets */
static void
addVariables(UA_Server *srv) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_UInt32 initVal32 = 42;
    UA_Variant_setScalar(&vAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_StatusCode res =
        UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50001),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(1, "Pub UInt32"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &pubVarId32);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_UInt64 initVal64 = 43;
    UA_Variant_setScalar(&vAttr.value, &initVal64, &UA_TYPES[UA_TYPES_UINT64]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    res = UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50002),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "Pub UInt64"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &pubVarId64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant_setScalar(&vAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    res = UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50003),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "Sub UInt32"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &subVarId32);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant_setScalar(&vAttr.value, &initVal64, &UA_TYPES[UA_TYPES_UINT64]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT64].typeId;
    res = UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50004),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "Sub UInt64"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &subVarId64);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_Variant_setScalar(&vAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    res = UA_Server_addVariableNode(srv, UA_NODEID_NUMERIC(1, 50005),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, "SSDS UInt32"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &ssdsVarId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

/* Create the same variables on a second server without overwriting the
 * global NodeIds that refer to the primary server */
static void
addVariablesKeepIds(UA_Server *srv) {
    UA_NodeId keep32 = pubVarId32, keep64 = pubVarId64,
        keepS32 = subVarId32, keepS64 = subVarId64, keepSsds = ssdsVarId;
    addVariables(srv);
    pubVarId32 = keep32; pubVarId64 = keep64;
    subVarId32 = keepS32; subVarId64 = keepS64; ssdsVarId = keepSsds;
}

static void
fillMetaData2Fields(UA_DataSetMetaDataType *md) {
    UA_DataSetMetaDataType_init(md);
    md->name = UA_STRING(PDS_NAME);
    md->fieldsSize = 2;
    md->fields = (UA_FieldMetaData*)
        UA_Array_new(md->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    UA_FieldMetaData_init(&md->fields[0]);
    md->fields[0].name = UA_STRING("UInt32 Field");
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId, &md->fields[0].dataType);
    md->fields[0].builtInType = UA_NS0ID_UINT32;
    md->fields[0].valueRank = -1;
    UA_FieldMetaData_init(&md->fields[1]);
    md->fields[1].name = UA_STRING("UInt64 Field");
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT64].typeId, &md->fields[1].dataType);
    md->fields[1].builtInType = UA_NS0ID_UINT64;
    md->fields[1].valueRank = -1;
}

/* Build a full PubSub configuration via the C API: 1 connection, 1 WG,
 * 1 DSW, 1 PDS (2 fields), 1 RG, 1 DSR (inline TargetVariables),
 * 1 SSDS + 1 DSR linked to the SSDS by name. */
/* The export writes the configured enabled flag (UA_*Config.enabled, set at
 * creation or load), not the runtime state. Mark a connection subtree that
 * was enabled at runtime as configured enabled. */
static void
markConfiguredEnabled(UA_PubSubConnection *c) {
    ck_assert_ptr_nonnull(c);
    c->config.enabled = true;
    UA_WriterGroup *wg;
    LIST_FOREACH(wg, &c->writerGroups, listEntry) {
        wg->config.enabled = true;
        UA_DataSetWriter *dsw;
        LIST_FOREACH(dsw, &wg->writers, listEntry)
            dsw->config.enabled = true;
    }
    UA_ReaderGroup *rg;
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        rg->config.enabled = true;
        UA_DataSetReader *dsr;
        LIST_FOREACH(dsr, &rg->readers, listEntry)
            dsr->config.enabled = true;
    }
}

static void
buildFullConfig(UA_Server *srv, UA_NodeId *connId) {
    addVariables(srv);

    /* PublishedDataSet with two fields, a folder path and an extension
     * field */
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.name = UA_STRING(PDS_NAME);
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    UA_String pdsFolder[2] = {UA_STRING_STATIC("Fixtures"),
                              UA_STRING_STATIC("Config2")};
    pdsConfig.dataSetFolder = pdsFolder;
    pdsConfig.dataSetFolderSize = 2;
    UA_KeyValuePair pdsExtension;
    pdsExtension.key = UA_QUALIFIEDNAME(0, "ExtensionField1");
    UA_UInt32 extensionValue = 7;
    UA_Variant_setScalar(&pdsExtension.value, &extensionValue,
                         &UA_TYPES[UA_TYPES_UINT32]);
    pdsConfig.extensionFields.map = &pdsExtension;
    pdsConfig.extensionFields.mapSize = 1;
    UA_NodeId pdsId;
    UA_AddPublishedDataSetResult pdsRes =
        UA_Server_addPublishedDataSet(srv, &pdsConfig, &pdsId);
    ck_assert_int_eq(pdsRes.addResult, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(fieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("UInt32 Field");
    fieldConfig.field.variable.promotedField = false;
    fieldConfig.field.variable.publishParameters.publishedVariable = pubVarId32;
    fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataSetFieldResult fieldRes =
        UA_Server_addDataSetField(srv, pdsId, &fieldConfig, NULL);
    ck_assert_int_eq(fieldRes.result, UA_STATUSCODE_GOOD);

    fieldConfig.field.variable.fieldNameAlias = UA_STRING("UInt64 Field");
    fieldConfig.field.variable.publishParameters.publishedVariable = pubVarId64;
    fieldRes = UA_Server_addDataSetField(srv, pdsId, &fieldConfig, NULL);
    ck_assert_int_eq(fieldRes.result, UA_STATUSCODE_GOOD);

    /* Connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING(CONNECTION_NAME);
    UA_NetworkAddressUrlDataType networkAddressUrl =
        UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4801);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;
    UA_StatusCode res =
        UA_Server_addPubSubConnection(srv, &connectionConfig, connId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* WriterGroup */
    UA_WriterGroupConfig wgConfig;
    memset(&wgConfig, 0, sizeof(wgConfig));
    wgConfig.name = UA_STRING(WG_NAME);
    wgConfig.writerGroupId = WG_ID;
    wgConfig.publishingInterval = 100.0;
    wgConfig.keepAliveTime = 5000.0;
    wgConfig.priority = 10;
    wgConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    wgConfig.maxNetworkMessageSize = 1400;
    wgConfig.headerLayoutUri = UA_STRING("http://opcfoundation.org/UA/PubSub-Layouts/UADP-Cyclic-Fixed");
    UA_String wgLocales[1] = {UA_STRING_STATIC("en-US")};
    wgConfig.localeIds = wgLocales;
    wgConfig.localeIdsSize = 1;
    UA_UadpWriterGroupMessageDataType wgMessage;
    UA_UadpWriterGroupMessageDataType_init(&wgMessage);
    wgMessage.networkMessageContentMask =
        (UA_UadpNetworkMessageContentMask)
        (UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
         UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
         UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
         UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
    UA_ExtensionObject_setValueNoDelete(&wgConfig.messageSettings, &wgMessage,
        &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]);
    UA_NodeId wgId;
    res = UA_Server_addWriterGroup(srv, *connId, &wgConfig, &wgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* DataSetWriter. The dataSetName is intentionally NOT set in the config:
     * the export must fall back to the name of the connected PDS. */
    UA_DataSetWriterConfig dswConfig;
    memset(&dswConfig, 0, sizeof(dswConfig));
    dswConfig.name = UA_STRING(DSW_NAME);
    dswConfig.dataSetWriterId = DSW_ID;
    dswConfig.keyFrameCount = 10;
    dswConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_NONE;
    UA_NodeId dswId;
    res = UA_Server_addDataSetWriter(srv, wgId, pdsId, &dswConfig, &dswId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* ReaderGroup */
    UA_ReaderGroupConfig rgConfig;
    memset(&rgConfig, 0, sizeof(rgConfig));
    rgConfig.name = UA_STRING(RG_NAME);
    rgConfig.maxNetworkMessageSize = 1400;
    UA_NodeId rgId;
    res = UA_Server_addReaderGroup(srv, *connId, &rgConfig, &rgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* DataSetReader with inline TargetVariables */
    UA_DataSetReaderConfig dsrConfig;
    memset(&dsrConfig, 0, sizeof(dsrConfig));
    dsrConfig.name = UA_STRING(DSR_NAME);
    dsrConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    dsrConfig.publisherId.id.uint16 = PUBLISHER_ID;
    dsrConfig.writerGroupId = WG_ID;
    dsrConfig.dataSetWriterId = DSW_ID;
    dsrConfig.messageReceiveTimeout = 400.0;
    dsrConfig.keyFrameCount = 10;
    dsrConfig.headerLayoutUri = UA_STRING("http://opcfoundation.org/UA/PubSub-Layouts/UADP-Cyclic-Fixed");
    UA_KeyValuePair dsrProperty;
    dsrProperty.key = UA_QUALIFIEDNAME(0, "ReaderProp1");
    UA_UInt32 dsrPropertyValue = 11;
    UA_Variant_setScalar(&dsrProperty.value, &dsrPropertyValue,
                         &UA_TYPES[UA_TYPES_UINT32]);
    dsrConfig.dataSetReaderProperties.map = &dsrProperty;
    dsrConfig.dataSetReaderProperties.mapSize = 1;
    fillMetaData2Fields(&dsrConfig.dataSetMetaData);
    UA_FieldTargetDataType targets[2];
    UA_FieldTargetDataType_init(&targets[0]);
    targets[0].attributeId = UA_ATTRIBUTEID_VALUE;
    targets[0].targetNodeId = subVarId32;
    UA_FieldTargetDataType_init(&targets[1]);
    targets[1].attributeId = UA_ATTRIBUTEID_VALUE;
    targets[1].targetNodeId = subVarId64;
    dsrConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    dsrConfig.subscribedDataSet.target.targetVariablesSize = 2;
    dsrConfig.subscribedDataSet.target.targetVariables = targets;
    UA_NodeId dsrId;
    res = UA_Server_addDataSetReader(srv, rgId, &dsrConfig, &dsrId);
    /* Shallow free: the FieldMetaData members are static literals and
     * numeric NodeIds */
    UA_free(dsrConfig.dataSetMetaData.fields);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Standalone SubscribedDataSet */
    UA_SubscribedDataSetConfig ssdsConfig;
    memset(&ssdsConfig, 0, sizeof(ssdsConfig));
    ssdsConfig.name = UA_STRING(SSDS_NAME);
    ssdsConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    UA_String ssdsFolder[1] = {UA_STRING_STATIC("Config2")};
    ssdsConfig.dataSetFolder = ssdsFolder;
    ssdsConfig.dataSetFolderSize = 1;
    UA_DataSetMetaDataType *md = &ssdsConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init(md);
    md->name = UA_STRING(SSDS_NAME);
    md->fieldsSize = 1;
    md->fields = (UA_FieldMetaData*)
        UA_Array_new(md->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    UA_FieldMetaData_init(&md->fields[0]);
    md->fields[0].name = UA_STRING("SSDS UInt32 Field");
    UA_NodeId_copy(&UA_TYPES[UA_TYPES_UINT32].typeId, &md->fields[0].dataType);
    md->fields[0].builtInType = UA_NS0ID_UINT32;
    md->fields[0].valueRank = -1;
    UA_FieldTargetDataType ssdsTarget;
    UA_FieldTargetDataType_init(&ssdsTarget);
    ssdsTarget.attributeId = UA_ATTRIBUTEID_VALUE;
    ssdsTarget.targetNodeId = ssdsVarId;
    ssdsConfig.subscribedDataSet.target.targetVariablesSize = 1;
    ssdsConfig.subscribedDataSet.target.targetVariables = &ssdsTarget;
    UA_NodeId ssdsId;
    res = UA_Server_addSubscribedDataSet(srv, &ssdsConfig, &ssdsId);
    UA_free(md->fields);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Second DataSetReader linked to the SSDS by name */
    UA_DataSetReaderConfig dsr2Config;
    memset(&dsr2Config, 0, sizeof(dsr2Config));
    dsr2Config.name = UA_STRING(DSR2_NAME);
    dsr2Config.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    dsr2Config.publisherId.id.uint16 = PUBLISHER_ID;
    dsr2Config.writerGroupId = WG_ID;
    dsr2Config.dataSetWriterId = DSW2_ID;
    fillMetaData2Fields(&dsr2Config.dataSetMetaData);
    dsr2Config.dataSetMetaData.fieldsSize = 1; /* match the SSDS */
    dsr2Config.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    dsr2Config.linkedStandaloneSubscribedDataSetName = UA_STRING(SSDS_NAME);
    UA_NodeId dsr2Id;
    res = UA_Server_addDataSetReader(srv, rgId, &dsr2Config, &dsr2Id);
    UA_free(dsr2Config.dataSetMetaData.fields);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
}

/* Compare the important fields of two configuration snapshots. The
 * configurationVersion is excluded (differs by design between export and
 * re-import). */
static void
compareConfig2(const UA_PubSubConfiguration2DataType *a,
               const UA_PubSubConfiguration2DataType *b) {
    ck_assert_uint_eq(a->connectionsSize, b->connectionsSize);
    ck_assert_uint_eq(a->publishedDataSetsSize, b->publishedDataSetsSize);
    ck_assert_uint_eq(a->subscribedDataSetsSize, b->subscribedDataSetsSize);
    ck_assert(a->enabled == b->enabled);

    for(size_t i = 0; i < a->publishedDataSetsSize; i++) {
        const UA_PublishedDataSetDataType *pa = &a->publishedDataSets[i];
        const UA_PublishedDataSetDataType *pb = &b->publishedDataSets[i];
        ck_assert(UA_String_equal(&pa->name, &pb->name));
        ck_assert_uint_eq(pa->dataSetMetaData.fieldsSize,
                          pb->dataSetMetaData.fieldsSize);
        for(size_t j = 0; j < pa->dataSetMetaData.fieldsSize; j++) {
            ck_assert(UA_String_equal(&pa->dataSetMetaData.fields[j].name,
                                      &pb->dataSetMetaData.fields[j].name));
        }
        ck_assert(UA_order(&pa->dataSetSource, &pb->dataSetSource,
                           &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) == UA_ORDER_EQ);
        ck_assert_uint_eq(pa->dataSetFolderSize, pb->dataSetFolderSize);
        for(size_t j = 0; j < pa->dataSetFolderSize; j++)
            ck_assert(UA_String_equal(&pa->dataSetFolder[j], &pb->dataSetFolder[j]));
        ck_assert_uint_eq(pa->extensionFieldsSize, pb->extensionFieldsSize);
        for(size_t j = 0; j < pa->extensionFieldsSize; j++) {
            ck_assert(UA_order(&pa->extensionFields[j], &pb->extensionFields[j],
                               &UA_TYPES[UA_TYPES_KEYVALUEPAIR]) == UA_ORDER_EQ);
        }
    }

    for(size_t i = 0; i < a->connectionsSize; i++) {
        const UA_PubSubConnectionDataType *ca = &a->connections[i];
        const UA_PubSubConnectionDataType *cb = &b->connections[i];
        ck_assert(UA_String_equal(&ca->name, &cb->name));
        ck_assert(ca->enabled == cb->enabled);
        ck_assert(UA_String_equal(&ca->transportProfileUri, &cb->transportProfileUri));
        ck_assert(UA_order(&ca->publisherId, &cb->publisherId,
                           &UA_TYPES[UA_TYPES_VARIANT]) == UA_ORDER_EQ);
        ck_assert(UA_order(&ca->address, &cb->address,
                           &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) == UA_ORDER_EQ);

        ck_assert_uint_eq(ca->writerGroupsSize, cb->writerGroupsSize);
        for(size_t j = 0; j < ca->writerGroupsSize; j++) {
            const UA_WriterGroupDataType *wa = &ca->writerGroups[j];
            const UA_WriterGroupDataType *wb = &cb->writerGroups[j];
            ck_assert(UA_String_equal(&wa->name, &wb->name));
            ck_assert(wa->enabled == wb->enabled);
            ck_assert_uint_eq(wa->writerGroupId, wb->writerGroupId);
            ck_assert(wa->publishingInterval == wb->publishingInterval);
            ck_assert(wa->keepAliveTime == wb->keepAliveTime);
            ck_assert_uint_eq(wa->priority, wb->priority);
            ck_assert(UA_order(&wa->messageSettings, &wb->messageSettings,
                               &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) == UA_ORDER_EQ);
            ck_assert_uint_eq(wa->maxNetworkMessageSize, wb->maxNetworkMessageSize);
            ck_assert(UA_String_equal(&wa->headerLayoutUri, &wb->headerLayoutUri));
            ck_assert_uint_eq(wa->localeIdsSize, wb->localeIdsSize);
            for(size_t k = 0; k < wa->localeIdsSize; k++)
                ck_assert(UA_String_equal(&wa->localeIds[k], &wb->localeIds[k]));

            ck_assert_uint_eq(wa->dataSetWritersSize, wb->dataSetWritersSize);
            for(size_t k = 0; k < wa->dataSetWritersSize; k++) {
                const UA_DataSetWriterDataType *da = &wa->dataSetWriters[k];
                const UA_DataSetWriterDataType *db = &wb->dataSetWriters[k];
                ck_assert(UA_String_equal(&da->name, &db->name));
                ck_assert(da->enabled == db->enabled);
                ck_assert_uint_eq(da->dataSetWriterId, db->dataSetWriterId);
                ck_assert_uint_eq(da->keyFrameCount, db->keyFrameCount);
                ck_assert_uint_eq(da->dataSetFieldContentMask,
                                  db->dataSetFieldContentMask);
                ck_assert(UA_String_equal(&da->dataSetName, &db->dataSetName));
            }
        }

        ck_assert_uint_eq(ca->readerGroupsSize, cb->readerGroupsSize);
        for(size_t j = 0; j < ca->readerGroupsSize; j++) {
            const UA_ReaderGroupDataType *ra = &ca->readerGroups[j];
            const UA_ReaderGroupDataType *rb = &cb->readerGroups[j];
            ck_assert(UA_String_equal(&ra->name, &rb->name));
            ck_assert(ra->enabled == rb->enabled);
            ck_assert_uint_eq(ra->maxNetworkMessageSize, rb->maxNetworkMessageSize);

            ck_assert_uint_eq(ra->dataSetReadersSize, rb->dataSetReadersSize);
            for(size_t k = 0; k < ra->dataSetReadersSize; k++) {
                const UA_DataSetReaderDataType *da = &ra->dataSetReaders[k];
                const UA_DataSetReaderDataType *db = &rb->dataSetReaders[k];
                ck_assert(UA_String_equal(&da->name, &db->name));
                ck_assert(da->enabled == db->enabled);
                ck_assert(UA_order(&da->publisherId, &db->publisherId,
                                   &UA_TYPES[UA_TYPES_VARIANT]) == UA_ORDER_EQ);
                ck_assert_uint_eq(da->writerGroupId, db->writerGroupId);
                ck_assert_uint_eq(da->dataSetWriterId, db->dataSetWriterId);
                ck_assert(da->messageReceiveTimeout == db->messageReceiveTimeout);
                ck_assert_uint_eq(da->dataSetMetaData.fieldsSize,
                                  db->dataSetMetaData.fieldsSize);
                ck_assert(UA_order(&da->subscribedDataSet, &db->subscribedDataSet,
                                   &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) == UA_ORDER_EQ);
                ck_assert_uint_eq(da->keyFrameCount, db->keyFrameCount);
                ck_assert(UA_String_equal(&da->headerLayoutUri, &db->headerLayoutUri));
                ck_assert_int_eq((int)da->securityMode, (int)db->securityMode);
                ck_assert(UA_String_equal(&da->securityGroupId, &db->securityGroupId));
                ck_assert_uint_eq(da->dataSetReaderPropertiesSize,
                                  db->dataSetReaderPropertiesSize);
                for(size_t m = 0; m < da->dataSetReaderPropertiesSize; m++) {
                    ck_assert(UA_order(&da->dataSetReaderProperties[m],
                                       &db->dataSetReaderProperties[m],
                                       &UA_TYPES[UA_TYPES_KEYVALUEPAIR]) == UA_ORDER_EQ);
                }
            }
        }
    }

    for(size_t i = 0; i < a->subscribedDataSetsSize; i++) {
        const UA_StandaloneSubscribedDataSetDataType *sa = &a->subscribedDataSets[i];
        const UA_StandaloneSubscribedDataSetDataType *sb = &b->subscribedDataSets[i];
        ck_assert(UA_String_equal(&sa->name, &sb->name));
        ck_assert_uint_eq(sa->dataSetMetaData.fieldsSize,
                          sb->dataSetMetaData.fieldsSize);
        ck_assert(UA_order(&sa->subscribedDataSet, &sb->subscribedDataSet,
                           &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) == UA_ORDER_EQ);
        ck_assert_uint_eq(sa->dataSetFolderSize, sb->dataSetFolderSize);
        for(size_t j = 0; j < sa->dataSetFolderSize; j++)
            ck_assert(UA_String_equal(&sa->dataSetFolder[j], &sb->dataSetFolder[j]));
    }
}

/* Extract the Config2 body from an encoded configuration file */
static void
decodeConfig2File(const UA_ByteString *buffer,
                  UA_PubSubConfiguration2DataType *config) {
    UA_ExtensionObject eo;
    size_t offset = 0;
    UA_StatusCode res = UA_ExtensionObject_decodeBinary(buffer, &offset, &eo);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq((int)eo.encoding, (int)UA_EXTENSIONOBJECT_DECODED);
    ck_assert(eo.content.decoded.type == &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE]);
    UA_UABinaryFileDataType *binFile =
        (UA_UABinaryFileDataType*)eo.content.decoded.data;
    ck_assert(UA_Variant_hasScalarType(&binFile->body,
        &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATION2DATATYPE]));
    res = UA_PubSubConfiguration2DataType_copy(
        (UA_PubSubConfiguration2DataType*)binFile->body.data, config);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_ExtensionObject_clear(&eo);
}

/* Snapshot of the running config via UA_Server_getPubSubConfig2 */
START_TEST(GetPubSubConfig2) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);

    UA_PubSubConfiguration2DataType cfg;
    UA_StatusCode res = UA_Server_getPubSubConfig2(server, &cfg);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    ck_assert_uint_eq(cfg.connectionsSize, 1);
    ck_assert_uint_eq(cfg.publishedDataSetsSize, 1);
    ck_assert_uint_eq(cfg.subscribedDataSetsSize, 1);

    UA_String tmp = UA_STRING(CONNECTION_NAME);
    ck_assert(UA_String_equal(&cfg.connections[0].name, &tmp));
    ck_assert_uint_eq(cfg.connections[0].writerGroupsSize, 1);
    ck_assert_uint_eq(cfg.connections[0].readerGroupsSize, 1);
    ck_assert_uint_eq(cfg.connections[0].writerGroups[0].dataSetWritersSize, 1);
    ck_assert_uint_eq(cfg.connections[0].readerGroups[0].dataSetReadersSize, 2);

    /* All components created disabled. The top-level enabled flag mirrors
     * the PubSubManager lifecycle, which is started with the server. */
    ck_assert(cfg.enabled);
    ck_assert(!cfg.connections[0].enabled);
    ck_assert(!cfg.connections[0].writerGroups[0].enabled);

    /* The exported DSW dataSetName is taken from the connected PDS */
    tmp = UA_STRING(PDS_NAME);
    UA_DataSetWriterDataType *dsw =
        &cfg.connections[0].writerGroups[0].dataSetWriters[0];
    ck_assert(UA_String_equal(&dsw->dataSetName, &tmp));

    /* The PDS exports its two published variables */
    UA_PublishedDataSetDataType *pds = &cfg.publishedDataSets[0];
    ck_assert_int_eq((int)pds->dataSetSource.encoding,
                     (int)UA_EXTENSIONOBJECT_DECODED);
    ck_assert(pds->dataSetSource.content.decoded.type ==
              &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);
    UA_PublishedDataItemsDataType *pdi = (UA_PublishedDataItemsDataType*)
        pds->dataSetSource.content.decoded.data;
    ck_assert_uint_eq(pdi->publishedDataSize, 2);
    ck_assert(UA_NodeId_equal(&pdi->publishedData[0].publishedVariable,
                              &pubVarId32));
    ck_assert(UA_NodeId_equal(&pdi->publishedData[1].publishedVariable,
                              &pubVarId64));

    /* The second DSR references the SSDS by name */
    UA_DataSetReaderDataType *dsr2 =
        &cfg.connections[0].readerGroups[0].dataSetReaders[1];
    ck_assert(dsr2->subscribedDataSet.encoding == UA_EXTENSIONOBJECT_DECODED);
    ck_assert(dsr2->subscribedDataSet.content.decoded.type ==
              &UA_TYPES[UA_TYPES_STANDALONESUBSCRIBEDDATASETREFDATATYPE]);
    UA_StandaloneSubscribedDataSetRefDataType *ref =
        (UA_StandaloneSubscribedDataSetRefDataType*)
        dsr2->subscribedDataSet.content.decoded.data;
    tmp = UA_STRING(SSDS_NAME);
    ck_assert(UA_String_equal(&ref->dataSetName, &tmp));

    UA_PubSubConfiguration2DataType_clear(&cfg);
} END_TEST

/* Build via API on server A, export, load on server B, compare the
 * configurations and the resulting component states */
START_TEST(ExportImportRoundTrip) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);

    UA_StatusCode res = UA_Server_enableAllPubSubComponents(server);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_PubSubConnection *conn;
    TAILQ_FOREACH(conn, &getPSM(server)->connections, listEntry)
        markConfiguredEnabled(conn);

    UA_ByteString exportA = UA_BYTESTRING_NULL;
    res = UA_Server_writePubSubConfigurationToByteString(server, &exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(exportA.length > 0);

    /* Load into a fresh server B */
    UA_Server *serverB = UA_Server_newForUnitTest();
    ck_assert(serverB != NULL);
    UA_Server_run_startup(serverB);
    /* The target/published variables must exist on B as well */
    addVariablesKeepIds(serverB);

    res = UA_Server_loadPubSubConfigFromByteString(serverB, exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Compare the snapshots of A and B */
    UA_PubSubConfiguration2DataType cfgA, cfgB;
    res = UA_Server_getPubSubConfig2(server, &cfgA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_getPubSubConfig2(serverB, &cfgB);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    compareConfig2(&cfgA, &cfgB);

    /* All components on B are enabled: the connection and writer side become
     * operational, the readers wait for the first message */
    UA_PubSubManager *psmB = getPSM(serverB);
    ck_assert(psmB != NULL);
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psmB->connections, listEntry) {
        ck_assert(UA_PubSubState_isEnabled(c->head.state));
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            ck_assert(UA_PubSubState_isEnabled(wg->head.state));
        }
        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            ck_assert(UA_PubSubState_isEnabled(rg->head.state));
        }
    }

    /* Export B and compare the decoded files (lossless round trip). The
     * configurationVersion differs by design. */
    UA_ByteString exportB = UA_BYTESTRING_NULL;
    res = UA_Server_writePubSubConfigurationToByteString(serverB, &exportB);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubConfiguration2DataType fileA, fileB;
    decodeConfig2File(&exportA, &fileA);
    decodeConfig2File(&exportB, &fileB);
    fileA.configurationVersion = 0;
    fileB.configurationVersion = 0;
    compareConfig2(&fileA, &fileB);

    UA_PubSubConfiguration2DataType_clear(&fileA);
    UA_PubSubConfiguration2DataType_clear(&fileB);
    UA_PubSubConfiguration2DataType_clear(&cfgA);
    UA_PubSubConfiguration2DataType_clear(&cfgB);
    UA_ByteString_clear(&exportA);
    UA_ByteString_clear(&exportB);
    UA_Server_run_shutdown(serverB);
    UA_Server_delete(serverB);
} END_TEST

/* A disabled connection in the file must stay disabled after the load while
 * enabled siblings become operational */
START_TEST(MixedEnabledFlags) {
    UA_NodeId connId;
    buildFullConfig(server, &connId);

    /* Second, disabled connection */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING("Disabled Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        UA_PUBSUB_TEST_NETWORKADDRESSURL(UA_PUBSUB_TEST_UDP_MULTICAST_URL_4840);
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID + 1;
    UA_NodeId conn2Id;
    UA_StatusCode res =
        UA_Server_addPubSubConnection(server, &connectionConfig, &conn2Id);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Enable only the first connection subtree */
    res = UA_Server_enablePubSubConnection(server, connId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    markConfiguredEnabled(UA_PubSubConnection_find(getPSM(server), connId));

    UA_ByteString exportA = UA_BYTESTRING_NULL;
    res = UA_Server_writePubSubConfigurationToByteString(server, &exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_Server *serverB = UA_Server_newForUnitTest();
    ck_assert(serverB != NULL);
    UA_Server_run_startup(serverB);
    addVariablesKeepIds(serverB);

    res = UA_Server_loadPubSubConfigFromByteString(serverB, exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psmB = getPSM(serverB);
    UA_String nameEnabled = UA_STRING(CONNECTION_NAME);
    UA_String nameDisabled = UA_STRING("Disabled Connection");
    size_t found = 0;
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psmB->connections, listEntry) {
        if(UA_String_equal(&c->config.name, &nameEnabled)) {
            ck_assert(UA_PubSubState_isEnabled(c->head.state));
            found++;
        } else if(UA_String_equal(&c->config.name, &nameDisabled)) {
            ck_assert_int_eq((int)c->head.state, (int)UA_PUBSUBSTATE_DISABLED);
            found++;
        }
    }
    ck_assert_uint_eq(found, 2);

    UA_ByteString_clear(&exportA);
    UA_Server_run_shutdown(serverB);
    UA_Server_delete(serverB);
} END_TEST

/* Namespace indices in the file body must be remapped to the server
 * NamespaceArray on load; unknown namespaces are added */
START_TEST(NamespaceRemapOnLoad) {
    /* A namespace only known to server A, used in the published variable */
    UA_UInt16 nsA = UA_Server_addNamespace(server, "http://config2.test/nsA");

    UA_NodeId connId;
    buildFullConfig(server, &connId);

    /* Add a variable in the new namespace and publish it */
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_UInt32 initVal32 = 42;
    UA_Variant_setScalar(&vAttr.value, &initVal32, &UA_TYPES[UA_TYPES_UINT32]);
    vAttr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    UA_NodeId nsVarId;
    UA_StatusCode res =
        UA_Server_addVariableNode(server, UA_NODEID_NUMERIC(nsA, 60001),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                  UA_QUALIFIEDNAME(nsA, "NS Var"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vAttr, NULL, &nsVarId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.name = UA_STRING("NS PDS");
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    UA_NodeId pdsId;
    UA_AddPublishedDataSetResult pdsRes =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId);
    ck_assert_int_eq(pdsRes.addResult, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig fieldConfig;
    memset(&fieldConfig, 0, sizeof(fieldConfig));
    fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    fieldConfig.field.variable.fieldNameAlias = UA_STRING("NS Field");
    fieldConfig.field.variable.publishParameters.publishedVariable = nsVarId;
    fieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataSetFieldResult fieldRes =
        UA_Server_addDataSetField(server, pdsId, &fieldConfig, NULL);
    ck_assert_int_eq(fieldRes.result, UA_STATUSCODE_GOOD);

    UA_ByteString exportA = UA_BYTESTRING_NULL;
    res = UA_Server_writePubSubConfigurationToByteString(server, &exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* Server B gets a different namespace first, so the loaded namespace
     * ends up at a different index */
    UA_Server *serverB = UA_Server_newForUnitTest();
    ck_assert(serverB != NULL);
    UA_Server_run_startup(serverB);
    UA_Server_addNamespace(serverB, "http://config2.test/other");
    addVariablesKeepIds(serverB);

    /* The published variable must exist on B in the (differently indexed)
     * namespace. Adding the namespace here also checks that the loader maps
     * onto an existing entry instead of duplicating it. */
    UA_UInt16 nsB = UA_Server_addNamespace(serverB, "http://config2.test/nsA");
    ck_assert_uint_ne(nsB, nsA);
    UA_NodeId nsVarIdB;
    res = UA_Server_addVariableNode(serverB, UA_NODEID_NUMERIC(nsB, 60001),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(nsB, "NS Var"),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    vAttr, NULL, &nsVarIdB);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_loadPubSubConfigFromByteString(serverB, exportA);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    /* The namespace was added to server B */
    size_t nsBIndex = 0;
    res = getNamespaceByName(serverB, UA_STRING("http://config2.test/nsA"),
                             &nsBIndex);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_ne(nsBIndex, nsA); /* different index than on server A */

    /* The published variable NodeId was remapped to the new index */
    UA_PubSubConfiguration2DataType cfgB;
    res = UA_Server_getPubSubConfig2(serverB, &cfgB);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_String pdsName = UA_STRING("NS PDS");
    UA_Boolean foundPds = false;
    for(size_t i = 0; i < cfgB.publishedDataSetsSize; i++) {
        UA_PublishedDataSetDataType *pds = &cfgB.publishedDataSets[i];
        if(!UA_String_equal(&pds->name, &pdsName))
            continue;
        foundPds = true;
        UA_PublishedDataItemsDataType *pdi = (UA_PublishedDataItemsDataType*)
            pds->dataSetSource.content.decoded.data;
        ck_assert_uint_eq(pdi->publishedDataSize, 1);
        ck_assert_uint_eq(pdi->publishedData[0].publishedVariable.namespaceIndex,
                          (UA_UInt16)nsBIndex);
    }
    ck_assert(foundPds);

    UA_PubSubConfiguration2DataType_clear(&cfgB);
    UA_ByteString_clear(&exportA);
    UA_Server_run_shutdown(serverB);
    UA_Server_delete(serverB);
} END_TEST

/* Malformed input must be rejected without touching the configuration */
START_TEST(InvalidFileBody) {
    /* Garbage buffer */
    UA_ByteString garbage = UA_BYTESTRING("this is not a pubsub config");
    UA_StatusCode res = UA_Server_loadPubSubConfigFromByteString(server, garbage);
    ck_assert_int_ne(res, UA_STATUSCODE_GOOD);

    /* UABinaryFileDataType with a wrong body type */
    UA_UABinaryFileDataType binFile;
    UA_UABinaryFileDataType_init(&binFile);
    UA_Int32 wrongBody = 42;
    UA_Variant_setScalar(&binFile.body, &wrongBody, &UA_TYPES[UA_TYPES_INT32]);

    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    eo.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    eo.content.decoded.type = &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE];
    eo.content.decoded.data = &binFile;

    UA_ByteString buf = UA_BYTESTRING_NULL;
    res = UA_encodeBinary(&eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], &buf, NULL);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_loadPubSubConfigFromByteString(server, buf);
    ck_assert_int_eq(res, UA_STATUSCODE_BADTYPEMISMATCH);

    /* Nothing was created */
    UA_PubSubManager *psm = getPSM(server);
    ck_assert_uint_eq(psm->connectionsSize, 0);

    UA_ByteString_clear(&buf);
} END_TEST

int main(void) {
    TCase *tc_config2 = tcase_create("PubSubConfiguration2");
    tcase_add_checked_fixture(tc_config2, setup, teardown);
    tcase_add_test(tc_config2, GetPubSubConfig2);
    tcase_add_test(tc_config2, ExportImportRoundTrip);
    tcase_add_test(tc_config2, MixedEnabledFlags);
    tcase_add_test(tc_config2, NamespaceRemapOnLoad);
    tcase_add_test(tc_config2, InvalidFileBody);

    Suite *s = suite_create("PubSub Configuration2 export/import");
    suite_add_tcase(s, tc_config2);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
