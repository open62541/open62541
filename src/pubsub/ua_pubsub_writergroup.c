/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2025 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_SKS
#include "ua_pubsub_keystorage.h"
#endif

static UA_StatusCode
encryptAndSign(UA_WriterGroup *wg, const UA_NetworkMessage *nm,
               UA_Byte *signStart, UA_Byte *encryptStart,
               UA_Byte *msgEnd);

static UA_StatusCode
generateNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                       UA_ExtensionObject *messageSettings,
                       UA_ExtensionObject *transportSettings,
                       UA_NetworkMessage *networkMessage);

static void
UA_WriterGroup_disconnect(UA_WriterGroup *wg);

static UA_StatusCode
UA_WriterGroup_connect(UA_PubSubManager *psm, UA_WriterGroup *wg,
                       UA_Boolean validate);

static UA_Boolean
UA_WriterGroup_canConnect(UA_WriterGroup *wg) {
    /* Already connected */
    if(wg->sendChannel != 0)
        return false;

    /* Is this a WriterGroup with custom TransportSettings beyond the
     * PubSubConnection? */
    if(wg->config.transportSettings.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return false;

    return true;
}

UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    /* Already registered */
    if(wg->publishCallbackId != 0)
        return UA_STATUSCODE_GOOD;

    /* Use EventLoop for cyclic callbacks */
    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    return el->addTimer(el, (UA_Callback)UA_WriterGroup_publishCallback,
                        psm, wg, wg->config.publishingInterval,
                        NULL /* TODO: use basetime */,
                        UA_TIMERPOLICY_CURRENTTIME,
                        &wg->publishCallbackId);
}

void
UA_WriterGroup_removePublishCallback(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    if(wg->publishCallbackId == 0)
        return;
    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    if(UA_LIKELY(el != NULL))
        el->removeTimer(el, wg->publishCallbackId);
    wg->publishCallbackId = 0;
}

#ifdef UA_ENABLE_PUBSUB_SKS
static UA_StatusCode
writerGroupAttachSKSKeystorage(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    /* No SecurityGroup defined */
    if(UA_String_isEmpty(&wg->config.securityGroupId) || !wg->config.securityPolicy)
        return UA_STATUSCODE_GOOD;

    /* KeyStorage already connected */
    if(wg->keyStorage)
        return UA_STATUSCODE_GOOD;

    /* Does the key storage already exist? */
    wg->keyStorage = UA_PubSubKeyStorage_find(psm, wg->config.securityGroupId);
    if(wg->keyStorage) {
        wg->keyStorage->referenceCount++; /* Increase the ref count */
        return UA_STATUSCODE_GOOD;
    }

    /* Create a new key storage */
    wg->keyStorage = (UA_PubSubKeyStorage *)UA_calloc(1, sizeof(UA_PubSubKeyStorage));
    if(!wg->keyStorage)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Initialize the KeyStorage */
    UA_StatusCode res =
        UA_PubSubKeyStorage_init(psm, wg->keyStorage, &wg->config.securityGroupId,
                                 wg->config.securityPolicy, 0, 0);
    if(res != UA_STATUSCODE_GOOD) {
        UA_PubSubKeyStorage_delete(psm, wg->keyStorage);
        wg->keyStorage = NULL;
        return res;
    }

    wg->keyStorage->referenceCount++; /* Increase the ref count */
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
validateWriterGroupConfig(UA_PubSubManager *psm, UA_PubSubComponentHead *logHead,
                          const UA_WriterGroupConfig *config) {
    const UA_ExtensionObject *ms = &config->messageSettings;
    if(ms->encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return UA_STATUSCODE_GOOD;

    if(config->encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
        if(!UA_ExtensionObject_hasDecodedType(ms,
                &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE])) {
            UA_LOG_WARNING_PUBSUB(psm->logging, (UA_PubSubConnection*)logHead,
                                  "WriTerGroupConfig MessageSettings need to be "
                                  "of type JSONWriterGroupMessageDataType");
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    } else if(config->encodingMimeType == UA_PUBSUB_ENCODING_UADP) {
        if(!UA_ExtensionObject_hasDecodedType(ms,
                &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])) {
            UA_LOG_WARNING_PUBSUB(psm->logging, (UA_PubSubConnection*)logHead,
                                  "WriTerGroupConfig MessageSettings need to be "
                                  "of type UADPWriterGroupMessageDataType");
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    } else {
        UA_LOG_WARNING_PUBSUB(psm->logging, (UA_PubSubConnection*)logHead,
                              "Wrong encoding MIME-type");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_WriterGroup_create(UA_PubSubManager *psm, const UA_NodeId connection,
                      const UA_WriterGroupConfig *config,
                      UA_NodeId *writerGroupIdentifier) {
    /* Delete the reserved IDs if the related session no longer exists. */
    UA_PubSubManager_freeIds(psm);
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connection);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Validate messageSettings type */
    UA_StatusCode res = validateWriterGroupConfig(psm, &c->head, config);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Allocate new WriterGroup */
    UA_WriterGroup *wg = (UA_WriterGroup*)UA_calloc(1, sizeof(UA_WriterGroup));
    if(!wg)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    wg->head.componentType = UA_PUBSUBCOMPONENT_WRITERGROUP;
    wg->linkedConnection = c;

    /* Deep copy of the config */
    res = UA_WriterGroupConfig_copy(config, &wg->config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(wg);
        return res;
    }

    /* Attach to the connection */
    LIST_INSERT_HEAD(&c->writerGroups, wg, listEntry);
    c->writerGroupsSize++;

    /* Add representation / create unique identifier */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    res = addWriterGroupRepresentation(psm->sc.server, wg);
    if(res != UA_STATUSCODE_GOOD) {
        UA_WriterGroup_remove(psm, wg);
        return res;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(psm, &wg->head.identifier);
#endif

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "%SWriterGroup %N\t| ",
                c->head.logIdString, wg->head.identifier);
    wg->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    /* Validate the connection settings */
    res = UA_WriterGroup_connect(psm, wg, true);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                            "Could not validate the connection parameters");
        UA_WriterGroup_remove(psm, wg);
        return res;
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    if(config->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       config->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        res = writerGroupAttachSKSKeystorage(psm, wg);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_PUBSUB(psm->logging, wg, "Attaching the SKS KeyStorage failed");
            UA_WriterGroup_remove(psm, wg);
            return res;
        }
    }
#endif

    /* Notify the application that a new WriterGroup was created.
     * This may internally adjust the config */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.componentLifecycleCallback) {
        res = server->config.pubSubConfig.
            componentLifecycleCallback(server, wg->head.identifier,
                                       UA_PUBSUBCOMPONENT_WRITERGROUP, false);
        if(res != UA_STATUSCODE_GOOD) {
            UA_WriterGroup_remove(psm, wg);
            return res;
        }
    }

    UA_LOG_INFO_PUBSUB(psm->logging, wg, "WriterGroup created (State: %s)",
                       UA_PubSubState_name(wg->head.state));

    /* Trigger the connection state machine. It might open a socket only when
     * the first WriterGroup is attached. */
    UA_PubSubConnection_setPubSubState(psm, c, c->head.state);

    /* Copying a numeric NodeId always succeeds */
    if(writerGroupIdentifier)
        UA_NodeId_copy(&wg->head.identifier, writerGroupIdentifier);

    /* Enable the WriterGroup immediately if the enabled flag is set */
    if(config->enabled)
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_OPERATIONAL);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_WriterGroup_remove(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    /* Check with the application if we can remove */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.componentLifecycleCallback) {
        UA_StatusCode res = server->config.pubSubConfig.
            componentLifecycleCallback(server, wg->head.identifier,
                                       UA_PUBSUBCOMPONENT_WRITERGROUP, true);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    UA_PubSubConnection *connection = wg->linkedConnection;
    UA_assert(connection);

    /* Disable (and disconnect) and set the deleteFlag. This prevents a
     * reconnect and triggers the deletion when the last open socket is
     * closed. */
    wg->deleteFlag = true;
    UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED);

    UA_DataSetWriter *dsw, *dsw_tmp;
    LIST_FOREACH_SAFE(dsw, &wg->writers, listEntry, dsw_tmp) {
        UA_DataSetWriter_remove(psm, dsw);
    }

    if(wg->config.securityPolicy && wg->securityPolicyContext) {
        wg->config.securityPolicy->deleteContext(wg->securityPolicyContext);
        wg->securityPolicyContext = NULL;
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    if(wg->keyStorage) {
        UA_PubSubKeyStorage_detachKeyStorage(psm, wg->keyStorage);
        wg->keyStorage = NULL;
    }
#endif

    if(wg->sendChannel == 0) {
        /* Unlink from the connection */
        LIST_REMOVE(wg, listEntry);
        connection->writerGroupsSize--;
        wg->linkedConnection = NULL;

        /* Actually remove the WriterGroup */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
        deleteNode(psm->sc.server, wg->head.identifier, true);
#endif

        UA_LOG_INFO_PUBSUB(psm->logging, wg, "WriterGroup deleted");

        UA_WriterGroupConfig_clear(&wg->config);
        UA_PubSubComponentHead_clear(&wg->head);
        UA_free(wg);
    }

    /* Update the connection state */
    UA_PubSubConnection_setPubSubState(psm, connection, connection->head.state);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_WriterGroupConfig_copy(const UA_WriterGroupConfig *src,
                          UA_WriterGroupConfig *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_WriterGroupConfig));
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_ExtensionObject_copy(&src->transportSettings, &dst->transportSettings);
    res |= UA_ExtensionObject_copy(&src->messageSettings, &dst->messageSettings);
    res |= UA_KeyValueMap_copy(&src->groupProperties, &dst->groupProperties);
    res |= UA_String_copy(&src->securityGroupId, &dst->securityGroupId);
    if(res != UA_STATUSCODE_GOOD)
        UA_WriterGroupConfig_clear(dst);
    return res;
}

