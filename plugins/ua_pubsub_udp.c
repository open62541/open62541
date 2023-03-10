/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright 2018 (c) Jose Cabral, fortiss GmbH
 * Copyright (c) 2020 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2020 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Linutronix GmbH (Author: Kurt Kanzenbach)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Jan Hermes)
 */
#include <open62541/util.h>

#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/server.h>

#include "pubsub_timer.h"

#include "open62541_queue.h"

#define RECEIVE_MSG_BUFFER_SIZE   4096
#define UA_MAX_DEFAULT_PARAM_SIZE 6

#define UA_MULTICAST_TTL_NO_LIMIT 255

#define MAX_URL_LENGTH 512
#define MAX_PORT_CHARACTER_COUNT 6

#define UA_STRING_ANY_ADDR "0.0.0.0"
#define UA_STRING_LOCALHOST "localhost"
#define UA_STRING_LENGTH_LOCALHOST 10

static UA_StatusCode
getRawAddressAndPortValues(const UA_NetworkAddressUrlDataType *address, char *outAddress, UA_UInt16 *outPort) {

    UA_String hostname = UA_STRING_NULL;
    UA_String path = UA_STRING_NULL;
    UA_StatusCode res = UA_parseEndpointUrl(&address->url, &hostname, outPort, &path);
    if(res != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid URL.");
        return res;
    }

    if(hostname.length >= MAX_URL_LENGTH) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. URL maximum length is 512.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    memcpy(outAddress, hostname.data, hostname.length);
    outAddress[hostname.length] = 0;

    return UA_STATUSCODE_GOOD;
}

typedef struct UA_UDPConnectionContext {
    UA_ConnectionManager *connectionManager;
    UA_Server *server;
    void *connection;
    uintptr_t connectionIdPublish;
    uintptr_t connectionIdSubscribe;
    UA_KeyValueMap subscriberParams;
    UA_KeyValueMap publisherParams;
    UA_StatusCode (*decodeAndProcessNetworkMessage)(UA_Server *server,
                                                    void *connection,
                                                    UA_ByteString *buffer);
} UA_UDPConnectionContext;

/* Callback of a TCP socket (server socket or an active connection) */
static void
UA_PubSub_udpCallbackSubscribe(UA_ConnectionManager *cm, uintptr_t connectionId,
                               void *application, void **connectionContext,
                               UA_ConnectionState state, const UA_KeyValueMap *params,
                               UA_ByteString msg) {
    if (state == UA_CONNECTIONSTATE_CLOSED || state == UA_CONNECTIONSTATE_CLOSING) {
        return;
    }

    UA_PubSubChannel *channel = (UA_PubSubChannel *) application;
    channel->sockfd = (int) connectionId;

    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext*) *connectionContext;
    ctx->connectionIdSubscribe = connectionId;
    ctx->connectionManager = cm;

    if(msg.length > 0) {
        ctx->decodeAndProcessNetworkMessage(ctx->server, ctx->connection, &msg);
    }
}

/* Callback of a TCP socket (server socket or an active connection) */
static void
UA_PubSub_udpCallbackPublish(UA_ConnectionManager *cm, uintptr_t connectionId,
                             void *application, void **connectionContext,
                             UA_ConnectionState state, const UA_KeyValueMap *params,
                             UA_ByteString msg) {

    if(state == UA_CONNECTIONSTATE_CLOSED || state == UA_CONNECTIONSTATE_CLOSING || !application) {
        return;
    }
    UA_PubSubChannel *channel = (UA_PubSubChannel *) application;
    channel->sockfd = (int) connectionId;

    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) *connectionContext;
    ctx->connectionIdPublish = connectionId;
    ctx->connectionManager = cm;
}

