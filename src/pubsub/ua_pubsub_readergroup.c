/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 *
 */

#include "ua_pubsub.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_networkmessage.h"
#include "ua_pubsub_ns0.h"
#endif

UA_ReaderGroup *
UA_ReaderGroup_findRGbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PubSubConnection *pubSubConnection;
    TAILQ_FOREACH(pubSubConnection, &server->pubSubManager.connections, listEntry){
        UA_ReaderGroup* readerGroup = NULL;
        LIST_FOREACH(readerGroup, &pubSubConnection->readerGroups, listEntry) {
            if(UA_NodeId_equal(&identifier, &readerGroup->identifier))
                return readerGroup;
        }
    }
    return NULL;
}

UA_DataSetReader *
UA_ReaderGroup_findDSRbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PubSubConnection *pubSubConnection;
    TAILQ_FOREACH(pubSubConnection, &server->pubSubManager.connections, listEntry){
        UA_ReaderGroup* readerGroup = NULL;
        LIST_FOREACH(readerGroup, &pubSubConnection->readerGroups, listEntry) {
            UA_DataSetReader *tmpReader;
            LIST_FOREACH(tmpReader, &readerGroup->readers, listEntry) {
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

    /* Deep copy of the config */
    UA_StatusCode retval = UA_ReaderGroupConfig_copy(rgc, &newGroup->config);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newGroup);
        return retval;
    }

    newGroup->linkedConnection = connection;

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
                if(!newGroup->keyStorage)
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                retval = UA_PubSubKeyStorage_init(server, newGroup->keyStorage,
                                                  &rgc->securityGroupId,
                                                  rgc->securityPolicy, 0, 0);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_ReaderGroupConfig_clear(&newGroup->config);
                    UA_free(newGroup);
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
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newGroup->identifier);
#endif


    if(readerGroupId)
        UA_NodeId_copy(&newGroup->identifier, readerGroupId);

    /* Trigger the connection to open a connection */
    UA_PubSubConnection_setPubSubState(server, connection,
                                       connection->state, UA_STATUSCODE_GOOD);

    /* If the connection is operational, we still reset the state. We might open
     * a different (recv) connection internally when a readergroup is
     * present. */
    if(connection->state == UA_PUBSUBSTATE_OPERATIONAL ||
       connection->state == UA_PUBSUBSTATE_PREOPERATIONAL)
        UA_PubSubConnection_setPubSubState(server, connection,
                                           UA_PUBSUBSTATE_OPERATIONAL, UA_STATUSCODE_GOOD);

    return retval;
}

