/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2022 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019, 2022 Fraunhofer IOSB (Author: Julius Pfrommer)
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

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

UA_StatusCode
decodeNetworkMessage(UA_Server *server, UA_ByteString *buffer, size_t *pos,
                     UA_NetworkMessage *nm, UA_PubSubConnection *connection) {
#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(buffer->data, buffer->length);
#endif

    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(buffer, pos, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CONNECTION(server->config.logging, connection,
                                  "PubSub receive. decoding headers failed");
        UA_NetworkMessage_clear(nm);
        return rv;
    }

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_Boolean processed = false;
    UA_ReaderGroup *readerGroup;
    UA_DataSetReader *reader;

    /* Choose a correct readergroup for decrypt/verify this message
     * (there could be multiple) */
    LIST_FOREACH(readerGroup, &connection->readerGroups, listEntry) {
        LIST_FOREACH(reader, &readerGroup->readers, listEntry) {
            UA_StatusCode retval =
                UA_DataSetReader_checkIdentifier(server, nm, reader, readerGroup->config);
            if(retval != UA_STATUSCODE_GOOD)
                continue;
            processed = true;
            rv = verifyAndDecryptNetworkMessage(server->config.logging, buffer, pos,
                                                nm, readerGroup);
            if(rv != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING_CONNECTION(server->config.logging, connection,
                                          "Subscribe failed, verify and decrypt "
                                          "network message failed.");
                UA_NetworkMessage_clear(nm);
                return rv;
            }

            /* break out of all loops when first verify & decrypt was successful */
            goto loops_exit;
        }
    }

loops_exit:
    if(!processed) {
        UA_LOG_INFO_CONNECTION(server->config.logging, connection,
                               "Dataset reader not found. Check PublisherId, "
                               "WriterGroupId and DatasetWriterId");
        /* Possible multicast scenario: there are multiple connections (with one
         * or more ReaderGroups) within a multicast group every connection
         * receives all network messages, even if some of them are not meant for
         * the connection currently processed -> therefore it is ok if the
         * connection does not have a DataSetReader for every received network
         * message. We must not return an error here, but continue with the
         * buffer decoding and see if we have a matching DataSetReader for the
         * next network message. */
    }
#endif

    rv = UA_NetworkMessage_decodePayload(buffer, pos, nm, server->config.customDataTypes, NULL);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_clear(nm);
        return rv;
    }

    rv = UA_NetworkMessage_decodeFooters(buffer, pos, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_clear(nm);
        return rv;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src,
                               UA_PubSubConnectionConfig *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PubSubConnectionConfig));
    res |= UA_PublisherId_copy(&src->publisherId, &dst->publisherId);
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
    UA_PublisherId_clear(&connectionConfig->publisherId);
    UA_String_clear(&connectionConfig->name);
    UA_String_clear(&connectionConfig->transportProfileUri);
    UA_Variant_clear(&connectionConfig->connectionTransportSettings);
    UA_Variant_clear(&connectionConfig->address);
    UA_KeyValueMap_clear(&connectionConfig->connectionProperties);
}

UA_StatusCode
UA_PubSubConnection_create(UA_Server *server, const UA_PubSubConnectionConfig *cc,
                           UA_NodeId *cId) {
    if(!server || !cc)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Allocate */
    UA_PubSubConnection *c = (UA_PubSubConnection*)
        UA_calloc(1, sizeof(UA_PubSubConnection));
    if(!c)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    c->componentType = UA_PUBSUB_COMPONENT_CONNECTION;

    /* Copy the connection config */
    UA_StatusCode ret = UA_PubSubConnectionConfig_copy(cc, &c->config);
    UA_CHECK_STATUS(ret, UA_free(c); return ret);

    /* Assign the connection identifier */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* Internally create a unique id */
    addPubSubConnectionRepresentation(server, c);
#else
    /* Create a unique NodeId that does not correspond to a Node */
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &c->identifier);
#endif

    /* Register */
    UA_PubSubManager *pubSubManager = &server->pubSubManager;
    TAILQ_INSERT_HEAD(&pubSubManager->connections, c, listEntry);
    pubSubManager->connectionsSize++;

    /* Cache the log string */
    UA_String idStr = UA_STRING_NULL;
    UA_NodeId_print(&c->identifier, &idStr);
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "PubSubConnection %.*s\t| ",
                (int)idStr.length, idStr.data);
    c->logIdString = UA_STRING_ALLOC(tmpLogIdStr);
    UA_String_clear(&idStr);

    UA_LOG_INFO_CONNECTION(server->config.logging, c, "Connection created");

    /* Validate-connect to check the parameters */
    ret = UA_PubSubConnection_connect(server, c, true);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_CONNECTION(server->config.logging, c,
                                "Could not validate connection parameters");
        UA_PubSubConnection_delete(server, c);
        return ret;
    }

    /* Make the connection operational */
    ret = UA_PubSubConnection_setPubSubState(server, c, UA_PUBSUBSTATE_OPERATIONAL);
    if(ret != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Copy the created NodeId to the output. Cannot fail as we create a
     * numerical NodeId. */
    if(cId)
        UA_NodeId_copy(&c->identifier, cId);

 cleanup:
    if(ret != UA_STATUSCODE_GOOD)
        UA_PubSubConnection_delete(server, c);
    return ret;
}

