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
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#define UA_MAX_STACKBUF 128 /* Max size of network messages on the stack */

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
static UA_StatusCode
encryptAndSign(UA_WriterGroup *wg, const UA_NetworkMessage *nm,
               UA_Byte *signStart, UA_Byte *encryptStart,
               UA_Byte *msgEnd);

#endif

static UA_StatusCode
generateNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                       UA_ExtensionObject *messageSettings,
                       UA_ExtensionObject *transportSettings,
                       UA_NetworkMessage *networkMessage);

/* Add new publishCallback. The first execution is triggered directly after
 * creation. */
UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_Server *server, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Already registered */
    if(wg->publishCallbackId != 0)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(wg->config.pubsubManagerCallback.addCustomCallback) {
        /* Use configured mechanism for cyclic callbacks */
        retval = wg->config.pubsubManagerCallback.
                 addCustomCallback(server, wg->identifier,
                                   (UA_ServerCallback)UA_WriterGroup_publishCallback,
                                   wg, wg->config.publishingInterval,
                                   NULL, UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
                                   &wg->publishCallbackId);
    } else {
        /* Use EventLoop for cyclic callbacks */
        UA_EventLoop *el = UA_PubSubConnection_getEL(server, wg->linkedConnection);
        retval = el->addCyclicCallback(el, (UA_Callback)UA_WriterGroup_publishCallback,
                                       server, wg, wg->config.publishingInterval,
                                       NULL /* TODO: use basetime */,
                                       UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
                                       &wg->publishCallbackId);
    }
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Run once after creation. The Publish callback itself takes the server
     * mutex. So we release it first. */
    UA_WriterGroup_publishCallback(server, wg);
    return retval;
}

static void
UA_WriterGroup_removePublishCallback(UA_Server *server, UA_WriterGroup *wg) {
    if(wg->publishCallbackId == 0)
        return;
    if(wg->config.pubsubManagerCallback.removeCustomCallback) {
        wg->config.pubsubManagerCallback.
            removeCustomCallback(server, wg->identifier, wg->publishCallbackId);
    } else {
        UA_EventLoop *el = UA_PubSubConnection_getEL(server, wg->linkedConnection);
        el->removeCyclicCallback(el, wg->publishCallbackId);
    }
    wg->publishCallbackId = 0;
}

UA_StatusCode
UA_WriterGroup_create(UA_Server *server, const UA_NodeId connection,
                      const UA_WriterGroupConfig *writerGroupConfig,
                      UA_NodeId *writerGroupIdentifier) {
    /* Delete the reserved IDs if the related session no longer exists. */
    UA_PubSubManager_freeIds(server);
    if(!writerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *currentConnectionContext =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!currentConnectionContext)
        return UA_STATUSCODE_BADNOTFOUND;

    if(currentConnectionContext->configurationFreezeCounter > 0) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Adding WriterGroup failed. PubSubConnection is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

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

    memset(newWriterGroup, 0, sizeof(UA_WriterGroup));
    newWriterGroup->componentType = UA_PUBSUB_COMPONENT_WRITERGROUP;
    newWriterGroup->linkedConnection = currentConnectionContext;

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
    LIST_INSERT_HEAD(&currentConnectionContext->writerGroups, newWriterGroup, listEntry);
    currentConnectionContext->writerGroupsSize++;

    /* Add representation / create unique identifier */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    res = addWriterGroupRepresentation(server, newWriterGroup);
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newWriterGroup->identifier);
#endif

#ifdef UA_ENABLE_PUBSUB_SKS
    if(writerGroupConfig->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       writerGroupConfig->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_String_isEmpty(&writerGroupConfig->securityGroupId) &&
           writerGroupConfig->securityPolicy) {
            /* Does the key storage already exist? */
            newWriterGroup->keyStorage =
                UA_PubSubKeyStorage_findKeyStorage(server, writerGroupConfig->securityGroupId);

            if(!newWriterGroup->keyStorage) {
                /* Create a new key storage */
                newWriterGroup->keyStorage = (UA_PubSubKeyStorage *)
                    UA_calloc(1, sizeof(UA_PubSubKeyStorage));
                if(!newWriterGroup)
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                res = UA_PubSubKeyStorage_init(server, newWriterGroup->keyStorage,
                                               &writerGroupConfig->securityGroupId,
                                               writerGroupConfig->securityPolicy, 0, 0);
                if(res != UA_STATUSCODE_GOOD) {
                    UA_free(newWriterGroup);
                    return res;
                }
            }

            /* Increase the ref count */
            newWriterGroup->keyStorage->referenceCount++;
        }
    }

#endif

    if(writerGroupIdentifier)
        UA_NodeId_copy(&newWriterGroup->identifier, writerGroupIdentifier);

    /* Trigger the connection to open a connection */
    UA_PubSubConnection_setPubSubState(server, currentConnectionContext,
                                       currentConnectionContext->state,
                                       UA_STATUSCODE_GOOD);

    return res;
}

