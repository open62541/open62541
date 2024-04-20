/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017 - 2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2021 Linutronix GmbH (Author: Kurt Kanzenbach)
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include "ua_server_internal.h"
#include "test_helpers.h"

#include <check.h>

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

START_TEST(AddConnectionsWithMinimalValidConfiguration){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(&server->pubSubManager.connections.tqh_first->listEntry.tqe_next != NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 2);
} END_TEST

START_TEST(AddConnectionsWithMinimalValidIPv6Configuration){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://[ff00::1:5]:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(&server->pubSubManager.connections.tqh_first->listEntry.tqe_next != NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 2);
} END_TEST

START_TEST(AddRemoveAddConnectionWithMinimalValidConfiguration){
        UA_StatusCode retVal;
        UA_PubSubConnectionConfig connectionConfig;
        memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
        connectionConfig.name = UA_STRING("UADP Connection");
        UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
        UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                             &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
        connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
        UA_NodeId connectionIdent;
        retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
        ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
        retVal |= UA_Server_removePubSubConnection(server, connectionIdent);
        ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
        ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
        ck_assert(&server->pubSubManager.connections.tqh_first->listEntry.tqe_next != NULL);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddRemoveAddConnectionWithMinimalValidIPv6Configuration){
        UA_StatusCode retVal;
        UA_PubSubConnectionConfig connectionConfig;
        memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
        connectionConfig.name = UA_STRING("UADP Connection");
        UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://[ff00::1:5]:4840/")};
        UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                             &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
        connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
        UA_NodeId connectionIdent;
        retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
        ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
        retVal |= UA_Server_removePubSubConnection(server, connectionIdent);
        ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
        ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
        ck_assert(&server->pubSubManager.connections.tqh_first->listEntry.tqe_next != NULL);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithInvalidAddress){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://256.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
} END_TEST

START_TEST(AddConnectionWithInvalidIPv6Address){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("UADP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://[wasd::1:5]:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
} END_TEST

START_TEST(AddConnectionWithUnknownTransportURL){
        UA_StatusCode retVal;
        UA_PubSubConnectionConfig connectionConfig;
        memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
        connectionConfig.name = UA_STRING("UADP Connection");
        UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING("opc.udp://224.0.0.22:4840/")};
        UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                             &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
        connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/unknown-udp-uadp");
        UA_NodeId connectionIdent;
        retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
        ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithNullConfig){
        UA_StatusCode retVal;
        retVal = UA_Server_addPubSubConnection(server, NULL, NULL);
        ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddSingleConnectionWithMaximalConfiguration){
    UA_NetworkAddressUrlDataType networkAddressUrlData = {UA_STRING("127.0.0.1"), UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant address;
    UA_Variant_setScalar(&address, &networkAddressUrlData, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_KeyValuePair connectionOptions[3];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "ttl");
    UA_UInt32 ttl = 10;
    UA_Variant_setScalar(&connectionOptions[0].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "loopback");
    UA_Boolean loopback = UA_FALSE;
    UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Boolean reuse = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[2].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_PubSubConnectionConfig connectionConf;
    memset(&connectionConf, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConf.name = UA_STRING("UADP Connection");
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConf.enabled = true;
    connectionConf.publisherId.idType = UA_PUBLISHERIDTYPE_UINT64;
    connectionConf.publisherId.id.uint64 = 223344;
    connectionConf.connectionProperties.mapSize = 3;
    connectionConf.connectionProperties.map = connectionOptions;
    connectionConf.address = address;
    UA_NodeId connection;
    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, &connectionConf, &connection);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
} END_TEST

START_TEST(GetMaximalConnectionConfigurationAndCompareValues){
    UA_NetworkAddressUrlDataType networkAddressUrlData = {UA_STRING("127.0.0.1"), UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant address;
    UA_Variant_setScalar(&address, &networkAddressUrlData, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_KeyValuePair connectionOptions[3];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "ttl");
    UA_UInt32 ttl = 10;
    UA_Variant_setScalar(&connectionOptions[0].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "loopback");
    UA_Boolean loopback = UA_FALSE;
    UA_Variant_setScalar(&connectionOptions[1].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Boolean reuse = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[2].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_PubSubConnectionConfig connectionConf;
    memset(&connectionConf, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConf.name = UA_STRING("UADP Connection");
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    connectionConf.enabled = true;
    connectionConf.publisherId.idType = UA_PUBLISHERIDTYPE_UINT64;
    connectionConf.publisherId.id.uint64 = 223344;
    connectionConf.connectionProperties.mapSize = 3;
    connectionConf.connectionProperties.map = connectionOptions;
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
    UA_NetworkAddressUrlDataType networkAddressUrlDataCopy = *((UA_NetworkAddressUrlDataType *)connectionConfig.address.data);
    ck_assert(UA_calcSizeBinary(&networkAddressUrlDataCopy, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]) ==
              UA_calcSizeBinary(&networkAddressUrlData, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]));
    for(size_t i = 0; i < connectionConfig.connectionProperties.mapSize; i++){
        ck_assert(UA_String_equal(&connectionConfig.connectionProperties.map[i].key.name, &connectionConf.connectionProperties.map[i].key.name) == UA_TRUE);
        ck_assert(UA_Variant_calcSizeBinary(&connectionConfig.connectionProperties.map[i].value) == UA_Variant_calcSizeBinary(&connectionConf.connectionProperties.map[i].value));
    }
    UA_PubSubConnectionConfig_clear(&connectionConfig);
    } END_TEST

int main(void) {
    TCase *tc_add_pubsub_connections_minimal_config = tcase_create("Create PubSub UDP Connections with minimal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_connections_minimal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddConnectionsWithMinimalValidConfiguration);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddConnectionsWithMinimalValidIPv6Configuration);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddRemoveAddConnectionWithMinimalValidConfiguration);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddRemoveAddConnectionWithMinimalValidIPv6Configuration);

    TCase *tc_add_pubsub_connections_invalid_config = tcase_create("Create PubSub UDP Connections with invalid configurations");
    tcase_add_checked_fixture(tc_add_pubsub_connections_invalid_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithInvalidAddress);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithInvalidIPv6Address);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithUnknownTransportURL);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithNullConfig);

    TCase *tc_add_pubsub_connections_maximal_config = tcase_create("Create PubSub UDP Connections with maximal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_connections_maximal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_maximal_config, AddSingleConnectionWithMaximalConfiguration);
    tcase_add_test(tc_add_pubsub_connections_maximal_config, GetMaximalConnectionConfigurationAndCompareValues);

    Suite *s = suite_create("PubSub UDP connection creation");
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
