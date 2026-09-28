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
#include "pubsub_config_fixtures.h"

#include <check.h>
#include <stdlib.h>

static UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

/* BaseConn with BaseWG + BaseDSW (BasePDS) and BaseRG */
static UA_NodeId baseConnId, baseWgId, baseRgId, basePdsId;

static void
buildBaseConfig(void) {
    UA_PubSubTestBase base;
    UA_PubSubTest_addBaseConfig(server, "Base", 51001, 100.0, true, &base);
    baseConnId = base.connId;
    baseWgId = base.wgId;
    baseRgId = base.rgId;
    basePdsId = base.pdsId;
}

/* Add a connection, then a WriterGroup/DataSetWriter/PDS under it in a
 * second call (parent referenced by name) */
START_TEST(AddElements) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    ck_assert_uint_eq(psm->connectionsSize, 1);

    /* Add a second connection */
    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "IncConn", 2334);
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(result.changesApplied);
    ck_assert_uint_eq(result.referencesResultsSize, 1);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert(!UA_NodeId_isNull(&result.configurationObjects[0]));
    ck_assert_uint_eq(psm->connectionsSize, 2);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Add PDS + WriterGroup + DataSetWriter + ReaderGroup + DataSetReader
     * under the new connection in one call. The connection element only
     * provides the parent name. */
    UA_PubSubConfiguration2DataType cfg2;
    UA_PubSubConfiguration2DataType_init(&cfg2);

    UA_NodeId varId = UA_PubSubTest_addVariable(server, 51002, 42);
    UA_PublishedDataSetDataType pds;
    UA_PublishedDataSetDataType_init(&pds);
    pds.name = UA_STRING("IncPDS");
    UA_FieldMetaData fmd;
    UA_FieldMetaData_init(&fmd);
    fmd.name = UA_STRING("Field1");
    fmd.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    fmd.builtInType = UA_NS0ID_UINT32;
    fmd.valueRank = -1;
    pds.dataSetMetaData.name = UA_STRING("IncPDS");
    pds.dataSetMetaData.fields = &fmd;
    pds.dataSetMetaData.fieldsSize = 1;
    UA_PublishedDataItemsDataType pdi;
    UA_PublishedDataItemsDataType_init(&pdi);
    UA_PublishedVariableDataType pv;
    UA_PublishedVariableDataType_init(&pv);
    pv.publishedVariable = varId;
    pv.attributeId = UA_ATTRIBUTEID_VALUE;
    pdi.publishedData = &pv;
    pdi.publishedDataSize = 1;
    pds.dataSetSource.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    pds.dataSetSource.content.decoded.type =
        &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE];
    pds.dataSetSource.content.decoded.data = &pdi;
    cfg2.publishedDataSets = &pds;
    cfg2.publishedDataSetsSize = 1;

    UA_PubSubConnectionDataType parentConn;
    UA_PubSubConnectionDataType_init(&parentConn);
    parentConn.name = UA_STRING("IncConn");

    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "IncWG", 101);
    UA_DataSetWriterDataType dsw;
    UA_DataSetWriterDataType_init(&dsw);
    dsw.name = UA_STRING("IncDSW");
    dsw.dataSetWriterId = 201;
    dsw.dataSetName = UA_STRING("IncPDS");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    parentConn.writerGroups = &wg;
    parentConn.writerGroupsSize = 1;

    UA_ReaderGroupDataType rg;
    UA_ReaderGroupDataType_init(&rg);
    rg.name = UA_STRING("IncRG");
    UA_DataSetReaderDataType dsr;
    UA_DataSetReaderDataType_init(&dsr);
    dsr.name = UA_STRING("IncDSR");
    dsr.writerGroupId = 101;
    dsr.dataSetWriterId = 201;
    UA_Variant_setScalar(&dsr.publisherId, &UA_PubSubTest_filePublisherId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    dsr.dataSetMetaData.fields = &fmd;
    dsr.dataSetMetaData.fieldsSize = 1;
    UA_TargetVariablesDataType targets;
    UA_TargetVariablesDataType_init(&targets);
    UA_FieldTargetDataType target;
    UA_FieldTargetDataType_init(&target);
    target.attributeId = UA_ATTRIBUTEID_VALUE;
    target.targetNodeId = varId;
    targets.targetVariables = &target;
    targets.targetVariablesSize = 1;
    dsr.subscribedDataSet.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    dsr.subscribedDataSet.content.decoded.type =
        &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE];
    dsr.subscribedDataSet.content.decoded.data = &targets;
    rg.dataSetReaders = &dsr;
    rg.dataSetReadersSize = 1;
    parentConn.readerGroups = &rg;
    parentConn.readerGroupsSize = 1;

    cfg2.connections = &parentConn;
    cfg2.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType refs[5];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);
    refs[3] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP, 0, 0, 0);
    refs[4] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER, 0, 0, 0);

    res = UA_PubSubTest_updateConfig(server, &cfg2, 5, refs, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 5; i++)
        ck_assert_int_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    ck_assert(result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Verify the model */
    UA_PubSubConnection *c = NULL, *iter;
    UA_String connName = UA_STRING("IncConn");
    TAILQ_FOREACH(iter, &psm->connections, listEntry) {
        if(UA_String_equal(&iter->config.name, &connName))
            c = iter;
    }
    ck_assert(c != NULL);
    ck_assert_uint_eq(c->writerGroupsSize, 1);
    ck_assert_uint_eq(c->readerGroupsSize, 1);
    UA_WriterGroup *liveWg = LIST_FIRST(&c->writerGroups);
    ck_assert_uint_eq(liveWg->writersCount, 1);
    UA_ReaderGroup *liveRg = LIST_FIRST(&c->readerGroups);
    ck_assert_uint_eq(liveRg->readersCount, 1);
    ck_assert_uint_eq(psm->publishedDataSetsSize, 2);
} END_TEST

/* Auto-assignment of names and identifiers */
START_TEST(AutoAssign) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    /* Connection with empty name and null PublisherId */
    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "", 0);
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.configurationValuesSize, 1);
    ck_assert(result.configurationValues[0].name.length > 0);
    ck_assert(!UA_Variant_isEmpty(&result.configurationValues[0].identifier));
    UA_String assignedConnName = result.configurationValues[0].name;
    ck_assert_uint_eq(psm->connectionsSize, 2);

    /* WriterGroup with empty name and id 0 under the new connection */
    UA_PubSubConfiguration2DataType cfg2;
    UA_PubSubConfiguration2DataType_init(&cfg2);
    UA_PubSubConnectionDataType parentConn;
    UA_PubSubConnectionDataType_init(&parentConn);
    parentConn.name = assignedConnName;
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "", 0);
    parentConn.writerGroups = &wg;
    parentConn.writerGroupsSize = 1;
    cfg2.connections = &parentConn;
    cfg2.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType ref2 =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result2;
    res = UA_PubSubTest_updateConfig(server, &cfg2, 1, &ref2, false, &result2);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result2.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result2.configurationValuesSize, 1);
    UA_Variant *ident = &result2.configurationValues[0].identifier;
    ck_assert(UA_Variant_hasScalarType(ident, &UA_TYPES[UA_TYPES_UINT16]));
    ck_assert_uint_ge(*(UA_UInt16*)ident->data, 0x8000);

    UA_PubSubConfigurationUpdateResult_clear(&result);
    UA_PubSubConfigurationUpdateResult_clear(&result2);
} END_TEST

