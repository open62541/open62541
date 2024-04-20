/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>

#include <check.h>
#include <time.h>

#include "test_helpers.h"
#include "ua_pubsub.h"
#include "ua_server_internal.h"

/* Adjust your configuration globally for the ethernet tests here: */
#include "ethernet_config.h"

#define PUBLISHING_MULTICAST_MAC_ADDRESS1 "opc.eth://01-00-5E-7F-00-01"
#define PUBLISHING_MULTICAST_MAC_ADDRESS2 "opc.eth://01-00-5E-7F-00-01:8.4"
#define BUFFER_STRING                     "Hello! This is Ethernet ETF Testing"
#define TRANSPORT_PROFILE_URI             "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define CONNECTION_NAME                   "ETF Ethernet Test Connection"
#define UA_SUBSCRIBER_PORT                4801 /* Port for Subscriber*/
#define PUBLISHER_ID                      2234
#define CYCLE_TIME                        0.25
#define SECONDS                           1000 * 1000 * 1000
#define MILLI_SECONDS                     1000 * 1000
#define SECONDS_SLEEP                     5
#define NANO_SECONDS_SLEEP_PUB            (long) (CYCLE_TIME * MILLI_SECONDS * 0.6)
#define QBV_OFFSET                        25 * 1000
#define CLOCKID                           CLOCK_TAI
#define SOCKET_PRIORITY                   3

/* Global declaration for test cases */
UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
UA_NodeId connection_test;
UA_PubSubConnection *connection; /* setup() is to create an environment for test cases */

static void setup(void) {
    /*Add setup by creating new server with valid configuration */
    server = UA_Server_newForUnitTest();
    ck_assert(server != NULL);
    config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, UA_SUBSCRIBER_PORT, NULL);
    UA_Server_run_startup(server);
}

/* teardown() is to delete the environment set for test cases */
static void teardown(void) {
    /*Call server delete functions */
   UA_Server_run_shutdown(server);
   UA_Server_delete(server);
}

