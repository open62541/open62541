/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include "ua_pubsub.h"
#include "../server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_SKS
#include "ua_pubsub_keystorage.h"
#endif

#define UA_MAX_STACKBUF 128 /* Max size of network messages on the stack */

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

UA_Boolean
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

static void
UA_WriterGroup_publishCallback_server(UA_Server *server, UA_WriterGroup *wg) {
    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        UA_LOG_WARNING_PUBSUB(server->config.logging, wg,
                              "Cannot publish -- PubSub not configured for the server");
        return;
    }
    UA_WriterGroup_publishCallback(psm, wg);
}

UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    /* Already registered */
    if(wg->publishCallbackId != 0)
        return UA_STATUSCODE_GOOD;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(wg->config.pubsubManagerCallback.addCustomCallback) {
        /* Use configured mechanism for cyclic callbacks */
        retval = wg->config.pubsubManagerCallback.
            addCustomCallback(psm->sc.server, wg->head.identifier,
                              (UA_ServerCallback)UA_WriterGroup_publishCallback_server,
                              wg, wg->config.publishingInterval,
                              NULL, UA_TIMERPOLICY_CURRENTTIME,
                              &wg->publishCallbackId);
    } else {
        /* Use EventLoop for cyclic callbacks */
        UA_EventLoop *el = UA_PubSubConnection_getEL(psm, wg->linkedConnection);
        retval = el->addTimer(el, (UA_Callback)UA_WriterGroup_publishCallback,
                              psm, wg, wg->config.publishingInterval,
                              NULL /* TODO: use basetime */,
                              UA_TIMERPOLICY_CURRENTTIME,
                              &wg->publishCallbackId);
    }

    return retval;
}

void
UA_WriterGroup_removePublishCallback(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    if(wg->publishCallbackId == 0)
        return;
    if(wg->config.pubsubManagerCallback.removeCustomCallback) {
        wg->config.pubsubManagerCallback.
            removeCustomCallback(psm->sc.server, wg->head.identifier, wg->publishCallbackId);
    } else {
        UA_EventLoop *el = UA_PubSubConnection_getEL(psm, wg->linkedConnection);
        el->removeTimer(el, wg->publishCallbackId);
    }
    wg->publishCallbackId = 0;
}

UA_StatusCode
UA_WriterGroup_create(UA_PubSubManager *psm, const UA_NodeId connection,
                      const UA_WriterGroupConfig *writerGroupConfig,
                      UA_NodeId *writerGroupIdentifier) {
    /* Delete the reserved IDs if the related session no longer exists. */
    UA_PubSubManager_freeIds(psm);
    if(!writerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connection);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Validate messageSettings type */
    const UA_ExtensionObject *ms = &writerGroupConfig->messageSettings;
    if(ms->content.decoded.type) {
        if(writerGroupConfig->encodingMimeType == UA_PUBSUB_ENCODING_JSON &&
           (ms->encoding != UA_EXTENSIONOBJECT_DECODED ||
            ms->content.decoded.type != &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE])) {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }

        if(writerGroupConfig->encodingMimeType == UA_PUBSUB_ENCODING_UADP &&
           (ms->encoding != UA_EXTENSIONOBJECT_DECODED ||
            ms->content.decoded.type != &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])) {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }

    /* Allocate new WriterGroup */
    UA_WriterGroup *newWriterGroup = (UA_WriterGroup*)UA_calloc(1, sizeof(UA_WriterGroup));
    if(!newWriterGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newWriterGroup->head.componentType = UA_PUBSUBCOMPONENT_WRITERGROUP;
    newWriterGroup->linkedConnection = c;

    /* Deep copy of the config */
    UA_WriterGroupConfig *newConfig = &newWriterGroup->config;
    UA_StatusCode res = UA_WriterGroupConfig_copy(writerGroupConfig, newConfig);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(newWriterGroup);
        return res;
    }

    /* Create the datatype value if not present */
    if(!newConfig->messageSettings.content.decoded.type) {
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        newConfig->messageSettings.content.decoded.data = wgm;
        newConfig->messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        newConfig->messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    }

    /* Attach to the connection */
    LIST_INSERT_HEAD(&c->writerGroups, newWriterGroup, listEntry);
    c->writerGroupsSize++;

    /* Add representation / create unique identifier */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    res = addWriterGroupRepresentation(psm->sc.server, newWriterGroup);
    if(res != UA_STATUSCODE_GOOD) {
        UA_WriterGroup_remove(psm, newWriterGroup);
        return res;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(psm, &newWriterGroup->head.identifier);
#endif

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "%SWriterGroup %N\t| ",
                c->head.logIdString, newWriterGroup->head.identifier);
    newWriterGroup->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    UA_LOG_INFO_PUBSUB(psm->logging, newWriterGroup,
                       "WriterGroup created (State: %s)",
                       UA_PubSubState_name(newWriterGroup->head.state));

    /* Validate the connection settings */
    res = UA_WriterGroup_connect(psm, newWriterGroup, true);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, newWriterGroup,
                            "Could not validate the connection parameters");
        UA_WriterGroup_remove(psm, newWriterGroup);
        return res;
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    if(writerGroupConfig->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       writerGroupConfig->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_String_isEmpty(&writerGroupConfig->securityGroupId) &&
           writerGroupConfig->securityPolicy) {
            /* Does the key storage already exist? */
            newWriterGroup->keyStorage =
                UA_PubSubKeyStorage_find(psm, writerGroupConfig->securityGroupId);

            if(!newWriterGroup->keyStorage) {
                /* Create a new key storage */
                newWriterGroup->keyStorage = (UA_PubSubKeyStorage *)
                    UA_calloc(1, sizeof(UA_PubSubKeyStorage));
                if(!newWriterGroup->keyStorage) {
                    UA_WriterGroup_remove(psm, newWriterGroup);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                res = UA_PubSubKeyStorage_init(psm, newWriterGroup->keyStorage,
                                               &writerGroupConfig->securityGroupId,
                                               writerGroupConfig->securityPolicy, 0, 0);
                if(res != UA_STATUSCODE_GOOD) {
                    UA_WriterGroup_remove(psm, newWriterGroup);
                    return res;
                }
            }

            /* Increase the ref count */
            newWriterGroup->keyStorage->referenceCount++;
        }
    }

