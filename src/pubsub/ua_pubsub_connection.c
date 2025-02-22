/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2022 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019, 2022, 2024 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Noel Graf)
 */

#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

static UA_Boolean
UA_PubSubConnection_canConnect(UA_PubSubConnection *c);

static UA_StatusCode
UA_PubSubConnection_connect(UA_PubSubManager *psm, UA_PubSubConnection *c,
                            UA_Boolean validate);

static void
UA_PubSubConnection_process(UA_PubSubManager *psm, UA_PubSubConnection *c,
                            const UA_ByteString msg);

static void
UA_PubSubConnection_disconnect(UA_PubSubConnection *c);

UA_StatusCode
UA_PubSubConnection_decodeNetworkMessage(UA_PubSubManager *psm,
                                         UA_PubSubConnection *connection,
                                         UA_ByteString buffer,
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
    ctx.opts.customTypes = psm->sc.server->config.customDataTypes;

    /* Decode the headers */
    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(&ctx, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, connection,
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
            UA_StatusCode res = UA_DataSetReader_checkIdentifier(psm, reader, nm);
            if(res != UA_STATUSCODE_GOOD)
                continue;
            processed = true;
            rv = verifyAndDecryptNetworkMessage(psm->logging, buffer, &ctx, nm, rg);
            if(rv != UA_STATUSCODE_GOOD) {
                UA_NetworkMessage_clear(nm);
                return rv;
            }

            /* break out of all loops when first verify & decrypt was successful */
            goto loops_exit;
        }
    }

loops_exit:
    if(!processed) {
        UA_DateTime nowM = UA_DateTime_nowMonotonic();
        if(connection->silenceErrorUntil < nowM) {
            UA_LOG_WARNING_PUBSUB(psm->logging, connection,
                                  "Could not decode the received NetworkMessage "
                                  "-- No matching ReaderGroup");
            connection->silenceErrorUntil = nowM + (UA_DateTime)(10.0 * UA_DATETIME_SEC);
        }
        UA_NetworkMessage_clear(nm);
        return UA_STATUSCODE_BADINTERNALERROR;
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

UA_PubSubConnection *
UA_PubSubConnection_find(UA_PubSubManager *psm,
                         const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        if(UA_NodeId_equal(&id, &c->head.identifier))
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
    addPubSubConnectionRepresentation(psm->sc.server, c);
#else
    /* Create a unique NodeId that does not correspond to a Node */
    UA_PubSubManager_generateUniqueNodeId(psm, &c->head.identifier);
#endif

    /* Register */
    TAILQ_INSERT_HEAD(&psm->connections, c, listEntry);
    psm->connectionsSize++;

    /* Validate-connect to check the parameters */
    ret = UA_PubSubConnection_connect(psm, c, true);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "Could not create the PubSubConnection. "
                     "The connection parameters did not validate.");
        UA_PubSubConnection_delete(psm, c);
        return ret;
    }

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "PubSubConnection %N\t| ", c->head.identifier);
    c->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    /* Notify the application that a new Connection was created.
     * This may internally adjust the config */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.componentLifecycleCallback) {
        UA_StatusCode res = server->config.pubSubConfig.
            componentLifecycleCallback(server, c->head.identifier,
                                       UA_PUBSUBCOMPONENT_CONNECTION, false);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PubSubConnection_delete(psm, c);
            return res;
        }
    }

    UA_LOG_INFO_PUBSUB(psm->logging, c, "Connection created (State: %s)",
                       UA_PubSubState_name(c->head.state));

    /* Copy the created NodeId to the output. Cannot fail as we create a
     * numerical NodeId. */
    if(cId)
        UA_NodeId_copy(&c->head.identifier, cId);

    return UA_STATUSCODE_GOOD;
}

static void
delayedPubSubConnection_delete(void *application, void *context) {
    UA_PubSubManager *psm = (UA_PubSubManager*)application;
    UA_Server *server = psm->sc.server;
    UA_PubSubConnection *c = (UA_PubSubConnection*)context;
    lockServer(server);
    UA_PubSubConnection_delete(psm, c);
    unlockServer(server);
}

