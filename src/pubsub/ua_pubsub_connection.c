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
UA_PubSubConnection_decodeNetworkMessage(UA_PubSubConnection *connection,
                                         UA_Server *server, UA_ByteString buffer,
                                         UA_NetworkMessage *nm) {
#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(buffer->data, buffer->length);
#endif

    /* Set up the decoding context */
    Ctx ctx;
    ctx.pos = buffer.data;
    ctx.end = buffer.data + buffer.length;
    ctx.depth = 0;
    memset(&ctx.opts, 0, sizeof(UA_DecodeBinaryOptions));
    ctx.opts.customTypes = server->config.customDataTypes;

    /* Decode the headers */
    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(&ctx, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(server->config.logging, connection,
                              "PubSub receive. decoding headers failed");
        UA_NetworkMessage_clear(nm);
        return rv;
    }

    /* Choose a correct readergroup for decrypt/verify this message
     * (there could be multiple) */
    UA_Boolean processed = false;
    UA_ReaderGroup *rg;
    LIST_FOREACH(rg, &connection->readerGroups, listEntry) {
        UA_DataSetReader *reader;
        LIST_FOREACH(reader, &rg->readers, listEntry) {
            UA_StatusCode retval =
                UA_DataSetReader_checkIdentifier(server, nm, reader, rg->config);
            if(retval != UA_STATUSCODE_GOOD)
                continue;
            processed = true;
            rv = verifyAndDecryptNetworkMessage(server->config.logging, buffer, &ctx, nm, rg);
            if(rv != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING_PUBSUB(server->config.logging, connection,
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
        UA_LOG_INFO_PUBSUB(server->config.logging, connection,
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

    rv = UA_NetworkMessage_decodePayload(&ctx, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_clear(nm);
        return rv;
    }

    rv = UA_NetworkMessage_decodeFooters(&ctx, nm);
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
    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;;
    }
    UA_PubSubConnection *c = UA_PubSubConnection_findConnectionbyId(psm, connection);
    UA_StatusCode res = (c) ?
        UA_PubSubConnectionConfig_copy(&c->config, config) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_PubSubManager *psm,
                                       UA_NodeId connectionId) {
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry){
        if(UA_NodeId_equal(&connectionId, &c->head.identifier))
            break;
    }
    return c;
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
UA_PubSubConnection_create(UA_PubSubManager *psm, const UA_PubSubConnectionConfig *cc,
                           UA_NodeId *cId) {
    UA_Server *server = psm->sc.server;

    /* Allocate */
    UA_PubSubConnection *c = (UA_PubSubConnection*)
        UA_calloc(1, sizeof(UA_PubSubConnection));
    if(!c)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    c->head.componentType = UA_PUBSUBCOMPONENT_CONNECTION;

    /* Copy the connection config */
    UA_StatusCode ret = UA_PubSubConnectionConfig_copy(cc, &c->config);
    UA_CHECK_STATUS(ret, UA_free(c); return ret);

    /* Assign the connection identifier */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* Internally create a unique id */
    addPubSubConnectionRepresentation(server, c);
#else
    /* Create a unique NodeId that does not correspond to a Node */
    UA_PubSubManager_generateUniqueNodeId(psm, &c->head.identifier);
#endif

    /* Register */
    TAILQ_INSERT_HEAD(&psm->connections, c, listEntry);
    psm->connectionsSize++;

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "PubSubConnection %N\t| ", c->head.identifier);
    c->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    UA_LOG_INFO_PUBSUB(server->config.logging, c, "Connection created");

    /* Validate-connect to check the parameters */
    ret = UA_PubSubConnection_connect(psm, c, true);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, c,
                            "Could not validate connection parameters");
        goto cleanup;
    }

    /* Make the connection operational */
    ret = UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_OPERATIONAL);
    if(ret != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Copy the created NodeId to the output. Cannot fail as we create a
     * numerical NodeId. */
    if(cId)
        UA_NodeId_copy(&c->head.identifier, cId);
    return ret;

 cleanup:
    UA_PubSubConnection_delete(psm, c);
    return ret;
}

UA_StatusCode
UA_Server_addPubSubConnection(UA_Server *server, const UA_PubSubConnectionConfig *cc,
                              UA_NodeId *cId) {
    if(!cc)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        UA_PubSubConnection_create(psm, cc, cId) : UA_STATUSCODE_BADINTERNALERROR;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static void
delayedPubSubConnection_delete(void *application, void *context) {
    UA_PubSubManager *psm = (UA_PubSubManager*)application;
    UA_Server *server = psm->sc.server;
    UA_PubSubConnection *c = (UA_PubSubConnection*)context;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubConnection_delete(psm, c);
    UA_UNLOCK(&server->serviceMutex);
}

/* Clean up the PubSubConnection. If no EventLoop connection is attached we can
 * immediately free. Otherwise we close the EventLoop connections and free in
 * the connection callback. */
void
UA_PubSubConnection_delete(UA_PubSubManager *psm, UA_PubSubConnection *c) {
    UA_Server *server = psm->sc.server;
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Disable (and disconnect) and set the deleteFlag. This prevents a
     * reconnect and triggers the deletion when the last open socket is
     * closed. */
    c->deleteFlag = true;
    UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_DISABLED);

    /* Stop and unfreeze all ReaderGroupds and WriterGroups attached to the
     * Connection. Do this before removing them because we need to unfreeze all
     * to remove the Connection.*/
    UA_ReaderGroup *rg, *tmpRg;
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_DISABLED);
        UA_ReaderGroup_unfreezeConfiguration(server, rg);
    }

    UA_WriterGroup *wg, *tmpWg;
    LIST_FOREACH(wg, &c->writerGroups, listEntry) {
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED);
        UA_WriterGroup_unfreezeConfiguration(psm, wg);
    }

    /* Remove all ReaderGorups and WriterGroups */
    LIST_FOREACH_SAFE(rg, &c->readerGroups, listEntry, tmpRg) {
        UA_ReaderGroup_remove(server, rg);
    }

    LIST_FOREACH_SAFE(wg, &c->writerGroups, listEntry, tmpWg) {
        UA_WriterGroup_remove(server, wg);
    }

    /* Not all sockets are closed. This method will be called again */
    if(c->sendChannel != 0 || c->recvChannelsSize > 0)
        return;

    /* The WriterGroups / ReaderGroups are not deleted. Try again in the next
     * iteration of the event loop.*/
    if(!LIST_EMPTY(&c->writerGroups) || !LIST_EMPTY(&c->readerGroups)) {
        UA_EventLoop *el = UA_PubSubConnection_getEL(server, c);
        c->dc.callback = delayedPubSubConnection_delete;
        c->dc.application = psm;
        c->dc.context = c;
        el->addDelayedCallback(el, &c->dc);
        return;
    }

    /* Remove from the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, c->head.identifier, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&psm->connections, c, listEntry);
    psm->connectionsSize--;

    UA_LOG_INFO_PUBSUB(server->config.logging, c, "Connection deleted");

    UA_PubSubConnectionConfig_clear(&c->config);
    UA_PubSubComponentHead_clear(&c->head);
    UA_free(c);
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_PubSubConnection *c = UA_PubSubConnection_findConnectionbyId(psm, connection);
    if(!c) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_DISABLED);
    UA_PubSubConnection_delete(psm, c);
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}

void
UA_PubSubConnection_process(UA_PubSubManager *psm, UA_PubSubConnection *c,
                            UA_ByteString msg) {
    UA_Server *server = psm->sc.server;

    /* Process RT ReaderGroups */
    UA_ReaderGroup *rg;
    UA_Boolean processed = false;
    UA_ReaderGroup *nonRtRg = NULL;
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        if(rg->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
           rg->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;
        if(rg->config.rtLevel != UA_PUBSUB_RT_FIXED_SIZE) {
            nonRtRg = rg;
            continue;
        } 
        processed |= UA_ReaderGroup_decodeAndProcessRT(server, rg, msg);
    }

    /* Any non-RT ReaderGroups? */
    if(!nonRtRg)
        goto finish;

    /* Decode the received message for the non-RT ReaderGroups */
    UA_StatusCode res;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    if(nonRtRg->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP) {
        res = UA_PubSubConnection_decodeNetworkMessage(c, server, msg, &nm);
    } else { /* if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) */
#ifdef UA_ENABLE_JSON_ENCODING
        res = UA_NetworkMessage_decodeJson(&msg, &nm, NULL);
#else
        res = UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(server->config.logging, c,
                              "Verify, decrypt and decode network message failed");
        return;
    }

    /* Process the received message for the non-RT ReaderGroups */
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        if(rg->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
           rg->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;
        if(rg->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE)
            continue;
        processed |= UA_ReaderGroup_process(server, rg, &nm);
    }
    UA_NetworkMessage_clear(&nm);

 finish:
    if(!processed) {
        UA_LOG_WARNING_PUBSUB(server->config.logging, c,
                              "Message received that could not be processed. "
                              "Check PublisherID, WriterGroupID and DatasetWriterID.");
    }
}