#endif

    /* Trigger the connection */
    UA_PubSubConnection_setPubSubState(psm, c, c->head.state);

    /* Copying a numeric NodeId always succeeds */
    if(writerGroupIdentifier)
        UA_NodeId_copy(&newWriterGroup->head.identifier, writerGroupIdentifier);

    return UA_STATUSCODE_GOOD;
}

void
UA_WriterGroup_remove(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

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
        UA_NetworkMessageOffsetBuffer_clear(&wg->bufferedMessage);
        UA_PubSubComponentHead_clear(&wg->head);
        UA_free(wg);
    }

    /* Update the connection state */
    UA_PubSubConnection_setPubSubState(psm, connection, connection->head.state);
}

static UA_StatusCode
UA_WriterGroup_freezeConfiguration(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    if(wg->configurationFrozen)
        return UA_STATUSCODE_GOOD;

    /* Freeze the WriterGroup */
    wg->configurationFrozen = true;

    /* Offset table enabled? */
    if((wg->config.rtLevel & UA_PUBSUB_RT_FIXED_SIZE) == 0)
        return UA_STATUSCODE_GOOD;

    /* Offset table only possible for binary encoding */
    if(wg->config.encodingMimeType != UA_PUBSUB_ENCODING_UADP) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "PubSub-RT configuration fail: Non-RT capable encoding.");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    //TODO Clarify: should we only allow = maxEncapsulatedDataSetMessageCount == 1 with RT?
    //TODO Clarify: Behaviour if the finale size is more than MTU

    /* Define variables here for goto */
    size_t msgSize;
    UA_ByteString buf;
    const UA_Byte *bufEnd;
    UA_Byte *bufPos;
    UA_NetworkMessage networkMessage;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_STACKARRAY(UA_UInt16, dsWriterIds, wg->writersCount);
    UA_STACKARRAY(UA_DataSetMessage, dsmStore, wg->writersCount);

    /* Validate the DataSetWriters and generate their DataSetMessage */
    size_t dsmCount = 0;
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        res = UA_DataSetWriter_prepareDataSet(psm, dsw, &dsmStore[dsmCount]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup_dsm;
        dsmCount++;
    }

    /* Generate the NetworkMessage */
    memset(&networkMessage, 0, sizeof(networkMessage));
    res = generateNetworkMessage(wg->linkedConnection, wg, dsmStore, dsWriterIds,
                                 (UA_Byte) dsmCount, &wg->config.messageSettings,
                                 &wg->config.transportSettings, &networkMessage);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup_dsm;

    /* Compute the message length and generate the offset-table (done inside
     * calcSizeBinary) */
    memset(&wg->bufferedMessage, 0, sizeof(UA_NetworkMessageOffsetBuffer));
    msgSize = UA_NetworkMessage_calcSizeBinaryWithOffsetBuffer(&networkMessage,
                                                               &wg->bufferedMessage);

    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        UA_PubSubSecurityPolicy *sp = wg->config.securityPolicy;
        msgSize += sp->symmetricModule.cryptoModule.
                   signatureAlgorithm.getLocalSignatureSize(sp->policyContext);
    }

    /* Generate the buffer for the pre-encoded message */
    res = UA_ByteString_allocBuffer(&buf, msgSize);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    wg->bufferedMessage.buffer = buf;

    /* Encode the NetworkMessage */
    bufEnd = &wg->bufferedMessage.buffer.data[wg->bufferedMessage.buffer.length];
    bufPos = wg->bufferedMessage.buffer.data;

    /* Preallocate the encryption buffer */
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        UA_Byte *payloadPosition;
        UA_NetworkMessage_encodeBinaryWithEncryptStart(&networkMessage, &bufPos, bufEnd,
                                                       &payloadPosition);
        wg->bufferedMessage.payloadPosition = payloadPosition;
        wg->bufferedMessage.nm = (UA_NetworkMessage *)UA_calloc(1,sizeof(UA_NetworkMessage));
        wg->bufferedMessage.nm->securityHeader = networkMessage.securityHeader;
        UA_ByteString_allocBuffer(&wg->bufferedMessage.encryptBuffer, msgSize);
    }

    if(wg->config.securityMode <= UA_MESSAGESECURITYMODE_NONE)
        UA_NetworkMessage_encodeBinaryWithEncryptStart(&networkMessage, &bufPos, bufEnd, NULL);

    /* Post-processing of the OffsetBuffer to set the external data source from
     * the DataSetField configuration */
    if(wg->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS) {
        size_t fieldPos = 0;
        LIST_FOREACH(dsw, &wg->writers, listEntry) {
            UA_PublishedDataSet *pds = dsw->connectedDataSet;
            if(!pds)
                continue;

            /* Loop over all DataSetFields */
            UA_DataSetField *dsf;
            TAILQ_FOREACH(dsf, &pds->fields, listEntry) {
                UA_NetworkMessageOffsetType contentType;
                /* Move forward to the next payload-type offset field */
                do {
                    fieldPos++;
                    contentType = wg->bufferedMessage.offsets[fieldPos].contentType;
                } while(contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE &&
                        contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT &&
                        contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW);
                UA_assert(fieldPos < wg->bufferedMessage.offsetsSize);

                if(!dsf->config.field.variable.rtValueSource.rtFieldSourceEnabled)
                    continue;

                /* Set the external value soure in the offset buffer */
                UA_DataValue_clear(&wg->bufferedMessage.offsets[fieldPos].content.value);
                wg->bufferedMessage.offsets[fieldPos].content.externalValue =
                    dsf->config.field.variable.rtValueSource.staticValueSource;

                /* Update the content type to _EXTERNAL */
                wg->bufferedMessage.offsets[fieldPos].contentType =
                    (UA_NetworkMessageOffsetType)(contentType + 1);
            }
        }
    }

 cleanup:
    UA_free(networkMessage.payload.dataSetPayload.sizes);

 cleanup_dsm:
    /* Clean up DataSetMessages */
    for(size_t i = 0; i < dsmCount; i++) {
        UA_DataSetMessage_clear(&dsmStore[i]);
    }
    return res;
}

