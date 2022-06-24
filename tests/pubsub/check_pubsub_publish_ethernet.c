/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <check.h>

#include "ua_pubsub.h"
#include "ua_server_internal.h"
#include "ua_pubsub_networkmessage.h"

#define PUBLISHING_MULTICAST_MAC_ADDRESS1  "opc.eth://01-00-5E-7F-00-01"
#define PUBLISHING_MULTICAST_MAC_ADDRESS2  "opc.eth://01-00-5E-7F-00-01:8.4"
#define TRANSPORT_PROFILE_URI              "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define UA_SUBSCRIBER_PORT                 4801    /* Port for Subscriber*/
#define PUBLISHER_ID                       2234    /* Publisher Id*/
#define ETHERNET_INTERFACE                 "enp2s0"
#define BUFFER_STRING                      "Hello! This is Ethernet Testing"
#define CONNECTION_NAME                    "Ethernet Test Connection"

/* Global declaration for test cases  */
UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
UA_PubSubConnection *connection;
UA_NodeId connection_test;

/* setup() is to create an environment for test cases */
static void setup(void) {
    /*Add setup by creating new server with valid configuration */
    server = UA_Server_new();
    config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, UA_SUBSCRIBER_PORT, NULL);
    UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
    UA_Server_run_startup(server);
}

/* teardown() is to delete the environment set for test cases */
static void teardown(void) {
    /*Call server delete functions */
   UA_Server_run_shutdown(server);
   UA_Server_delete(server);
}

START_TEST(EthernetSendWithoutVLANTag) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ByteString testBuffer;
    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(CONNECTION_NAME);
    /* Provide a multicast MAC without VLAN tag */
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(PUBLISHING_MULTICAST_MAC_ADDRESS1)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI);
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.uint16 = PUBLISHER_ID;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection_test);
    connection = UA_PubSubConnection_findConnectionbyId(server, connection_test);
    /* Remove the connection if invalid*/
    if(!connection) {
        return;
    }

    /* Initialize a buffer to send data */
    testBuffer = UA_STRING(BUFFER_STRING);
    /* Validate the Ethernet send API */
    retVal = connection->channel->send(connection->channel, NULL, &testBuffer);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    } END_TEST

START_TEST(EthernetSendWithVLANTag) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ByteString testBuffer ;
    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(CONNECTION_NAME);
    /* Provide a multicast MAC without VLAN tag */
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(PUBLISHING_MULTICAST_MAC_ADDRESS2)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI);
    connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.uint16 = PUBLISHER_ID;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection_test);
    connection = UA_PubSubConnection_findConnectionbyId(server, connection_test);
    /* Remove the connection if invalid*/
    if(!connection) {
        return;
    }

    /* Initialize a buffer to send data */
    testBuffer = UA_STRING(BUFFER_STRING);
    /* Validate the Ethernet send API */
    retVal = connection->channel->send(connection->channel, NULL, &testBuffer);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    } END_TEST


int main(void) {
    /*Test case to run both publisher*/
    TCase *tc_pubsub_ethernet_send = tcase_create("Publisher publishing Ethernet packets");
    tcase_add_checked_fixture(tc_pubsub_ethernet_send, setup, teardown);
    tcase_add_test(tc_pubsub_ethernet_send, EthernetSendWithoutVLANTag);
    tcase_add_test(tc_pubsub_ethernet_send, EthernetSendWithVLANTag);

    Suite *suite = suite_create("Ethernet Send");
    suite_add_tcase(suite, tc_pubsub_ethernet_send);

    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


