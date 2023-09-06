/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include <open62541/server_config_default.h>
#include <open62541/types_generated.h>

#include "ua_server_internal.h"
#include "ua_pubsub.h"
#include "test_helpers.h"

#include <linux/if_packet.h>
#include <netinet/ether.h>

#include <linux/bpf.h>
#include <linux/if_link.h>
#include <sys/resource.h>
#include <sys/mman.h>

#include <check.h>

#define             MULTICAST_MAC_ADDRESS              "opc.eth://01-00-5E-00-00-01"
#define             RECEIVE_QUEUE_0                    0
#define             RECEIVE_QUEUE_1                    1
#define             RECEIVE_QUEUE_2                    2
#define             RECEIVE_QUEUE_3                    3
#define             XDP_FLAG                           XDP_FLAGS_SKB_MODE

UA_Server *server = NULL;
UA_String ethernetInterface;

static void setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    UA_Server_run_startup(server);
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

static void
usage(char *progname) {
    printf("usage: %s <ethernet_interface> \n", progname);
    printf("Provide the Interface parameter to run the application. Exiting \n");
}

START_TEST(AddConnectionsWithMinimalValidConfiguration){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("XDP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {ethernetInterface, UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    /* Connection options are given as Key/Value Pairs. */
    UA_KeyValuePair connectionOptions[3];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "enableXdpSocket");
    UA_Boolean enableXdp = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[0].value, &enableXdp, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "xdpflag");
    UA_UInt32 flags = XDP_FLAG;
    UA_Variant_setScalar(&connectionOptions[1].value, &flags, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "hwreceivequeue");
    UA_UInt32 rxqueue = RECEIVE_QUEUE_2;
    UA_Variant_setScalar(&connectionOptions[2].value, &rxqueue, &UA_TYPES[UA_TYPES_UINT32]);
    connectionConfig.connectionProperties.mapSize = 3;
    connectionConfig.connectionProperties.map = connectionOptions;
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
    rxqueue = RECEIVE_QUEUE_1;
    UA_Variant_setScalar(&connectionOptions[1].value, &rxqueue, &UA_TYPES[UA_TYPES_UINT32]);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(&server->pubSubManager.connections.tqh_first->listEntry.tqe_next != NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 2);
} END_TEST


START_TEST(AddRemoveAddConnectionWithMinimalValidConfiguration){
        UA_StatusCode retVal;
        UA_PubSubConnectionConfig connectionConfig;
        memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
        connectionConfig.name = UA_STRING("XDP Connection");
        UA_NetworkAddressUrlDataType networkAddressUrl = {ethernetInterface, UA_STRING(MULTICAST_MAC_ADDRESS)};
        UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                             &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
        connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
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

START_TEST(AddConnectionsWithInvalidConfiguration){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("XDP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {ethernetInterface, UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    /* Connection options are given as Key/Value Pairs.*/
    UA_KeyValuePair connectionOptions[3];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "enableXdpSocket");
    UA_Boolean enableXdp = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[0].value, &enableXdp, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "xdpflag");
    UA_UInt32 flags = XDP_FLAG;
    UA_Variant_setScalar(&connectionOptions[1].value, &flags, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "hwreceivequeue");
    UA_UInt32 rxqueue = RECEIVE_QUEUE_2;
    UA_Variant_setScalar(&connectionOptions[2].value, &rxqueue, &UA_TYPES[UA_TYPES_UINT32]);
    connectionConfig.connectionProperties.map = connectionOptions;
    connectionConfig.connectionProperties.mapSize = 3;
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
} END_TEST

