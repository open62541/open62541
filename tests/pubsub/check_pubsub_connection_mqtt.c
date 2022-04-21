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

#include <open62541/plugin/pubsub_mqtt.h>

#include "ua_server_internal.h"

#include <check.h>

//#define TEST_MQTT_SERVER "opc.mqtt://test.mosquitto.org:1883"
#define TEST_MQTT_SERVER "opc.mqtt://localhost:1883"

UA_Server *server = NULL;
UA_ServerConfig *config = NULL;

static void setup(void) {
    server = UA_Server_new();
    config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerMQTT());
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddConnectionsWithMinimalValidConfiguration){
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Mqtt Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING(TEST_MQTT_SERVER)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt");
    UA_StatusCode retVal =
        UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(&server->pubSubManager.connections.tqh_first->listEntry.tqe_next != NULL);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 2);
} END_TEST

START_TEST(AddRemoveAddConnectionWithMinimalValidConfiguration){
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Mqtt Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING(TEST_MQTT_SERVER)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt");
    UA_NodeId connectionIdent;
    UA_StatusCode retVal =
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
    retVal |= UA_Server_removePubSubConnection(server, connectionIdent);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithInvalidAddress){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("MQTT Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING("opc.mqtt://127.0..1:1883/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-invalid");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
} END_TEST

START_TEST(AddConnectionWithUnknownTransportURL){
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
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithNullConfig){
    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, NULL, NULL);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddSingleConnectionWithMaximalConfiguration){
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
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt");
    connectionConf.enabled = true;
    connectionConf.publisherId.numeric = 223344;
    connectionConf.connectionPropertiesSize = 3;
    connectionConf.connectionProperties = connectionOptions;
    connectionConf.address = address;
    UA_NodeId connection;
    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, &connectionConf, &connection);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
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
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt");
    connectionConf.enabled = true;
    connectionConf.publisherId.numeric = 223344;
    connectionConf.connectionPropertiesSize = 3;
    connectionConf.connectionProperties = connectionOptions;
    connectionConf.address = address;
    UA_NodeId connection;
    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, &connectionConf, &connection);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    retVal |= UA_Server_getPubSubConnectionConfig(server, connection, &connectionConfig);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(connectionConfig.connectionPropertiesSize == connectionConf.connectionPropertiesSize);
    ck_assert(UA_String_equal(&connectionConfig.name, &connectionConf.name) == UA_TRUE);
    ck_assert(UA_String_equal(&connectionConfig.transportProfileUri, &connectionConf.transportProfileUri) == UA_TRUE);
    UA_NetworkAddressUrlDataType networkAddressUrlDataCopy =
        *((UA_NetworkAddressUrlDataType *)connectionConfig.address.data);
    ck_assert(UA_calcSizeBinary(&networkAddressUrlDataCopy,
                                &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]) ==
              UA_calcSizeBinary(&networkAddressUrlData,
                                &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]));
    for(size_t i = 0; i < connectionConfig.connectionPropertiesSize; i++){
        ck_assert(UA_String_equal(&connectionConfig.connectionProperties[i].key.name,
                                  &connectionConf.connectionProperties[i].key.name) == UA_TRUE);
        ck_assert(UA_Variant_calcSizeBinary(&connectionConfig.connectionProperties[i].value) ==
                  UA_Variant_calcSizeBinary(&connectionConf.connectionProperties[i].value));
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