UA_WriterGroup *
UA_WriterGroup_find(UA_PubSubManager *psm, const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            if(UA_NodeId_equal(&id, &wg->head.identifier))
                return wg;
        }
    }
    return NULL;
}

UA_StatusCode
UA_WriterGroup_setEncryptionKeys(UA_PubSubManager *psm, UA_WriterGroup *wg,
                                 UA_UInt32 securityTokenId,
                                 const UA_ByteString signingKey,
                                 const UA_ByteString encryptingKey,
                                 const UA_ByteString keyNonce) {
    if(wg->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "JSON encoding is enabled. The message security "
                              "iis only defined for the UADP message mapping.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(!wg->config.securityPolicy) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "No SecurityPolicy configured for the WriterGroup");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(securityTokenId != wg->securityTokenId) {
        wg->securityTokenId = securityTokenId;
        wg->nonceSequenceNumber = 1;
    }

    UA_StatusCode res = UA_STATUSCODE_BAD;
    if(!wg->securityPolicyContext) {
        /* Create a new context */
        res = wg->config.securityPolicy->
            newContext(wg->config.securityPolicy->policyContext,
                       &signingKey, &encryptingKey, &keyNonce,
                       &wg->securityPolicyContext);
    } else {
        /* Update the context */
        res = wg->config.securityPolicy->
            setSecurityKeys(wg->securityPolicyContext, &signingKey,
                            &encryptingKey, &keyNonce);
    }

    return (res == UA_STATUSCODE_GOOD) ?
        UA_WriterGroup_setPubSubState(psm, wg, wg->head.state) : res;
}

