/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_server_internal.h"
#include "ua_pubsub_internal.h"
#include "test_helpers.h"
#include "testing_clock.h"

#include <check.h>
#include <stdlib.h>

UA_Server *server = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddPDSWithMinimalValidConfiguration){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("TEST PDS 1");
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, NULL).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_NodeId newPDSNodeID;
    pdsConfig.name = UA_STRING("TEST PDS 2");
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &newPDSNodeID).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 2);
    ck_assert_int_eq(newPDSNodeID.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_ne(newPDSNodeID.identifier.numeric, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddRemoveAddPDSWithMinimalValidConfiguration){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("TEST PDS 1");
    UA_NodeId newPDSNodeID;
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &newPDSNodeID).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal |= UA_Server_removePublishedDataSet(server, newPDSNodeID);
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &newPDSNodeID).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddPDSWithNullConfig){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_addPublishedDataSet(server, NULL, NULL).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddPDSWithUnsupportedType){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.name = UA_STRING("TEST PDS 1");
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS;
    retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, NULL).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADINVALIDARGUMENT);
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS_TEMPLATE;
    retVal = UA_Server_addPublishedDataSet(server, &pdsConfig, NULL).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADINVALIDARGUMENT);
} END_TEST

START_TEST(GetPDSConfigurationAndCompareValues){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("TEST PDS 1");
    UA_NodeId pdsIdentifier;
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsIdentifier).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PublishedDataSetConfig pdsConfigCopy;
    memset(&pdsConfigCopy, 0, sizeof(UA_PublishedDataSetConfig));
        UA_Server_getPublishedDataSetConfig(server, pdsIdentifier, &pdsConfigCopy);
    ck_assert_int_eq(UA_String_equal(&pdsConfig.name, &pdsConfigCopy.name), UA_TRUE);
    UA_PublishedDataSetConfig_clear(&pdsConfigCopy);
} END_TEST

/* ---------------------------------------------------------------------------
 * Additional coverage tests:
 * Additional coverage tests (Phase A4):
 *  - findByName / find for unknown ids
 *  - DataSetField add/remove for the VARIABLE type, with slot reuse
 *  - DataSetField rejected for the EVENT type
 *  - removePublishedDataSet on unknown id returns BADNOTFOUND
 *  - getPublishedDataSetConfig invalid-arg paths
 *  - custom dataSetMetaData survives roundtrip
 * ------------------------------------------------------------------------- */

START_TEST(FindPDSByNameAndById_UnknownReturnsNull) {
    UA_PubSubManager *psm = getPSM(server);
    UA_PublishedDataSet *pds =
        UA_PublishedDataSet_findByName(psm, UA_STRING("does-not-exist"));
    ck_assert_ptr_eq(pds, NULL);
    pds = UA_PublishedDataSet_find(psm, UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
    ck_assert_ptr_eq(pds, NULL);
} END_TEST

START_TEST(RemovePublishedDataSetUnknownReturnsBadNotFound) {
    UA_StatusCode retVal =
        UA_Server_removePublishedDataSet(server,
                                         UA_NODEID_NUMERIC(0, UA_UINT32_MAX));
    ck_assert_int_eq(retVal, UA_STATUSCODE_BADNOTFOUND);
} END_TEST

START_TEST(DataSetFieldAddRemoveSlotReuse) {
    UA_PubSubManager *psm = getPSM(server);
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PDS-Slots");
    UA_NodeId pdsId;
    UA_StatusCode rv =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId).addResult;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, pdsId);
    ck_assert_ptr_ne(pds, NULL);
    ck_assert_uint_eq(pds->fieldSize, 0);

    UA_DataSetFieldConfig f;
    memset(&f, 0, sizeof(f));
    f.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    f.field.variable.publishParameters.publishedVariable =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    f.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_NodeId f1, f2, f3;
    f.field.variable.fieldNameAlias = UA_STRING("alias1");
    rv = UA_Server_addDataSetField(server, pdsId, &f, &f1).result;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    f.field.variable.fieldNameAlias = UA_STRING("alias2");
    rv = UA_Server_addDataSetField(server, pdsId, &f, &f2).result;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    f.field.variable.fieldNameAlias = UA_STRING("alias3");
    rv = UA_Server_addDataSetField(server, pdsId, &f, &f3).result;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pds->fieldSize, 3);

    /* Remove middle field, then re-add → fieldSize round-trips */
    rv = UA_Server_removeDataSetField(server, f2).result;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pds->fieldSize, 2);
    f.field.variable.fieldNameAlias = UA_STRING("alias2-new");
    rv = UA_Server_addDataSetField(server, pdsId, &f, &f2).result;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pds->fieldSize, 3);

    /* Remove on an unknown nodeid */
    rv = UA_Server_removeDataSetField(server,
                                      UA_NODEID_NUMERIC(0, UA_UINT32_MAX)).result;
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    /* Removing the PDS clears all attached fields */
    rv = UA_Server_removePublishedDataSet(server, pdsId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
} END_TEST