UA_StatusCode
UA_Server_addWriterGroup(UA_Server *server, const UA_NodeId connection,
                         const UA_WriterGroupConfig *writerGroupConfig,
                         UA_NodeId *writerGroupIdentifier) {
    lockServer(server);
    UA_StatusCode res = UA_WriterGroup_create(server, connection, writerGroupConfig,
                                              writerGroupIdentifier);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_WriterGroup_remove(UA_Server *server, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(wg->configurationFrozen) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                   "Deleting the WriterGroup failed. "
                                   "WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PubSubConnection *connection = wg->linkedConnection;
    UA_assert(connection);
    if(connection->configurationFreezeCounter > 0) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                   "Deleting the WriterGroup failed. "
                                   "PubSubConnection is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Disable (and disconnect) and set the deleteFlag. This prevents a
     * reconnect and triggers the deletion when the last open socket is
     * closed. */
    wg->deleteFlag = true;
    UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_DISABLED, UA_STATUSCODE_GOOD);

    UA_DataSetWriter *dsw, *dsw_tmp;
    LIST_FOREACH_SAFE(dsw, &wg->writers, listEntry, dsw_tmp) {
        UA_DataSetWriter_remove(server, dsw);
    }

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(wg->config.securityPolicy && wg->securityPolicyContext) {
        wg->config.securityPolicy->deleteContext(wg->securityPolicyContext);
        wg->securityPolicyContext = NULL;
    }
#endif

#ifdef UA_ENABLE_PUBSUB_SKS
    if(wg->keyStorage) {
        UA_PubSubKeyStorage_detachKeyStorage(server, wg->keyStorage);
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
        deleteNode(server, wg->identifier, true);
#endif
        UA_WriterGroupConfig_clear(&wg->config);
        UA_NodeId_clear(&wg->identifier);
        UA_NetworkMessageOffsetBuffer_clear(&wg->bufferedMessage);
        UA_free(wg);
    }

    /* Update the connection state */
    UA_PubSubConnection_setPubSubState(server, connection, connection->state,
                                       UA_STATUSCODE_GOOD);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeWriterGroup(UA_Server *server, const UA_NodeId writerGroup) {
    lockServer(server);
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_WriterGroup_remove(server, wg);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_WriterGroup_freezeConfiguration(UA_Server *server, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(wg->configurationFrozen)
        return UA_STATUSCODE_GOOD;

    /* Increase PubSubConnection freezeCounter */
    UA_PubSubConnection *pubSubConnection =  wg->linkedConnection;
    pubSubConnection->configurationFreezeCounter++;

    /* Freeze the WriterGroup */
    wg->configurationFrozen = true;

    /* Freeze the DataSetWriter */
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        UA_DataSetWriter_freezeConfiguration(server, dsw);
    }

    /* Offset table enabled? */
    if((wg->config.rtLevel & UA_PUBSUB_RT_FIXED_SIZE) == 0)
        return UA_STATUSCODE_GOOD;

    /* Offset table only possible for binary encoding */
    if(wg->config.encodingMimeType != UA_PUBSUB_ENCODING_UADP) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
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
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        res = UA_DataSetWriter_prepareDataSet(server, dsw, &dsmStore[dsmCount]);
        if(res != UA_STATUSCODE_GOOD)
            goto cleanup_dsm;
        dsmCount++;
    }

    /* Generate the NetworkMessage */
    memset(&networkMessage, 0, sizeof(networkMessage));
    res = generateNetworkMessage(pubSubConnection, wg, dsmStore, dsWriterIds,
                                 (UA_Byte) dsmCount, &wg->config.messageSettings,
                                 &wg->config.transportSettings, &networkMessage);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup_dsm;

    /* Compute the message length and generate the offset-table (done inside
     * calcSizeBinary) */
    memset(&wg->bufferedMessage, 0, sizeof(UA_NetworkMessageOffsetBuffer));
    msgSize = UA_NetworkMessage_calcSizeBinary(&networkMessage, &wg->bufferedMessage);
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        UA_PubSubSecurityPolicy *sp = wg->config.securityPolicy;
        msgSize += sp->symmetricModule.cryptoModule.
                   signatureAlgorithm.getLocalSignatureSize(sp->policyContext);
    }
#endif

    /* Generate the buffer for the pre-encoded message */
    res = UA_ByteString_allocBuffer(&buf, msgSize);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    wg->bufferedMessage.buffer = buf;

    /* Encode the NetworkMessage */
    bufEnd = &wg->bufferedMessage.buffer.data[wg->bufferedMessage.buffer.length];
    bufPos = wg->bufferedMessage.buffer.data;

    /* Preallocate the encryption buffer */
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        UA_Byte *payloadPosition;
        UA_NetworkMessage_encodeBinary(&networkMessage, &bufPos, bufEnd, &payloadPosition);
        wg->bufferedMessage.payloadPosition = payloadPosition;
        wg->bufferedMessage.nm = (UA_NetworkMessage *)UA_calloc(1,sizeof(UA_NetworkMessage));
        wg->bufferedMessage.nm->securityHeader = networkMessage.securityHeader;
        UA_ByteString_allocBuffer(&wg->bufferedMessage.encryptBuffer, msgSize);
    }
