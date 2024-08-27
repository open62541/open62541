/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/server_pubsub.h>
#include "ua_pubsub.h"
#include "../server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_SKS
#include "ua_pubsub_keystorage.h"
#endif

#define MALLOCMEMBUFSIZE 256

typedef struct {
    size_t pos;
    char buf[MALLOCMEMBUFSIZE];
} MembufCalloc;

static void *
membufCalloc(void *context,
             size_t nelem, size_t elsize) {
    if(nelem > MALLOCMEMBUFSIZE || elsize > MALLOCMEMBUFSIZE)
        return NULL;
    size_t total = nelem * elsize;

    MembufCalloc *mc = (MembufCalloc*)context;
    if(mc->pos + total > MALLOCMEMBUFSIZE)
        return NULL;
    void *mem = mc->buf + mc->pos;
    mc->pos += total;
    memset(mem, 0, total);
    return mem;
}

UA_ReaderGroup *
UA_ReaderGroup_find(UA_PubSubManager *psm, const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_PubSubConnection *psc;
    TAILQ_FOREACH(psc, &psm->connections, listEntry) {
        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &psc->readerGroups, listEntry) {
            if(UA_NodeId_equal(&id, &rg->head.identifier))
                return rg;
        }
    }
    return NULL;
}

/* ReaderGroup Config Handling */

UA_StatusCode
UA_ReaderGroupConfig_copy(const UA_ReaderGroupConfig *src,
                          UA_ReaderGroupConfig *dst) {
    memcpy(dst, src, sizeof(UA_ReaderGroupConfig));
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_KeyValueMap_copy(&src->groupProperties, &dst->groupProperties);
    res |= UA_String_copy(&src->securityGroupId, &dst->securityGroupId);
    if(res != UA_STATUSCODE_GOOD)
        UA_ReaderGroupConfig_clear(dst);
    return res;
}

void
UA_ReaderGroupConfig_clear(UA_ReaderGroupConfig *readerGroupConfig) {
    UA_String_clear(&readerGroupConfig->name);
    UA_KeyValueMap_clear(&readerGroupConfig->groupProperties);
    UA_String_clear(&readerGroupConfig->securityGroupId);
}

/* ReaderGroup Lifecycle */

UA_StatusCode
UA_ReaderGroup_create(UA_PubSubManager *psm, UA_NodeId connectionId,
                      const UA_ReaderGroupConfig *rgc,
                      UA_NodeId *readerGroupId) {
    /* Check for valid readergroup configuration */
    if(!psm || !rgc)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionId);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Allocate memory for new reader group and add settings */
    UA_ReaderGroup *newGroup = (UA_ReaderGroup *)UA_calloc(1, sizeof(UA_ReaderGroup));
    if(!newGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newGroup->head.componentType = UA_PUBSUBCOMPONENT_READERGROUP;
    newGroup->linkedConnection = c;

    /* Deep copy of the config */
    UA_StatusCode retval = UA_ReaderGroupConfig_copy(rgc, &newGroup->config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newGroup);
        return retval;
    }

    /* Add to the connection */
    LIST_INSERT_HEAD(&c->readerGroups, newGroup, listEntry);
    c->readerGroupsSize++;

#ifdef UA_ENABLE_PUBSUB_SKS
    if(rgc->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       rgc->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_String_isEmpty(&rgc->securityGroupId) && rgc->securityPolicy) {
            /* Does the key storage already exist? */
            newGroup->keyStorage = UA_PubSubKeyStorage_find(psm, rgc->securityGroupId);

            if(!newGroup->keyStorage) {
                /* Create a new key storage */
                newGroup->keyStorage = (UA_PubSubKeyStorage *)
                    UA_calloc(1, sizeof(UA_PubSubKeyStorage));
                if(!newGroup->keyStorage) {
                    UA_ReaderGroup_remove(psm, newGroup);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                retval = UA_PubSubKeyStorage_init(psm, newGroup->keyStorage,
                                                  &rgc->securityGroupId,
                                                  rgc->securityPolicy, 0, 0);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_ReaderGroup_remove(psm, newGroup);
                    return retval;
                }
            }

            /* Increase the ref count */
            newGroup->keyStorage->referenceCount++;
        }
    }
