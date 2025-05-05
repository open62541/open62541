/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "test_helpers.h"
#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

//#define TEST_MQTT_SERVER "opc.mqtt://test.mosquitto.org:1883"
#define TEST_MQTT_SERVER "opc.mqtt://localhost:1883"

UA_Server *server = NULL;
UA_ServerConfig *config = NULL;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddConnectionsWithMinimalValidConfiguration){
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Mqtt Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING(TEST_MQTT_SERVER)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    UA_StatusCode retVal =
        UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&psm->connections));
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(&psm->connections.tqh_first->listEntry.tqe_next != NULL);
    ck_assert_uint_eq(psm->connectionsSize, 2);
} END_TEST

START_TEST(AddRemoveAddConnectionWithMinimalValidConfiguration){
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Mqtt Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING(TEST_MQTT_SERVER)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    UA_NodeId connectionIdent;
    UA_StatusCode retVal =
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&psm->connections));
    retVal |= UA_Server_removePubSubConnection(server, connectionIdent);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert(! TAILQ_EMPTY(&psm->connections));
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithInvalidAddress) {
    UA_PubSubManager *psm = getPSM(server);
    /* This succeeds as the connection is only actually connected when there
     * is a Reader/WriterGroup */
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("MQTT Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING("opc.mqtt://127.0..1:1883/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");

    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);

    ck_assert_uint_eq(psm->connectionsSize, 2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithUnknownTransportURL){
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("MQTT Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING(TEST_MQTT_SERVER)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/unknown-udp-uadp");
    UA_NodeId connectionIdent;
    UA_StatusCode retVal =
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithNullConfig){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, NULL, NULL);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddSingleConnectionWithMaximalConfiguration){
    UA_PubSubManager *psm = getPSM(server);
    UA_NetworkAddressUrlDataType networkAddressUrlData =
        {UA_STRING("127.0.0.1"), UA_STRING(TEST_MQTT_SERVER)};
    UA_Variant address;
    UA_Variant_setScalar(&address, &networkAddressUrlData, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_KeyValuePair connectionOptions[3];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "sendBufferSize");
    UA_UInt32 sBs = 1000;
    UA_Variant_setScalar(&connectionOptions[0].value, &sBs, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "recvBufferSize");
    UA_UInt32 rBs = 1000;
    UA_Variant_setScalar(&connectionOptions[1].value, &rBs, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "mqttClientId");
    UA_String id = UA_STRING("client");
    UA_Variant_setScalar(&connectionOptions[2].value, &id, &UA_TYPES[UA_TYPES_STRING]);

    UA_PubSubConnectionConfig connectionConf;
    memset(&connectionConf, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConf.name = UA_STRING("MQTT Connection");
    connectionConf.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    connectionConf.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConf.publisherId.id.uint32 = 223344;
    connectionConf.connectionProperties.map = connectionOptions;
    connectionConf.connectionProperties.mapSize = 3;
    connectionConf.address = address;
    UA_NodeId connection;
    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, &connectionConf, &connection);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&psm->connections));
} END_TEST

START_TEST(GetMaximalConnectionConfigurationAndCompareValues){
    UA_NetworkAddressUrlDataType networkAddressUrlData =
        {UA_STRING("127.0.0.1"), UA_STRING(TEST_MQTT_SERVER)};
    UA_Variant address;
    UA_Variant_setScalar(&address, &networkAddressUrlData, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_KeyValuePair connectionOptions[3];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "sendBufferSize");
    UA_UInt32 sBs = 1000;
    UA_Variant_setScalar(&connectionOptions[0].value, &sBs, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "recvBufferSize");
    UA_UInt32 rBs = 1000;
    UA_Variant_setScalar(&connectionOptions[1].value, &rBs, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "mqttClientId");
    UA_String id = UA_STRING("client");
    UA_Variant_setScalar(&connectionOptions[2].value, &id, &UA_TYPES[UA_TYPES_STRING]);

    UA_PubSubConnectionConfig connectionConf;
    memset(&connectionConf, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConf.name = UA_STRING("MQTT Connection");
    connectionConf.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    connectionConf.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConf.publisherId.id.uint32 = 223344;
    connectionConf.connectionProperties.map = connectionOptions;
    connectionConf.connectionProperties.mapSize = 3;
    connectionConf.address = address;
    UA_NodeId connection;
    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, &connectionConf, &connection);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    retVal |= UA_Server_getPubSubConnectionConfig(server, connection, &connectionConfig);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(connectionConfig.connectionProperties.mapSize == connectionConf.connectionProperties.mapSize);
    ck_assert(UA_String_equal(&connectionConfig.name, &connectionConf.name) == UA_TRUE);
    ck_assert(UA_String_equal(&connectionConfig.transportProfileUri, &connectionConf.transportProfileUri) == UA_TRUE);
    UA_NetworkAddressUrlDataType networkAddressUrlDataCopy =
        *((UA_NetworkAddressUrlDataType *)connectionConfig.address.data);
    ck_assert(UA_calcSizeBinary(&networkAddressUrlDataCopy,
                                &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE], NULL) ==
              UA_calcSizeBinary(&networkAddressUrlData,
                                &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE], NULL));
    for(size_t i = 0; i < connectionConfig.connectionProperties.mapSize; i++){
        ck_assert(UA_String_equal(&connectionConfig.connectionProperties.map[i].key.name,
                                  &connectionConf.connectionProperties.map[i].key.name) == UA_TRUE);
        ck_assert(UA_Variant_calcSizeBinary(&connectionConfig.connectionProperties.map[i].value) ==
                  UA_Variant_calcSizeBinary(&connectionConf.connectionProperties.map[i].value));
    }
    UA_PubSubConnectionConfig_clear(&connectionConfig);
} END_TEST

int main(void) {
    TCase *tc_add_pubsub_connections_minimal_config =
        tcase_create("Create PubSub Mqtt Connections with minimal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_connections_minimal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_minimal_config,
                   AddConnectionsWithMinimalValidConfiguration);
    tcase_add_test(tc_add_pubsub_connections_minimal_config,
                   AddRemoveAddConnectionWithMinimalValidConfiguration);

    TCase *tc_add_pubsub_connections_invalid_config =
        tcase_create("Create PubSub Mqtt Connections with invalid configurations");
    tcase_add_checked_fixture(tc_add_pubsub_connections_invalid_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_invalid_config,
                   AddConnectionWithInvalidAddress);
    tcase_add_test(tc_add_pubsub_connections_invalid_config,
                   AddConnectionWithUnknownTransportURL);
    tcase_add_test(tc_add_pubsub_connections_invalid_config,
                   AddConnectionWithNullConfig);

    TCase *tc_add_pubsub_connections_maximal_config =
        tcase_create("Create PubSub Mqtt Connections with maximal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_connections_maximal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_maximal_config,
                   AddSingleConnectionWithMaximalConfiguration);
    tcase_add_test(tc_add_pubsub_connections_maximal_config,
                   GetMaximalConnectionConfigurationAndCompareValues);

    Suite *s = suite_create("PubSub Mqtt connection creation");
    suite_add_tcase(s, tc_add_pubsub_connections_minimal_config);
    suite_add_tcase(s, tc_add_pubsub_connections_invalid_config);
    suite_add_tcase(s, tc_add_pubsub_connections_maximal_config);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
