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

#ifdef UA_ENABLE_PUBSUB_BUFMALLOC
#include "ua_pubsub_bufmalloc.h"
#endif

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
        UA_LOG_ERROR_READERGROUP(server->config.logging, readerGroup,
                                 "Add DataSetReader failed, create message "
                                 "receive timeout timer failed");
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
                    subscribedDataSet->connectedReader = newDataSetReader->identifier;

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
    /* Stop and remove message receive timeout timer */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(dsr->msgRcvTimeoutTimerRunning) {
        res = server->config.pubSubConfig.monitoringInterface.
            stopMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                           UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_READER(server->config.logging, dsr,
                                "Remove DataSetReader failed. Stop message "
                                "receive timeout timer of DataSetReader '%.*s' failed.",
                                (int) dsr->config.name.length, dsr->config.name.data);
        }
    }

    res |= server->config.pubSubConfig.monitoringInterface.
        deleteMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                         UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READER(server->config.logging, dsr,
                            "Remove DataSetReader failed. Delete message receive "
                            "timeout timer of DataSetReader '%.*s' failed.",
                            (int) dsr->config.name.length, dsr->config.name.data);
    }
#endif /* UA_ENABLE_PUBSUB_MONITORING */
    /* check if a Standalone-SubscribedDataSet is associated with this reader and disconnect it*/
    if(!UA_String_isEmpty(&dsr->config.linkedStandaloneSubscribedDataSetName)) {
        UA_StandaloneSubscribedDataSet *subscribedDataSet =
            UA_StandaloneSubscribedDataSet_findSDSbyName(
                server, dsr->config.linkedStandaloneSubscribedDataSetName);
        if(subscribedDataSet != NULL) {
            subscribedDataSet->config.isConnected = false;
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
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR_READER(server->config.logging, dsr,
                                    "Update DataSetReader message receive timeout timer failed.");
            }
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

/* This functionality of this API will be used in future to create mirror Variables - TODO */
/* UA_StatusCode
UA_Server_DataSetReader_createDataSetMirror(UA_Server *server, UA_String *parentObjectNodeName,
                                            UA_NodeId dataSetReaderIdentifier) {
    if((server == NULL) || (parentNode == NULL)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_DataSetReader* pDataSetReader = UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    if(pDataSetReader == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(pDataSetReader->configurationFrozen) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Add Target Variables failed. Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    } // TODO: Frozen configuration variable in TargetVariable structure

    UA_TargetVariables targetVars;
    targetVars.targetVariablesSize = pDataSetReader->config.dataSetMetaData.fieldsSize;
    targetVars.targetVariables = (UA_FieldTargetVariable *)
        UA_calloc(targetVars.targetVariablesSize, sizeof(UA_FieldTargetVariable));

    for(size_t i = 0; i < pDataSetReader->config.dataSetMetaData.fieldsSize; i++) {
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.valueRank = pDataSetReader->config.dataSetMetaData.fields[i].valueRank;
        if(pDataSetReader->config.dataSetMetaData.fields[i].arrayDimensionsSize > 0) {
            retval = UA_Array_copy(pDataSetReader->config.dataSetMetaData.fields[i].arrayDimensions,
                                   pDataSetReader->config.dataSetMetaData.fields[i].arrayDimensionsSize,
                                   (void**)&vAttr.arrayDimensions, &UA_TYPES[UA_TYPES_UINT32]);
            if(retval == UA_STATUSCODE_GOOD) {
                vAttr.arrayDimensionsSize =
                    pDataSetReader->config.dataSetMetaData.fields[i].arrayDimensionsSize;
            }

        }

        vAttr.dataType = pDataSetReader->config.dataSetMetaData.fields[i].dataType;

        vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
        UA_LocalizedText_copy(&pDataSetReader->config.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        UA_QualifiedName qn;
        UA_QualifiedName_init(&qn);
        char szTmpName[UA_MAX_SIZENAME];
        if(pDataSetReader->config.dataSetMetaData.fields[i].name.length > 0) {
            UA_UInt16 slen = UA_MAX_SIZENAME -1;
            vAttr.displayName.locale = UA_STRING("en-US");
            vAttr.displayName.text = pDataSetReader->config.dataSetMetaData.fields[i].name;
            if(pDataSetReader->config.dataSetMetaData.fields[i].name.length < slen) {
                slen = (UA_UInt16)pDataSetReader->config.dataSetMetaData.fields[i].name.length;
                mp_snprintf(szTmpName, sizeof(szTmpName), "%.*s", (int)slen,
                            (const char*)pDataSetReader->config.dataSetMetaData.fields[i].name.data);
            }

            szTmpName[slen] = '\0';
            qn = UA_QUALIFIEDNAME(1, szTmpName);
        }
        else {
            strcpy(szTmpName, "SubscribedVariable");
            vAttr.displayName = UA_LOCALIZEDTEXT("en-US", szTmpName);
            qn = UA_QUALIFIEDNAME(1, "SubscribedVariable");
        }

        // Add variable to the given parent node
        UA_NodeId newNode;
        retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, *parentNode,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), qn,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newNode);
        if(retval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                        "addVariableNode %s succeeded", szTmpName);
        }
        else {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                         "addVariableNode: error 0x%" PRIx32, retval);
        }

        targetVars.targetVariables[i].targetVariable.attributeId = UA_ATTRIBUTEID_VALUE;
        UA_NodeId_copy(&newNode, &targetVars.targetVariables[i].targetVariable.targetNodeId);
        UA_NodeId_clear(&newNode);
        if(vAttr.arrayDimensionsSize > 0) {
            UA_Array_delete(vAttr.arrayDimensions, vAttr.arrayDimensionsSize,
                            &UA_TYPES[UA_TYPES_UINT32]);
        }
    }

    UA_TargetVariables_clear(&targetVars);
    return retval;
}*/

static void
DataSetReader_processRaw(UA_Server *server, UA_DataSetReader *dsr,
                         UA_DataSetMessage* msg) {
    UA_LOG_TRACE_READER(server->config.logging, dsr, "Received RAW Frame");
    msg->data.keyFrameData.fieldCount = (UA_UInt16)
        dsr->config.dataSetMetaData.fieldsSize;

    size_t offset = 0;
    /* start iteration from beginning of rawFields buffer */
    msg->data.keyFrameData.rawFields.length = 0;
    for(size_t i = 0; i < dsr->config.dataSetMetaData.fieldsSize; i++) {
        /* TODO The datatype reference should be part of the internal
         * pubsub configuration to avoid the time-expensive lookup */
        const UA_DataType *type =
            UA_findDataTypeWithCustom(&dsr->config.dataSetMetaData.fields[i].dataType,
                                      server->config.customDataTypes);
        size_t fieldLength = 0;
        if(type->typeKind == UA_DATATYPEKIND_STRING || type->typeKind == UA_DATATYPEKIND_BYTESTRING)
        {
            UA_assert(dsr->config.dataSetMetaData.fields[i].maxStringLength != 0);
            // Length of binary encoded string = 4 (string length encoded as uint32) + N (actual string data) bytes.
            // For fixed message layout N equals maxStringlength.
            fieldLength = sizeof(UA_UInt32) + dsr->config.dataSetMetaData.fields[i].maxStringLength;
        }
else
        {
            fieldLength = type->memSize;
        }

        // For arrays the length of the array is encoded before the actual data
        size_t elementCount = 1;
        size_t dimCnt = 0;
        for(int cnt = 0; cnt < dsr->config.dataSetMetaData.fields[i].valueRank; cnt++) {
            UA_UInt32 dimSize =
                *(UA_UInt32 *)&msg->data.keyFrameData.rawFields.data[offset];
            if(dimSize != dsr->config.dataSetMetaData.fields[i].arrayDimensions[cnt]) {
                UA_LOG_INFO_READER(
                    server->config.logging, dsr,
                    "Error during Raw-decode KeyFrame field %u: "
                    "Dimension size in received data doesn't match the dataSetMetaData",
                    (unsigned)i);
                return;
            }
            offset += sizeof(UA_UInt32);
            elementCount *= dimSize;
            dimCnt++;
        }

        msg->data.keyFrameData.rawFields.length +=
            fieldLength * elementCount + dimCnt * sizeof(UA_UInt32);
        UA_STACKARRAY(UA_Byte, value, elementCount * type->memSize);
        UA_Byte *valPtr = value;
        UA_StatusCode res = UA_STATUSCODE_GOOD;
        for(size_t cnt = 0; cnt < elementCount; cnt++) {
            res = UA_decodeBinaryInternal(&msg->data.keyFrameData.rawFields, &offset,
                                          valPtr, type, NULL);
            if(dsr->config.dataSetMetaData.fields[i].maxStringLength != 0) {
                if(type->typeKind == UA_DATATYPEKIND_STRING ||
                    type->typeKind == UA_DATATYPEKIND_BYTESTRING) {
                    UA_ByteString *bs = (UA_ByteString *) valPtr;
                    //check if length < maxStringLength, The types ByteString and String are equal in their base definition
                    size_t lengthDifference = dsr->config.dataSetMetaData.fields[i].maxStringLength - bs->length;
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

        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];

        if(dsr->linkedReaderGroup->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE) {
            if (tv->beforeWrite) {
                void *pData = (**tv->externalDataValue).value.data;
                (**tv->externalDataValue).value.data = value;   // set raw data as "preview"
                tv->beforeWrite(server, &dsr->identifier, &dsr->linkedReaderGroup->identifier,
                                &tv->targetVariable.targetNodeId,
                                tv->targetVariableContext,
                                tv->externalDataValue);
                (**tv->externalDataValue).value.data = pData;  // restore previous data pointer
            }
            memcpy((**tv->externalDataValue).value.data, value, type->memSize);
            if(tv->afterWrite)
                tv->afterWrite(server, &dsr->identifier,
                                &dsr->linkedReaderGroup->identifier,
                                &tv->targetVariable.targetNodeId,
                                tv->targetVariableContext,
                                tv->externalDataValue);
            continue; /* No dynamic allocation for fixed-size msg, no need to _clear */
        }

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
        UA_clear(value, type);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_READER(server->config.logging, dsr,
                               "Error writing KeyFrame field %u: %s",
                               (unsigned)i, UA_StatusCode_name(res));
        }
    }
}

static void
DataSetReader_processFixedSize(UA_Server *server, UA_DataSetReader *dsr,
                               UA_DataSetMessage *msg, size_t fieldCount) {
    for(size_t i = 0; i < fieldCount; i++) {
        if(!msg->data.keyFrameData.dataSetFields[i].hasValue)
            continue;

        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];
        if(tv->targetVariable.attributeId != UA_ATTRIBUTEID_VALUE)
            continue;

        if(msg->data.keyFrameData.dataSetFields[i].value.type !=
           (*tv->externalDataValue)->value.type) {
            UA_LOG_WARNING_READER(server->config.logging, dsr,
                                  "Mismatching type");
            continue;
        }

        if (tv->beforeWrite) {
            UA_DataValue *tmp = &msg->data.keyFrameData.dataSetFields[i];
            tv->beforeWrite(server, &dsr->identifier, &dsr->linkedReaderGroup->identifier,
                            &tv->targetVariable.targetNodeId,
                            tv->targetVariableContext, &tmp);
        }
        if(UA_LIKELY(tv->externalDataValue != NULL)) {
            memcpy((**tv->externalDataValue).value.data,
                   msg->data.keyFrameData.dataSetFields[i].value.data,
                   msg->data.keyFrameData.dataSetFields[i].value.type->memSize);
        }
        if(tv->afterWrite)
            tv->afterWrite(server, &dsr->identifier, &dsr->linkedReaderGroup->identifier,
                           &tv->targetVariable.targetNodeId,
                           tv->targetVariableContext, tv->externalDataValue);
    }
}

//Changes made for JNI implementation
static int transferFlag =
    1;  // Flag to stop message processing till messages are transfered to Java
static int processFlag =
    1;  // Flag to stop message transfer process till current message is processed
static int messagesFlag =
    0;  // Flag to see if messages are received(Checked only for 1st connection)
static int messagesCaptured = 0;  // Variable to store number of captured messages
struct Node {
    char *publisherId;
    int datasetWriterId;
    int writerGroupId;
    int fields;
    void *data[100];
};
static struct Node Messages[300];  // Variable to store captured messages

void
UA_DataSetReader_process(UA_Server *server, UA_DataSetReader *dsr,
                         UA_DataSetMessage *msg) {
    if(!dsr || !msg || !server)
        return;

    //Make the processing wait till transfer of messages from JNI is completed
    while(transferFlag == 0)
        printf("");
    processFlag = 0;

    /* Received a (first) message for the Reader.
     * Transition from PreOperational to Operational. */
    if(dsr->state == UA_PUBSUBSTATE_PREOPERATIONAL)
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_OPERATIONAL);

    /* Check the metadata, to see if this reader is configured for a heartbeat */
    if(dsr->config.dataSetMetaData.fieldsSize == 0 &&
       dsr->config.dataSetMetaData.configurationVersion.majorVersion == 0 &&
       dsr->config.dataSetMetaData.configurationVersion.minorVersion == 0) {
        /* Expecting a heartbeat, check the message */
        if(msg->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME ||
            msg->header.configVersionMajorVersion != 0 ||
            msg->header.configVersionMinorVersion != 0 ||
            msg->data.keyFrameData.fieldCount != 0) {
            UA_LOG_INFO_READER(server->config.logging, dsr,
                               "This DSR expects heartbeat, but the received "
                               "message doesn't seem to be so.");
        }
#ifdef UA_ENABLE_PUBSUB_MONITORING
        UA_DataSetReader_checkMessageReceiveTimeout(server, dsr);
#endif
        UA_EventLoop *el = UA_PubSubConnection_getEL(server,
                                                     dsr->linkedReaderGroup->linkedConnection);
        dsr->lastHeartbeatReceived = el->dateTime_nowMonotonic(el);
        return;
    }

    UA_LOG_DEBUG_READER(server->config.logging, dsr,
                        "DataSetReader '%.*s': received a network message",
                        (int)dsr->config.name.length, dsr->config.name.data);

    if(!msg->header.dataSetMessageValid) {
        UA_LOG_INFO_READER(server->config.logging, dsr,
                           "DataSetMessage is discarded: message is not valid");
        /* To Do check ConfigurationVersion */
        /* if(msg->header.configVersionMajorVersionEnabled) {
         *     if(msg->header.configVersionMajorVersion !=
         *            dsr->config.dataSetMetaData.configurationVersion.majorVersion) {
         *         UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
         *                        "DataSetMessage is discarded: ConfigurationVersion "
         *                        "MajorVersion does not match");
         *         return;
         *     }
         * } */
        return;
    }

    if(msg->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME) {
        UA_LOG_WARNING_READER(server->config.logging, dsr,
                       "DataSetMessage is discarded: Only keyframes are supported");
        return;
    }

    /* Process message with raw encoding (realtime and non-realtime) */
    if(msg->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
        DataSetReader_processRaw(server, dsr, msg);
#ifdef UA_ENABLE_PUBSUB_MONITORING
        UA_DataSetReader_checkMessageReceiveTimeout(server, dsr);
#endif
        return;
    }

    //Parse the incoming messages based on the metadata(type of received data)
    //Convert it to string(Char*) and then store in "data" variable of "Node" structure
    //Then once the JNI call is recieved from Java, send all the stored messages to Java through JNI
    size_t i = 0;
    char buf[20];
    void *data_temp[100];
    for(i = 0; i < msg->data.keyFrameData.fieldCount; i++) {
        if(!msg->data.keyFrameData.dataSetFields[i].hasValue)
            continue;
        UA_Byte type = dsr->config.dataSetMetaData.fields[i].builtInType;

        switch(type) {
            case UA_NS0ID_DATETIME:;
                // Convert Datetime to it's original form and remove Double quotes
                UA_DateTime *varcontentt = UA_DateTime_new();
                *varcontentt =
                    *((UA_DateTime *)msg->data.keyFrameData.dataSetFields[i].value.data);
                const UA_DataType *typee = &UA_TYPES[UA_TYPES_DATETIME];

                size_t sizee = UA_calcSizeJson((void *)varcontentt, typee, NULL);
                UA_ByteString receivedTime;

                UA_ByteString_allocBuffer(&receivedTime, sizee + 1);

                status ss =
                    UA_encodeJson((void *)varcontentt, typee, &receivedTime, NULL);
                char buff[40];
                int sizeInd;
                for(sizeInd = 1; sizeInd < sizee - 1; sizeInd++) {
                    buff[sizeInd - 1] = receivedTime.data[sizeInd];
                }
                buff[sizeInd - 1] = '\0';

                data_temp[i] = malloc(sizeInd);
                strncpy(data_temp[i], buff, sizeInd);

                // printf("\n\n\n\n\t\t\tMessage Received by
                // subscriber:'%s'\n\n\n\n",buff);
                break;
            case UA_NS0ID_BOOLEAN:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufBool[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        if(((UA_Boolean *)msg->data.keyFrameData.dataSetFields[i]
                                .value.data)[arrayElem] == 0)
                            strncpy(buf, "false", 6);
                        else
                            strncpy(buf, "true", 5);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufBool, buf, strlen(buf));
                    }
                    bufBool[strlen(bufBool)] = '\0';
                    data_temp[i] = malloc(strlen(bufBool) + 1);
                    strncpy(data_temp[i], bufBool, strlen(bufBool) + 1);
                } else {
                    if(*((UA_Int32 *)msg->data.keyFrameData.dataSetFields[i]
                             .value.data) == 0)
                        data_temp[i] = "false";
                    else
                        data_temp[i] = "true";
                }
                // printf("\n\n\n\n\t\t\tMessage Received by
                // subscriber:'%s'\n\n\n\n",data_temp[i]);
                break;
            case UA_NS0ID_INT32:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufInt32[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%d",
                                ((UA_Int32 *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufInt32, buf, strlen(buf));
                    }
                    bufInt32[strlen(bufInt32)] = '\0';
                    data_temp[i] = malloc(strlen(bufInt32) + 1);
                    strncpy(data_temp[i], bufInt32, strlen(bufInt32) + 1);
                } else {
                    sprintf(buf, "%d",
                            *((UA_Int32 *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/

                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber: '%s' at time:
                   '%02d:%02d:%02d'\n\n\n\n", buf,current_time->tm_hour,
                   current_time->tm_min,current_time->tm_sec);*/
                break;
            case UA_NS0ID_FLOAT:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufFloat[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%.3f",
                                ((UA_Float *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufFloat, buf, strlen(buf));
                    }
                    bufFloat[strlen(bufFloat)] = '\0';
                    data_temp[i] = malloc(strlen(bufFloat) + 1);
                    strncpy(data_temp[i], bufFloat, strlen(bufFloat) + 1);
                } else {
                    sprintf(buf, "%.3f",
                            *((UA_Float *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/

                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber: '%s' at time:
                   '%02d:%02d:%02d'\n\n\n\n", buf,current_time->tm_hour,
                   current_time->tm_min,current_time->tm_sec);*/
                break;
            case UA_NS0ID_STRING:;
                data_temp[i] = malloc(
                    ((UA_String *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->length +
                    1);
                strncpy(data_temp[i],
                        ((UA_String *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->data,
                        ((UA_String *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->length);
                char *ptr = data_temp[i];
                for(int x = 0;
                    x < ((UA_String *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->length;
                    x++)
                    ptr++;
                *ptr = '\0';
                // printf("\n\n\n\n\t\t\tMessage Received by
                // subscriber:'%s'\n\n\n\n",data_temp[i]);
                break;
            case UA_NS0ID_INT16:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufInt16[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%d",
                                ((UA_Int16 *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufInt16, buf, strlen(buf));
                    }
                    bufInt16[strlen(bufInt16)] = '\0';
                    data_temp[i] = malloc(strlen(bufInt16) + 1);
                    strncpy(data_temp[i], bufInt16, strlen(bufInt16) + 1);
                } else {
                    sprintf(buf, "%d",
                            *((UA_Int16 *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case UA_NS0ID_INT64:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufInt64[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%d",
                                ((UA_Int64 *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufInt64, buf, strlen(buf));
                    }
                    bufInt64[strlen(bufInt64)] = '\0';
                    data_temp[i] = malloc(strlen(bufInt64) + 1);
                    strncpy(data_temp[i], bufInt64, strlen(bufInt64) + 1);
                } else {
                    sprintf(buf, "%d",
                            *((UA_Int64 *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber: '%s'\n\n\n\n",
                 * data_temp[i]);*/
                break;
            case UA_NS0ID_UINT16:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufUInt16[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%d",
                                ((UA_UInt16 *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufUInt16, buf, strlen(buf));
                    }
                    bufUInt16[strlen(bufUInt16)] = '\0';
                    data_temp[i] = malloc(strlen(bufUInt16) + 1);
                    strncpy(data_temp[i], bufUInt16, strlen(bufUInt16) + 1);
                } else {
                    sprintf(buf, "%d",
                            *((UA_UInt16 *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case UA_NS0ID_UINT32:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufUInt32[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%d",
                                ((UA_UInt32 *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufUInt32, buf, strlen(buf));
                    }
                    bufUInt32[strlen(bufUInt32)] = '\0';
                    data_temp[i] = malloc(strlen(bufUInt32) + 1);
                    strncpy(data_temp[i], bufUInt32, strlen(bufUInt32) + 1);
                } else {
                    sprintf(buf, "%d",
                            *((UA_UInt32 *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case UA_NS0ID_UINT64:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufUInt64[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%d",
                                ((UA_UInt64 *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufUInt64, buf, strlen(buf));
                    }
                    bufUInt64[strlen(bufUInt64)] = '\0';
                    data_temp[i] = malloc(strlen(bufUInt64) + 1);
                    strncpy(data_temp[i], bufUInt64, strlen(bufUInt64) + 1);
                } else {
                    sprintf(buf, "%d",
                            *((UA_UInt64 *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case UA_NS0ID_BYTE:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufByte[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%d",
                                ((UA_Byte *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufByte, buf, strlen(buf));
                    }
                    bufByte[strlen(bufByte)] = '\0';
                    data_temp[i] = malloc(strlen(bufByte) + 1);
                    strncpy(data_temp[i], bufByte, strlen(bufByte) + 1);
                } else {
                    sprintf(
                        buf, "%d",
                        *((UA_Byte *)msg->data.keyFrameData.dataSetFields[i].value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case UA_NS0ID_SBYTE:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufSByte[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        if(arrayElem > 0)
                            strncat(buf, ", ", 2);
                        sprintf(buf, "%d",
                                ((UA_SByte *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufSByte, buf, strlen(buf));
                    }
                    bufSByte[strlen(bufSByte)] = '\0';
                    data_temp[i] = malloc(strlen(bufSByte) + 1);
                    strncpy(data_temp[i], bufSByte, strlen(bufSByte) + 1);
                } else {
                    sprintf(buf, "%d",
                            *((UA_SByte *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case UA_NS0ID_DOUBLE:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufDouble[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%.6lf",
                                ((UA_Double *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufDouble, buf, strlen(buf));
                    }
                    bufDouble[strlen(bufDouble)] = '\0';
                    data_temp[i] = malloc(strlen(bufDouble) + 1);
                    strncpy(data_temp[i], bufDouble, strlen(bufDouble) + 1);
                } else {
                    sprintf(buf, "%.6lf",
                            *((UA_Double *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case UA_NS0ID_BYTESTRING:;
                // Convert Datetime to it's original form and remove Double quotes
                char bufByte[40] = "";
                UA_ByteString *srcb =
                    ((UA_ByteString *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->data;

                for(int h = 0;
                    h <
                    ((UA_ByteString *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->length;
                    h++) {
                    sprintf(buf, "%d",
                            ((UA_ByteString *)msg->data.keyFrameData.dataSetFields[i]
                                 .value.data)
                                ->data[h]);
                    /*printf("\n\t\t\tMessage Received bysubscriber: '%d'\n",
                     * ((UA_ByteString*)msg->data.keyFrameData.dataSetFields[i].value.data)->data[h]);*/
                    strncat(bufByte, buf, strlen(buf));
                }
                bufByte[strlen(bufByte)] = '\0';

                data_temp[i] = malloc(strlen(bufByte) + 1);
                strncpy(data_temp[i], bufByte, strlen(bufByte) + 1);

                break;
            case UA_NS0ID_GUID:;
                // Convert Datetime to it's original form and remove Double quotes
                UA_Guid varcontentg =
                    *((UA_Guid *)msg->data.keyFrameData.dataSetFields[i].value.data);
                const UA_DataType *typeg = &UA_TYPES[UA_TYPES_GUID];

                size_t sizeg = UA_calcSizeJson((void *)&varcontentg, typeg, NULL);
                UA_ByteString receivedGuid;

                UA_ByteString_allocBuffer(&receivedGuid, sizeg + 1);

                status sg =
                    UA_encodeJson((void *)&varcontentg, typeg, &receivedGuid, NULL);

                char bufg[40];
                int sizegInd;
                for(sizegInd = 1; sizegInd < sizeg - 1; sizegInd++) {
                    bufg[sizegInd - 1] = receivedGuid.data[sizegInd];
                }
                bufg[sizegInd - 1] = '\0';

                data_temp[i] = malloc(sizegInd);
                strncpy(data_temp[i], bufg, sizegInd);

                break;
            case UA_NS0ID_LOCALIZEDTEXT:;
                data_temp[i] =
                    malloc(((UA_LocalizedText *)msg->data.keyFrameData.dataSetFields[i]
                                .value.data)
                               ->text.length +
                           1);
                strncpy(data_temp[i],
                        ((UA_LocalizedText *)msg->data.keyFrameData.dataSetFields[i]
                             .value.data)
                            ->text.data,
                        ((UA_LocalizedText *)msg->data.keyFrameData.dataSetFields[i]
                             .value.data)
                            ->text.length);
                char *ptr1 = data_temp[i];
                for(int x = 0;
                    x < ((UA_LocalizedText *)msg->data.keyFrameData.dataSetFields[i]
                             .value.data)
                            ->text.length;
                    x++)
                    ptr1++;
                *ptr1 = '\0';
                break;
            case(UA_Byte)UA_NS0ID_DURATION:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufDouble[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(buf, "%.6lf",
                                ((UA_Duration *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)[arrayElem]);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufDouble, buf, strlen(buf));
                    }
                    bufDouble[strlen(bufDouble)] = '\0';
                    data_temp[i] = malloc(strlen(bufDouble) + 1);
                    strncpy(data_temp[i], bufDouble, strlen(bufDouble) + 1);
                } else {
                    sprintf(buf, "%.6lf",
                            *((UA_Duration *)msg->data.keyFrameData.dataSetFields[i]
                                  .value.data));
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case(UA_Byte)UA_NS0ID_ENUMVALUETYPE:;
                if(msg->data.keyFrameData.dataSetFields[i].value.arrayLength > 0) {
                    char bufDouble[100] = "";
                    for(int arrayElem = 0;
                        arrayElem <
                        msg->data.keyFrameData.dataSetFields[i].value.arrayLength;
                        arrayElem++) {
                        sprintf(
                            buf, "%d",
                            ((UA_EnumValueType *)msg->data.keyFrameData.dataSetFields[i]
                                 .value.data)[arrayElem]
                                .value);
                        if(arrayElem <
                           msg->data.keyFrameData.dataSetFields[i].value.arrayLength - 1)
                            strncat(buf, ", ", 2);
                        strncat(bufDouble, buf, strlen(buf));
                    }
                    bufDouble[strlen(bufDouble)] = '\0';
                    data_temp[i] = malloc(strlen(bufDouble) + 1);
                    strncpy(data_temp[i], bufDouble, strlen(bufDouble) + 1);
                } else {
                    sprintf(buf, "%d",
                            ((UA_EnumValueType *)msg->data.keyFrameData.dataSetFields[i]
                                 .value.data)
                                ->value);
                    data_temp[i] = malloc(strlen(buf) + 1);
                    strncpy(data_temp[i], buf, strlen(buf) + 1);
                }
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case(UA_Byte)UA_NS0ID_LOCALEID:;
                data_temp[i] = malloc(
                    ((UA_LocaleId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->length +
                    1);
                strncpy(
                    data_temp[i],
                    ((UA_LocaleId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->data,
                    ((UA_LocaleId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->length);
                char *ptrLocale = data_temp[i];
                for(int x = 0;
                    x <
                    ((UA_LocaleId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->length;
                    x++)
                    ptrLocale++;
                *ptrLocale = '\0';
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case(UA_Byte)UA_NS0ID_NODEID:;
                char bufNodeID[100] = "ns=";
                sprintf(buf, "%d",
                        ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->namespaceIndex);
                strncat(bufNodeID, buf, strlen(buf));

                if(((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                           ->identifierType == 0 ||
                   ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                           ->identifierType == 1 ||
                   ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                           ->identifierType == 2) {
                    strncat(bufNodeID, ";i=", 3);
                    sprintf(
                        buf, "%d",
                        ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->identifier.numeric);
                    strncat(bufNodeID, buf, strlen(buf));
                } else if(((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i]
                               .value.data)
                              ->identifierType == 3) {
                    strncat(bufNodeID, ";s=", 3);
                    strncpy(
                        buf,
                        ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->identifier.string.data,
                        ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->identifier.string.length);

                    strncat(bufNodeID, buf, strlen(buf));
                } else if(((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i]
                               .value.data)
                              ->identifierType == 4) {
                    strncat(bufNodeID, ";guid=", 6);
                    UA_Guid varcontentg =
                        ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->identifier.guid;
                    const UA_DataType *typeg = &UA_TYPES[UA_TYPES_GUID];

                    size_t sizeg = UA_calcSizeJson((void *)&varcontentg, typeg, NULL);
                    UA_ByteString receivedGuid;

                    UA_ByteString_allocBuffer(&receivedGuid, sizeg + 1);

                    status sg =
                        UA_encodeJson((void *)&varcontentg, typeg, &receivedGuid, NULL);

                    char bufg[40];
                    int sizegInd;
                    for(sizegInd = 1; sizegInd < sizeg - 1; sizegInd++) {
                        bufg[sizegInd - 1] = receivedGuid.data[sizegInd];
                    }
                    bufg[sizegInd - 1] = '\0';

                    strncat(bufNodeID, bufg, strlen(bufg));
                } else {
                    strncat(bufNodeID, ";bs=", 4);
                    char bufByteString[40] = "";
                    UA_ByteString *srcb =
                        ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->identifier.byteString.data;

                    for(int h = 0;
                        h <
                        ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i].value.data)
                            ->identifier.byteString.length;
                        h++) {
                        sprintf(buf, "%d",
                                ((UA_NodeId *)msg->data.keyFrameData.dataSetFields[i]
                                     .value.data)
                                    ->identifier.byteString.data[h]);
                        /*printf("\n\t\t\tMessage Received by subscriber: '%d'\n",
                                ((UA_ByteString*
                           *)msg->data.keyFrameData.dataSetFields[i].value.data)->data[h]);*/
                        strncat(bufByteString, buf, strlen(buf));
                    }
                    bufByteString[strlen(bufByteString)] = '\0';
                    strncat(bufNodeID, bufByteString, strlen(bufByteString));
                }
                bufNodeID[strlen(bufNodeID)] = '\0';
                data_temp[i] = malloc(strlen(bufNodeID) + 1);
                strncpy(data_temp[i], bufNodeID, strlen(bufNodeID) + 1);

                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case(UA_Byte)UA_NS0ID_QUALIFIEDNAME:;
                data_temp[i] =
                    malloc(((UA_QualifiedName *)msg->data.keyFrameData.dataSetFields[i]
                                .value.data)
                               ->name.length +
                           1);
                strncpy(data_temp[i],
                        ((UA_QualifiedName *)msg->data.keyFrameData.dataSetFields[i]
                             .value.data)
                            ->name.data,
                        ((UA_QualifiedName *)msg->data.keyFrameData.dataSetFields[i]
                             .value.data)
                            ->name.length);
                char *ptrQualified = data_temp[i];
                for(int x = 0;
                    x < ((UA_QualifiedName *)msg->data.keyFrameData.dataSetFields[i]
                             .value.data)
                            ->name.length;
                    x++)
                    ptrQualified++;
                *ptrQualified = '\0';
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            case(UA_Byte)UA_NS0ID_UTCTIME:;
                // Convert Datetime to it's original form and remove Double quotes
                UA_UtcTime *varcontentutc = UA_DateTime_new();
                *varcontentutc =
                    *((UA_UtcTime *)msg->data.keyFrameData.dataSetFields[i].value.data);
                const UA_DataType *typeutc = &UA_TYPES[UA_TYPES_DATETIME];

                size_t sizeutc = UA_calcSizeJson((void *)varcontentutc, typeutc, NULL);
                UA_ByteString receivedTimeUtc;

                UA_ByteString_allocBuffer(&receivedTimeUtc, sizeutc + 1);

                status sutc =
                    UA_encodeJson((void *)varcontentutc, typeutc, &receivedTimeUtc, NULL);
                char buffUtc[40];
                int sizeIndUtc;
                for(sizeIndUtc = 1; sizeIndUtc < sizeutc - 1; sizeIndUtc++) {
                    buffUtc[sizeIndUtc - 1] = receivedTimeUtc.data[sizeIndUtc];
                }
                buffUtc[sizeIndUtc - 1] = '\0';

                data_temp[i] = malloc(sizeIndUtc);
                strncpy(data_temp[i], buffUtc, sizeIndUtc);

                /*printf("\n\n\n\n\t\t\tMessage Received by
                 * subscriber:'%s'\n\n\n\n",data_temp[i]);*/
                break;
            case UA_NS0ID_XMLELEMENT:;
                data_temp[i] = malloc(
                    ((UA_XmlElement *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->length +
                    1);
                strncpy(
                    data_temp[i],
                    ((UA_XmlElement *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->data,
                    ((UA_XmlElement *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->length);
                char *ptrXml = data_temp[i];
                for(int x = 0;
                    x <
                    ((UA_XmlElement *)msg->data.keyFrameData.dataSetFields[i].value.data)
                        ->length;
                    x++)
                    ptrXml++;
                *ptrXml = '\0';
                /*printf("\n\n\n\n\t\t\tMessage Received by subscriber:
                 * '%s'\n\n\n\n",data_temp[i]);*/
                break;
            default:
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "%d This field does not exist!!", type);
                // printf("\n\n%d This field does not exist!!\n\n",type);
        }
    }
    if(dsr->config.publisherId.type == &UA_TYPES[UA_TYPES_STRING]) {
        Messages[messagesCaptured].publisherId =
            ((UA_String *)dsr->config.publisherId.data)->data;
        char *ptr = Messages[messagesCaptured].publisherId;
        for(int i = 0; i < ((UA_String *)dsr->config.publisherId.data)->length; i++)
            ptr++;
        *ptr = '\0';
    } else {
        char tempBuf[20];
        sprintf(tempBuf, "%d", *((UA_UInt16 *)dsr->config.publisherId.data));
        Messages[messagesCaptured].publisherId = malloc(strlen(tempBuf) + 1);
        strncpy(Messages[messagesCaptured].publisherId, tempBuf, strlen(tempBuf) + 1);
    }
    // Received messages are stored in an array along with its parameters
    // i.e., datasetWriterId, WriterGroupId and number of fields
    Messages[messagesCaptured].datasetWriterId = dsr->config.dataSetWriterId;
    Messages[messagesCaptured].writerGroupId = dsr->config.writerGroupId;
    Messages[messagesCaptured].fields = i;
    for(int j = 0; j < i; j++) {
        Messages[messagesCaptured].data[j] = data_temp[j];
    }
    messagesCaptured++;
    // Notify user once the messages are received
    if(messagesFlag == 0) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_NETWORK,
                       "Messages Received!!");
        messagesFlag = 1;
    }
    // Size of array(Messages[]) in which messages are stored is 300 for now
    // Once the JNI call is received to transfer messages received to Java, this array
    // will be cleared and "messagesCaptured" will be set to 0. Still, if there is any
    // delay in JNI call and messages stored in "Messages" array exceeds 300, reassign
    // "messagesCaptured" to 150, so that dataloss is reduced to 150 instead of 300. Also,
    // notify user to adjust(reduce) publishing interval to avoid data loss
    if(messagesCaptured >= 300) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "MESSAGES EXCEEDED LIMIT.Please adjust Publishing Interval to avoid "
                     "data loss!!!");
        messagesCaptured = 150;
    }
    processFlag = 1;

    /* Check and adjust the field count
     * TODO Throw an error if non-matching? */
    size_t fieldCount = msg->data.keyFrameData.fieldCount;
    if(dsr->config.dataSetMetaData.fieldsSize < fieldCount)
        fieldCount = dsr->config.dataSetMetaData.fieldsSize;

    if(dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize < fieldCount)
        fieldCount = dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize;

    /* Process message with fixed size fields (realtime capable) */
    if(dsr->linkedReaderGroup->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE) {
        DataSetReader_processFixedSize(server, dsr, msg, fieldCount);
#ifdef UA_ENABLE_PUBSUB_MONITORING
        UA_DataSetReader_checkMessageReceiveTimeout(server, dsr);
#endif
        return;
    }

    /* Write the message fields via the write service (non realtime) */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < fieldCount; i++) {
        if(!msg->data.keyFrameData.dataSetFields[i].hasValue)
            continue;

        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];

        UA_WriteValue writeVal;
        UA_WriteValue_init(&writeVal);
        writeVal.attributeId = tv->targetVariable.attributeId;
        writeVal.indexRange = tv->targetVariable.receiverIndexRange;
        writeVal.nodeId = tv->targetVariable.targetNodeId;
        writeVal.value = msg->data.keyFrameData.dataSetFields[i];
        Operation_Write(server, &server->adminSession, NULL, &writeVal, &res);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_INFO_READER(server->config.logging, dsr,
                               "Error writing KeyFrame field %u: %s",
                               (unsigned)i, UA_StatusCode_name(res));
    }

#ifdef UA_ENABLE_PUBSUB_MONITORING
    UA_DataSetReader_checkMessageReceiveTimeout(server, dsr);
#endif
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
    if(dsr->msgRcvTimeoutTimerRunning) {
        res = server->config.pubSubConfig.monitoringInterface.
            stopMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                           UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
        if(res == UA_STATUSCODE_GOOD) {
            dsr->msgRcvTimeoutTimerRunning = false;
        } else {
            UA_LOG_ERROR_READER(server->config.logging, dsr,
                                "DataSetReader '%.*s': stop receive timeout timer failed",
                                (int)dsr->config.name.length, dsr->config.name.data);
            UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR);
        }
    }

    /* Start message receive timeout timer */
    res = server->config.pubSubConfig.monitoringInterface.
        startMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                        UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
    if(res == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_READER(server->config.logging, dsr,
                            "Info: DataSetReader '%.*s': start receive timeout timer",
                            (int)dsr->config.name.length, dsr->config.name.data);
        dsr->msgRcvTimeoutTimerRunning = true;
    } else {
        UA_LOG_ERROR_READER(server->config.logging, dsr,
                            "Starting Message Receive Timeout timer failed.");
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR);
    }
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
                        "MessageReceiveTimeout occurred at DataSetReader "
                        "'%.*s': MessageReceiveTimeout = %f Timer Id = %u ",
                        (int)dsr->config.name.length, dsr->config.name.data,
                        dsr->config.messageReceiveTimeout,
                        (UA_UInt32) dsr->msgRcvTimeoutTimerId);

    UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR);
}
#endif /* UA_ENABLE_PUBSUB_MONITORING */

UA_StatusCode
UA_DataSetReader_prepareOffsetBuffer(UA_Server *server, UA_DataSetReader *reader,
                                     UA_ByteString *buf, size_t *pos) {
    UA_NetworkMessage *nm = (UA_NetworkMessage*)UA_calloc(1, sizeof(UA_NetworkMessage));
    if(!nm)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Decode using the non-rt decoding */
    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(buf, pos, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_NetworkMessage_clear(nm);
        UA_free(nm);
        return rv;
    }
    rv |= UA_NetworkMessage_decodePayload(buf, pos, nm, server->config.customDataTypes, &reader->config.dataSetMetaData);
    rv |= UA_NetworkMessage_decodeFooters(buf, pos, nm);
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
                                    UA_ByteString *buf) {
    size_t pos = 0;
    UA_StatusCode rv;
    if(!dsr->bufferedMessage.nm) {
        /* This is the first message being received for the RT fastpath.
         * Prepare the offset buffer. */
        rv = UA_DataSetReader_prepareOffsetBuffer(server, dsr, buf, &pos);
    } else {
        /* Decode with offset information and update the networkMessage */
        rv = UA_NetworkMessage_updateBufferedNwMessage(&dsr->bufferedMessage, buf, &pos);
    }
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_READER(server->config.logging, dsr,
                           "PubSub decoding failed. Could not decode with "
                           "status code %s.", UA_StatusCode_name(rv));
        return;
    }

    UA_DataSetReader_process(server, dsr,
                             dsr->bufferedMessage.nm->payload.dataSetPayload.dataSetMessages);
}

#include <open62541/plugin/log_stdout.h>

#include <jni.h>
#include "jni_pubsub_PubSubMain.h"

static int noMessageCount = 0;

// JNI CODE TO PASS ON MESSAGES FROM C TO JAVA
// User can put their package name in place of "jni_pubsub"
JNIEXPORT jobject JNICALL
Java_jni_pubsub_PubSubMain_getMessage(
    JNIEnv *env, jobject javaobj) {
    // Wait till message processing completes
    while(processFlag == 0)
        printf("");
    // Stop message processing till messages copied to temporary variable
    transferFlag = 0;

    // Copy the contents of messages array to a temporary array to reduce delay
    struct Node tempMessages[100];
    int tempCount = messagesCaptured;

    // Wait for 50 continuous iterations without any message received.
    // Once 50 iteration are complete, enable message not recieved flag.
    if(messagesCaptured == 0 && noMessageCount > 50) {
        messagesFlag = 0;
        noMessageCount = 0;
    } else if(messagesCaptured == 0 && noMessageCount < 51)
        noMessageCount++;
    else {
        if(messagesFlag == 0)
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK, "Messages Received!!");
        messagesFlag = 1;
        noMessageCount = 0;
    }

    for(int copyi = 0; copyi < tempCount; copyi++)
        tempMessages[copyi] = Messages[copyi];
    // Start message processing after creation of temporary variable
    messagesCaptured = 0;
    transferFlag = 1;

    // User has to create a "Messages.java" class in java project
    // This class should contain all the fields mentioned in Node structure, defined above 
    // and a constructor to set the received fields to appropriate variables
    // User has to provide path of that "Messages" class in <Provide class path>
    jclass messagesClass = (*env)->FindClass(
        env, "<Provide class path>/Messages");
    jclass arrayListcls = (*env)->FindClass(env, "Ljava/util/ArrayList;");
    jmethodID arrayListInit = (*env)->GetMethodID(env, arrayListcls, "<init>", "(I)V");
    jmethodID add =
        (*env)->GetMethodID(env, arrayListcls, "add", "(Ljava/lang/Object;)Z");

    jobject messagesArray =
        (*env)->NewObject(env, arrayListcls, arrayListInit, tempCount);

    if(tempCount > 0) {
        // printf("\n\n\t\tCreating Messages List for %d messages\n\n", tempCount);
        for(int messageIndex = 0; messageIndex < tempCount; messageIndex++) {
            jobject newMessages = (*env)->AllocObject(env, messagesClass);

            // Get the UserData fields to be set
            jobject arrayList = (*env)->NewObject(env, arrayListcls, arrayListInit,
                                                  tempMessages[messageIndex].fields);
            for(int dataIndex = 0; dataIndex < tempMessages[messageIndex].fields;
                dataIndex++) {
                (*env)->CallBooleanMethod(
                    env, arrayList, add,
                    (*env)->NewStringUTF(env,
                                         tempMessages[messageIndex].data[dataIndex]));
            }

            // Get FieldIDs to set respective fields
            jfieldID publisherId = (*env)->GetFieldID(env, messagesClass, "publisherId",
                                                      "Ljava/lang/String;");
            jfieldID datasetWriterId =
                (*env)->GetFieldID(env, messagesClass, "datasetWriterId", "I");
            jfieldID writerGroupId =
                (*env)->GetFieldID(env, messagesClass, "writerGroupId", "I");
            jfieldID fields = (*env)->GetFieldID(env, messagesClass, "fields", "I");
            jfieldID data =
                (*env)->GetFieldID(env, messagesClass, "data", "Ljava/util/ArrayList;");

            // Set Fields with FieldIDs
            (*env)->SetObjectField(
                env, newMessages, publisherId,
                (*env)->NewStringUTF(env, tempMessages[messageIndex].publisherId));
            (*env)->SetIntField(env, newMessages, datasetWriterId,
                                tempMessages[messageIndex].datasetWriterId);
            (*env)->SetIntField(env, newMessages, writerGroupId,
                                tempMessages[messageIndex].writerGroupId);
            (*env)->SetIntField(env, newMessages, fields,
                                tempMessages[messageIndex].fields);
            (*env)->SetObjectField(env, newMessages, data, arrayList);

            /*
            printf("\n\n\t\tMessage is
            DW:............%d\n\n",tempMessages[messageIndex].datasetWriterId);
            printf("\n\n\t\tMessage is
            DWG:............%d\n\n",tempMessages[messageIndex].writerGroupId);
            printf("\n\n\t\tMessage is F:............%d\n\n",

            tempMessages[messageIndex].fields);

            jmethodID string = (*env)->GetMethodID(env, messagesClass, "toString",
            "()Ljava/lang/String;"); jstring str = (*env)->CallObjectMethod(env,
            newMessages, string); char* strr = (char*)(*env)->GetStringUTFChars(env, str,
            NULL); printf("\n\n\nOBJECT CREATED: %s\n\n\n", strr);
            */

            (*env)->CallBooleanMethod(env, messagesArray, add, newMessages);

            (*env)->DeleteLocalRef(env, arrayList);
            (*env)->DeleteLocalRef(env, newMessages);
        }
    } else {

        if(messagesFlag == 0)
            UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_NETWORK,
                           "Messages not yet received.Please check your publisher or "
                           "publishing interval");
        messagesArray = NULL;
    }
    return messagesArray;
}


#endif /* UA_ENABLE_PUBSUB */