#endif

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retval |= addReaderGroupRepresentation(psm->sc.server, newGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReaderGroup_remove(psm, newGroup);
        return retval;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(psm, &newGroup->head.identifier);
#endif

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "%SReaderGroup %N\t| ",
                c->head.logIdString, newGroup->head.identifier);
    newGroup->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    UA_LOG_INFO_PUBSUB(psm->logging, newGroup, "ReaderGroup created");

    /* Validate the connection settings */
    retval = UA_ReaderGroup_connect(psm, newGroup, true);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, newGroup,
                            "Could not validate the connection parameters");
        UA_ReaderGroup_remove(psm, newGroup);
        return retval;
    }

    /* Trigger the connection */
    UA_PubSubConnection_setPubSubState(psm, c, c->head.state);

    /* Copying a numeric NodeId always succeeds */
    if(readerGroupId)
        UA_NodeId_copy(&newGroup->head.identifier, readerGroupId);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ReaderGroup_remove(UA_PubSubManager *psm, UA_ReaderGroup *rg) {
    if(!psm || !rg)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    if(UA_PubSubState_isEnabled(rg->head.state)) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "Deleting the ReaderGroup failed while still enabled");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PubSubConnection *connection = rg->linkedConnection;
    UA_assert(connection);

    /* Disable (and disconnect) and set the deleteFlag. This prevents a
     * reconnect and triggers the deletion when the last open socket is
     * closed. */
    rg->deleteFlag = true;
    UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED);

    UA_DataSetReader *dsr, *tmp_dsr;
    LIST_FOREACH_SAFE(dsr, &rg->readers, listEntry, tmp_dsr) {
        UA_DataSetReader_remove(psm, dsr);
    }

    if(rg->config.securityPolicy && rg->securityPolicyContext) {
        rg->config.securityPolicy->deleteContext(rg->securityPolicyContext);
        rg->securityPolicyContext = NULL;
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    if(rg->keyStorage) {
        UA_PubSubKeyStorage_detachKeyStorage(psm, rg->keyStorage);
        rg->keyStorage = NULL;
    }
#endif

    if(rg->recvChannelsSize == 0) {
        /* Unlink from the connection */
        LIST_REMOVE(rg, listEntry);
        connection->readerGroupsSize--;
        rg->linkedConnection = NULL;

        /* Actually remove the ReaderGroup */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
        deleteNode(psm->sc.server, rg->head.identifier, true);
#endif

        UA_LOG_INFO_PUBSUB(psm->logging, rg, "ReaderGroup deleted");

        UA_ReaderGroupConfig_clear(&rg->config);
        UA_PubSubComponentHead_clear(&rg->head);
        UA_free(rg);
    }

    /* Update the connection state */
    UA_PubSubConnection_setPubSubState(psm, connection, connection->head.state);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ReaderGroup_setPubSubState(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                              UA_PubSubState targetState) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    if(rg->deleteFlag && targetState != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "The ReaderGroup is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Are we doing a top-level state update or recursively? */
    UA_Boolean isTransient = rg->head.transientState;
    rg->head.transientState = true;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = rg->head.state;
    UA_PubSubConnection *connection = rg->linkedConnection;

    switch(targetState) {
        /* Disabled or Error */
    case UA_PUBSUBSTATE_DISABLED:
    case UA_PUBSUBSTATE_ERROR:
        rg->head.state = targetState;
        UA_ReaderGroup_unfreezeConfiguration(rg);
        UA_ReaderGroup_disconnect(rg);
        rg->hasReceived = false;
        break;

        /* Enabled */
    case UA_PUBSUBSTATE_PAUSED:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
    case UA_PUBSUBSTATE_OPERATIONAL:
        /* Freeze the configuration */
        ret = UA_ReaderGroup_freezeConfiguration(psm, rg);
        if(ret != UA_STATUSCODE_GOOD)
            break;

        if(psm->sc.state != UA_LIFECYCLESTATE_STARTED) {
            /* Avoid repeat warnings */
            if(oldState != UA_PUBSUBSTATE_PAUSED) {
                UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                                      "Cannot enable the ReaderGroup "
                                      "while the server is not running");
            }
            rg->head.state = UA_PUBSUBSTATE_PAUSED;
            UA_ReaderGroup_disconnect(rg);
            break;
        }

        /* Connection is not operational -> ReaderGroup paused */
        if(connection->head.state != UA_PUBSUBSTATE_OPERATIONAL) {
            UA_ReaderGroup_disconnect(rg);
            rg->head.state = UA_PUBSUBSTATE_PAUSED;
            break;
        }

        /* Connect RG-specific connections. For example for MQTT. */
        if(UA_ReaderGroup_canConnect(rg))
            ret = UA_ReaderGroup_connect(psm, rg, false);

        /* Preoperational until a message was received */
        rg->head.state = (rg->hasReceived) ?
            UA_PUBSUBSTATE_OPERATIONAL : UA_PUBSUBSTATE_PREOPERATIONAL;
        break;

        /* Unknown case */
    default:
        ret = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }

    /* Failure */
    if(ret != UA_STATUSCODE_GOOD) {
        rg->head.state = UA_PUBSUBSTATE_ERROR;
        UA_ReaderGroup_unfreezeConfiguration(rg);
        UA_ReaderGroup_disconnect(rg);
        rg->hasReceived = false;
    }

    /* Only the top-level state update (if recursive calls are happening)
     * notifies the application and updates Reader and WriterGroups */
    rg->head.transientState = isTransient;
    if(rg->head.transientState)
        return ret;

    /* Inform application about state change */
    if(rg->head.state != oldState) {
        UA_ServerConfig *pConfig = &psm->sc.server->config;
        UA_LOG_INFO_PUBSUB(psm->logging, rg, "State change: %s -> %s",
                           UA_PubSubState_name(oldState),
                           UA_PubSubState_name(rg->head.state));
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            UA_UNLOCK(&psm->sc.server->serviceMutex);
            pConfig->pubSubConfig.
                stateChangeCallback(psm->sc.server, rg->head.identifier, rg->head.state, ret);
            UA_LOCK(&psm->sc.server->serviceMutex);
        }
    }

    /* Update the attached DataSetReaders */
    UA_DataSetReader *dsr;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        UA_DataSetReader_setPubSubState(psm, dsr, dsr->head.state,
                                        UA_STATUSCODE_GOOD);
    }

    /* Update the PubSubManager state. It will go from STOPPING to STOPPED when
     * the last socket has closed. */
    UA_PubSubManager_setState(psm, psm->sc.state);

    return ret;
}

