/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2023 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_pubsub.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

/********************/
/* PubSubConnection */
/********************/

static UA_StatusCode
UA_PubSubConnection_connectUDP(UA_Server *server, UA_PubSubConnection *c,
                               UA_Boolean validate);

static UA_StatusCode
UA_PubSubConnection_connectETH(UA_Server *server, UA_PubSubConnection *c,
                               UA_Boolean validate);

static UA_StatusCode
UA_ReaderGroup_connectMQTT(UA_Server *server, UA_ReaderGroup *rg,
                           UA_Boolean validate);

static UA_StatusCode
UA_WriterGroup_connectMQTT(UA_Server *server, UA_WriterGroup *wg,
                           UA_Boolean validate);

static UA_StatusCode
UA_WriterGroup_connectUDPUnicast(UA_Server *server, UA_WriterGroup *wg,
                                 UA_Boolean validate);

#define UA_PUBSUB_PROFILES_SIZE 4

typedef struct  {
    UA_String profileURI;
    UA_String protocol;
    UA_Boolean json;
    UA_StatusCode (*connect)(UA_Server *server, UA_PubSubConnection *c,
                             UA_Boolean validate);
    UA_StatusCode (*connectWriterGroup)(UA_Server *server, UA_WriterGroup *wg,
                                        UA_Boolean validate);
    UA_StatusCode (*connectReaderGroup)(UA_Server *server, UA_ReaderGroup *rg,
                                        UA_Boolean validate);
} ProfileMapping;

static ProfileMapping transportProfiles[UA_PUBSUB_PROFILES_SIZE] = {
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"),
     UA_STRING_STATIC("udp"), false, UA_PubSubConnection_connectUDP,
     UA_WriterGroup_connectUDPUnicast, NULL},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp"),
     UA_STRING_STATIC("mqtt"), false, NULL,
     UA_WriterGroup_connectMQTT, UA_ReaderGroup_connectMQTT},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json"),
     UA_STRING_STATIC("mqtt"), true, NULL,
     UA_WriterGroup_connectMQTT, UA_ReaderGroup_connectMQTT},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"),
     UA_STRING_STATIC("eth"), false, UA_PubSubConnection_connectETH, NULL, NULL}
};

static ProfileMapping *
getProfile(UA_String profileUri) {    
    for(size_t profile = 0; profile < UA_PUBSUB_PROFILES_SIZE; profile++) {
        if(UA_String_equal(&profileUri, &transportProfiles[profile].profileURI))
            return &transportProfiles[profile];
    }
    return NULL;
}

static UA_ConnectionManager *
getCM(UA_EventLoop *el, UA_String protocol) {    
    for(UA_EventSource *es = el->eventSources; es != NULL; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(UA_String_equal(&protocol, &cm->protocol))
            return cm;
    }
    return NULL;
}

static void
UA_PubSubConnection_removeConnection(UA_PubSubConnection *c,
                                     uintptr_t connectionId) {
    if(c->sendChannel == connectionId) {
        c->sendChannel = 0;
        return;
    }
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(c->recvChannels[i] != connectionId)
            continue;
        c->recvChannels[i] = 0;
        c->recvChannelsSize--;
        return;
    }
}

static UA_StatusCode
UA_PubSubConnection_addSendConnection(UA_PubSubConnection *c,
                                      uintptr_t connectionId) {
    if(c->sendChannel != 0 && c->sendChannel != connectionId)
        return UA_STATUSCODE_BADINTERNALERROR;
    c->sendChannel = connectionId;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubConnection_addRecvConnection(UA_PubSubConnection *c,
                                      uintptr_t connectionId) {
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(c->recvChannels[i] == connectionId)
            return UA_STATUSCODE_GOOD;
    }
    if(c->recvChannelsSize >= UA_PUBSUB_MAXCHANNELS)
        return UA_STATUSCODE_BADINTERNALERROR;
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(c->recvChannels[i] != 0)
            continue;
        c->recvChannels[i] = connectionId;
        c->recvChannelsSize++;
        break;
    }
    return UA_STATUSCODE_GOOD;
}

void
UA_PubSubConnection_disconnect(UA_PubSubConnection *c) {   
    if(!c->cm)
        return;
    if(c->sendChannel != 0)
        c->cm->closeConnection(c->cm, c->sendChannel);
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(c->recvChannels[i] != 0)
            c->cm->closeConnection(c->cm, c->recvChannels[i]);
    }
}

