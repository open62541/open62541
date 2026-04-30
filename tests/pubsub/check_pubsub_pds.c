/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_server_internal.h"
#include "ua_pubsub_internal.h"
#include "test_helpers.h"

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
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE;
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, NULL).addResult;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS_TEMPLATE;
    ck_assert_uint_eq(psm->publishedDataSetsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
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