UA_StatusCode
UA_Server_addReaderGroup(UA_Server *server, UA_NodeId connectionIdentifier,
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
    UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_DISABLED, UA_STATUSCODE_GOOD);

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
        UA_ReaderGroupConfig_clear(&rg->config);
        UA_NodeId_clear(&rg->identifier);
        UA_free(rg);
    }

    /* Update the connection state */
    UA_PubSubConnection_setPubSubState(server, connection, connection->state,
                                       UA_STATUSCODE_GOOD);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeReaderGroup(UA_Server *server, UA_NodeId groupIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_ReaderGroup* readerGroup =
        UA_ReaderGroup_findRGbyId(server, groupIdentifier);
    if(!readerGroup) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_ReaderGroup_remove(server, readerGroup);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_ReaderGroup_getConfig(UA_Server *server, UA_NodeId readerGroupIdentifier,
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
UA_Server_ReaderGroup_getState(UA_Server *server, UA_NodeId readerGroupIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg =
        UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(rg) {
        *state = rg->state;
        ret = UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

/* ReaderGroup State */

static UA_StatusCode
UA_ReaderGroup_setPubSubState_disable(UA_Server *server,
                                      UA_ReaderGroup *rg,
                                      UA_StatusCode cause) {
    /* Disconnect if not already done */
    UA_ReaderGroup_disconnect(rg);

    UA_DataSetReader *dataSetReader;
    switch(rg->state) {
    case UA_PUBSUBSTATE_DISABLED:
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_PAUSED:
        break;
    case UA_PUBSUBSTATE_OPERATIONAL:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
        LIST_FOREACH(dataSetReader, &rg->readers, listEntry) {
            UA_DataSetReader_setPubSubState(server, dataSetReader,
                                            UA_PUBSUBSTATE_DISABLED, cause);
        }
        rg->state = UA_PUBSUBSTATE_DISABLED;
        break;
    case UA_PUBSUBSTATE_ERROR:
        break;
    default:
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "Unknown PubSub state!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ReaderGroup_setPubSubState_paused(UA_Server *server,
                                     UA_ReaderGroup *rg,
                                     UA_StatusCode cause) {
    UA_LOG_DEBUG_READERGROUP(server->config.logging, rg,
                             "PubSub state paused is unsupported at the moment!");
    (void)cause;
    switch(rg->state) {
    case UA_PUBSUBSTATE_DISABLED:
        break;
    case UA_PUBSUBSTATE_PAUSED:
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_OPERATIONAL:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
        break;
    case UA_PUBSUBSTATE_ERROR:
        break;
    default:
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg, "Unknown PubSub state!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

static UA_StatusCode
UA_ReaderGroup_setPubSubState_operational(UA_Server *server,
                                          UA_ReaderGroup *rg,
                                          UA_StatusCode cause) {
    UA_PubSubConnection *pubSubConnection = rg->linkedConnection;
    UA_StatusCode ret =
        UA_PubSubConnection_setPubSubState(server, pubSubConnection,
                                           UA_PUBSUBSTATE_OPERATIONAL,
                                           UA_STATUSCODE_GOOD);
    if(ret != UA_STATUSCODE_GOOD ||
       (pubSubConnection->state != UA_PUBSUBSTATE_OPERATIONAL &&
        pubSubConnection->state != UA_PUBSUBSTATE_PREOPERATIONAL)) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "Connection not operational");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Connect if the ReaderGroup has dedicated connections */
    if(rg->recvChannelsSize == 0)
        ret = UA_ReaderGroup_connect(server, rg, false);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READERGROUP(server->config.logging, rg, "Could not connect");
        UA_PubSubConnection_setPubSubState(server, pubSubConnection,
                                           UA_PUBSUBSTATE_ERROR, ret);
    }

    /* Set to preoperational until the first message was received */
    if(rg->state != UA_PUBSUBSTATE_OPERATIONAL)
        rg->state = UA_PUBSUBSTATE_PREOPERATIONAL;

    /* Set all readers operational */
    UA_DataSetReader *dsr;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        UA_DataSetReader_setPubSubState(server, dsr,
                                        UA_PUBSUBSTATE_OPERATIONAL, cause);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ReaderGroup_setPubSubState_error(UA_Server *server,
                                    UA_ReaderGroup *rg,
                                    UA_StatusCode cause) {
    UA_DataSetReader *dataSetReader;
    switch(rg->state) {
    case UA_PUBSUBSTATE_DISABLED:
        break;
    case UA_PUBSUBSTATE_PAUSED:
        break;
    case UA_PUBSUBSTATE_OPERATIONAL:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
        LIST_FOREACH(dataSetReader, &rg->readers, listEntry){
            UA_DataSetReader_setPubSubState(server, dataSetReader, UA_PUBSUBSTATE_ERROR,
                                            cause);
        }
        break;
    case UA_PUBSUBSTATE_ERROR:
        return UA_STATUSCODE_GOOD;
    default:
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg, "Unknown PubSub state!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    rg->state = UA_PUBSUBSTATE_ERROR;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ReaderGroup_setPubSubState(UA_Server *server,
                              UA_ReaderGroup *readerGroup,
                              UA_PubSubState state,
                              UA_StatusCode cause) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(readerGroup->deleteFlag && state != UA_PUBSUBSTATE_DISABLED) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, readerGroup,
                                  "The ReaderGroup is being deleted. Can only be disabled.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode ret = UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_PubSubState oldState = readerGroup->state;
    switch(state) {
        case UA_PUBSUBSTATE_DISABLED:
            ret = UA_ReaderGroup_setPubSubState_disable(server, readerGroup, cause);
            break;
        case UA_PUBSUBSTATE_PAUSED:
            ret = UA_ReaderGroup_setPubSubState_paused(server, readerGroup, cause);
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            ret = UA_ReaderGroup_setPubSubState_operational(server, readerGroup, cause);
            break;
        case UA_PUBSUBSTATE_ERROR:
            ret = UA_ReaderGroup_setPubSubState_error(server, readerGroup, cause);
            break;
        default:
            UA_LOG_WARNING_READERGROUP(server->config.logging, readerGroup,
                                       "Received unknown PubSub state!");
            break;
    }

    /* inform application about state change */
    if(readerGroup->state != oldState) {
        UA_ServerConfig *pConfig = &server->config;
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            pConfig->pubSubConfig.
                stateChangeCallback(server, &readerGroup->identifier,
                                    readerGroup->state, cause);
        }
    }
    return ret;
}

UA_StatusCode
UA_Server_setReaderGroupOperational(UA_Server *server,
                                    const UA_NodeId readerGroupId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(rg) {
#ifdef UA_ENABLE_PUBSUB_SKS
        if(rg->keyStorage && rg->keyStorage->currentItem) {
            UA_StatusCode retval = UA_PubSubKeyStorage_activateKeyToChannelContext(
                server, rg->identifier, rg->config.securityGroupId);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_UNLOCK(&server->serviceMutex);
                return retval;
            }
        }
#endif
        ret = UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_OPERATIONAL,
                                            UA_STATUSCODE_GOOD);
    }
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_setReaderGroupDisabled(UA_Server *server,
                                 const UA_NodeId readerGroupId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(rg)
        ret = UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_DISABLED,
                                            UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
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
    if((rg->config.rtLevel & UA_PUBSUB_RT_FIXED_SIZE) == 0)
        return UA_STATUSCODE_GOOD;

    if(dsrCount > 1) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, rg,
                                   "Multiple DSR in a readerGroup not supported in RT "
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
    if(!dsr->config.publisherId.type->pointerFree) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "PubSub-RT configuration fail: String PublisherId");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    size_t fieldsSize = dsr->config.dataSetMetaData.fieldsSize;
    UA_Boolean externalDataValueSet = false;
    for(size_t i = 0; i < fieldsSize; i++) {
        /* Use the datasource from the node, if available and no externalDataValue
           has been set in the configuration */
        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];
        if(tv->externalDataValue == NULL) {
            const UA_VariableNode *rtNode = (const UA_VariableNode *)
                UA_NODESTORE_GET(server, &tv->targetVariable.targetNodeId);
            if(rtNode &&
               rtNode->valueBackend.backendType == UA_VALUEBACKENDTYPE_EXTERNAL) {
                /* Set the external data source in the tv */
                tv->externalDataValue = rtNode->valueBackend.backend.external.value;
                externalDataValueSet = true;
            }

            UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
        }

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
    if (externalDataValueSet) {
        UA_LOG_INFO_READER(server->config.logging, dsr,
                           "PubSub-RT configuration: Take externalDataValue from "
                           "external value backend for at least one configured target "
                           "variable.");
    }

    /* Reset the OffsetBuffer. The OffsetBuffer for a frozen configuration is
     * generated when the first message is received. So we know the exact
     * settings which headers are present, etc. Until then the ReaderGroup is
     * "PreOperational". */
    UA_NetworkMessageOffsetBuffer_clear(&dsr->bufferedMessage);

    /* Set the current state again. This can move the state from Operational to
     * PreOperational. */
    return UA_ReaderGroup_setPubSubState(server, rg, rg->state, UA_STATUSCODE_GOOD);
}

UA_StatusCode
UA_Server_freezeReaderGroupConfiguration(UA_Server *server,
                                         const UA_NodeId readerGroupId) {
    UA_LOCK(&server->serviceMutex);
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(!rg) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_ReaderGroup_freezeConfiguration(server, rg);
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
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(!rg) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_ReaderGroup_unfreezeConfiguration(server, rg);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