/* Mask and index validation */
START_TEST(MaskValidation) {
    buildBaseConfig();

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "X", 1);
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationUpdateResult result;

    /* Empty references */
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 0, NULL,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_BADNOTHINGTODO);

    /* Add and Modify combined */
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* No reference bit */
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Two reference bits */
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Index out of range */
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 5, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Match on a writer reference */
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* SecurityGroup reference is not supported */
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESECURITYGROUP, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0],
                     UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* Remove and re-add with the same name in one call (removes first) */
START_TEST(RemoveAndReAdd) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    ck_assert_uint_eq(psm->connectionsSize, 1);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conns[2];
    /* [0] the new element to add, [1] the remove reference (name only) */
    UA_PubSubTest_fileConnection(&conns[0], "BaseConn", 7777);
    UA_PubSubConnectionDataType_init(&conns[1]);
    conns[1].name = UA_STRING("BaseConn");
    cfg.connections = conns;
    cfg.connectionsSize = 2;

    UA_PubSubConfigurationRefDataType refs[2];
    /* The add is passed first -- the engine must process the remove first */
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 1, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->connectionsSize, 1);

    /* The children of the removed connection are gone, the new connection
     * has the new PublisherId */
    UA_PubSubConnection *c = TAILQ_FIRST(&psm->connections);
    ck_assert_uint_eq(c->writerGroupsSize, 0);
    ck_assert_uint_eq(c->readerGroupsSize, 0);
    ck_assert_uint_eq(c->config.publisherId.id.uint16, 7777);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* Modify a WriterGroup and check that the state is preserved */
START_TEST(ModifyElements) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType parentConn;
    UA_PubSubConnectionDataType_init(&parentConn);
    parentConn.name = UA_STRING("BaseConn");
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "BaseWG", 100);
    wg.publishingInterval = 250.0;
    parentConn.writerGroups = &wg;
    parentConn.writerGroupsSize = 1;
    cfg.connections = &parentConn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    UA_WriterGroup *liveWg = UA_WriterGroup_find(psm, baseWgId);
    ck_assert(liveWg != NULL);
    ck_assert(liveWg->config.publishingInterval == 250.0);
    /* The base config is disabled -- the state stays disabled */
    ck_assert_int_eq((int)liveWg->head.state, (int)UA_PUBSUBSTATE_DISABLED);

    /* Modify with an unknown name */
    wg.name = UA_STRING("MissingWG");
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADNOMATCH);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* Match operations on connections */
START_TEST(MatchElements) {
    buildBaseConfig();

    /* Matching connection: same profile and address as BaseConn */
    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.transportProfileUri = UA_STRING(UA_PUBSUBTEST_PROFILE_UDP);
    UA_NetworkAddressUrlDataType addr;
    UA_PubSubTest_initNetworkAddressUrl(&addr,
        UA_PubSubTest_getUdpMulticastUrl4801());
    conn.address.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    conn.address.content.decoded.type =
        &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE];
    conn.address.content.decoded.data = &addr;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.configurationValuesSize, 1);
    UA_String baseName = UA_STRING("BaseConn");
    ck_assert(UA_String_equal(&result.configurationValues[0].name, &baseName));
    /* A pure match does not change the configuration */
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Mismatching address */
    UA_PubSubTest_initNetworkAddressUrl(&addr, (char*)(uintptr_t)"opc.udp://224.0.0.99:9999/");
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADNOMATCH);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Match|Add: no match -> the connection is added */
    conn.name = UA_STRING("MatchAddConn");
    UA_UInt16 pubId = 4567;
    UA_Variant_setScalar(&conn.publisherId, &pubId, &UA_TYPES[UA_TYPES_UINT16]);
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                  UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert(result.changesApplied);
    UA_PubSubManager *psm = getPSM(server);
    ck_assert_uint_eq(psm->connectionsSize, 2);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* Top-level fields: ConfigurationProperties merge and version bump */
START_TEST(TopLevelFields) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_UInt32 versionBefore = psm->configurationVersion;

    /* Insert a property along with an add operation */
    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "PropConn", 3434);
    cfg.connections = &conn;
    cfg.connectionsSize = 1;
    UA_KeyValuePair prop;
    prop.key = UA_QUALIFIEDNAME(0, "Vendor");
    UA_UInt32 vendorValue = 99;
    UA_Variant_setScalar(&prop.value, &vendorValue, &UA_TYPES[UA_TYPES_UINT32]);
    cfg.configurationProperties = &prop;
    cfg.configurationPropertiesSize = 1;

    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    const UA_Variant *v = UA_KeyValueMap_get(&psm->configurationProperties,
                                             UA_QUALIFIEDNAME(0, "Vendor"));
    ck_assert(v != NULL);
    ck_assert_uint_eq(*(UA_UInt32*)v->data, 99);
    ck_assert(psm->configurationVersion != versionBefore);

    /* Delete the property with a null value. The referenced remove op keeps
     * the update valid. */
    UA_Variant_init(&prop.value);
    conn.name = UA_STRING("PropConn");
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    v = UA_KeyValueMap_get(&psm->configurationProperties,
                           UA_QUALIFIEDNAME(0, "Vendor"));
    ck_assert(v == NULL);
} END_TEST



/* Removing a standalone SubscribedDataSet removes the connected reader, also
 * when its ReaderGroup is enabled. The group state is restored. */
