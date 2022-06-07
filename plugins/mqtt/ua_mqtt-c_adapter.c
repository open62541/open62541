/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Lukas Meling)
 * Copyright (c) 2020 basysKom GmbH
 */

#include "ua_mqtt-c_adapter.h"
#include <mqtt.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/util.h>

#if defined(UA_ENABLE_MQTT_TLS_OPENSSL)
#include <openssl_sockets.h>
#elif defined(UA_ENABLE_MQTT_TLS_MBEDTLS)
#include <mbedtls_sockets.h>
#else
#include <posix_sockets.h>
#endif
#include <time.h>


/* forward decl for callback */
void
publish_callback(void**, struct mqtt_response_publish*);

void freeTLS(UA_PubSubChannelDataMQTT *data) {
#if defined(UA_ENABLE_MQTT_TLS_OPENSSL)
    if (!data->sockfd)
        return;
    BIO_free_all(data->sockfd);
    //SSL_CTX_free(SSL_get_SSL_CTX(data->ssl));
    data->sockfd = NULL;
#elif defined(UA_ENABLE_MQTT_TLS_MBEDTLS)
    if (!data->sockfd)
        return;
    mbedtls_ssl_free(data->sockfd);
    data->sockfd = NULL;
#else
    close(data->sockfd);
#endif
}

UA_StatusCode
connectMqtt(UA_PubSubChannelDataMQTT* channelData){
    if(channelData == NULL){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

#if defined(UA_ENABLE_MQTT_TLS_OPENSSL) || defined(UA_ENABLE_MQTT_TLS_MBEDTLS) // Extend condition when mbedTLS support is added
    if ((channelData->mqttClientCertPath.length && !channelData->mqttClientKeyPath.length) ||
            (channelData->mqttClientKeyPath.length && !channelData->mqttClientCertPath.length)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "MQTT PubSub: If a client certificate is used, mqttClientCertPath and mqttClientKeyPath must be both specified");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
#else
    if (channelData->mqttUseTLS) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "MQTT PubSub: TLS connection requested but open62541 has been built without TLS support");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
#endif

    /* Get address and remove 'opc.mqtt://' from the address */
    UA_NetworkAddressUrlDataType address = channelData->address;

    UA_String hostname, path;
    UA_UInt16 networkPort;
    if(UA_parseEndpointUrl(&address.url, &hostname, &networkPort, &path) != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "MQTT PubSub Connection creation failed. Invalid URL.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Build the url and port*/
    char rest[512];
    memcpy(rest, path.data, path.length);
    rest[path.length] = '\0';
    char *rest2 = rest;
    const char *addr = strtok_r(rest2, ":", &rest2);
    const char *port = strtok_r(rest2, ":", &rest2);

#if defined(UA_ENABLE_MQTT_TLS_OPENSSL) || defined(UA_ENABLE_MQTT_TLS_MBEDTLS)
#if defined(UA_ENABLE_MQTT_TLS_OPENSSL)
    BIO *sockfd = NULL;
    SSL_CTX *ctx;
#else
    struct mbedtls_context ctx;
    mqtt_pal_socket_handle sockfd;
#endif
    if (channelData->mqttUseTLS) {
        char *mqttCaFilePath = NULL;
        if(channelData->mqttCaFilePath.length > 0) {
            /* Convert tls certificate path UA_String to char* null terminated */
            mqttCaFilePath = (char *)calloc(1, channelData->mqttCaFilePath.length + 1);
            if(!mqttCaFilePath) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                             "PubSub MQTT: Connection creation failed. Out of memory.");
                // TODO: There are several places in the existing code where channelData->connection is freed but not set to NULL
                // The call to disconnectMqtt by the function calling this function in case of error causes heap-use-after-free
                // Where should channelData->connection be cleaned up?
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            memcpy(mqttCaFilePath, channelData->mqttCaFilePath.data, channelData->mqttCaFilePath.length);
        }

        char *mqttCaPath = NULL;
        if(channelData->mqttCaPath.length > 0) {
            /* Convert tls CA path UA_String to char* null terminated */
            mqttCaPath = (char *)calloc(1, channelData->mqttCaPath.length + 1);
            if(!mqttCaPath) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                             "PubSub MQTT: Connection creation failed. Out of memory (mqttCAPath).");
                UA_free(mqttCaFilePath);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            memcpy(mqttCaPath, channelData->mqttCaPath.data, channelData->mqttCaPath.length);
        }

        char *mqttClientCertPath = NULL;
        if(channelData->mqttClientCertPath.length > 0) {
            /* Convert tls client cert path UA_String to char* null terminated */
            mqttClientCertPath = (char *)calloc(1, channelData->mqttClientCertPath.length + 1);
            if(!mqttClientCertPath) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                             "PubSub MQTT: Connection creation failed. Out of memory (mqttClientCertPath).");
                UA_free(mqttCaFilePath);
                UA_free(mqttCaPath);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            memcpy(mqttClientCertPath, channelData->mqttClientCertPath.data, channelData->mqttClientCertPath.length);
        }

        char *mqttClientKeyPath = NULL;
        if(channelData->mqttClientKeyPath.length > 0) {
            /* Convert tls client key path UA_String to char* null terminated */
            mqttClientKeyPath = (char *)calloc(1, channelData->mqttClientKeyPath.length + 1);
            if(!mqttClientKeyPath) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                             "PubSub MQTT: Connection creation failed. Out of memory (mqttClientKeyPath).");
                UA_free(mqttCaFilePath);
                UA_free(mqttCaPath);
                UA_free(mqttClientCertPath);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            memcpy(mqttClientKeyPath, channelData->mqttClientKeyPath.data, channelData->mqttClientKeyPath.length);
        }
