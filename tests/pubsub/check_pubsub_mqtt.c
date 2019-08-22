/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019 Kalycito Infotech Private Limited */

#include "open62541/server.h"
#include "open62541/types_generated_encoding_binary.h"
#include "open62541/server_config_default.h"
#include "ua_network_pubsub_mqtt.h"
#include "ua_server_internal.h"
#include "check.h"

#define BROKER_ADDRESS_URL      "opc.mqtt://test.mosquitto.org:1883/"
#define TRANSPORT_PROFILE_URI   "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt"
#define CONNECTION_NAME         "Mqtt Connection"
#define FIRST_SUBSCRIBER_TOPIC  "customTopic"
#define SECOND_SUBSCRIBER_TOPIC "TopicCustom"

UA_Server *server = NULL;
UA_ServerConfig *config = NULL;
UA_PubSubConnection *connection;
UA_NodeId connectionIdent;

static void setup(void) {
    UA_PubSubConnectionConfig connectionConfig;

    /* Set up the server config */
    server = UA_Server_new();
    config = UA_Server_getConfig(server);
    UA_Server_run_startup(server);
    config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(1 * sizeof(UA_PubSubTransportLayer));
    if(!config->pubsubTransportLayers) {
       UA_ServerConfig_clean(config);
    }
    config->pubsubTransportLayers[0] = UA_PubSubTransportLayerMQTT();
    config->pubsubTransportLayersSize++;

    /* Create a connection */
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.name = UA_STRING(CONNECTION_NAME);
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING(BROKER_ADDRESS_URL)};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI);
    UA_Server_addPubSubConnection(server, &connectionConfig, &connectionIdent);
    connection = UA_PubSubConnection_findConnectionbyId(server, connectionIdent);

    if(!connection) {
        UA_Server_delete(server);
    }
}

static void teardown(void) {
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

START_TEST (RegisterUnregisterToATopic) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* Add brokerTransportSettings */
    UA_BrokerWriterGroupTransportDataType brokerTransportSettings;
    memset(&brokerTransportSettings, 0, sizeof(UA_BrokerWriterGroupTransportDataType));

    /* Assign the Topic at which MQTT subscription should happen */
    brokerTransportSettings.queueName = UA_STRING(FIRST_SUBSCRIBER_TOPIC);
    brokerTransportSettings.resourceUri = UA_STRING_NULL;
    brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;

    /* Choose the QOS Level for MQTT */
    brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    /* Encapsulate config in transportSettings */
    UA_ExtensionObject transportSettings;
    memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
    transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
    transportSettings.content.decoded.data = &brokerTransportSettings;

    /* Register transport settings for Subscriber */
    retVal = connection->channel->regist(connection->channel, &transportSettings, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Unregister subscriptions to the subscribed topic */
    retVal |= connection->channel->unregist(connection->channel, &transportSettings);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

START_TEST (RegisterUnregisterRegisterToMultipleTopic) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* Add brokerTransportSettings to customTopic as topic*/
    UA_BrokerWriterGroupTransportDataType brokerTransportSettings1;
    memset(&brokerTransportSettings1, 0, sizeof(UA_BrokerWriterGroupTransportDataType));

    /* Assign the Topic at which MQTT subscription should happen */
    brokerTransportSettings1.queueName = UA_STRING(FIRST_SUBSCRIBER_TOPIC);
    brokerTransportSettings1.resourceUri = UA_STRING_NULL;
    brokerTransportSettings1.authenticationProfileUri = UA_STRING_NULL;

    /* Choose the QOS Level for MQTT */
    brokerTransportSettings1.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    /* Encapsulate config in transportSettings */
    UA_ExtensionObject transportSettings1;
    memset(&transportSettings1, 0, sizeof(UA_ExtensionObject));
    transportSettings1.encoding = UA_EXTENSIONOBJECT_DECODED;
    transportSettings1.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
    transportSettings1.content.decoded.data = &brokerTransportSettings1;

    /* Add brokerTransportSettings to TopicCustom as topic*/
    UA_BrokerWriterGroupTransportDataType brokerTransportSettings2;
    memset(&brokerTransportSettings2, 0, sizeof(UA_BrokerWriterGroupTransportDataType));

    /* Assign the Topic at which MQTT subscription should happen */
    brokerTransportSettings2.queueName = UA_STRING(SECOND_SUBSCRIBER_TOPIC);
    brokerTransportSettings2.resourceUri = UA_STRING_NULL;
    brokerTransportSettings2.authenticationProfileUri = UA_STRING_NULL;

    /* Choose the QOS Level for MQTT */
    brokerTransportSettings2.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

    /* Encapsulate config in transportSettings */
    UA_ExtensionObject transportSettings2;
    memset(&transportSettings2, 0, sizeof(UA_ExtensionObject));
    transportSettings2.encoding = UA_EXTENSIONOBJECT_DECODED;
    transportSettings2.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
    transportSettings2.content.decoded.data = &brokerTransportSettings2;

    /* Register transport settings for customTopic*/
    retVal = connection->channel->regist(connection->channel, &transportSettings1, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Register transport settings for TopicCustom*/
    retVal |= connection->channel->regist(connection->channel, &transportSettings2, NULL);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Unregister transport settings for customTopic*/
    retVal |= connection->channel->unregist(connection->channel, &transportSettings1);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);

    /* Unregister transport settings for TopicCustom*/
    retVal |= connection->channel->unregist(connection->channel, &transportSettings2);
    ck_assert_int_eq(retVal, UA_STATUSCODE_GOOD);
    } END_TEST

int main(void) {
    TCase *tc_register_unregister_mqtt = tcase_create("Register and Unregister subscriptions");
    tcase_add_checked_fixture(tc_register_unregister_mqtt, setup, teardown);
    tcase_add_test(tc_register_unregister_mqtt, RegisterUnregisterToATopic);
    tcase_add_test(tc_register_unregister_mqtt, RegisterUnregisterRegisterToMultipleTopic);

    Suite *s = suite_create("PubSub Mqtt");
    suite_add_tcase(s, tc_register_unregister_mqtt);

    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
