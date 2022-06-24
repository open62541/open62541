/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 * Copyright (c) 2020 basysKom GmbH
 */

/**
 * This uses the mqtt/ua_mqtt_adapter.h to call mqtt functions.
 * Currently only ua_mqtt_adapter.c implements this
 * interface and maps them to the specific "MQTT-C" library functions.
 * Another mqtt lib could be used.
 * "mqtt_pal.c" forwards the network calls (send/recv) to UA_Connection (TCP).
 */

#include <open62541/server_pubsub.h>
#include <open62541/util.h>

#include "mqtt/ua_mqtt-c_adapter.h"
#include "open62541/plugin/log_stdout.h"

static UA_StatusCode
UA_uaQos_toMqttQos(UA_BrokerTransportQualityOfService uaQos, UA_Byte *qos){
    switch (uaQos){
    	case UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT:
        case UA_BROKERTRANSPORTQUALITYOFSERVICE_ATMOSTONCE:
            *qos = 0;
            break;
        case UA_BROKERTRANSPORTQUALITYOFSERVICE_ATLEASTONCE:
            *qos = 1;
            break;
        case UA_BROKERTRANSPORTQUALITYOFSERVICE_EXACTLYONCE:
            *qos = 2;
            break;
        default:
            break;
    }
    return UA_STATUSCODE_GOOD;
}

/**
 * Open mqtt connection based on the connectionConfig.
 *
 *
 * @return ref to created channel, NULL on error
 */