START_TEST(RemoveSsdsWithReaderInEnabledGroup) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId targetVar = UA_PubSubTest_addVariable(server, 51010, 42);

    UA_FieldMetaData fmd;
    UA_FieldMetaData_init(&fmd);
    fmd.name = UA_STRING("Field1");
    fmd.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    fmd.builtInType = UA_NS0ID_UINT32;
    fmd.valueRank = -1;

    UA_SubscribedDataSetConfig ssdsConfig;
    memset(&ssdsConfig, 0, sizeof(ssdsConfig));
    ssdsConfig.name = UA_STRING("S1");
    ssdsConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    ssdsConfig.dataSetMetaData.name = UA_STRING("S1");
    ssdsConfig.dataSetMetaData.fields = &fmd;
    ssdsConfig.dataSetMetaData.fieldsSize = 1;
    UA_FieldTargetDataType target;
    UA_FieldTargetDataType_init(&target);
    target.attributeId = UA_ATTRIBUTEID_VALUE;
    target.targetNodeId = targetVar;
    ssdsConfig.subscribedDataSet.target.targetVariables = &target;
    ssdsConfig.subscribedDataSet.target.targetVariablesSize = 1;
    UA_NodeId ssdsId;
    UA_StatusCode res = UA_Server_addSubscribedDataSet(server, &ssdsConfig, &ssdsId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_DataSetReaderConfig dsrConfig;
    memset(&dsrConfig, 0, sizeof(dsrConfig));
    dsrConfig.name = UA_STRING("R1");
    dsrConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    dsrConfig.publisherId.id.uint16 = 2234;
    dsrConfig.writerGroupId = 100;
    dsrConfig.dataSetWriterId = 200;
    dsrConfig.dataSetMetaData.fields = &fmd;
    dsrConfig.dataSetMetaData.fieldsSize = 1;
    dsrConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    dsrConfig.linkedStandaloneSubscribedDataSetName = UA_STRING("S1");
    UA_NodeId dsrId;
    res = UA_Server_addDataSetReader(server, baseRgId, &dsrConfig, &dsrId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    res = UA_Server_enableReaderGroup(server, baseRgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, baseRgId);
    ck_assert_ptr_nonnull(rg);
    UA_PubSubState rgState = rg->head.state;
    ck_assert(UA_PubSubState_isEnabled(rgState));

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_StandaloneSubscribedDataSetDataType ssds;
    UA_StandaloneSubscribedDataSetDataType_init(&ssds);
    ssds.name = UA_STRING("S1");
    cfg.subscribedDataSets = &ssds;
    cfg.subscribedDataSetsSize = 1;
    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESUBDATASET, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ck_assert_ptr_null(UA_SubscribedDataSet_find(psm, ssdsId));
    ck_assert_ptr_null(UA_DataSetReader_find(psm, dsrId));
    ck_assert_int_eq(rg->head.state, rgState);
} END_TEST

/* Parent binding (Part 14 9.1.3.7.3). The loop tests run without (0) and
 * with (1) requireCompleteUpdate. */

/* The references may list children before their parents. The elements are
 * processed in the order of the hierarchy. */
START_TEST(ChildrenBeforeParents) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "OrderConn", 2335);
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "OrderWG", 102);
    UA_DataSetWriterDataType dsw;
    UA_PubSubTest_fileWriter(&dsw, "OrderDSW", 202, "OrderPDS");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;
    UA_PublishedDataSetDataType pds;
    UA_PubSubTest_filePds(&pds, "OrderPDS",
                          UA_PubSubTest_addVariable(server, 51020, 42));
    cfg.publishedDataSets = &pds;
    cfg.publishedDataSetsSize = 1;

    UA_PubSubConfigurationRefDataType refs[4];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[3] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 4, refs,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 4; i++)
        ck_assert_int_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    UA_DataSetWriter *liveDsw =
        UA_DataSetWriter_find(psm, result.configurationObjects[0]);
    UA_WriterGroup *liveWg = UA_WriterGroup_find(psm, result.configurationObjects[1]);
    ck_assert_ptr_nonnull(liveDsw);
    ck_assert_ptr_nonnull(liveWg);
    ck_assert_ptr_eq(liveDsw->linkedWriterGroup, liveWg);
    ck_assert(UA_NodeId_equal(&liveWg->linkedConnection->head.identifier,
                              &result.configurationObjects[2]));
    ck_assert(UA_NodeId_equal(&liveDsw->connectedDataSet->head.identifier,
                              &result.configurationObjects[3]));
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* Children of an element with an assigned name bind to the added element */
START_TEST(AutoNamedParent) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "", 0);
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "", 0);
    UA_DataSetWriterDataType dsw;
    UA_PubSubTest_fileWriter(&dsw, "AutoDSW", 203, "BasePDS");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType refs[3];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 3, refs,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 3; i++)
        ck_assert_int_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    UA_PubSubConnection *liveConn =
        UA_PubSubConnection_find(psm, result.configurationObjects[0]);
    UA_WriterGroup *liveWg = UA_WriterGroup_find(psm, result.configurationObjects[1]);
    UA_DataSetWriter *liveDsw =
        UA_DataSetWriter_find(psm, result.configurationObjects[2]);
    ck_assert_ptr_nonnull(liveConn);
    ck_assert(liveConn->config.name.length > 0);
    ck_assert_ptr_nonnull(liveWg);
    ck_assert_ptr_eq(liveWg->linkedConnection, liveConn);
    ck_assert_ptr_nonnull(liveDsw);
    ck_assert_ptr_eq(liveDsw->linkedWriterGroup, liveWg);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* Children of a matched element are added to the matched component. With
 * Add|Match the name of the file element is only used for an added element. */
START_TEST(MatchedParent) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *baseConn = UA_PubSubConnection_find(psm, baseConnId);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "", 0); /* Matches BaseConn */
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "MatchWG", 103);
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType refs[2];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&result.configurationObjects[0], &baseConnId));
    UA_WriterGroup *liveWg = UA_WriterGroup_find(psm, result.configurationObjects[1]);
    ck_assert_ptr_nonnull(liveWg);
    ck_assert_ptr_eq(liveWg->linkedConnection, baseConn);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Add|Match with a name that does not exist uses the match */
    conn.name = UA_STRING("OtherName");
    UA_UInt16 pubId = 4444;
    UA_Variant_setScalar(&conn.publisherId, &pubId, &UA_TYPES[UA_TYPES_UINT16]);
    UA_PubSubTest_fileWriterGroup(&wg, "MatchWG2", 104);
    refs[0].configurationMask |= UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD;
    res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs, (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&result.configurationObjects[0], &baseConnId));
    liveWg = UA_WriterGroup_find(psm, result.configurationObjects[1]);
    ck_assert_ptr_nonnull(liveWg);
    ck_assert_ptr_eq(liveWg->linkedConnection, baseConn);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_uint_eq(baseConn->writerGroupsSize, 3);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* A parent whose operation failed in this call is not available */
START_TEST(FailedParentNotFound) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *baseConn = UA_PubSubConnection_find(psm, baseConnId);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "BaseConn", 9999); /* Duplicate name */
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "FailWG", 105);
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType refs[2];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0],
                     UA_STATUSCODE_BADBROWSENAMEDUPLICATED);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_BADNOTFOUND);
    ck_assert(!result.changesApplied);
    ck_assert_uint_eq(baseConn->writerGroupsSize, 1);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* A parent removed in this call is not available, also when an element with
 * the same name is added again */