void
UA_WriterGroupConfig_clear(UA_WriterGroupConfig *writerGroupConfig) {
    UA_String_clear(&writerGroupConfig->name);
    UA_ExtensionObject_clear(&writerGroupConfig->transportSettings);
    UA_ExtensionObject_clear(&writerGroupConfig->messageSettings);
    UA_KeyValueMap_clear(&writerGroupConfig->groupProperties);
    UA_String_clear(&writerGroupConfig->securityGroupId);
    memset(writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
}

UA_StatusCode
UA_WriterGroup_setPubSubState(UA_PubSubManager *psm, UA_WriterGroup *wg,
                              UA_PubSubState targetState) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    if(wg->deleteFlag && targetState != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "The WriterGroup is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Callback to modify the WriterGroup config and change the targetState
     * before the state machine executes */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.beforeStateChangeCallback) {
        server->config.pubSubConfig.
            beforeStateChangeCallback(server, wg->head.identifier, &targetState);
    }

    /* Are we doing a top-level state update or recursively? */
    UA_Boolean isTransient = wg->head.transientState;
    wg->head.transientState = true;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = wg->head.state;
    UA_PubSubConnection *connection = wg->linkedConnection;

    /* Custom state machine */
    if(wg->config.customStateMachine) {
        ret = wg->config.customStateMachine(server, wg->head.identifier, wg->config.context,
                                            &wg->head.state, targetState);
        goto finalize_state_machine;
    }

    /* Internal state machine */
    switch(targetState) {
        /* Disabled or Error */
    case UA_PUBSUBSTATE_DISABLED:
    case UA_PUBSUBSTATE_ERROR:
        wg->head.state = targetState;
        UA_WriterGroup_disconnect(wg);
        UA_WriterGroup_removePublishCallback(psm, wg);
        break;

        /* Enabled */
    case UA_PUBSUBSTATE_PAUSED:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
    case UA_PUBSUBSTATE_OPERATIONAL:
        /* PAUSED has no open connections and periodic callbacks */
        if(psm->sc.state != UA_LIFECYCLESTATE_STARTED) {
            /* Avoid repeat warnings */
            if(oldState != UA_PUBSUBSTATE_PAUSED) {
                UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                                      "Cannot enable the WriterGroup while the "
                                      "server is not running -> Paused State");
            }
            wg->head.state = UA_PUBSUBSTATE_PAUSED;
            UA_WriterGroup_disconnect(wg);
            UA_WriterGroup_removePublishCallback(psm, wg);
            break;
        }

        if(connection->head.state != UA_PUBSUBSTATE_OPERATIONAL) {
            wg->head.state = UA_PUBSUBSTATE_PAUSED;
            UA_WriterGroup_disconnect(wg);
            UA_WriterGroup_removePublishCallback(psm, wg);
            break;
        }

        wg->head.state = UA_PUBSUBSTATE_OPERATIONAL;

        /* Not fully connected -> connect */
        if(UA_WriterGroup_canConnect(wg)) {
            ret = UA_WriterGroup_connect(psm, wg, false);
            if(ret != UA_STATUSCODE_GOOD)
                break;
        }

        /* Security Mode not set-> PreOperational */
        if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE &&
           wg->securityTokenId == 0)
            wg->head.state = UA_PUBSUBSTATE_PREOPERATIONAL;

        /* Enable publish callback if operational */
        if(wg->head.state == UA_PUBSUBSTATE_OPERATIONAL)
            ret = UA_WriterGroup_addPublishCallback(psm, wg);
        break;

        /* Unknown case */
    default:
        ret = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }

    /* Failure */
    if(ret != UA_STATUSCODE_GOOD) {
        wg->head.state = UA_PUBSUBSTATE_ERROR;;
        UA_WriterGroup_disconnect(wg);
        UA_WriterGroup_removePublishCallback(psm, wg);
    }

 finalize_state_machine:

    /* Only the top-level state update (if recursive calls are happening)
     * notifies the application and updates Reader and WriterGroups */
    wg->head.transientState = isTransient;
    if(wg->head.transientState)
        return ret;

    /* No state change has happened */
    if(wg->head.state == oldState)
        return ret;

    UA_LOG_INFO_PUBSUB(psm->logging, wg, "%s -> %s",
                       UA_PubSubState_name(oldState),
                       UA_PubSubState_name(wg->head.state));

    /* Inform the application about state change */
    if(server->config.pubSubConfig.stateChangeCallback)
        server->config.pubSubConfig.
            stateChangeCallback(server, wg->head.identifier, wg->head.state, ret);

    /* Children evaluate their state machine after the state change of the parent.
     * Keep the current child state as the target state for the child. */
    UA_DataSetWriter *writer;
    LIST_FOREACH(writer, &wg->writers, listEntry) {
        UA_DataSetWriter_setPubSubState(psm, writer, writer->head.state);
    }

    /* Update the PubSubManager state. It will go from STOPPING to STOPPED when
     * the last socket has closed. */
    UA_PubSubManager_setState(psm, psm->sc.state);

    return ret;
}

static UA_StatusCode
encryptAndSign(UA_WriterGroup *wg, const UA_NetworkMessage *nm,
               UA_Byte *signStart, UA_Byte *encryptStart,
               UA_Byte *msgEnd) {
    UA_StatusCode rv;
    void *channelContext = wg->securityPolicyContext;

    if(nm->securityHeader.networkMessageEncrypted) {
        /* Set the temporary MessageNonce in the SecurityPolicy */
        const UA_ByteString nonce = {
            (size_t)nm->securityHeader.messageNonceSize,
            (UA_Byte*)(uintptr_t)nm->securityHeader.messageNonce
        };
        rv = wg->config.securityPolicy->setMessageNonce(channelContext, &nonce);
        UA_CHECK_STATUS(rv, return rv);

        /* The encryption is done in-place, no need to encode again */
        UA_ByteString toBeEncrypted =
            {(uintptr_t)msgEnd - (uintptr_t)encryptStart, encryptStart};
        rv = wg->config.securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.
            encrypt(channelContext, &toBeEncrypted);
        UA_CHECK_STATUS(rv, return rv);
    }

    if(nm->securityHeader.networkMessageSigned) {
        UA_ByteString toBeSigned = {(uintptr_t)msgEnd - (uintptr_t)signStart,
                                    signStart};

        size_t sigSize = wg->config.securityPolicy->symmetricModule.cryptoModule.
                     signatureAlgorithm.getLocalSignatureSize(channelContext);
        UA_ByteString signature = {sigSize, msgEnd};

        rv = wg->config.securityPolicy->symmetricModule.cryptoModule.
            signatureAlgorithm.sign(channelContext, &toBeSigned, &signature);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
encodeNetworkMessage(UA_WriterGroup *wg, PubSubEncodeCtx *ctx,
                     UA_NetworkMessage *nm, UA_ByteString *buf) {
    UA_Byte *networkMessageStart = buf->data;
    UA_StatusCode rv = UA_NetworkMessage_encodeHeaders(ctx, nm);
    UA_CHECK_STATUS(rv, return rv);

    UA_Byte *payloadStart = ctx->ctx.pos;
    rv = UA_NetworkMessage_encodePayload(ctx, nm);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_encodeFooters(ctx, nm);
    UA_CHECK_STATUS(rv, return rv);

    /* Encrypt and Sign the message */
    UA_Byte *footerEnd = ctx->ctx.pos;
    return encryptAndSign(wg, nm, networkMessageStart, payloadStart, footerEnd);
}

static void
sendNetworkMessageBuffer(UA_PubSubManager *psm, UA_WriterGroup *wg, 
                         UA_PubSubConnection *connection, uintptr_t connectionId,
                         UA_ByteString *buffer) {
    UA_StatusCode res = connection->cm->
        sendWithConnection(connection->cm, connectionId,
                           &UA_KEYVALUEMAP_NULL, buffer);

    /* Failure, set the WriterGroup into an error mode */
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                            "Sending NetworkMessage failed");
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_ERROR);
        UA_PubSubConnection_setPubSubState(psm, connection, UA_PUBSUBSTATE_ERROR);
        return;
    }

    /* Sending successful - increase the sequence number */
    wg->sequenceNumber++;
}