UA_StatusCode
UA_ReaderGroup_setEncryptionKeys(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                                 UA_UInt32 securityTokenId,
                                 const UA_ByteString signingKey,
                                 const UA_ByteString encryptingKey,
                                 const UA_ByteString keyNonce) {
    if(rg->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "JSON encoding is enabled. The message security is "
                              "only defined for the UADP message mapping.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(!rg->config.securityPolicy) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "No SecurityPolicy configured for the ReaderGroup");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(securityTokenId != rg->securityTokenId) {
        rg->securityTokenId = securityTokenId;
        rg->nonceSequenceNumber = 1;
    }

    /* Create a new context */
    if(!rg->securityPolicyContext) {
        return rg->config.securityPolicy->
            newContext(rg->config.securityPolicy->policyContext,
                       &signingKey, &encryptingKey, &keyNonce,
                       &rg->securityPolicyContext);
    }

    /* Update the context */
    return rg->config.securityPolicy->
        setSecurityKeys(rg->securityPolicyContext, &signingKey,
                        &encryptingKey, &keyNonce);
}

/* Freezing of the configuration */

UA_StatusCode
UA_ReaderGroup_freezeConfiguration(UA_PubSubManager *psm, UA_ReaderGroup *rg) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    if(rg->configurationFrozen)
        return UA_STATUSCODE_GOOD;

    /* ReaderGroup freeze */
    rg->configurationFrozen = true;

    /* Not rt, we don't have to adjust anything */
    if((rg->config.rtLevel & UA_PUBSUB_RT_FIXED_SIZE) == 0)
        return UA_STATUSCODE_GOOD;

    UA_DataSetReader *dsr;
    UA_UInt16 dsrCount = 0;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        dsrCount++;
    }
    if(dsrCount > 1) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "Multiple DSR in a readerGroup not supported in RT "
                              "fixed size configuration");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    dsr = LIST_FIRST(&rg->readers);

    /* Support only to UADP encoding */
    if(dsr->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE]) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "PubSub-RT configuration fail: Non-RT capable encoding.");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    /* Don't support string PublisherId for the fast-path (at this time) */
    if(dsr->config.publisherId.idType == UA_PUBLISHERIDTYPE_STRING) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "PubSub-RT configuration fail: String PublisherId");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    size_t fieldsSize = dsr->config.dataSetMetaData.fieldsSize;
    for(size_t i = 0; i < fieldsSize; i++) {
        /* TODO: Use the datasource from the node */
        /* UA_FieldTargetVariable *tv = */
        /*     &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i]; */
        /* const UA_VariableNode *rtNode = (const UA_VariableNode *) */
        /*     UA_NODESTORE_GET(server, &tv->targetVariable.targetNodeId); */
        /* if(!rtNode || */
        /*    rtNode->valueBackend.backendType != UA_VALUEBACKENDTYPE_EXTERNAL) { */
        /*     UA_LOG_WARNING_PUBSUB(server->config.logging, dsr, */
        /*                           "PubSub-RT configuration fail: PDS contains field " */
        /*                           "without external data source."); */
        /*     UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode); */
        /*     return UA_STATUSCODE_BADNOTSUPPORTED; */
        /* } */

        /* /\* Set the external data source in the tv *\/ */
        /* tv->externalDataValue = rtNode->valueBackend.backend.external.value; */

        /* UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode); */

        UA_FieldMetaData *field = &dsr->config.dataSetMetaData.fields[i];
        if((UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_STRING].typeId) ||
            UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_BYTESTRING].typeId)) &&
           field->maxStringLength == 0) {
            UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                                  "PubSub-RT configuration fail: "
                                  "PDS contains String/ByteString with dynamic length.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        } else if(!UA_DataType_isNumeric(UA_findDataType(&field->dataType)) &&
                  !UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_BOOLEAN].typeId)) {
            UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                                  "PubSub-RT configuration fail: "
                                  "PDS contains variable with dynamic size.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
    }

    /* Reset the OffsetBuffer. The OffsetBuffer for a frozen configuration is
     * generated when the first message is received. So we know the exact
     * settings which headers are present, etc. Until then the ReaderGroup is
     * "PreOperational". */
    UA_NetworkMessageOffsetBuffer_clear(&dsr->bufferedMessage);
    return UA_STATUSCODE_GOOD;
}

