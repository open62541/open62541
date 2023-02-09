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

void
UA_PubSubConnection_clear(UA_Server *server, UA_PubSubConnection *connection) {
    /* Remove WriterGroups */
    UA_WriterGroup *writerGroup, *tmpWriterGroup;
    LIST_FOREACH_SAFE(writerGroup, &connection->writerGroups,
                      listEntry, tmpWriterGroup) {
        removeWriterGroup(server, writerGroup->identifier);
    }

    /* Remove ReaderGroups */
    UA_ReaderGroup *readerGroups, *tmpReaderGroup;
    LIST_FOREACH_SAFE(readerGroups, &connection->readerGroups, listEntry, tmpReaderGroup)
        removeReaderGroup(server, readerGroups->identifier);

    UA_NodeId_clear(&connection->identifier);
    if(connection->channel)
        connection->channel->close(connection->channel);

    UA_PubSubConnectionConfig_clear(&connection->config);
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

static UA_StatusCode
channelErrorHandling(UA_Server *server, UA_PubSubConnection *newConnectionsField) {
    UA_PubSubConnection_clear(server, newConnectionsField);
    TAILQ_REMOVE(&server->pubSubManager.connections, newConnectionsField, listEntry);
    server->pubSubManager.connectionsSize--;
    UA_free(newConnectionsField);
    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "PubSub Connection creation failed. Transport layer creation problem.");
    return UA_STATUSCODE_BADINTERNALERROR;
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
    UA_CHECK_MEM(newConnectionsField->channel,
                 return channelErrorHandling(server, newConnectionsField));

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

    if (connectionConfig->enabled)
    {
        UA_Server_enablePubSubConnection(server, *connectionIdentifier);
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
removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    /* Find the connection */
    UA_PubSubConnection *c =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Stop, unfreeze and delete all WriterGroups attached to the Connection */
    UA_WriterGroup *writerGroup, *tmpWriterGroup;
    LIST_FOREACH_SAFE(writerGroup, &c->writerGroups, listEntry, tmpWriterGroup) {
        UA_WriterGroup_setPubSubState(server, writerGroup, UA_PUBSUBSTATE_DISABLED,
                                      UA_STATUSCODE_BADSHUTDOWN);
        UA_WriterGroup_unfreezeConfiguration(server, writerGroup);
        removeWriterGroup(server, writerGroup->identifier);
    }

    /* Stop, unfreeze and delete all ReaderGroups attached to the Connection */
    UA_ReaderGroup *readerGroup, *tmpReaderGroup;
    LIST_FOREACH_SAFE(readerGroup, &c->readerGroups, listEntry, tmpReaderGroup) {
        UA_ReaderGroup_setPubSubState(server, readerGroup, UA_PUBSUBSTATE_DISABLED,
                                      UA_STATUSCODE_BADSHUTDOWN);
        UA_ReaderGroup_unfreezeConfiguration(server, readerGroup);
        removeReaderGroup(server, readerGroup->identifier);
    }
    /* Close the related channel */
    if(c->channel->closeSubscriber) {
        c->channel->closeSubscriber(c->channel);
    }

    /* Remove from the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, c->identifier, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&server->pubSubManager.connections, c, listEntry);
    server->pubSubManager.connectionsSize--;

    /* Clean up the connection structure */
    UA_PubSubConnection_clear(server, c);
    UA_free(c);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = removePubSubConnection(server, connection);
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


/* Connection State */

static UA_StatusCode
setPubSubState_disable(UA_Server *server,
                                      UA_PubSubConnection *connection,
                                      UA_StatusCode cause) {
    UA_WriterGroup *wg;
    UA_ReaderGroup *rg;
    switch (connection->state){
        case UA_PUBSUBSTATE_DISABLED:
            break;
        case UA_PUBSUBSTATE_PAUSED:
            break;
        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            LIST_FOREACH(wg, &connection->writerGroups, listEntry){
                UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_DISABLED,
                                                cause);
            }
            LIST_FOREACH(rg, &connection->readerGroups, listEntry){
                UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_DISABLED,
                                                cause);
            }

            break;
        case UA_PUBSUBSTATE_ERROR:
            break;
        default:
            UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                        "Received unknown PubSub state!");
    }
    connection->state = UA_PUBSUBSTATE_DISABLED;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setPubSubState_paused(UA_Server *server,
                                     UA_PubSubConnection *connection,
                                     UA_StatusCode cause) {
    UA_LOG_DEBUG_CONNECTION(&server->config.logger, connection,
                             "PubSub state paused is unsupported at the moment!");
    (void)cause;
    switch (connection->state) {
        case UA_PUBSUBSTATE_DISABLED:
            break;
        case UA_PUBSUBSTATE_PAUSED:
            break;
        case UA_PUBSUBSTATE_PREOPERATIONAL:
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            break;
        case UA_PUBSUBSTATE_ERROR:
            break;
        default:
            UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                        "Received unknown PubSub state!");
    }
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