#ifdef UA_ENABLE_JSON_ENCODING
static UA_StatusCode
sendNetworkMessageJson(UA_PubSubManager *psm, UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount) {
    /* Prepare the NetworkMessage */
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    nm.version = 1;
    nm.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    nm.payloadHeaderEnabled = true;
    nm.payload.dataSetMessages = dsm;
    nm.messageCount = dsmCount;
    nm.publisherIdEnabled = true;
    nm.publisherId = connection->config.publisherId;

    for(size_t i = 0; i < dsmCount; i++)
        nm.dataSetWriterIds[i] = writerIds[i];

    PubSubEncodeJsonCtx ctx;
    memset(&ctx, 0, sizeof(PubSubEncodeJsonCtx));

    /* Prepare the metadata to encode the DataSetMessages */
    size_t i = 0;
    UA_STACKARRAY(UA_DataSetMessage_EncodingMetaData, emd, wg->writersCount);
    memset(emd, 0, sizeof(UA_DataSetMessage_EncodingMetaData) * wg->writersCount);
    ctx.eo.metaData = emd;
    ctx.eo.metaDataSize = wg->writersCount;
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        emd[i].dataSetWriterId = dsw->config.dataSetWriterId;
        UA_PublishedDataSet *pds = dsw->connectedDataSet;
        if(pds) {
            emd[i].fields = pds->dataSetMetaData.fields;
            emd[i].fieldsSize = pds->dataSetMetaData.fieldsSize;
        }
        i++;
    }

    /* Compute the message length */
    size_t msgSize = UA_NetworkMessage_calcSizeJson(&nm, &ctx.eo, NULL);

    UA_ConnectionManager *cm = connection->cm;
    if(!cm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Select the wg sendchannel if configured */
    uintptr_t sendChannel = connection->sendChannel;
    if(wg->sendChannel != 0)
        sendChannel = wg->sendChannel;
    if(sendChannel == 0) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg, "Cannot send, no open connection");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Allocate the buffer */
    UA_ByteString buf;
    UA_StatusCode res = cm->allocNetworkBuffer(cm, sendChannel, &buf, msgSize);
    UA_CHECK_STATUS(res, return res);

    /* Encode the message */
    ctx.ctx.pos = buf.data;
    ctx.ctx.end = &buf.data[msgSize];
    res = UA_NetworkMessage_encodeJsonInternal(&ctx, &nm);
    if(res != UA_STATUSCODE_GOOD) {
        cm->freeNetworkBuffer(cm, sendChannel, &buf);
        return res;
    }
    UA_assert(ctx.ctx.pos == ctx.ctx.end);

    /* Send the prepared messages */
    sendNetworkMessageBuffer(psm, wg, connection, sendChannel, &buf);
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
generateNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                       UA_ExtensionObject *messageSettings,
                       UA_ExtensionObject *transportSettings,
                       UA_NetworkMessage *nm) {
    UA_UadpWriterGroupMessageDataType tmpWgm;
    UA_UadpWriterGroupMessageDataType *wgm;
    if(UA_ExtensionObject_hasDecodedType(messageSettings,
           &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])) {
        wgm = (UA_UadpWriterGroupMessageDataType*)messageSettings->content.decoded.data;
    } else {
        /* Use default settings */
        UA_UadpWriterGroupMessageDataType_init(&tmpWgm);
        wgm = &tmpWgm;
    }

    nm->publisherIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID) != 0;
    nm->groupHeaderEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER) != 0;
    nm->groupHeader.writerGroupIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID) != 0;
    nm->groupHeader.groupVersionEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION) != 0;
    nm->groupHeader.networkMessageNumberEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER) != 0;
    nm->groupHeader.sequenceNumberEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER) != 0;
    nm->payloadHeaderEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER) != 0;
    nm->timestampEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_TIMESTAMP) != 0;
    nm->picosecondsEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PICOSECONDS) != 0;
    nm->dataSetClassIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID) != 0;
    nm->promotedFieldsEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PROMOTEDFIELDS) != 0;

    /* Set the SecurityHeader */
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        nm->securityEnabled = true;
        nm->securityHeader.networkMessageSigned = true;
        if(wg->config.securityMode >= UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            nm->securityHeader.networkMessageEncrypted = true;
        nm->securityHeader.securityTokenId = wg->securityTokenId;

        /* Generate the MessageNonce. Four random bytes followed by a four-byte
         * sequence number */
        UA_ByteString nonce = {4, nm->securityHeader.messageNonce};
        UA_StatusCode rv = wg->config.securityPolicy->symmetricModule.
            generateNonce(wg->config.securityPolicy->policyContext, &nonce);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
        UA_Byte *pos = &nm->securityHeader.messageNonce[4];
        const UA_Byte *end = &nm->securityHeader.messageNonce[8];
        UA_UInt32_encodeBinary(&wg->nonceSequenceNumber, &pos, end);
        nm->securityHeader.messageNonceSize = 8;
    }

    nm->version = 1;
    nm->networkMessageType = UA_NETWORKMESSAGE_DATASET;
    /* shallow copy of the PublisherId from connection configuration
        -> the configuration needs to be stable during publishing process
        -> it must not be cleaned after network message has been sent */
    nm->publisherId = connection->config.publisherId;

    if(nm->groupHeader.sequenceNumberEnabled)
        nm->groupHeader.sequenceNumber = wg->sequenceNumber;

    if(nm->groupHeader.groupVersionEnabled)
        nm->groupHeader.groupVersion = wgm->groupVersion;

    nm->groupHeader.writerGroupId = wg->config.writerGroupId;
    /* number of the NetworkMessage inside a PublishingInterval */
    nm->groupHeader.networkMessageNumber = 1;
    nm->payload.dataSetMessages = dsm;
    nm->messageCount = dsmCount;

    for(size_t i = 0; i < dsmCount; i++)
        nm->dataSetWriterIds[i] = writerIds[i];

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sendNetworkMessageBinary(UA_PubSubManager *psm, UA_PubSubConnection *connection,
                         UA_WriterGroup *wg, UA_DataSetMessage *dsm, UA_UInt16 *writerIds,
                         UA_Byte dsmCount) {
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));

    /* Fill the message structure */
    UA_StatusCode rv =
        generateNetworkMessage(connection, wg, dsm, writerIds, dsmCount,
                               &wg->config.messageSettings,
                               &wg->config.transportSettings, &nm);
    UA_CHECK_STATUS(rv, return rv);

    PubSubEncodeCtx ctx;
    memset(&ctx, 0, sizeof(PubSubEncodeCtx));

    /* Prepare the metadata with information from the readers to decode the
     * DataSetMessages */
    size_t i = 0;
    UA_STACKARRAY(UA_DataSetMessage_EncodingMetaData, emd, wg->writersCount);
    memset(emd, 0, sizeof(UA_DataSetMessage_EncodingMetaData) * wg->writersCount);
    ctx.eo.metaData = emd;
    ctx.eo.metaDataSize = wg->writersCount;
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        emd[i].dataSetWriterId = dsw->config.dataSetWriterId;
        UA_PublishedDataSet *pds = dsw->connectedDataSet;
        if(pds) {
            emd[i].fields = pds->dataSetMetaData.fields;
            emd[i].fieldsSize = pds->dataSetMetaData.fieldsSize;
        }
        i++;
    }

    /* Compute the message size. Add the overhead for the security signature.
     * There is no padding and the encryption incurs no size overhead. */
    size_t msgSize = UA_NetworkMessage_calcSizeBinaryInternal(&ctx, &nm);
    if(msgSize == 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Add the overhead for the security signature.
     * There is no padding and the encryption incurs no size overhead. */
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        UA_PubSubSecurityPolicy *sp = wg->config.securityPolicy;
        msgSize += sp->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(sp->policyContext);
    }

    UA_ConnectionManager *cm = connection->cm;
    if(!cm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Select the wg sendchannel if configured */
    uintptr_t sendChannel = connection->sendChannel;
    if(wg->sendChannel != 0)
        sendChannel = wg->sendChannel;
    if(sendChannel == 0) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg, "Cannot send, no open connection");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Allocate the buffer. Allocate on the stack if the buffer is small. */
    UA_ByteString buf = UA_BYTESTRING_NULL;
    rv = cm->allocNetworkBuffer(cm, sendChannel, &buf, msgSize);
    UA_CHECK_STATUS(rv, return rv);

    /* Encode and encrypt the message */
    ctx.ctx.pos = buf.data;
    ctx.ctx.end = &buf.data[buf.length];
    rv = encodeNetworkMessage(wg, &ctx, &nm, &buf);
    if(rv != UA_STATUSCODE_GOOD) {
        cm->freeNetworkBuffer(cm, sendChannel, &buf);
        return rv;
    }

    /* Send out the message */
    sendNetworkMessageBuffer(psm, wg, connection, sendChannel, &buf);
    return UA_STATUSCODE_GOOD;
}

