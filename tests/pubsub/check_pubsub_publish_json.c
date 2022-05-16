/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <open62541/types.h>

#include "ua_pubsub.h"
#include "ua_server_internal.h"

#include <check.h>

UA_Server *server = NULL;
UA_NodeId connection1, writerGroup1, publishedDataSet1, dataSetWriter1;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

    UA_Server_run_startup(server);
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection1);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(SinglePublishDataSetField){
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));

    writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    writerGroupConfig.messageSettings.content.decoded.type =
        &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE];
    UA_JsonDataSetWriterMessageDataType d;
    d.dataSetMessageContentMask = UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER;
    writerGroupConfig.messageSettings.content.decoded.data = &d;

    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
    UA_StatusCode retVal =
        UA_Server_addWriterGroup(server, connection1, &writerGroupConfig, &writerGroup1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_setWriterGroupOperational(server, writerGroup1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_PublishedDataSetConfig pdsConfig;
    memset(&pdsConfig, 0, sizeof(UA_PublishedDataSetConfig));
    pdsConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    pdsConfig.name = UA_STRING("PublishedDataSet 1");
    UA_AddPublishedDataSetResult result =
        UA_Server_addPublishedDataSet(server, &pdsConfig, &publishedDataSet1);
    ck_assert_int_eq(result.addResult, UA_STATUSCODE_GOOD);

    UA_DataSetFieldConfig dataSetFieldConfig;
    memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
    dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dataSetFieldConfig.field.variable.fieldNameAlias = UA_STRING("Server localtime");
    dataSetFieldConfig.field.variable.promotedField = UA_FALSE;
    dataSetFieldConfig.field.variable.publishParameters.publishedVariable =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_STATE);
    dataSetFieldConfig.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
    UA_DataSetFieldResult dsFieldResult =
        UA_Server_addDataSetField(server, publishedDataSet1, &dataSetFieldConfig, NULL);
    ck_assert_int_eq(dsFieldResult.result, UA_STATUSCODE_GOOD);

    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(dataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("DataSetWriter 1");
    retVal = UA_Server_addDataSetWriter(server, writerGroup1, publishedDataSet1,
                                        &dataSetWriterConfig, &dataSetWriter1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup1);
    ck_assert(wg != 0);
    UA_WriterGroup_publishCallback(server, wg);
} END_TEST

int main(void) {
    TCase *tc_pubsub_publish = tcase_create("PubSub publish");
    tcase_add_checked_fixture(tc_pubsub_publish, setup, teardown);
    tcase_add_test(tc_pubsub_publish, SinglePublishDataSetField);

    Suite *s = suite_create("PubSub publishing json via udp");
    suite_add_tcase(s, tc_pubsub_publish);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
