/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 * Copyright (c) 2020 basysKom GmbH
 */

#ifndef UA_PLUGIN_MQTT_H_
#define UA_PLUGIN_MQTT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <open62541/plugin/pubsub_mqtt.h>
#include <open62541/network_tcp.h>

#if defined(UA_ENABLE_MQTT_TLS_OPENSSL)
#include <openssl/ssl.h>
#elif defined(UA_ENABLE_MQTT_TLS_MBEDTLS)
#include <mqtt_pal.h>
#endif

/* mqtt network layer specific internal data */
typedef struct {
    UA_NetworkAddressUrlDataType address;
    UA_UInt32 mqttRecvBufferSize;
    UA_UInt32 mqttSendBufferSize;
    uint8_t *mqttSendBuffer;
    uint8_t *mqttRecvBuffer;
    UA_String *mqttClientId;
    UA_Connection *connection;
#if defined(UA_ENABLE_MQTT_TLS_OPENSSL)
    BIO *sockfd;
#elif defined(UA_ENABLE_MQTT_TLS_MBEDTLS)
    mqtt_pal_socket_handle sockfd;
#else
    int sockfd;
#endif
    void * mqttClient;
    void (*callback)(UA_ByteString *encodedBuffer, UA_ByteString *topic);
    UA_String mqttUsername;
    UA_String mqttPassword;
    UA_String mqttCaFilePath;
    UA_String mqttCaPath;
    UA_String mqttClientCertPath;
    UA_String mqttClientKeyPath;
    UA_Boolean mqttUseTLS;
} UA_PubSubChannelDataMQTT;
/* TODO:
 * will topic,
 * will message,
 * keep alive
 * ssl: flag
 */

void freeTLS(UA_PubSubChannelDataMQTT *data);

UA_StatusCode
connectMqtt(UA_PubSubChannelDataMQTT*);

UA_StatusCode
disconnectMqtt(UA_PubSubChannelDataMQTT*);

UA_StatusCode
unSubscribeMqtt(UA_PubSubChannelDataMQTT*, UA_String topic);

UA_StatusCode
publishMqtt(UA_PubSubChannelDataMQTT*, UA_String topic, const UA_ByteString *buf, UA_Byte qos);

UA_StatusCode
subscribeMqtt(UA_PubSubChannelDataMQTT*, UA_String topic, UA_Byte qos);

UA_StatusCode
yieldMqtt(UA_PubSubChannelDataMQTT*, UA_UInt16 timeout);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_PLUGIN_MQTT_H_ */