START_TEST(AddDataSetFieldEventTypeRejected) {
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PDS-NoEvents");
    UA_NodeId pdsId;
    UA_StatusCode rv =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId).addResult;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig f;
    memset(&f, 0, sizeof(f));
    f.dataSetFieldType = UA_PUBSUB_DATASETFIELD_EVENT;
    rv = UA_Server_addDataSetField(server, pdsId, &f, NULL).result;
    /* Events are not supported on a PUBLISHEDITEMS PDS - must fail */
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    UA_Server_removePublishedDataSet(server, pdsId);
} END_TEST

START_TEST(AddDataSetFieldNullConfigReturnsBadInvalidArgument) {
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PDS-NullFieldCfg");
    UA_NodeId pdsId;
    UA_StatusCode rv =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId).addResult;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    rv = UA_Server_addDataSetField(server, pdsId, NULL, NULL).result;
    ck_assert_int_eq(rv, UA_STATUSCODE_BADINVALIDARGUMENT);

    UA_Server_removePublishedDataSet(server, pdsId);
} END_TEST

START_TEST(AddDataSetFieldVariableInvalidSourceNodeReturnsError) {
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PDS-BadSourceNode");
    UA_NodeId pdsId;
    UA_StatusCode rv =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId).addResult;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig f;
    memset(&f, 0, sizeof(f));
    f.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    f.field.variable.fieldNameAlias = UA_STRING("bad-source");
    f.field.variable.publishParameters.publishedVariable =
        UA_NODEID_NUMERIC(42, UA_UINT32_MAX);
    f.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    rv = UA_Server_addDataSetField(server, pdsId, &f, NULL).result;
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    UA_Server_removePublishedDataSet(server, pdsId);
} END_TEST

START_TEST(AddDataSetFieldRejectedWhenPDSInUse) {
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PDS-InUse");
    UA_NodeId pdsId;
    UA_StatusCode rv =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId).addResult;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_PubSubConnectionConfig cc;
    memset(&cc, 0, sizeof(cc));
    cc.name = UA_STRING("Conn-InUse");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&cc.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    cc.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NodeId connId;
    rv = UA_Server_addPubSubConnection(server, &cc, &connId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING("WG-InUse");
    wgc.publishingInterval = 100;
    UA_NodeId wgId;
    rv = UA_Server_addWriterGroup(server, connId, &wgc, &wgId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dswc;
    memset(&dswc, 0, sizeof(dswc));
    dswc.name = UA_STRING("DSW-InUse");
    dswc.dataSetWriterId = 1;
    rv = UA_Server_addDataSetWriter(server, wgId, pdsId, &dswc, NULL);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);

    UA_PubSubManager *psm = getPSM(server);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, pdsId);
    ck_assert_ptr_ne(pds, NULL);
    pds->configurationFreezeCounter = 1;

    UA_DataSetFieldConfig f;
    memset(&f, 0, sizeof(f));
    f.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    f.field.variable.fieldNameAlias = UA_STRING("blocked-by-freeze");
    f.field.variable.publishParameters.publishedVariable =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    f.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    rv = UA_Server_addDataSetField(server, pdsId, &f, NULL).result;
    ck_assert_int_eq(rv, UA_STATUSCODE_BADCONFIGURATIONERROR);

    pds->configurationFreezeCounter = 0;
    UA_Server_removePublishedDataSet(server, pdsId);
} END_TEST