START_TEST(AddConnectionWithInvalidAddress){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("XDP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {ethernetInterface, UA_STRING("opc.eth://a0:36:9f:04:5b:11")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 0);
} END_TEST

START_TEST(AddConnectionWithInvalidInterface){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("XDP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING("eth0"), UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
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
        connectionConfig.name = UA_STRING("XDP Connection");
        UA_NetworkAddressUrlDataType networkAddressUrl = {ethernetInterface, UA_STRING(MULTICAST_MAC_ADDRESS)};
        UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                             &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
        connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/unknown-eth-uadp");
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
    UA_NetworkAddressUrlDataType networkAddressUrlData = {ethernetInterface, UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant address;
    UA_Variant_setScalar(&address, &networkAddressUrlData, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_KeyValuePair connectionOptions[4];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "enableXdpSocket");
    UA_Boolean enableXdp = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[0].value, &enableXdp, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "ttl");
    UA_UInt32 ttl = 10;
    UA_Variant_setScalar(&connectionOptions[1].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "loopback");
    UA_Boolean loopback = UA_FALSE;
    UA_Variant_setScalar(&connectionOptions[2].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[3].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Boolean reuse = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[3].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_PubSubConnectionConfig connectionConf;
    memset(&connectionConf, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConf.name = UA_STRING("XDP Connection");
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    connectionConf.enabled = true;
    connectionConf.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConf.publisherId.uint32 = 223344;
    connectionConf.connectionProperties.mapSize = 4;
    connectionConf.connectionProperties.map = connectionOptions;
    connectionConf.address = address;
    UA_NodeId connection;
    UA_StatusCode retVal = UA_Server_addPubSubConnection(server, &connectionConf, &connection);
    ck_assert_uint_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
} END_TEST

START_TEST(GetMaximalConnectionConfigurationAndCompareValues){
    UA_NetworkAddressUrlDataType networkAddressUrlData = {ethernetInterface, UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant address;
    UA_Variant_setScalar(&address, &networkAddressUrlData, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    UA_KeyValuePair connectionOptions[5];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "enableXdpSocket");
    UA_Boolean enableXdp = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[0].value, &enableXdp, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "hwreceivequeue");
    UA_UInt32 rxqueue = RECEIVE_QUEUE_2;
    UA_Variant_setScalar(&connectionOptions[1].value, &rxqueue, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[2].key = UA_QUALIFIEDNAME(0, "ttl");
    UA_UInt32 ttl = 10;
    UA_Variant_setScalar(&connectionOptions[2].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[3].key = UA_QUALIFIEDNAME(0, "loopback");
    UA_Boolean loopback = UA_FALSE;
    UA_Variant_setScalar(&connectionOptions[3].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionOptions[4].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Boolean reuse = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[4].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_PubSubConnectionConfig connectionConf;
    memset(&connectionConf, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConf.name = UA_STRING("XDP Connection");
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    connectionConf.enabled = true;
    connectionConf.publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
    connectionConf.publisherId.uint32 = 223344;
    connectionConf.connectionProperties.mapSize = 5;
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

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }
    else {
        if (strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        ethernetInterface = UA_STRING(argv[1]);
    }

    TCase *tc_add_pubsub_connections_minimal_config = tcase_create("Create PubSub XDP Connections with minimal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_connections_minimal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddConnectionsWithMinimalValidConfiguration);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddRemoveAddConnectionWithMinimalValidConfiguration);

    TCase *tc_add_pubsub_connections_invalid_config = tcase_create("Create PubSub XDP Connections with invalid configurations");
    tcase_add_checked_fixture(tc_add_pubsub_connections_invalid_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_minimal_config, AddConnectionsWithInvalidConfiguration);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithInvalidAddress);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithInvalidInterface);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithUnknownTransportURL);
    tcase_add_test(tc_add_pubsub_connections_invalid_config, AddConnectionWithNullConfig);

    TCase *tc_add_pubsub_connections_maximal_config = tcase_create("Create PubSub XDP Connections with maximal valid config");
    tcase_add_checked_fixture(tc_add_pubsub_connections_maximal_config, setup, teardown);
    tcase_add_test(tc_add_pubsub_connections_maximal_config, AddSingleConnectionWithMaximalConfiguration);
    tcase_add_test(tc_add_pubsub_connections_maximal_config, GetMaximalConnectionConfigurationAndCompareValues);

    Suite *s = suite_create("PubSub XDP connection creation");
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


