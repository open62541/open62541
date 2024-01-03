/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "test_helpers.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdio.h>
#include <time.h>

UA_Server *server = NULL;
UA_NodeId connection1, writerGroup1, publishedDataSet1, dataSetWriter1;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Server_run_startup(server);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retval |= UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    retval |= UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);
    retval |= UA_Server_enableWriterGroup(server, writerGroup1);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet 1");
    retval |= UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1).addResult;
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(PublishSpeedTest) {
    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_StatusCode retval = UA_Server_addDataSetField(server, publishedDataSet1, &dataSetFieldConfig, NULL).result;
    ck_assert_int_eq(retval, UA_STATUSCODE_GOOD);

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup1);

    printf("start sending 8000 publish messages via UDP\n");

    clock_t begin, finish;
    begin = clock();

    for(int i = 0; i < 8000; i++) {
        UA_WriterGroup_publishCallback(server, wg);
    }

    finish = clock();
    double time_spent = (double)(finish - begin) / CLOCKS_PER_SEC;
    printf("duration was %f s\n", time_spent);

} END_TEST

int main(void) {
    TCase *tc_publishspeed = tcase_create("Speed of the publisher");
    tcase_add_checked_fixture(tc_publishspeed, setup, teardown);
    tcase_add_test(tc_publishspeed, PublishSpeedTest);

    Suite *s = suite_create("PubSub Speed Test");
    suite_add_tcase(s, tc_publishspeed);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