static void
sendNetworkMessage(UA_PubSubManager *psm, UA_WriterGroup *wg, UA_PubSubConnection *connection,
                   UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount) {
    if(dsmCount >= UA_NETWORKMESSAGE_MAXMESSAGECOUNT) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                            "More DataSetMessages than allowed in "
                            "UA_NETWORKMESSAGE_MAXMESSAGECOUNT");
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_ERROR);
        return;
    }

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(wg->config.encodingMimeType) {
    case UA_PUBSUB_ENCODING_UADP:
        res = sendNetworkMessageBinary(psm, connection, wg, dsm, writerIds, dsmCount);
        break;
#ifdef UA_ENABLE_JSON_ENCODING
    case UA_PUBSUB_ENCODING_JSON:
        res = sendNetworkMessageJson(psm, connection, wg, dsm, writerIds, dsmCount);
        break;
#endif
    default:
        res = UA_STATUSCODE_BADNOTSUPPORTED;
        break;
    }

    /* If sending failed, disable all writer of the writergroup */
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                            "PubSub Publish: Could not send a NetworkMessage "
                            "with status code %s", UA_StatusCode_name(res));
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_ERROR);
    }
}

/* This callback triggers the collection and publish of NetworkMessages and the
 * contained DataSetMessages. */
void
UA_WriterGroup_publishCallback(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    UA_assert(wg != NULL);
    UA_assert(psm != NULL);

    UA_LOG_DEBUG_PUBSUB(psm->logging, wg, "Publish Callback");

    lockServer(psm->sc.server);

    /* Find the connection associated with the writer */
    UA_PubSubConnection *connection = wg->linkedConnection;
    if(!connection) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                            "Publish failed. PubSubConnection invalid");
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_ERROR);
        unlockServer(psm->sc.server);
        return;
    }

    /* How many DSM can be sent in one NM? */
    UA_Byte maxDSM = (UA_Byte)wg->config.maxEncapsulatedDataSetMessageCount;
    if(wg->config.maxEncapsulatedDataSetMessageCount > UA_BYTE_MAX)
        maxDSM = UA_BYTE_MAX;
    if(maxDSM == 0)
        maxDSM = 1; /* Send at least one dsm */

    /* It is possible to put several DataSetMessages into one NetworkMessage.
     * But only if they do not contain promoted fields. NM with promoted fields
     * are sent out right away. The others are kept in a buffer for
     * "batching". */
    size_t dsmCount = 0;
    UA_STACKARRAY(UA_UInt16, dsWriterIds, wg->writersCount);
    UA_STACKARRAY(UA_DataSetMessage, dsmStore, wg->writersCount);

    size_t enabledWriters = 0;

    UA_DataSetWriter *dsw;
    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        if(dsw->head.state != UA_PUBSUBSTATE_OPERATIONAL)
            continue;

        enabledWriters++;

        /* PDS can be NULL -> Heartbeat */
        UA_PublishedDataSet *pds = dsw->connectedDataSet;

        /* Generate the DSM */
        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        UA_StatusCode res =
            UA_DataSetWriter_generateDataSetMessage(psm, dsw, &dsmStore[dsmCount]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_PUBSUB(psm->logging, dsw,
                                "PubSub Publish: DataSetMessage creation failed");
            UA_DataSetWriter_setPubSubState(psm, dsw, UA_PUBSUBSTATE_ERROR);
            continue;
        }

        /* There is no promoted field -> send right away */
        if(pds && pds->promotedFieldsCount > 0) {
            wg->lastPublishTimeStamp = el->dateTime_nowMonotonic(el);
            sendNetworkMessage(psm, wg, connection, &dsmStore[dsmCount],
                               &dsWriterIds[dsmCount], 1);

            UA_DataSetMessage_clear(&dsmStore[dsmCount]);
            continue; /* Don't increase the dsmCount, reuse the slot */
        }

        dsmCount++;
    }

    /* No enabled Writers */
    if(enabledWriters == 0) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "Cannot publish -- No Writers are enabled");
        unlockServer(psm->sc.server);
        return;
    }

    /* Send the NetworkMessages with batched DataSetMessages */
    UA_Byte nmDsmCount = 0;
    for(size_t i = 0; i < dsmCount; i += nmDsmCount) {
        /* How many dsm are batched in this iteration? */
        nmDsmCount = (i + maxDSM > dsmCount) ? (UA_Byte)(dsmCount - i) : maxDSM;
        wg->lastPublishTimeStamp = el->dateTime_nowMonotonic(el);
        /* Send the batched messages */
        sendNetworkMessage(psm, wg, connection, &dsmStore[i],
                           &dsWriterIds[i], nmDsmCount);
    }

    /* Clean up DSM */
    for(size_t i = 0; i < dsmCount; i++) {
        UA_DataSetMessage_clear(&dsmStore[i]);
    }

    unlockServer(psm->sc.server);
}