static UA_NodeId
addTestPds(const char *name) {
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING((char*)(uintptr_t)name);
    UA_NodeId pdsId;
    UA_StatusCode rv =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId).addResult;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    return pdsId;
}

static UA_NodeId
addTestConnection(void) {
    UA_PubSubConnectionConfig cc;
    memset(&cc, 0, sizeof(cc));
    cc.name = UA_STRING("Conn-Remove");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&cc.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    cc.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NodeId connId;
    UA_StatusCode rv = UA_Server_addPubSubConnection(server, &cc, &connId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    return connId;
}

static UA_NodeId
addTestWriterGroup(UA_NodeId connId, const char *name, UA_UInt16 id) {
    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING((char*)(uintptr_t)name);
    wgc.writerGroupId = id;
    wgc.publishingInterval = 100;
    UA_NodeId wgId;
    UA_StatusCode rv = UA_Server_addWriterGroup(server, connId, &wgc, &wgId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    return wgId;
}

static UA_NodeId
addTestWriter(UA_NodeId wgId, UA_NodeId pdsId, const char *name, UA_UInt16 id) {
    UA_DataSetWriterConfig dswc;
    memset(&dswc, 0, sizeof(dswc));
    dswc.name = UA_STRING((char*)(uintptr_t)name);
    dswc.dataSetWriterId = id;
    UA_NodeId dswId;
    UA_StatusCode rv = UA_Server_addDataSetWriter(server, wgId, pdsId, &dswc, &dswId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    return dswId;
}

/* A disabled writer in an enabled WriterGroup does not freeze the PDS, but it
 * cannot be removed. The PDS must then stay, the writer keeps pointing to it. */
START_TEST(RemovePdsWithWriterInEnabledGroupFails) {
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId pdsId = addTestPds("PDS-Remove");
    UA_NodeId connId = addTestConnection();
    UA_NodeId wgId = addTestWriterGroup(connId, "WG-Remove", 1);
    UA_NodeId dswId = addTestWriter(wgId, pdsId, "DSW-Remove", 1);

    UA_StatusCode rv = UA_Server_enableWriterGroup(server, wgId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, pdsId);
    ck_assert_ptr_nonnull(pds);
    ck_assert_uint_eq(pds->configurationFreezeCounter, 0);

    rv = UA_Server_removePublishedDataSet(server, pdsId);
    ck_assert_int_eq(rv, UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(psm->publishedDataSetsSize, 1);
    UA_DataSetWriter *dsw = UA_DataSetWriter_find(psm, dswId);
    ck_assert_ptr_nonnull(dsw);
    ck_assert_ptr_eq(dsw->connectedDataSet, pds);

    /* Enabling the writer freezes the (still valid) PDS */
    rv = UA_Server_enableDataSetWriter(server, dswId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pds->configurationFreezeCounter, 1);

    /* With the writer and the group disabled the removal succeeds */
    UA_Server_disableDataSetWriter(server, dswId);
    UA_Server_disableWriterGroup(server, wgId);
    rv = UA_Server_removePublishedDataSet(server, pdsId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert_ptr_null(UA_DataSetWriter_find(psm, dswId));
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    ck_assert_ptr_nonnull(wg);
    ck_assert_uint_eq(wg->writersCount, 0);
} END_TEST

static UA_StatusCode
vetoWriterRemoval(UA_Server *s, const UA_NodeId id,
                  const UA_PubSubComponentType componentType,
                  UA_Boolean remove) {
    if(remove && componentType == UA_PUBSUBCOMPONENT_DATASETWRITER)
        return UA_STATUSCODE_BADUSERACCESSDENIED;
    return UA_STATUSCODE_GOOD;
}

/* A vetoed writer removal stops the PDS removal. The PDS is not freed. */
START_TEST(RemovePdsStopsOnVetoedWriterRemoval) {
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId pdsId = addTestPds("PDS-Veto");
    UA_NodeId connId = addTestConnection();
    UA_NodeId wgId = addTestWriterGroup(connId, "WG-Veto", 1);
    UA_NodeId dswId = addTestWriter(wgId, pdsId, "DSW-Veto", 1);

    UA_ServerConfig *sc = UA_Server_getConfig(server);
    sc->pubSubConfig.componentLifecycleCallback = vetoWriterRemoval;
    UA_StatusCode rv = UA_Server_removePublishedDataSet(server, pdsId);
    sc->pubSubConfig.componentLifecycleCallback = NULL;

    ck_assert_int_eq(rv, UA_STATUSCODE_BADUSERACCESSDENIED);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, pdsId);
    ck_assert_ptr_nonnull(pds);
    UA_DataSetWriter *dsw = UA_DataSetWriter_find(psm, dswId);
    ck_assert_ptr_nonnull(dsw);
    ck_assert_ptr_eq(dsw->connectedDataSet, pds);
} END_TEST

/* The removal takes all connected writers along, over several groups, and
 * leaves the writers of other PDS in place */
START_TEST(RemovePdsRemovesWritersInDisabledGroups) {
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId pdsA = addTestPds("PDS-A");
    UA_NodeId pdsB = addTestPds("PDS-B");
    UA_NodeId connId = addTestConnection();
    UA_NodeId wg1 = addTestWriterGroup(connId, "WG-1", 1);
    UA_NodeId wg2 = addTestWriterGroup(connId, "WG-2", 2);
    UA_NodeId dswA1 = addTestWriter(wg1, pdsA, "DSW-A1", 1);
    UA_NodeId dswA2 = addTestWriter(wg2, pdsA, "DSW-A2", 1);
    UA_NodeId dswB = addTestWriter(wg2, pdsB, "DSW-B", 2);

    UA_StatusCode rv = UA_Server_removePublishedDataSet(server, pdsA);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(UA_PublishedDataSet_find(psm, pdsA));
    ck_assert_ptr_null(UA_DataSetWriter_find(psm, dswA1));
    ck_assert_ptr_null(UA_DataSetWriter_find(psm, dswA2));
    UA_DataSetWriter *writerB = UA_DataSetWriter_find(psm, dswB);
    ck_assert_ptr_nonnull(writerB);
    ck_assert_ptr_eq(writerB->connectedDataSet,
                     UA_PublishedDataSet_find(psm, pdsB));
} END_TEST

static UA_NodeId
addTestUnicastWriterGroup(UA_NodeId connId) {
    UA_WriterGroupConfig wgc;
    memset(&wgc, 0, sizeof(wgc));
    wgc.name = UA_STRING("WG-Unicast-Remove");
    wgc.writerGroupId = 1;
    wgc.publishingInterval = 100;
    wgc.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    UA_DatagramWriterGroupTransport2DataType transport;
    memset(&transport, 0, sizeof(transport));
    UA_NetworkAddressUrlDataType address =
        {UA_STRING_NULL, UA_STRING("opc.udp://127.0.0.1:4841/")};
    UA_ExtensionObject_setValue(&transport.address, &address,
                               &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_ExtensionObject_setValue(&wgc.transportSettings, &transport,
                               &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORT2DATATYPE]);
    UA_NodeId wgId;
    UA_StatusCode rv = UA_Server_addWriterGroup(server, connId, &wgc, &wgId);
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    return wgId;
}

static void
iteratePubSub(void) {
    for(size_t i = 0; i < 10; i++) {
        UA_fakeSleep(10);
        UA_Server_run_iterate(server, false);
    }
}

static size_t writerRemovalVetoes;
static size_t writerRemovalNotifications;
static size_t groupRemovalNotifications;

static UA_StatusCode
countAndVetoWriterRemoval(UA_Server *s, const UA_NodeId id,
                         const UA_PubSubComponentType componentType,
                         UA_Boolean remove) {
    if(!remove)
        return UA_STATUSCODE_GOOD;
    if(componentType == UA_PUBSUBCOMPONENT_WRITERGROUP)
        groupRemovalNotifications++;
    if(componentType == UA_PUBSUBCOMPONENT_DATASETWRITER) {
        writerRemovalNotifications++;
        if(writerRemovalVetoes > 0) {
            writerRemovalVetoes--;
            return UA_STATUSCODE_BADUSERACCESSDENIED;
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void
setWriterRemovalVetoes(size_t count) {
    writerRemovalVetoes = count;
    writerRemovalNotifications = 0;
    groupRemovalNotifications = 0;
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback =
        countAndVetoWriterRemoval;
}

/* The group has its own socket, so closing it really resumes removal. A
 * previously vetoed writer must be removed without asking about the group again. */
START_TEST(DeferredWriterGroupDeleteRetriesVetoedWriter) {
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId pdsId = addTestPds("PDS-Deferred-Veto");
    UA_NodeId connId = addTestConnection();
    UA_NodeId wgId = addTestUnicastWriterGroup(connId);
    UA_NodeId dswId = addTestWriter(wgId, pdsId, "DSW-Deferred-Veto", 1);
    ck_assert_int_eq(UA_Server_enableAllPubSubComponents(server), UA_STATUSCODE_GOOD);
    iteratePubSub();
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, pdsId);
    ck_assert(wg->sendChannel != 0);
    ck_assert_uint_eq(pds->configurationFreezeCounter, 1);

    setWriterRemovalVetoes(1);
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, wgId),
                     UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert(wg->deleteFlag && wg->sendChannel != 0);
    ck_assert_uint_eq(wg->writersCount, 1);
    iteratePubSub();
    ck_assert_ptr_null(UA_WriterGroup_find(psm, wgId));
    ck_assert_ptr_null(UA_DataSetWriter_find(psm, dswId));
    ck_assert_uint_eq(groupRemovalNotifications, 1);
    ck_assert_uint_eq(writerRemovalNotifications, 2);
    ck_assert_uint_eq(pds->configurationFreezeCounter, 0);
    ck_assert_int_eq(UA_Server_removePublishedDataSet(server, pdsId), UA_STATUSCODE_GOOD);
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;
} END_TEST

/* A persistent child veto must retain both parent and child, with or without a
 * deferred channel close. An explicit retry completes the approved deletion. */
START_TEST(WriterGroupDeletePreservesVetoedWriter) {
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId pdsId = addTestPds("PDS-Persistent-Veto");
    UA_NodeId connId = addTestConnection();
    UA_NodeId wgId = (_i == 0) ? addTestWriterGroup(connId, "WG-Veto", 1) :
        addTestUnicastWriterGroup(connId);
    UA_NodeId dswId = addTestWriter(wgId, pdsId, "DSW-Persistent-Veto", 1);
    ck_assert_int_eq(UA_Server_enableAllPubSubComponents(server), UA_STATUSCODE_GOOD);
    iteratePubSub();
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    if(_i != 0)
        ck_assert(wg->sendChannel != 0);
    setWriterRemovalVetoes(100);
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, wgId),
                     UA_STATUSCODE_BADUSERACCESSDENIED);
    iteratePubSub();
    ck_assert_ptr_eq(UA_WriterGroup_find(psm, wgId), wg);
    ck_assert(wg->deleteFlag && wg->sendChannel == 0);
    ck_assert_uint_eq(wg->writersCount, 1);
    UA_DataSetWriter *dsw = UA_DataSetWriter_find(psm, dswId);
    ck_assert_ptr_nonnull(dsw);
    ck_assert_ptr_eq(dsw->linkedWriterGroup, wg);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, pdsId);
    ck_assert_uint_eq(pds->configurationFreezeCounter, 1);
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, wgId),
                     UA_STATUSCODE_BADUSERACCESSDENIED);
    ck_assert_ptr_eq(UA_WriterGroup_find(psm, wgId), wg);

    writerRemovalVetoes = 0;
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, wgId), UA_STATUSCODE_GOOD);
    ck_assert_ptr_null(UA_WriterGroup_find(psm, wgId));
    ck_assert_ptr_null(UA_DataSetWriter_find(psm, dswId));
    ck_assert_uint_eq(groupRemovalNotifications, 1);
    ck_assert_uint_eq(pds->configurationFreezeCounter, 0);
    ck_assert_int_eq(UA_Server_removePublishedDataSet(server, pdsId), UA_STATUSCODE_GOOD);
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;
} END_TEST

