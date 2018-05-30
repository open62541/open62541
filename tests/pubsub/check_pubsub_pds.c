/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_server_pubsub.h"
#include "src_generated/ua_types_generated_encoding_binary.h"
#include "ua_types.h"
#include "ua_config_default.h"
#include "ua_network_pubsub_udp.h"
#include "ua_server_internal.h"
#include "check.h"

UA_Server *server = NULL;
UA_ServerConfig *config = NULL;

static void setup(void) {
    config = UA_ServerConfig_new_default();
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
        UA_ServerConfig_delete(config);
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
    config->pubsubTransportLayersSize++;
    server = UA_Server_new(config);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
}

START_TEST(AddPDSWithMinimalValidConfiguration){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("TEST PDS 1");
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, NULL).addResult;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_NodeId newPDSNodeID;
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &newPDSNodeID).addResult;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 2);
    ck_assert_int_eq(newPDSNodeID.identifierType, UA_NODEIDTYPE_NUMERIC);
    ck_assert_int_ne(newPDSNodeID.identifier.numeric, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddRemoveAddPDSWithMinimalValidConfiguration){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("TEST PDS 1");
    UA_NodeId newPDSNodeID;
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &newPDSNodeID).addResult;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal |= UA_Server_removePublishedDataSet(server, newPDSNodeID);
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &newPDSNodeID).addResult;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddPDSWithNullConfig){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    retVal |= UA_Server_addPublishedDataSet(server, NULL, NULL).addResult;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddPDSWithUnsupportedType){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.name = UA_STRING("TEST PDS 1");
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE;
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, NULL).addResult;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS_TEMPLATE;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(GetPDSConfigurationAndCompareValues){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("TEST PDS 1");
    UA_NodeId pdsIdentifier;
    retVal |= UA_Server_addPublishedDataSet(server, &pdsConfig, &pdsIdentifier).addResult;
    ck_assert_int_eq(server->pubSubManager.publishedDataSetsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PublishedDataSetConfig pdsConfigCopy;
    memset(&pdsConfigCopy, 0, sizeof(UA_PublishedDataSetConfig));
        UA_Server_getPublishedDataSetConfig(server, pdsIdentifier, &pdsConfigCopy);
    ck_assert_int_eq(UA_String_equal(&pdsConfig.name, &pdsConfigCopy.name), UA_TRUE);
    UA_PublishedDataSetConfig_deleteMembers(&pdsConfigCopy);
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
