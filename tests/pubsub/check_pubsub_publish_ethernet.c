/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ethernet_config.h"
#include "test_helpers.h"
#include "ua_pubsub_internal.h"
#include "ua_server_internal.h"
#include "ua_pubsub_networkmessage.h"

#define PUBLISHING_MULTICAST_MAC_ADDRESS1  "opc.eth://01-00-5E-7F-00-01"
#define PUBLISHING_MULTICAST_MAC_ADDRESS2  "opc.eth://01-00-5E-7F-00-01:8.4"
#define TRANSPORT_PROFILE_URI              "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define UA_SUBSCRIBER_PORT                 4801
#define PUBLISHER_ID                       2234
#define BUFFER_STRING                      "Hello! This is Ethernet Testing"
#define CONNECTION_NAME                    "Ethernet Test Connection"

static UA_Server *server;

static void
setup(void) {
    server = UA_Server_newForUnitTest();
    ck_assert_ptr_ne(server, NULL);
    UA_StatusCode retVal =
        UA_ServerConfig_setMinimal(UA_Server_getConfig(server),
                                   UA_SUBSCRIBER_PORT, NULL);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_run_startup(server);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
}

static void
teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    server = NULL;
}

static void
sendEthernetPacket(UA_String address) {
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));
    connectionConfig.name = UA_STRING(CONNECTION_NAME);
    UA_NetworkAddressUrlDataType networkAddressUrl = {
        UA_STRING(ETHERNET_INTERFACE), address
    };
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI);
    connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT16;
    connectionConfig.publisherId.id.uint16 = PUBLISHER_ID;

    UA_NodeId connectionId = UA_NODEID_NULL;
    UA_StatusCode retVal =
        UA_Server_addPubSubConnection(server, &connectionConfig, &connectionId);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    UA_PubSubConnection *connection =
        UA_PubSubConnection_find(getPSM(server), connectionId);
    ck_assert_ptr_ne(connection, NULL);
    ck_assert_ptr_ne(connection->cm, NULL);

    /* A writer group and an enabled connection attach the send channel. */
    UA_WriterGroupConfig wgConfig;
    memset(&wgConfig, 0, sizeof(wgConfig));
    wgConfig.name = UA_STRING("Ethernet Test Writer Group");
    wgConfig.publishingInterval = 1000.0;
    UA_NodeId wgId = UA_NODEID_NULL;
    retVal = UA_Server_addWriterGroup(server, connectionId, &wgConfig, &wgId);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_enableWriterGroup(server, wgId);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    retVal = UA_Server_enablePubSubConnection(server, connectionId);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    for(size_t i = 0; i < 100 && connection->sendChannel == 0; ++i)
        UA_Server_run_iterate(server, false);
    ck_assert(connection->sendChannel != 0);

    /* The connection manager consumes this buffer, including on send failure. */
    UA_ByteString testBuffer = UA_BYTESTRING_NULL;
    size_t payloadLength = strlen(BUFFER_STRING);
    retVal = connection->cm->allocNetworkBuffer(
        connection->cm, connection->sendChannel, &testBuffer, payloadLength);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(testBuffer.data, NULL);
    ck_assert(testBuffer.length >= payloadLength);
    memcpy(testBuffer.data, BUFFER_STRING, payloadLength);
    testBuffer.length = payloadLength;
    retVal = connection->cm->sendWithConnection(
        connection->cm, connection->sendChannel,
        &UA_KEYVALUEMAP_NULL, &testBuffer);
    ck_assert_uint_eq(retVal, UA_STATUSCODE_GOOD);

    UA_NodeId_clear(&wgId);
    UA_NodeId_clear(&connectionId);
    UA_Server_run_iterate(server, false);
}

START_TEST(EthernetSendWithoutVLANTag) {
    sendEthernetPacket(UA_STRING(PUBLISHING_MULTICAST_MAC_ADDRESS1));
} END_TEST

START_TEST(EthernetSendWithVLANTag) {
    sendEthernetPacket(UA_STRING(PUBLISHING_MULTICAST_MAC_ADDRESS2));
} END_TEST

int
main(void) {
    const char *interface = ETHERNET_INTERFACE;
    const char *skip = SKIP_ETHERNET;
    if((skip && skip[0]) || !interface || !interface[0]) {
        fprintf(stderr, "SKIP: configure ETHERNET_INTERFACE and unset "
                        "SKIP_ETHERNET to run Ethernet send tests\n");
        return 77;
    }

    TCase *tc = tcase_create("Publisher publishing Ethernet packets");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, EthernetSendWithoutVLANTag);
    tcase_add_test(tc, EthernetSendWithVLANTag);

    Suite *suite = suite_create("Ethernet Send");
    suite_add_tcase(suite, tc);
    SRunner *suiteRunner = srunner_create(suite);
    srunner_set_fork_status(suiteRunner, CK_NOFORK);
    srunner_run_all(suiteRunner, CK_NORMAL);
    int numberFailed = srunner_ntests_failed(suiteRunner);
    srunner_free(suiteRunner);
    return numberFailed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
