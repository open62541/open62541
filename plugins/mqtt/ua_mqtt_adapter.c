/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 */

#ifndef PLUGIN_MQTT_mqttc_H_
#define PLUGIN_MQTT_mqttc_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "ua_mqtt_adapter.h"
#include "../../deps/mqtt-c/mqtt.h" //#include "mqtt.h"
#include <ua_network_tcp.h>
#include "ua_log_stdout.h"
#include <fcntl.h>
#include "ua_util.h"
    

UA_StatusCode disconnectMqtt(UA_PubSubChannelDataMQTT* channelData){
    if(channelData == NULL){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    channelData->callback = NULL;
    struct mqtt_client* client = (struct mqtt_client*)channelData->mqttClient;
    if(client){
        mqtt_disconnect(client);
        yieldMqtt(channelData);
        free(client->socketfd);
    }
    
    if(channelData->connection != NULL){
        channelData->connection->close(channelData->connection);
        channelData->connection->free(channelData->connection);
        free(channelData->connection);
        channelData->connection = NULL;
    }
    free(channelData->mqttRecvBuffer);
    channelData->mqttRecvBuffer = NULL;
    free(channelData->mqttSendBuffer);
    channelData->mqttSendBuffer = NULL;
    free(channelData->mqttClient);
    channelData->mqttClient = NULL;
    
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
             "PubSub Connection disconnected.");
    return UA_STATUSCODE_GOOD;
}

void publish_callback(void** channelDataPtr, struct mqtt_response_publish *published);

void publish_callback(void** channelDataPtr, struct mqtt_response_publish *published) 
{
    //printf("Received publish('%s'): %s\n", "", (const char*) published->application_message);
    if(channelDataPtr != NULL){
        UA_PubSubChannelDataMQTT *channelData = (UA_PubSubChannelDataMQTT*)*channelDataPtr;
        if(channelData != NULL){
            if(channelData->callback != NULL){
                
                //Setup topic
                UA_ByteString *topic = UA_ByteString_new();
                if(!topic) return;
                UA_ByteString *msg = UA_ByteString_new();  
                if(!msg) return;
                
                UA_StatusCode ret = UA_ByteString_allocBuffer(topic, published->topic_name_size);
                if(ret){
                    free(topic); free(msg); return;
                }
                
                ret = UA_ByteString_allocBuffer(msg, published->application_message_size);
                if(ret){
                    UA_ByteString_delete(topic); free(msg); return;
                }
                    
                memcpy(topic->data, published->topic_name, published->topic_name_size);
                memcpy(msg->data, published->application_message, published->application_message_size);
                
                /* callback with message and topic as bytestring. */
                channelData->callback(msg, topic);
            }
        }
    }  
}