START_TEST(EthernetSendWithoutVLANTag) {
    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(CONNECTION_NAME);
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(PUBLISHING_MULTICAST_MAC_ADDRESS1)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;
    /* Connection options are given as Key/Value Pairs - Sockprio and Txtime */
    UA_KeyValuePair connectionOptions[2];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "sockpriority");
    UA_UInt32 sockPriority   = SOCKET_PRIORITY;
    UA_Variant_setScalar(&connectionOptions[0].value, &sockPriority, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "enablesotxtime");
    UA_Boolean enableTxTime  = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[1].value, &enableTxTime, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionConfig.connectionProperties.map = connectionOptions;
    connectionConfig.connectionProperties.mapSize = 2;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection_test);
    connection = UA_PubSubConnection_findConnectionbyId(server, connection_test);
    ck_assert(connection);

    /* Add a writer group to enable the connection */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    UA_NodeId localWriterGroup;
    UA_StatusCode retVal =
        UA_Server_addWriterGroup(server, connection_test,
                                 &writerGroupConfig, &localWriterGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    retVal = UA_Server_enableWriterGroup(server, localWriterGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* TODO: Encapsulate ETF config in transportSettings */
    /* UA_ExtensionObject transportSettings; */
    /* memset(&transportSettings, 0, sizeof(UA_ExtensionObject)); */
    /* transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED; */
    /* transportSettings.content.decoded.data = &ethernettransportSettings; */
    /* clock_gettime(CLOCKID, &nextnanosleeptime); */
    /* transmission_time = ((UA_UInt64)nextnanosleeptime.tv_sec * SECONDS + (UA_UInt64)nextnanosleeptime.tv_nsec) + roundOffCycleTime + QBV_OFFSET; */
    /* ethernettransportSettings.transmission_time = transmission_time; */

    /* Initialize a buffer to send data */
    UA_ByteString testBuffer = UA_STRING(BUFFER_STRING);
    UA_ByteString networkBuffer = UA_STRING_NULL;
    connection->cm->allocNetworkBuffer(connection->cm,
                                       connection->sendChannel,
                                       &networkBuffer,
                                       testBuffer.length);
    memcpy(networkBuffer.data, testBuffer.data, testBuffer.length);

    retVal = connection->cm->
        sendWithConnection(connection->cm, connection->sendChannel,
                           &UA_KEYVALUEMAP_NULL, &networkBuffer);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

START_TEST(EthernetSendWithVLANTag) {
    /* Add connection to the server */
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(CONNECTION_NAME);
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING(ETHERNET_INTERFACE), UA_STRING(PUBLISHING_MULTICAST_MAC_ADDRESS2)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;
    /* Connection options are given as Key/Value Pairs - Sockprio and Txtime */
    UA_KeyValuePair connectionOptions[2];
    connectionOptions[0].key = UA_QUALIFIEDNAME(0, "sockpriority");
    UA_UInt32 sockPriority   = SOCKET_PRIORITY;
    UA_Variant_setScalar(&connectionOptions[0].value, &sockPriority, &UA_TYPES[UA_TYPES_UINT32]);
    connectionOptions[1].key = UA_QUALIFIEDNAME(0, "enablesotxtime");
    UA_Boolean enableTxTime  = UA_TRUE;
    UA_Variant_setScalar(&connectionOptions[1].value, &enableTxTime, &UA_TYPES[UA_TYPES_BOOLEAN]);
    connectionConfig.connectionProperties.map = connectionOptions;
    connectionConfig.connectionProperties.mapSize = 2;
    UA_Server_addPubSubConnection(server, &connectionConfig, &connection_test);
    connection = UA_PubSubConnection_findConnectionbyId(server, connection_test);
    ck_assert(connection);

    /* Add a writer group to enable the connection */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(writerGroupConfig));
    writerGroupConfig.name = UA_STRING("WriterGroup 1");
    writerGroupConfig.publishingInterval = 10;
    UA_NodeId localWriterGroup;
    UA_StatusCode retVal =
        UA_Server_addWriterGroup(server, connection_test,
                                 &writerGroupConfig, &localWriterGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    retVal = UA_Server_setWriterGroupOperational(server, localWriterGroup);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* TODO: Encapsulate ETF config in transportSettings */
    /* UA_ExtensionObject transportSettings; */
    /* memset(&transportSettings, 0, sizeof(UA_ExtensionObject)); */
    /* transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED; */
    /* transportSettings.content.decoded.data = &ethernettransportSettings; */
    /* clock_gettime(CLOCKID, &nextnanosleeptime); */
    /* transmission_time = ((UA_UInt64)nextnanosleeptime.tv_sec * SECONDS + (UA_UInt64)nextnanosleeptime.tv_nsec) + roundOffCycleTime + QBV_OFFSET; */
    /* ethernettransportSettings.transmission_time = transmission_time; */

    /* Initialize a buffer to send data */
    UA_ByteString testBuffer = UA_STRING(BUFFER_STRING);
    UA_ByteString networkBuffer = UA_STRING_NULL;
    connection->cm->allocNetworkBuffer(connection->cm,
                                       connection->sendChannel,
                                       &networkBuffer,
                                       testBuffer.length);
    memcpy(networkBuffer.data, testBuffer.data, testBuffer.length);

    retVal = connection->cm->sendWithConnection(connection->cm, connection->sendChannel,
                                                &UA_KEYVALUEMAP_NULL, &networkBuffer);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
} END_TEST

int main(void) {
    if(SKIP_ETHERNET && strlen(SKIP_ETHERNET) > 0)
        return EXIT_SUCCESS;

    /*Test case to run both publisher*/
    TCase *tc_pubsub_ethernet_etf_publish = tcase_create("Publisher publishing Ethernet packets based on etf");
    tcase_add_checked_fixture(tc_pubsub_ethernet_etf_publish, setup, teardown);
    tcase_add_test(tc_pubsub_ethernet_etf_publish, EthernetSendWithoutVLANTag);
    tcase_add_test(tc_pubsub_ethernet_etf_publish, EthernetSendWithVLANTag);
    Suite *suite = suite_create("Ethernet ETF Publisher");
    suite_add_tcase(suite, tc_pubsub_ethernet_etf_publish);
    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner,CK_NORMAL);
    int number_failed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
