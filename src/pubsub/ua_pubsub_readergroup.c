/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include <open62541/server_pubsub.h>

#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub.h"

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

    UA_StatusCode res = UA_String_copy(&src->name, &dst->name);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_Array_copy(src->groupProperties, src->groupPropertiesSize,
                        (void**)&dst->groupProperties,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_String_clear(&dst->name);
        return res;
    }
    dst->groupPropertiesSize = src->groupPropertiesSize;
    return UA_STATUSCODE_GOOD;
}

void
UA_ReaderGroupConfig_clear(UA_ReaderGroupConfig *readerGroupConfig) {
    UA_String_clear(&readerGroupConfig->name);
    UA_Array_delete(readerGroupConfig->groupProperties,
                    readerGroupConfig->groupPropertiesSize,
                    &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    readerGroupConfig->groupProperties = NULL;
    readerGroupConfig->groupPropertiesSize = 0;
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

    if(!readerGroupConfig->pubsubManagerCallback.addCustomCallback &&
       readerGroupConfig->enableBlockingSocket) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding ReaderGroup failed, blocking socket functionality "
                       "only supported in customcallback");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *currentConnectionContext =
        UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    if(!currentConnectionContext)
        return UA_STATUSCODE_BADNOTFOUND;

    if(currentConnectionContext->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding ReaderGroup failed. Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Regist (bind) the connection channel if it is not already registered */
    if(!currentConnectionContext->isRegistered) {
        retval |= UA_PubSubConnection_regist(server, &connectionIdentifier);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Allocate memory for new reader group */
    UA_ReaderGroup *newGroup = (UA_ReaderGroup *)UA_calloc(1, sizeof(UA_ReaderGroup));
    if(!newGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newGroup->componentType = UA_PUBSUB_COMPONENT_READERGROUP;

    /* Deep copy of the config */
    retval |= UA_NodeId_copy(&currentConnectionContext->identifier, &newGroup->linkedConnection);
    retval |= UA_ReaderGroupConfig_copy(readerGroupConfig, &newGroup->config);

    /* Check user configured params and define it accordingly */
    if(newGroup->config.subscribingInterval <= 0.0)
        newGroup->config.subscribingInterval = 5; // Set default to 5 ms

    if(newGroup->config.enableBlockingSocket)
        newGroup->config.timeout = 0; // Set timeout to 0 for blocking socket

    if((!newGroup->config.enableBlockingSocket) && (!newGroup->config.timeout))
        newGroup->config.timeout = 1000; /* Set default to 1ms socket timeout
                                            when non-blocking socket allows with
                                            zero timeout */

    LIST_INSERT_HEAD(&currentConnectionContext->readerGroups, newGroup, listEntry);
    currentConnectionContext->readerGroupsSize++;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retval |= addReaderGroupRepresentation(server, newGroup);
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newGroup->identifier);
#endif
    if(readerGroupIdentifier)
        UA_NodeId_copy(&newGroup->identifier, readerGroupIdentifier);

    return retval;
}

UA_StatusCode
removeReaderGroup(UA_Server *server, UA_NodeId groupIdentifier) {
    UA_ReaderGroup* readerGroup =
        UA_ReaderGroup_findRGbyId(server, groupIdentifier);
    if(readerGroup == NULL)
        return UA_STATUSCODE_BADNOTFOUND;

    if(readerGroup->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove ReaderGroup failed. Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Search the connection to which the given readergroup is connected to */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, readerGroup->linkedConnection);
    if(connection == NULL)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Unregister subscribe callback */
    if(readerGroup->state == UA_PUBSUBSTATE_OPERATIONAL)
        UA_ReaderGroup_removeSubscribeCallback(server, readerGroup);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeReaderGroupRepresentation(server, readerGroup);
#endif

    /* UA_Server_ReaderGroup_clear also removes itself from the list */
    UA_Server_ReaderGroup_clear(server, readerGroup);

    /* Remove readerGroup from Connection */
    LIST_REMOVE(readerGroup, listEntry);
    UA_free(readerGroup);
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

    /* Identify the readergroup through the readerGroupIdentifier */
    UA_ReaderGroup *currentReaderGroup =
        UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(!currentReaderGroup)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_ReaderGroupConfig tmpReaderGroupConfig;
    /* deep copy of the actual config */
    UA_ReaderGroupConfig_copy(&currentReaderGroup->config, &tmpReaderGroupConfig);
    *config = tmpReaderGroupConfig;
    return UA_STATUSCODE_GOOD;
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
        UA_ReaderGroup_removeSubscribeCallback(server, rg);
        LIST_FOREACH(dataSetReader, &rg->readers, listEntry) {
            UA_DataSetReader_setPubSubState(server, dataSetReader,
                                            UA_PUBSUBSTATE_DISABLED, cause);
        }
        rg->state = UA_PUBSUBSTATE_DISABLED;
        break;
    case UA_PUBSUBSTATE_ERROR:
        break;
    default:
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Unknown PubSub state!");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_ReaderGroup_setPubSubState_paused(UA_Server *server,
                                     UA_ReaderGroup *rg,
                                     UA_StatusCode cause) {
    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Unknown PubSub state!");
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
        UA_ReaderGroup_addSubscribeCallback(server, rg);
        rg->state = UA_PUBSUBSTATE_OPERATIONAL;
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_PAUSED:
        break;
    case UA_PUBSUBSTATE_OPERATIONAL:
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_ERROR:
        break;
    default:
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Unknown PubSub state!");
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
        UA_ReaderGroup_removeSubscribeCallback(server, rg);
        LIST_FOREACH(dataSetReader, &rg->readers, listEntry){
            UA_DataSetReader_setPubSubState(server, dataSetReader, UA_PUBSUBSTATE_ERROR,
                                            cause);
        }
        break;
    case UA_PUBSUBSTATE_ERROR:
        return UA_STATUSCODE_GOOD;
    default:
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Unknown PubSub state!");
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
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(!rg)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_OPERATIONAL,
                                         UA_STATUSCODE_GOOD);
}

UA_StatusCode
UA_Server_setReaderGroupDisabled(UA_Server *server, const UA_NodeId readerGroupId){
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupId);
    if(!rg)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_ReaderGroup_setPubSubState(server, rg, UA_PUBSUBSTATE_DISABLED,
                                         UA_STATUSCODE_BADRESOURCEUNAVAILABLE);
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

    UA_CHECK_MEM_WARN(rg->config.securityPolicy, return UA_STATUSCODE_BADINTERNALERROR,
                      &server->config.logger, UA_LOGCATEGORY_SERVER,
                      "No SecurityPolicy configured for the ReaderGroup");

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
    pubSubConnection->configurationFrozen = UA_TRUE;

    /* ReaderGroup freeze */
    /* TODO: Clarify on the freeze functionality in multiple DSR, multiple
     * networkMessage conf in a RG */
    rg->configurationFrozen = UA_TRUE;

    /* DataSetReader freeze */
    UA_DataSetReader *dataSetReader;
    UA_UInt16 dsrCount = 0;
    LIST_FOREACH(dataSetReader, &rg->readers, listEntry){
        dataSetReader->configurationFrozen = UA_TRUE;
        dsrCount++;
        /* TODO: Configuration frozen for subscribedDataSet once
         * UA_Server_DataSetReader_addTargetVariables API modified to support
         * adding target variable one by one or in a group stored in a list. */
    }

    /* Not rt, we don't have to adjust anything */
    if(rg->config.rtLevel != UA_PUBSUB_RT_FIXED_SIZE)
        return UA_STATUSCODE_GOOD;

    if(dsrCount > 1) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Mutiple DSR in a readerGroup not supported in RT "
                       "fixed size configuration");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    dataSetReader = LIST_FIRST(&rg->readers);

    /* Support only to UADP encoding */
    if(dataSetReader->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE]) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub-RT configuration fail: "
                           "PDS contains String/ByteString with dynamic length.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        } else if(!UA_DataType_isNumeric(UA_findDataType(&field->dataType)) &&
                  !UA_NodeId_equal(&field->dataType, &UA_TYPES[UA_TYPES_BOOLEAN].typeId)) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub-RT configuration fail: "
                           "PDS contains variable with dynamic size.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
    }

    UA_DataSetMessage *dsm = (UA_DataSetMessage*)
        UA_calloc(1, sizeof(UA_DataSetMessage));
    if(!dsm) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub RT Offset calculation: DSM creation failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Generate the DSM */
    UA_StatusCode res = UA_DataSetReader_generateDataSetMessage(server, dsm, dataSetReader);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "PubSub RT Offset calculation: DataSetMessage generation failed");
        UA_DataSetMessage_clear(dsm);
        UA_free(dsm);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Generate data set messages - Considering 1 DSM as max */
    UA_UInt16 *dsWriterIds = (UA_UInt16 *)UA_calloc(1, sizeof(UA_UInt16));
    if(!dsWriterIds) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub RT Offset calculation: DataSetWriterId creation failed");
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
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub RT Offset calculation: Network message creation failed");
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
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "PubSub RT Offset calculation: NetworkMessage generation failed");
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
        pubSubConnection->configurationFrozen = UA_FALSE;
    }

    /* ReaderGroup unfreeze */
    rg->configurationFrozen = UA_FALSE;

    /* DataSetReader unfreeze */
    UA_DataSetReader *dataSetReader;
    LIST_FOREACH(dataSetReader, &rg->readers, listEntry) {
        dataSetReader->configurationFrozen = UA_FALSE;
        UA_NetworkMessageOffsetBuffer_clear(&dataSetReader->bufferedMessage);
    }

    return UA_STATUSCODE_GOOD;
}

