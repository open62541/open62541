/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2022 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Noel Graf)
 */

#include "ua_pubsub.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_networkmessage.h"
#include "ua_pubsub_ns0.h"
#endif

#include "ua_types_encoding_binary.h"

#ifdef UA_ENABLE_PUBSUB_MONITORING
static void
UA_DataSetReader_checkMessageReceiveTimeout(UA_Server *server, UA_DataSetReader *dsr);

static void
UA_DataSetReader_handleMessageReceiveTimeout(UA_Server *server, UA_DataSetReader *dsr);
#endif

static UA_Boolean
publisherIdIsMatching(UA_NetworkMessage *msg, UA_PublisherId *idB) {
    if(!msg->publisherIdEnabled)
        return true;
    UA_PublisherId *idA = &msg->publisherId;
    if(idA->idType != idB->idType)
        return false;
    switch(idA->idType) {
        case UA_PUBLISHERIDTYPE_BYTE:   return idA->id.byte == idB->id.byte;
        case UA_PUBLISHERIDTYPE_UINT16: return idA->id.uint16 == idB->id.uint16;
        case UA_PUBLISHERIDTYPE_UINT32: return idA->id.uint32 == idB->id.uint32;
        case UA_PUBLISHERIDTYPE_UINT64: return idA->id.uint64 == idB->id.uint64;
        case UA_PUBLISHERIDTYPE_STRING: return UA_String_equal(&idA->id.string, &idB->id.string);
        default: break;
    }
    return false;
}

UA_StatusCode
UA_DataSetReader_checkIdentifier(UA_Server *server, UA_NetworkMessage *msg,
                                 UA_DataSetReader *reader,
                                 UA_ReaderGroupConfig readerGroupConfig) {
    if(readerGroupConfig.encodingMimeType != UA_PUBSUB_ENCODING_JSON){
        if(!publisherIdIsMatching(msg, &reader->config.publisherId)) {
            return UA_STATUSCODE_BADNOTFOUND;
        }
        if(msg->groupHeaderEnabled && msg->groupHeader.writerGroupIdEnabled) {
            if(reader->config.writerGroupId != msg->groupHeader.writerGroupId) {
                UA_LOG_DEBUG_READER(server->config.logging, reader,
                                   "WriterGroupId doesn't match");
                return UA_STATUSCODE_BADNOTFOUND;
            }
        }
        if(msg->payloadHeaderEnabled) {
            UA_Byte totalDataSets = msg->payloadHeader.dataSetPayloadHeader.count;
            UA_Byte iterator = 0;
            for(iterator = 0; iterator < totalDataSets; iterator++) { 
                if(reader->config.dataSetWriterId == msg->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[iterator]) {
                    return UA_STATUSCODE_GOOD;
                }
            }
            if (iterator == totalDataSets) {
                UA_LOG_DEBUG_READER(server->config.logging, reader, "DataSetWriterId doesn't match");
                return UA_STATUSCODE_BADNOTFOUND;
            }
        }
        return UA_STATUSCODE_GOOD;
    } else {
        if(!publisherIdIsMatching(msg, &reader->config.publisherId))
            return UA_STATUSCODE_BADNOTFOUND;

        if(reader->config.dataSetWriterId == *msg->payloadHeader.dataSetPayloadHeader.dataSetWriterIds) {
            UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "DataSetReader found. Process NetworkMessage");
            return UA_STATUSCODE_GOOD;
        }
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_DataSetReader_create(UA_Server *server, UA_NodeId readerGroupIdentifier,
                        const UA_DataSetReaderConfig *dataSetReaderConfig,
                        UA_NodeId *readerIdentifier) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

#if defined(UA_ENABLE_PUBSUB_INFORMATIONMODEL) || defined(UA_ENABLE_PUBSUB_MONITORING)
	UA_StatusCode retVal;
#endif
	/* Search the reader group by the given readerGroupIdentifier */
    UA_ReaderGroup *readerGroup = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(readerGroup == NULL)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!dataSetReaderConfig)
        return UA_STATUSCODE_BADNOTFOUND;

    if(readerGroup->configurationFrozen) {
        UA_LOG_WARNING_READERGROUP(server->config.logging, readerGroup,
                                   "Add DataSetReader failed, Subscriber configuration is frozen");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Allocate memory for new DataSetReader */
    UA_DataSetReader *newDataSetReader = (UA_DataSetReader *)
        UA_calloc(1, sizeof(UA_DataSetReader));
    if(!newDataSetReader)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newDataSetReader->componentType = UA_PUBSUB_COMPONENT_DATASETREADER;
    newDataSetReader->linkedReaderGroup = readerGroup;

    /* Copy the config into the new dataSetReader */
    UA_DataSetReaderConfig_copy(dataSetReaderConfig, &newDataSetReader->config);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retVal = addDataSetReaderRepresentation(server, newDataSetReader);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READERGROUP(server->config.logging, readerGroup,
                                 "Add DataSetReader failed, addDataSetReaderRepresentation failed");
        UA_DataSetReaderConfig_clear(&newDataSetReader->config);
        UA_free(newDataSetReader);
        newDataSetReader = 0;
        return retVal;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newDataSetReader->identifier);