void
UA_ReaderGroup_unfreezeConfiguration(UA_ReaderGroup *rg) {
    if(!rg->configurationFrozen)
        return;
    rg->configurationFrozen = false;

    UA_DataSetReader *dataSetReader;
    LIST_FOREACH(dataSetReader, &rg->readers, listEntry) {
        UA_NetworkMessageOffsetBuffer_clear(&dataSetReader->bufferedMessage);
    }
}

UA_Boolean
UA_ReaderGroup_process(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                       UA_NetworkMessage *nm) {
    /* Check if the ReaderGroup is enabled */
    if(rg->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
       rg->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
        return false;

    /* Set to operational if required */
    rg->hasReceived = true;
    UA_ReaderGroup_setPubSubState(psm, rg, rg->head.state);

    /* Safe iteration. The current Reader might be deleted in the ReaderGroup
     * _setPubSubState callback. */
    UA_Boolean processed = false;
    UA_DataSetReader *reader, *reader_tmp;
    LIST_FOREACH_SAFE(reader, &rg->readers, listEntry, reader_tmp) {
        UA_StatusCode res = UA_DataSetReader_checkIdentifier(psm, nm,
                                                             reader, rg->config);
        if(res != UA_STATUSCODE_GOOD)
            continue;

        /* Check if the reader is enabled */
        if(reader->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
           reader->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;

        /* Update the ReaderGroup state if this is the first received message */
        if(!rg->hasReceived) {
            rg->hasReceived = true;
            UA_ReaderGroup_setPubSubState(psm, rg, rg->head.state);
        }

        /* The message was processed by at least one reader */
        processed = true;

        /* No payload header. The message ontains a single DataSetMessage that
         * is processed by every Reader. */
        if(!nm->payloadHeaderEnabled) {
            UA_DataSetReader_process(psm, reader,
                                     nm->payload.dataSetPayload.dataSetMessages);
            continue;
        }

        /* Process only the payloads where the WriterId from the header is expected */
        UA_DataSetPayloadHeader *ph = &nm->payloadHeader.dataSetPayloadHeader;
        for(UA_Byte i = 0; i < ph->count; i++) {
            if(reader->config.dataSetWriterId == ph->dataSetWriterIds[i]) {
                UA_DataSetReader_process(psm, reader,
                                         &nm->payload.dataSetPayload.dataSetMessages[i]);
            }
        }
    }

    return processed;
}

UA_Boolean
UA_ReaderGroup_decodeAndProcessRT(UA_PubSubManager *psm, UA_ReaderGroup *rg,
                                  UA_ByteString buf) {
    /* Received a (first) message for the ReaderGroup.
     * Transition from PreOperational to Operational. */
    rg->hasReceived = true;
    if(rg->head.state == UA_PUBSUBSTATE_PREOPERATIONAL)
        UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_OPERATIONAL);

    /* Set up the decoding context */
    Ctx ctx;
    ctx.pos = buf.data;
    ctx.end = buf.data + buf.length;
    ctx.depth = 0;
    memset(&ctx.opts, 0, sizeof(UA_DecodeBinaryOptions));
    ctx.opts.customTypes = psm->sc.server->config.customDataTypes;

    UA_Boolean processed = false;
    UA_NetworkMessage currentNetworkMessage;
    memset(&currentNetworkMessage, 0, sizeof(UA_NetworkMessage));

    /* Set the arena-based calloc for realtime processing. This is because the
     * header can (in theory) contain PromotedFields that require
     * heap-allocation.
     *
     * The arena is stack-allocated and rather small. So this method is
     * reentrant. Typically the arena-based calloc is not used at all for the
     * "static" network message headers encountered in an RT context. */
    MembufCalloc mc;
    mc.pos = 0;
    ctx.opts.callocContext = &mc;
    ctx.opts.calloc = membufCalloc;

    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(&ctx, &currentNetworkMessage);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "PubSub receive. decoding headers failed");
        return false;
    }

    /* Decrypt the message. Keep pos right after the header. */
    rv = verifyAndDecryptNetworkMessage(psm->logging, buf, &ctx,
                                        &currentNetworkMessage, rg);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg, "Subscribe failed. "
                              "Verify and decrypt network message failed.");
        return false;
    }

    /* Process the message for each reader */
    UA_DataSetReader *dsr;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        /* Check if the reader is enabled */
        if(dsr->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
           dsr->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;

        /* Check the identifier */
        rv = UA_DataSetReader_checkIdentifier(psm, &currentNetworkMessage,
                                              dsr, rg->config);
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG_PUBSUB(psm->logging, dsr,
                                "PubSub receive. Message intended for a different reader.");
            continue;
        }

        /* Process the message */
        UA_DataSetReader_decodeAndProcessRT(psm, dsr, buf);
        processed = true;
    }

    return processed;
}

