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

UA_DataSetReader *UA_ReaderGroup_findDSRbyId(UA_Server *server, UA_NodeId identifier) {
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

/* Clear ReaderGroup */
static void
UA_Server_ReaderGroup_clear(UA_Server* server, UA_ReaderGroup *readerGroup);

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
UA_Server_addReaderGroup(UA_Server *server, UA_NodeId connectionIdentifier,
                         const UA_ReaderGroupConfig *readerGroupConfig,
                         UA_NodeId *readerGroupIdentifier) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Check for valid readergroup configuration */
    if(!readerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    if(!connection)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!readerGroupConfig->pubsubManagerCallback.addCustomCallback &&
       readerGroupConfig->enableBlockingSocket) {
        UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                  "Adding ReaderGroup failed, blocking socket functionality "
                                  "only supported in customcallback");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    if(connection->configurationFrozen) {
        UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                  "Adding ReaderGroup failed. "
                                  "Connection configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Allocate memory for new reader group and add settings */
    UA_ReaderGroup *newGroup = (UA_ReaderGroup *)UA_calloc(1, sizeof(UA_ReaderGroup));
    if(!newGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newGroup->componentType = UA_PUBSUB_COMPONENT_READERGROUP;
    retval |= UA_NodeId_copy(&connection->identifier, &newGroup->linkedConnection);
    retval |= UA_ReaderGroupConfig_copy(readerGroupConfig, &newGroup->config);

    /* Check user configured params and define it accordingly */
    if(newGroup->config.subscribingInterval <= 0.0)
        newGroup->config.subscribingInterval = 5; // Set default to 5 ms

    if(newGroup->config.enableBlockingSocket)
        newGroup->config.timeout = 0; // Set timeout to 0 for blocking socket

    if((!newGroup->config.enableBlockingSocket) && (!newGroup->config.timeout))
        newGroup->config.timeout = 1000; /* Set default to 1ms socket timeout
                                          * when non-blocking socket allows with
                                          * zero timeout */

    /* Add to the connection */
    LIST_INSERT_HEAD(&connection->readerGroups, newGroup, listEntry);
    connection->readerGroupsSize++;

    /* If the connection is operational, we still might need to start actively
     * listen on the network. */
    if(connection->state == UA_PUBSUBSTATE_OPERATIONAL)
        UA_PubSubConnection_setPubSubState(server, connection,
                                           UA_PUBSUBSTATE_OPERATIONAL, UA_STATUSCODE_GOOD);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retval |= addReaderGroupRepresentation(server, newGroup);
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newGroup->identifier);
#endif

#ifdef UA_ENABLE_PUBSUB_SKS
    if(readerGroupConfig->securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       readerGroupConfig->securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        if(!UA_String_isEmpty(&readerGroupConfig->securityGroupId) &&
           readerGroupConfig->securityPolicy) {
            UA_String policyUri = readerGroupConfig->securityPolicy->policyUri;

            newGroup->keyStorage = UA_Server_findKeyStorage(
                server, readerGroupConfig->securityGroupId);
            if(!newGroup->keyStorage) {
                newGroup->keyStorage = (UA_PubSubKeyStorage *)UA_calloc(1, sizeof(UA_PubSubKeyStorage));
                if(!newGroup->keyStorage)
                    return UA_STATUSCODE_BADOUTOFMEMORY;
            }

            retval = UA_PubSubKeyStorage_init(server, &readerGroupConfig->securityGroupId,
                                              &policyUri, 0, 0, newGroup->keyStorage);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_free(newGroup);
                return retval;
            }
        }
    }

#endif

    if(readerGroupIdentifier)
        UA_NodeId_copy(&newGroup->identifier, readerGroupIdentifier);

    /* Set the assigment between ReaderGroup and Topic if the transport layer is MQTT. */
    const UA_String transport_uri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt");
    if(UA_String_equal(&connection->config.transportProfileUri, &transport_uri)) {
        UA_String topic = ((UA_BrokerWriterGroupTransportDataType *)readerGroupConfig->transportSettings.content.decoded.data)->queueName;
        retval |= UA_PubSubManager_addPubSubTopicAssign(server, newGroup, topic);
    }
    return retval;
}

