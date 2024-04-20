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
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_networkmessage.h"
#include "ua_pubsub_ns0.h"
#endif

UA_ReaderGroup *
UA_ReaderGroup_findRGbyId(UA_Server *server, UA_NodeId identifier) {
    UA_ReaderGroup *rg;
    UA_PubSubConnection *psc;
    TAILQ_FOREACH(psc, &server->pubSubManager.connections, listEntry) {
        LIST_FOREACH(rg, &psc->readerGroups, listEntry) {
            if(UA_NodeId_equal(&identifier, &rg->identifier))
                return rg;
        }
    }
    return NULL;
}

UA_DataSetReader *
UA_ReaderGroup_findDSRbyId(UA_Server *server, UA_NodeId identifier) {
    UA_ReaderGroup *rg;
    UA_PubSubConnection *psc;
    UA_DataSetReader *tmpReader;
    TAILQ_FOREACH(psc, &server->pubSubManager.connections, listEntry) {
        LIST_FOREACH(rg, &psc->readerGroups, listEntry) {
            LIST_FOREACH(tmpReader, &rg->readers, listEntry) {
                if(UA_NodeId_equal(&tmpReader->identifier, &identifier))
                    return tmpReader;
            }
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
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    res = UA_String_copy(&src->securityGroupId, &dst->securityGroupId);
#endif
    if(res != UA_STATUSCODE_GOOD)
        UA_ReaderGroupConfig_clear(dst);
    return res;
}

void
UA_ReaderGroupConfig_clear(UA_ReaderGroupConfig *readerGroupConfig) {
    UA_String_clear(&readerGroupConfig->name);
    UA_KeyValueMap_clear(&readerGroupConfig->groupProperties);
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_String_clear(&readerGroupConfig->securityGroupId);
#endif
}

/* ReaderGroup Lifecycle */

UA_StatusCode
UA_ReaderGroup_create(UA_Server *server, UA_NodeId connectionId,
                      const UA_ReaderGroupConfig *rgc,
                      UA_NodeId *readerGroupId) {
    /* Check for valid readergroup configuration */
    if(!rgc)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, connectionId);
    if(!connection)
        return UA_STATUSCODE_BADNOTFOUND;

    if(connection->configurationFreezeCounter > 0) {
        UA_LOG_WARNING_CONNECTION(server->config.logging, connection,
                                  "Adding ReaderGroup failed. "
                                  "Connection configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Allocate memory for new reader group and add settings */
    UA_ReaderGroup *newGroup = (UA_ReaderGroup *)UA_calloc(1, sizeof(UA_ReaderGroup));
    if(!newGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newGroup->componentType = UA_PUBSUB_COMPONENT_READERGROUP;
    newGroup->linkedConnection = connection;

    /* Deep copy of the config */
    UA_StatusCode retval = UA_ReaderGroupConfig_copy(rgc, &newGroup->config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newGroup);
        return retval;
    }

    /* Add to the connection */
    LIST_INSERT_HEAD(&connection->readerGroups, newGroup, listEntry);
    connection->readerGroupsSize++;

#ifdef UA_ENABLE_PUBSUB_SKS
    if(rgc->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       rgc->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_String_isEmpty(&rgc->securityGroupId) && rgc->securityPolicy) {
            /* Does the key storage already exist? */
            newGroup->keyStorage =
                UA_PubSubKeyStorage_findKeyStorage(server, rgc->securityGroupId);

            if(!newGroup->keyStorage) {
                /* Create a new key storage */
                newGroup->keyStorage = (UA_PubSubKeyStorage *)
                    UA_calloc(1, sizeof(UA_PubSubKeyStorage));
                if(!newGroup->keyStorage) {
                    UA_ReaderGroup_remove(server, newGroup);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                retval = UA_PubSubKeyStorage_init(server, newGroup->keyStorage,
                                                  &rgc->securityGroupId,
                                                  rgc->securityPolicy, 0, 0);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_ReaderGroup_remove(server, newGroup);
                    return retval;
                }
            }

            /* Increase the ref count */
            newGroup->keyStorage->referenceCount++;
        }
    }
#endif

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retval |= addReaderGroupRepresentation(server, newGroup);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ReaderGroup_remove(server, newGroup);
        return retval;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newGroup->identifier);
#endif

    /* Cache the log string */
    UA_String idStr = UA_STRING_NULL;
    UA_NodeId_print(&newGroup->identifier, &idStr);
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "%.*sReaderGroup %.*s\t| ",
                (int)connection->logIdString.length,
                (char*)connection->logIdString.data,
                (int)idStr.length, idStr.data);
    newGroup->logIdString = UA_STRING_ALLOC(tmpLogIdStr);
    UA_String_clear(&idStr);

    UA_LOG_INFO_READERGROUP(server->config.logging, newGroup, "ReaderGroup created");

    /* Validate the connection settings */
    retval = UA_ReaderGroup_connect(server, newGroup, true);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READERGROUP(server->config.logging, newGroup,
                                 "Could not validate the connection parameters");
        UA_ReaderGroup_remove(server, newGroup);
        return retval;
    }

    /* Trigger the connection */
    UA_PubSubConnection_setPubSubState(server, connection, connection->state);

    /* Copying a numeric NodeId always succeeds */
    if(readerGroupId)
        UA_NodeId_copy(&newGroup->identifier, readerGroupId);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addReaderGroup(UA_Server *server, const UA_NodeId connectionIdentifier,
                         const UA_ReaderGroupConfig *readerGroupConfig,
                         UA_NodeId *readerGroupIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res =
        UA_ReaderGroup_create(server, connectionIdentifier,
                              readerGroupConfig, readerGroupIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_ReaderGroup_remove(UA_Server *server, UA_ReaderGroup *rg) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(rg->configurationFrozen) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "Remove ReaderGroup failed. "
                                   "Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PubSubConnection *connection = rg->linkedConnection;
    UA_assert(connection);
    if(connection->configurationFreezeCounter > 0) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "Deleting the ReaderGroup failed. "
                                   "PubSubConnection is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Disable (and disconnect) and set the deleteFlag. This prevents a
     * reconnect and triggers the deletion when the last open socket is
     * closed. */
    rg->deleteFlag = true;
    UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_DISABLED);

    UA_DataSetReader *dsr, *tmp_dsr;
    LIST_FOREACH_SAFE(dsr, &rg->readers, listEntry, tmp_dsr) {
        UA_DataSetReader_remove(server, dsr);
    }

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(rg->config.securityPolicy && rg->securityPolicyContext) {
        rg->config.securityPolicy->deleteContext(rg->securityPolicyContext);
        rg->securityPolicyContext = NULL;
    }
#endif

#ifdef UA_ENABLE_PUBSUB_SKS
    if(rg->keyStorage) {
        UA_PubSubKeyStorage_detachKeyStorage(server, rg->keyStorage);
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
        deleteNode(server, rg->identifier, true);
#endif

        UA_LOG_INFO_READERGROUP(server->config.logging, rg, "ReaderGroup deleted");

        UA_ReaderGroupConfig_clear(&rg->config);
        UA_NodeId_clear(&rg->identifier);
        UA_String_clear(&rg->logIdString);
        UA_free(rg);
    }

    /* Update the connection state */
    UA_PubSubConnection_setPubSubState(server, connection, connection->state);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeReaderGroup(UA_Server *server, const UA_NodeId groupIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, groupIdentifier);
    if(rg)
        res = UA_ReaderGroup_remove(server, rg);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_ReaderGroup_getConfig(UA_Server *server, const UA_NodeId readerGroupIdentifier,
                                UA_ReaderGroupConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK(&server->serviceMutex);

    /* Identify the readergroup through the readerGroupIdentifier */
    UA_ReaderGroup *currentReaderGroup =
        UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(!currentReaderGroup) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_StatusCode ret =
        UA_ReaderGroupConfig_copy(&currentReaderGroup->config, config);

    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_ReaderGroup_getState(UA_Server *server, const UA_NodeId readerGroupIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(rg) {
        *state = rg->state;
        ret = UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_ReaderGroup_setPubSubState(UA_Server *server, UA_ReaderGroup *rg,
                              UA_PubSubState targetState) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(rg->deleteFlag && targetState != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                  "The ReaderGroup is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubConnection *connection = rg->linkedConnection;
    UA_PubSubState oldState = rg->state;
    rg->state = targetState;

    switch(rg->state) {
        /* Disabled */
    default:
        rg->state = UA_PUBSUBSTATE_ERROR;
        ret = UA_STATUSCODE_BADINTERNALERROR;
        /* fallthrough */
    case UA_PUBSUBSTATE_DISABLED:
    case UA_PUBSUBSTATE_ERROR:
        UA_ReaderGroup_disconnect(rg);
        rg->hasReceived = false;
        break;

        /* Enabled */
    case UA_PUBSUBSTATE_PAUSED:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
    case UA_PUBSUBSTATE_OPERATIONAL:
        if(connection->state == UA_PUBSUBSTATE_DISABLED ||
           connection->state == UA_PUBSUBSTATE_ERROR) {
            /* Connection is disabled -> paused */
            rg->state = UA_PUBSUBSTATE_PAUSED;
        } else {
            /* Pre-operational until a message was received */
            rg->state = connection->state;
            if(rg->state == UA_PUBSUBSTATE_OPERATIONAL && !rg->hasReceived)
                rg->state = UA_PUBSUBSTATE_PREOPERATIONAL;

            /* Connect RG-specific connections. For example for MQTT. */
            ret = UA_ReaderGroup_connect(server, rg, false);
            if(ret != UA_STATUSCODE_GOOD)
                rg->state = UA_PUBSUBSTATE_ERROR;
        }
        break;
    }

    /* Inform application about state change */
    if(rg->state != oldState) {
        UA_ServerConfig *pConfig = &server->config;
        UA_LOG_INFO_READERGROUP(pConfig->logging, rg, "State change: %s -> %s",
                                UA_PubSubState_name(oldState),
                                UA_PubSubState_name(rg->state));
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            UA_UNLOCK(&server->serviceMutex);
            pConfig->pubSubConfig.
                stateChangeCallback(server, &rg->identifier, rg->state, ret);
            UA_LOCK(&server->serviceMutex);
        }
    }

    /* Update the attached DataSetReaders */
    UA_DataSetReader *dsr;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        UA_DataSetReader_setPubSubState(server, dsr, dsr->state);
    }

    return ret;
}

#ifdef UA_ENABLE_PUBSUB_SKS
UA_StatusCode
UA_Server_setReaderGroupActivateKey(UA_Server *server,
                                    const UA_NodeId readerGroupId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(rg) {
        if(rg->keyStorage && rg->keyStorage->currentItem) {
            UA_StatusCode retval = UA_PubSubKeyStorage_activateKeyToChannelContext(
                server, rg->identifier, rg->config.securityGroupId);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_UNLOCK(&server->serviceMutex);
                return retval;
            }
        }
    }
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}
#endif

UA_StatusCode
UA_Server_enableReaderGroup(UA_Server *server, const UA_NodeId readerGroupId){
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(rg)
        ret = UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_OPERATIONAL);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_disableReaderGroup(UA_Server *server, const UA_NodeId readerGroupId){
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(rg)
        ret = UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_DISABLED);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
UA_StatusCode
setReaderGroupEncryptionKeys(UA_Server *server, const UA_NodeId readerGroup,
                             UA_UInt32 securityTokenId,
                             const UA_ByteString signingKey,
                             const UA_ByteString encryptingKey,
                             const UA_ByteString keyNonce) {
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroup);
    UA_CHECK_MEM(rg, return UA_STATUSCODE_BADNOTFOUND);
    if(rg->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "JSON encoding is enabled. The message security is "
                                   "only defined for the UADP message mapping.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(!rg->config.securityPolicy) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
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

UA_StatusCode
UA_Server_setReaderGroupEncryptionKeys(UA_Server *server,
                                       const UA_NodeId readerGroup,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = setReaderGroupEncryptionKeys(server, readerGroup,
                                                     securityTokenId, signingKey,
                                                     encryptingKey, keyNonce);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}
#endif

/* Freezing of the configuration */

UA_StatusCode
UA_ReaderGroup_freezeConfiguration(UA_Server *server, UA_ReaderGroup *rg) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(rg->configurationFrozen)
        return UA_STATUSCODE_GOOD;

    /* PubSubConnection freezeCounter++ */
    UA_PubSubConnection *pubSubConnection = rg->linkedConnection;
    pubSubConnection->configurationFreezeCounter++;

    /* ReaderGroup freeze */
    /* TODO: Clarify on the freeze functionality in multiple DSR, multiple
     * networkMessage conf in a RG */
    rg->configurationFrozen = true;

    /* DataSetReader freeze */
    UA_DataSetReader *dsr;
    UA_UInt16 dsrCount = 0;
    LIST_FOREACH(dsr, &rg->readers, listEntry){
        dsr->configurationFrozen = true;
        dsrCount++;
        /* TODO: Configuration frozen for subscribedDataSet once
         * UA_Server_DataSetReader_addTargetVariables API modified to support
         * adding target variable one by one or in a group stored in a list. */
    }

    /* Not rt, we don't have to adjust anything */
    if(rg->config.rtLevel != UA_PUBSUB_RT_FIXED_SIZE)
        return UA_STATUSCODE_GOOD;

    if(dsrCount > 1) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "Mutiple DSR in a readerGroup not supported in RT "
                                   "fixed size configuration");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    dsr = LIST_FIRST(&rg->readers);

    /* Support only to UADP encoding */
    if(dsr->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE]) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "PubSub-RT configuration fail: Non-RT capable encoding.");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    /* Don't support string PublisherId for the fast-path (at this time) */
    if(dsr->config.publisherId.idType == UA_PUBLISHERIDTYPE_STRING) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "PubSub-RT configuration fail: String PublisherId");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    size_t fieldsSize = dsr->config.dataSetMetaData.fieldsSize;
    for(size_t i = 0; i < fieldsSize; i++) {
        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];
        const UA_VariableNode *rtNode = (const UA_VariableNode *)
            UA_NODESTORE_GET(server, &tv->targetVariable.targetNodeId);
        if(!rtNode ||
           rtNode->valueBackend.backendType != UA_VALUEBACKENDTYPE_EXTERNAL) {
            UA_LOG_WARNING_READER(server->config.logging, dsr,
                                  "PubSub-RT configuration fail: PDS contains field "
                                  "without external data source.");
            UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }

        /* Set the external data source in the tv */
        tv->externalDataValue = rtNode->valueBackend.backend.external.value;

        UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);

        UA_FieldMetaData *field = &dsr->config.dataSetMetaData.fields[i];
        if((UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_STRING].typeId) ||
            UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_BYTESTRING].typeId)) &&
           field->maxStringLength == 0) {
            UA_LOG_WARNING_READER(server->config.logging, dsr,
                                  "PubSub-RT configuration fail: "
                                  "PDS contains String/ByteString with dynamic length.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        } else if(!UA_DataType_isNumeric(UA_findDataType(&field->dataType)) &&
                  !UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_BOOLEAN].typeId)) {
            UA_LOG_WARNING_READER(server->config.logging, dsr,
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

    /* Set the current state again. This can move the state from Operational to
     * PreOperational. */
    return UA_ReaderGroup_setPubSubState(server, rg, rg->state);
}

UA_StatusCode
UA_Server_freezeReaderGroupConfiguration(UA_Server *server,
                                         const UA_NodeId readerGroupId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(rg)
        res = UA_ReaderGroup_freezeConfiguration(server, rg);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_ReaderGroup_unfreezeConfiguration(UA_Server *server, UA_ReaderGroup *rg) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Already unfrozen */
    if(!rg->configurationFrozen)
        return UA_STATUSCODE_GOOD;

    /* PubSubConnection freezeCounter-- */
    UA_PubSubConnection *pubSubConnection = rg->linkedConnection;
    pubSubConnection->configurationFreezeCounter--;

    /* ReaderGroup unfreeze */
    rg->configurationFrozen = false;

    /* DataSetReader unfreeze */
    UA_DataSetReader *dataSetReader;
    LIST_FOREACH(dataSetReader, &rg->readers, listEntry) {
        dataSetReader->configurationFrozen = false;
        UA_NetworkMessageOffsetBuffer_clear(&dataSetReader->bufferedMessage);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_unfreezeReaderGroupConfiguration(UA_Server *server,
                                           const UA_NodeId readerGroupId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(rg)
        res = UA_ReaderGroup_unfreezeConfiguration(server, rg);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_Boolean
UA_ReaderGroup_process(UA_Server *server, UA_ReaderGroup *readerGroup,
                       UA_NetworkMessage *nm) {
    /* Check if the ReaderGroup is enabled */
    if(readerGroup->state != UA_PUBSUBSTATE_OPERATIONAL &&
       readerGroup->state != UA_PUBSUBSTATE_PREOPERATIONAL)
        return false;

    readerGroup->hasReceived = true;
    if(readerGroup->state == UA_PUBSUBSTATE_PREOPERATIONAL)
        UA_ReaderGroup_setPubSubState(server, readerGroup, UA_PUBSUBSTATE_OPERATIONAL);

    /* Safe iteration. The current Reader might be deleted in the ReaderGroup
     * _setPubSubState callback. */
    UA_Boolean processed = false;
    UA_DataSetReader *reader, *reader_tmp;
    LIST_FOREACH_SAFE(reader, &readerGroup->readers, listEntry, reader_tmp) {
        UA_StatusCode res = UA_DataSetReader_checkIdentifier(server, nm, reader,
                                                             readerGroup->config);
        if(res != UA_STATUSCODE_GOOD)
            continue;

        /* Check if the reader is enabled */
        if(reader->state != UA_PUBSUBSTATE_OPERATIONAL &&
           reader->state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;

        /* Update the ReaderGroup state if this is the first received message */
        if(!readerGroup->hasReceived) {
            readerGroup->hasReceived = true;
            UA_ReaderGroup_setPubSubState(server, readerGroup, readerGroup->state);
        }

        /* The message was processed by at least one reader */
        processed = true;

        /* No payload header. The message ontains a single DataSetMessage that
         * is processed by every Reader. */
        if(!nm->payloadHeaderEnabled) {
            UA_DataSetReader_process(server, reader,
                                     nm->payload.dataSetPayload.dataSetMessages);
            continue;
        }

        /* Process only the payloads where the WriterId from the header is expected */
        UA_DataSetPayloadHeader *ph = &nm->payloadHeader.dataSetPayloadHeader;
        for(UA_Byte i = 0; i < ph->count; i++) {
            if(reader->config.dataSetWriterId == ph->dataSetWriterIds[i]) {
                UA_DataSetReader_process(server, reader,
                                         &nm->payload.dataSetPayload.dataSetMessages[i]);
            }
        }
    }

    return processed;
}

UA_Boolean
UA_ReaderGroup_decodeAndProcessRT(UA_Server *server, UA_ReaderGroup *rg,
                                  UA_ByteString *buf) {
    /* Received a (first) message for the ReaderGroup.
     * Transition from PreOperational to Operational. */
    rg->hasReceived = true;
    if(rg->state == UA_PUBSUBSTATE_PREOPERATIONAL)
        UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_OPERATIONAL);

    UA_Boolean processed = false;
    UA_NetworkMessage currentNetworkMessage;
    memset(&currentNetworkMessage, 0, sizeof(UA_NetworkMessage));

    /* Decode headers necessary for matching identifiers. This can use malloc.
     * So enable membufAlloc if you need RT timings. Reset back to the normal
     * malloc before processing the message. The userland (callbacks) below
     * might rely on that. It needs to be ensured that the membuf-memory is not
     * reset (zeroed out) in "useNormalAlloc". So the decoded memory can be used
     * until the end of this method. */
#ifdef UA_ENABLE_PUBSUB_BUFMALLOC
    useMembufAlloc();
#endif
    size_t pos = 0;
    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(buf, &pos, &currentNetworkMessage);
#ifdef UA_ENABLE_PUBSUB_BUFMALLOC
    useNormalAlloc();
#endif
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                              "PubSub receive. decoding headers failed");
        goto cleanup;
    }

    /* Decrypt the message. Keep pos right after the header. */
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    rv = verifyAndDecryptNetworkMessage(server->config.logging, buf, &pos,
                                        &currentNetworkMessage, rg);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "Subscribe failed. verify and decrypt network "
                                   "message failed.");
        goto cleanup;
    }
#endif

    /* Process the message for each reader */
    UA_DataSetReader *dsr;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        /* Check if the reader is enabled */
        if(dsr->state != UA_PUBSUBSTATE_OPERATIONAL &&
           dsr->state != UA_PUBSUBSTATE_PREOPERATIONAL)
            continue;

        /* Check the identifier */
        rv = UA_DataSetReader_checkIdentifier(server, &currentNetworkMessage,
                                              dsr, rg->config);
        if(rv != UA_STATUSCODE_GOOD) {
            UA_LOG_DEBUG_READER(server->config.logging, dsr,
                                "PubSub receive. Message intended for a different reader.");
            continue;
        }

        /* Process the message */
        UA_DataSetReader_decodeAndProcessRT(server, dsr, buf);
        processed = true;
    }

 cleanup:
#ifndef UA_ENABLE_PUBSUB_BUFMALLOC
    UA_NetworkMessage_clear(&currentNetworkMessage);
#endif
    return processed;
}

#endif /* UA_ENABLE_PUBSUB */