static void
UA_WriterGroup_unfreezeConfiguration(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    if(!wg->configurationFrozen)
        return;
    wg->configurationFrozen = false;
    UA_NetworkMessageOffsetBuffer_clear(&wg->bufferedMessage);
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
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    if(wg->deleteFlag && targetState != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "The WriterGroup is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Are we doing a top-level state update or recursively? */
    UA_Boolean isTransient = wg->head.transientState;
    wg->head.transientState = true;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = wg->head.state;
    UA_PubSubConnection *connection = wg->linkedConnection;

    switch(targetState) {
        /* Disabled or Error */
    case UA_PUBSUBSTATE_DISABLED:
    case UA_PUBSUBSTATE_ERROR:
        wg->head.state = targetState;
        UA_WriterGroup_unfreezeConfiguration(psm, wg);
        UA_WriterGroup_disconnect(wg);
        UA_WriterGroup_removePublishCallback(psm, wg);
        break;

        /* Enabled */
    case UA_PUBSUBSTATE_PAUSED:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
    case UA_PUBSUBSTATE_OPERATIONAL:
        /* Freeze the configuration */
        ret = UA_WriterGroup_freezeConfiguration(psm, wg);
        if(ret != UA_STATUSCODE_GOOD)
            break;

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
        UA_WriterGroup_unfreezeConfiguration(psm, wg);
        UA_WriterGroup_disconnect(wg);
        UA_WriterGroup_removePublishCallback(psm, wg);
    }

    /* Only the top-level state update (if recursive calls are happening)
     * notifies the application and updates Reader and WriterGroups */
    wg->head.transientState = isTransient;
    if(wg->head.transientState)
        return ret;

    /* Inform the application about state change */
    if(wg->head.state != oldState) {
        UA_ServerConfig *pConfig = &psm->sc.server->config;
        UA_LOG_INFO_PUBSUB(psm->logging, wg, "%s -> %s",
                           UA_PubSubState_name(oldState),
                           UA_PubSubState_name(wg->head.state));
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            UA_UNLOCK(&psm->sc.server->serviceMutex);
            pConfig->pubSubConfig.
                stateChangeCallback(psm->sc.server, wg->head.identifier, wg->head.state, ret);
            UA_LOCK(&psm->sc.server->serviceMutex);
        }
    }

    /* Update the attached DataSetWriters */
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
encodeNetworkMessage(UA_WriterGroup *wg, UA_NetworkMessage *nm,
                     UA_ByteString *buf) {
    UA_Byte *bufPos = buf->data;
    UA_Byte *bufEnd = &buf->data[buf->length];

    UA_Byte *networkMessageStart = bufPos;
    UA_StatusCode rv = UA_NetworkMessage_encodeHeaders(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    UA_Byte *payloadStart = bufPos;
    rv = UA_NetworkMessage_encodePayload(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_encodeFooters(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    /* Encrypt and Sign the message */
    UA_Byte *footerEnd = bufPos;
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
    nm.payloadHeader.dataSetPayloadHeader.count = dsmCount;
    nm.payloadHeader.dataSetPayloadHeader.dataSetWriterIds = writerIds;
    nm.payload.dataSetPayload.dataSetMessages = dsm;
    nm.publisherIdEnabled = true;
    nm.publisherId = connection->config.publisherId;

    /* Compute the message length */
    size_t msgSize = UA_NetworkMessage_calcSizeJsonInternal(&nm, NULL, 0, NULL, 0, true);

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
    UA_Byte *bufPos = buf.data;
    const UA_Byte *bufEnd = &buf.data[msgSize];
    res = UA_NetworkMessage_encodeJsonInternal(&nm, &bufPos, &bufEnd, NULL, 0, NULL, 0, true);
    if(res != UA_STATUSCODE_GOOD) {
        cm->freeNetworkBuffer(cm, sendChannel, &buf);
        return res;
    }
    UA_assert(bufPos == bufEnd);

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
    if(messageSettings->content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
            messageSettings->content.decoded.data;

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

    /* Compute the length of the dsm separately for the header */
    UA_UInt16 *dsmLengths = (UA_UInt16 *) UA_calloc(dsmCount, sizeof(UA_UInt16));
    if(!dsmLengths)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(UA_Byte i = 0; i < dsmCount; i++)
        dsmLengths[i] = (UA_UInt16) UA_DataSetMessage_calcSizeBinary(&dsm[i], NULL, 0);

    nm->payloadHeader.dataSetPayloadHeader.count = dsmCount;
    nm->payloadHeader.dataSetPayloadHeader.dataSetWriterIds = writerIds;
    nm->groupHeader.writerGroupId = wg->config.writerGroupId;
    /* number of the NetworkMessage inside a PublishingInterval */
    nm->groupHeader.networkMessageNumber = 1;
    nm->payload.dataSetPayload.sizes = dsmLengths;
    nm->payload.dataSetPayload.dataSetMessages = dsm;
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

    /* Compute the message size. Add the overhead for the security signature.
     * There is no padding and the encryption incurs no size overhead. */
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&nm);
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
    rv = encodeNetworkMessage(wg, &nm, &buf);
    if(rv != UA_STATUSCODE_GOOD) {
        cm->freeNetworkBuffer(cm, sendChannel, &buf);
        UA_free(nm.payload.dataSetPayload.sizes);
        return rv;
    }

    /* Send out the message */
    sendNetworkMessageBuffer(psm, wg, connection, sendChannel, &buf);

    UA_free(nm.payload.dataSetPayload.sizes);
    return UA_STATUSCODE_GOOD;
}

static void
sampleOffsetPublishingValues(UA_PubSubManager *psm, UA_WriterGroup *wg) {
    size_t fieldPos = 0;
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        UA_PublishedDataSet *pds = dsw->connectedDataSet;
        if(!pds)
            continue;

        /* Loop over the fields */
        UA_DataSetField *dsf;
        TAILQ_FOREACH(dsf, &pds->fields, listEntry) {
            /* Get the matching offset table entry */
            UA_NetworkMessageOffsetType contentType;
            do {
                fieldPos++;
                contentType = wg->bufferedMessage.offsets[fieldPos].contentType;
            } while(contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE &&
                    contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE_EXTERNAL &&
                    contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT &&
                    contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT_EXTERNAL &&
                    contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW &&
                    contentType != UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW_EXTERNAL);

            /* External data source is never sampled, but accessed directly in
             * the encoding */
            if(contentType == UA_PUBSUB_OFFSETTYPE_PAYLOAD_DATAVALUE_EXTERNAL ||
               contentType == UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT_EXTERNAL ||
               contentType == UA_PUBSUB_OFFSETTYPE_PAYLOAD_RAW_EXTERNAL)
                continue;

            /* Sample the value into the offset table */
            UA_DataValue *dfv = &wg->bufferedMessage.offsets[fieldPos].content.value;
            UA_DataValue_clear(dfv);
            UA_PubSubDataSetField_sampleValue(psm, dsf, dfv);
        }
    }
}

static void
publishWithOffsets(UA_PubSubManager *psm, UA_WriterGroup *wg,
                   UA_PubSubConnection *connection) {
    UA_assert(wg->configurationFrozen);

    /* Fixed size but no direct value access. Sample to get recent values into
     * the offset buffer structure. */
    if((wg->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS) == 0)
        sampleOffsetPublishingValues(psm, wg);

    UA_StatusCode res =
        UA_NetworkMessage_updateBufferedMessage(&wg->bufferedMessage);

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_PUBSUB(psm->logging, wg,
                            "PubSub sending. Unknown field type.");
        return;
    }

    UA_ByteString *buf = &wg->bufferedMessage.buffer;

    /* Send the encrypted buffered message if PubSub encryption is enabled */
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        size_t sigSize = wg->config.securityPolicy->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(wg->securityPolicyContext);

        UA_Byte payloadOffset = (UA_Byte)(wg->bufferedMessage.payloadPosition -
                                          wg->bufferedMessage.buffer.data);
        memcpy(wg->bufferedMessage.encryptBuffer.data,
               wg->bufferedMessage.buffer.data,
               wg->bufferedMessage.buffer.length);
        res = encryptAndSign(wg, wg->bufferedMessage.nm,
                             wg->bufferedMessage.encryptBuffer.data,
                             wg->bufferedMessage.encryptBuffer.data + payloadOffset,
                             wg->bufferedMessage.encryptBuffer.data +
                             wg->bufferedMessage.encryptBuffer.length - sigSize);

        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_PUBSUB(psm->logging, wg, "PubSub Encryption failed");
            return;
        }

        buf = &wg->bufferedMessage.encryptBuffer;
    }

    UA_ConnectionManager *cm = connection->cm;
    if(!cm)
        return;

    /* Select the wg sendchannel if configured */
    uintptr_t sendChannel = connection->sendChannel;
    if(wg->sendChannel != 0)
        sendChannel = wg->sendChannel;
    if(sendChannel == 0) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg, "Cannot send, no open connection");
        return;
    }

    /* Copy into the network buffer */
    UA_ByteString outBuf;
    res = cm->allocNetworkBuffer(cm, sendChannel, &outBuf, buf->length);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg, "PubSub message memory allocation failed");
        return;
    }
    memcpy(outBuf.data, buf->data, buf->length);
    sendNetworkMessageBuffer(psm, wg, connection, sendChannel, &outBuf);
}