#endif

    /* Cache the log string */
    UA_String idStr = UA_STRING_NULL;
    UA_NodeId_print(&newDataSetReader->identifier, &idStr);
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "%.*sDataSetReader %.*s\t| ",
                (int)newDataSetReader->linkedReaderGroup->logIdString.length,
                (char*)newDataSetReader->linkedReaderGroup->logIdString.data,
                (int)idStr.length, idStr.data);
    newDataSetReader->logIdString = UA_STRING_ALLOC(tmpLogIdStr);
    UA_String_clear(&idStr);

    UA_LOG_INFO_READER(server->config.logging, newDataSetReader, "DataSetReader created");

#ifdef UA_ENABLE_PUBSUB_MONITORING
    /* Create message receive timeout timer */
    retVal = server->config.pubSubConfig.monitoringInterface.
        createMonitoring(server, newDataSetReader->identifier,
                         UA_PUBSUB_COMPONENT_DATASETREADER,
                         UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT,
                         newDataSetReader,
                         (void (*)(UA_Server *, void *))
                         UA_DataSetReader_handleMessageReceiveTimeout);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_DataSetReaderConfig_clear(&newDataSetReader->config);
        UA_free(newDataSetReader);
        newDataSetReader = 0;
        return retVal;
    }
#endif /* UA_ENABLE_PUBSUB_MONITORING */

    /* Add the new reader to the group */
    LIST_INSERT_HEAD(&readerGroup->readers, newDataSetReader, listEntry);
    readerGroup->readersCount++;

    if(!UA_String_isEmpty(&newDataSetReader->config.linkedStandaloneSubscribedDataSetName)) {
        // find sds by name
        UA_StandaloneSubscribedDataSet *subscribedDataSet =
            UA_StandaloneSubscribedDataSet_findSDSbyName(server,
               newDataSetReader->config.linkedStandaloneSubscribedDataSetName);
        if(subscribedDataSet != NULL) {
            if(subscribedDataSet->config.subscribedDataSetType != UA_PUBSUB_SDS_TARGET) {
                UA_LOG_ERROR_READER(server->config.logging, newDataSetReader,
                                    "Not implemented! Currently only SubscribedDataSet as "
                                    "TargetVariables is implemented");
            } else {
                if(subscribedDataSet->config.isConnected) {
                    UA_LOG_ERROR_READER(server->config.logging, newDataSetReader,
                                        "SubscribedDataSet is already connected");
                } else {
                    UA_LOG_DEBUG_READER(server->config.logging, newDataSetReader,
                                        "Found SubscribedDataSet");
                    subscribedDataSet->config.isConnected = true;
                    UA_DataSetMetaDataType_copy(
                        &subscribedDataSet->config.dataSetMetaData,
                        &newDataSetReader->config.dataSetMetaData);
                    UA_FieldTargetVariable *targetVars =
                        (UA_FieldTargetVariable *)UA_calloc(
                            subscribedDataSet->config.subscribedDataSet.target
                                .targetVariablesSize,
                            sizeof(UA_FieldTargetVariable));
                    for(size_t index = 0;
                        index < subscribedDataSet->config.subscribedDataSet.target
                                    .targetVariablesSize;
                        index++) {
                        UA_FieldTargetDataType_copy(
                            &subscribedDataSet->config.subscribedDataSet.target
                                 .targetVariables[index],
                            &targetVars[index].targetVariable);
                    }

                    DataSetReader_createTargetVariables(server, newDataSetReader,
                                                        subscribedDataSet->config.subscribedDataSet.
                                                        target.targetVariablesSize, targetVars);
                    subscribedDataSet->connectedReader = newDataSetReader;

                    for(size_t index = 0;
                        index < subscribedDataSet->config.subscribedDataSet.target
                                    .targetVariablesSize;
                        index++) {
                        UA_FieldTargetDataType_clear(&targetVars[index].targetVariable);
                    }

                    UA_free(targetVars);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
                    connectDataSetReaderToDataSet(server, newDataSetReader->identifier,
                                                  subscribedDataSet->identifier);
#endif
                }
            }
        }
    }

    /* Check if used dataSet metaData is valid in context of the rest of the config */
    if(newDataSetReader->config.dataSetFieldContentMask &
       UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        for(size_t fieldIdx = 0;
            fieldIdx < newDataSetReader->config.dataSetMetaData.fieldsSize; fieldIdx++) {
            const UA_FieldMetaData *field =
                &newDataSetReader->config.dataSetMetaData.fields[fieldIdx];
            if((field->builtInType == UA_TYPES_STRING ||
                field->builtInType == UA_TYPES_BYTESTRING) &&
               field->maxStringLength == 0) {
                /* Fields of type String or ByteString need to have defined
                 * MaxStringLength*/
                UA_LOG_ERROR_READER(server->config.logging, newDataSetReader,
                                    "Add DataSetReader failed. MaxStringLength must be "
                                    "set in MetaData when using RawData field encoding.");
                UA_DataSetReaderConfig_clear(&newDataSetReader->config);
                UA_free(newDataSetReader);
                newDataSetReader = NULL;
                return UA_STATUSCODE_BADCONFIGURATIONERROR;
            }
        }
    }

    if(readerIdentifier)
        UA_NodeId_copy(&newDataSetReader->identifier, readerIdentifier);

    /* Set the DataSetReader state after finalizing the configuration */
    return UA_DataSetReader_setPubSubState(server, newDataSetReader,
                                           UA_PUBSUBSTATE_OPERATIONAL);
}

