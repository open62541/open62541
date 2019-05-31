/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 *    Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#ifndef UA_NETWORK_MQTT_H_
#define UA_NETWORK_MQTT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "open62541/plugin/pubsub.h"
#include "open62541/network_tcp.h"

/* Topic to be subscribed by the MQTT subcriber */
char* mqttTopic;
/* mqtt network layer specific internal data */
typedef struct {
    UA_NetworkAddressUrlDataType address;
    UA_UInt32 mqttRecvBufferSize;
    UA_UInt32 mqttSendBufferSize;
    uint8_t *mqttSendBuffer;
    uint8_t *mqttRecvBuffer;
    UA_String *mqttClientId;
    UA_Connection *connection;
    void * mqttClient;
    void (*mqttSubscribeCallback)(UA_ByteString *encodedBuffer, UA_ByteString *topic);
} UA_PubSubChannelDataMQTT;
/* TODO:
 * will topic,
 * will message,
 * user name,
 * password,
 * keep alive
 * ssl: cert, flag
 */


UA_PubSubTransportLayer
UA_PubSubTransportLayerMQTT(void);


#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NETWORK_MQTT_H_ */