UA_StatusCode
UA_Server_addPubSubConnection(UA_Server *server, const UA_PubSubConnectionConfig *cc,
                              UA_NodeId *cId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_PubSubConnection_create(server, cc, cId);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static void
delayedPubSubConnection_delete(void *application, void *context) {
    UA_Server *server = (UA_Server*)application;
    UA_PubSubConnection *c = (UA_PubSubConnection*)context;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubConnection_delete(server, c);
    UA_UNLOCK(&server->serviceMutex);
}

/* Clean up the PubSubConnection. If no EventLoop connection is attached we can
 * immediately free. Otherwise we close the EventLoop connections and free in
 * the connection callback. */
void
UA_PubSubConnection_delete(UA_Server *server, UA_PubSubConnection *c) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Disable (and disconnect) and set the deleteFlag. This prevents a
     * reconnect and triggers the deletion when the last open socket is
     * closed. */
    c->deleteFlag = true;
    UA_PubSubConnection_setPubSubState(server, c, UA_PUBSUBSTATE_DISABLED);

    /* Stop and unfreeze all ReaderGroupds and WriterGroups attached to the
     * Connection. Do this before removing them because we need to unfreeze all
     * to remove the Connection.*/
    UA_ReaderGroup *readerGroup, *tmpReaderGroup;
    LIST_FOREACH(readerGroup, &c->readerGroups, listEntry) {
        UA_ReaderGroup_setPubSubState(server, readerGroup, UA_PUBSUBSTATE_DISABLED);
        UA_ReaderGroup_unfreezeConfiguration(server, readerGroup);
    }

    UA_WriterGroup *writerGroup, *tmpWriterGroup;
    LIST_FOREACH(writerGroup, &c->writerGroups, listEntry) {
        UA_WriterGroup_setPubSubState(server, writerGroup, UA_PUBSUBSTATE_DISABLED);
        UA_WriterGroup_unfreezeConfiguration(server, writerGroup);
    }

    /* Remove all ReaderGorups and WriterGroups */
    LIST_FOREACH_SAFE(readerGroup, &c->readerGroups, listEntry, tmpReaderGroup) {
        UA_ReaderGroup_remove(server, readerGroup);
    }

    LIST_FOREACH_SAFE(writerGroup, &c->writerGroups, listEntry, tmpWriterGroup) {
        UA_WriterGroup_remove(server, writerGroup);
    }

    /* Not all sockets are closed. This method will be called again */
    if(c->sendChannel != 0 || c->recvChannelsSize > 0)
        return;

    /* The WriterGroups / ReaderGroups are not deleted. Try again in the next
     * iteration of the event loop.*/
    if(!LIST_EMPTY(&c->writerGroups) || !LIST_EMPTY(&c->readerGroups)) {
        UA_EventLoop *el = UA_PubSubConnection_getEL(server, c);
        c->dc.callback = delayedPubSubConnection_delete;
        c->dc.application = server;
        c->dc.context = c;
        el->addDelayedCallback(el, &c->dc);
        return;
    }

    /* Remove from the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, c->identifier, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&server->pubSubManager.connections, c, listEntry);
    server->pubSubManager.connectionsSize--;

    UA_LOG_INFO_CONNECTION(server->config.logging, c, "Connection deleted");

    UA_PubSubConnectionConfig_clear(&c->config);
    UA_NodeId_clear(&c->identifier);
    UA_String_clear(&c->logIdString);
    UA_free(c);
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    UA_LOCK(&server->serviceMutex);
    UA_PubSubConnection *psc =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!psc) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_PubSubConnection_setPubSubState(server, psc, UA_PUBSUBSTATE_DISABLED);
    UA_PubSubConnection_delete(server, psc);
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_PubSubConnection_process(UA_Server *server, UA_PubSubConnection *c,
                            UA_ByteString msg) {
    /* Process RT ReaderGroups */
    UA_ReaderGroup *rg;
    UA_Boolean processed = false;
    UA_ReaderGroup *nonRtRg = NULL;
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        if(rg->state != UA_PUBSUBSTATE_OPERATIONAL &&
           rg->state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;
        if(rg->config.rtLevel != UA_PUBSUB_RT_FIXED_SIZE) {
            nonRtRg = rg;
            continue;
        } 
        processed |= UA_ReaderGroup_decodeAndProcessRT(server, rg, &msg);
    }

    /* Any non-RT ReaderGroups? */
    if(!nonRtRg)
        goto finish;

    /* Decode the received message for the non-RT ReaderGroups */
    UA_StatusCode res;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    if(nonRtRg->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP) {
        size_t currentPosition = 0;
        res = decodeNetworkMessage(server, &msg, &currentPosition, &nm, c);
    } else { /* if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) */
#ifdef UA_ENABLE_JSON_ENCODING
        res = UA_NetworkMessage_decodeJson(&msg, &nm, NULL);
#else
        res = UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CONNECTION(server->config.logging, c,
                                  "Verify, decrypt and decode network message failed");
        return;
    }

    /* Process the received message for the non-RT ReaderGroups */
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        if(rg->state != UA_PUBSUBSTATE_OPERATIONAL &&
           rg->state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;
        if(rg->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE)
            continue;
        processed |= UA_ReaderGroup_process(server, rg, &nm);
    }
    UA_NetworkMessage_clear(&nm);

 finish:
    if(!processed) {
        UA_LOG_WARNING_CONNECTION(server->config.logging, c,
                                  "Message received that could not be processed. "
                                  "Check PublisherID, WriterGroupID and DatasetWriterID.");
    }
}