UA_StatusCode
UA_Server_addDataSetReader(UA_Server *server, UA_NodeId readerGroupIdentifier,
                           const UA_DataSetReaderConfig *dataSetReaderConfig,
                           UA_NodeId *readerIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_DataSetReader_create(server, readerGroupIdentifier,
                                                dataSetReaderConfig, readerIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_DataSetReader_remove(UA_Server *server, UA_DataSetReader *dsr) {
    if(dsr->configurationFrozen) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "Remove DataSetReader failed, "
                              "Subscriber configuration is frozen");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, dsr->identifier, true);
#endif

#ifdef UA_ENABLE_PUBSUB_MONITORING
    /* Remove message receive timeout timer */
    server->config.pubSubConfig.monitoringInterface.
        deleteMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                         UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
#endif /* UA_ENABLE_PUBSUB_MONITORING */

    /* Check if a Standalone-SubscribedDataSet is associated with this reader and disconnect it*/
    if(!UA_String_isEmpty(&dsr->config.linkedStandaloneSubscribedDataSetName)) {
        UA_StandaloneSubscribedDataSet *sds =
            UA_StandaloneSubscribedDataSet_findSDSbyName(
                server, dsr->config.linkedStandaloneSubscribedDataSetName);
        if(sds != NULL) {
            sds->config.isConnected = false;
            sds->connectedReader = NULL;
        }
    }

    /* Delete DataSetReader config */
    UA_DataSetReaderConfig_clear(&dsr->config);

    /* Get the ReaderGroup. This must succeed since all Readers are removed from
     * the group before it is deleted in UA_ReaderGroup_remove.*/
    UA_ReaderGroup *rg = dsr->linkedReaderGroup;
    UA_assert(rg);

    /* Remove DataSetReader from group */
    LIST_REMOVE(dsr, listEntry);
    rg->readersCount--;

    /* THe offset buffer is only set when the dsr is frozen
     * UA_NetworkMessageOffsetBuffer_clear(&dsr->bufferedMessage); */

    UA_LOG_INFO_READER(server->config.logging, dsr, "DataSetReader deleted");

    UA_NodeId_clear(&dsr->identifier);
    UA_String_clear(&dsr->logIdString);
    UA_free(dsr);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeDataSetReader(UA_Server *server, UA_NodeId readerIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_DataSetReader *dsr = UA_ReaderGroup_findDSRbyId(server, readerIdentifier);
    if(!dsr) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_DataSetReader_remove(server, dsr);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
DataSetReader_updateConfig(UA_Server *server, UA_ReaderGroup *rg, UA_DataSetReader *dsr,
                           const UA_DataSetReaderConfig *config) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(dsr->configurationFrozen) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "Update DataSetReader config failed. "
                              "Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(rg->configurationFrozen) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "Update DataSetReader config failed. "
                              "Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(dsr->config.subscribedDataSetType != UA_PUBSUB_SDS_TARGET) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "Unsupported SubscribedDataSetType.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* The update functionality will be extended during the next PubSub batches.
     * Currently changes for writerGroupId, dataSetWriterId and TargetVariables are possible. */
    if(dsr->config.writerGroupId != config->writerGroupId)
        dsr->config.writerGroupId = config->writerGroupId;
    if(dsr->config.dataSetWriterId != config->dataSetWriterId)
        dsr->config.dataSetWriterId = config->dataSetWriterId;

    UA_TargetVariables *oldTV = &dsr->config.subscribedDataSet.subscribedDataSetTarget;
    const UA_TargetVariables *newTV = &config->subscribedDataSet.subscribedDataSetTarget;
    if(oldTV->targetVariablesSize == newTV->targetVariablesSize) {
        for(size_t i = 0; i < newTV->targetVariablesSize; i++) {
            if(!UA_NodeId_equal(&oldTV->targetVariables[i].targetVariable.targetNodeId,
                                &newTV->targetVariables[i].targetVariable.targetNodeId)) {
                DataSetReader_createTargetVariables(server, dsr,
                                                    newTV->targetVariablesSize,
                                                    newTV->targetVariables);
                break;
            }
        }
    } else {
        DataSetReader_createTargetVariables(server, dsr, newTV->targetVariablesSize,
                                            newTV->targetVariables);
    }

    UA_StatusCode res = UA_STATUSCODE_GOOD;
#ifdef UA_ENABLE_PUBSUB_MONITORING
    if(dsr->config.messageReceiveTimeout != config->messageReceiveTimeout) {
        /* Update message receive timeout timer interval */
        dsr->config.messageReceiveTimeout = config->messageReceiveTimeout;
        if(dsr->msgRcvTimeoutTimerId != 0) {
            res = server->config.pubSubConfig.monitoringInterface.
                updateMonitoringInterval(server, dsr->identifier,
                                         UA_PUBSUB_COMPONENT_DATASETREADER,
                                         UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT,
                                         dsr);
        }
    }
#endif /* UA_ENABLE_PUBSUB_MONITORING */
    return res;
}

UA_StatusCode
UA_Server_DataSetReader_updateConfig(UA_Server *server, const UA_NodeId dataSetReaderIdentifier,
                                     UA_NodeId readerGroupIdentifier,
                                     const UA_DataSetReaderConfig *config) {
    if(config == NULL)
       return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK(&server->serviceMutex);
    UA_DataSetReader *dsr = UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(!dsr || !rg) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = DataSetReader_updateConfig(server, rg, dsr, config);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_DataSetReader_getConfig(UA_Server *server, const UA_NodeId dataSetReaderIdentifier,
                                 UA_DataSetReaderConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_DataSetReader *dsr = UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    if(dsr)
        res = UA_DataSetReaderConfig_copy(&dsr->config, config);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_DataSetReaderConfig_copy(const UA_DataSetReaderConfig *src,
                            UA_DataSetReaderConfig *dst) {
    memset(dst, 0, sizeof(UA_DataSetReaderConfig));
    UA_StatusCode retVal = UA_String_copy(&src->name, &dst->name);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal = UA_PublisherId_copy(&src->publisherId, &dst->publisherId);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    dst->writerGroupId = src->writerGroupId;
    dst->dataSetWriterId = src->dataSetWriterId;
    dst->expectedEncoding = src->expectedEncoding;
    retVal = UA_DataSetMetaDataType_copy(&src->dataSetMetaData, &dst->dataSetMetaData);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    dst->dataSetFieldContentMask = src->dataSetFieldContentMask;
    dst->messageReceiveTimeout = src->messageReceiveTimeout;

    retVal = UA_ExtensionObject_copy(&src->messageSettings, &dst->messageSettings);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    retVal = UA_ExtensionObject_copy(&src->transportSettings, &dst->transportSettings);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    if(src->subscribedDataSetType == UA_PUBSUB_SDS_TARGET) {
        retVal = UA_TargetVariables_copy(&src->subscribedDataSet.subscribedDataSetTarget,
                                         &dst->subscribedDataSet.subscribedDataSetTarget);
        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }
    }

    retVal = UA_String_copy(&src->linkedStandaloneSubscribedDataSetName, &dst->linkedStandaloneSubscribedDataSetName);

    return retVal;
}

void
UA_DataSetReaderConfig_clear(UA_DataSetReaderConfig *cfg) {
    UA_String_clear(&cfg->name);
    UA_String_clear(&cfg->linkedStandaloneSubscribedDataSetName);
    UA_PublisherId_clear(&cfg->publisherId);
    UA_DataSetMetaDataType_clear(&cfg->dataSetMetaData);
    UA_ExtensionObject_clear(&cfg->messageSettings);
    UA_ExtensionObject_clear(&cfg->transportSettings);
    if(cfg->subscribedDataSetType == UA_PUBSUB_SDS_TARGET) {
        UA_TargetVariables_clear(&cfg->subscribedDataSet.subscribedDataSetTarget);
    }
}

UA_StatusCode
UA_Server_DataSetReader_getState(UA_Server *server, UA_NodeId dsrId,
                                 UA_PubSubState *state) {
    if(!server || !state)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_DataSetReader *dsr = UA_ReaderGroup_findDSRbyId(server, dsrId);
    if(dsr) {
        res = UA_STATUSCODE_GOOD;
        *state = dsr->state;
    }
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_enableDataSetReader(UA_Server *server, const UA_NodeId dsrId) {
    UA_LOCK(&server->serviceMutex);
    UA_DataSetReader *dsr = UA_ReaderGroup_findDSRbyId(server, dsrId);
    if(!dsr) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_StatusCode ret =
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_OPERATIONAL);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_disableDataSetReader(UA_Server *server, const UA_NodeId dsrId) {
    UA_LOCK(&server->serviceMutex);
    UA_DataSetReader *dsr = UA_ReaderGroup_findDSRbyId(server, dsrId);
    if(!dsr) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_StatusCode ret =
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_DISABLED);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_DataSetReader_setPubSubState(UA_Server *server, UA_DataSetReader *dsr,
                                UA_PubSubState targetState) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_ReaderGroup *rg = dsr->linkedReaderGroup;
    UA_assert(rg);

    UA_PubSubState oldState = dsr->state;
    dsr->state = targetState;

    switch(dsr->state) {
        /* Disabled */
    case UA_PUBSUBSTATE_DISABLED:
    case UA_PUBSUBSTATE_ERROR:
        break;

        /* Enabled */
    case UA_PUBSUBSTATE_PAUSED:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
    case UA_PUBSUBSTATE_OPERATIONAL:
        if(rg->state == UA_PUBSUBSTATE_DISABLED ||
           rg->state == UA_PUBSUBSTATE_ERROR) {
            dsr->state = UA_PUBSUBSTATE_PAUSED; /* RG is disabled -> paused */
        } else {
            dsr->state = rg->state; /* RG is enabled -> same state */
        }
        break;

    default:
        dsr->state = UA_PUBSUBSTATE_ERROR;
        res = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }

    /* Inform application about state change */
    if(dsr->state != oldState) {
        UA_ServerConfig *config = &server->config;
        UA_LOG_INFO_READER(config->logging, dsr, "State change: %s -> %s",
                           UA_PubSubState_name(oldState),
                           UA_PubSubState_name(dsr->state));
        if(config->pubSubConfig.stateChangeCallback != 0) {
            UA_UNLOCK(&server->serviceMutex);
            config->pubSubConfig.
                stateChangeCallback(server, &dsr->identifier, dsr->state, res);
            UA_LOCK(&server->serviceMutex);
        }
    }

    return res;
}

UA_StatusCode
UA_FieldTargetVariable_copy(const UA_FieldTargetVariable *src, UA_FieldTargetVariable *dst) {
    /* Do a simple memcpy */
    memcpy(dst, src, sizeof(UA_FieldTargetVariable));
    return UA_FieldTargetDataType_copy(&src->targetVariable, &dst->targetVariable);
}

UA_StatusCode
UA_TargetVariables_copy(const UA_TargetVariables *src, UA_TargetVariables *dst) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_TargetVariables));
    if(src->targetVariablesSize > 0) {
        dst->targetVariables = (UA_FieldTargetVariable*)
            UA_calloc(src->targetVariablesSize, sizeof(UA_FieldTargetVariable));
        if(!dst->targetVariables)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        for(size_t i = 0; i < src->targetVariablesSize; i++)
            retVal |= UA_FieldTargetVariable_copy(&src->targetVariables[i], &dst->targetVariables[i]);
    }
    return retVal;
}