#endif

    if(wg->config.securityMode <= UA_MESSAGESECURITYMODE_NONE)
        UA_NetworkMessage_encodeBinary(&networkMessage, &bufPos, bufEnd, NULL);

    /* Post-processing of the OffsetBuffer to set the external data source from
     * the DataSetField configuration */
    if(wg->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS) {
        size_t fieldPos = 0;
        LIST_FOREACH(dsw, &wg->writers, listEntry) {
            UA_PublishedDataSet *pds =
                UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
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

UA_StatusCode
UA_Server_freezeWriterGroupConfiguration(UA_Server *server,
                                         const UA_NodeId writerGroup) {
    lockServer(server);
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_WriterGroup_freezeConfiguration(server, wg);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_WriterGroup_unfreezeConfiguration(UA_Server *server, UA_WriterGroup *wg) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Already unfrozen */
    if(!wg->configurationFrozen)
        return UA_STATUSCODE_GOOD;

    UA_PubSubConnection *pubSubConnection =  wg->linkedConnection;
    pubSubConnection->configurationFreezeCounter--;

    /* DataSetWriter unfreeze */
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        UA_DataSetWriter_unfreezeConfiguration(server, dsw);
    }

    UA_NetworkMessageOffsetBuffer_clear(&wg->bufferedMessage);
    wg->configurationFrozen = false;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_unfreezeWriterGroupConfiguration(UA_Server *server,
                                           const UA_NodeId writerGroup) {
    lockServer(server);
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_WriterGroup_unfreezeConfiguration(server, wg);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_setWriterGroupOperational(UA_Server *server,
                                    const UA_NodeId writerGroup) {
    lockServer(server);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(wg) {
#ifdef UA_ENABLE_PUBSUB_SKS
        if(wg->keyStorage && wg->keyStorage->currentItem) {
            res = UA_PubSubKeyStorage_activateKeyToChannelContext(
                server, wg->identifier, wg->config.securityGroupId);
            if(res != UA_STATUSCODE_GOOD) {
                unlockServer(server);
                return res;
            }
        }
#endif

        res = UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_OPERATIONAL,
                                            UA_STATUSCODE_GOOD);
    }
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_setWriterGroupDisabled(UA_Server *server,
                                 const UA_NodeId writerGroup) {
    lockServer(server);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(wg)
        res = UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_DISABLED,
                                            UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
    unlockServer(server);
    return res;
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
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    res |= UA_String_copy(&src->securityGroupId, &dst->securityGroupId);
#endif
    if(res != UA_STATUSCODE_GOOD)
        UA_WriterGroupConfig_clear(dst);
    return res;
}

UA_StatusCode
UA_Server_getWriterGroupConfig(UA_Server *server, const UA_NodeId writerGroup,
                               UA_WriterGroupConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_WriterGroup *currentWG = UA_WriterGroup_findWGbyId(server, writerGroup);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    if(currentWG)
        res = UA_WriterGroupConfig_copy(&currentWG->config, config);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_WriterGroup_updateConfig(UA_Server *server, UA_WriterGroup *wg,
                            const UA_WriterGroupConfig *config) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(wg->configurationFrozen){
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                   "Modify WriterGroup failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    //The update functionality will be extended during the next PubSub batches.
    //Currently is only a change of the publishing interval possible.
    if(wg->config.maxEncapsulatedDataSetMessageCount != config->maxEncapsulatedDataSetMessageCount) {
        wg->config.maxEncapsulatedDataSetMessageCount = config->maxEncapsulatedDataSetMessageCount;
        if(wg->config.messageSettings.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
            UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                       "MaxEncapsulatedDataSetMessag need enabled "
                                       "'PayloadHeader' within the message settings.");
        }
    }

    if(wg->config.publishingInterval != config->publishingInterval) {
        wg->config.publishingInterval = config->publishingInterval;
        if(wg->config.rtLevel == UA_PUBSUB_RT_NONE &&
           wg->state == UA_PUBSUBSTATE_OPERATIONAL) {
            UA_WriterGroup_removePublishCallback(server, wg);
            res = UA_WriterGroup_addPublishCallback(server, wg);
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                           "Modify WriterGroup failed. Adding publish callback failed"
                                           "with status code %s", UA_StatusCode_name(res));
                return res;
            }
        }
    }

    if(wg->config.priority != config->priority) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                   "Priority parameter is not yet "
                                   "supported for WriterGroup updates");
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_updateWriterGroupConfig(UA_Server *server, UA_NodeId writerGroupIdentifier,
                                  const UA_WriterGroupConfig *config) {
    lockServer(server);
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroupIdentifier);
    if(!wg) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_StatusCode res = UA_WriterGroup_updateConfig(server, wg, config);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_WriterGroup_getState(UA_Server *server, UA_NodeId writerGroupIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_WriterGroup *currentWriterGroup =
        UA_WriterGroup_findWGbyId(server, writerGroupIdentifier);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(currentWriterGroup) {
        *state = currentWriterGroup->state;
    } else {
        res = UA_STATUSCODE_BADNOTFOUND;
    }
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_WriterGroup_publish(UA_Server *server, const UA_NodeId writerGroupIdentifier){
    lockServer(server);

    //search WriterGroup ToDo create lookup table for more efficiency
    UA_WriterGroup *writerGroup;
    writerGroup = UA_WriterGroup_findWGbyId(server, writerGroupIdentifier);
    if(writerGroup == NULL){
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    unlockServer(server);
    UA_WriterGroup_publishCallback(server, writerGroup);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_WriterGroup_lastPublishTimestamp(UA_Server *server, const UA_NodeId writerGroupId,
                                    UA_DateTime *timestamp){
    lockServer(server);
    //search WriterGroup ToDo create lookup table for more efficiency
    UA_WriterGroup *writerGroup;
    writerGroup = UA_WriterGroup_findWGbyId(server, writerGroupId);
    if(writerGroup == NULL){
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    *timestamp = writerGroup->lastPublishTimeStamp;
    unlockServer(server);
    return UA_STATUSCODE_BADNOTFOUND;
}

UA_WriterGroup *
UA_WriterGroup_findWGbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PubSubConnection *tmpConnection;
    TAILQ_FOREACH(tmpConnection, &server->pubSubManager.connections, listEntry) {
        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &tmpConnection->writerGroups, listEntry) {
            if(UA_NodeId_equal(&identifier, &tmpWriterGroup->identifier))
                return tmpWriterGroup;
        }
    }
    return NULL;
}

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
UA_StatusCode
setWriterGroupEncryptionKeys(UA_Server *server, const UA_NodeId writerGroup,
                             UA_UInt32 securityTokenId,
                             const UA_ByteString signingKey,
                             const UA_ByteString encryptingKey,
                             const UA_ByteString keyNonce) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;
    if(wg->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                   "JSON encoding is enabled. The message security is only defined for the UADP message mapping.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(!wg->config.securityPolicy) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                   "No SecurityPolicy configured for the WriterGroup");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(securityTokenId != wg->securityTokenId) {
        wg->securityTokenId = securityTokenId;
        wg->nonceSequenceNumber = 1;
    }

    /* Create a new context */
    if(!wg->securityPolicyContext) {
        return wg->config.securityPolicy->
            newContext(wg->config.securityPolicy->policyContext,
                       &signingKey, &encryptingKey, &keyNonce,
                       &wg->securityPolicyContext);
    }

    /* Update the context */
    return wg->config.securityPolicy->
        setSecurityKeys(wg->securityPolicyContext, &signingKey, &encryptingKey, &keyNonce);
}

UA_StatusCode
UA_Server_setWriterGroupEncryptionKeys(UA_Server *server, const UA_NodeId writerGroup,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce) {
    lockServer(server);
    UA_StatusCode res = setWriterGroupEncryptionKeys(server, writerGroup, securityTokenId,
                                                     signingKey, encryptingKey, keyNonce);
    unlockServer(server);
    return res;
}
#endif

void
UA_WriterGroupConfig_clear(UA_WriterGroupConfig *writerGroupConfig) {
    UA_String_clear(&writerGroupConfig->name);
    UA_ExtensionObject_clear(&writerGroupConfig->transportSettings);
    UA_ExtensionObject_clear(&writerGroupConfig->messageSettings);
    UA_KeyValueMap_clear(&writerGroupConfig->groupProperties);
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_String_clear(&writerGroupConfig->securityGroupId);
#endif
    memset(writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
}

UA_StatusCode
UA_WriterGroup_setPubSubState(UA_Server *server, UA_WriterGroup *writerGroup,
                              UA_PubSubState state, UA_StatusCode cause) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(writerGroup->deleteFlag && state != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, writerGroup,
                                  "The WriterGroup is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_DataSetWriter *dataSetWriter;
    UA_PubSubState oldState = writerGroup->state;
    switch(state) {
        case UA_PUBSUBSTATE_DISABLED:
            UA_WriterGroup_disconnect(writerGroup);
            switch (writerGroup->state){
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL: {
                    UA_WriterGroup_removePublishCallback(server, writerGroup);
                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry) {
                        UA_DataSetWriter_setPubSubState(server, dataSetWriter,
                                                        UA_PUBSUBSTATE_DISABLED,
                                                        UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
                    }

                    writerGroup->state = UA_PUBSUBSTATE_DISABLED;
                    break;
                }
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING_WRITERGROUP(server->config.logging, writerGroup,
                                               "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_PAUSED:
            switch (writerGroup->state) {
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING_WRITERGROUP(server->config.logging, writerGroup,
                                               "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            switch (writerGroup->state) {
                case UA_PUBSUBSTATE_DISABLED: {
                    writerGroup->state = UA_PUBSUBSTATE_OPERATIONAL;
                    UA_WriterGroup_removePublishCallback(server, writerGroup);
                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry) {
                        UA_DataSetWriter_setPubSubState(server, dataSetWriter,
                                                        UA_PUBSUBSTATE_OPERATIONAL, cause);
                    }

                    ret = UA_WriterGroup_connect(server, writerGroup, false);
                    if(ret != UA_STATUSCODE_GOOD) {
                        UA_WriterGroup_setPubSubState(server, writerGroup,
                                                        UA_PUBSUBSTATE_ERROR, ret);
                        break;
                    }
                    ret = UA_WriterGroup_addPublishCallback(server, writerGroup);
                    if(ret != UA_STATUSCODE_GOOD) {
                        UA_WriterGroup_setPubSubState(server, writerGroup,
                                                        UA_PUBSUBSTATE_ERROR, ret);
                        break;
                    }
                    break;
                }
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING_WRITERGROUP(server->config.logging, writerGroup,
                                               "Received unknown PubSub state!");
            }

            UA_PubSubConnection *c = writerGroup->linkedConnection;
            ret = UA_PubSubConnection_setPubSubState(server, c,
                                                     UA_PUBSUBSTATE_OPERATIONAL,
                                                     UA_STATUSCODE_GOOD);
            if(ret != UA_STATUSCODE_GOOD ||
               (c->state != UA_PUBSUBSTATE_OPERATIONAL &&
                c->state != UA_PUBSUBSTATE_PREOPERATIONAL)) {
                UA_LOG_WARNING_WRITERGROUP(server->config.logging, writerGroup,
                                           "Connection not operational");
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            break;
        case UA_PUBSUBSTATE_ERROR: {
            switch (writerGroup->state){
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL: {
                    UA_WriterGroup_removePublishCallback(server, writerGroup);
                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                        UA_DataSetWriter_setPubSubState(server, dataSetWriter,
                                                        UA_PUBSUBSTATE_ERROR,
                                                        UA_STATUSCODE_GOOD);
                    }
                    break;
                }
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING_WRITERGROUP(server->config.logging, writerGroup,
                                    "Received unknown PubSub state!");
            }
            writerGroup->state = UA_PUBSUBSTATE_ERROR;
            break;
        }
        default:
            UA_LOG_WARNING_WRITERGROUP(server->config.logging, writerGroup,
                                       "Received unknown PubSub state!");
    }

    if(state != oldState) {
        /* inform application about state change */
        UA_ServerConfig *pConfig = &server->config;
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            pConfig->pubSubConfig.
                stateChangeCallback(server, &writerGroup->identifier, state, cause);
        }
    }
    return ret;
}

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
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
#endif

static UA_StatusCode
encodeNetworkMessage(UA_WriterGroup *wg, UA_NetworkMessage *nm,
                     UA_ByteString *buf) {
    UA_Byte *bufPos = buf->data;
    UA_Byte *bufEnd = &buf->data[buf->length];

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_Byte *networkMessageStart = bufPos;
#endif
    UA_StatusCode rv = UA_NetworkMessage_encodeHeaders(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_Byte *payloadStart = bufPos;
#endif
    rv = UA_NetworkMessage_encodePayload(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_encodeFooters(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    /* Encrypt and Sign the message */
    UA_Byte *footerEnd = bufPos;
    rv = encryptAndSign(wg, nm, networkMessageStart, payloadStart, footerEnd);
    UA_CHECK_STATUS(rv, return rv);
#endif

    return UA_STATUSCODE_GOOD;
}

static void
sendNetworkMessageBuffer(UA_Server *server, UA_WriterGroup *wg, 
                         UA_PubSubConnection *connection, uintptr_t connectionId,
                         UA_ByteString *buffer) {
    UA_StatusCode res = connection->cm->
        sendWithConnection(connection->cm, connectionId,
                           &UA_KEYVALUEMAP_NULL, buffer);

    /* Failure, set the WriterGroup into an error mode */
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "Sending NetworkMessage failed");
        UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_ERROR, res);
        UA_PubSubConnection_setPubSubState(server, connection, UA_PUBSUBSTATE_ERROR, res);
        return;
    }

    /* Sending successful - increase the sequence number */
    wg->sequenceNumber++;
}

#ifdef UA_ENABLE_JSON_ENCODING
static UA_StatusCode
sendNetworkMessageJson(UA_Server *server, UA_PubSubConnection *connection, UA_WriterGroup *wg,
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
    nm.publisherIdType = connection->config.publisherIdType;
    nm.publisherId = connection->config.publisherId;

    /* Compute the message length */
    size_t msgSize = UA_NetworkMessage_calcSizeJson(&nm, NULL, 0, NULL, 0, true);

    UA_ConnectionManager *cm = connection->cm;
    if(!cm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Select the wg sendchannel if configured */
    uintptr_t sendChannel = connection->sendChannel;
    if(wg->sendChannel != 0)
        sendChannel = wg->sendChannel;
    if(sendChannel == 0) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "Cannot send, no open connection");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Allocate the buffer */
    UA_ByteString buf;
    UA_StatusCode res = cm->allocNetworkBuffer(cm, sendChannel, &buf, msgSize);
    UA_CHECK_STATUS(res, return res);

    /* Encode the message */
    UA_Byte *bufPos = buf.data;
    const UA_Byte *bufEnd = &buf.data[msgSize];
    res = UA_NetworkMessage_encodeJson(&nm, &bufPos, &bufEnd, NULL, 0, NULL, 0, true);
    if(res != UA_STATUSCODE_GOOD) {
        cm->freeNetworkBuffer(cm, sendChannel, &buf);
        return res;
    }
    UA_assert(bufPos == bufEnd);

    /* Send the prepared messages */
    sendNetworkMessageBuffer(server, wg, connection, sendChannel, &buf);
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
generateNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                       UA_ExtensionObject *messageSettings,
                       UA_ExtensionObject *transportSettings,
                       UA_NetworkMessage *networkMessage) {
    if(messageSettings->content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
            messageSettings->content.decoded.data;

    networkMessage->publisherIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID) != 0;
    networkMessage->groupHeaderEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER) != 0;
    networkMessage->groupHeader.writerGroupIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID) != 0;
    networkMessage->groupHeader.groupVersionEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION) != 0;
    networkMessage->groupHeader.networkMessageNumberEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER) != 0;
    networkMessage->groupHeader.sequenceNumberEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER) != 0;
    networkMessage->payloadHeaderEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER) != 0;
    networkMessage->timestampEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_TIMESTAMP) != 0;
    networkMessage->picosecondsEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PICOSECONDS) != 0;
    networkMessage->dataSetClassIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID) != 0;
    networkMessage->promotedFieldsEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PROMOTEDFIELDS) != 0;

    /* Set the SecurityHeader */
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        networkMessage->securityEnabled = true;
        networkMessage->securityHeader.networkMessageSigned = true;
        if(wg->config.securityMode >= UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            networkMessage->securityHeader.networkMessageEncrypted = true;
        networkMessage->securityHeader.securityTokenId = wg->securityTokenId;

        /* Generate the MessageNonce. Four random bytes followed by a four-byte
         * sequence number */
        UA_ByteString nonce = {4, networkMessage->securityHeader.messageNonce};
        UA_StatusCode rv = wg->config.securityPolicy->symmetricModule.
            generateNonce(wg->config.securityPolicy->policyContext, &nonce);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
        UA_Byte *pos = &networkMessage->securityHeader.messageNonce[4];
        const UA_Byte *end = &networkMessage->securityHeader.messageNonce[8];
        UA_UInt32_encodeBinary(&wg->nonceSequenceNumber, &pos, end);
        networkMessage->securityHeader.messageNonceSize = 8;
    }