static void
sendNetworkMessage(UA_PubSubManager *psm, UA_WriterGroup *wg, UA_PubSubConnection *connection,
                   UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount) {
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

    /* Find the connection associated with the writer */
    UA_PubSubConnection *connection = wg->linkedConnection;
    if(!connection) {
        UA_LOG_ERROR_PUBSUB(psm->logging, wg,
                            "Publish failed. PubSubConnection invalid");
        UA_LOCK(&psm->sc.server->serviceMutex);
        UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_ERROR);
        UA_UNLOCK(&psm->sc.server->serviceMutex);
        return;
    }

    /* Realtime path - update the buffer message and send directly */
    if(wg->config.rtLevel & UA_PUBSUB_RT_FIXED_SIZE) {
        publishWithOffsets(psm, wg, connection);
        return;
    }

    UA_LOCK(&psm->sc.server->serviceMutex);

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
    UA_EventLoop *el = UA_PubSubConnection_getEL(psm, wg->linkedConnection);
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        if(dsw->head.state != UA_PUBSUBSTATE_OPERATIONAL)
            continue;

        enabledWriters++;

        /* PDS can be NULL -> Heartbeat */
        UA_PublishedDataSet *pds = dsw->connectedDataSet;

        /* Generate the DSM */
        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        UA_StatusCode res =
            UA_DataSetWriter_generateDataSetMessage(psm, &dsmStore[dsmCount], dsw);
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

            /* Clean up the current store entry */
            if(wg->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS &&
               dsmStore[dsmCount].header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
                for(size_t i = 0; i < dsmStore[dsmCount].data.keyFrameData.fieldCount; ++i) {
                    dsmStore[dsmCount].data.keyFrameData.dataSetFields[i].value.data = NULL;
                }
            }
            UA_DataSetMessage_clear(&dsmStore[dsmCount]);

            continue; /* Don't increase the dsmCount, reuse the slot */
        }

        dsmCount++;
    }

    /* No enabled Writers */
    if(enabledWriters == 0) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "Cannot publish -- No Writers are enabled");
        UA_UNLOCK(&psm->sc.server->serviceMutex);
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
        if(wg->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS &&
           dsmStore[i].header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
            for(size_t j = 0; j < dsmStore[i].data.keyFrameData.fieldCount; ++j) {
                dsmStore[i].data.keyFrameData.dataSetFields[j].value.data = NULL;
            }
        }
        UA_DataSetMessage_clear(&dsmStore[i]);
    }

    UA_UNLOCK(&psm->sc.server->serviceMutex);
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
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    UA_StatusCode res = UA_WriterGroup_create(psm, connection, writerGroupConfig,
                                              writerGroupIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_removeWriterGroup(UA_Server *server, const UA_NodeId writerGroup) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    if(wg)
        UA_WriterGroup_remove(psm, wg);
    else
        res = UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_enableWriterGroup(UA_Server *server, const UA_NodeId writerGroup)  {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    UA_StatusCode res =
        (wg) ? UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_OPERATIONAL)
             :  UA_STATUSCODE_BADINTERNALERROR;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_disableWriterGroup(UA_Server *server,
                             const UA_NodeId writerGroup) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    UA_StatusCode res =
        (wg) ? UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED)
             :  UA_STATUSCODE_BADINTERNALERROR;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#ifdef UA_ENABLE_PUBSUB_SKS