void
UA_TargetVariables_clear(UA_TargetVariables *subscribedDataSetTarget) {
    for(size_t i = 0; i < subscribedDataSetTarget->targetVariablesSize; i++) {
        UA_FieldTargetDataType_clear(&subscribedDataSetTarget->targetVariables[i].targetVariable);
    }
    if(subscribedDataSetTarget->targetVariablesSize > 0)
        UA_free(subscribedDataSetTarget->targetVariables);
    memset(subscribedDataSetTarget, 0, sizeof(UA_TargetVariables));
}

/* This Method is used to initially set the SubscribedDataSet to
 * TargetVariablesType and to create the list of target Variables of a
 * SubscribedDataSetType. */
UA_StatusCode
DataSetReader_createTargetVariables(UA_Server *server, UA_DataSetReader *dsr,
                                    size_t targetVariablesSize,
                                    const UA_FieldTargetVariable *targetVariables) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(dsr->configurationFrozen) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "Create Target Variables failed. "
                              "Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize > 0)
        UA_TargetVariables_clear(&dsr->config.subscribedDataSet.subscribedDataSetTarget);

    /* Set subscribed dataset to TargetVariableType */
    dsr->config.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    UA_TargetVariables tmp;
    tmp.targetVariablesSize = targetVariablesSize;
    tmp.targetVariables = (UA_FieldTargetVariable*)(uintptr_t)targetVariables;
    return UA_TargetVariables_copy(&tmp, &dsr->config.subscribedDataSet.subscribedDataSetTarget);
}