static UA_PubSubChannel *
UA_PubSubChannelMQTT_open(const UA_PubSubConnectionConfig *connectionConfig) {
    UA_NetworkAddressUrlDataType address;
    memset(&address, 0, sizeof(UA_NetworkAddressUrlDataType));
    if(UA_Variant_hasScalarType(&connectionConfig->address, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])){
        address = *(UA_NetworkAddressUrlDataType *)connectionConfig->address.data;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT Connection creation failed. Invalid Address.");
        return NULL;
    }

    /* allocate and init memory for the Mqtt specific internal data */
    UA_PubSubChannelDataMQTT * channelDataMQTT =
            (UA_PubSubChannelDataMQTT *) UA_calloc(1, (sizeof(UA_PubSubChannelDataMQTT)));
    if(!channelDataMQTT){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT Connection creation failed. Out of memory.");
        return NULL;
    }

    /* set default values */
    UA_String mqttClientId = UA_STRING("open62541_pub");
    memcpy(channelDataMQTT, &(UA_PubSubChannelDataMQTT){address, 2000, 2000, NULL, NULL, &mqttClientId, NULL,
                                                    #if defined(UA_ENABLE_MQTT_TLS_OPENSSL) // Initialize the "ssl" member
                                                        NULL,
                                                    #elif defined(UA_ENABLE_MQTT_TLS_MBEDTLS)
                                                        NULL,
                                                    #else
                                                        -1,
                                                    #endif
                                                        NULL, NULL,
                                                        UA_STRING_NULL, UA_STRING_NULL, UA_STRING_NULL, UA_STRING_NULL,
                                                        UA_STRING_NULL, UA_STRING_NULL, UA_FALSE},
           sizeof(UA_PubSubChannelDataMQTT));
    /* iterate over the given KeyValuePair paramters */
    UA_String sendBuffer = UA_STRING("sendBufferSize"), recvBuffer = UA_STRING("recvBufferSize"), clientId = UA_STRING("mqttClientId"),
            username = UA_STRING("mqttUsername"), password = UA_STRING("mqttPassword"), caFilePath = UA_STRING("mqttCaFilePath"),
            caPath = UA_STRING("mqttCaPath"), useTLS = UA_STRING("mqttUseTLS"), clientCertPath = UA_STRING("mqttClientCertPath"),
            clientKeyPath = UA_STRING("mqttClientKeyPath");
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++){
        if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &sendBuffer)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_UINT32])){
                channelDataMQTT->mqttSendBufferSize = *(UA_UInt32 *) connectionConfig->connectionProperties[i].value.data;
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &recvBuffer)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_UINT32])){
                channelDataMQTT->mqttRecvBufferSize = *(UA_UInt32 *) connectionConfig->connectionProperties[i].value.data;
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &clientId)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_STRING])){
                channelDataMQTT->mqttClientId = (UA_String *) connectionConfig->connectionProperties[i].value.data;
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &username)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_STRING])){
                UA_String_copy((UA_String *) connectionConfig->connectionProperties[i].value.data, &channelDataMQTT->mqttUsername);
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &password)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_STRING])){
                UA_String_copy((UA_String *) connectionConfig->connectionProperties[i].value.data, &channelDataMQTT->mqttPassword);
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &caFilePath)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_STRING])){
                UA_String_copy((UA_String *) connectionConfig->connectionProperties[i].value.data, &channelDataMQTT->mqttCaFilePath);
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &caPath)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_STRING])){
                UA_String_copy((UA_String *) connectionConfig->connectionProperties[i].value.data, &channelDataMQTT->mqttCaPath);
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &useTLS)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_BOOLEAN])){
                channelDataMQTT->mqttUseTLS = *(UA_Boolean *) connectionConfig->connectionProperties[i].value.data;
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &clientCertPath)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_STRING])){
                UA_String_copy((UA_String *) connectionConfig->connectionProperties[i].value.data, &channelDataMQTT->mqttClientCertPath);
            }
        } else if(UA_String_equal(&connectionConfig->connectionProperties[i].key.name, &clientKeyPath)){
            if(UA_Variant_hasScalarType(&connectionConfig->connectionProperties[i].value, &UA_TYPES[UA_TYPES_STRING])){
                UA_String_copy((UA_String *) connectionConfig->connectionProperties[i].value.data, &channelDataMQTT->mqttClientKeyPath);
            }
        }  else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT Connection creation. Unknown connection parameter.");
        }
    }

    /* Create a new channel */
    UA_PubSubChannel *newChannel = (UA_PubSubChannel *) UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT Connection creation failed. Out of memory.");
        UA_String_clear(&channelDataMQTT->mqttUsername);
        UA_String_clear(&channelDataMQTT->mqttPassword);
        UA_String_clear(&channelDataMQTT->mqttCaFilePath);
        UA_String_clear(&channelDataMQTT->mqttCaPath);
        UA_String_clear(&channelDataMQTT->mqttClientCertPath);
        UA_String_clear(&channelDataMQTT->mqttClientKeyPath);
        UA_free(channelDataMQTT);
        return NULL;
    }

    /* Allocate memory for mqtt receive buffer */
    if(channelDataMQTT->mqttRecvBufferSize > 0){
        channelDataMQTT->mqttRecvBuffer = (uint8_t*)UA_calloc(channelDataMQTT->mqttRecvBufferSize, sizeof(uint8_t));
        if(channelDataMQTT->mqttRecvBuffer == NULL){
            UA_String_clear(&channelDataMQTT->mqttUsername);
            UA_String_clear(&channelDataMQTT->mqttPassword);
            UA_String_clear(&channelDataMQTT->mqttCaFilePath);
            UA_String_clear(&channelDataMQTT->mqttCaPath);
            UA_String_clear(&channelDataMQTT->mqttClientCertPath);
            UA_String_clear(&channelDataMQTT->mqttClientKeyPath);
            UA_free(channelDataMQTT);
            UA_free(newChannel);
            return NULL;
        }
    }

    /* Allocate memory for mqtt send buffer */
    if(channelDataMQTT->mqttSendBufferSize > 0){
        channelDataMQTT->mqttSendBuffer = (uint8_t*)UA_calloc(channelDataMQTT->mqttSendBufferSize, sizeof(uint8_t));
        if(channelDataMQTT->mqttSendBuffer == NULL){
            if(channelDataMQTT->mqttRecvBufferSize > 0){
                UA_free(channelDataMQTT->mqttRecvBuffer);
            }
            UA_String_clear(&channelDataMQTT->mqttUsername);
            UA_String_clear(&channelDataMQTT->mqttPassword);
            UA_String_clear(&channelDataMQTT->mqttCaFilePath);
            UA_String_clear(&channelDataMQTT->mqttCaPath);
            UA_String_clear(&channelDataMQTT->mqttClientCertPath);
            UA_String_clear(&channelDataMQTT->mqttClientKeyPath);
            UA_free(channelDataMQTT);
            UA_free(newChannel);
            return NULL;
        }
    }

    /*link channel and internal channel data*/
    newChannel->handle = channelDataMQTT;

    /* MQTT Client connect call. */
    UA_StatusCode ret = connectMqtt(channelDataMQTT);

    if(ret != UA_STATUSCODE_GOOD){
        /* try to disconnect tcp */
        disconnectMqtt(channelDataMQTT);
        UA_free(channelDataMQTT->mqttSendBuffer);
        UA_free(channelDataMQTT->mqttRecvBuffer);
        UA_String_clear(&channelDataMQTT->mqttUsername);
        UA_String_clear(&channelDataMQTT->mqttPassword);
        UA_String_clear(&channelDataMQTT->mqttCaFilePath);
        UA_String_clear(&channelDataMQTT->mqttCaPath);
        UA_String_clear(&channelDataMQTT->mqttClientCertPath);
        UA_String_clear(&channelDataMQTT->mqttClientKeyPath);
        UA_free(channelDataMQTT);
        UA_free(newChannel);
        return NULL;
    }
    newChannel->state = UA_PUBSUB_CHANNEL_RDY;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT Connection established.");
    return newChannel;
}