/* This triggers the collection and reception of NetworkMessages and the
 * contained DataSetMessages. */
void
UA_ReaderGroup_subscribeCallback(UA_Server *server,
                                 UA_ReaderGroup *readerGroup) {
    // TODO: feedback for debug-assert vs runtime-check
    UA_assert(server);
    UA_assert(readerGroup);

    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "PubSub subscribe callback");

    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, readerGroup->linkedConnection);
    if(!connection) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "SubscribeCallback(): Find linked connection failed");
        UA_ReaderGroup_setPubSubState(server, readerGroup, UA_PUBSUBSTATE_ERROR,
                                      UA_STATUSCODE_BADCONNECTIONCLOSED);
        return;
    }

    receiveBufferedNetworkMessage(server, readerGroup, connection);
}

/* Add new subscribeCallback. The first execution is triggered directly after
 * creation. */
UA_StatusCode
UA_ReaderGroup_addSubscribeCallback(UA_Server *server, UA_ReaderGroup *readerGroup) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(readerGroup->config.pubsubManagerCallback.addCustomCallback)
        retval = readerGroup->config.pubsubManagerCallback.
            addCustomCallback(server, readerGroup->identifier,
               (UA_ServerCallback)UA_ReaderGroup_subscribeCallback,
               readerGroup, readerGroup->config.subscribingInterval,
               NULL, // TODO: Send base time from reader group config
               // TODO: Send timer policy from reader group config
               UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
               &readerGroup->subscribeCallbackId);
    else {
        if(readerGroup->config.enableBlockingSocket == UA_TRUE) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "addSubscribeCallback() failed, blocking socket "
                           "functionality only supported in customcallback");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }

        retval = UA_PubSubManager_addRepeatedCallback(server,
                    (UA_ServerCallback)UA_ReaderGroup_subscribeCallback,
                    readerGroup, readerGroup->config.subscribingInterval,
                    NULL, // TODO: Send base time from reader group config
                    // TODO: Send timer policy from reader group config
                    UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
                    &readerGroup->subscribeCallbackId);
    }

    /* Run once after creation */
    /* When using blocking socket functionality, the server mechanism might get
     * blocked. It is highly recommended to use custom callback when using
     * blockingsocket. */
    if(readerGroup->config.enableBlockingSocket != UA_TRUE)
        UA_ReaderGroup_subscribeCallback(server, readerGroup);

    return retval;
}

void
UA_ReaderGroup_removeSubscribeCallback(UA_Server *server, UA_ReaderGroup *readerGroup) {
    if(readerGroup->config.pubsubManagerCallback.removeCustomCallback)
        readerGroup->config.pubsubManagerCallback.
            removeCustomCallback(server, readerGroup->identifier,
                                 readerGroup->subscribeCallbackId);
    else
        UA_PubSubManager_removeRepeatedPubSubCallback(server,
                                                      readerGroup->subscribeCallbackId);
}

#endif /* UA_ENABLE_PUBSUB */