/******************************/
/* Decrypt the NetworkMessage */
/******************************/

static UA_StatusCode
needsDecryption(const UA_Logger *logger,
                const UA_NetworkMessage *networkMessage,
                const UA_MessageSecurityMode securityMode,
                UA_Boolean *doDecrypt) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Boolean requiresEncryption = securityMode > UA_MESSAGESECURITYMODE_SIGN;
    UA_Boolean isEncrypted = networkMessage->securityHeader.networkMessageEncrypted;

    if(isEncrypted && requiresEncryption) {
        *doDecrypt = true;
    } else if(!isEncrypted && !requiresEncryption) {
        *doDecrypt = false;
    } else {
        if(isEncrypted) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is encrypted but ReaderGroup does not expect encryption");
            retval = UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
        } else {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is not encrypted but ReaderGroup requires encryption");
            retval = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        }
    }
    return retval;
}

static UA_StatusCode
needsValidation(const UA_Logger *logger,
                const UA_NetworkMessage *networkMessage,
                const UA_MessageSecurityMode securityMode,
                UA_Boolean *doValidate) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Boolean isSigned = networkMessage->securityHeader.networkMessageSigned;
    UA_Boolean requiresSignature = securityMode > UA_MESSAGESECURITYMODE_NONE;

    if(isSigned &&
       requiresSignature) {
        *doValidate = true;
    } else if(!isSigned && !requiresSignature) {
        *doValidate = false;
    } else {
        if(isSigned) {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is signed but ReaderGroup does not expect signatures");
            retval = UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;
        } else {
            UA_LOG_ERROR(logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. "
                         "Message is not signed but ReaderGroup requires signature");
            retval = UA_STATUSCODE_BADSECURITYMODEREJECTED;
        }
    }
    return retval;
}

