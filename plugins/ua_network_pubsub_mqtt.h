/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 * 
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 *    Copyright (c) 2020 basysKom GmbH
 */

#ifndef UA_NETWORK_MQTT_H_
#define UA_NETWORK_MQTT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "open62541/plugin/pubsub.h"
#include "open62541/network_tcp.h"

#ifdef UA_ENABLE_MQTT_TLS
#include <openssl/ssl.h>
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
#ifdef UA_ENABLE_MQTT_TLS_OPENSSL
    SSL *ssl;
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


UA_PubSubTransportLayer
UA_PubSubTransportLayerMQTT(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NETWORK_MQTT_H_ */