static void
PubSubChannelCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                     void *application, void **connectionContext,
                     UA_ConnectionState state, const UA_KeyValueMap *params,
                     UA_ByteString msg, UA_Boolean recv) {
    if(!connectionContext)
        return;

    /* Get the context pointers */
    UA_Server *server = (UA_Server*)application;
    UA_PubSubConnection *psc = (UA_PubSubConnection*)*connectionContext;

    UA_LOCK(&server->serviceMutex);

    /* The connection is closing in the EventLoop. This is the last callback
     * from that connection. Clean up the SecureChannel in the client. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        /* Reset the connection identifiers */
        UA_PubSubConnection_removeConnection(psc, connectionId);

        /* PSC marked for deletion and the last EventLoop connection has closed */
        if(psc->deleteFlag && psc->recvChannelsSize == 0 && psc->sendChannel == 0) {
            UA_PubSubConnection_delete(server, psc);
            UA_UNLOCK(&server->serviceMutex);
            return;
        }

        /* Reconnect automatically if the connection was operational. This sets
         * the connection state if connecting fails. Attention! If there are
         * several send or recv channels, then the connection is only reopened if
         * all of them close - which is usually the case. */
        if(psc->state == UA_PUBSUBSTATE_OPERATIONAL)
            UA_PubSubConnection_connect(server, psc, false);

        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    /* Store the connectionId (if a new connection) */
    UA_StatusCode res = (recv) ?
        UA_PubSubConnection_addRecvConnection(psc, connectionId) :
        UA_PubSubConnection_addSendConnection(psc, connectionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CONNECTION(server->config.logging, psc,
                                  "No more space for an additional EventLoop connection");
        if(psc->cm)
            psc->cm->closeConnection(psc->cm, connectionId);
        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    /* Connection open, set to operational if not already done */
    UA_PubSubConnection_setPubSubState(server, psc, psc->state);

    /* Message received */
    if(UA_LIKELY(recv && msg.length > 0))
        UA_PubSubConnection_process(server, psc, msg);
    
    UA_UNLOCK(&server->serviceMutex);
}

static void
PubSubRecvChannelCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                         void *application, void **connectionContext,
                         UA_ConnectionState state, const UA_KeyValueMap *params,
                         UA_ByteString msg) {
    PubSubChannelCallback(cm, connectionId, application, connectionContext,
                         state, params, msg, true);
}

static void
PubSubSendChannelCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                         void *application, void **connectionContext,
                         UA_ConnectionState state, const UA_KeyValueMap *params,
                         UA_ByteString msg) {
    PubSubChannelCallback(cm, connectionId, application, connectionContext,
                         state, params, msg, false);
}