START_TEST(DeferredWriterGroupDeleteRejectsNewWriters) {
    UA_PubSubManager *psm = getPSM(server);
    UA_NodeId pdsId = addTestPds("PDS-Late-Writer");
    UA_NodeId connId = addTestConnection();
    UA_NodeId wgId = addTestUnicastWriterGroup(connId);
    ck_assert_int_eq(UA_Server_enableAllPubSubComponents(server), UA_STATUSCODE_GOOD);
    iteratePubSub();
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    ck_assert(wg->sendChannel != 0);
    setWriterRemovalVetoes(0);
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, wgId), UA_STATUSCODE_GOOD);
    ck_assert(wg->deleteFlag && wg->sendChannel != 0);

    UA_DataSetWriterConfig dswc;
    memset(&dswc, 0, sizeof(dswc));
    dswc.name = UA_STRING("DSW-Late");
    dswc.dataSetWriterId = 1;
    ck_assert_int_eq(UA_Server_addDataSetWriter(server, wgId, pdsId, &dswc, NULL),
                     UA_STATUSCODE_BADINVALIDSTATE);
    ck_assert_uint_eq(wg->writersCount, 0);
    ck_assert_uint_eq(UA_PublishedDataSet_find(psm, pdsId)->configurationFreezeCounter, 0);
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, wgId), UA_STATUSCODE_GOOD);
    iteratePubSub();
    ck_assert_ptr_null(UA_WriterGroup_find(psm, wgId));
    ck_assert_uint_eq(groupRemovalNotifications, 1);
    ck_assert_uint_eq(writerRemovalNotifications, 0);
    ck_assert_int_eq(UA_Server_removePublishedDataSet(server, pdsId), UA_STATUSCODE_GOOD);
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;
} END_TEST