/**
 * Close channel and free the channel data.
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelUDP_close(UA_PubSubChannel *channel) {
    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) channel->handle;
    if (ctx != NULL) {
        UA_KeyValueMap_clear(&ctx->subscriberParams);
        UA_KeyValueMap_clear(&ctx->publisherParams);
        UA_free(ctx);
    }
    UA_free(channel);
    
    UA_PubSubTimedSend *pubsubTimedSend = (UA_PubSubTimedSend *) channel->pubsubTimedSend;
    if(pubsubTimedSend) {
        /* Clear if any holding packets */
        UA_PublishEntry *timedPublishFrames, *tmpPublishFrames;
        LIST_FOREACH_SAFE(timedPublishFrames, &pubsubTimedSend->sendBuffers, listEntry, tmpPublishFrames) {
            UA_ByteString_clear(&timedPublishFrames->buffer);
            LIST_REMOVE(timedPublishFrames, listEntry);
            UA_free(timedPublishFrames);
        }

        UA_free(pubsubTimedSend);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_openSubscribeDirection(UA_ConnectionManager *connectionManager,
                          const UA_PubSubConnectionConfig *connectionConfig, UA_PubSubChannel *newChannel,
                          UA_NetworkAddressUrlDataType *address, char *addressAsChar, UA_UInt16 port) {
    UA_KeyValueMap *defaultParams = UA_KeyValueMap_new();
    if(!defaultParams) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Boolean listen = true;
    UA_StatusCode res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "listen"), &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    UA_String listenHost = UA_STRING(addressAsChar);
    if(UA_strncasecmp(addressAsChar, UA_STRING_LOCALHOST, UA_STRING_LENGTH_LOCALHOST) == 0){
        listenHost = UA_STRING(UA_STRING_ANY_ADDR);
    }
    UA_Variant listenHostnames; // = UA_Variant_new();
    UA_Variant_setArray(&listenHostnames, &listenHost, 1, &UA_TYPES[UA_TYPES_STRING]);
    res = UA_KeyValueMap_set(defaultParams, UA_QUALIFIEDNAME(0, "address"), &listenHostnames);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    UA_UInt16 targetPort = port;
    res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "port"), &targetPort, &UA_TYPES[UA_TYPES_UINT16]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    if(address->networkInterface.length > 0) {
        res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "interface"), &address->networkInterface, &UA_TYPES[UA_TYPES_STRING]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_KeyValueMap_delete(defaultParams);
            return res;
        }
    }
    UA_Boolean reuse = true;
    res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "reuse"), &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }

    res = UA_KeyValueMap_merge(defaultParams, &connectionConfig->connectionProperties);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }

    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) newChannel->handle;
    res = UA_KeyValueMap_copy(defaultParams, &ctx->subscriberParams);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    UA_Boolean validate = true;
    UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "validate"), &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = connectionManager->openConnection(connectionManager, defaultParams, NULL, NULL, NULL);
    UA_KeyValueMap_delete(defaultParams);
    return res;
}

static UA_StatusCode
UA_openPublishDirection(UA_ConnectionManager *connectionManager,
                        const UA_PubSubConnectionConfig *connectionConfig, UA_PubSubChannel *newChannel,
                        UA_NetworkAddressUrlDataType *address, char *addressAsChar, UA_UInt16 port) {
    UA_KeyValueMap *defaultParams = UA_KeyValueMap_new();
    if(!defaultParams) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_Boolean listen = false;
    UA_StatusCode res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "listen"), &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }

    UA_String targetHost = UA_STRING(addressAsChar);
    res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "address"),
                                                 &targetHost, &UA_TYPES[UA_TYPES_STRING]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    UA_UInt16 targetPort = port;
    res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "port"),
                                   &targetPort, &UA_TYPES[UA_TYPES_UINT16]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    if(address->networkInterface.length > 0) {
        res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "interface"),
                                       &address->networkInterface, &UA_TYPES[UA_TYPES_STRING]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_KeyValueMap_delete(defaultParams);
            return res;
        }
    }
    UA_UInt32 ttl = UA_MULTICAST_TTL_NO_LIMIT;
    res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "ttl"),
                             &ttl, &UA_TYPES[UA_TYPES_UINT32]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    UA_Boolean reuse = true;
    res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "reuse"), &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    UA_Boolean loopback = true;
    res = UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "loopback"), &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    UA_KeyValueMap_merge(defaultParams, &connectionConfig->connectionProperties);

    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) newChannel->handle;
    res = UA_KeyValueMap_copy(defaultParams, &ctx->publisherParams);
    if(res != UA_STATUSCODE_GOOD) {
        UA_KeyValueMap_delete(defaultParams);
        return res;
    }
    UA_Boolean validate = true;
    UA_KeyValueMap_setScalar(defaultParams, UA_QUALIFIEDNAME(0, "validate"), &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = connectionManager->openConnection(connectionManager, defaultParams, NULL, NULL, NULL);
    UA_KeyValueMap_delete(defaultParams);
    return res;
}