START_TEST(RemovedParentNotFound) {
    buildBaseConfig();

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conns[2];
    UA_PubSubConnectionDataType_init(&conns[0]);
    conns[0].name = UA_STRING("BaseConn");
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "LateWG", 106);
    conns[0].writerGroups = &wg;
    conns[0].writerGroupsSize = 1;
    UA_PubSubTest_fileConnection(&conns[1], "BaseConn", 7778);
    cfg.connections = conns;
    cfg.connectionsSize = 2;

    UA_PubSubConfigurationRefDataType refs[3];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 1, 0);
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 3, refs,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[2], UA_STATUSCODE_BADNOTFOUND);
    ck_assert(result.changesApplied == !_i);
    UA_PubSubConnection *c = UA_PubSubTest_findConnection(server, "BaseConn");
    ck_assert_ptr_nonnull(c);
    if(_i) {
        /* Complete update: nothing applied */
        ck_assert_uint_eq(c->config.publisherId.id.uint16, 2234);
        ck_assert_uint_eq(c->writerGroupsSize, 1);
    } else {
        ck_assert_uint_eq(c->config.publisherId.id.uint16, 7778);
        ck_assert_uint_eq(c->writerGroupsSize, 0);
    }
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* An unreferenced parent element with an empty name identifies nothing */
START_TEST(EmptyParentName) {
    buildBaseConfig();

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "OrphanWG", 107);
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType ref =
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADNOTFOUND);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Remove of a WriterGroup under the unnamed connection */
    wg.name = UA_STRING("BaseWG");
    ref = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                  UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &ref, (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADNOTFOUND);
    ck_assert_ptr_nonnull(UA_WriterGroup_find(getPSM(server), baseWgId));
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* A pure Match requires a null name and Id (Part 14 9.1.3.7.2) */
START_TEST(MatchRequiresNullNameAndId) {
    buildBaseConfig();

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "BaseConn", 0);
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "", 0);
    wg.publishingInterval = 100.0;
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    UA_ReaderGroupDataType rg;
    UA_ReaderGroupDataType_init(&rg);
    conn.readerGroups = &rg;
    conn.readerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType refs[3];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP, 0, 0, 0);

    /* Connection with a name */
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 1, refs,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Connection with a PublisherId */
    conn.name = UA_STRING_NULL;
    UA_UInt16 pubId = 2234;
    UA_Variant_setScalar(&conn.publisherId, &pubId, &UA_TYPES[UA_TYPES_UINT16]);
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, refs, (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* WriterGroup with a name, then with a WriterGroupId. The parent is
     * identified by the connection name. */
    conn.name = UA_STRING("BaseConn");
    UA_Variant_init(&conn.publisherId);
    wg.name = UA_STRING("BaseWG");
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &refs[1], (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);
    wg.name = UA_STRING_NULL;
    wg.writerGroupId = 100;
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &refs[1], (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* ReaderGroup with a name */
    rg.name = UA_STRING("BaseRG");
    res = UA_PubSubTest_updateConfig(server, &cfg, 1, &refs[2], (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Null name and Id: the groups are matched under the named connection */
    wg.writerGroupId = 0;
    rg.name = UA_STRING_NULL;
    res = UA_PubSubTest_updateConfig(server, &cfg, 2, &refs[1], (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_GOOD);
    ck_assert(UA_NodeId_equal(&result.configurationObjects[0], &baseWgId));
    ck_assert(UA_NodeId_equal(&result.configurationObjects[1], &baseRgId));
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* Removes given parent-first are processed children-first */
START_TEST(RemoveParentAndChild) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.name = UA_STRING("BaseConn");
    UA_WriterGroupDataType wg;
    UA_WriterGroupDataType_init(&wg);
    wg.name = UA_STRING("BaseWG");
    UA_DataSetWriterDataType dsw;
    UA_DataSetWriterDataType_init(&dsw);
    dsw.name = UA_STRING("BaseDSW");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    UA_ReaderGroupDataType rg;
    UA_ReaderGroupDataType_init(&rg);
    rg.name = UA_STRING("BaseRG");
    conn.readerGroups = &rg;
    conn.readerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType refs[4];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);
    refs[3] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 4, refs,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 4; i++)
        ck_assert_int_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* The second remove of the same element finds nothing */
START_TEST(DoubleRemove) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.name = UA_STRING("BaseConn");
    UA_WriterGroupDataType wg;
    UA_WriterGroupDataType_init(&wg);
    wg.name = UA_STRING("BaseWG");
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType refs[2];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[1] = refs[0];

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs,
                                                      (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_BADNOMATCH);
    ck_assert((UA_WriterGroup_findByName(UA_PubSubTest_findConnection(server, "BaseConn"),
                                         UA_STRING("BaseWG")) != NULL) == _i);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = refs[0];
    res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs, (UA_Boolean)_i, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_BADNOMATCH);
    ck_assert_uint_eq(psm->connectionsSize, (_i) ? 1 : 0);
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* Complete update (Part 14 9.1.3.7.6): an invalid reference prevents all
 * changes. The rollback reports the same codes as a partial update. */
#define VALIDATION_CASES 14

START_TEST(CompleteUpdateValidation) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    /* [0] a valid new connection, [1] the base connection with a group
     * element of each type, [2] an unknown connection, [3] a connection
     * element that matches nothing */
    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conns[4];
    UA_PubSubTest_fileConnection(&conns[0], "ValidConn", 2400);
    UA_PubSubTest_fileConnection(&conns[1], "BaseConn", 0);
    UA_WriterGroupDataType wg;
    UA_WriterGroupDataType_init(&wg);
    wg.name = UA_STRING("BaseWG");
    UA_DataSetWriterDataType dsw;
    UA_PubSubTest_fileWriter(&dsw, "CaseDSW", 201, "BasePDS");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    conns[1].writerGroups = &wg;
    conns[1].writerGroupsSize = 1;
    UA_ReaderGroupDataType rg;
    UA_ReaderGroupDataType_init(&rg);
    rg.name = UA_STRING("BaseRG");
    UA_DataSetReaderDataType dsr;
    UA_DataSetReaderDataType_init(&dsr);
    dsr.name = UA_STRING("CaseDSR");
    rg.dataSetReaders = &dsr;
    rg.dataSetReadersSize = 1;
    conns[1].readerGroups = &rg;
    conns[1].readerGroupsSize = 1;
    UA_PubSubConnectionDataType_init(&conns[2]);
    conns[2].name = UA_STRING("MissingConn");
    UA_WriterGroupDataType wg2;
    UA_PubSubTest_fileWriterGroup(&wg2, "NewWG", 108);
    conns[2].writerGroups = &wg2;
    conns[2].writerGroupsSize = 1;
    UA_PubSubConnectionDataType_init(&conns[3]);
    conns[3].transportProfileUri = UA_STRING(UA_PUBSUBTEST_PROFILE_UDP);
    UA_NetworkAddressUrlDataType otherAddr;
    UA_PubSubTest_initNetworkAddressUrl(&otherAddr,
        (char*)(uintptr_t)"opc.udp://224.0.0.99:9999/");
    conns[3].address.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    conns[3].address.content.decoded.type =
        &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE];
    conns[3].address.content.decoded.data = &otherAddr;
    cfg.connections = conns;
    cfg.connectionsSize = 4;
    UA_PublishedDataSetDataType pds;
    UA_PublishedDataSetDataType_init(&pds);
    pds.name = UA_STRING("BasePDS");
    cfg.publishedDataSets = &pds;
    cfg.publishedDataSetsSize = 1;

    UA_StandaloneSubscribedDataSetRefDataType ssdsRef;
    UA_StandaloneSubscribedDataSetRefDataType_init(&ssdsRef);
    ssdsRef.dataSetName = UA_STRING("NoSSDS");
    UA_SubscribedDataSetMirrorDataType mirror;
    UA_SubscribedDataSetMirrorDataType_init(&mirror);

    const UA_UInt32 addDsw = UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
        UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER;
    const UA_UInt32 addDsr = UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
        UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER;
    UA_PubSubConfigurationRefDataType refs[3];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    size_t refsSize = 2;
    UA_StatusCode expected[3] = {UA_STATUSCODE_GOOD, UA_STATUSCODE_GOOD,
                                 UA_STATUSCODE_GOOD};
    switch(_i) {
    case 0: /* Duplicate connection name */
        refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 1, 0);
        expected[1] = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        break;
    case 1: /* Duplicate name of two elements added in the call */
        refs[1] = refs[0];
        expected[1] = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        break;
    case 2: /* Unknown parent connection */
        refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 2, 0);
        expected[1] = UA_STATUSCODE_BADNOTFOUND;
        break;
    case 3: /* Parent removed in the call */
        refs[1] = UA_PubSubTest_ref(addDsw, 0, 1, 0);
        refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 1, 0);
        refsSize = 3;
        expected[1] = UA_STATUSCODE_BADNOTFOUND;
        break;
    case 4: /* Unknown PublishedDataSet */
        dsw.dataSetName = UA_STRING("NoPDS");
        refs[1] = UA_PubSubTest_ref(addDsw, 0, 1, 0);
        expected[1] = UA_STATUSCODE_BADNOTFOUND;
        break;
    case 5: /* PublishedDataSet removed in the call */
        refs[1] = UA_PubSubTest_ref(addDsw, 0, 1, 0);
        refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0);
        refsSize = 3;
        expected[1] = UA_STATUSCODE_BADNOTFOUND;
        break;
    case 6: /* Heartbeat writer without KeyFrameCount 1 */
        dsw.dataSetName = UA_STRING_NULL;
        refs[1] = UA_PubSubTest_ref(addDsw, 0, 1, 0);
        expected[1] = UA_STATUSCODE_BADCONFIGURATIONERROR;
        break;
    case 7: /* Duplicate DataSetWriterId in the group */
        dsw.dataSetWriterId = 200;
        refs[1] = UA_PubSubTest_ref(addDsw, 0, 1, 0);
        expected[1] = UA_STATUSCODE_BADCONFIGURATIONERROR;
        break;
    case 8: /* Duplicate DataSetWriter name */
        dsw.name = UA_STRING("BaseDSW");
        refs[1] = UA_PubSubTest_ref(addDsw, 0, 1, 0);
        expected[1] = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        break;
    case 9: /* Modify of an unknown WriterGroup */
        wg.name = UA_STRING("NoWG");
        refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 1, 0);
        expected[1] = UA_STATUSCODE_BADNOMATCH;
        break;
    case 10: /* Modify of a PublishedDataSet */
        refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0);
        expected[1] = UA_STATUSCODE_BADNOTIMPLEMENTED;
        break;
    case 11: /* Unknown linked StandaloneSubscribedDataSet */
        dsr.subscribedDataSet.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
        dsr.subscribedDataSet.content.decoded.type =
            &UA_TYPES[UA_TYPES_STANDALONESUBSCRIBEDDATASETREFDATATYPE];
        dsr.subscribedDataSet.content.decoded.data = &ssdsRef;
        refs[1] = UA_PubSubTest_ref(addDsr, 0, 1, 0);
        expected[1] = UA_STATUSCODE_BADNOTFOUND;
        break;
    case 12: /* The file element cannot be converted */
        dsr.subscribedDataSet.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
        dsr.subscribedDataSet.content.decoded.type =
            &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE];
        dsr.subscribedDataSet.content.decoded.data = &mirror;
        refs[1] = UA_PubSubTest_ref(addDsr, 0, 1, 0);
        expected[1] = UA_STATUSCODE_BADNOTIMPLEMENTED;
        break;
    default: /* No matching connection */
        refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 3, 0);
        expected[1] = UA_STATUSCODE_BADNOMATCH;
        break;
    }

    UA_UInt32 versionBefore = psm->configurationVersion;
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, refsSize, refs,
                                                      true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < refsSize; i++)
        ck_assert_uint_eq(result.referencesResults[i], expected[i]);
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* Nothing is applied. The removed components are recreated with new
     * NodeIds. */
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_ptr_null(UA_PubSubTest_findConnection(server, "ValidConn"));
    UA_PubSubConnection *baseConn = UA_PubSubTest_findConnection(server, "BaseConn");
    ck_assert_ptr_nonnull(baseConn);
    ck_assert_uint_eq(baseConn->writerGroupsSize, 1);
    UA_WriterGroup *baseWg = UA_WriterGroup_findByName(baseConn, UA_STRING("BaseWG"));
    ck_assert_ptr_nonnull(baseWg);
    ck_assert_uint_eq(baseWg->writersCount, 1);
    ck_assert_ptr_nonnull(UA_PublishedDataSet_findByName(psm, UA_STRING("BasePDS")));
    ck_assert_uint_eq(psm->configurationVersion, versionBefore);

    /* A partial update reports the same codes */
    res = UA_PubSubTest_updateConfig(server, &cfg, refsSize, refs, false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < refsSize; i++)
        ck_assert_uint_eq(result.referencesResults[i], expected[i]);
    ck_assert(result.changesApplied);
    ck_assert_ptr_nonnull(UA_PubSubTest_findConnection(server, "ValidConn"));
    UA_PubSubConfigurationUpdateResult_clear(&result);
} END_TEST