#endif
#if defined(UA_ENABLE_MQTT_TLS_OPENSSL) // Extend condition when mbedTLS support is added
        /* open the non-blocking TCP socket (connecting to the broker) */
        UA_StatusCode rv = open_nb_socket(&sockfd, &ctx, addr, port, mqttCaFilePath, NULL, NULL, NULL);;
        if(rv != UA_STATUSCODE_GOOD){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed.");
            UA_free(mqttCaFilePath);
            UA_free(mqttCaPath);
            UA_free(mqttClientCertPath);
            UA_free(mqttClientKeyPath);
            return rv;
        }

        if (!ctx) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed.");
            UA_free(mqttCaFilePath);
            UA_free(mqttCaPath);
            UA_free(mqttClientCertPath);
            UA_free(mqttClientKeyPath);
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: TLS connection successfully opened.");
    }

#elif defined(UA_ENABLE_MQTT_TLS_MBEDTLS)
        /* open the non-blocking TCP socket (connecting to the broker) */
        UA_StatusCode rv = open_nb_socket(&ctx,addr,port,mqttCaFilePath);
        if(rv != UA_STATUSCODE_GOOD){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed.");
            UA_free(mqttCaFilePath);
            UA_free(mqttCaPath);
            UA_free(mqttClientCertPath);
            UA_free(mqttClientKeyPath);
            return rv;
        }
        sockfd = &ctx.ssl_ctx;

        if (sockfd == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed.");
            UA_free(mqttCaFilePath);
            UA_free(mqttCaPath);
            UA_free(mqttClientCertPath);
            UA_free(mqttClientKeyPath);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
    }
#else
    int sockfd = -1;
    /* open the non-blocking TCP socket (connecting to the broker) */
    UA_StatusCode rv = open_nb_socket(&sockfd,addr, port);
    if(rv != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed.");
        return rv;
    }