static UA_StatusCode
UA_PubSubConnection_connectUDP(UA_Server *server, UA_PubSubConnection *c,
                               UA_Boolean validate) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType*)
        c->config.address.data;

    /* Extract hostname and port */
    UA_String address;
    UA_UInt16 port;
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "Could not parse the UDP network URL");
        return res;
    }

    /* Detect a wildcard address for unicast receiving. The individual
     * DataSetWriters then contain additional target hostnames for sending.
     *
     * "localhost" and the empty hostname are used as a special "receive all"
     * wildcard for PubSub UDP. All other addresses (also the 127.0.0/8 and ::1
     * range) are handled differently. For them we only receive messages that
     * originate from these addresses.
     *
     * The EventLoop backend detects whether an address is multicast capable and
     * registers it for the multicast group in the background. */
    UA_String localhostAddr = UA_STRING_STATIC("localhost");
    UA_Boolean receive_all =
        (address.length == 0) || UA_String_equal(&localhostAddr, &address);

    /* Set up the connection parameters */
    UA_Boolean listen = true;
    UA_Boolean reuse = true;
    UA_Boolean loopback = true;
    UA_KeyValuePair kvp[7];
    UA_KeyValueMap kvm = {5, kvp};
    kvp[0].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&kvp[0].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&kvp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    kvp[2].key = UA_QUALIFIEDNAME(0, "validate");
    UA_Variant_setScalar(&kvp[2].value, &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);
    kvp[3].key = UA_QUALIFIEDNAME(0, "reuse");
    UA_Variant_setScalar(&kvp[3].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);
    kvp[4].key = UA_QUALIFIEDNAME(0, "loopback");
    UA_Variant_setScalar(&kvp[4].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(!receive_all) {
        /* The "receive all" wildcard is different in the eventloop UDP layer.
         * Omit the address entirely to receive all.*/
        kvp[5].key = UA_QUALIFIEDNAME(0, "address");
        UA_Variant_setScalar(&kvp[5].value, &address, &UA_TYPES[UA_TYPES_STRING]);
        kvm.mapSize++;
    }
    if(!UA_String_isEmpty(&addressUrl->networkInterface)) {
        kvp[kvm.mapSize].key = UA_QUALIFIEDNAME(0, "interface");
        UA_Variant_setScalar(&kvp[kvm.mapSize].value, &addressUrl->networkInterface,
                             &UA_TYPES[UA_TYPES_STRING]);
        kvm.mapSize++;
    }

    /* Open a recv connection */
    if(c->recvChannelsSize == 0) {
        /* Validate only if no ReaderGroup configured */
        validate = (c->readerGroupsSize == 0);
        if(validate) {
            UA_LOG_INFO_CONNECTION(server->config.logging, c,
                                   "No ReaderGroups configured. "
                                   "Only validate the connection parameters "
                                   "instead of opening a receiving channel.");
        }

        UA_UNLOCK(&server->serviceMutex);
        res = c->cm->openConnection(c->cm, &kvm, server, c, PubSubRecvChannelCallback);
        UA_LOCK(&server->serviceMutex);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                    "Could not open an UDP channel for receiving");
            return res;
        }
    }

    /* Receive all -- sending is handled in the DataSetWriter */
    if(receive_all) {
        UA_LOG_INFO_CONNECTION(server->config.logging, c,
                               "Localhost address - don't open UDP send connection");
        return UA_STATUSCODE_GOOD;
    }

    /* Open a send connection */
    if(c->sendChannel == 0) {
        /* Validate only if no WriterGroup configured */
        validate = (c->writerGroupsSize == 0);
        if(validate) {
            UA_LOG_INFO_CONNECTION(server->config.logging, c,
                                   "No WriterGroups configured. "
                                   "Only validate the connection parameters "
                                   "instead of opening a channel for sending.");
        }

        listen = false;
        UA_UNLOCK(&server->serviceMutex);
        res = c->cm->openConnection(c->cm, &kvm, server, c, PubSubSendChannelCallback);
        UA_LOCK(&server->serviceMutex);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                    "Could not open an UDP recv channel");
            return res;
        }
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubConnection_connectETH(UA_Server *server, UA_PubSubConnection *c,
                               UA_Boolean validate) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType*)
        c->config.address.data;

    /* Extract hostname and port */
    UA_String address;
    UA_String vidPCP = UA_STRING_NULL;
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, NULL, &vidPCP);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "Could not parse the ETH network URL");
        return res;
    }

    /* Set up the connection parameters.
     * TDOD: Complete the considered parameters. VID, PCP, etc. */
    UA_Boolean listen = true;
    UA_KeyValuePair kvp[4];
    UA_KeyValueMap kvm = {4, kvp};
    kvp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&kvp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&kvp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    kvp[2].key = UA_QUALIFIEDNAME(0, "interface");
    UA_Variant_setScalar(&kvp[2].value, &addressUrl->networkInterface,
                         &UA_TYPES[UA_TYPES_STRING]);
    kvp[3].key = UA_QUALIFIEDNAME(0, "validate");
    UA_Variant_setScalar(&kvp[3].value, &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* Open recv channels */
    if(c->recvChannelsSize == 0) {
        UA_UNLOCK(&server->serviceMutex);
        res = c->cm->openConnection(c->cm, &kvm, server, c, PubSubRecvChannelCallback);
        UA_LOCK(&server->serviceMutex);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                    "Could not open an ETH recv channel");
            return res;
        }
    }

    /* Open send channels */
    if(c->sendChannel == 0) {
        listen = false;
        UA_UNLOCK(&server->serviceMutex);
        res = c->cm->openConnection(c->cm, &kvm, server, c, PubSubSendChannelCallback);
        UA_LOCK(&server->serviceMutex);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                    "Could not open an ETH channel for sending");
        }
    }

    return res;
}