UA_StatusCode
UA_PubSubConnection_setPubSubState(UA_PubSubManager *psm, UA_PubSubConnection *c,
                                   UA_PubSubState targetState) {
    UA_Server *server = psm->sc.server;
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(c->deleteFlag && targetState != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_PUBSUB(server->config.logging, c,
                              "The connection is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Are we doing a top-level state update or recursively? */
    UA_Boolean isTransient = c->head.transientState;
    c->head.transientState = true;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = c->head.state;

    switch(targetState) {
        /* Disabled or Error */
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_DISABLED:
            UA_PubSubConnection_disconnect(c);
            c->head.state = targetState;
            break;

        case UA_PUBSUBSTATE_PAUSED:
        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            /* Cannot go operational if the PubSubManager is not started */
            if(psm->sc.state != UA_LIFECYCLESTATE_STARTED) {
                /* Avoid repeat warnings */
                if(oldState != UA_PUBSUBSTATE_PAUSED) {
                    UA_LOG_WARNING_PUBSUB(server->config.logging, c,
                                          "Cannot enable the connection "
                                          "while the server is not running");
                }
                c->head.state = UA_PUBSUBSTATE_PAUSED;
                UA_PubSubConnection_disconnect(c);
                break;
            }

            c->head.state = UA_PUBSUBSTATE_OPERATIONAL;

            /* Whether the connection needs to connect depends on whether a
             * ReaderGroup or WriterGroup is attached. If not, then we don't
             * open any connections. */
            if(UA_PubSubConnection_canConnect(c))
                ret = UA_PubSubConnection_connect(psm, c, false);
            break;

            /* Unknown case */
        default:
            ret = UA_STATUSCODE_BADINTERNALERROR;
            break;
    }

    /* Failure */
    if(ret != UA_STATUSCODE_GOOD) {
        c->head.state = UA_PUBSUBSTATE_ERROR;
        UA_PubSubConnection_disconnect(c);
    }

    /* Only the top-level state update (if recursive calls are happening)
     * notifies the application and updates Reader and WriterGroups */
    c->head.transientState = isTransient;
    if(c->head.transientState)
        return ret;

    /* Inform application about state change */
    if(c->head.state != oldState) {
        UA_ServerConfig *config = &server->config;
        UA_LOG_INFO_PUBSUB(config->logging, c, "State change: %s -> %s",
                           UA_PubSubState_name(oldState),
                           UA_PubSubState_name(c->head.state));
        if(config->pubSubConfig.stateChangeCallback) {
            UA_UNLOCK(&server->serviceMutex);
            config->pubSubConfig.
                stateChangeCallback(server, c->head.identifier, targetState, ret);
            UA_LOCK(&server->serviceMutex);
        }
    }

    /* Update Reader and WriterGroups. This will set them to PAUSED (if
     * they were operational) as the Connection is now
     * non-operational. */
    UA_ReaderGroup *readerGroup;
    LIST_FOREACH(readerGroup, &c->readerGroups, listEntry) {
        UA_ReaderGroup_setPubSubState(server, readerGroup, readerGroup->head.state);
    }
    UA_WriterGroup *writerGroup;
    LIST_FOREACH(writerGroup, &c->writerGroups, listEntry) {
        UA_WriterGroup_setPubSubState(psm, writerGroup, writerGroup->head.state);
    }
    return ret;
}

static UA_StatusCode
enablePubSubConnection(UA_PubSubManager *psm, const UA_NodeId connectionId) {
    UA_PubSubConnection *c = UA_PubSubConnection_findConnectionbyId(psm, connectionId);
    return (c) ? UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_OPERATIONAL)
        : UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
disablePubSubConnection(UA_PubSubManager *psm, const UA_NodeId connectionId) {
    UA_PubSubConnection *c = UA_PubSubConnection_findConnectionbyId(psm, connectionId);
    return (c) ? UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_DISABLED)
        : UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_enablePubSubConnection(UA_Server *server,
                                 const UA_NodeId connectionId) {
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        enablePubSubConnection(psm, connectionId) : UA_STATUSCODE_BADINTERNALERROR;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_disablePubSubConnection(UA_Server *server,
                                  const UA_NodeId connectionId) {
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        disablePubSubConnection(psm, connectionId) : UA_STATUSCODE_BADINTERNALERROR;
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