/* Clean up the PubSubConnection. If no EventLoop connection is attached we can
 * immediately free. Otherwise we close the EventLoop connections and free in
 * the connection callback. */
UA_StatusCode
UA_PubSubConnection_delete(UA_PubSubManager *psm, UA_PubSubConnection *c) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    /* Check with the application if we can remove */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.componentLifecycleCallback) {
        UA_StatusCode res = server->config.pubSubConfig.
            componentLifecycleCallback(server, c->head.identifier,
                                       UA_PUBSUBCOMPONENT_CONNECTION, true);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    /* Disable (and disconnect) and set the deleteFlag. This prevents a
     * reconnect and triggers the deletion when the last open socket is
     * closed. */
    c->deleteFlag = true;
    UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_DISABLED);

    /* Stop and all ReaderGroupds and WriterGroups attached to the Connection.
     * We need to disable all to remove the Connection.*/
    UA_ReaderGroup *rg, *tmpRg;
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED);
    }

    UA_WriterGroup *wg, *tmpWg;
    LIST_FOREACH(wg, &c->writerGroups, listEntry) {
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED);
    }

    /* Remove all ReaderGorups and WriterGroups */
    LIST_FOREACH_SAFE(rg, &c->readerGroups, listEntry, tmpRg) {
        UA_ReaderGroup_remove(psm, rg);
    }

    LIST_FOREACH_SAFE(wg, &c->writerGroups, listEntry, tmpWg) {
        UA_WriterGroup_remove(psm, wg);
    }

    /* Not all sockets are closed. This method will be called again */
    if(c->sendChannel != 0 || c->recvChannelsSize > 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* The WriterGroups / ReaderGroups are not deleted. Try again in the next
     * iteration of the event loop.*/
    if(!LIST_EMPTY(&c->writerGroups) || !LIST_EMPTY(&c->readerGroups)) {
        UA_EventLoop *el = psm->sc.server->config.eventLoop;
        c->dc.callback = delayedPubSubConnection_delete;
        c->dc.application = psm;
        c->dc.context = c;
        el->addDelayedCallback(el, &c->dc);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Remove from the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(psm->sc.server, c->head.identifier, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&psm->connections, c, listEntry);
    psm->connectionsSize--;

    UA_LOG_INFO_PUBSUB(psm->logging, c, "Connection deleted");

    UA_PubSubConnectionConfig_clear(&c->config);
    UA_PubSubComponentHead_clear(&c->head);
    UA_free(c);

    return UA_STATUSCODE_GOOD;
}

static void
UA_PubSubConnection_process(UA_PubSubManager *psm, UA_PubSubConnection *c,
                            const UA_ByteString msg) {
    UA_LOG_TRACE_PUBSUB(psm->logging, c, "Processing a received buffer");

    /* Process RT ReaderGroups */
    UA_Boolean processed = false;
    UA_ReaderGroup *rg = LIST_FIRST(&c->readerGroups);
    /* Any interested ReaderGroups? */
    if(!rg)
        goto finish;

    /* Decode the received message for the non-RT ReaderGroups */
    UA_StatusCode res;
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    if(rg->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP) {
        res = UA_PubSubConnection_decodeNetworkMessage(psm, c, msg, &nm);
    } else { /* if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) */
#ifdef UA_ENABLE_JSON_ENCODING
        res = UA_NetworkMessage_decodeJson(&msg, &nm, NULL);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_PUBSUB(psm->logging, c,
                                  "Decoding the JSON network message failed");
        }
#else
        res = UA_STATUSCODE_BADNOTSUPPORTED;
        UA_LOG_WARNING_PUBSUB(psm->logging, c,
                              "JSON support is not activated");
#endif
    }

    if(res != UA_STATUSCODE_GOOD)
        return;

    /* Process the received message for the non-RT ReaderGroups */
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        if(rg->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
           rg->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;
        processed |= UA_ReaderGroup_process(psm, rg, &nm);
    }
    UA_NetworkMessage_clear(&nm);

 finish:
    if(!processed) {
        UA_DateTime nowM = UA_DateTime_nowMonotonic();
        if(c->silenceErrorUntil < nowM) {
            UA_LOG_WARNING_PUBSUB(psm->logging, c,
                                  "Message received that could not be processed. "
                                  "Check PublisherID, WriterGroupID and DatasetWriterID.");
            c->silenceErrorUntil = nowM + (UA_DateTime)(10.0 * UA_DATETIME_SEC);
        }
    }
}