static UA_Boolean
UA_PubSubConnection_isConnected(UA_PubSubConnection *c) {
    if(c->sendChannel == 0 && c->writerGroupsSize > 0)
        return false;
    if(c->recvChannelsSize == 0 && c->readerGroupsSize > 0)
        return false;
    return true;
}

UA_StatusCode
UA_PubSubConnection_connect(UA_Server *server, UA_PubSubConnection *c,
                            UA_Boolean validate) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Already connected -> success */
    if(UA_PubSubConnection_isConnected(c) && !validate)
        return UA_STATUSCODE_GOOD;

    UA_EventLoop *el = UA_PubSubConnection_getEL(server, c);
    if(!el) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c, "No EventLoop configured");
        UA_PubSubConnection_setPubSubState(server, c, UA_PUBSUBSTATE_ERROR);
        return UA_STATUSCODE_BADINTERNALERROR;;
    }

    /* Look up the connection manager for the connection */
    ProfileMapping *profile = getProfile(c->config.transportProfileUri);
    UA_ConnectionManager *cm = NULL;
    if(profile)
        cm = getCM(el, profile->protocol);
    if(!cm) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "The requested protocol is not supported");
        UA_PubSubConnection_setPubSubState(server, c, UA_PUBSUBSTATE_ERROR);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Are we changing the protocol after the initial connect? */
    if(c->cm && cm != c->cm) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "The connection is configured for a different protocol already");
        UA_PubSubConnection_setPubSubState(server, c, UA_PUBSUBSTATE_ERROR);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check the configuration address type */
    if(!UA_Variant_hasScalarType(&c->config.address,
                                 &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c, "No NetworkAddressUrlDataType "
                                "for the address configuration");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Connect */
    c->cm = cm;
    c->json = profile->json;
    return (profile->connect) ? profile->connect(server, c, validate) : UA_STATUSCODE_GOOD;
}

/***************/
/* WriterGroup */
/***************/

static void
WriterGroupChannelCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                          void *application, void **connectionContext,
                          UA_ConnectionState state, const UA_KeyValueMap *params,
                          UA_ByteString msg) {
    if(!connectionContext)
        return;

    /* Get the context pointers */
    UA_Server *server = (UA_Server*)application;
    UA_WriterGroup *wg = (UA_WriterGroup*)*connectionContext;

    UA_LOCK(&server->serviceMutex);

    /* The connection is closing in the EventLoop. This is the last callback
     * from that connection. Clean up the SecureChannel in the client. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        if(wg->sendChannel == connectionId) {
            /* Reset the connection channel */
            wg->sendChannel = 0;

            /* PSC marked for deletion and the last EventLoop connection has closed */
            if(wg->deleteFlag) {
                UA_WriterGroup_remove(server, wg);
                UA_UNLOCK(&server->serviceMutex);
                return;
            }
        }

        /* Reconnect automatically if the connection was operational. This sets
         * the connection state if connecting fails. Attention! If there are
         * several send or recv channels, then the connection is only reopened if
         * all of them close - which is usually the case. */
        if(wg->state == UA_PUBSUBSTATE_OPERATIONAL)
            UA_WriterGroup_connect(server, wg, false);

        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    /* Store the connectionId (if a new connection) */
    if(wg->sendChannel && wg->sendChannel != connectionId) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                  "WriterGroup is already bound to a different channel");
        UA_UNLOCK(&server->serviceMutex);
        return;
    }
    wg->sendChannel = connectionId;

    /* Connection open, set to operational if not already done */
    UA_WriterGroup_setPubSubState(server, wg, wg->state);
    
    /* Send-channels don't receive messages */
    UA_UNLOCK(&server->serviceMutex);
}