#endif

    networkMessage->version = 1;
    networkMessage->networkMessageType = UA_NETWORKMESSAGE_DATASET;
    networkMessage->publisherIdType = connection->config.publisherIdType;
    /* shallow copy of the PublisherId from connection configuration
        -> the configuration needs to be stable during publishing process
        -> it must not be cleaned after network message has been sent */
    networkMessage->publisherId = connection->config.publisherId;

    if(networkMessage->groupHeader.sequenceNumberEnabled)
        networkMessage->groupHeader.sequenceNumber = wg->sequenceNumber;

    if(networkMessage->groupHeader.groupVersionEnabled)
        networkMessage->groupHeader.groupVersion = wgm->groupVersion;

    /* Compute the length of the dsm separately for the header */
    UA_UInt16 *dsmLengths = (UA_UInt16 *) UA_calloc(dsmCount, sizeof(UA_UInt16));
    if(!dsmLengths)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(UA_Byte i = 0; i < dsmCount; i++)
        dsmLengths[i] = (UA_UInt16) UA_DataSetMessage_calcSizeBinary(&dsm[i], NULL, 0);

    networkMessage->payloadHeader.dataSetPayloadHeader.count = dsmCount;
    networkMessage->payloadHeader.dataSetPayloadHeader.dataSetWriterIds = writerIds;
    networkMessage->groupHeader.writerGroupId = wg->config.writerGroupId;
    /* number of the NetworkMessage inside a PublishingInterval */
    networkMessage->groupHeader.networkMessageNumber = 1;
    networkMessage->payload.dataSetPayload.sizes = dsmLengths;
    networkMessage->payload.dataSetPayload.dataSetMessages = dsm;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sendNetworkMessageBinary(UA_Server *server, UA_PubSubConnection *connection, UA_WriterGroup *wg,
                         UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount) {
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
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&nm, NULL);
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        UA_PubSubSecurityPolicy *sp = wg->config.securityPolicy;
        msgSize += sp->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(sp->policyContext);
    }