UA_StatusCode
UA_PubSubConnection_setPubSubState(UA_PubSubManager *psm, UA_PubSubConnection *c,
                                   UA_PubSubState targetState) {
    if(c->deleteFlag && targetState != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_PUBSUB(psm->logging, c,
                              "The connection is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Callback to modify the WriterGroup config and change the targetState
     * before the state machine executes */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.beforeStateChangeCallback) {
        server->config.pubSubConfig.
            beforeStateChangeCallback(server, c->head.identifier, &targetState);
    }

    /* Are we doing a top-level state update or recursively? */
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = c->head.state;
    UA_Boolean isTransient = c->head.transientState;
    c->head.transientState = true;

    /* Custom state machine */
    if(c->config.customStateMachine) {
        ret = c->config.customStateMachine(server, c->head.identifier, c->config.context,
                                           &c->head.state, targetState);
        goto finalize_state_machine;
    }

    /* Internal state machine */
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
                    UA_LOG_WARNING_PUBSUB(psm->logging, c,
                                          "Cannot enable the connection while the "
                                          "server is not running -> Paused State");
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

 finalize_state_machine:

    /* Only the top-level state update (if recursive calls are happening)
     * notifies the application and updates Reader and WriterGroups */
    c->head.transientState = isTransient;
    if(c->head.transientState)
        return ret;

    /* Inform application about state change */
    if(c->head.state != oldState) {
        UA_LOG_INFO_PUBSUB(psm->logging, c, "%s -> %s",
                           UA_PubSubState_name(oldState),
                           UA_PubSubState_name(c->head.state));
        if(server->config.pubSubConfig.stateChangeCallback) {
            server->config.pubSubConfig.
                stateChangeCallback(server, c->head.identifier, targetState, ret);
        }
    }

    /* Update Reader and WriterGroups state. This will set them to PAUSED (if
     * they were operational) as the Connection is now non-operational. */
    UA_ReaderGroup *readerGroup;
    LIST_FOREACH(readerGroup, &c->readerGroups, listEntry) {
        UA_ReaderGroup_setPubSubState(psm, readerGroup, readerGroup->head.state);
    }
    UA_WriterGroup *writerGroup;
    LIST_FOREACH(writerGroup, &c->writerGroups, listEntry) {
        UA_WriterGroup_setPubSubState(psm, writerGroup, writerGroup->head.state);
    }

    /* Update the PubSubManager state. It will go from STOPPING to STOPPED when
     * the last socket has closed. */
    UA_PubSubManager_setState(psm, psm->sc.state);

    return ret;
}

static UA_StatusCode
enablePubSubConnection(UA_PubSubManager *psm, const UA_NodeId connectionId) {
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionId);
    return (c) ? UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_OPERATIONAL)
        : UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
disablePubSubConnection(UA_PubSubManager *psm, const UA_NodeId connectionId) {
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionId);
    return (c) ? UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_DISABLED)
        : UA_STATUSCODE_BADNOTFOUND;
}

/***********************/
/* Connection Handling */
/***********************/

static UA_StatusCode
UA_PubSubConnection_connectUDP(UA_PubSubManager *psm, UA_PubSubConnection *c,
                               UA_Boolean validate);

static UA_StatusCode
UA_PubSubConnection_connectETH(UA_PubSubManager *psm, UA_PubSubConnection *c,
                               UA_Boolean validate);