/* A complete update converts the referenced elements first. An element that
 * cannot be converted is rejected before anything is changed: the removed
 * group keeps its NodeId. */
START_TEST(PreCheckRejectsUnconvertible) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.name = UA_STRING("BaseConn");
    UA_WriterGroupDataType wg;
    UA_WriterGroupDataType_init(&wg);
    wg.name = UA_STRING("BaseWG");
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    UA_ReaderGroupDataType rg;
    UA_ReaderGroupDataType_init(&rg);
    rg.name = UA_STRING("BaseRG");
    UA_DataSetReaderDataType dsr;
    UA_DataSetReaderDataType_init(&dsr);
    dsr.name = UA_STRING("MirrorDSR");
    UA_SubscribedDataSetMirrorDataType mirror;
    UA_SubscribedDataSetMirrorDataType_init(&mirror);
    UA_ExtensionObject_setValueNoDelete(&dsr.subscribedDataSet, &mirror,
        &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE]);
    rg.dataSetReaders = &dsr;
    rg.dataSetReadersSize = 1;
    conn.readerGroups = &rg;
    conn.readerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;

    UA_PubSubConfigurationRefDataType refs[2] = {
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0),
        UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                          UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER, 0, 0, 0)};
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs, true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert(!result.changesApplied);
    ck_assert_int_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_int_eq(result.referencesResults[1], UA_STATUSCODE_BADNOTIMPLEMENTED);
    ck_assert(UA_NodeId_isNull(&result.configurationObjects[0]));
    UA_PubSubConfigurationUpdateResult_clear(&result);

    UA_WriterGroup *baseWg = UA_WriterGroup_find(psm, baseWgId);
    ck_assert_ptr_nonnull(baseWg);
    ck_assert_uint_eq(baseWg->writersCount, 1);
} END_TEST

