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
        UA_DateTime nowM = UA_DateTime_nowMonotonic();
        if(connection->silenceErrorUntil < nowM) {
            UA_LOG_INFO_CONNECTION(server->config.logging, connection,
                                   "Dataset reader not found. Check PublisherId, "
                                   "WriterGroupId and DatasetWriterId. "
                                   "(This error is now silenced for 10s.)");
            connection->silenceErrorUntil = nowM + (UA_DateTime)(10.0 * UA_DATETIME_SEC);
        }
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
    if(src->publisherIdType == UA_PUBLISHERIDTYPE_STRING) {
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
    lockServer(server);
    UA_PubSubConnection *currentPubSubConnection =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    if(currentPubSubConnection)
        res = UA_PubSubConnectionConfig_copy(&currentPubSubConnection->config, config);
    unlockServer(server);
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
    if(connectionConfig->publisherIdType == UA_PUBLISHERIDTYPE_STRING)
        UA_String_clear(&connectionConfig->publisherId.string);
    UA_String_clear(&connectionConfig->name);
    UA_String_clear(&connectionConfig->transportProfileUri);
    UA_Variant_clear(&connectionConfig->connectionTransportSettings);
    UA_Variant_clear(&connectionConfig->address);
    UA_KeyValueMap_clear(&connectionConfig->connectionProperties);
}

UA_StatusCode
UA_PubSubConnection_create(UA_Server *server, const UA_PubSubConnectionConfig *cc,
                           UA_NodeId *cId) {
    /* Validate preconditions */
    UA_CHECK_MEM(server, return UA_STATUSCODE_BADINTERNALERROR);
    UA_CHECK_ERROR(cc != NULL, return UA_STATUSCODE_BADINTERNALERROR,
                   server->config.logging, UA_LOGCATEGORY_SERVER,
                   "PubSub Connection creation failed. Missing connection configuration.");

    /* Allocate */
    UA_PubSubConnection *c = (UA_PubSubConnection *)
        UA_calloc(1, sizeof(UA_PubSubConnection));
    if(!c) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of Memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
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

    /* Validate-connect to check the parameters */
    ret = UA_PubSubConnection_connect(server, c, true);
    if(ret != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Make the connection operational */
    ret = UA_PubSubConnection_setPubSubState(server, c, UA_PUBSUBSTATE_OPERATIONAL,
                                             UA_STATUSCODE_GOOD);
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
    lockServer(server);
    UA_StatusCode res = UA_PubSubConnection_create(server, cc, cId);
    unlockServer(server);
    return res;
}

static void
delayedPubSubConnection_delete(void *application, void *context) {
    UA_Server *server = (UA_Server*)application;
    UA_PubSubConnection *c = (UA_PubSubConnection*)context;
    lockServer(server);
    UA_PubSubConnection_delete(server, c);
    unlockServer(server);
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
    UA_PubSubConnection_setPubSubState(server, c, UA_PUBSUBSTATE_DISABLED, UA_STATUSCODE_GOOD);

    /* Stop and unfreeze all ReaderGroupds and WriterGroups attached to the
     * Connection. Do this before removing them because we need to unfreeze all
     * to remove the Connection.*/
    UA_ReaderGroup *readerGroup, *tmpReaderGroup;
    LIST_FOREACH(readerGroup, &c->readerGroups, listEntry) {
        UA_ReaderGroup_setPubSubState(server, readerGroup, UA_PUBSUBSTATE_DISABLED,
                                      UA_STATUSCODE_BADSHUTDOWN);
        UA_ReaderGroup_unfreezeConfiguration(server, readerGroup);
    }

    UA_WriterGroup *writerGroup, *tmpWriterGroup;
    LIST_FOREACH(writerGroup, &c->writerGroups, listEntry) {
        UA_WriterGroup_setPubSubState(server, writerGroup, UA_PUBSUBSTATE_DISABLED,
                                      UA_STATUSCODE_BADSHUTDOWN);
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
        if(el != server->config.eventLoop)
            unlockServer(server); /* Unlock to avoid deadlocks between different EventLoops */
        el->addDelayedCallback(el, &c->dc);
        if(el != server->config.eventLoop)
            lockServer(server);
        return;
    }

    /* Remove from the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, c->identifier, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&server->pubSubManager.connections, c, listEntry);
    server->pubSubManager.connectionsSize--;

    UA_PubSubConnectionConfig_clear(&c->config);
    UA_NodeId_clear(&c->identifier);
    UA_free(c);
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    lockServer(server);
    UA_PubSubConnection *psc =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!psc) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_PubSubConnection_delete(server, psc);
    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PubSubConnection_setPubSubState(UA_Server *server, UA_PubSubConnection *c,
                                   UA_PubSubState state, UA_StatusCode cause) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(c->deleteFlag && state != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_CONNECTION(server->config.logging, c,
                                  "The connection is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = c->state;
    UA_WriterGroup *writerGroup;
    UA_ReaderGroup *readerGroup;

    switch(state) {
        case UA_PUBSUBSTATE_ERROR:
        case UA_PUBSUBSTATE_PAUSED:
        case UA_PUBSUBSTATE_DISABLED:
            if(state == oldState)
                break;

            /* Close the EventLoop connection */
            c->state = state;
            UA_PubSubConnection_disconnect(c);

            /* Disable Reader and WriterGroups */
            LIST_FOREACH(readerGroup, &c->readerGroups, listEntry) {
                UA_ReaderGroup_setPubSubState(server, readerGroup, state,
                                              UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
            }
            LIST_FOREACH(writerGroup, &c->writerGroups, listEntry) {
                UA_WriterGroup_setPubSubState(server, writerGroup, state,
                                              UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
            }
            break;

        case UA_PUBSUBSTATE_PREOPERATIONAL:
        case UA_PUBSUBSTATE_OPERATIONAL:
            /* Called also if the connection is already operational. We might to
             * open an additional recv connection, etc. Sets the new state
             * internally. */
            if(oldState != UA_PUBSUBSTATE_OPERATIONAL)
                c->state = UA_PUBSUBSTATE_PREOPERATIONAL;
            ret = UA_PubSubConnection_connect(server, c, false);
            if(ret != UA_STATUSCODE_GOOD)
                UA_PubSubConnection_setPubSubState(server, c,
                                                   UA_PUBSUBSTATE_ERROR, ret);
            break;
        default:
            UA_LOG_WARNING_CONNECTION(server->config.logging, c,
                                      "Received unknown PubSub state!");
            return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Inform application about state change */
    if(c->state != oldState) {
        UA_ServerConfig *config = &server->config;
        if(config->pubSubConfig.stateChangeCallback)
            config->pubSubConfig.stateChangeCallback(server, &c->identifier, state, cause);
    }
    return ret;
}

UA_EventLoop *
UA_PubSubConnection_getEL(UA_Server *server, UA_PubSubConnection *c) {
    if(c->config.eventLoop)
        return c->config.eventLoop;
    return server->config.eventLoop;
}

#endif /* UA_ENABLE_PUBSUB */