typedef struct  {
    UA_String profileURI;
    UA_String protocol;
    UA_Boolean json;
    UA_StatusCode (*connect)(UA_PubSubManager *psm, UA_PubSubConnection *c,
                             UA_Boolean validate);
} ConnectionProfileMapping;

static ConnectionProfileMapping connectionProfiles[UA_PUBSUB_PROFILES_SIZE] = {
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"),
     UA_STRING_STATIC("udp"), false, UA_PubSubConnection_connectUDP},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp"),
     UA_STRING_STATIC("mqtt"), false, NULL},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json"),
     UA_STRING_STATIC("mqtt"), true, NULL},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"),
     UA_STRING_STATIC("eth"), false, UA_PubSubConnection_connectETH}
};

static void
UA_PubSubConnection_detachConnection(UA_PubSubManager *psm,
                                     UA_ConnectionManager *cm,
                                     UA_PubSubConnection *c,
                                     uintptr_t connectionId) {
    if(c->sendChannel == connectionId) {
        UA_LOG_INFO_PUBSUB(psm->logging, c, "Detach send-connection %S %u",
                           cm->protocol, (unsigned)connectionId);
        c->sendChannel = 0;
        return;
    }
    for(size_t i = 0; i < UA_PUBSUB_MAXCHANNELS; i++) {
        if(c->recvChannels[i] != connectionId)
            continue;
        UA_LOG_INFO_PUBSUB(psm->logging, c, "Detach receive-connection %S %u",
                           cm->protocol, (unsigned)connectionId);
        c->recvChannels[i] = 0;
        c->recvChannelsSize--;
        return;
    }
}

static UA_StatusCode
UA_PubSubConnection_attachSendConnection(UA_PubSubManager *psm,
                                         UA_ConnectionManager *cm,
                                         UA_PubSubConnection *c,
                                         uintptr_t connectionId) {
    if(c->sendChannel != 0 && c->sendChannel != connectionId)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_LOG_INFO_PUBSUB(psm->logging, c, "Attach send-connection %S %u",
                       cm->protocol, (unsigned)connectionId);
    c->sendChannel = connectionId;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubConnection_attachRecvConnection(UA_PubSubManager *psm,
                                         UA_ConnectionManager *cm,
                                         UA_PubSubConnection *c,
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
        UA_LOG_INFO_PUBSUB(psm->logging, c, "Attach receive-connection %S %u",
                           cm->protocol, (unsigned)connectionId);
        c->recvChannels[i] = connectionId;
        c->recvChannelsSize++;
        break;
    }
    return UA_STATUSCODE_GOOD;
}

