/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/log.h>
#include <open62541/types_generated.h>

#include "test_helpers.h"
#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"

#include <check.h>
#include <stdlib.h>

/* Adjust your configuration globally for the ethernet tests here: */
#include "ethernet_config.h"

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
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Ethernet Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");

    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert(! TAILQ_EMPTY(&psm->connections));

    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(&psm->connections.tqh_first->listEntry.tqe_next != NULL);
    ck_assert_uint_eq(psm->connectionsSize, 2);
} END_TEST


START_TEST(AddRemoveAddConnectionWithMinimalValidConfiguration){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Ethernet Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    UA_NodeId connectionIdent;
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&psm->connections));
    retVal |= UA_Server_removePubSubConnection(server, connectionIdent);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_uint_eq(psm->connectionsSize, 1);
    ck_assert(&psm->connections.tqh_first->listEntry.tqe_next != NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithInvalidAddress){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Ethernet Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING("opc.eth://a0:36:9f:04:5b:11")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->connectionsSize, 0);
} END_TEST

START_TEST(AddConnectionWithInvalidInterface){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Ethernet Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(psm->connectionsSize, 0);
} END_TEST

START_TEST(AddConnectionWithUnknownTransportURL){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("Ethernet Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/unknown-eth-uadp");
    UA_NodeId connectionIdent;
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithNullConfig){
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode retVal;
    retVal = UA_Server_addPubSubConnection(server, NULL, NULL);
    ck_assert_uint_eq(psm->connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddSingleConnectionWithMaximalConfiguration){
    UA_PubSubManager *psm = getPSM(server);
    UA_NetworkAddressUrlDataType networkAddressUrlData = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(MULTICAST_MAC_ADDRESS)};
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
    connectionConf.name = UA_STRING("Ethernet Connection");
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
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
    UA_NetworkAddressUrlDataType networkAddressUrlData = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(MULTICAST_MAC_ADDRESS)};
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
    connectionConf.name = UA_STRING("Ethernet Connection");
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
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
    UA_NetworkAddressUrlDataType networkAddressUrlDataCopy = *((UA_NetworkAddressUrlDataType *)connectionConfig.address.data);
    ck_assert(UA_calcSizeBinary(&networkAddressUrlDataCopy, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE], NULL) ==
              UA_calcSizeBinary(&networkAddressUrlData, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE], NULL));
    for(size_t i = 0; i < connectionConfig.connectionProperties.mapSize; i++){
        ck_assert(UA_String_equal(&connectionConfig.connectionProperties.map[i].key.name, &connectionConf.connectionProperties.map[i].key.name) == UA_TRUE);
        ck_assert(UA_Variant_calcSizeBinary(&connectionConfig.connectionProperties.map[i].value) == UA_Variant_calcSizeBinary(&connectionConf.connectionProperties.map[i].value));
    }
    UA_PubSubConnectionConfig_clear(&connectionConfig);
} END_TEST

int main(void) {
    char *skip_eth = SKIP_ETHERNET;
    if(skip_eth && strlen(skip_eth) > 0)
        return EXIT_SUCCESS;

    TCase *tc_add_pubsub_connections_minimal_config = tcase_create("Create PubSub Ethernet Connections with minimal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_connections_minimal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddConnectionsWithMinimalValidConfiguration);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddRemoveAddConnectionWithMinimalValidConfiguration);

    TCase *tc_add_pubsub_connections_invalid_config = tcase_create("Create PubSub Ethernet Connections with invalid configurations");
    tcase_add_checked_fixture(tc_add_pubsub_connections_invalid_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithInvalidAddress);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithInvalidInterface);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithUnknownTransportURL);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithNullConfig);

    TCase *tc_add_pubsub_connections_maximal_config = tcase_create("Create PubSub Ethernet Connections with maximal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_connections_maximal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_maximal_config, AddSingleConnectionWithMaximalConfiguration);
    tcase_add_test(tc_add_pubsub_connections_maximal_config, GetMaximalConnectionConfigurationAndCompareValues);

    Suite *s = suite_create("PubSub Ethernet connection creation");
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

