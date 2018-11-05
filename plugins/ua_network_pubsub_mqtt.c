/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 */

/**
 * This uses the mqtt/ua_mqtt_adapter.h to call mqtt functions.
 * Currently only ua_mqtt_adapter.c implements this 
 * interface and maps them to the specific "MQTT-C" library functions. 
 * Another mqtt lib could be used.
 * "ua_mqtt_pal.c" forwards the network calls (send/recv) to UA_Connection (TCP).
 */


/* Enable POSIX features */
#if !defined(_XOPEN_SOURCE) && !defined(_WRS_KERNEL)
# define _XOPEN_SOURCE 600
#endif
#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif

/* On older systems we need to define _BSD_SOURCE.
 * _DEFAULT_SOURCE is an alias for that. */
#ifndef _BSD_SOURCE
# define _BSD_SOURCE
#endif

 /* Disable some security warnings on MSVC */
#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif

 /* Assume that Windows versions are newer than Windows XP */
#if defined(__MINGW32__) && (!defined(WINVER) || WINVER < 0x501)
# undef WINVER
# undef _WIN32_WINDOWS
# undef _WIN32_WINNT
# define WINVER 0x0501
# define _WIN32_WINDOWS 0x0501
# define _WIN32_WINNT 0x0501
#endif

#ifdef _WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
# include <Iphlpapi.h>
# define CLOSESOCKET(S) closesocket((SOCKET)S)
# define ssize_t int
# define UA_fd_set(fd, fds) FD_SET((unsigned int)fd, fds)
# define UA_fd_isset(fd, fds) FD_ISSET((unsigned int)fd, fds)
#else /* _WIN32 */
#  define CLOSESOCKET(S) close(S)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
# endif /* Not Windows */

#include <stdio.h>
#include "ua_plugin_network.h"
#include "ua_network_pubsub_mqtt.h"
#include "mqtt/ua_mqtt_adapter.h"
#include "ua_log_stdout.h"
#include <ua_network_tcp.h>

static UA_StatusCode
UA_uaQos_toMqttQos(UA_BrokerTransportQualityOfService uaQos, UA_Byte *qos){
    switch (uaQos){
        case UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT:
            *qos = 0;
            break;
        case UA_BROKERTRANSPORTQUALITYOFSERVICE_ATLEASTONCE:
            *qos = 1;
            break;
        case UA_BROKERTRANSPORTQUALITYOFSERVICE_ATMOSTONCE:
            *qos = 2;
            break;
        default:
            break;
    }
    return UA_STATUSCODE_GOOD;
}

/**
 * Open communication socket based on the connectionConfig. Protocol specific parameters are
 * provided within the connectionConfig as KeyValuePair.
 * 
 *
 * @return ref to created channel, NULL on error
 */
static UA_PubSubChannel *
UA_PubSubChannelMQTT_open(const UA_PubSubConnectionConfig *connectionConfig) {
    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif /* Not Windows */

    UA_NetworkAddressUrlDataType address;
    if(UA_Variant_hasScalarType(&connectionConfig->address, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])){
        address = *(UA_NetworkAddressUrlDataType *)connectionConfig->address.data;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Invalid Address.");
        return NULL;
    }
    
    //allocate and init memory for the Mqtt specific internal data
    UA_PubSubChannelDataMQTT * channelDataMQTT =
            (UA_PubSubChannelDataMQTT *) UA_calloc(1, (sizeof(UA_PubSubChannelDataMQTT)));
    if(!channelDataMQTT){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        return NULL;
    }
    
    //set default values
    UA_String mqttClientId = UA_STRING("open62541_pub");
    memcpy(channelDataMQTT, &(UA_PubSubChannelDataMQTT){address, 2000,2000, NULL, NULL,&mqttClientId, NULL, NULL, NULL}, sizeof(UA_PubSubChannelDataMQTT));
    //iterate over the given KeyValuePair paramters
    UA_String sendBuffer = UA_STRING("sendBufferSize"), recvBuffer = UA_STRING("recvBufferSize"), clientId = UA_STRING("mqttClientId");
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
        } else {
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation. Unknown connection parameter.");
        }
    }
    
    /* Create a new channel */
    UA_PubSubChannel *newChannel = (UA_PubSubChannel *) UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        UA_free(channelDataMQTT);
        return NULL;
    }
    
    /* TODO: is the memory better allocated here or in mqtt specific impl? */
    /* Allocate memory for mqtt receive buffer */
    if(channelDataMQTT->mqttRecvBufferSize > 0){
        channelDataMQTT->mqttRecvBuffer = (uint8_t*)UA_calloc(channelDataMQTT->mqttRecvBufferSize, sizeof(uint8_t));
        if(channelDataMQTT->mqttRecvBuffer == NULL){
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
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK, "PubSub Connection failed");
        UA_free(channelDataMQTT->mqttSendBuffer);
        UA_free(channelDataMQTT->mqttRecvBuffer);
        UA_free(channelDataMQTT);
        UA_free(newChannel);
        return NULL;
    }
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Mqtt (and Tcp) Connection established.");
    
    newChannel->state = UA_PUBSUB_CHANNEL_RDY;
    return newChannel;
}