static void
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
    UA_PubSubConnection *psc = (UA_PubSubConnection*)*connectionContext;
    UA_PubSubManager *psm = (UA_PubSubManager*)application;
    UA_Server *server = psm->sc.server;

    UA_LOG_TRACE_PUBSUB(psm->logging, psc,
                        "Connection Callback with state %i", state);

    lockServer(server);

    /* The connection is closing in the EventLoop. This is the last callback
     * from that connection. Clean up the SecureChannel in the client. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        /* Reset the connection identifiers */
        UA_PubSubConnection_detachConnection(psm, cm, psc, connectionId);

        /* PSC marked for deletion and the last EventLoop connection has closed */
        if(psc->deleteFlag && psc->recvChannelsSize == 0 && psc->sendChannel == 0) {
            UA_PubSubConnection_delete(psm, psc);
            unlockServer(server);
            return;
        }

        /* Reconnect automatically if the connection was operational. This sets
         * the connection state if connecting fails. Attention! If there are
         * several send or recv channels, then the connection is only reopened if
         * all of them close - which is usually the case. */
        if(psc->head.state == UA_PUBSUBSTATE_OPERATIONAL)
            UA_PubSubConnection_connect(psm, psc, false);

        /* Switch the psm state from stopping to stopped once the last
         * connection has closed */
        UA_PubSubManager_setState(psm, psm->sc.state);

        unlockServer(server);
        return;
    }

    /* Store the connectionId (if a new connection) */
    UA_StatusCode res = (recv) ?
        UA_PubSubConnection_attachRecvConnection(psm, cm, psc, connectionId) :
        UA_PubSubConnection_attachSendConnection(psm, cm, psc, connectionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, psc,
                              "No more space for an additional EventLoop connection");
        if(psc->cm)
            psc->cm->closeConnection(psc->cm, connectionId);
        unlockServer(server);
        return;
    }

    /* Connection open, set to operational if not already done */
    UA_PubSubConnection_setPubSubState(psm, psc, psc->head.state);

    /* Message received */
    if(UA_LIKELY(recv && msg.length > 0))
        UA_PubSubConnection_process(psm, psc, msg);
    
    unlockServer(server);
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
UA_PubSubConnection_connectUDP(UA_PubSubManager *psm, UA_PubSubConnection *c,
                               UA_Boolean validate) {
    UA_Server *server = psm->sc.server;
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType*)
        c->config.address.data;

    /* Extract hostname and port */
    UA_String address;
    UA_UInt16 port;
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c, "Could not parse the UDP network URL");
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
    if(validate || (c->recvChannelsSize == 0 && c->readerGroupsSize > 0)) {
        res = c->cm->openConnection(c->cm, &kvm, psm, c, PubSubRecvChannelCallback);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_PUBSUB(psm->logging, c,
                                "Could not open an UDP channel for receiving");
            return res;
        }
    }

    /* Receive all -- sending is handled in the DataSetWriter */
    if(receive_all) {
        UA_LOG_INFO_PUBSUB(psm->logging, c,
                           "Localhost address - don't open UDP send connection");
        return UA_STATUSCODE_GOOD;
    }

    /* Open a send connection */
    if(validate || (c->sendChannel == 0 && c->writerGroupsSize > 0)) {
        listen = false;
        res = c->cm->openConnection(c->cm, &kvm, psm, c, PubSubSendChannelCallback);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_PUBSUB(psm->logging, c, "Could not open an UDP recv channel");
            return res;
        }
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_PubSubConnection_connectETH(UA_PubSubManager *psm, UA_PubSubConnection *c,
                               UA_Boolean validate) {
    UA_Server *server = psm->sc.server;
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType*)
        c->config.address.data;

    /* Extract hostname and port */
    UA_String address;
    UA_String vidPCP = UA_STRING_NULL;
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, NULL, &vidPCP);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c, "Could not parse the ETH network URL");
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
    if(validate || (c->recvChannelsSize == 0 && c->readerGroupsSize > 0)) {
        res = c->cm->openConnection(c->cm, &kvm, psm, c, PubSubRecvChannelCallback);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_PUBSUB(psm->logging, c, "Could not open an ETH recv channel");
            return res;
        }
    }

    /* Open send channels */
    if(validate || (c->sendChannel == 0 && c->writerGroupsSize > 0)) {
        listen = false;
        res = c->cm->openConnection(c->cm, &kvm, psm, c, PubSubSendChannelCallback);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_PUBSUB(psm->logging, c,
                                "Could not open an ETH channel for sending");
        }
    }

    return res;
}

static UA_Boolean
UA_PubSubConnection_canConnect(UA_PubSubConnection *c) {
    if(c->sendChannel == 0 && c->writerGroupsSize > 0)
        return true;
    if(c->recvChannelsSize == 0 && c->readerGroupsSize > 0)
        return true;
    return false;
}