static UA_Boolean
startsWith(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

/**
 * Open communication socket based on the connectionConfig. Protocol specific parameters are
 * provided within the connectionConfig as KeyValuePair.
 * Currently supported options: "ttl" , "loopback", "reuse"
 *
 * @return ref to created channel, NULL on error
 */
static UA_PubSubChannel *
UA_PubSubChannelUDP_open(UA_ConnectionManager *connectionManager, UA_TransportLayerContext *ctx, UA_NetworkAddressUrlDataType *address) {
    UA_initialize_architecture_network();

    UA_PubSubConnectionConfig *connectionConfig = ctx->connectionConfig;
    UA_UDPConnectionContext *context = NULL;
    UA_PubSubChannel *newChannel = (UA_PubSubChannel *)
        UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        return NULL;
    }

    char addressAsChar[MAX_URL_LENGTH];
    UA_UInt16 port;
    UA_StatusCode res = getRawAddressAndPortValues(address, addressAsChar, &port);
    if(res != UA_STATUSCODE_GOOD) {
        goto error;
    }
    context = (UA_UDPConnectionContext *) UA_calloc(1, sizeof(UA_UDPConnectionContext));
    if(!context) {
        goto error;
    }
    context->server = ctx->server;
    context->connection = ctx->connection;
    context->connectionManager = connectionManager;
    context->decodeAndProcessNetworkMessage = ctx->decodeAndProcessNetworkMessage;
    newChannel->handle = context; /* Link channel and internal channel data */

    // if the connection does not start with localhost, then it is a multicast connection
    // so the publishing of messages originates from the pubsub connection,
    // otherwise, in the unicast-case the publish direction only originates in
    // a writergroup channel
    if(!startsWith("opc.udp://localhost", (char*) address->url.data)) {
        res = UA_openPublishDirection(connectionManager, connectionConfig, newChannel, address, addressAsChar, port);
        if(res != UA_STATUSCODE_GOOD) {
            goto error;
        }
    }

    res = UA_openSubscribeDirection(connectionManager, connectionConfig, newChannel, address, addressAsChar, port);
    if(res != UA_STATUSCODE_GOOD) {
        goto error;
    }

    /* Set the timedSend to pubsub connection channel for timed publish */
    UA_PubSubTimedSend *pubsubTimedSend = (UA_PubSubTimedSend *) UA_calloc(1, sizeof(UA_PubSubTimedSend));
    if(!pubsubTimedSend) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Bad out of memory");
        goto error;
    }

    return newChannel;
error:
    if(context != NULL) {
        newChannel->handle = NULL;
        UA_free(context);
    }
    UA_PubSubChannelUDP_close(newChannel);
    return NULL;
}

static UA_PubSubChannel *
UA_PubSubChannelUDP_openUnicast(UA_ConnectionManager *connectionManager, UA_TransportLayerContext *ctx, UA_NetworkAddressUrlDataType *address) {
    UA_initialize_architecture_network();

    UA_PubSubConnectionConfig *connectionConfig = ctx->connectionConfig;
    UA_UDPConnectionContext *context = NULL;
    UA_PubSubChannel *newChannel = (UA_PubSubChannel *)
        UA_calloc(1, sizeof(UA_PubSubChannel));
    if(!newChannel) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of memory.");
        return NULL;
    }

    char addressAsChar[MAX_URL_LENGTH];
    UA_UInt16 port;
    UA_StatusCode res = getRawAddressAndPortValues(address, addressAsChar, &port);
    if(res != UA_STATUSCODE_GOOD) {
        goto error;
    }

    context = (UA_UDPConnectionContext *) UA_calloc(1, sizeof(UA_UDPConnectionContext));
    context->server = ctx->server;
    context->connection = ctx->connection;
    context->connectionManager = connectionManager;
    newChannel->handle = context; /* Link channel and internal channel data */

    res = UA_openPublishDirection(connectionManager, connectionConfig, newChannel, address, addressAsChar, port);
    if(res != UA_STATUSCODE_GOOD) {
        goto error;
    }

    return newChannel;
error:
    if(context != NULL) {
        newChannel->handle = NULL;
        UA_free(context);
    }
    UA_free(newChannel);
    return NULL;
}