/***********************/
/* Connection Handling */
/***********************/

static UA_StatusCode
UA_WriterGroup_connectMQTT(UA_PubSubManager *psm, UA_WriterGroup *wg,
                           UA_Boolean validate);

static UA_StatusCode
UA_WriterGroup_connectUDPUnicast(UA_PubSubManager *psm, UA_WriterGroup *wg,
                                 UA_Boolean validate);

typedef struct  {
    UA_String profileURI;
    UA_String protocol;
    UA_Boolean json;
    UA_StatusCode (*connectWriterGroup)(UA_PubSubManager *psm, UA_WriterGroup *wg,
                                        UA_Boolean validate);
} WriterGroupProfileMapping;

static WriterGroupProfileMapping writerGroupProfiles[UA_PUBSUB_PROFILES_SIZE] = {
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"),
     UA_STRING_STATIC("udp"), false, UA_WriterGroup_connectUDPUnicast},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp"),
     UA_STRING_STATIC("mqtt"), false, UA_WriterGroup_connectMQTT},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json"),
     UA_STRING_STATIC("mqtt"), true, UA_WriterGroup_connectMQTT},
    {UA_STRING_STATIC("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"),
     UA_STRING_STATIC("eth"), false, NULL}
};

static void
WriterGroupChannelCallback(UA_ConnectionManager *cm, uintptr_t connectionId,
                          void *application, void **connectionContext,
                          UA_ConnectionState state, const UA_KeyValueMap *params,
                          UA_ByteString msg) {
    if(!connectionContext)
        return;

    /* Get the context pointers */
    UA_WriterGroup *wg = (UA_WriterGroup*)*connectionContext;
    UA_PubSubManager *psm = (UA_PubSubManager*)application;
    UA_Server *server = psm->sc.server;

    lockServer(server);

    /* The connection is closing in the EventLoop. This is the last callback
     * from that connection. Clean up the SecureChannel in the client. */
    if(state == UA_CONNECTIONSTATE_CLOSING) {
        if(wg->sendChannel == connectionId) {
            /* Reset the connection channel */
            wg->sendChannel = 0;

            /* PSC marked for deletion and the last EventLoop connection has closed */
            if(wg->deleteFlag) {
                UA_WriterGroup_remove(psm, wg);
                unlockServer(server);
                return;
            }
        }

        /* Reconnect automatically if the connection was operational. This sets
         * the connection state if connecting fails. Attention! If there are
         * several send or recv channels, then the connection is only reopened if
         * all of them close - which is usually the case. */
        if(wg->head.state == UA_PUBSUBSTATE_OPERATIONAL)
            UA_WriterGroup_connect(psm, wg, false);

        /* Switch the psm state from stopping to stopped once the last
         * connection has closed */
        UA_PubSubManager_setState(psm, psm->sc.state);

        unlockServer(server);
        return;
    }

    /* Store the connectionId (if a new connection) */
    if(wg->sendChannel && wg->sendChannel != connectionId) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "WriterGroup is already bound to a different channel");
        unlockServer(server);
        return;
    }
    wg->sendChannel = connectionId;

    /* Connection open, set to operational if not already done */
    UA_WriterGroup_setPubSubState(psm, wg, wg->head.state);
    
    /* Send-channels don't receive messages */
    unlockServer(server);
}

static UA_StatusCode
UA_WriterGroup_connectUDPUnicast(UA_PubSubManager *psm, UA_WriterGroup *wg,
                                 UA_Boolean validate) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

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
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
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
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
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
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
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
    res = cm->openConnection(cm, &kvm, psm, wg, WriterGroupChannelCallback);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg, "Could not open a UDP send channel");
    }
    return res;
}