UA_StatusCode
UA_Server_DataSetReader_createTargetVariables(UA_Server *server,
                                              const UA_NodeId dataSetReaderIdentifier,
                                              size_t targetVariablesSize,
                                              const UA_FieldTargetVariable *targetVariables) {
    UA_LOCK(&server->serviceMutex);
    UA_DataSetReader *dataSetReader = UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    if(!dataSetReader) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode res = DataSetReader_createTargetVariables(server, dataSetReader,
                                                            targetVariablesSize, targetVariables);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static void
DataSetReader_processRaw(UA_Server *server, UA_DataSetReader *dsr,
                         UA_DataSetMessage* msg) {
    UA_LOG_TRACE_READER(server->config.logging, dsr, "Received RAW Frame");
    msg->data.keyFrameData.fieldCount = (UA_UInt16)
        dsr->config.dataSetMetaData.fieldsSize;

    /* Start iteration from beginning of rawFields buffer */
    size_t offset = 0;
    for(size_t i = 0; i < dsr->config.dataSetMetaData.fieldsSize; i++) {
        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];

        /* TODO The datatype reference should be part of the internal
         * pubsub configuration to avoid the time-expensive lookup */
        const UA_DataType *type =
            UA_findDataTypeWithCustom(&dsr->config.dataSetMetaData.fields[i].dataType,
                                      server->config.customDataTypes);
        if(!type) {
            UA_LOG_ERROR_READER(server->config.logging, dsr, "Type not found");
            return;
        }

        /* For arrays the length of the array is encoded before the actual data */
        size_t elementCount = 1;
        for(int cnt = 0; cnt < dsr->config.dataSetMetaData.fields[i].valueRank; cnt++) {
            UA_UInt32 dimSize =
                *(UA_UInt32 *)&msg->data.keyFrameData.rawFields.data[offset];
            if(dimSize != dsr->config.dataSetMetaData.fields[i].arrayDimensions[cnt]) {
                UA_LOG_INFO_READER(server->config.logging, dsr,
                                   "Error during Raw-decode KeyFrame field %u: "
                                   "Dimension size in received data doesn't match the dataSetMetaData",
                                   (unsigned)i);
                return;
            }
            offset += sizeof(UA_UInt32);
            elementCount *= dimSize;
        }

        /* Decode the value */
        UA_STACKARRAY(UA_Byte, value, elementCount * type->memSize);
        memset(value, 0, elementCount * type->memSize);
        UA_Byte *valPtr = value;
        UA_StatusCode res = UA_STATUSCODE_GOOD;
        for(size_t cnt = 0; cnt < elementCount; cnt++) {
            res = UA_decodeBinaryInternal(&msg->data.keyFrameData.rawFields,
                                          &offset, valPtr, type, NULL);
            if(dsr->config.dataSetMetaData.fields[i].maxStringLength != 0) {
                if(type->typeKind == UA_DATATYPEKIND_STRING ||
                   type->typeKind == UA_DATATYPEKIND_BYTESTRING) {
                    UA_ByteString *bs = (UA_ByteString *)valPtr;
                    /* Check if length < maxStringLength, The types ByteString and
                     * String are equal in their base definition */
                    size_t lengthDifference =
                        dsr->config.dataSetMetaData.fields[i].maxStringLength - bs->length;
                    offset += lengthDifference;
                }
            }
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_INFO_READER(server->config.logging, dsr,
                                   "Error during Raw-decode KeyFrame field %u: %s",
                                   (unsigned)i, UA_StatusCode_name(res));
                return;
            }
            valPtr += type->memSize;
        }

        /* Write the value */
        if(tv->beforeWrite || tv->externalDataValue) {
            if(tv->beforeWrite)
                tv->beforeWrite(server, &dsr->identifier, &dsr->linkedReaderGroup->identifier,
                                &tv->targetVariable.targetNodeId,
                                tv->targetVariableContext, tv->externalDataValue);
            memcpy((*tv->externalDataValue)->value.data, value, type->memSize);
            if(tv->afterWrite)
                tv->afterWrite(server, &dsr->identifier, &dsr->linkedReaderGroup->identifier,
                               &tv->targetVariable.targetNodeId,
                               tv->targetVariableContext, tv->externalDataValue);
        } else {
            UA_WriteValue writeVal;
            UA_WriteValue_init(&writeVal);
            writeVal.attributeId = tv->targetVariable.attributeId;
            writeVal.indexRange = tv->targetVariable.receiverIndexRange;
            writeVal.nodeId = tv->targetVariable.targetNodeId;
            if(dsr->config.dataSetMetaData.fields[i].valueRank > 0) {
                UA_Variant_setArray(&writeVal.value.value, value, elementCount, type);
            } else {
                UA_Variant_setScalar(&writeVal.value.value, value, type);
            }
            writeVal.value.hasValue = true;
            Operation_Write(server, &server->adminSession, NULL, &writeVal, &res);
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING_READER(server->config.logging, dsr,
                                      "Error writing KeyFrame field %u: %s",
                                      (unsigned)i, UA_StatusCode_name(res));
            }
        }

        /* Clean up if string-type (with mallocs) was used */
        if(!type->pointerFree) {
            valPtr = value;
            for(size_t cnt = 0; cnt < elementCount; cnt++) {
                UA_clear(value, type);
                valPtr += type->memSize;
            }
        }
    }
}