/**
 * Subscribe to topic specified in brokerTransportSettings->queueName.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_regist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                            void (*callback)(UA_ByteString *encodedBuffer, UA_ByteString *topic)) {
    if(channel->state != UA_PUBSUB_CHANNEL_RDY){
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT regist failed. Channel closed.");
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    UA_PubSubChannelDataMQTT *channelDataMQTT = (UA_PubSubChannelDataMQTT *) channel->handle;
    channelDataMQTT->callback = callback;

    if(transportSettings == NULL ||
       transportSettings->encoding != UA_EXTENSIONOBJECT_DECODED ||
       transportSettings->content.decoded.type != &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE]) {
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }

    UA_BrokerDataSetReaderTransportDataType *brokerTransportSettings =
        (UA_BrokerDataSetReaderTransportDataType*)transportSettings->content.decoded.data;

    UA_Byte qos = 0;
    UA_uaQos_toMqttQos(brokerTransportSettings->requestedDeliveryGuarantee, &qos);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: register");
    return subscribeMqtt(channelDataMQTT, brokerTransportSettings->queueName, qos);
}

/**
 * Remove subscription specified in brokerTransportSettings->queueName.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_unregist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings) {
    if(channel->state != UA_PUBSUB_CHANNEL_RDY){
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: unregister failed. Channel closed.");
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    UA_PubSubChannelDataMQTT * channelDataMQTT = (UA_PubSubChannelDataMQTT *) channel->handle;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: unregister");

    if(transportSettings == NULL ||
       transportSettings->encoding != UA_EXTENSIONOBJECT_DECODED ||
       transportSettings->content.decoded.type != &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE]) {
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }

    UA_BrokerWriterGroupTransportDataType *brokerTransportSettings =
        (UA_BrokerWriterGroupTransportDataType*)transportSettings->content.decoded.data;

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: unregister");
    return unSubscribeMqtt(channelDataMQTT, brokerTransportSettings->queueName);
}

/**
 * Send a message.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_send(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings, const UA_ByteString *buf) {
    if(channel->state != UA_PUBSUB_CHANNEL_RDY){
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: sending failed. Invalid state.");
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }

    if(transportSettings == NULL ||
       transportSettings->encoding != UA_EXTENSIONOBJECT_DECODED ||
       transportSettings->content.decoded.type != &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE]) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Transport settings not found.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_Byte qos = 0;
    UA_BrokerWriterGroupTransportDataType *brokerTransportSettings =
        (UA_BrokerWriterGroupTransportDataType*)transportSettings->content.decoded.data;
    UA_uaQos_toMqttQos(brokerTransportSettings->requestedDeliveryGuarantee, &qos);

    UA_PubSubChannelDataMQTT *channelDataMQTT = (UA_PubSubChannelDataMQTT *) channel->handle;
    UA_StatusCode ret = publishMqtt(channelDataMQTT, brokerTransportSettings->queueName, buf, qos);
    if(ret != UA_STATUSCODE_GOOD) {
        channel->state = UA_PUBSUB_CHANNEL_ERROR;
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Publish failed");
        return ret;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Publish");
    return ret;
}

/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_close(UA_PubSubChannel *channel) {
    /* already closed */
    if(channel->state == UA_PUBSUB_CHANNEL_CLOSED)
        return UA_STATUSCODE_GOOD;
    UA_PubSubChannelDataMQTT *channelDataMQTT = (UA_PubSubChannelDataMQTT *) channel->handle;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Closing PubSubChannel.");
    disconnectMqtt(channelDataMQTT);
    UA_String_clear(&channelDataMQTT->mqttUsername);
    UA_String_clear(&channelDataMQTT->mqttPassword);
    UA_String_clear(&channelDataMQTT->mqttCaFilePath);
    UA_String_clear(&channelDataMQTT->mqttCaPath);
    UA_String_clear(&channelDataMQTT->mqttClientCertPath);
    UA_String_clear(&channelDataMQTT->mqttClientKeyPath);
    UA_free(channelDataMQTT);
    UA_free(channel);
    return UA_STATUSCODE_GOOD;
}

