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

    Suite *s = suite_create("PubSub PublishedDataSets handling");
    suite_add_tcase(s, tc_add_pubsub_pds_minimal_config);
    suite_add_tcase(s, tc_add_pubsub_pds_invalid_config);
    suite_add_tcase(s, tc_add_pubsub_pds_handling_utils);


    //suite_add_tcase(s, tc_add_pubsub_connections_maximal_config);
    //suite_add_tcase(s, tc_decode);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