static UA_StatusCode
UA_WriterGroup_connectMQTT(UA_PubSubManager *psm, UA_WriterGroup *wg,
                           UA_Boolean validate) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    UA_PubSubConnection *c = wg->linkedConnection;
    UA_NetworkAddressUrlDataType *addressUrl = (UA_NetworkAddressUrlDataType*)
        c->config.address.data;

    /* Get the TransportSettings */
    UA_ExtensionObject *ts = &wg->config.transportSettings;
    if((ts->encoding != UA_EXTENSIONOBJECT_DECODED &&
        ts->encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       ts->content.decoded.type !=
       &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE]) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg, "Wrong TransportSettings type for MQTT");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_BrokerWriterGroupTransportDataType *transportSettings =
        (UA_BrokerWriterGroupTransportDataType*)ts->content.decoded.data;

    /* Extract hostname and port */
    UA_String address;
    UA_UInt16 port = 1883; /* Default */
    UA_StatusCode res = UA_parseEndpointUrl(&addressUrl->url, &address, &port, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c, "Could not parse the MQTT network URL");
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
    res = c->cm->openConnection(c->cm, &kvm, psm, wg, WriterGroupChannelCallback);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg, "Could not open the MQTT connection");
    }
    return res;
}

static void
UA_WriterGroup_disconnect(UA_WriterGroup *wg) {
    if(wg->sendChannel == 0)
        return;
    UA_PubSubConnection *c = wg->linkedConnection;
    if(!c || !c->cm)
        return;
    c->cm->closeConnection(c->cm, wg->sendChannel);
}

static UA_StatusCode
UA_WriterGroup_connect(UA_PubSubManager *psm, UA_WriterGroup *wg,
                       UA_Boolean validate) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    /* Check if already connected or no WG TransportSettings */
    if(!UA_WriterGroup_canConnect(wg) && !validate)
        return UA_STATUSCODE_GOOD;

    /* Is this a WriterGroup with custom TransportSettings beyond the
     * PubSubConnection? */
    if(wg->config.transportSettings.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY)
        return UA_STATUSCODE_GOOD;

    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    if(!el) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg, "No EventLoop configured");
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_ERROR);
        return UA_STATUSCODE_BADINTERNALERROR;;
    }

    UA_PubSubConnection *c = wg->linkedConnection;
    if(!c)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Look up the connection manager for the connection */
    WriterGroupProfileMapping *profile = NULL;
    for(size_t i = 0; i < UA_PUBSUB_PROFILES_SIZE; i++) {
        if(!UA_String_equal(&c->config.transportProfileUri,
                            &writerGroupProfiles[i].profileURI))
            continue;
        profile = &writerGroupProfiles[i];
        break;
    }

    UA_ConnectionManager *cm = (profile) ? getCM(el, profile->protocol) : NULL;
    if(!cm || (c->cm && cm != c->cm)) {
        UA_LOG_ERROR_PUBSUB(psm->logging, c,
                            "The requested profile \"%S\"is not supported",
                            c->config.transportProfileUri);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    c->cm = cm;
    c->json = profile->json;

    /* Connect */
    return (profile->connectWriterGroup) ?
        profile->connectWriterGroup(psm, wg, validate) : UA_STATUSCODE_GOOD;
}

/**************/
/* Server API */
/**************/

UA_StatusCode
UA_Server_addWriterGroup(UA_Server *server, const UA_NodeId connection,
                         const UA_WriterGroupConfig *writerGroupConfig,
                         UA_NodeId *writerGroupIdentifier) {
    if(!server || !writerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_StatusCode res = UA_WriterGroup_create(psm, connection, writerGroupConfig,
                                              writerGroupIdentifier);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_removeWriterGroup(UA_Server *server, const UA_NodeId writerGroup) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    if(wg)
        UA_WriterGroup_remove(psm, wg);
    else
        res = UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_enableWriterGroup(UA_Server *server, const UA_NodeId writerGroup)  {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    UA_StatusCode res =
        (wg) ? UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_OPERATIONAL)
             :  UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_disableWriterGroup(UA_Server *server,
                             const UA_NodeId writerGroup) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    UA_StatusCode res =
        (wg) ? UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED)
             :  UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

#ifdef UA_ENABLE_PUBSUB_SKS
UA_StatusCode
UA_Server_setWriterGroupActivateKey(UA_Server *server,
                                    const UA_NodeId writerGroup) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    if(wg) {
        if(wg->keyStorage && wg->keyStorage->currentItem) {
            res = UA_PubSubKeyStorage_activateKeyToChannelContext(
                psm, wg->head.identifier, wg->config.securityGroupId);
        } else {
            res = UA_STATUSCODE_BADINTERNALERROR;
        }
    }
    unlockServer(server);
    return res;
}
#endif

