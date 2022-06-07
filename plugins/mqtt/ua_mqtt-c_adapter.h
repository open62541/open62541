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