static UA_StatusCode
UA_WriterGroup_connectUDPUnicast(UA_Server *server, UA_WriterGroup *wg,
                                 UA_Boolean validate) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Already connected? */
    if(wg->sendChannel != 0 && !validate)
        return UA_STATUSCODE_GOOD;

    /* Check if address is available in TransportSettings */
    if(((wg->config.transportSettings.encoding == UA_EXTENSIONOBJECT_DECODED ||
         wg->config.transportSettings.encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) &&
        wg->config.transportSettings.content.decoded.type ==
        &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORTDATATYPE]))
        return UA_STATUSCODE_GOOD;

    /* Unpack the TransportSettings */
    if((wg->config.transportSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        wg->config.transportSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       wg->config.transportSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORT2DATATYPE]) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "Invalid TransportSettings for a UDP Connection");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_DatagramWriterGroupTransport2DataType *ts =
        (UA_DatagramWriterGroupTransport2DataType*)
        wg->config.transportSettings.content.decoded.data;

    /* Unpack the address */
    if((ts->address.encoding != UA_EXTENSIONOBJECT_DECODED &&
        ts->address.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       ts->address.content.decoded.type != &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "Invalid TransportSettings Address for a UDP Connection");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType *)
        ts->address.content.decoded.data;

    /* Extract hostname and port */
    UA_String address;
    UA_UInt16 port;
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                "Could not parse the UDP network URL");
        return res;
    }

    /* Set up the connection parameters */
    UA_Boolean listen = false;
    UA_KeyValuePair kvp[5];
    UA_KeyValueMap kvm = {4, kvp};
    kvp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&kvp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&kvp[1].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    kvp[2].key = UA_QUALIFIEDNAME(0, "listen");
    UA_Variant_setScalar(&kvp[2].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    kvp[3].key = UA_QUALIFIEDNAME(0, "validate");
    UA_Variant_setScalar(&kvp[3].value, &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);
    if(!UA_String_isEmpty(&addressUrl->networkInterface)) {
        kvp[4].key = UA_QUALIFIEDNAME(0, "interface");
        UA_Variant_setScalar(&kvp[4].value, &addressUrl->networkInterface,
                             &UA_TYPES[UA_TYPES_STRING]);
        kvm.mapSize++;
    }

    /* Connect */
    UA_ConnectionManager *cm = wg->linkedConnection->cm;
    UA_UNLOCK(&server->serviceMutex);
    res = cm->openConnection(cm, &kvm, server, wg, WriterGroupChannelCallback);
    UA_LOCK(&server->serviceMutex);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "Could not open a UDP send channel");
    }
    return res;
}

static UA_StatusCode
UA_WriterGroup_connectMQTT(UA_Server *server, UA_WriterGroup *wg,
                           UA_Boolean validate) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_PubSubConnection *c = wg->linkedConnection;
    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType*)
        c->config.address.data;

    /* Get the TransportSettings */
    UA_ExtensionObject *ts = &wg->config.transportSettings;
    if((ts->encoding != UA_EXTENSIONOBJECT_DECODED &&
        ts->encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       ts->content.decoded.type !=
       &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE]) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "Wrong TransportSettings type for MQTT");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_BrokerWriterGroupTransportDataType *transportSettings =
        (UA_BrokerWriterGroupTransportDataType*)ts->content.decoded.data;

    /* Extract hostname and port */
    UA_String address;
    UA_UInt16 port = 1883; /* Default */
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "Could not parse the MQTT network URL");
        return res;
    }

    /* Set up the connection parameters.
     * TODO: Complete the MQTT parameters. */
    UA_Boolean listen = false;
    UA_KeyValuePair kvp[5];
    UA_KeyValueMap kvm = {5, kvp};
    kvp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&kvp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&kvp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    kvp[2].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&kvp[2].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    kvp[3].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&kvp[3].value, &transportSettings->queueName,
                         &UA_TYPES[UA_TYPES_STRING]);
    kvp[4].key = UA_QUALIFIEDNAME(0, "validate");
    UA_Variant_setScalar(&kvp[4].value, &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* Connect */
    UA_UNLOCK(&server->serviceMutex);
    res = c->cm->openConnection(c->cm, &kvm, server, wg, WriterGroupChannelCallback);
    UA_LOCK(&server->serviceMutex);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "Could not open the MQTT connection");
    }
    return res;
}

void
UA_WriterGroup_disconnect(UA_WriterGroup *wg) {
    if(wg->sendChannel == 0)
        return;
    UA_PubSubConnection *c = wg->linkedConnection;
    c->cm->closeConnection(c->cm, c->sendChannel);
}