/* Rollback of a complete update (Part 14 9.1.3.7.6). The DataSetWriter with a
 * fixed NetworkMessageNumber is rejected when it is created, after other
 * operations were applied. */
static UA_UadpDataSetWriterMessageDataType badUadpSettings;

static void
fileBadWriterInit(UA_DataSetWriterDataType *dsw, const char *dataSetName) {
    UA_PubSubTest_fileWriter(dsw, "BadDSW", 290, dataSetName);
    UA_UadpDataSetWriterMessageDataType_init(&badUadpSettings);
    badUadpSettings.networkMessageNumber = 1;
    dsw->messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    dsw->messageSettings.content.decoded.type =
        &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE];
    dsw->messageSettings.content.decoded.data = &badUadpSettings;
}

/* Remove BasePDS (with BaseDSW), add a PDS and a connection, modify BaseWG
 * and add the failing writer */
static UA_PubSubConfigurationRefDataType mixedRefs[5];
static UA_PubSubConnectionDataType mixedConns[2];
static UA_WriterGroupDataType mixedWg;
static UA_DataSetWriterDataType mixedDsw;
static UA_PublishedDataSetDataType mixedPds[2];

static void
buildMixedUpdate(UA_PubSubConfiguration2DataType *cfg) {
    UA_PubSubConfiguration2DataType_init(cfg);
    UA_PubSubConnectionDataType_init(&mixedConns[0]);
    mixedConns[0].name = UA_STRING("BaseConn");
    UA_PubSubTest_fileWriterGroup(&mixedWg, "BaseWG", 100);
    mixedWg.publishingInterval = 250.0;
    fileBadWriterInit(&mixedDsw, "AddedPDS");
    mixedWg.dataSetWriters = &mixedDsw;
    mixedWg.dataSetWritersSize = 1;
    mixedConns[0].writerGroups = &mixedWg;
    mixedConns[0].writerGroupsSize = 1;
    UA_PubSubTest_fileConnection(&mixedConns[1], "AddedConn", 2500);
    cfg->connections = mixedConns;
    cfg->connectionsSize = 2;
    UA_PublishedDataSetDataType_init(&mixedPds[0]);
    mixedPds[0].name = UA_STRING("BasePDS");
    UA_PubSubTest_filePds(&mixedPds[1], "AddedPDS",
                          UA_PubSubTest_addVariable(server, 51030, 42));
    cfg->publishedDataSets = mixedPds;
    cfg->publishedDataSetsSize = 2;

    mixedRefs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                           UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 0, 0, 0);
    mixedRefs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY |
                           UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    mixedRefs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                           UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 1, 0);
    mixedRefs[3] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                           UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, 1, 0, 0);
    mixedRefs[4] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                           UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);
}

START_TEST(RollbackRestoresRemovedModifiedAdded) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, basePdsId);
    UA_DataSetMetaDataType metaBefore;
    UA_DataSetMetaDataType_copy(&pds->dataSetMetaData, &metaBefore);
    UA_UInt32 versionBefore = psm->configurationVersion;

    UA_PubSubConfiguration2DataType cfg;
    buildMixedUpdate(&cfg);
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 5, mixedRefs,
                                                      true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 4; i++)
        ck_assert_uint_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[4], UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert(!result.changesApplied);
    /* The modified group keeps its NodeId, the other objects are gone */
    ck_assert(UA_NodeId_isNull(&result.configurationObjects[0]));
    ck_assert(UA_NodeId_equal(&result.configurationObjects[1], &baseWgId));
    ck_assert(UA_NodeId_isNull(&result.configurationObjects[2]));
    ck_assert(UA_NodeId_isNull(&result.configurationObjects[3]));
    UA_PubSubConfigurationUpdateResult_clear(&result);

    /* The added elements are removed */
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_ptr_null(UA_PubSubTest_findConnection(server, "AddedConn"));
    ck_assert_ptr_null(UA_PublishedDataSet_findByName(psm, UA_STRING("AddedPDS")));

    /* The modification is undone */
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, baseWgId);
    ck_assert_ptr_nonnull(wg);
    ck_assert(wg->config.publishingInterval == 100.0);

    /* The removed PDS is restored with its metadata, the writer is linked */
    pds = UA_PublishedDataSet_findByName(psm, UA_STRING("BasePDS"));
    ck_assert_ptr_nonnull(pds);
    ck_assert_uint_eq(pds->fieldSize, 1);
    ck_assert(UA_order(&pds->dataSetMetaData, &metaBefore,
                       &UA_TYPES[UA_TYPES_DATASETMETADATATYPE]) == UA_ORDER_EQ);
    ck_assert_uint_eq(wg->writersCount, 1);
    UA_DataSetWriter *dsw = LIST_FIRST(&wg->writers);
    UA_String dswName = UA_STRING("BaseDSW");
    ck_assert(UA_String_equal(&dsw->config.name, &dswName));
    ck_assert_uint_eq(dsw->config.dataSetWriterId, 200);
    ck_assert_ptr_eq(dsw->connectedDataSet, pds);
    ck_assert_uint_eq(psm->configurationVersion, versionBefore);
    UA_DataSetMetaDataType_clear(&metaBefore);

    /* The restored configuration can be removed */
    res = UA_Server_removePublishedDataSet(server, pds->head.identifier);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(wg->writersCount, 0);
} END_TEST