void
UA_DataSetReader_process(UA_Server *server, UA_DataSetReader *dsr,
                         UA_DataSetMessage *msg) {
    if(!dsr || !msg || !server)
        return;

    UA_LOG_DEBUG_READER(server->config.logging, dsr, "Received a network message");

    /* Received a (first) message for the Reader.
     * Transition from PreOperational to Operational. */
    if(dsr->state == UA_PUBSUBSTATE_PREOPERATIONAL)
        UA_DataSetReader_setPubSubState(server, dsr, dsr->state);

#ifdef UA_ENABLE_PUBSUB_MONITORING
    UA_DataSetReader_checkMessageReceiveTimeout(server, dsr);
#endif

    if(dsr->state != UA_PUBSUBSTATE_OPERATIONAL &&
       dsr->state != UA_PUBSUBSTATE_PREOPERATIONAL) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                            "Received a network message but not operational");
        return;
    }

    if(!msg->header.dataSetMessageValid) {
        UA_LOG_INFO_READER(server->config.logging, dsr,
                           "DataSetMessage is discarded: message is not valid");
        return;
    }

    /* TODO: Check ConfigurationVersion */
    /* if(msg->header.configVersionMajorVersionEnabled) {
     *     if(msg->header.configVersionMajorVersion !=
     *            dsr->config.dataSetMetaData.configurationVersion.majorVersion) {
     *         UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
     *                        "DataSetMessage is discarded: ConfigurationVersion "
     *                        "MajorVersion does not match");
     *         return;
     *     }
     * } */

    if(msg->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                       "DataSetMessage is discarded: Only keyframes are supported");
        return;
    }

    /* Process message with raw encoding. We have no field-count information for
     * the message. */
    if(msg->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
        DataSetReader_processRaw(server, dsr, msg);
        return;
    }

    /* Received a heartbeat with no fields */
    if(msg->data.keyFrameData.fieldCount == 0) {
        UA_EventLoop *el = UA_PubSubConnection_getEL(server,
                                                     dsr->linkedReaderGroup->linkedConnection);
        dsr->lastHeartbeatReceived = el->dateTime_nowMonotonic(el);
        return;
    }

    /* Check whether the field count matches the configuration */
    size_t fieldCount = msg->data.keyFrameData.fieldCount;
    if(dsr->config.dataSetMetaData.fieldsSize != fieldCount) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                       "Number of fields does not match the DataSetMetaData configuration");
        return;
    }

    if(dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize != fieldCount) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                       "Number of fields does not match the TargetVariables configuration");
        return;
    }

    /* Write the message fields. RT has the external data value configured. */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < fieldCount; i++) {
        UA_DataValue *field = &msg->data.keyFrameData.dataSetFields[i];
        if(!field->hasValue)
            continue;

        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];

        /* RT-path: write directly into the target memory */
        if(tv->externalDataValue) {
            if(field->value.type != (*tv->externalDataValue)->value.type) {
                UA_LOG_WARNING_READER(server->config.logging, dsr, "Mismatching type");
                continue;
            }

            if(tv->beforeWrite)
                tv->beforeWrite(server, &dsr->identifier, &dsr->linkedReaderGroup->identifier,
                                &tv->targetVariable.targetNodeId,
                                tv->targetVariableContext, tv->externalDataValue);
            memcpy((*tv->externalDataValue)->value.data,
                   field->value.data, field->value.type->memSize);
            if(tv->afterWrite)
                tv->afterWrite(server, &dsr->identifier, &dsr->linkedReaderGroup->identifier,
                               &tv->targetVariable.targetNodeId,
                               tv->targetVariableContext, tv->externalDataValue);
            continue;
        }

        /* Write via the Write-Service */
        UA_WriteValue writeVal;
        UA_WriteValue_init(&writeVal);
        writeVal.attributeId = tv->targetVariable.attributeId;
        writeVal.indexRange = tv->targetVariable.receiverIndexRange;
        writeVal.nodeId = tv->targetVariable.targetNodeId;
        writeVal.value = *field;
        Operation_Write(server, &server->adminSession, NULL, &writeVal, &res);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_INFO_READER(server->config.logging, dsr,
                               "Error writing KeyFrame field %u: %s",
                               (unsigned)i, UA_StatusCode_name(res));
    }
}