#endif

    UA_ConnectionManager *cm = connection->cm;
    if(!cm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Select the wg sendchannel if configured */
    uintptr_t sendChannel = connection->sendChannel;
    if(wg->sendChannel != 0)
        sendChannel = wg->sendChannel;
    if(sendChannel == 0) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "Cannot send, no open connection");
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
    sendNetworkMessageBuffer(server, wg, connection, sendChannel, &buf);

    UA_free(nm.payload.dataSetPayload.sizes);
    return UA_STATUSCODE_GOOD;
}

static void
sampleOffsetPublishingValues(UA_Server *server, UA_WriterGroup *wg) {
    lockServer(server);

    size_t fieldPos = 0;
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        UA_PublishedDataSet *pds =
            UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
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
            UA_PubSubDataSetField_sampleValue(server, dsf, dfv);
        }
    }

    unlockServer(server);
}

static void
publishWithOffsets(UA_Server *server, UA_WriterGroup *writerGroup,
                   UA_PubSubConnection *connection) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_assert(writerGroup->configurationFrozen);

    /* Fixed size but no direct value access. Sample to get recent values into
     * the offset buffer structure. */
    if((writerGroup->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS) == 0)
        sampleOffsetPublishingValues(server, writerGroup);

    UA_StatusCode res =
        UA_NetworkMessage_updateBufferedMessage(&writerGroup->bufferedMessage);

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_WRITERGROUP(server->config.logging, writerGroup,
                                 "PubSub sending. Unknown field type.");
        return;
    }

    UA_ByteString *buf = &writerGroup->bufferedMessage.buffer;

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    /* Send the encrypted buffered message if PubSub encryption is enabled */
    if(writerGroup->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        size_t sigSize = writerGroup->config.securityPolicy->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(writerGroup->securityPolicyContext);

        UA_Byte payloadOffset = (UA_Byte)(writerGroup->bufferedMessage.payloadPosition -
                                          writerGroup->bufferedMessage.buffer.data);
        memcpy(writerGroup->bufferedMessage.encryptBuffer.data,
               writerGroup->bufferedMessage.buffer.data,
               writerGroup->bufferedMessage.buffer.length);
        res = encryptAndSign(writerGroup, writerGroup->bufferedMessage.nm,
                             writerGroup->bufferedMessage.encryptBuffer.data,
                             writerGroup->bufferedMessage.encryptBuffer.data + payloadOffset,
                             writerGroup->bufferedMessage.encryptBuffer.data +
                                 writerGroup->bufferedMessage.encryptBuffer.length - sigSize);

        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_WRITERGROUP(server->config.logging, writerGroup,
                                     "PubSub Encryption failed");
            return;
        }

        buf = &writerGroup->bufferedMessage.encryptBuffer;
    }