/**
 * Subscribe to a given address.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelUDP_regist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                           void (*notUsedHere)(UA_ByteString *encodedBuffer,
                                                 UA_ByteString *topic)) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB || channel->state == UA_PUBSUB_CHANNEL_RDY)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection regist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/**
 * Remove current subscription.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubChannelUDP_unregist(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings) {
    if(!(channel->state == UA_PUBSUB_CHANNEL_PUB_SUB || channel->state == UA_PUBSUB_CHANNEL_SUB)){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "PubSub Connection unregist failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/**
 * Send messages to the connection defined address
 *
 * @return UA_STATUSCODE_GOOD if success
 */
static UA_StatusCode
UA_PubSubChannelUDP_send(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                         UA_ByteString *buf) {

    UA_UDPConnectionContext *udpContext = (UA_UDPConnectionContext *) channel->handle;
    UA_ConnectionManager *cm = udpContext->connectionManager;

    uintptr_t connectionId = udpContext->connectionIdPublish;

    return cm->sendWithConnection(cm, connectionId, &UA_KEYVALUEMAP_NULL, buf);
}

/**
 * Receive messages. The regist function should be called before.
 *
 * @param timeout in usec | on windows platforms are only multiples of 1000usec possible
 * @return
 */
static UA_StatusCode
UA_PubSubChannelUDP_receive(UA_PubSubChannel *channel,
                            UA_ExtensionObject *transportSettings,
                            UA_PubSubReceiveCallback receiveCallback,
                            void *receiveCallbackContext,
                            UA_UInt32 timeout) {
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubChannelUDP_openPublisher(UA_PubSubChannel *channel) {
    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) channel->handle;
    ctx->connectionManager->openConnection(ctx->connectionManager, &ctx->publisherParams,
                                           channel, ctx, UA_PubSub_udpCallbackPublish);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubChannelUDP_openSubscriber(UA_PubSubChannel *channel) {
    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) channel->handle;
    ctx->connectionManager->openConnection(ctx->connectionManager, &ctx->subscriberParams,
                                           channel, ctx, UA_PubSub_udpCallbackSubscribe);
    return UA_STATUSCODE_GOOD;
}
static UA_StatusCode
UA_PubSubChannelUDP_closePublisher(UA_PubSubChannel *channel) {
    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) channel->handle;
    // UA_KeyValueMap_clear(&ctx->publisherParams);
    ctx->connectionManager->closeConnection(ctx->connectionManager, ctx->connectionIdPublish);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubChannelUDP_closeSubscriber(UA_PubSubChannel *channel) {
    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) channel->handle;
    // UA_KeyValueMap_clear(&ctx->subscriberParams);
    ctx->connectionManager->closeConnection(ctx->connectionManager, ctx->connectionIdSubscribe);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubChannelUDP_allocNetworkBuffer(UA_PubSubChannel *channel, UA_ByteString *buf, size_t bufSize) {
    UA_UDPConnectionContext *ctx = (UA_UDPConnectionContext *) channel->handle;
    return ctx->connectionManager->allocNetworkBuffer(ctx->connectionManager, ctx->connectionIdPublish, buf, bufSize);
}
static UA_StatusCode
UA_PubSubChannelUDP_freeNetworkBuffer(UA_PubSubChannel *channel, UA_ByteString *buf) {
    /* noop */
    return UA_STATUSCODE_GOOD;
}
/**
 * Generate a new channel. based on the given configuration.
 *
 * @param connectionConfig connection configuration
 * @return  ref to created channel, NULL on error
 */