#endif

    /* calloc mqtt_client */
    struct mqtt_client* client = (struct mqtt_client*)UA_calloc(1, sizeof(struct mqtt_client));
    if(!client){
        freeTLS(channelData);
        UA_free(channelData->connection);

        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed. Out of memory (mqtt_client).");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    client->keep_alive = 400;

    /* save reference */
    channelData->mqttClient = client;

    /* save socket fd*/
    channelData->sockfd = sockfd;

    /* init mqtt client struct with buffers and callback */
    enum MQTTErrors mqttErr = mqtt_init(client, sockfd, channelData->mqttSendBuffer, channelData->mqttSendBufferSize,
                channelData->mqttRecvBuffer, channelData->mqttRecvBufferSize, publish_callback);
    if(mqttErr != MQTT_OK){
        freeTLS(channelData);
        UA_free(channelData->connection);
        UA_free(client);

        const char* errorStr = mqtt_error_str(mqttErr);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed. MQTT error (sendBuffer & RecvBuffer): %s", errorStr);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    /* Init custom data for subscribe callback function:
     * A reference to the channeldata will be available in the callback.
     * This is used to call the user callback channelData.callback */
    client->publish_response_callback_state = channelData;

    /* Convert clientId UA_String to char* null terminated */
    char* clientId = (char*)calloc(1,channelData->mqttClientId->length + 1);
    if(!clientId){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed. Out of memory (mqttClientId).");
        freeTLS(channelData);
        UA_free(channelData->connection);
        UA_free(client);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    memcpy(clientId, channelData->mqttClientId->data, channelData->mqttClientId->length);

    char *username = NULL;
    if (channelData->mqttUsername.length > 0) {
        /* Convert username UA_String to char* null terminated */
        username = (char*)calloc(1,channelData->mqttUsername.length + 1);
        if(!username){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed. Out of memory (mqttUsername).");
            freeTLS(channelData);
            UA_free(channelData->connection);
            UA_free(client);
            UA_free(clientId);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        memcpy(username, channelData->mqttUsername.data, channelData->mqttUsername.length);
    }

    char *password = NULL;
    if (channelData->mqttPassword.length > 0) {
        /* Convert password UA_String to char* null terminated */
        password = (char*)calloc(1,channelData->mqttPassword.length + 1);
        if(!password){
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: Connection creation failed. Out of memory (mqttPassword).");
            freeTLS(channelData);
            UA_free(channelData->connection);
            UA_free(client);
            UA_free(clientId);
            UA_free(username);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        memcpy(password, channelData->mqttPassword.data, channelData->mqttPassword.length);
    }

    /* Connect mqtt with socket */
    mqttErr = mqtt_connect(client, clientId, NULL, NULL, 0, username, password,
                           0, client->keep_alive);
    UA_free(clientId);
    UA_free(username);
    UA_free(password);

    if(mqttErr != MQTT_OK){
        freeTLS(channelData);
        UA_free(channelData->connection);
        UA_free(client);

        const char* errorStr = mqtt_error_str(mqttErr);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "%s", errorStr);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
disconnectMqtt(UA_PubSubChannelDataMQTT* channelData){
    if(channelData == NULL){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    channelData->callback = NULL;
    struct mqtt_client* client = (struct mqtt_client*)channelData->mqttClient;
    if(client){
        mqtt_disconnect(client);
        yieldMqtt(channelData, 10);
#ifdef UA_ENABLE_MQTT_TLS_OPENSSL //mbedTLS condition is missing
        BIO_free_all(client->socketfd);
#endif
    }

    freeTLS(channelData);

    UA_free(channelData->mqttRecvBuffer);
    channelData->mqttRecvBuffer = NULL;
    UA_free(channelData->mqttSendBuffer);
    channelData->mqttSendBuffer = NULL;
    UA_free(channelData->mqttClient);
    channelData->mqttClient = NULL;
    return UA_STATUSCODE_GOOD;
}

void
publish_callback(void** channelDataPtr, struct mqtt_response_publish *published)
{
    if(channelDataPtr != NULL){
        UA_PubSubChannelDataMQTT *channelData = (UA_PubSubChannelDataMQTT*)*channelDataPtr;
        if(channelData != NULL){
            if(channelData->callback != NULL){
                //Setup topic
                UA_ByteString *topic = UA_ByteString_new();
                if(!topic) return;
                UA_ByteString *msg = UA_ByteString_new();
                if(!msg) return;

                /* memory for topic */
                UA_StatusCode ret = UA_ByteString_allocBuffer(topic, published->topic_name_size);
                if(ret){
                    UA_free(topic);
                    UA_free(msg);
                    return;
                }
                /* memory for message */
                ret = UA_ByteString_allocBuffer(msg, published->application_message_size);
                if(ret){
                    UA_ByteString_delete(topic);
                    UA_free(msg);
                    return;
                }
                /* copy topic and msg, call the cb */
                memcpy(topic->data, published->topic_name, published->topic_name_size);
                memcpy(msg->data, published->application_message, published->application_message_size);
                channelData->callback(msg, topic);
            }
        }
    }
}

UA_StatusCode
subscribeMqtt(UA_PubSubChannelDataMQTT* channelData, UA_String topic, UA_Byte qos){
    if(channelData == NULL || topic.length == 0){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    struct mqtt_client* client = (struct mqtt_client*)channelData->mqttClient;

    UA_STACKARRAY(char, topicStr, sizeof(char) * topic.length +1);
    memcpy(topicStr, topic.data, topic.length);
    topicStr[topic.length] = '\0';

    enum MQTTErrors mqttErr = mqtt_subscribe(client, topicStr, (UA_Byte) qos);
    if(mqttErr != MQTT_OK){
        const char* errorStr = mqtt_error_str(mqttErr);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: subscribe: %s", errorStr);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
unSubscribeMqtt(UA_PubSubChannelDataMQTT* channelData, UA_String topic){
    if(channelData == NULL || topic.length == 0){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    struct mqtt_client* client = (struct mqtt_client*)channelData->mqttClient;

    UA_STACKARRAY(char, topicStr, sizeof(char) * topic.length +1);
    memcpy(topicStr, topic.data, topic.length);
    topicStr[topic.length] = '\0';

    enum MQTTErrors mqttErr = mqtt_unsubscribe(client, topicStr);
    if(mqttErr != MQTT_OK){
        const char* errorStr = mqtt_error_str(mqttErr);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: subscribe: %s", errorStr);
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
yieldMqtt(UA_PubSubChannelDataMQTT* channelData, UA_UInt16 timeout){

    struct mqtt_client* client = (struct mqtt_client*)channelData->mqttClient;

    enum MQTTErrors error = mqtt_sync(client);
    if(error == MQTT_OK){
        return UA_STATUSCODE_GOOD;
    }else if(error == -1){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK, "PubSub MQTT: yield: Communication Error.");
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }

    /* map mqtt errors to ua errors */
    const char* errorStr = mqtt_error_str(error);
    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: yield: error: %s", errorStr);

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
}

UA_StatusCode
publishMqtt(UA_PubSubChannelDataMQTT* channelData, UA_String topic, const UA_ByteString *buf, UA_Byte qos){
    if(channelData == NULL || buf == NULL ){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_STACKARRAY(char, topicChar, sizeof(char) * topic.length +1);
    memcpy(topicChar, topic.data, topic.length);
    topicChar[topic.length] = '\0';

    struct mqtt_client* client = (struct mqtt_client*)channelData->mqttClient;
    if(client == NULL)
        return UA_STATUSCODE_BADNOTCONNECTED;

    /* publish */
    enum MQTTPublishFlags flags;
    if(qos == 0){
        flags = MQTT_PUBLISH_QOS_0;
    }else if( qos == 1){
        flags = MQTT_PUBLISH_QOS_1;
    }else if( qos == 2){
        flags = MQTT_PUBLISH_QOS_2;
    }else{
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK, "PubSub MQTT: publish: Bad Qos Level.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    enum MQTTErrors mqttErr = mqtt_publish(client, topicChar, buf->data, buf->length, (uint8_t)flags);
    if(mqttErr != MQTT_OK){
        if(mqttErr == MQTT_ERROR_SEND_BUFFER_IS_FULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: publish: Send buffer is full. "
                                                               "Possible reasons: send buffer is to small, "
                                                               "sending to fast, broker not responding.");
        }else{
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub MQTT: publish: %s", mqtt_error_str(mqttErr));
        }
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    }
    return UA_STATUSCODE_GOOD;
}