#ifdef UA_ENABLE_PUBSUB_MONITORING

static void
UA_DataSetReader_checkMessageReceiveTimeout(UA_Server *server,
                                            UA_DataSetReader *dsr) {
    UA_assert(server != 0);
    UA_assert(dsr != 0);

    /* If previous reader state was error (because we haven't received messages
     * and ran into timeout) we should set the state back to operational */
    if(dsr->state == UA_PUBSUBSTATE_ERROR) {
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_OPERATIONAL);
    }

    /* Stop message receive timeout timer */
    UA_StatusCode res;
    if(dsr->msgRcvTimeoutTimerId != 0) {
        res = server->config.pubSubConfig.monitoringInterface.
            stopMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                           UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
        if(res != UA_STATUSCODE_GOOD)
            UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR);
    }

    /* Start message receive timeout timer */
    res = server->config.pubSubConfig.monitoringInterface.
        startMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                        UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
    if(res != UA_STATUSCODE_GOOD)
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR);
}

/* Timeout callback for DataSetReader MessageReceiveTimeout handling */
static void
UA_DataSetReader_handleMessageReceiveTimeout(UA_Server *server, UA_DataSetReader *dsr) {
    UA_assert(server);
    UA_assert(dsr);

    if(dsr->componentType != UA_PUBSUB_COMPONENT_DATASETREADER) {
        UA_LOG_ERROR_READER(server->config.logging, dsr,
                            "UA_DataSetReader_handleMessageReceiveTimeout(): "
                            "input param is not of type DataSetReader");
        return;
    }

    /* Don't signal an error if we don't expect messages to arrive */
    if(dsr->state != UA_PUBSUBSTATE_OPERATIONAL &&
       dsr->state != UA_PUBSUBSTATE_PREOPERATIONAL)
        return;

    UA_LOG_DEBUG_READER(server->config.logging, dsr,
                        "UA_DataSetReader_handleMessageReceiveTimeout(): "
                        "MessageReceiveTimeout occurred "
                        "MessageReceiveTimeout = %f Timer Id = %u ",
                        dsr->config.messageReceiveTimeout,
                        (UA_UInt32) dsr->msgRcvTimeoutTimerId);

    UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR);
}
#endif /* UA_ENABLE_PUBSUB_MONITORING */