UA_StatusCode
UA_Server_setWriterGroupActivateKey(UA_Server *server,
                                    const UA_NodeId writerGroup) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
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
    UA_UNLOCK(&server->serviceMutex);
    return res;
}
#endif

UA_StatusCode
UA_Server_getWriterGroupConfig(UA_Server *server, const UA_NodeId writerGroup,
                               UA_WriterGroupConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    UA_StatusCode res = (wg) ?
        UA_WriterGroupConfig_copy(&wg->config, config) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getWriterGroupState(UA_Server *server, const UA_NodeId wgId,
                              UA_PubSubState *state) {
    if(!server || !state)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_WriterGroup *wg = UA_WriterGroup_find(getPSM(server), wgId);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(wg)
        *state = wg->head.state;
    else
        res = UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_triggerWriterGroupPublish(UA_Server *server,
                                    const UA_NodeId wgId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgId);
    if(!wg) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_UNLOCK(&server->serviceMutex);
    UA_WriterGroup_publishCallback(psm, wg);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getWriterGroupLastPublishTimestamp(UA_Server *server,
                                             const UA_NodeId wgId,
                                             UA_DateTime *timestamp) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_WriterGroup *wg = UA_WriterGroup_find(getPSM(server), wgId);
    if(!wg) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    *timestamp = wg->lastPublishTimeStamp;
    UA_UNLOCK(&server->serviceMutex);
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
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    UA_StatusCode res =
        (wg) ? UA_WriterGroup_setEncryptionKeys(psm, wg, securityTokenId,
                                                 signingKey, encryptingKey, keyNonce)
              : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