/* A retained group has no open socket after its close callback. It must not
 * prevent the PubSubManager from completing a stop request. */
START_TEST(DeferredWriterGroupVetoCompletesManagerStop) {
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnectionConfig cc;
    memset(&cc, 0, sizeof(cc));
    cc.name = UA_STRING("Conn-Unicast-Only");
    cc.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_NetworkAddressUrlDataType address =
        {UA_STRING_NULL, UA_STRING("opc.udp://localhost:4840/")};
    UA_Variant_setScalar(&cc.address, &address,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_NodeId connId;
    ck_assert_int_eq(UA_Server_addPubSubConnection(server, &cc, &connId), UA_STATUSCODE_GOOD);
    UA_NodeId pdsId = addTestPds("PDS-Stop-Veto");
    UA_NodeId wgId = addTestUnicastWriterGroup(connId);
    addTestWriter(wgId, pdsId, "DSW-Stop-Veto", 1);
    ck_assert_int_eq(UA_Server_enableAllPubSubComponents(server), UA_STATUSCODE_GOOD);
    iteratePubSub();
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    ck_assert(wg->sendChannel != 0);
    UA_PubSubConnection *conn = UA_PubSubConnection_find(psm, connId);
    ck_assert_uint_eq(conn->sendChannel, 0);
    ck_assert_uint_eq(conn->recvChannelsSize, 0);

    setWriterRemovalVetoes(100);
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, wgId),
                     UA_STATUSCODE_BADUSERACCESSDENIED);
    lockServer(server);
    UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STOPPED);
    unlockServer(server);
    ck_assert_int_eq(psm->drv.state, UA_LIFECYCLESTATE_STOPPING);
    iteratePubSub();
    ck_assert_ptr_eq(UA_WriterGroup_find(psm, wgId), wg);
    ck_assert_uint_eq(wg->sendChannel, 0);
    ck_assert_int_eq(psm->drv.state, UA_LIFECYCLESTATE_STOPPED);

    writerRemovalVetoes = 0;
    ck_assert_int_eq(UA_Server_removeWriterGroup(server, wgId), UA_STATUSCODE_GOOD);
    ck_assert_int_eq(UA_Server_removePublishedDataSet(server, pdsId), UA_STATUSCODE_GOOD);
    UA_Server_getConfig(server)->pubSubConfig.componentLifecycleCallback = NULL;
} END_TEST