UA_StatusCode
UA_WriterGroup_connect(UA_Server *server, UA_WriterGroup *wg, UA_Boolean validate) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Check if already connected or no WG TransportSettings */
    if(!UA_WriterGroup_canConnect(wg) && !validate)
        return UA_STATUSCODE_GOOD;

    /* Is this a WriterGroup with custom TransportSettings beyond the
     * PubSubConnection? */
    if(wg->config.transportSettings.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return UA_STATUSCODE_GOOD;

    UA_EventLoop *el = UA_PubSubConnection_getEL(server, wg->linkedConnection);
    if(!el) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg, "No EventLoop configured");
        UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_ERROR);
        return UA_STATUSCODE_BADINTERNALERROR;;
    }

    UA_PubSubConnection *c = wg->linkedConnection;
    if(!c)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Look up the connection manager for the connection */
    ProfileMapping *profile = getProfile(c->config.transportProfileUri);
    UA_ConnectionManager *cm = NULL;
    if(profile)
        cm = getCM(el, profile->protocol);
    if(!cm || (c->cm && cm != c->cm)) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "The requested protocol is not supported");
        UA_PubSubConnection_setPubSubState(server, c, UA_PUBSUBSTATE_ERROR);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    c->cm = cm;
    c->json = profile->json;

    /* Connect */
    if(profile->connectWriterGroup)
        return profile->connectWriterGroup(server, wg, validate);
    return UA_STATUSCODE_GOOD;
}

/***************/
/* ReaderGroup */
/***************/

static void
UA_ReaderGroup_removeConnection(UA_ReaderGroup *rg,
                                uintptr_t connectionId) {
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(rg->recvChannels[i] != connectionId)
            continue;
        rg->recvChannels[i] = 0;
        rg->recvChannelsSize--;
        return;
    }
}

static UA_StatusCode
UA_ReaderGroup_addRecvConnection(UA_ReaderGroup*c,
                                 uintptr_t connectionId) {
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(c->recvChannels[i] == connectionId)
            return UA_STATUSCODE_GOOD;
    }
    if(c->recvChannelsSize >= UA_PUBSUB_MAXCHANNELS)
        return UA_STATUSCODE_BADINTERNALERROR;
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(c->recvChannels[i] != 0)
            continue;
        c->recvChannels[i] = connectionId;
        c->recvChannelsSize++;
        break;
    }
    return UA_STATUSCODE_GOOD;
}

static void
ReaderGroupChannelCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                          void *application, void **connectionContext,
                          UA_ConnectionState state, const UA_KeyValueMap *params,
                          UA_ByteString msg) {
    if(!connectionContext)
        return;

    /* Get the context pointers */
    UA_Server *server = (UA_Server*)application;
    UA_ReaderGroup *rg = (UA_ReaderGroup*)*connectionContext;

    UA_LOCK(&server->serviceMutex);

    /* The connection is closing in the EventLoop. This is the last callback
     * from that connection. Clean up the SecureChannel in the client. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        /* Reset the connection identifiers */
        UA_ReaderGroup_removeConnection(rg, connectionId);

        /* PSC marked for deletion and the last EventLoop connection has closed */
        if(rg->deleteFlag && rg->recvChannelsSize == 0) {
            UA_ReaderGroup_remove(server, rg);
            UA_UNLOCK(&server->serviceMutex);
            return;
        }

        /* Reconnect if still operational */
        UA_ReaderGroup_setPubSubState(server, rg, rg->state);
        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    /* Store the connectionId (if a new connection) */
    UA_StatusCode res = UA_ReaderGroup_addRecvConnection(rg, connectionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                  "No more space for an additional EventLoop connection");
        UA_PubSubConnection *c = rg->linkedConnection;
        if(c && c->cm)
            c->cm->closeConnection(c->cm, connectionId);
        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    /* The connection has opened - set the ReaderGroup to operational */
    UA_ReaderGroup_setPubSubState(server, rg, rg->state);

    /* No message received */
    if(msg.length == 0) {
        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    if(rg->state != UA_PUBSUBSTATE_OPERATIONAL) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "Received a messaage for a non-operational ReaderGroup");
        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    /* ReaderGroup with realtime processing */
    if(rg->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE) {
        UA_ReaderGroup_decodeAndProcessRT(server, rg, &msg);
        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    /* Decode message */
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    if(rg->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP) {
        size_t currentPosition = 0;
        res = decodeNetworkMessage(server, &msg, &currentPosition,
                                   &nm, rg->linkedConnection);
    } else { /* if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) */
#ifdef UA_ENABLE_JSON_ENCODING
        res = UA_NetworkMessage_decodeJson(&msg, &nm, NULL);
#else
        res = UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                  "Verify, decrypt and decode network message failed");
        UA_UNLOCK(&server->serviceMutex);
        return;
    }

    /* Process the decoded message */
    UA_ReaderGroup_process(server, rg, &nm);
    UA_NetworkMessage_clear(&nm);
    UA_UNLOCK(&server->serviceMutex);
}