static UA_PubSubChannel *
TransportLayerUDP_addChannel(UA_PubSubTransportLayer *tl, void *ctx) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PubSub channel requested");
    UA_TransportLayerContext *tctx  = (UA_TransportLayerContext *) ctx;
    UA_PubSubConnectionConfig *connectionConfig = tctx->connectionConfig;
    UA_PubSubChannel *pubSubChannel = NULL;
    UA_Server *server = tctx->server;

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_EventLoop *el = config->eventLoop;
    UA_String needle = UA_STRING("udp connection manager");

    UA_EventSource *es;
    for(es = el->eventSources; es != NULL; es=es->next) {
        if(UA_String_equal(&es->name, &needle)) {
            break;
        }
    }
    if(!es) {
        return NULL;
    }
    tl->connectionManager = (UA_ConnectionManager *) es;

    if(!UA_Variant_hasScalarType(&connectionConfig->address,
                                 &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Invalid Address.");
        return NULL;
    }
    UA_NetworkAddressUrlDataType *address =
        (UA_NetworkAddressUrlDataType *)connectionConfig->address.data;

    if(tctx->writerGroupAddress)  {
        pubSubChannel = UA_PubSubChannelUDP_openUnicast((UA_ConnectionManager*) tl->connectionManager, tctx, tctx->writerGroupAddress);
    } else {
        pubSubChannel = UA_PubSubChannelUDP_open((UA_ConnectionManager*) tl->connectionManager, tctx, address);
    }

    if(pubSubChannel){
        pubSubChannel->regist = UA_PubSubChannelUDP_regist;
        pubSubChannel->unregist = UA_PubSubChannelUDP_unregist;
        pubSubChannel->send = UA_PubSubChannelUDP_send;
        pubSubChannel->receive = UA_PubSubChannelUDP_receive;
        pubSubChannel->close = UA_PubSubChannelUDP_close;
        pubSubChannel->closeSubscriber = UA_PubSubChannelUDP_closeSubscriber;
        pubSubChannel->closePublisher = UA_PubSubChannelUDP_closePublisher;
        pubSubChannel->openSubscriber = UA_PubSubChannelUDP_openSubscriber;
        pubSubChannel->openPublisher = UA_PubSubChannelUDP_openPublisher;
        pubSubChannel->allocNetworkBuffer = UA_PubSubChannelUDP_allocNetworkBuffer;
        pubSubChannel->freeNetworkBuffer = UA_PubSubChannelUDP_freeNetworkBuffer;
        pubSubChannel->connectionConfig = connectionConfig;

        pubSubChannel->state = UA_PUBSUB_CHANNEL_PUB;
    }
    return pubSubChannel;
}

static UA_StatusCode
TransportLayerUDP_addWritergroupChannel(UA_PubSubChannel **out, UA_PubSubTransportLayer *tl, const UA_ExtensionObject *writerGroupTransportSettings, void* ctx)  {
    UA_TransportLayerContext *tctx  = (UA_TransportLayerContext *) ctx;
    if(writerGroupTransportSettings && writerGroupTransportSettings->content.decoded.type == &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORT2DATATYPE]) {
        UA_DatagramWriterGroupTransport2DataType *ts = (UA_DatagramWriterGroupTransport2DataType *) writerGroupTransportSettings->content.decoded.data;
        const char *assertedPrefix = "opc.udp://localhost:";

        if(strncmp(assertedPrefix, (char *) tctx->connectionAddress->url.data, strlen(assertedPrefix)) != 0) {

            UA_LOG_ERROR(tctx->logger, UA_LOGCATEGORY_PUBSUB,
                         "PubSub WriterGroup creating failed. Invalid Address of PubSub Connection.");
            return UA_STATUSCODE_BADCONNECTIONREJECTED;
        }

        if(ts->address.content.decoded.type == &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]) {
            UA_NetworkAddressUrlDataType *address = (UA_NetworkAddressUrlDataType *) ts->address.content.decoded.data;
            const char *prefix = "opc.udp://";
            if(strncmp(prefix, (char *) address->url.data, strlen(prefix)) == 0) {

                /* Retrieve the transport layer for the given profile uri */
                tctx->writerGroupAddress = address;
                *out = tl->createPubSubChannel(tl, tctx);
                return UA_STATUSCODE_GOOD;
            } else {
                return UA_STATUSCODE_BADCONNECTIONREJECTED;
            }
        } else {
            return UA_STATUSCODE_BADCONNECTIONREJECTED;
        }
    } else {
        return UA_STATUSCODE_GOOD;
    }
}

//UDP channel factory
UA_PubSubTransportLayer
UA_PubSubTransportLayerUDP(void) {
    // UA_ServerConfig *config = UA_Server_getConfig(server);
    // UA_EventLoop *el = config->eventLoop;

    UA_PubSubTransportLayer pubSubTransportLayer;
    memset(&pubSubTransportLayer, 0, sizeof(UA_PubSubTransportLayer));
    // pubSubTransportLayer.connectionManager = UA_ConnectionManager_new_POSIX_UDP(UA_STRING("udp-cm"));
    // pubSubTransportLayer.server = server;
    // el->registerEventSource(el, &pubSubTransportLayer.connectionManager->eventSource);

    pubSubTransportLayer.transportProfileUri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    pubSubTransportLayer.createPubSubChannel = &TransportLayerUDP_addChannel;
    pubSubTransportLayer.createWriterGroupPubSubChannel= &TransportLayerUDP_addWritergroupChannel;
    pubSubTransportLayer.connectionManager = NULL;
    return pubSubTransportLayer;
}