UA_StatusCode connectMqtt(UA_PubSubChannelDataMQTT* channelData){
    /* Get address and replace mqtt with tcp 
     * because we use a tcp UA_ClientConnectionTCP for mqtt */
    UA_NetworkAddressUrlDataType address = channelData->address;
    
    UA_String hostname, path;
    UA_UInt16 networkPort;
    if(UA_parseEndpointUrl(&address.url, &hostname, &networkPort, &path) != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid URL.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    /* Build the url, replace mqtt with tcp */
    UA_STACKARRAY(UA_Byte, addressAsChar, 10 + (sizeof(char) * path.length));
    memcpy((char*)addressAsChar, "opc.tcp://", 10);
    memcpy((char*)&addressAsChar[10],(char*)path.data, path.length);
    address.url.data = addressAsChar;
    address.url.length = 10 + (sizeof(char) * path.length);

    /* check if buffers are correct */
    if(!(channelData->mqttRecvBufferSize > 0 && channelData->mqttRecvBuffer != NULL 
            && channelData->mqttSendBufferSize > 0 && channelData->mqttSendBuffer != NULL)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. No Mqtt buffer allocated.");
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
    
    /* Config with default parameters, TODO: adjust */
    UA_ConnectionConfig conf;
    conf.protocolVersion = 0;
    conf.sendBufferSize = 1000;
    conf.recvBufferSize = 2000;
    conf.maxMessageSize = 1000;
    conf.maxChunkCount = 1;
    
    /* Convert UA_String to char* null terminated */
    char* url = (char*)calloc(1, address.url.length + 1);
    if(!url){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memcpy(url, address.url.data, address.url.length);
    
    /* Create TCP connection: open the blocking TCP socket (connecting to the broker) */
    UA_Connection connection = UA_ClientConnectionTCP( conf, url, 1000,NULL);
    free(url); /*free temp url for connect*/
    if(connection.state != UA_CONNECTION_ESTABLISHED && connection.state != UA_CONNECTION_OPENING){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK, "%s", "Tcp connection failed!");
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    
    /* Get the socketfd for mqtt client! */
    int sockfd = connection.sockfd;
    
    /* Set socket to nonblocking!*/
    UA_socket_set_nonblocking(sockfd);
    
    /* save connection */
    channelData->connection = (UA_Connection*)UA_calloc(1, sizeof(UA_Connection));
    if(!channelData->connection){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    memcpy(channelData->connection, &connection, sizeof(UA_Connection));
    
    /* calloc mqtt_client */
    struct mqtt_client* client = (struct mqtt_client*)UA_calloc(1, sizeof(struct mqtt_client));
    if(!client){
        UA_free(channelData->connection);
        
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    /* save reference */
    channelData->mqttClient = client;
    
    /* create custom sockethandle */
    struct my_custom_socket_handle* handle = 
        (struct my_custom_socket_handle*)UA_calloc(1, sizeof(struct my_custom_socket_handle));
    if(!handle){
        UA_free(channelData->connection);
        UA_free(client);
        
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    handle->client = client;
    handle->connection = channelData->connection;
    
    /* init mqtt client struct with buffers and callback */
    enum MQTTErrors mqttErr = mqtt_init(client, handle, channelData->mqttSendBuffer, channelData->mqttSendBufferSize, 
                channelData->mqttRecvBuffer, channelData->mqttRecvBufferSize, publish_callback);
    if(mqttErr != MQTT_OK){
        UA_free(channelData->connection);
        UA_free(client);
        
        const char* errorStr = mqtt_error_str(mqttErr);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", errorStr);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    
    
    /* Init custom data for subscribe callback function: 
     * A reference to the channeldata will be available in the callback.
     * This is used to call the user callback channelData.callback */
    client->publish_response_callback_state = channelData;


    /* Convert clientId UA_String to char* null terminated */
    char* clientId = (char*)calloc(1,channelData->mqttClientId->length + 1);
    if(!clientId){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Out of memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memcpy(clientId, channelData->mqttClientId->data, channelData->mqttClientId->length);

    /* Connect mqtt with socket fd of networktcp  */
    mqttErr = mqtt_connect(client, clientId, NULL, NULL, 0, NULL, NULL, 0, 400);
    free(clientId);
    if(mqttErr != MQTT_OK){
        UA_free(channelData->connection);
        UA_free(client);
        
        const char* errorStr = mqtt_error_str(mqttErr);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", errorStr);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    
    /* sync the first mqtt packets in the buffer to send connection request.
       After that yield must be called frequently to exchange mqtt messages. */
    UA_StatusCode ret = yieldMqtt(channelData);
    if(ret != UA_STATUSCODE_GOOD){
        UA_free(channelData->connection);
        UA_free(client);
        return ret;
    }
    
    /* we did it */
    return UA_STATUSCODE_GOOD;
}



UA_StatusCode subscribeMqtt(UA_PubSubChannelDataMQTT* chanData, UA_String topic, UA_Byte qos){
    struct mqtt_client* client = (struct mqtt_client*)chanData->mqttClient;
    
    UA_STACKARRAY(char, topicStr, sizeof(char) * topic.length +1);
    memcpy(topicStr, topic.data, topic.length);
    topicStr[topic.length] = 0;
    enum MQTTErrors mqttErr = mqtt_subscribe(client, topicStr, (UA_Byte) qos);

    if(mqttErr != MQTT_OK){
        const char* errorStr = mqtt_error_str(mqttErr);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", errorStr);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    
    return UA_STATUSCODE_GOOD;
}


UA_StatusCode unSubscribeMqtt(UA_PubSubChannelDataMQTT* chanData, UA_String topic){

    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

UA_StatusCode yieldMqtt(UA_PubSubChannelDataMQTT* chanData){
    UA_Connection *connection = chanData->connection;
    if(connection == NULL){
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    
    if(connection->state != UA_CONNECTION_ESTABLISHED && connection->state != UA_CONNECTION_OPENING){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK, "PubSub Mqtt yield: Tcp Connection not established!");
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    
    struct mqtt_client* client = (struct mqtt_client*)chanData->mqttClient;
    enum MQTTErrors error = mqtt_sync(client);
    if(error == MQTT_OK){
        return UA_STATUSCODE_GOOD;
    }
    
    if(error == -1){ // is this an unkown error?
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK, "PubSub Mqtt yield: Communication Error.");
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    
    const char* errorStr = mqtt_error_str(error);
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", errorStr);
    
    switch(error){
        case MQTT_ERROR_CONNECTION_CLOSED:
            return UA_STATUSCODE_BADNOTCONNECTED;
        case MQTT_ERROR_SOCKET_ERROR:
            return UA_STATUSCODE_BADCOMMUNICATIONERROR;
        case MQTT_ERROR_CONNECTION_REFUSED:
            return UA_STATUSCODE_BADCONNECTIONREJECTED;
            
        default:
            return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
        /*MQTT_ERROR(MQTT_ERROR_NULLPTR)                 \
    MQTT_ERROR(MQTT_ERROR_CONTROL_FORBIDDEN_TYPE)        \
    MQTT_ERROR(MQTT_ERROR_CONTROL_INVALID_FLAGS)         \
    MQTT_ERROR(MQTT_ERROR_CONTROL_WRONG_TYPE)            \
    MQTT_ERROR(MQTT_ERROR_CONNECT_NULL_CLIENT_ID)        \
    MQTT_ERROR(MQTT_ERROR_CONNECT_NULL_WILL_MESSAGE)     \
    MQTT_ERROR(MQTT_ERROR_CONNECT_FORBIDDEN_WILL_QOS)    \
    MQTT_ERROR(MQTT_ERROR_CONNACK_FORBIDDEN_FLAGS)       \
    MQTT_ERROR(MQTT_ERROR_CONNACK_FORBIDDEN_CODE)        \
    MQTT_ERROR(MQTT_ERROR_PUBLISH_FORBIDDEN_QOS)         \
    MQTT_ERROR(MQTT_ERROR_SUBSCRIBE_TOO_MANY_TOPICS)     \
    MQTT_ERROR(MQTT_ERROR_MALFORMED_RESPONSE)            \
    MQTT_ERROR(MQTT_ERROR_UNSUBSCRIBE_TOO_MANY_TOPICS)   \
    MQTT_ERROR(MQTT_ERROR_RESPONSE_INVALID_CONTROL_TYPE) \
    MQTT_ERROR(MQTT_ERROR_CLIENT_NOT_CONNECTED)          \
    MQTT_ERROR(MQTT_ERROR_SEND_BUFFER_IS_FULL)           \
    MQTT_ERROR(MQTT_ERROR_SOCKET_ERROR)                  \
    MQTT_ERROR(MQTT_ERROR_MALFORMED_REQUEST)             \
    MQTT_ERROR(MQTT_ERROR_RECV_BUFFER_TOO_SMALL)         \
    MQTT_ERROR(MQTT_ERROR_ACK_OF_UNKNOWN)                \
    MQTT_ERROR(MQTT_ERROR_NOT_IMPLEMENTED)               \
    MQTT_ERROR(MQTT_ERROR_CONNECTION_REFUSED)            \
    MQTT_ERROR(MQTT_ERROR_SUBSCRIBE_FAILED)              \
    MQTT_ERROR(MQTT_ERROR_CONNECTION_CLOSED) */
    
    return UA_STATUSCODE_BADCOMMUNICATIONERROR;
}



UA_StatusCode publishMqtt(UA_PubSubChannelDataMQTT* chanData, UA_String topic, const UA_ByteString *buf, UA_Byte qos){
    UA_STACKARRAY(char, topicChar, sizeof(char) * topic.length +1);
    memcpy(topicChar, topic.data, topic.length);
    topicChar[topic.length] = 0;
    
    struct mqtt_client* client = (struct mqtt_client*)chanData->mqttClient;
    if(client == NULL)
        return UA_STATUSCODE_BADNOTCONNECTED;
    
    /* publish */
    enum MQTTPublishFlags flags;
    memset(&flags, 0, sizeof(enum MQTTPublishFlags));
    if(qos == 0){
        flags = MQTT_PUBLISH_QOS_0;
    }else if( qos == 1){
        flags = MQTT_PUBLISH_QOS_1;
    }else if( qos == 2){
        flags = MQTT_PUBLISH_QOS_2;
    }else{
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK, "Bad Qos Level.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    mqtt_publish(client, topicChar, buf->data, buf->length, flags);

    /* check for errors */
    if (client->error != MQTT_OK) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", mqtt_error_str(client->error));
        return UA_STATUSCODE_BADCONNECTIONREJECTED;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode recvMqtt(UA_PubSubChannelDataMQTT* chanData, UA_ByteString *buf){
     
    return yieldMqtt(chanData);
    //mqtt_sync((struct mqtt_client*) &client);

    //if(lastMessage){
    //    buf->data = (UA_Byte*)malloc(lastMessage->application_message_size);
    //    memcpy(buf->data, lastMessage->application_message, lastMessage->application_message_size);
    //    buf->length = lastMessage->application_message_size;
    //}
}
    
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* PLUGIN_MQTT_mqttc_H_ */