UA_StatusCode
removeReaderGroup(UA_Server *server, UA_NodeId groupIdentifier) {
    UA_ReaderGroup* readerGroup =
        UA_ReaderGroup_findRGbyId(server, groupIdentifier);
    if(readerGroup == NULL)
        return UA_STATUSCODE_BADNOTFOUND;

    if(readerGroup->configurationFrozen){
        UA_LOG_WARNING_READERGROUP(&server->config.logger, readerGroup,
                                   "Remove ReaderGroup failed. "
                                   "Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Search the connection to which the given readergroup is connected to */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, readerGroup->linkedConnection);
    if(connection == NULL)
        return UA_STATUSCODE_BADNOTFOUND;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeReaderGroupRepresentation(server, readerGroup);
#endif

    /* UA_Server_ReaderGroup_clear also removes itself from the list */
    UA_Server_ReaderGroup_clear(server, readerGroup);

    /* Remove readerGroup from Connection */
    LIST_REMOVE(readerGroup, listEntry);
    UA_free(readerGroup);

    UA_PubSubConnection_setPubSubState(server, connection, connection->state, UA_STATUSCODE_GOOD);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeReaderGroup(UA_Server *server, UA_NodeId groupIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = removeReaderGroup(server, groupIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

/* TODO: Implement
UA_StatusCode
UA_Server_ReaderGroup_updateConfig(UA_Server *server, UA_NodeId readerGroupIdentifier,
                                   const UA_ReaderGroupConfig *config) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}
*/

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

static void
UA_Server_ReaderGroup_clear(UA_Server* server, UA_ReaderGroup *readerGroup) {
    UA_ReaderGroupConfig_clear(&readerGroup->config);
    UA_DataSetReader *dataSetReader;
    UA_DataSetReader *tmpDataSetReader;
    LIST_FOREACH_SAFE(dataSetReader, &readerGroup->readers, listEntry, tmpDataSetReader) {
        removeDataSetReader(server, dataSetReader->identifier);
    }
    UA_PubSubConnection* pConn =
        UA_PubSubConnection_findConnectionbyId(server, readerGroup->linkedConnection);
    if(pConn != NULL)
        pConn->readerGroupsSize--;

    /* Delete ReaderGroup and its members */
    UA_NodeId_clear(&readerGroup->linkedConnection);
    UA_NodeId_clear(&readerGroup->identifier);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(readerGroup->config.securityPolicy && readerGroup->securityPolicyContext) {
        readerGroup->config.securityPolicy->deleteContext(readerGroup->securityPolicyContext);
        readerGroup->securityPolicyContext = NULL;
    }
#endif

#ifdef UA_ENABLE_PUBSUB_SKS
    if(readerGroup->config.securityMode == UA_MESSAGESECURITYMODE_SIGN ||
       readerGroup->config.securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT) {
        UA_PubSubKeyStorage_removeKeyStorage(server, readerGroup->keyStorage);
        readerGroup->keyStorage = NULL;
    }

#endif

    UA_ReaderGroupConfig_clear(&readerGroup->config);
}

UA_StatusCode
UA_Server_ReaderGroup_getState(UA_Server *server, UA_NodeId readerGroupIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ReaderGroup *currentReaderGroup =
        UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(!currentReaderGroup)
        return UA_STATUSCODE_BADNOTFOUND;
    *state = currentReaderGroup->state;
    return UA_STATUSCODE_GOOD;
}

/* ReaderGroup State */

static UA_StatusCode
UA_ReaderGroup_setPubSubState_disable(UA_Server *server,
                                      UA_ReaderGroup *rg,
                                      UA_StatusCode cause) {
    UA_DataSetReader *dataSetReader;
    switch(rg->state) {
    case UA_PUBSUBSTATE_DISABLED:
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_PAUSED:
        break;
    case UA_PUBSUBSTATE_OPERATIONAL:
        LIST_FOREACH(dataSetReader, &rg->readers, listEntry) {
            UA_DataSetReader_setPubSubState(server, dataSetReader,
                                            UA_PUBSUBSTATE_DISABLED, cause);
        }
        rg->state = UA_PUBSUBSTATE_DISABLED;
        break;
    case UA_PUBSUBSTATE_ERROR:
        break;
    default:
        UA_LOG_WARNING_READERGROUP(&server->config.logger, rg,
                                   "Unknown PubSub state!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ReaderGroup_setPubSubState_paused(UA_Server *server,
                                     UA_ReaderGroup *rg,
                                     UA_StatusCode cause) {
    UA_LOG_DEBUG_READERGROUP(&server->config.logger, rg,
                             "PubSub state paused is unsupported at the moment!");
    (void)cause;
    switch(rg->state) {
    case UA_PUBSUBSTATE_DISABLED:
        break;
    case UA_PUBSUBSTATE_PAUSED:
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_OPERATIONAL:
        break;
    case UA_PUBSUBSTATE_ERROR:
        break;
    default:
        UA_LOG_WARNING_READERGROUP(&server->config.logger, rg, "Unknown PubSub state!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_BADNOTSUPPORTED;
}

static UA_StatusCode
UA_ReaderGroup_setPubSubState_operational(UA_Server *server,
                                          UA_ReaderGroup *rg,
                                          UA_StatusCode cause) {
    UA_DataSetReader *dataSetReader;
    switch(rg->state) {
    case UA_PUBSUBSTATE_DISABLED:
        LIST_FOREACH(dataSetReader, &rg->readers, listEntry) {
            UA_DataSetReader_setPubSubState(server, dataSetReader, UA_PUBSUBSTATE_OPERATIONAL,
                                            cause);
        }
        rg->state = UA_PUBSUBSTATE_OPERATIONAL;
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_PAUSED:
        break;
    case UA_PUBSUBSTATE_OPERATIONAL:
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_ERROR:
        break;
    default:
        UA_LOG_WARNING_READERGROUP(&server->config.logger, rg, "Unknown PubSub state!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_BADNOTSUPPORTED;
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
        LIST_FOREACH(dataSetReader, &rg->readers, listEntry) {
            UA_DataSetReader_setPubSubState(server, dataSetReader, UA_PUBSUBSTATE_ERROR, cause);
        }
        break;
    case UA_PUBSUBSTATE_ERROR:
        return UA_STATUSCODE_GOOD;
    default:
        UA_LOG_WARNING_READERGROUP(&server->config.logger, rg, "Unknown PubSub state!");
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
            UA_LOG_WARNING_READERGROUP(&server->config.logger, readerGroup,
                                       "Received unknown PubSub state!");
            break;
    }
    if (state != oldState) {
        /* inform application about state change */
        UA_ServerConfig *pConfig = UA_Server_getConfig(server);
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            pConfig->pubSubConfig.
                stateChangeCallback(server, &readerGroup->identifier, state, cause);
        }
    }
    return ret;
}

UA_StatusCode
UA_Server_setReaderGroupOperational(UA_Server *server, const UA_NodeId readerGroupId){
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = UA_STATUSCODE_BADNOTFOUND;
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(!rg)
        ret = UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_OPERATIONAL,
                                            UA_STATUSCODE_GOOD);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_setReaderGroupDisabled(UA_Server *server, const UA_NodeId readerGroupId){
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
UA_Server_setReaderGroupEncryptionKeys(UA_Server *server, const UA_NodeId readerGroup,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce) {
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroup);
    UA_CHECK_MEM(rg, return UA_STATUSCODE_BADNOTFOUND);
    if(rg->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_WARNING_READERGROUP(&server->config.logger, rg,
                                   "JSON encoding is enabled. The message security is only defined for the UADP message mapping.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if(!rg->config.securityPolicy) {
        UA_LOG_WARNING_READERGROUP(&server->config.logger, rg,
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
#endif

/* Freezing of the configuration */

UA_StatusCode
UA_Server_freezeReaderGroupConfiguration(UA_Server *server,
                                         const UA_NodeId readerGroupId) {
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(!rg)
        return UA_STATUSCODE_BADNOTFOUND;

    if(rg->configurationFrozen)
        return UA_STATUSCODE_GOOD;

    /* PubSubConnection freezeCounter++ */
    UA_NodeId pubSubConnectionId =  rg->linkedConnection;
    UA_PubSubConnection *pubSubConnection =
        UA_PubSubConnection_findConnectionbyId(server, pubSubConnectionId);
    pubSubConnection->configurationFreezeCounter++;
    pubSubConnection->configurationFrozen = true;

    /* ReaderGroup freeze */
    /* TODO: Clarify on the freeze functionality in multiple DSR, multiple
     * networkMessage conf in a RG */
    rg->configurationFrozen = true;

    /* DataSetReader freeze */
    UA_DataSetReader *dataSetReader;
    UA_UInt16 dsrCount = 0;
    LIST_FOREACH(dataSetReader, &rg->readers, listEntry){
        dataSetReader->configurationFrozen = true;
        dsrCount++;
        /* TODO: Configuration frozen for subscribedDataSet once
         * UA_Server_DataSetReader_addTargetVariables API modified to support
         * adding target variable one by one or in a group stored in a list. */
    }

    /* Not rt, we don't have to adjust anything */
    if(rg->config.rtLevel != UA_PUBSUB_RT_FIXED_SIZE)
        return UA_STATUSCODE_GOOD;

    if(dsrCount > 1) {
        UA_LOG_WARNING_READERGROUP(&server->config.logger, rg,
                                   "Mutiple DSR in a readerGroup not supported in RT "
                                   "fixed size configuration");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    dataSetReader = LIST_FIRST(&rg->readers);

    /* Support only to UADP encoding */
    if(dataSetReader->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE]) {
        UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                              "PubSub-RT configuration fail: Non-RT capable encoding.");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    size_t fieldsSize = dataSetReader->config.dataSetMetaData.fieldsSize;
    for(size_t i = 0; i < fieldsSize; i++) {
        UA_FieldTargetVariable *tv =
            &dataSetReader->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];
        const UA_VariableNode *rtNode = (const UA_VariableNode *)
            UA_NODESTORE_GET(server, &tv->targetVariable.targetNodeId);
        if(rtNode != NULL &&
           rtNode->valueBackend.backendType != UA_VALUEBACKENDTYPE_EXTERNAL) {
            UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                                  "PubSub-RT configuration fail: PDS contains field "
                                  "without external data source.");
            UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }

        UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);

        UA_FieldMetaData *field = &dataSetReader->config.dataSetMetaData.fields[i];
        if((UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_STRING].typeId) ||
            UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_BYTESTRING].typeId)) &&
           field->maxStringLength == 0) {
            UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                                  "PubSub-RT configuration fail: "
                                  "PDS contains String/ByteString with dynamic length.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        } else if(!UA_DataType_isNumeric(UA_findDataType(&field->dataType)) &&
                  !UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_BOOLEAN].typeId)) {
            UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                                  "PubSub-RT configuration fail: "
                                  "PDS contains variable with dynamic size.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
    }

    UA_DataSetMessage *dsm = (UA_DataSetMessage*)
        UA_calloc(1, sizeof(UA_DataSetMessage));
    if(!dsm)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Generate the DSM */
    UA_StatusCode res = UA_DataSetReader_generateDataSetMessage(server, dsm, dataSetReader);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READER(&server->config.logger, dataSetReader,
                            "PubSub RT Offset calculation: DataSetMessage generation failed");
        UA_DataSetMessage_clear(dsm);
        UA_free(dsm);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Generate data set messages - Considering 1 DSM as max */
    UA_UInt16 *dsWriterIds = (UA_UInt16 *)UA_calloc(1, sizeof(UA_UInt16));
    if(!dsWriterIds) {
        UA_DataSetMessage_clear(dsm);
        UA_free(dsm);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    *dsWriterIds = dataSetReader->config.dataSetWriterId;

    UA_NetworkMessage *networkMessage = (UA_NetworkMessage *)
        UA_calloc(1, sizeof(UA_NetworkMessage));
    if(!networkMessage) {
        UA_free(dsWriterIds);
        UA_DataSetMessage_clear(dsm);
        UA_free(dsm);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Move the dsm into the NetworkMessage */
    res = UA_DataSetReader_generateNetworkMessage(pubSubConnection, rg, dataSetReader, dsm,
                                                  dsWriterIds, 1, networkMessage);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(networkMessage->payload.dataSetPayload.sizes);
        UA_free(networkMessage);
        UA_free(dsWriterIds);
        UA_DataSetMessage_clear(dsm);
        UA_free(dsm);
        UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                              "PubSub RT Offset calculation: "
                              "NetworkMessage generation failed");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* The offset buffer is already clear if the ReaderGroup was unfrozen
     * UA_NetworkMessageOffsetBuffer_clear(&dataSetReader->bufferedMessage); */
    memset(&dataSetReader->bufferedMessage, 0, sizeof(UA_NetworkMessageOffsetBuffer));
    dataSetReader->bufferedMessage.RTsubscriberEnabled = true;

    /* Compute and store the offsets necessary to decode */
    UA_NetworkMessage_calcSizeBinary(networkMessage, &dataSetReader->bufferedMessage);
    dataSetReader->bufferedMessage.nm = networkMessage;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_unfreezeReaderGroupConfiguration(UA_Server *server,
                                           const UA_NodeId readerGroupId) {
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(!rg)
        return UA_STATUSCODE_BADNOTFOUND;

    /* PubSubConnection freezeCounter-- */
    UA_NodeId pubSubConnectionId =  rg->linkedConnection;
    UA_PubSubConnection *pubSubConnection =
        UA_PubSubConnection_findConnectionbyId(server, pubSubConnectionId);
    pubSubConnection->configurationFreezeCounter--;
    if(pubSubConnection->configurationFreezeCounter == 0){
        pubSubConnection->configurationFrozen = false;
    }

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

#endif /* UA_ENABLE_PUBSUB */
