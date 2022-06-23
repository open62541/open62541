/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 * 
 *    Copyright 2018 (c) Fraunhofer IOSB (Author: Lukas Meling)
 *    Copyright (c) 2020 basysKom GmbH
 */

#ifndef UA_PUBSUB_MQTT_H_
#define UA_PUBSUB_MQTT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <open62541/plugin/pubsub.h>

UA_PubSubTransportLayer UA_EXPORT
UA_PubSubTransportLayerMQTT(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_PUBSUB_MQTT_H_ */