#endif

    UA_ConnectionManager *cm = connection->cm;
    if(!cm)
        return;

    /* Select the wg sendchannel if configured */
    uintptr_t sendChannel = connection->sendChannel;
    if(writerGroup->sendChannel != 0)
        sendChannel = writerGroup->sendChannel;
    if(sendChannel == 0) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, writerGroup,
                                 "Cannot send, no open connection");
        return;
    }

    /* Copy into the network buffer */
    UA_ByteString outBuf;
    res = cm->allocNetworkBuffer(cm, sendChannel, &outBuf, buf->length);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, writerGroup,
                                 "PubSub message memory allocation failed");
        return;
    }
    memcpy(outBuf.data, buf->data, buf->length);
    sendNetworkMessageBuffer(server, writerGroup, connection, sendChannel, &outBuf);
}

static void
sendNetworkMessage(UA_Server *server, UA_WriterGroup *wg, UA_PubSubConnection *connection,
                   UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(wg->config.encodingMimeType) {
    case UA_PUBSUB_ENCODING_UADP:
        res = sendNetworkMessageBinary(server, connection, wg, dsm, writerIds, dsmCount);
        break;
#ifdef UA_ENABLE_JSON_ENCODING
    case UA_PUBSUB_ENCODING_JSON:
        res = sendNetworkMessageJson(server, connection, wg, dsm, writerIds, dsmCount);
        break;
#endif
    default:
        res = UA_STATUSCODE_BADNOTSUPPORTED;
        break;
    }

    /* If sending failed, disable all writer of the writergroup */
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                 "PubSub Publish: Could not send a NetworkMessage "
                                 "with status code %s", UA_StatusCode_name(res));
        UA_WriterGroup_setPubSubState(server, wg, UA_PUBSUBSTATE_ERROR, res);
    }
}