static UA_StatusCode
setPubSubState_preoperational(UA_Server *server,
                                            UA_PubSubConnection *connection,
                                            UA_StatusCode cause) {
    switch(connection->state) {
        case UA_PUBSUBSTATE_DISABLED:
        case UA_PUBSUBSTATE_PAUSED:
            connection->state = UA_PUBSUBSTATE_PREOPERATIONAL;
            return UA_STATUSCODE_GOOD;
        case UA_PUBSUBSTATE_PREOPERATIONAL:
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            return UA_STATUSCODE_GOOD;
        case UA_PUBSUBSTATE_ERROR:
            connection->state = UA_PUBSUBSTATE_PREOPERATIONAL;
            return UA_STATUSCODE_GOOD;
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Unknown PubSub state!");
            return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

static UA_StatusCode
setPubSubState_operational(UA_Server *server,
                                          UA_PubSubConnection *connection,
                                          UA_StatusCode cause) {
    UA_WriterGroup *wg;
    UA_ReaderGroup *rg;
    switch(connection->state) {
    case UA_PUBSUBSTATE_DISABLED:
        break;
    case UA_PUBSUBSTATE_PAUSED:
        break;
    case UA_PUBSUBSTATE_ERROR:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
        connection->state = UA_PUBSUBSTATE_OPERATIONAL;
        LIST_FOREACH(wg, &connection->writerGroups, listEntry){
            UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_PREOPERATIONAL,
                                            cause);
        }
        LIST_FOREACH(rg, &connection->readerGroups, listEntry){
            UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_PREOPERATIONAL,
                                            cause);
        }

        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_OPERATIONAL:
        return UA_STATUSCODE_GOOD;

    default:
        UA_LOG_WARNING_CONNECTION(&server->config.logger, connection, "Unknown PubSub state!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

static UA_StatusCode
setPubSubState_error(UA_Server *server,
                                    UA_PubSubConnection *connection,
                                    UA_StatusCode cause) {
    UA_WriterGroup *wg;
    UA_ReaderGroup *rg;
    switch (connection->state){
        case UA_PUBSUBSTATE_DISABLED:
            break;
        case UA_PUBSUBSTATE_PAUSED:
            break;
        case UA_PUBSUBSTATE_PREOPERATIONAL:
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            LIST_FOREACH(wg, &connection->writerGroups, listEntry){
                UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_ERROR,
                                                cause);
            }
            LIST_FOREACH(rg, &connection->readerGroups, listEntry){
                UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_ERROR,
                                                cause);
            }

            break;
        case UA_PUBSUBSTATE_ERROR:
            break;
        default:
            UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                            "Received unknown PubSub state!");
    }
    connection->state = UA_PUBSUBSTATE_ERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubConnection_setPubSubState(UA_Server *server, UA_PubSubConnection *connection,
                              UA_PubSubState state, UA_StatusCode cause) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = connection->state;

    switch(state) {
        case UA_PUBSUBSTATE_DISABLED:
            ret = setPubSubState_disable(server, connection, cause);
            break;
        case UA_PUBSUBSTATE_PAUSED:
            ret = setPubSubState_paused(server, connection, cause);
            break;
        case UA_PUBSUBSTATE_PREOPERATIONAL:
            ret = setPubSubState_preoperational(server, connection, cause);
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            ret = setPubSubState_operational(server, connection, cause);
            break;
        case UA_PUBSUBSTATE_ERROR: 
            ret = setPubSubState_error(server, connection, cause);
            break;
        default:
            UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                       "Received unknown PubSub state!");
    }
    if(state != oldState) {
        /* inform application about state change */
        UA_ServerConfig *pConfig = &server->config;
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            pConfig->pubSubConfig.
                stateChangeCallback(server, &connection->identifier, state, cause);
        }
    }
    return ret;
}

UA_StatusCode
UA_Server_enablePubSubConnection(UA_Server *server,
                                    const UA_NodeId connectionIdent) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_PubSubConnection *conn = UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    if(conn)
        res = UA_PubSubConnection_setPubSubState(server, conn, UA_PUBSUBSTATE_PREOPERATIONAL,
                                            UA_STATUSCODE_GOOD);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_setPubSubConnectionOperational(UA_Server *server,
                                    const UA_NodeId connectionIdent) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_PubSubConnection *conn = UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    if(conn)
        res = UA_PubSubConnection_setPubSubState(server, conn, UA_PUBSUBSTATE_OPERATIONAL,
                                            UA_STATUSCODE_GOOD);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_disablePubSubConnection(UA_Server *server,
                                 const UA_NodeId connectionIdent) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_PubSubConnection *conn = UA_PubSubConnection_findConnectionbyId(server, connectionIdent);
    if(conn)
        res = UA_PubSubConnection_setPubSubState(server, conn, UA_PUBSUBSTATE_DISABLED,
                                                UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
