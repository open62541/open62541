/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2022 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Noel Graf)
 */

#include "ua_pubsub.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#ifdef UA_ENABLE_PUBSUB_MQTT
#include "../../plugins/mqtt/ua_mqtt-c_adapter.h"
#include "mqtt.h"
#endif

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src,
                               UA_PubSubConnectionConfig *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PubSubConnectionConfig));
    if (src->publisherIdType == UA_PUBLISHERIDTYPE_STRING) {
        res |= UA_String_copy(&src->publisherId.string, &dst->publisherId.string);
    }
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_Variant_copy(&src->address, &dst->address);
    res |= UA_String_copy(&src->transportProfileUri, &dst->transportProfileUri);
    res |= UA_Variant_copy(&src->connectionTransportSettings,
                           &dst->connectionTransportSettings);
    res |= UA_KeyValueMap_copy(&src->connectionProperties,
                               &dst->connectionProperties);
    if(res != UA_STATUSCODE_GOOD)
        UA_PubSubConnectionConfig_clear(dst);
    return res;
}

UA_StatusCode
UA_Server_getPubSubConnectionConfig(UA_Server *server, const UA_NodeId connection,
                                    UA_PubSubConnectionConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubConnection *currentPubSubConnection =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    if(currentPubSubConnection)
        res = UA_PubSubConnectionConfig_copy(&currentPubSubConnection->config, config);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_Server *server, UA_NodeId connectionIdentifier) {
    UA_PubSubConnection *pubSubConnection;
    TAILQ_FOREACH(pubSubConnection, &server->pubSubManager.connections, listEntry){
        if(UA_NodeId_equal(&connectionIdentifier, &pubSubConnection->identifier))
            break;
    }
    return pubSubConnection;
}

void
UA_PubSubConnectionConfig_clear(UA_PubSubConnectionConfig *connectionConfig) {
    if (connectionConfig->publisherIdType == UA_PUBLISHERIDTYPE_STRING) {
        UA_String_clear(&connectionConfig->publisherId.string);
    }
    UA_String_clear(&connectionConfig->name);
    UA_String_clear(&connectionConfig->transportProfileUri);
    UA_Variant_clear(&connectionConfig->connectionTransportSettings);
    UA_Variant_clear(&connectionConfig->address);
    UA_KeyValueMap_clear(&connectionConfig->connectionProperties);
}

static void
assignConnectionIdentifier(UA_Server *server, UA_PubSubConnection *newConnectionsField,
                           UA_NodeId *connectionIdentifier) {
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* Internally createa a unique id */
    addPubSubConnectionRepresentation(server, newConnectionsField);
#else
    /* Create a unique NodeId that does not correspond to a Node */
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newConnectionsField->identifier);
#endif
    if(connectionIdentifier) {
        UA_NodeId_copy(&newConnectionsField->identifier, connectionIdentifier);
    }
}

