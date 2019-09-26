/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#ifndef UA_PLUGIN_PUBSUB_H_
#define UA_PLUGIN_PUBSUB_H_

#include <open62541/server_pubsub.h>

_UA_BEGIN_DECLS

#ifdef UA_ENABLE_PUBSUB

/**
 * .. _pubsub_connection:
 *
 * PubSub Connection Plugin API
 * ============================
 *
 * The PubSub Connection API is the interface between concrete network
 * implementations and the internal pubsub code.
 *
 * The PubSub specification enables the creation of new connections on runtime.
 * Wording: 'Connection' -> OPC UA standard 'highlevel' perspective, 'Channel'
 * -> open62541 implementation 'lowlevel' perspective. A channel can be assigned
 * with different network implementations like UDP, MQTT, AMQP. The channel
 * provides basis services like send, regist, unregist, receive, close. */

typedef enum {
    UA_PUBSUB_CHANNEL_RDY,
    UA_PUBSUB_CHANNEL_PUB,
    UA_PUBSUB_CHANNEL_SUB,
    UA_PUBSUB_CHANNEL_PUB_SUB,
    UA_PUBSUB_CHANNEL_ERROR,
    UA_PUBSUB_CHANNEL_CLOSED
} UA_PubSubChannelState;

struct UA_PubSubChannel;
typedef struct UA_PubSubChannel UA_PubSubChannel;

/* Interface structure between network plugin and internal implementation */
struct UA_PubSubChannel {
    UA_UInt32 publisherId; /* unique identifier */
    UA_PubSubChannelState state;
    UA_PubSubConnectionConfig *connectionConfig; /* link to parent connection config */
    UA_SOCKET sockfd;
    void *handle; /* implementation specific data */
    /*@info for handle: each network implementation should provide an structure
    * UA_PubSubChannelData[ImplementationName] This structure can be used by the
    * network implementation to store network implementation specific data.*/

    /* Sending out the content of the buf parameter */
    UA_StatusCode (*send)(UA_PubSubChannel *channel, UA_ExtensionObject *transportSettings,
                          const UA_ByteString *buf);

    /* Register to an specified message source, e.g. multicast group or topic. Callback is used for mqtt. */
    UA_StatusCode (*regist)(UA_PubSubChannel * channel, UA_ExtensionObject *transportSettings,
        void (*callback)(UA_ByteString *encodedBuffer, UA_ByteString *topic));

    /* Remove subscription to an specified message source, e.g. multicast group or topic */
    UA_StatusCode (*unregist)(UA_PubSubChannel * channel, UA_ExtensionObject *transportSettings);

    /* Receive messages. A regist to the message source is needed before. */
    UA_StatusCode (*receive)(UA_PubSubChannel * channel, UA_ByteString *,
                             UA_ExtensionObject *transportSettings, UA_UInt32 timeout);

    /* Closing the connection and implicit free of the channel structures. */
    UA_StatusCode (*close)(UA_PubSubChannel *channel);

    /* Giving the connection protocoll time to process inbound and outbound traffic. */
    UA_StatusCode (*yield)(UA_PubSubChannel *channel, UA_UInt16 timeout);
};

/**
 * The UA_PubSubTransportLayer is used for the creation of new connections.
 * Whenever on runtime a new connection is request, the internal PubSub
 * implementation call * the 'createPubSubChannel' function. The
 * 'transportProfileUri' contains the standard defined transport profile
 * information and is used to identify the type of connections which can be
 * created by the TransportLayer. The server config contains a list of
 * UA_PubSubTransportLayer. Take a look in the tutorial_pubsub_connection to get
 * informations about the TransportLayer handling. */

typedef struct {
    UA_String transportProfileUri;
    UA_PubSubChannel *(*createPubSubChannel)(UA_PubSubConnectionConfig *connectionConfig);
} UA_PubSubTransportLayer;

/**
 * The UA_ServerConfig_addPubSubTransportLayer is used to add a transport layer
 * to the server configuration. The list memory is allocated and will be freed
 * with UA_PubSubManager_delete.
 *
 * .. note:: If the UA_String transportProfileUri was dynamically allocated
 *           the memory has to be freed when no longer required.
 *
 * .. note:: This has to be done before the server is started with UA_Server_run. */
UA_StatusCode UA_EXPORT
UA_ServerConfig_addPubSubTransportLayer(UA_ServerConfig *config,
                                        UA_PubSubTransportLayer *pubsubTransportLayer);

#endif /* UA_ENABLE_PUBSUB */

_UA_END_DECLS

#endif /* UA_PLUGIN_PUBSUB_H_ */