UA_StatusCode
verifyAndDecryptNetworkMessage(const UA_Logger *logger, UA_ByteString buffer,
                               Ctx *ctx, UA_NetworkMessage *nm,
                               UA_ReaderGroup *rg) {
    UA_MessageSecurityMode securityMode = rg->config.securityMode;
    UA_Boolean doValidate = false;
    UA_Boolean doDecrypt = false;

    UA_StatusCode rv = needsValidation(logger, nm, securityMode, &doValidate);
    UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. Validation security mode error");

    rv = needsDecryption(logger, nm, securityMode, &doDecrypt);
    UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                         "PubSub receive. Decryption security mode error");

    if(!doValidate && !doDecrypt)
        return UA_STATUSCODE_GOOD;

    void *channelContext = rg->securityPolicyContext;
    UA_PubSubSecurityPolicy *securityPolicy = rg->config.securityPolicy;
    UA_CHECK_MEM_ERROR(channelContext, return UA_STATUSCODE_BADINVALIDARGUMENT,
                       logger, UA_LOGCATEGORY_PUBSUB,
                       "PubSub receive. securityPolicyContext must be initialized "
                       "when security mode is enabled to sign and/or encrypt");
    UA_CHECK_MEM_ERROR(securityPolicy, return UA_STATUSCODE_BADINVALIDARGUMENT,
                       logger, UA_LOGCATEGORY_PUBSUB,
                       "PubSub receive. securityPolicy must be set when security mode"
                       "is enabled to sign and/or encrypt");

    /* Validate the signature */
    if(doValidate) {
        size_t sigSize = securityPolicy->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(channelContext);
        UA_ByteString toBeVerified = {buffer.length - sigSize, buffer.data};
        UA_ByteString signature = {sigSize, buffer.data + buffer.length - sigSize};

        rv = securityPolicy->symmetricModule.cryptoModule.signatureAlgorithm.
            verify(channelContext, &toBeVerified, &signature);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Signature nvalid");

        /* Remove the signature from the ctx->end. We do not want to decode that. */
        ctx->end -= sigSize;
    }

    /* Decrypt the content */
    if(doDecrypt) {
        const UA_ByteString nonce = {
            (size_t)nm->securityHeader.messageNonceSize,
            (UA_Byte*)(uintptr_t)nm->securityHeader.messageNonce
        };
        rv = securityPolicy->setMessageNonce(channelContext, &nonce);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Faulty Nonce set");

        UA_ByteString toBeDecrypted = {(uintptr_t)(ctx->end - ctx->pos), ctx->pos};
        rv = securityPolicy->symmetricModule.cryptoModule
            .encryptionAlgorithm.decrypt(channelContext, &toBeDecrypted);
        UA_CHECK_STATUS_WARN(rv, return rv, logger, UA_LOGCATEGORY_SECURITYPOLICY,
                             "PubSub receive. Faulty Decryption");
    }

    return UA_STATUSCODE_GOOD;
}