/* This callback triggers the collection and publish of NetworkMessages and the
 * contained DataSetMessages. */
void
UA_WriterGroup_publishCallback(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_assert(writerGroup != NULL);
    UA_assert(server != NULL);

    lockServer(server);

    UA_LOG_DEBUG_WRITERGROUP(server->config.logging, writerGroup, "Publish Callback");

    /* Find the connection associated with the writer */
    UA_PubSubConnection *connection = writerGroup->linkedConnection;
    if(!connection) {
        UA_LOG_ERROR_WRITERGROUP(server->config.logging, writerGroup,
                                 "Publish failed. PubSubConnection invalid");
        UA_WriterGroup_setPubSubState(server, writerGroup, UA_PUBSUBSTATE_ERROR,
                                      UA_STATUSCODE_BADNOTCONNECTED);
        unlockServer(server);
        return;
    }

    /* Realtime path - update the buffer message and send directly */
    if(writerGroup->config.rtLevel & UA_PUBSUB_RT_FIXED_SIZE) {
        publishWithOffsets(server, writerGroup, connection);
        unlockServer(server);
        return;
    }

    /* Nothing to do? */
    if(writerGroup->writersCount == 0) {
        unlockServer(server);
        return;
    }

    /* How many DSM can be sent in one NM? */
    UA_Byte maxDSM = (UA_Byte)writerGroup->config.maxEncapsulatedDataSetMessageCount;
    if(writerGroup->config.maxEncapsulatedDataSetMessageCount > UA_BYTE_MAX)
        maxDSM = UA_BYTE_MAX;
    if(maxDSM == 0)
        maxDSM = 1; /* Send at least one dsm */

    /* It is possible to put several DataSetMessages into one NetworkMessage.
     * But only if they do not contain promoted fields. NM with promoted fields
     * are sent out right away. The others are kept in a buffer for
     * "batching". */
    size_t dsmCount = 0;
    UA_STACKARRAY(UA_UInt16, dsWriterIds, writerGroup->writersCount);
    UA_STACKARRAY(UA_DataSetMessage, dsmStore, writerGroup->writersCount);

    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &writerGroup->writers, listEntry) {
        if(dsw->state != UA_PUBSUBSTATE_OPERATIONAL)
            continue;

        /* Heartbeats are send when no dataset is attached */
        UA_Boolean heartbeat = UA_NodeId_isNull(&dsw->connectedDataSet);
        UA_PublishedDataSet *pds = (heartbeat) ? NULL :
            UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
        if(!heartbeat && !pds) {
            UA_LOG_ERROR_WRITER(server->config.logging, dsw,
                                "PubSub Publish: PublishedDataSet not found");
            UA_DataSetWriter_setPubSubState(server, dsw, UA_PUBSUBSTATE_ERROR,
                                            UA_STATUSCODE_BADINTERNALERROR);
            continue;
        }

        /* Generate the DSM */
        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        UA_StatusCode res =
            UA_DataSetWriter_generateDataSetMessage(server, &dsmStore[dsmCount], dsw);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_WRITER(server->config.logging, dsw,
                         "PubSub Publish: DataSetMessage creation failed");
            UA_DataSetWriter_setPubSubState(server, dsw, UA_PUBSUBSTATE_ERROR, res);
            continue;
        }

        /* There is no promoted field -> send right away */
        if(pds && pds->promotedFieldsCount > 0) {
            writerGroup->lastPublishTimeStamp = UA_DateTime_nowMonotonic();
            sendNetworkMessage(server, writerGroup, connection, &dsmStore[dsmCount],
                               &dsWriterIds[dsmCount], 1);

            /* Clean up the current store entry */
            if(writerGroup->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS &&
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

    /* Send the NetworkMessages with batched DataSetMessages */
    UA_Byte nmDsmCount = 0;
    for(size_t i = 0; i < dsmCount; i += nmDsmCount) {
        /* How many dsm are batched in this iteration? */
        nmDsmCount = (i + maxDSM > dsmCount) ? (UA_Byte)(dsmCount - i) : maxDSM;

        writerGroup->lastPublishTimeStamp = UA_DateTime_nowMonotonic();
        /* Send the batched messages */
        sendNetworkMessage(server, writerGroup, connection, &dsmStore[i],
                           &dsWriterIds[i], nmDsmCount);
    }

    /* Clean up DSM */
    for(size_t i = 0; i < dsmCount; i++) {
        if(writerGroup->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS &&
           dsmStore[i].header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
            for(size_t j = 0; j < dsmStore[i].data.keyFrameData.fieldCount; ++j) {
                dsmStore[i].data.keyFrameData.dataSetFields[j].value.data = NULL;
            }
        }
        UA_DataSetMessage_clear(&dsmStore[i]);
    }

    unlockServer(server);
}

#endif /* UA_ENABLE_PUBSUB */