UA_StatusCode
UA_PubSubConnection_create(UA_Server *server,
                          const UA_PubSubConnectionConfig *connectionConfig,
                          UA_NodeId *connectionIdentifier) {
    /* Validate preconditions */
    UA_CHECK_MEM(server, return UA_STATUSCODE_BADINTERNALERROR);
    UA_CHECK_MEM_ERROR(connectionConfig, return UA_STATUSCODE_BADINTERNALERROR,
                       &server->config.logger, UA_LOGCATEGORY_SERVER,
                       "PubSub Connection creation failed. No connection configuration supplied.");

    /* Retrieve the transport layer for the given profile uri */
    UA_PubSubTransportLayer *tl =
        UA_getTransportProtocolLayer(server, &connectionConfig->transportProfileUri);
    UA_CHECK_MEM_ERROR(tl, return UA_STATUSCODE_BADNOTFOUND,
                       &server->config.logger, UA_LOGCATEGORY_SERVER,
                       "PubSub Connection creation failed. Requested transport layer not found.");

    /* Create new connection from connection config */
    UA_PubSubConnection *newConnectionsField = (UA_PubSubConnection *)
        UA_calloc(1, sizeof(UA_PubSubConnection));
    if(!newConnectionsField) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of Memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    newConnectionsField->componentType = UA_PUBSUB_COMPONENT_CONNECTION;
    UA_StatusCode res = UA_PubSubConnectionConfig_copy(connectionConfig,
                                                       &newConnectionsField->config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(newConnectionsField);
        return res;
    }

    /* Register in the PubSubManager */
    UA_PubSubManager *pubSubManager = &server->pubSubManager;
    TAILQ_INSERT_TAIL(&pubSubManager->connections, newConnectionsField, listEntry);
    pubSubManager->connectionsSize++;

    /* Open the communication channel */
    UA_TransportLayerContext ctx;
    ctx.connection = newConnectionsField;
    ctx.connectionConfig = &newConnectionsField->config;
    ctx.decodeAndProcessNetworkMessage =
        (UA_StatusCode (*)(UA_Server *, void *, UA_ByteString *))
        UA_decodeAndProcessNetworkMessage;
    ctx.writerGroupAddress = NULL;
    ctx.server = server;

    newConnectionsField->channel = tl->createPubSubChannel(tl, &ctx);
    if(!newConnectionsField->channel) {
        UA_PubSubConnection_remove(server, newConnectionsField);
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Transport layer creation problem.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

#ifdef UA_ENABLE_PUBSUB_MQTT
    /* If the transport layer is MQTT, attach the server pointer to the callback function
     * that is called when a PUBLISH is received. */
    const UA_String transport_uri =
        UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt");
    if(UA_String_equal(&newConnectionsField->config.transportProfileUri, &transport_uri)) {
        UA_PubSubChannelDataMQTT *channelDataMQTT = (UA_PubSubChannelDataMQTT *)
            newConnectionsField->channel->handle;
        struct mqtt_client* client = (struct mqtt_client*)channelDataMQTT->mqttClient;
        client->publish_response_callback_state = server;
    }
#endif

    assignConnectionIdentifier(server, newConnectionsField, connectionIdentifier);

    /* Open the connection channel */
    if(newConnectionsField->channel->openSubscriber) {
        newConnectionsField->channel->openSubscriber(newConnectionsField->channel);
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addPubSubConnection(UA_Server *server,
                              const UA_PubSubConnectionConfig *connectionConfig,
                              UA_NodeId *connectionIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_PubSubConnection_create(server, connectionConfig, connectionIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_PubSubConnection_remove(UA_Server *server, UA_PubSubConnection *c) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Stop, unfreeze and delete all WriterGroups attached to the Connection */
    UA_WriterGroup *writerGroup, *tmpWriterGroup;
    LIST_FOREACH_SAFE(writerGroup, &c->writerGroups, listEntry, tmpWriterGroup) {
        UA_WriterGroup_setPubSubState(server, writerGroup, UA_PUBSUBSTATE_DISABLED,
                                      UA_STATUSCODE_BADSHUTDOWN);
        UA_WriterGroup_unfreezeConfiguration(server, writerGroup);
        UA_WriterGroup_remove(server, writerGroup);
    }

    /* Stop, unfreeze and delete all ReaderGroups attached to the Connection */
    UA_ReaderGroup *readerGroup, *tmpReaderGroup;
    LIST_FOREACH_SAFE(readerGroup, &c->readerGroups, listEntry, tmpReaderGroup) {
        UA_ReaderGroup_setPubSubState(server, readerGroup, UA_PUBSUBSTATE_DISABLED,
                                      UA_STATUSCODE_BADSHUTDOWN);
        UA_ReaderGroup_unfreezeConfiguration(server, readerGroup);
        UA_ReaderGroup_remove(server, readerGroup);
    }
    /* Close the related channel */
    if(c->channel) {
        if(c->channel->closeSubscriber)
            c->channel->closeSubscriber(c->channel);
        c->channel->close(c->channel);
    }

    /* Remove from the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, c->identifier, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&server->pubSubManager.connections, c, listEntry);
    server->pubSubManager.connectionsSize--;

    UA_NodeId_clear(&c->identifier);
    UA_PubSubConnectionConfig_clear(&c->config);
    UA_free(c);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    UA_LOCK(&server->serviceMutex);
    /* Find the connection */
    UA_PubSubConnection *c =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!c) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_StatusCode res = UA_PubSubConnection_remove(server, c);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_PubSubConnection_regist(UA_Server *server, UA_NodeId *connectionIdentifier,
                           const UA_ReaderGroupConfig *readerGroupConfig) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, *connectionIdentifier);
    if(!connection)
        return UA_STATUSCODE_BADNOTFOUND;

    if(connection->isRegistered) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Connection already registered");
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode retval = UA_STATUSCODE_BAD;
    if(readerGroupConfig != NULL) {
        UA_ExtensionObject transportSettings = readerGroupConfig->transportSettings;
        retval = connection->channel->regist(connection->channel, &transportSettings, NULL);
    } else {
        retval = connection->channel->regist(connection->channel, NULL, NULL);
    }

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "register channel failed: 0x%" PRIx32 "!", retval);
    }

    connection->isRegistered = true;
    return retval;
}

#endif /* UA_ENABLE_PUBSUB */
