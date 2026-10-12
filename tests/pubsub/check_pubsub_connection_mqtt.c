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

UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
static UA_ConnectionManager *overriddenMqtt = NULL;
static UA_StatusCode (*savedMqttOpen)(UA_ConnectionManager*, const UA_KeyValueMap*, void*, void*, UA_ConnectionManager_connectionCallback);

static char* get_mqtt_broker_address(void) {
    char* broker = getenv("OPEN62541_TEST_MQTT_BROKER");
    if (!broker) broker = "opc.mqtt://127.0.0.1:1883";
    return broker;
}

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    if(overriddenMqtt) {
        overriddenMqtt->openConnection = savedMqttOpen;
        overriddenMqtt = NULL;
        savedMqttOpen = NULL;
    }
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST(AddConnectionsWithMinimalValidConfiguration){
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Mqtt Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl =
        {UA_STRING_NULL, UA_STRING(get_mqtt_broker_address())};
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
        {UA_STRING_NULL, UA_STRING(get_mqtt_broker_address())};
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
        {UA_STRING_NULL, UA_STRING(get_mqtt_broker_address())};
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
        {UA_STRING_NULL, UA_STRING(get_mqtt_broker_address())};
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
        {UA_STRING("127.0.0.1"), UA_STRING(get_mqtt_broker_address())};
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
        {UA_STRING("127.0.0.1"), UA_STRING(get_mqtt_broker_address())};
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

START_TEST(MqttConnectionPropertiesForwarded) {
    UA_KeyValueMap properties = UA_KEYVALUEMAP_NULL;
    UA_KeyValueMap params = UA_KEYVALUEMAP_NULL;
    UA_String clientId = UA_STRING("client");
    UA_String username = UA_STRING("user");
    UA_String password = UA_STRING("password");
    UA_String ca = UA_STRING("ca.pem");
    UA_Boolean tls = true;
    UA_KeyValueMap_setScalar(&properties, UA_QUALIFIEDNAME(0, "mqttClientId"), &clientId, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(&properties, UA_QUALIFIEDNAME(0, "mqttUsername"), &username, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(&properties, UA_QUALIFIEDNAME(0, "mqttPassword"), &password, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(&properties, UA_QUALIFIEDNAME(0, "mqttCaFilePath"), &ca, &UA_TYPES[UA_TYPES_STRING]);
    UA_KeyValueMap_setScalar(&properties, UA_QUALIFIEDNAME(0, "mqttUseTLS"), &tls, &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_uint_eq(UA_PubSubConnection_mqttProperties(&properties, &params), UA_STATUSCODE_GOOD);
    const char *keys[] = {"client-id", "username", "password", "ca-file-path"};
    UA_String *values[] = {&clientId, &username, &password, &ca};
    for(size_t i = 0; i < 4; i++) {
        const UA_String *value = (const UA_String*)UA_KeyValueMap_getScalar(
            &params, UA_QUALIFIEDNAME(0, (char*)(uintptr_t)keys[i]), &UA_TYPES[UA_TYPES_STRING]);
        ck_assert_ptr_ne(value, NULL);
        ck_assert(UA_String_equal(value, values[i]));
    }
    const UA_Boolean *enabled = (const UA_Boolean*)UA_KeyValueMap_getScalar(
        &params, UA_QUALIFIEDNAME(0, "useSSL"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_ptr_ne(enabled, NULL);
    ck_assert(*enabled);
    UA_String other = UA_STRING("other-user");
    UA_KeyValueMap_setScalar(&properties, UA_QUALIFIEDNAME(0, "username"), &other, &UA_TYPES[UA_TYPES_STRING]);
    ck_assert_uint_eq(UA_PubSubConnection_mqttProperties(&properties, &params), UA_STATUSCODE_BADINVALIDARGUMENT);
    UA_KeyValueMap_clear(&params);
    UA_KeyValueMap_clear(&properties);
} END_TEST

static unsigned mqttValidationCalls;
static UA_Boolean mqttSubscribe;

static UA_StatusCode
checkMqttParameters(UA_ConnectionManager *cm, const UA_KeyValueMap *params, void *application,
                    void *context, UA_ConnectionManager_connectionCallback callback) {
    const char *keys[] = {"client-id", "username", "password", "ca-file-path"};
    const char *values[] = {"client", "user", "password", "ca.pem"};
    for(size_t i = 0; i < 4; i++) {
        const UA_String *value = (const UA_String*)UA_KeyValueMap_getScalar(
            params, UA_QUALIFIEDNAME(0, (char*)(uintptr_t)keys[i]), &UA_TYPES[UA_TYPES_STRING]);
        UA_String expected = UA_STRING((char*)(uintptr_t)values[i]);
        ck_assert_ptr_ne(value, NULL);
        ck_assert(UA_String_equal(value, &expected));
    }
    const UA_Boolean *tls = (const UA_Boolean*)UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "useSSL"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *validate = (const UA_Boolean*)UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "validate"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    const UA_Boolean *subscribe = (const UA_Boolean*)UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "subscribe"), &UA_TYPES[UA_TYPES_BOOLEAN]);
    ck_assert_ptr_ne(tls, NULL);
    ck_assert_ptr_ne(validate, NULL);
    ck_assert_ptr_ne(subscribe, NULL);
    ck_assert(*tls && *validate);
    ck_assert_uint_eq(*subscribe, mqttSubscribe);
    mqttValidationCalls++;
    return UA_STATUSCODE_GOOD;
}

START_TEST(MqttGroupPropertiesReachTransport) {
    UA_EventLoop *loop = UA_Server_getConfig(server)->eventLoop;
    UA_ConnectionManager *mqtt = NULL;
    UA_String protocol = UA_STRING("mqtt");
    for(UA_EventSource *es = loop->eventSources; es; es = es->next) {
        if(es->eventSourceType == UA_EVENTSOURCETYPE_CONNECTIONMANAGER &&
           UA_String_equal(&((UA_ConnectionManager*)es)->protocol, &protocol)) {
            mqtt = (UA_ConnectionManager*)es;
            break;
        }
    }
    ck_assert_ptr_ne(mqtt, NULL);
    overriddenMqtt = mqtt;
    savedMqttOpen = mqtt->openConnection;
    mqtt->openConnection = checkMqttParameters;
    mqttValidationCalls = 0;
    mqttSubscribe = (_i == 0);

    UA_PubSubConnectionConfig connection = {0};
    connection.name = UA_STRING("MQTT properties");
    connection.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    UA_NetworkAddressUrlDataType address = {UA_STRING_NULL, UA_STRING("opc.mqtt://broker.example:8883")};
    UA_Variant_setScalar(&connection.address, &address, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    const char *keys[] = {"mqttClientId", "mqttUsername", "mqttPassword", "mqttCaFilePath"};
    const char *values[] = {"client", "user", "password", "ca.pem"};
    for(size_t i = 0; i < 4; i++) {
        UA_String value = UA_STRING((char*)(uintptr_t)values[i]);
        ck_assert_uint_eq(UA_KeyValueMap_setScalar(&connection.connectionProperties,
                         UA_QUALIFIEDNAME(0, (char*)(uintptr_t)keys[i]), &value, &UA_TYPES[UA_TYPES_STRING]), UA_STATUSCODE_GOOD);
    }
    UA_Boolean tls = true;
    UA_KeyValueMap_setScalar(&connection.connectionProperties, UA_QUALIFIEDNAME(0, "mqttUseTLS"), &tls, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_NodeId connectionId;
    ck_assert_uint_eq(UA_Server_addPubSubConnection(server, &connection, &connectionId), UA_STATUSCODE_GOOD);
    UA_KeyValueMap_clear(&connection.connectionProperties);
    if(mqttSubscribe) {
        UA_ReaderGroupConfig reader = {0};
        reader.name = UA_STRING("Reader");
        reader.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        UA_BrokerDataSetReaderTransportDataType transport = {0};
        transport.queueName = UA_STRING("topic");
        UA_ExtensionObject_setValueNoDelete(&reader.transportSettings, &transport, &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE]);
        ck_assert_uint_eq(UA_Server_addReaderGroup(server, connectionId, &reader, NULL), UA_STATUSCODE_GOOD);
    } else {
        UA_WriterGroupConfig writer = {0};
        writer.name = UA_STRING("Writer");
        writer.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writer.publishingInterval = 100;
        UA_BrokerWriterGroupTransportDataType transport = {0};
        transport.queueName = UA_STRING("topic");
        UA_ExtensionObject_setValueNoDelete(&writer.transportSettings, &transport, &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE]);
        ck_assert_uint_eq(UA_Server_addWriterGroup(server, connectionId, &writer, NULL), UA_STATUSCODE_GOOD);
    }
    ck_assert_uint_eq(mqttValidationCalls, 1);
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

    tcase_add_test(tc_add_pubsub_connections_minimal_config, MqttConnectionPropertiesForwarded);
    tcase_add_loop_test(tc_add_pubsub_connections_minimal_config, MqttGroupPropertiesReachTransport, 0, 2);

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