/* A partial update keeps the operations that were applied */
START_TEST(NonAtomicApplyFailureBestEffort) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);

    UA_PubSubConfiguration2DataType cfg;
    buildMixedUpdate(&cfg);
    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 5, mixedRefs,
                                                      false, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 4; i++)
        ck_assert_uint_eq(result.referencesResults[i], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[4], UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert(result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ck_assert_ptr_nonnull(UA_PubSubTest_findConnection(server, "AddedConn"));
    ck_assert_ptr_null(UA_PublishedDataSet_findByName(psm, UA_STRING("BasePDS")));
    ck_assert_ptr_nonnull(UA_PublishedDataSet_findByName(psm, UA_STRING("AddedPDS")));
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, baseWgId);
    ck_assert(wg->config.publishingInterval == 250.0);
    ck_assert_uint_eq(wg->writersCount, 0);
} END_TEST

/* A removed SubscribedDataSet is restored with its reader. The ReaderGroup
 * gets its prior state back. */
START_TEST(RollbackRestoresSsdsAndReader) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId targetVar = UA_PubSubTest_addVariable(server, 51031, 42);

    UA_FieldMetaData fmd;
    UA_FieldMetaData_init(&fmd);
    fmd.name = UA_STRING("Field1");
    fmd.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    fmd.builtInType = UA_NS0ID_UINT32;
    fmd.valueRank = -1;
    UA_SubscribedDataSetConfig ssdsConfig;
    memset(&ssdsConfig, 0, sizeof(ssdsConfig));
    ssdsConfig.name = UA_STRING("S1");
    ssdsConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    ssdsConfig.dataSetMetaData.name = UA_STRING("S1");
    ssdsConfig.dataSetMetaData.fields = &fmd;
    ssdsConfig.dataSetMetaData.fieldsSize = 1;
    UA_FieldTargetDataType target;
    UA_FieldTargetDataType_init(&target);
    target.attributeId = UA_ATTRIBUTEID_VALUE;
    target.targetNodeId = targetVar;
    ssdsConfig.subscribedDataSet.target.targetVariables = &target;
    ssdsConfig.subscribedDataSet.target.targetVariablesSize = 1;
    UA_NodeId ssdsId;
    UA_StatusCode res = UA_Server_addSubscribedDataSet(server, &ssdsConfig, &ssdsId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);

    UA_DataSetReaderConfig dsrConfig;
    memset(&dsrConfig, 0, sizeof(dsrConfig));
    dsrConfig.name = UA_STRING("R1");
    dsrConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    dsrConfig.publisherId.id.uint16 = 2234;
    dsrConfig.writerGroupId = 100;
    dsrConfig.dataSetWriterId = 200;
    dsrConfig.dataSetMetaData.fields = &fmd;
    dsrConfig.dataSetMetaData.fieldsSize = 1;
    dsrConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    dsrConfig.linkedStandaloneSubscribedDataSetName = UA_STRING("S1");
    UA_NodeId dsrId;
    res = UA_Server_addDataSetReader(server, baseRgId, &dsrConfig, &dsrId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    res = UA_Server_enableReaderGroup(server, baseRgId);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, baseRgId);
    UA_PubSubState rgState = rg->head.state;
    ck_assert(UA_PubSubState_isEnabled(rgState));

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_StandaloneSubscribedDataSetDataType ssds;
    UA_StandaloneSubscribedDataSetDataType_init(&ssds);
    ssds.name = UA_STRING("S1");
    cfg.subscribedDataSets = &ssds;
    cfg.subscribedDataSetsSize = 1;
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.name = UA_STRING("BaseConn");
    UA_WriterGroupDataType wg;
    UA_WriterGroupDataType_init(&wg);
    wg.name = UA_STRING("BaseWG");
    UA_DataSetWriterDataType dsw;
    fileBadWriterInit(&dsw, "BasePDS");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;
    UA_PubSubConfigurationRefDataType refs[2];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESUBDATASET, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs, true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[1], UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    UA_SubscribedDataSet *sds = UA_SubscribedDataSet_findByName(psm, UA_STRING("S1"));
    ck_assert_ptr_nonnull(sds);
    ck_assert_ptr_nonnull(sds->connectedReader);
    UA_String readerName = UA_STRING("R1");
    ck_assert(UA_String_equal(&sds->connectedReader->config.name, &readerName));
    ck_assert_ptr_eq(sds->connectedReader->linkedReaderGroup, rg);
    ck_assert_uint_eq(rg->readersCount, 1);
    ck_assert_int_eq(rg->head.state, rgState);
    ck_assert_uint_eq(UA_WriterGroup_find(psm, baseWgId)->writersCount, 1);
} END_TEST

/* The results of a match remain after a rollback. The names assigned to
 * added elements are dropped. */
START_TEST(RollbackKeepsMatchResults) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *baseConn = UA_PubSubConnection_find(psm, baseConnId);

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubTest_fileConnection(&conn, "", 0); /* Matches BaseConn */
    UA_WriterGroupDataType wg;
    UA_PubSubTest_fileWriterGroup(&wg, "", 0); /* Name and id are assigned */
    UA_DataSetWriterDataType dsw;
    fileBadWriterInit(&dsw, "BasePDS");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    conn.writerGroups = &wg;
    conn.writerGroupsSize = 1;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;
    UA_PubSubConfigurationRefDataType refs[3];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[2] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 3, refs,
                                                      true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[1], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[2], UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert(!result.changesApplied);
    ck_assert(UA_NodeId_equal(&result.configurationObjects[0], &baseConnId));
    ck_assert(UA_NodeId_isNull(&result.configurationObjects[1]));
    ck_assert_uint_eq(result.configurationValuesSize, 1);
    UA_String baseName = UA_STRING("BaseConn");
    ck_assert(UA_String_equal(&result.configurationValues[0].name, &baseName));
    UA_PubSubConfigurationUpdateResult_clear(&result);
    ck_assert_uint_eq(baseConn->writerGroupsSize, 1);
} END_TEST