UA_StatusCode
UA_DataSetReader_prepareOffsetBuffer(Ctx *ctx, UA_DataSetReader *reader,
                                     UA_ByteString *buf) {
    UA_NetworkMessage *nm = (UA_NetworkMessage *)
        UA_calloc(1, sizeof(UA_NetworkMessage));
    if(!nm)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode using the non-rt decoding */
    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(ctx, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_clear(nm);
        UA_free(nm);
        return rv;
    }
    rv |= UA_NetworkMessage_decodePayload(ctx, nm);
    rv |= UA_NetworkMessage_decodeFooters(ctx, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_clear(nm);
        UA_free(nm);
        return rv;
    }

    /* Compute and store the offsets necessary to decode */
    size_t nmSize = UA_NetworkMessage_calcSizeBinaryWithOffsetBuffer(nm, &reader->bufferedMessage);
    if(nmSize == 0) {
        UA_NetworkMessage_clear(nm);
        UA_free(nm);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Set the offset buffer in the reader */
    reader->bufferedMessage.nm = nm;

    return rv;
}

void
UA_DataSetReader_decodeAndProcessRT(UA_Server *server, UA_DataSetReader *dsr,
                                    UA_ByteString buf) {
    /* Set up the decoding context */
    Ctx ctx;
    ctx.pos = buf.data;
    ctx.end = buf.data + buf.length;
    ctx.depth = 0;
    memset(&ctx.opts, 0, sizeof(UA_DecodeBinaryOptions));
    ctx.opts.customTypes = server->config.customDataTypes;

    UA_StatusCode rv;
    if(!dsr->bufferedMessage.nm) {
        /* This is the first message being received for the RT fastpath.
         * Prepare the offset buffer. */
        rv = UA_DataSetReader_prepareOffsetBuffer(&ctx, dsr, &buf);
    } else {
        /* Decode with offset information and update the networkMessage */
        rv = UA_NetworkMessage_updateBufferedNwMessage(&ctx, &dsr->bufferedMessage);
    }
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                              "PubSub decoding failed. Could not decode with "
                              "status code %s.", UA_StatusCode_name(rv));
        return;
    }

    UA_DataSetReader_process(server, dsr,
                             dsr->bufferedMessage.nm->payload.dataSetPayload.dataSetMessages);
}

#endif /* UA_ENABLE_PUBSUB */