/**
 * Calls the send and receive functions of the mqtt network stack.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_yield(UA_PubSubChannel *channel, UA_UInt16 timeout){
    UA_StatusCode ret = UA_STATUSCODE_BADINVALIDARGUMENT;
    if(channel == NULL){
        return ret;
    }

    if(channel->state == UA_PUBSUB_CHANNEL_ERROR){
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_PubSubChannelDataMQTT *channelDataMQTT = (UA_PubSubChannelDataMQTT *) channel->handle;
    ret = yieldMqtt(channelDataMQTT, timeout);
    if(ret != UA_STATUSCODE_GOOD){
        channel->state = UA_PUBSUB_CHANNEL_ERROR;
        return ret;
    }

    return ret;
}

/**
 * Generate a new MQTT channel. Based on the given configuration. Uses yield and no recv call.
 *
 * @param connectionConfig connection configuration
 * @return  ref to created channel, NULL on error
 */
static UA_PubSubChannel *
TransportLayerMQTT_addChannel(UA_PubSubConnectionConfig *connectionConfig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSub MQTT: channel requested.");
    UA_PubSubChannel * pubSubChannel = UA_PubSubChannelMQTT_open(connectionConfig);
    if(pubSubChannel){
        pubSubChannel->regist = UA_PubSubChannelMQTT_regist;
        pubSubChannel->unregist = UA_PubSubChannelMQTT_unregist;
        pubSubChannel->send = UA_PubSubChannelMQTT_send;
        pubSubChannel->close = UA_PubSubChannelMQTT_close;
        pubSubChannel->yield = UA_PubSubChannelMQTT_yield;

        pubSubChannel->connectionConfig = connectionConfig;
    }
    return pubSubChannel;
}

//MQTT channel factory
UA_PubSubTransportLayer
UA_PubSubTransportLayerMQTT(){
    UA_PubSubTransportLayer pubSubTransportLayer;
    pubSubTransportLayer.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt");
    pubSubTransportLayer.createPubSubChannel = &TransportLayerMQTT_addChannel;
    return pubSubTransportLayer;
}

#undef _POSIX_C_SOURCE