/* The top-level fields are not applied when the update is rolled back */
START_TEST(RollbackDiscardsTopLevelFields) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    UA_UInt32 versionBefore = psm->configurationVersion;

    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conns[2];
    UA_PubSubTest_fileConnection(&conns[0], "PropConn", 3435);
    UA_PubSubConnectionDataType_init(&conns[1]);
    conns[1].name = UA_STRING("BaseConn");
    UA_WriterGroupDataType wg;
    UA_WriterGroupDataType_init(&wg);
    wg.name = UA_STRING("BaseWG");
    UA_DataSetWriterDataType dsw;
    fileBadWriterInit(&dsw, "BasePDS");
    wg.dataSetWriters = &dsw;
    wg.dataSetWritersSize = 1;
    conns[1].writerGroups = &wg;
    conns[1].writerGroupsSize = 1;
    cfg.connections = conns;
    cfg.connectionsSize = 2;
    UA_KeyValuePair prop;
    prop.key = UA_QUALIFIEDNAME(0, "Vendor");
    UA_UInt32 vendorValue = 99;
    UA_Variant_setScalar(&prop.value, &vendorValue, &UA_TYPES[UA_TYPES_UINT32]);
    cfg.configurationProperties = &prop;
    cfg.configurationPropertiesSize = 1;
    UA_PubSubConfigurationRefDataType refs[2];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 1, 0);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs,
                                                      true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[1], UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    ck_assert_ptr_null(UA_PubSubTest_findConnection(server, "PropConn"));
    ck_assert_ptr_null(UA_KeyValueMap_get(&psm->configurationProperties,
                                          UA_QUALIFIEDNAME(0, "Vendor")));
    ck_assert_uint_eq(psm->configurationVersion, versionBefore);
} END_TEST

/* A rolled back removal puts the component back to its list position */
START_TEST(RollbackKeepsListOrder) {
    buildBaseConfig();
    UA_PubSubManager *psm = getPSM(server);
    const char *names[2] = {"OrderA", "OrderB"};
    for(size_t i = 0; i < 2; i++) {
        UA_WriterGroupConfig wgConfig;
        memset(&wgConfig, 0, sizeof(wgConfig));
        wgConfig.name = UA_STRING((char*)(uintptr_t)names[i]);
        wgConfig.writerGroupId = (UA_UInt16)(111 + i);
        wgConfig.publishingInterval = 100.0;
        UA_NodeId wgId;
        UA_StatusCode res = UA_Server_addWriterGroup(server, baseConnId, &wgConfig, &wgId);
        ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    }
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, baseConnId);
    UA_String before[3];
    size_t i = 0;
    UA_WriterGroup *wg;
    LIST_FOREACH(wg, &c->writerGroups, listEntry)
        UA_String_copy(&wg->config.name, &before[i++]);
    ck_assert_uint_eq(i, 3);

    /* Remove the middle group, then fail with the writer */
    UA_PubSubConfiguration2DataType cfg;
    UA_PubSubConfiguration2DataType_init(&cfg);
    UA_PubSubConnectionDataType conn;
    UA_PubSubConnectionDataType_init(&conn);
    conn.name = UA_STRING("BaseConn");
    UA_WriterGroupDataType wgs[2];
    UA_WriterGroupDataType_init(&wgs[0]);
    wgs[0].name = before[1];
    UA_WriterGroupDataType_init(&wgs[1]);
    wgs[1].name = UA_STRING("BaseWG");
    UA_DataSetWriterDataType dsw;
    fileBadWriterInit(&dsw, "BasePDS");
    wgs[1].dataSetWriters = &dsw;
    wgs[1].dataSetWritersSize = 1;
    conn.writerGroups = wgs;
    conn.writerGroupsSize = 2;
    cfg.connections = &conn;
    cfg.connectionsSize = 1;
    UA_PubSubConfigurationRefDataType refs[2];
    refs[0] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, 0, 0);
    refs[1] = UA_PubSubTest_ref(UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                      UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, 0, 0, 1);

    UA_PubSubConfigurationUpdateResult result;
    UA_StatusCode res = UA_PubSubTest_updateConfig(server, &cfg, 2, refs, true, &result);
    ck_assert_int_eq(res, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[0], UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(result.referencesResults[1], UA_STATUSCODE_BADNOTSUPPORTED);
    ck_assert(!result.changesApplied);
    UA_PubSubConfigurationUpdateResult_clear(&result);

    i = 0;
    LIST_FOREACH(wg, &c->writerGroups, listEntry) {
        ck_assert(i < 3);
        ck_assert(UA_String_equal(&wg->config.name, &before[i]));
        i++;
    }
    ck_assert_uint_eq(i, 3);
    for(i = 0; i < 3; i++)
        UA_String_clear(&before[i]);
} END_TEST

int main(void) {
    TCase *tc_incremental = tcase_create("CloseAndUpdate element operations");
    tcase_add_checked_fixture(tc_incremental, setup, teardown);
    tcase_add_test(tc_incremental, AddElements);
    tcase_add_test(tc_incremental, AutoAssign);
    tcase_add_test(tc_incremental, MaskValidation);
    tcase_add_test(tc_incremental, RemoveAndReAdd);
    tcase_add_test(tc_incremental, ModifyElements);
    tcase_add_test(tc_incremental, MatchElements);
    tcase_add_test(tc_incremental, TopLevelFields);
    tcase_add_test(tc_incremental, RemoveSsdsWithReaderInEnabledGroup);
    tcase_add_loop_test(tc_incremental, ChildrenBeforeParents, 0, 2);
    tcase_add_loop_test(tc_incremental, AutoNamedParent, 0, 2);
    tcase_add_loop_test(tc_incremental, MatchedParent, 0, 2);
    tcase_add_loop_test(tc_incremental, FailedParentNotFound, 0, 2);
    tcase_add_loop_test(tc_incremental, RemovedParentNotFound, 0, 2);
    tcase_add_loop_test(tc_incremental, EmptyParentName, 0, 2);
    tcase_add_loop_test(tc_incremental, MatchRequiresNullNameAndId, 0, 2);
    tcase_add_loop_test(tc_incremental, RemoveParentAndChild, 0, 2);
    tcase_add_loop_test(tc_incremental, DoubleRemove, 0, 2);
    tcase_add_loop_test(tc_incremental, CompleteUpdateValidation, 0,
                        VALIDATION_CASES);
    tcase_add_test(tc_incremental, PreCheckRejectsUnconvertible);
    tcase_add_test(tc_incremental, RollbackRestoresRemovedModifiedAdded);
    tcase_add_test(tc_incremental, NonAtomicApplyFailureBestEffort);
    tcase_add_test(tc_incremental, RollbackRestoresSsdsAndReader);
    tcase_add_test(tc_incremental, RollbackKeepsMatchResults);
    tcase_add_test(tc_incremental, RollbackDiscardsTopLevelFields);
    tcase_add_test(tc_incremental, RollbackKeepsListOrder);

    Suite *s = suite_create("PubSub Configuration2 incremental update");
    suite_add_tcase(s, tc_incremental);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