static UA_StatusCode
UA_ReaderGroup_connectMQTT(UA_Server *server, UA_ReaderGroup *rg,
                           UA_Boolean validate) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_PubSubConnection *c = rg->linkedConnection;
    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType*)
        c->config.address.data;

    /* Get the TransportSettings */
    UA_ExtensionObject *ts = &rg->config.transportSettings;
    if((ts->encoding != UA_EXTENSIONOBJECT_DECODED &&
        ts->encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       ts->content.decoded.type !=
       &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE]) {
        UA_LOG_ERROR_READERGROUP(server->config.logging, rg,
                                "Wrong TransportSettings type for MQTT");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_BrokerDataSetReaderTransportDataType *transportSettings =
        (UA_BrokerDataSetReaderTransportDataType*)ts->content.decoded.data;

    /* Extract hostname and port */
    UA_String address;
    UA_UInt16 port = 1883; /* Default */
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "Could not parse the MQTT network URL");
        return res;
    }

    /* Set up the connection parameters.
     * TODO: Complete the MQTT parameters. */
    UA_Boolean listen = true;
    UA_KeyValuePair kvp[5];
    UA_KeyValueMap kvm = {5, kvp};
    kvp[0].key = UA_QUALIFIEDNAME(0, "address");
    UA_Variant_setScalar(&kvp[0].value, &address, &UA_TYPES[UA_TYPES_STRING]);
    kvp[1].key = UA_QUALIFIEDNAME(0, "subscribe");
    UA_Variant_setScalar(&kvp[1].value, &listen, &UA_TYPES[UA_TYPES_BOOLEAN]);
    kvp[2].key = UA_QUALIFIEDNAME(0, "port");
    UA_Variant_setScalar(&kvp[2].value, &port, &UA_TYPES[UA_TYPES_UINT16]);
    kvp[3].key = UA_QUALIFIEDNAME(0, "topic");
    UA_Variant_setScalar(&kvp[3].value, &transportSettings->queueName,
                         &UA_TYPES[UA_TYPES_STRING]);
    kvp[4].key = UA_QUALIFIEDNAME(0, "validate");
    UA_Variant_setScalar(&kvp[4].value, &validate, &UA_TYPES[UA_TYPES_BOOLEAN]);

    /* Connect */
    UA_UNLOCK(&server->serviceMutex);
    res = c->cm->openConnection(c->cm, &kvm, server, rg, ReaderGroupChannelCallback);
    UA_LOCK(&server->serviceMutex);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READERGROUP(server->config.logging, rg,
                                 "Could not open the MQTT connection");
    }
    return res;
}

void
UA_ReaderGroup_disconnect(UA_ReaderGroup *rg) {
    UA_PubSubConnection *c = rg->linkedConnection;
    if(!c)
        return;
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(rg->recvChannels[i] != 0)
            c->cm->closeConnection(c->cm, rg->recvChannels[i]);
    }
}

UA_StatusCode
UA_ReaderGroup_connect(UA_Server *server, UA_ReaderGroup *rg, UA_Boolean validate) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Already connected */
    if(rg->recvChannelsSize != 0 && !validate)
        return UA_STATUSCODE_GOOD;

    /* Is this a ReaderGroup with custom TransportSettings beyond the
     * PubSubConnection? */
    if(rg->config.transportSettings.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return UA_STATUSCODE_GOOD;

    UA_EventLoop *el = UA_PubSubConnection_getEL(server, rg->linkedConnection);
    if(!el) {
        UA_LOG_ERROR_READERGROUP(server->config.logging, rg, "No EventLoop configured");
        return UA_STATUSCODE_BADINTERNALERROR;;
    }

    UA_PubSubConnection *c = rg->linkedConnection;
    if(!c)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Look up the connection manager for the connection */
    ProfileMapping *profile = getProfile(c->config.transportProfileUri);
    UA_ConnectionManager *cm = NULL;
    if(profile)
        cm = getCM(el, profile->protocol);
    if(!cm || (c->cm && cm != c->cm)) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "The requested protocol is not supported");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    c->cm = cm;
    c->json = profile->json;

    /* Connect */
    if(profile->connectReaderGroup)
        return profile->connectReaderGroup(server, rg, validate);
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_PUBSUB */