static UA_StatusCode
UA_PubSubConnection_connect(UA_PubSubManager *psm, UA_PubSubConnection *c,
                            UA_Boolean validate) {
    UA_Server *server = psm->sc.server;
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    if(!el) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c, "No EventLoop configured");
        return UA_STATUSCODE_BADINTERNALERROR;;
    }

    /* Look up the connection manager for the connection */
    ConnectionProfileMapping *profile = NULL;
    for(size_t i = 0; i < UA_PUBSUB_PROFILES_SIZE; i++) {
        if(!UA_String_equal(&c->config.transportProfileUri,
                            &connectionProfiles[i].profileURI))
            continue;
        profile = &connectionProfiles[i];
        break;
    }

    UA_ConnectionManager *cm = (profile) ? getCM(el, profile->protocol) : NULL;
    if(!cm || (c->cm && cm != c->cm)) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c,
                            "The requested profile \"%S\"is not supported",
                            c->config.transportProfileUri);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check the configuration address type */
    if(!UA_Variant_hasScalarType(&c->config.address,
                                 &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE])) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c, "No NetworkAddressUrlDataType "
                            "for the address configuration");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Update the connection settings from the profile information */
    c->cm = cm;
    c->json = profile->json;

    /* Some protocols (such as MQTT) don't connect at this level */
    return (profile->connect) ?
        profile->connect(psm, c, validate) : UA_STATUSCODE_GOOD;
}

/**************/
/* Server API */
/**************/

UA_StatusCode
UA_Server_getPubSubConnectionConfig(UA_Server *server, const UA_NodeId connection,
                                    UA_PubSubConnectionConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubConnection *c = UA_PubSubConnection_find(getPSM(server), connection);
    UA_StatusCode res = (c) ?
        UA_PubSubConnectionConfig_copy(&c->config, config) : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_addPubSubConnection(UA_Server *server,
                              const UA_PubSubConnectionConfig *cc,
                              UA_NodeId *cId) {
    if(!server || !cc)
        return UA_STATUSCODE_BADINTERNALERROR;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        UA_PubSubConnection_create(psm, cc, cId) : UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId cId) {
    if(!server)
        return UA_STATUSCODE_BADINTERNALERROR;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, cId);
    if(!c) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_DISABLED);
    UA_PubSubConnection_delete(psm, c);
    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_enablePubSubConnection(UA_Server *server, const UA_NodeId cId) {
    if(!server)
        return UA_STATUSCODE_BADINTERNALERROR;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        enablePubSubConnection(psm, cId) : UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_disablePubSubConnection(UA_Server *server, const UA_NodeId cId) {
    if(!server)
        return UA_STATUSCODE_BADINTERNALERROR;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        disablePubSubConnection(psm, cId) : UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_processPubSubConnectionReceive(UA_Server *server,
                                         const UA_NodeId connectionId,
                                         const UA_ByteString packet) {
    if(!server)
        return UA_STATUSCODE_BADINTERNALERROR;
    lockServer(server);
    UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
    UA_PubSubManager *psm = getPSM(server);
    if(psm) {
        UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionId);
        if(c) {
            res = UA_STATUSCODE_GOOD;
            UA_PubSubConnection_process(psm, c, packet);
        } else {
            res = UA_STATUSCODE_BADCONNECTIONCLOSED;
            UA_LOG_WARNING_PUBSUB(psm->logging, c,
                                  "Cannot process a packet if the "
                                  "PubSubConnection is not operational");
        }
    }
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_updatePubSubConnectionConfig(UA_Server *server,
                                       const UA_NodeId connectionId,
                                       const UA_PubSubConnectionConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    /* Find the connection */
    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionId);
    if(!c) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Verify the connection is disabled */
    if(UA_PubSubState_isEnabled(c->head.state)) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c,
                            "The PubSubConnection must be disabled to update the config");
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Store the old config */
    UA_PubSubConnectionConfig oldConfig = c->config;
    memset(&c->config, 0, sizeof(UA_PubSubConnectionConfig));

    /* Copy the connection config */
    UA_StatusCode res = UA_PubSubConnectionConfig_copy(config, &c->config);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Validate-connect to check the parameters */
    res = UA_PubSubConnection_connect(psm, c, true);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c, "The connection parameters did not validate");
        goto errout;
    }

    UA_PubSubConnectionConfig_clear(&oldConfig);
    unlockServer(server);
    return UA_STATUSCODE_GOOD;

 errout:
    /* Restore the old config */
    UA_PubSubConnectionConfig_clear(&c->config);
    c->config = oldConfig;
    unlockServer(server);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