UA_StatusCode
UA_PubSubConnection_setPubSubState(UA_Server *server, UA_PubSubConnection *c,
                                   UA_PubSubState targetState) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(c->deleteFlag && targetState != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_CONNECTION(server->config.logging, c,
                                  "The connection is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = c->state;

 set_state:

    switch(targetState) {
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_PAUSED:
        case UA_PUBSUBSTATE_DISABLED:
            /* Close the EventLoop connection */
            UA_PubSubConnection_disconnect(c);
            c->state = targetState;
            break;

        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            /* Called also if the connection is already operational. We might to
             * open an additional recv connection, etc. Sets the new state
             * internally. */
            if(oldState == UA_PUBSUBSTATE_PREOPERATIONAL || oldState == UA_PUBSUBSTATE_OPERATIONAL)
                c->state = UA_PUBSUBSTATE_OPERATIONAL;
            else
                c->state = UA_PUBSUBSTATE_PREOPERATIONAL;

            /* This is the only place where UA_PubSubConnection_connect is
             * called (other than to validate the parameters). So we handle the
             * fallout of a failed connection here. */
            ret = UA_PubSubConnection_connect(server, c, false);
            if(ret != UA_STATUSCODE_GOOD) {
                targetState = UA_PUBSUBSTATE_ERROR;
                goto set_state;
            }
            break;
        default:
            UA_LOG_WARNING_CONNECTION(server->config.logging, c,
                                      "Received unknown PubSub state!");
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Inform application about state change */
    if(c->state != oldState) {
        UA_ServerConfig *config = &server->config;
        UA_LOG_INFO_CONNECTION(config->logging, c, "State change: %s -> %s",
                               UA_PubSubState_name(oldState),
                               UA_PubSubState_name(c->state));
        UA_UNLOCK(&server->serviceMutex);
        if(config->pubSubConfig.stateChangeCallback)
            config->pubSubConfig.stateChangeCallback(server, &c->identifier, targetState, ret);
        UA_LOCK(&server->serviceMutex);
    }

    /* Update Reader and WriterGroups. This will set them to PAUSED (if
     * they were operational) as the Connection is now
     * non-operational. */
    UA_ReaderGroup *readerGroup;
    LIST_FOREACH(readerGroup, &c->readerGroups, listEntry) {
        UA_ReaderGroup_setPubSubState(server, readerGroup, readerGroup->state);
    }
    UA_WriterGroup *writerGroup;
    LIST_FOREACH(writerGroup, &c->writerGroups, listEntry) {
        UA_WriterGroup_setPubSubState(server, writerGroup, writerGroup->state);
    }
    return ret;
}

static UA_StatusCode
enablePubSubConnection(UA_Server *server, const UA_NodeId connectionId) {
    UA_PubSubConnection *psc = UA_PubSubConnection_findConnectionbyId(server, connectionId);
    return (psc) ? UA_PubSubConnection_setPubSubState(server, psc, UA_PUBSUBSTATE_OPERATIONAL)
        : UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
disablePubSubConnection(UA_Server *server, const UA_NodeId connectionId) {
    UA_PubSubConnection *psc = UA_PubSubConnection_findConnectionbyId(server, connectionId);
    return (psc) ? UA_PubSubConnection_setPubSubState(server, psc, UA_PUBSUBSTATE_DISABLED)
        : UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_enablePubSubConnection(UA_Server *server,
                                 const UA_NodeId connectionId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = enablePubSubConnection(server, connectionId);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_disablePubSubConnection(UA_Server *server,
                                  const UA_NodeId connectionId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = disablePubSubConnection(server, connectionId);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_EventLoop *
UA_PubSubConnection_getEL(UA_Server *server, UA_PubSubConnection *c) {
    if(c->config.eventLoop)
        return c->config.eventLoop;
    return server->config.eventLoop;
}

#endif /* UA_ENABLE_PUBSUB */