START_TEST(GetPublishedDataSetConfigInvalidArgs) {
    /* unknown id */
    UA_PublishedDataSetConfig copy;
    memset(&copy, 0, sizeof(copy));
    UA_StatusCode rv =
        UA_Server_getPublishedDataSetConfig(server,
                                            UA_NODEID_NUMERIC(0, UA_UINT32_MAX),
                                            &copy);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);

    /* NULL output pointer */
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(pdsConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PDS-GetCfg");
    UA_NodeId pdsId;
    rv = UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsId).addResult;
    ck_assert_int_eq(rv, UA_STATUSCODE_GOOD);
    rv = UA_Server_getPublishedDataSetConfig(server, pdsId, NULL);
    ck_assert_int_ne(rv, UA_STATUSCODE_GOOD);
    UA_Server_removePublishedDataSet(server, pdsId);
} END_TEST

int main(void) {
    TCase *tc_add_pubsub_pds_minimal_config = tcase_create("Create PubSub PublishedDataItem with minimal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_pds_minimal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_pds_minimal_config, AddPDSWithMinimalValidConfiguration);
    tcase_add_test(tc_add_pubsub_pds_minimal_config, AddRemoveAddPDSWithMinimalValidConfiguration);

    TCase *tc_add_pubsub_pds_invalid_config = tcase_create("Create PubSub PublishedDataItem with minimal invalid config");
    tcase_add_checked_fixture(tc_add_pubsub_pds_invalid_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_pds_invalid_config, AddPDSWithNullConfig);
    tcase_add_test(tc_add_pubsub_pds_invalid_config, AddPDSWithUnsupportedType);

    TCase *tc_add_pubsub_pds_handling_utils = tcase_create("PubSub PublishedDataSet handling");
    tcase_add_checked_fixture(tc_add_pubsub_pds_handling_utils, setup, teardown);
    tcase_add_test(tc_add_pubsub_pds_handling_utils, GetPDSConfigurationAndCompareValues);
    //tcase_add_test(tc_add_pubsub_connections_maximal_config, GetMaximalConnectionConfigurationAndCompareValues);

    TCase *tc_pds_extra = tcase_create("PubSub PDS extra coverage");
    tcase_add_checked_fixture(tc_pds_extra, setup, teardown);
    tcase_add_test(tc_pds_extra, FindPDSByNameAndById_UnknownReturnsNull);
    tcase_add_test(tc_pds_extra, RemovePublishedDataSetUnknownReturnsBadNotFound);
    tcase_add_test(tc_pds_extra, DataSetFieldAddRemoveSlotReuse);
    tcase_add_test(tc_pds_extra, AddDataSetFieldEventTypeRejected);
    tcase_add_test(tc_pds_extra, AddDataSetFieldNullConfigReturnsBadInvalidArgument);
    tcase_add_test(tc_pds_extra, AddDataSetFieldVariableInvalidSourceNodeReturnsError);
    tcase_add_test(tc_pds_extra, AddDataSetFieldRejectedWhenPDSInUse);
    tcase_add_test(tc_pds_extra, GetPublishedDataSetConfigInvalidArgs);
    tcase_add_test(tc_pds_extra, RemovePdsWithWriterInEnabledGroupFails);
    tcase_add_test(tc_pds_extra, RemovePdsStopsOnVetoedWriterRemoval);
    tcase_add_test(tc_pds_extra, RemovePdsRemovesWritersInDisabledGroups);
    tcase_add_test(tc_pds_extra, DeferredWriterGroupDeleteRetriesVetoedWriter);
    tcase_add_loop_test(tc_pds_extra, WriterGroupDeletePreservesVetoedWriter, 0, 2);
    tcase_add_test(tc_pds_extra, DeferredWriterGroupDeleteRejectsNewWriters);
    tcase_add_test(tc_pds_extra, DeferredWriterGroupVetoCompletesManagerStop);

    Suite *s = suite_create("PubSub PublishedDataSets handling");
    suite_add_tcase(s, tc_add_pubsub_pds_minimal_config);
    suite_add_tcase(s, tc_add_pubsub_pds_invalid_config);
    suite_add_tcase(s, tc_add_pubsub_pds_handling_utils);
    suite_add_tcase(s, tc_pds_extra);


    //suite_add_tcase(s, tc_add_pubsub_connections_maximal_config);
    //suite_add_tcase(s, tc_decode);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