/**************/
/* Server API */
/**************/

UA_StatusCode
UA_Server_addReaderGroup(UA_Server *server, const UA_NodeId connectionIdentifier,
                         const UA_ReaderGroupConfig *readerGroupConfig,
                         UA_NodeId *readerGroupIdentifier) {
    if(!server || !readerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res =
        UA_ReaderGroup_create(psm, connectionIdentifier,
                              readerGroupConfig, readerGroupIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_removeReaderGroup(UA_Server *server, const UA_NodeId groupIdentifier) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, groupIdentifier);
    UA_StatusCode res = (rg) ? UA_ReaderGroup_remove(psm, rg) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getReaderGroupConfig(UA_Server *server, const UA_NodeId rgId,
                               UA_ReaderGroupConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(getPSM(server), rgId);
    UA_StatusCode ret = (rg) ?
        UA_ReaderGroupConfig_copy(&rg->config, config) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_getReaderGroupState(UA_Server *server, const UA_NodeId rgId,
                              UA_PubSubState *state) {
    if(!server || !state)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_find(getPSM(server), rgId);
    if(rg) {
        *state = rg->head.state;
        ret = UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

#ifdef UA_ENABLE_PUBSUB_SKS
UA_StatusCode
UA_Server_setReaderGroupActivateKey(UA_Server *server,
                                    const UA_NodeId readerGroupId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupId);
    if(!rg || !rg->keyStorage || !rg->keyStorage->currentItem) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode ret =
        UA_PubSubKeyStorage_activateKeyToChannelContext(psm, rg->head.identifier,
                                                        rg->config.securityGroupId);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}
#endif

UA_StatusCode
UA_Server_enableReaderGroup(UA_Server *server, const UA_NodeId readerGroupId){
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupId);
    UA_StatusCode ret = (rg) ?
        UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_OPERATIONAL) :
        UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_disableReaderGroup(UA_Server *server, const UA_NodeId readerGroupId){
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupId);
    UA_StatusCode ret = (rg) ?
        UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED) :
        UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_setReaderGroupEncryptionKeys(UA_Server *server,
                                       const UA_NodeId readerGroup,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_ReaderGroup *rg = UA_ReaderGroup_find(getPSM(server), readerGroup);
    UA_StatusCode res = (rg) ?
        UA_ReaderGroup_setEncryptionKeys(psm, rg, securityTokenId, signingKey,
                                         encryptingKey, keyNonce) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