UA_StatusCode
UA_Server_getWriterGroupConfig(UA_Server *server, const UA_NodeId writerGroup,
                               UA_WriterGroupConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    UA_StatusCode res = (wg) ?
        UA_WriterGroupConfig_copy(&wg->config, config) : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getWriterGroupState(UA_Server *server, const UA_NodeId wgId,
                              UA_PubSubState *state) {
    if(!server || !state)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(getPSM(server), wgId);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(wg)
        *state = wg->head.state;
    else
        res = UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_triggerWriterGroupPublish(UA_Server *server,
                                    const UA_NodeId wgId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    if(!wg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    unlockServer(server);
    UA_WriterGroup_publishCallback(psm, wg);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getWriterGroupLastPublishTimestamp(UA_Server *server,
                                             const UA_NodeId wgId,
                                             UA_DateTime *timestamp) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(getPSM(server), wgId);
    if(!wg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    *timestamp = wg->lastPublishTimeStamp;
    unlockServer(server);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setWriterGroupEncryptionKeys(UA_Server *server, const UA_NodeId writerGroup,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    UA_StatusCode res =
        (wg) ? UA_WriterGroup_setEncryptionKeys(psm, wg, securityTokenId,
                                                 signingKey, encryptingKey, keyNonce)
              : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_updateWriterGroupConfig(UA_Server *server, const UA_NodeId wgId,
                                  const UA_WriterGroupConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    if(!wg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    if(UA_PubSubState_isEnabled(wg->head.state)) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                            "The WriterGroup must be disabled to update the config");
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Validate the new configuration */
    UA_StatusCode res = validateWriterGroupConfig(psm, &wg->head, config);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    /* Store the old configuration */
    UA_WriterGroupConfig oldConfig = wg->config;
    memset(&wg->config, 0, sizeof(UA_WriterGroupConfig));

    /* Deep copy the new config */
    res = UA_WriterGroupConfig_copy(config, &wg->config);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Validate the connection settings */
    res = UA_WriterGroup_connect(psm, wg, true);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                            "Could not validate the connection parameters");
        goto errout;
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    if(!UA_String_equal(&wg->config.securityGroupId, &oldConfig.securityGroupId) ||
       wg->config.securityMode != oldConfig.securityMode) {
        /* Detach keystorage and reattach if needed */
        if(wg->keyStorage) {
            UA_PubSubKeyStorage_detachKeyStorage(psm, wg->keyStorage);
            wg->keyStorage = NULL;
        }
        if(wg->config.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
           wg->config.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
            res = writerGroupAttachSKSKeystorage(psm, wg);
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                                    "Attaching the SKS KeyStorage failed");
                goto errout;
            }
        }
    }
#endif

    /* Call the state-machine. This can move the wg state from _ERROR to
     * _DISABLED. */
    UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED);

    /* Clean up and return */
    UA_WriterGroupConfig_clear(&oldConfig);
    unlockServer(server);
    return UA_STATUSCODE_GOOD;

 errout:
    UA_WriterGroupConfig_clear(&wg->config);
    wg->config = oldConfig; /* Restore the old config */
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_computeWriterGroupOffsetTable(UA_Server *server,
                                        const UA_NodeId writerGroupId,
                                        UA_PubSubOffsetTable *ot) {
    if(!server || !ot)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    /* Get the Writer Group */
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = (psm) ? UA_WriterGroup_find(psm, writerGroupId) : NULL;
    if(!wg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Initialize variables so we can goto cleanup below */
    UA_DataSetField *field = NULL;
    UA_NetworkMessage networkMessage;
    memset(&networkMessage, 0, sizeof(networkMessage));
    memset(ot, 0, sizeof(UA_PubSubOffsetTable));

    /* Prepare the metadata encode the DataSetMessages */
    PubSubEncodeCtx ctx;
    memset(&ctx, 0, sizeof(PubSubEncodeCtx));
    ctx.ot = ot;

    size_t i = 0;
    UA_STACKARRAY(UA_DataSetMessage_EncodingMetaData, emd, wg->writersCount);
    memset(emd, 0, sizeof(UA_DataSetMessage_EncodingMetaData) * wg->writersCount);
    ctx.eo.metaData = emd;
    ctx.eo.metaDataSize = wg->writersCount;
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        emd[i].dataSetWriterId = dsw->config.dataSetWriterId;
        UA_PublishedDataSet *pds = dsw->connectedDataSet;
        if(pds) {
            emd[i].fields = pds->dataSetMetaData.fields;
            emd[i].fieldsSize = pds->dataSetMetaData.fieldsSize;
        }
        i++;
    }

    /* Validate the DataSetWriters and generate their DataSetMessage */
    size_t msgSize;
    size_t dsmCount = 0;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_STACKARRAY(UA_UInt16, dsWriterIds, wg->writersCount);
    UA_STACKARRAY(UA_DataSetMessage, dsmStore, wg->writersCount);
    memset(dsmStore, 0, sizeof(UA_DataSetMessage) * wg->writersCount);
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        res = UA_DataSetWriter_generateDataSetMessage(psm, dsw, &dsmStore[dsmCount]);
        dsmCount++;
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Generate the NetworkMessage */
    res = generateNetworkMessage(wg->linkedConnection, wg, dsmStore, dsWriterIds,
                                 (UA_Byte) dsmCount, &wg->config.messageSettings,
                                 &wg->config.transportSettings, &networkMessage);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Compute the message length and generate the old format offset-table (done
     * inside calcSizeBinary) */
    msgSize = UA_NetworkMessage_calcSizeBinaryInternal(&ctx, &networkMessage);
    if(msgSize == 0) {
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    /* Create the encoded network message */
    res = UA_NetworkMessage_encodeBinary(&networkMessage, &ot->networkMessage, &ctx.eo);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Pick up the component NodeIds */
    dsw = NULL;
    for(size_t i = 0; i < ot->offsetsSize; i++) {
        UA_PubSubOffset *o = &ot->offsets[i];
        switch(o->offsetType) {
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_SEQUENCENUMBER:
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_TIMESTAMP:
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_PICOSECONDS:
        case UA_PUBSUBOFFSETTYPE_NETWORKMESSAGE_GROUPVERSION:
            res |= UA_NodeId_copy(&wg->head.identifier, &o->component);
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE:
            dsw = (dsw == NULL) ? LIST_FIRST(&wg->writers) : LIST_NEXT(dsw, listEntry);
            field = NULL;
            /* fall through */
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_STATUS:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_TIMESTAMP:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_PICOSECONDS:
            UA_assert(dsw);
            res |= UA_NodeId_copy(&dsw->head.identifier, &o->component);
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE:
            UA_assert(dsw && dsw->connectedDataSet);
            field = (field == NULL) ?
                TAILQ_FIRST(&dsw->connectedDataSet->fields) : TAILQ_NEXT(field, listEntry);
            res |= UA_NodeId_copy(&field->identifier, &o->component);
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_VARIANT:
            UA_assert(dsw && dsw->connectedDataSet);
            field = (field == NULL) ?
                TAILQ_FIRST(&dsw->connectedDataSet->fields) : TAILQ_NEXT(field, listEntry);
            res |= UA_NodeId_copy(&field->identifier, &o->component);
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW:
            UA_assert(dsw && dsw->connectedDataSet);
            field = (field == NULL) ?
                TAILQ_FIRST(&dsw->connectedDataSet->fields) : TAILQ_NEXT(field, listEntry);
            res |= UA_NodeId_copy(&field->identifier, &o->component);
            break;
        default:
            break;
        }
    }

    /* Clean up */
 cleanup:
    if(res != UA_STATUSCODE_GOOD)
        UA_PubSubOffsetTable_clear(ot);

    for(size_t i = 0; i < dsmCount; i++) {
        UA_DataSetMessage_clear(&dsmStore[i]);
    }

    unlockServer(server);

    return res;
}

void
UA_PubSubOffsetTable_clear(UA_PubSubOffsetTable *ot) {
    for(size_t i = 0; i < ot->offsetsSize; i++) {
        UA_NodeId_clear(&ot->offsets[i].component);
    }
    UA_ByteString_clear(&ot->networkMessage);
    UA_free(ot->offsets);
    memset(ot, 0, sizeof(UA_PubSubOffsetTable));
}

#endif /* UA_ENABLE_PUBSUB */