/**
 * Subscribe to a given address.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_regist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettigns, void (*callback)(UA_ByteString *encodedBuffer, UA_ByteString *topic)) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_RDY)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    
    UA_PubSubChannelDataMQTT *networkLayerData = (UA_PubSubChannelDataMQTT *) channel->handle;
    networkLayerData->callback = callback;
    
     if(transportSettigns != NULL && transportSettigns->encoding == UA_EXTENSIONOBJECT_DECODED 
            && transportSettigns->content.decoded.type->typeIndex == UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE){
        UA_BrokerWriterGroupTransportDataType *brokerTransportSettings = (UA_BrokerWriterGroupTransportDataType*)transportSettigns->content.decoded.data;

        UA_Byte qos = 0;
        UA_uaQos_toMqttQos(brokerTransportSettings->requestedDeliveryGuarantee, &qos);
        
        UA_PubSubChannelDataMQTT * connectionConfig = (UA_PubSubChannelDataMQTT *) channel->handle;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection register");
        ret = subscribeMqtt(connectionConfig, brokerTransportSettings->queueName, qos);

        if(!ret){
            channel->state = UA_PUBSUB_CHANNEL_PUB_SUB;
        }
    
    }else{
         ret = UA_STATUSCODE_BADARGUMENTSMISSING;
    }
    return ret;
}

/**
 * Remove current subscription.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_unregist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB_SUB || channel->state == UA_PUBSUB_CHANNEL_SUB)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection unregist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    UA_PubSubChannelDataMQTT * connectionConfig = (UA_PubSubChannelDataMQTT *) channel->handle;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection unregister");
    
    UA_StatusCode ret = unSubscribeMqtt(connectionConfig, UA_STRING("Topic"));

    if(!ret){
        channel->state = UA_PUBSUB_CHANNEL_PUB;
    }
    return ret;
}

/**
 * Send messages to the connection defined address
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_send(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings, const UA_ByteString *buf) {
    UA_PubSubChannelDataMQTT *channelConfigMQTT = (UA_PubSubChannelDataMQTT *) channel->handle;
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_PUB_SUB)){
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection sending failed. Invalid state.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_Byte qos = 0;
    
    if(transportSettings != NULL && transportSettings->encoding == UA_EXTENSIONOBJECT_DECODED 
            && transportSettings->content.decoded.type->typeIndex == UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE){
        UA_BrokerWriterGroupTransportDataType *brokerTransportSettings = (UA_BrokerWriterGroupTransportDataType*)transportSettings->content.decoded.data;
        UA_uaQos_toMqttQos(brokerTransportSettings->requestedDeliveryGuarantee, &qos);
        
        UA_String topic;
        topic = brokerTransportSettings->queueName;
        ret = publishMqtt(channelConfigMQTT, topic, buf, qos);

        if(ret){
            channel->state = UA_PUBSUB_CHANNEL_ERROR;
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Publish failed");
        }else{
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Publish");
        }
    }else{
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Transport settings not found.");
    }
    return ret;
}

/**
 * Receive messages. The regist function should be called before.
 *
 * @param timeout in usec | on windows platforms are only multiples of 1000usec possible
 * @return
 */
static UA_StatusCode
UA_PubSubChannelMQTT_receive(UA_PubSubChannel *channel, UA_ByteString *message, UA_ExtensionObject *transportSettigns, UA_UInt32 timeout){
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelMQTT_close(UA_PubSubChannel *channel) {
    //cleanup the internal NetworkLayer data
    UA_PubSubChannelDataMQTT *networkLayerData = (UA_PubSubChannelDataMQTT *) channel->handle;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Disconnect from Mqtt broker");
    UA_StatusCode ret = 0;
    ret = disconnectMqtt(networkLayerData);
    if(ret){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Disconnect from Mqtt broker failed");
    }
    UA_free(networkLayerData);
    UA_free(channel);
    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode 
UA_PubSubChannelMQTT_yield(UA_PubSubChannel *channel){
    UA_StatusCode ret = UA_STATUSCODE_BADCOMMUNICATIONERROR;
    if(channel->state != UA_PUBSUB_CHANNEL_ERROR){
        UA_PubSubChannelDataMQTT *networkLayerData = (UA_PubSubChannelDataMQTT *) channel->handle;
        
        ret = yieldMqtt(networkLayerData);

        if(ret != UA_STATUSCODE_GOOD){
            channel->state = UA_PUBSUB_CHANNEL_ERROR;
        }
    }
    return ret;
}

/**
 * Generate a new channel. based on the given configuration.
 *
 * @param connectionConfig connection configuration
 * @return  ref to created channel, NULL on error
 */
static UA_PubSubChannel *
TransportLayerMQTT_addChannel(UA_PubSubConnectionConfig *connectionConfig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSub channel requested");
    UA_PubSubChannel * pubSubChannel = UA_PubSubChannelMQTT_open(connectionConfig);
    if(pubSubChannel){
        pubSubChannel->regist = UA_PubSubChannelMQTT_regist;
        pubSubChannel->unregist = UA_PubSubChannelMQTT_unregist;
        pubSubChannel->send = UA_PubSubChannelMQTT_send;
        pubSubChannel->receive = UA_PubSubChannelMQTT_receive;
        pubSubChannel->close = UA_PubSubChannelMQTT_close;
        
        /*Additional yield func for mqtt*/
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
