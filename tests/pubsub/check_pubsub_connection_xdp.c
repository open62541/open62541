/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019-2020 Kalycito Infotech Private Limited
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <ua_server_internal.h>
#include <open62541/types_generated_encoding_binary.h>
#include <open62541/types_generated.h>
#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/plugin/pubsub_ethernet_xdp.h>
#include "ua_pubsub.h"

#include <linux/if_packet.h>
#include <netinet/ether.h>

#include <linux/bpf.h>
#include <linux/if_link.h>
#include "/usr/local/src/bpf-next/include/uapi/linux/if_xdp.h"
#include "/usr/local/src/bpf-next/tools/testing/selftests/bpf/bpf_util.h"
#include "/usr/local/src/bpf-next/tools/lib/bpf/bpf.h"
#include "/usr/local/src/bpf-next/tools/lib/bpf/libbpf.h"
#include "/usr/local/src/bpf-next/samples/bpf/xdpsock.h"
#include <sys/resource.h>
#include <sys/mman.h>

#include <check.h>

#define             MULTICAST_MAC_ADDRESS              "opc.eth://01-00-5E-00-00-01"
#define             RECEIVE_QUEUE                      2
#define             XDP_FLAG                           XDP_FLAGS_SKB_MODE

UA_Server *server = NULL;
UA_String ethernetInterface;

static void setup(void) {
    server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setDefault(config);

    config->pubsubTransportLayers = (UA_PubSubTransportLayer *)
        UA_malloc(sizeof(UA_PubSubTransportLayer));
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerEthernetXDP();
    config->pubsubTransportLayersSize++;
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
    UA_KeyValuePair connectionOptions[2];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "xdpflag");
    UA_UInt32 flags = XDP_FLAG;
    UA_Variant_setScalar(&connectionOptions[0].value, &flags, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "hwreceivequeue");
    UA_UInt32 rxqueue = RECEIVE_QUEUE;
    UA_Variant_setScalar(&connectionOptions[1].value, &rxqueue, &UA_TYPES[UA_TYPES_UINT32]);
    connectionConfig.connectionProperties = connectionOptions;
    connectionConfig.connectionPropertiesSize = 2;
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert(&server->pubSubManager.connections.tqh_first->listEntry.tqe_next != NULL);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 2);
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
        ck_assert_int_eq(server->pubSubManager.connectionsSize, 1);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        ck_assert(! TAILQ_EMPTY(&server->pubSubManager.connections));
        retVal |= UA_Server_removePubSubConnection(server, connectionIdent);
        ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
        retVal = UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
        ck_assert_int_eq(server->pubSubManager.connectionsSize, 1);
        ck_assert(&server->pubSubManager.connections.tqh_first->listEntry.tqe_next != NULL);
        ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
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
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
} END_TEST

START_TEST(AddConnectionWithInvalidInterface){
    UA_StatusCode retVal;
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING("XDP Connection");
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING(MULTICAST_MAC_ADDRESS)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_addPubSubConnection(server, &connectionConfig, NULL);
    ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
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
        ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(AddConnectionWithNullConfig){
        UA_StatusCode retVal;
        retVal = UA_Server_addPubSubConnection(server, NULL, NULL);
        ck_assert_int_eq(server->pubSubManager.connectionsSize, 0);
        ck_assert_int_ne(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST(AddSingleConnectionWithMaximalConfiguration){
    UA_NetworkAddressUrlDataType networkAddressUrlData = {ethernetInterface, UA_STRING(MULTICAST_MAC_ADDRESS)};
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
    connectionConf.name = UA_STRING("XDP Connection");
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
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
    UA_NetworkAddressUrlDataType networkAddressUrlData = {ethernetInterface, UA_STRING(MULTICAST_MAC_ADDRESS)};
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
    connectionConf.name = UA_STRING("XDP Connection");
    connectionConf.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
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
    UA_NetworkAddressUrlDataType networkAddressUrlDataCopy = *((UA_NetworkAddressUrlDataType *)connectionConfig.address.data);
    ck_assert(UA_NetworkAddressUrlDataType_calcSizeBinary(&networkAddressUrlDataCopy) == UA_NetworkAddressUrlDataType_calcSizeBinary(&networkAddressUrlData));
    for(size_t i = 0; i < connectionConfig.connectionPropertiesSize; i++){
        ck_assert(UA_String_equal(&connectionConfig.connectionProperties[i].key.name, &connectionConf.connectionProperties[i].key.name) == UA_TRUE);
        ck_assert(UA_Variant_calcSizeBinary(&connectionConfig.connectionProperties[i].value) == UA_Variant_calcSizeBinary(&connectionConf.connectionProperties[i].value));
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


