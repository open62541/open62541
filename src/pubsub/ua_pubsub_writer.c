/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019-2021 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include "ua_pubsub.h"
#include "ua_pubsub_internal.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#include "ua_types_encoding_binary.h"

UA_StatusCode
UA_DataSetWriterConfig_copy(const UA_DataSetWriterConfig *src,
                            UA_DataSetWriterConfig *dst){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_DataSetWriterConfig));
    retVal |= UA_String_copy(&src->name, &dst->name);
    retVal |= UA_String_copy(&src->dataSetName, &dst->dataSetName);
    retVal |= UA_ExtensionObject_copy(&src->messageSettings, &dst->messageSettings);
    retVal |= UA_KeyValueMap_copy(&src->dataSetWriterProperties, &dst->dataSetWriterProperties);
    if(retVal != UA_STATUSCODE_GOOD)
        UA_DataSetWriterConfig_clear(dst);
    return retVal;
}

UA_StatusCode
UA_Server_getDataSetWriterConfig(UA_Server *server, const UA_NodeId dsw,
                                 UA_DataSetWriterConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockPubSubServer(server);
    UA_DataSetWriter *currentDataSetWriter = UA_DataSetWriter_findDSWbyId(server, dsw);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    if(currentDataSetWriter)
        res = UA_DataSetWriterConfig_copy(&currentDataSetWriter->config, config);
    unlockPubSubServer(server);
    return res;
}

UA_StatusCode
UA_Server_DataSetWriter_getState(UA_Server *server, UA_NodeId dataSetWriterIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockPubSubServer(server);
    UA_DataSetWriter *currentDataSetWriter =
        UA_DataSetWriter_findDSWbyId(server, dataSetWriterIdentifier);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(currentDataSetWriter) {
        *state = currentDataSetWriter->state;
    } else {
        res = UA_STATUSCODE_BADNOTFOUND;
    }
    unlockPubSubServer(server);
    return res;
}

UA_DataSetWriter *
UA_DataSetWriter_findDSWbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PubSubConnection *pubSubConnection;
    TAILQ_FOREACH(pubSubConnection, &server->pubSubManager.connections, listEntry){
        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &pubSubConnection->writerGroups, listEntry){
            UA_DataSetWriter *tmpWriter;
            LIST_FOREACH(tmpWriter, &tmpWriterGroup->writers, listEntry){
                if(UA_NodeId_equal(&tmpWriter->identifier, &identifier)){
                    return tmpWriter;
                }
            }
        }
    }
    return NULL;
}

void
UA_DataSetWriterConfig_clear(UA_DataSetWriterConfig *pdsConfig) {
    UA_String_clear(&pdsConfig->name);
    UA_String_clear(&pdsConfig->dataSetName);
    UA_KeyValueMap_clear(&pdsConfig->dataSetWriterProperties);
    UA_ExtensionObject_clear(&pdsConfig->messageSettings);
    memset(pdsConfig, 0, sizeof(UA_DataSetWriterConfig));
}

//state machine methods not part of the open62541 state machine API
UA_StatusCode
UA_DataSetWriter_setPubSubState(UA_Server *server,
                                UA_DataSetWriter *dataSetWriter,
                                UA_PubSubState state,
                                UA_StatusCode cause) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = dataSetWriter->state;
    switch(state){
        case UA_PUBSUBSTATE_DISABLED:
            switch (dataSetWriter->state){
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    dataSetWriter->state = UA_PUBSUBSTATE_DISABLED;
                    //no further action is required
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    dataSetWriter->state = UA_PUBSUBSTATE_DISABLED;
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING_WRITER(server->config.logging, dataSetWriter,
                                          "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_PAUSED:
            switch (dataSetWriter->state){
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING_WRITER(server->config.logging, dataSetWriter,
                                          "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            switch (dataSetWriter->state){
                case UA_PUBSUBSTATE_DISABLED:
                    dataSetWriter->state = UA_PUBSUBSTATE_OPERATIONAL;
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING_WRITER(server->config.logging, dataSetWriter,
                                          "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_ERROR:
            switch (dataSetWriter->state){
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING_WRITER(server->config.logging, dataSetWriter,
                                          "Received unknown PubSub state!");
            }
            break;
        default:
            UA_LOG_WARNING_WRITER(server->config.logging, dataSetWriter,
                                  "Received unknown PubSub state!");
    }
    if (state != oldState) {
        /* inform application about state change */
        UA_ServerConfig *pConfig = &server->config;
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            pConfig->pubSubConfig.
                stateChangeCallback(server, &dataSetWriter->identifier, state, cause);
        }
    }
    return ret;
}

UA_StatusCode
UA_DataSetWriter_create(UA_Server *server,
                        const UA_NodeId writerGroup, const UA_NodeId dataSet,
                        const UA_DataSetWriterConfig *dataSetWriterConfig,
                        UA_NodeId *writerIdentifier) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(!dataSetWriterConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Make checks for a heartbeat */
    if(UA_NodeId_isNull(&dataSet) && dataSetWriterConfig->keyFrameCount != 1) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                   "Adding DataSetWriter failed: DataSet can be null only for "
                                   "a heartbeat in which case KeyFrameCount shall be 1");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(wg->configurationFrozen) {
        UA_LOG_WARNING_WRITERGROUP(server->config.logging, wg,
                                   "Adding DataSetWriter failed: WriterGroup is frozen");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PublishedDataSet *currentDataSetContext = NULL;

    if(!UA_NodeId_isNull(&dataSet)) {
        currentDataSetContext = UA_PublishedDataSet_findPDSbyId(server, dataSet);
        if(!currentDataSetContext)
            return UA_STATUSCODE_BADNOTFOUND;

        if(currentDataSetContext->configurationFreezeCounter > 0) {
            UA_LOG_WARNING_DATASET(server->config.logging, currentDataSetContext,
                                   "Adding DataSetWriter failed: PublishedDataSet is frozen");
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }

        if(wg->config.rtLevel != UA_PUBSUB_RT_NONE) {
            UA_DataSetField *tmpDSF;
            TAILQ_FOREACH(tmpDSF, &currentDataSetContext->fields, listEntry) {
                if(!tmpDSF->config.field.variable.rtValueSource.rtFieldSourceEnabled &&
                   !tmpDSF->config.field.variable.rtValueSource.rtInformationModelNode) {
                    UA_LOG_WARNING_DATASET(server->config.logging, currentDataSetContext,
                                           "Adding DataSetWriter failed: "
                                           "Fields in PDS are not RT capable");
                    return UA_STATUSCODE_BADCONFIGURATIONERROR;
                }
            }
        }
    }

    UA_DataSetWriter *newDataSetWriter = (UA_DataSetWriter *)
        UA_calloc(1, sizeof(UA_DataSetWriter));
    if(!newDataSetWriter)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newDataSetWriter->componentType = UA_PUBSUB_COMPONENT_DATASETWRITER;

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(wg->state == UA_PUBSUBSTATE_OPERATIONAL) {
        res = UA_DataSetWriter_setPubSubState(server, newDataSetWriter,
                                              UA_PUBSUBSTATE_OPERATIONAL,
                                              UA_STATUSCODE_GOOD);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_WRITERGROUP(server->config.logging, wg,
                                     "Add DataSetWriter failed: setPubSubState failed");
            UA_free(newDataSetWriter);
            return res;
        }
    }

    /* Copy the config into the new dataSetWriter */
    res = UA_DataSetWriterConfig_copy(dataSetWriterConfig, &newDataSetWriter->config);
    UA_CHECK_STATUS(res, UA_free(newDataSetWriter); return res);

    if(!UA_NodeId_isNull(&dataSet) && currentDataSetContext != NULL) {
        /* Save the current version of the connected PublishedDataSet */
        newDataSetWriter->connectedDataSetVersion =
            currentDataSetContext->dataSetMetaData.configurationVersion;

        if(server->config.pubSubConfig.enableDeltaFrames) {
            /* Initialize the queue for the last values */
            if(currentDataSetContext->fieldSize > 0) {
                newDataSetWriter->lastSamples = (UA_DataSetWriterSample*)
                    UA_calloc(currentDataSetContext->fieldSize, sizeof(UA_DataSetWriterSample));
                if(!newDataSetWriter->lastSamples) {
                    UA_DataSetWriterConfig_clear(&newDataSetWriter->config);
                    UA_free(newDataSetWriter);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                newDataSetWriter->lastSamplesCount = currentDataSetContext->fieldSize;
                for(size_t i = 0; i < newDataSetWriter->lastSamplesCount; i++) {
                    UA_DataValue_init(&newDataSetWriter->lastSamples[i].value);
                    newDataSetWriter->lastSamples[i].valueChanged = false;
                }
            }
        }
        /* Connect PublishedDataSet with DataSetWriter */
        newDataSetWriter->connectedDataSet = currentDataSetContext->identifier;
    } else {
        /* If the dataSet is NULL, we are adding a heartbeat writer */
        newDataSetWriter->connectedDataSetVersion.majorVersion = 0;
        newDataSetWriter->connectedDataSetVersion.minorVersion = 0;
        newDataSetWriter->connectedDataSet = UA_NODEID_NULL;
    }

    newDataSetWriter->linkedWriterGroup = wg->identifier;

    /* Add the new writer to the group. Add to the end of the linked list to
     * ensure the order in the generated NetworkMessage is as expected. */
    UA_DataSetWriter *after = LIST_FIRST(&wg->writers);
    if(!after) {
        LIST_INSERT_HEAD(&wg->writers, newDataSetWriter, listEntry);
    } else {
        while(LIST_NEXT(after, listEntry))
            after = LIST_NEXT(after, listEntry);
        LIST_INSERT_AFTER(after, newDataSetWriter, listEntry);
    }
    wg->writersCount++;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    res |= addDataSetWriterRepresentation(server, newDataSetWriter);
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newDataSetWriter->identifier);
#endif
    if(writerIdentifier)
        UA_NodeId_copy(&newDataSetWriter->identifier, writerIdentifier);
    return res;
}

UA_StatusCode
UA_Server_addDataSetWriter(UA_Server *server,
                           const UA_NodeId writerGroup, const UA_NodeId dataSet,
                           const UA_DataSetWriterConfig *dataSetWriterConfig,
                           UA_NodeId *writerIdentifier) {
    lockPubSubServer(server);
    /* Delete the reserved IDs if the related session no longer exists. */
    UA_PubSubManager_freeIds(server);
    UA_StatusCode res = UA_DataSetWriter_create(server, writerGroup, dataSet,
                                                dataSetWriterConfig, writerIdentifier);
    unlockPubSubServer(server);
    return res;
}

void
UA_DataSetWriter_freezeConfiguration(UA_Server *server,
                                     UA_DataSetWriter *dsw) {
    UA_PublishedDataSet *pds =
        UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
    if(pds) { /* Skip for heartbeat writers */
        pds->configurationFreezeCounter++;
        UA_DataSetField *dsf;
        TAILQ_FOREACH(dsf, &pds->fields, listEntry) {
            dsf->configurationFrozen = true;
        }
    }
    dsw->configurationFrozen = true;
}

void
UA_DataSetWriter_unfreezeConfiguration(UA_Server *server,
                                       UA_DataSetWriter *dsw) {
    UA_PublishedDataSet *pds =
        UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
    if(pds) { /* Skip for heartbeat writers */
        pds->configurationFreezeCounter--;
        if(pds->configurationFreezeCounter == 0) {
            UA_DataSetField *dsf;
            TAILQ_FOREACH(dsf, &pds->fields, listEntry){
                dsf->configurationFrozen = false;
            }
        }
        dsw->configurationFrozen = false;
    }
}

UA_StatusCode
UA_DataSetWriter_prepareDataSet(UA_Server *server, UA_DataSetWriter *dsw,
                                UA_DataSetMessage *dsm) {
    /* No PublishedDataSet defined -> Heartbeat messages only */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(UA_NodeId_isNull(&dsw->connectedDataSet)) {
        res = UA_DataSetWriter_generateDataSetMessage(server, dsm, dsw);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_WRITER(server->config.logging, dsw,
                                  "PubSub-RT configuration fail: "
                                  "Heartbeat DataSetMessage creation failed");
        }
        return res;
    }

    /* Get the PublishedDataSet */
    UA_PublishedDataSet *pds =
        UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
    if(!pds) {
        UA_LOG_WARNING_WRITER(server->config.logging, dsw,
                              "PubSub-RT configuration fail: "
                              "PublishedDataSet not found");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, dsw->linkedWriterGroup);
    UA_assert(wg);

    /* Promoted Fields not allowed if RT is enabled */
    if(wg->config.rtLevel > UA_PUBSUB_RT_NONE &&
       pds->promotedFieldsCount > 0) {
        UA_LOG_WARNING_WRITER(server->config.logging, dsw,
                              "PubSub-RT configuration fail: "
                              "PDS contains promoted fields");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    /* Test the DataSetFields */
    UA_DataSetField *dsf;
    TAILQ_FOREACH(dsf, &pds->fields, listEntry) {
        UA_NodeId *publishedVariable =
            &dsf->config.field.variable.publishParameters.publishedVariable;

        /* Check that the target is a VariableNode */
        const UA_VariableNode *rtNode = (const UA_VariableNode*)
            UA_NODESTORE_GET(server, publishedVariable);
        if(rtNode && rtNode->head.nodeClass != UA_NODECLASS_VARIABLE) {
            UA_LOG_ERROR_WRITER(server->config.logging, dsw,
                                "PubSub-RT configuration fail: "
                                "PDS points to a node that is not a variable");
            UA_NODESTORE_RELEASE(server, (const UA_Node *)rtNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
        UA_NODESTORE_RELEASE(server, (const UA_Node *)rtNode);

        /* TODO: Get the External Value Source from the node instead of from the config */

        /* If direct-value-access is enabled, the pointers need to be set */
        if(wg->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS &&
           !dsf->config.field.variable.rtValueSource.rtFieldSourceEnabled) {
            UA_LOG_ERROR_WRITER(server->config.logging, dsw,
                                "PubSub-RT configuration fail: PDS published-variable "
                                "does not have an external data source");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }

        /* Check that the values have a fixed size if fixed offsets are needed */
        if(wg->config.rtLevel & UA_PUBSUB_RT_FIXED_SIZE) {
            if((UA_NodeId_equal(&dsf->fieldMetaData.dataType,
                                &UA_TYPES[UA_TYPES_STRING].typeId) ||
                UA_NodeId_equal(&dsf->fieldMetaData.dataType,
                                &UA_TYPES[UA_TYPES_BYTESTRING].typeId)) &&
               dsf->fieldMetaData.maxStringLength == 0) {
                UA_LOG_WARNING_WRITER(server->config.logging, dsw,
                                      "PubSub-RT configuration fail: "
                                      "PDS contains String/ByteString with dynamic length");
                return UA_STATUSCODE_BADNOTSUPPORTED;
            } else if(!UA_DataType_isNumeric(
                          UA_findDataType(&dsf->fieldMetaData.dataType)) &&
                      !UA_NodeId_equal(&dsf->fieldMetaData.dataType,
                                       &UA_TYPES[UA_TYPES_BOOLEAN].typeId)) {
                UA_LOG_WARNING_WRITER(server->config.logging, dsw,
                                      "PubSub-RT configuration fail: "
                                      "PDS contains variable with dynamic size");
                return UA_STATUSCODE_BADNOTSUPPORTED;
            }
        }
    }

    /* Generate the DSM */
    res = UA_DataSetWriter_generateDataSetMessage(server, dsm, dsw);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_WRITER(server->config.logging, dsw,
                              "PubSub-RT configuration fail: "
                              "DataSetMessage buffering failed");
    }

    return res;
}

UA_StatusCode
UA_DataSetWriter_remove(UA_Server *server, UA_DataSetWriter *dataSetWriter) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Frozen? */
    if(dataSetWriter->configurationFrozen) {
        UA_LOG_WARNING_WRITER(server->config.logging, dataSetWriter,
                              "Remove DataSetWriter failed: WriterGroup is frozen");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Remove from information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, dataSetWriter->identifier, true);
#endif

    /* Remove DataSetWriter from group */
    UA_WriterGroup *linkedWriterGroup =
        UA_WriterGroup_findWGbyId(server, dataSetWriter->linkedWriterGroup);
    if(linkedWriterGroup) {
        LIST_REMOVE(dataSetWriter, listEntry);
        linkedWriterGroup->writersCount--;
    }

    UA_DataSetWriterConfig_clear(&dataSetWriter->config);
    UA_NodeId_clear(&dataSetWriter->identifier);
    UA_NodeId_clear(&dataSetWriter->linkedWriterGroup);
    UA_NodeId_clear(&dataSetWriter->connectedDataSet);

    if(server->config.pubSubConfig.enableDeltaFrames) {
        /* Delete lastSamples store */
        for(size_t i = 0; i < dataSetWriter->lastSamplesCount; i++) {
            UA_DataValue_clear(&dataSetWriter->lastSamples[i].value);
        }
        UA_free(dataSetWriter->lastSamples);
        dataSetWriter->lastSamples = NULL;
        dataSetWriter->lastSamplesCount = 0;
    }

    UA_free(dataSetWriter);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeDataSetWriter(UA_Server *server, const UA_NodeId dsw) {
    lockPubSubServer(server);
    UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_findDSWbyId(server, dsw);
    if(!dataSetWriter) {
        unlockPubSubServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_StatusCode res = UA_DataSetWriter_remove(server, dataSetWriter);
    unlockPubSubServer(server);
    return res;
}

/*********************************************************/
/*               PublishValues handling                  */
/*********************************************************/

/* Compare two variants. Internally used for value change detection. */
static UA_Boolean
valueChangedVariant(UA_Variant *oldValue, UA_Variant *newValue) {
    if(!oldValue || !newValue)
        return false;

    size_t oldValueEncodingSize = UA_calcSizeBinary(oldValue, &UA_TYPES[UA_TYPES_VARIANT]);
    size_t newValueEncodingSize = UA_calcSizeBinary(newValue, &UA_TYPES[UA_TYPES_VARIANT]);
    if(oldValueEncodingSize == 0 || newValueEncodingSize == 0)
        return false;

    if(oldValueEncodingSize != newValueEncodingSize)
        return true;

    UA_ByteString oldValueEncoding = UA_BYTESTRING_NULL;
    UA_StatusCode res = UA_ByteString_allocBuffer(&oldValueEncoding, oldValueEncodingSize);
    if(res != UA_STATUSCODE_GOOD)
        return false;

    UA_ByteString newValueEncoding = UA_BYTESTRING_NULL;
    res = UA_ByteString_allocBuffer(&newValueEncoding, newValueEncodingSize);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&oldValueEncoding);
        return false;
    }

    UA_Byte *bufPosOldValue = oldValueEncoding.data;
    const UA_Byte *bufEndOldValue = &oldValueEncoding.data[oldValueEncoding.length];
    UA_Byte *bufPosNewValue = newValueEncoding.data;
    const UA_Byte *bufEndNewValue = &newValueEncoding.data[newValueEncoding.length];

    UA_Boolean compareResult = false; /* default */

    res = UA_encodeBinaryInternal(oldValue, &UA_TYPES[UA_TYPES_VARIANT],
                                  &bufPosOldValue, &bufEndOldValue, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    res = UA_encodeBinaryInternal(newValue, &UA_TYPES[UA_TYPES_VARIANT],
                                  &bufPosNewValue, &bufEndNewValue, NULL, NULL);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    oldValueEncoding.length = (uintptr_t)bufPosOldValue - (uintptr_t)oldValueEncoding.data;
    newValueEncoding.length = (uintptr_t)bufPosNewValue - (uintptr_t)newValueEncoding.data;
    compareResult = !UA_ByteString_equal(&oldValueEncoding, &newValueEncoding);

 cleanup:
    UA_ByteString_clear(&oldValueEncoding);
    UA_ByteString_clear(&newValueEncoding);
    return compareResult;
}

static UA_StatusCode
UA_PubSubDataSetWriter_generateKeyFrameMessage(UA_Server *server,
                                               UA_DataSetMessage *dataSetMessage,
                                               UA_DataSetWriter *dataSetWriter) {
    UA_PublishedDataSet *currentDataSet =
        UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
    if(!currentDataSet)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Prepare DataSetMessageContent */
    dataSetMessage->header.dataSetMessageValid = true;
    dataSetMessage->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dataSetMessage->data.keyFrameData.fieldCount = currentDataSet->fieldSize;
    dataSetMessage->data.keyFrameData.dataSetFields = (UA_DataValue *)
            UA_Array_new(currentDataSet->fieldSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
    dataSetMessage->data.keyFrameData.dataSetMetaDataType = &pds->dataSetMetaData;
    if(!dataSetMessage->data.keyFrameData.dataSetFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

#ifdef UA_ENABLE_JSON_ENCODING
    dataSetMessage->data.keyFrameData.fieldNames = (UA_String *)
        UA_Array_new(currentDataSet->fieldSize, &UA_TYPES[UA_TYPES_STRING]);
    if(!dataSetMessage->data.keyFrameData.fieldNames) {
        UA_DataSetMessage_clear(dataSetMessage);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
#endif

    /* Loop over the fields */
    size_t counter = 0;
    UA_DataSetField *dsf;
    TAILQ_FOREACH(dsf, &currentDataSet->fields, listEntry) {
#ifdef UA_ENABLE_JSON_ENCODING
        /* Set the field name alias */
        UA_String_copy(&dsf->config.field.variable.fieldNameAlias,
                       &dataSetMessage->data.keyFrameData.fieldNames[counter]);
#endif

        /* Sample the value */
        UA_DataValue *dfv = &dataSetMessage->data.keyFrameData.dataSetFields[counter];
        UA_PubSubDataSetField_sampleValue(server, dsf, dfv);

        /* Deactivate statuscode? */
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
            dfv->hasStatus = false;

        /* Deactivate timestamps */
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP) == 0)
            dfv->hasSourceTimestamp = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS) == 0)
            dfv->hasSourcePicoseconds = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERTIMESTAMP) == 0)
            dfv->hasServerTimestamp = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS) == 0)
            dfv->hasServerPicoseconds = false;

        if(server->config.pubSubConfig.enableDeltaFrames) {
            /* Update lastValue store */
            UA_DataValue_clear(&dataSetWriter->lastSamples[counter].value);
            UA_DataValue_copy(dfv, &dataSetWriter->lastSamples[counter].value);
        }
        counter++;
    }
    return UA_STATUSCODE_GOOD;
}

/* the input message is already initialized and that the method 
 * must not be called twice for the same message */
static UA_StatusCode
UA_PubSubDataSetWriter_generateDeltaFrameMessage(UA_Server *server,
                                                 UA_DataSetMessage *dataSetMessage,
                                                 UA_DataSetWriter *dataSetWriter) {
    UA_PublishedDataSet *currentDataSet =
        UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
    if(!currentDataSet)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Prepare DataSetMessageContent */
    dataSetMessage->header.dataSetMessageValid = true;
    dataSetMessage->header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    if(currentDataSet->fieldSize == 0)
        return UA_STATUSCODE_GOOD;

    UA_DataSetField *dsf;
    UA_UInt16 counter = 0;
    TAILQ_FOREACH(dsf, &currentDataSet->fields, listEntry) {
        /* Sample the value */
        UA_DataValue value;
        UA_DataValue_init(&value);
        UA_PubSubDataSetField_sampleValue(server, dsf, &value);

        /* Check if the value has changed */
        UA_DataSetWriterSample *ls = &dataSetWriter->lastSamples[counter];
        if(valueChangedVariant(&ls->value.value, &value.value)) {
            /* increase fieldCount for current delta message */
            dataSetMessage->data.deltaFrameData.fieldCount++;
            ls->valueChanged = true;

            /* Update last stored sample */
            UA_DataValue_clear(&ls->value);
            ls->value = value;
        } else {
            UA_DataValue_clear(&value);
            ls->valueChanged = false;
        }

        counter++;
    }

    /* Allocate DeltaFrameFields */
    UA_DataSetMessage_DeltaFrameField *deltaFields = (UA_DataSetMessage_DeltaFrameField *)
        UA_calloc(counter, sizeof(UA_DataSetMessage_DeltaFrameField));
    if(!deltaFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dataSetMessage->data.deltaFrameData.deltaFrameFields = deltaFields;
    dataSetMessage->data.deltaFrameData.fieldCount = counter;

    size_t currentDeltaField = 0;
    for(size_t i = 0; i < currentDataSet->fieldSize; i++) {
        if(!dataSetWriter->lastSamples[i].valueChanged)
            continue;

        UA_DataSetMessage_DeltaFrameField *dff = &deltaFields[currentDeltaField];

        dff->fieldIndex = (UA_UInt16) i;
        UA_DataValue_copy(&dataSetWriter->lastSamples[i].value, &dff->fieldValue);

        /* Reset the changed flag */
        dataSetWriter->lastSamples[i].valueChanged = false;

        /* Deactivate statuscode? */
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
            dff->fieldValue.hasStatus = false;

        /* Deactivate timestamps? */
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP) == 0)
            dff->fieldValue.hasSourceTimestamp = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS) == 0)
            dff->fieldValue.hasServerPicoseconds = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERTIMESTAMP) == 0)
            dff->fieldValue.hasServerTimestamp = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS) == 0)
            dff->fieldValue.hasServerPicoseconds = false;

        currentDeltaField++;
    }
    return UA_STATUSCODE_GOOD;
}

/* Generate a DataSetMessage for the given writer. */
UA_StatusCode
UA_DataSetWriter_generateDataSetMessage(UA_Server *server,
                                        UA_DataSetMessage *dataSetMessage,
                                        UA_DataSetWriter *dataSetWriter) {
    UA_Boolean heartbeat = false;
    UA_PublishedDataSet *currentDataSet = NULL;
    if(UA_NodeId_isNull(&dataSetWriter->connectedDataSet)){
        heartbeat = true;
    } else {
        currentDataSet =
            UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
        if(!currentDataSet){
            return UA_STATUSCODE_BADNOTFOUND;
        }
    }

    /* Reset the message */
    memset(dataSetMessage, 0, sizeof(UA_DataSetMessage));

    /* The configuration Flags are included
     * inside the std. defined UA_UadpDataSetWriterMessageDataType */
    UA_UadpDataSetWriterMessageDataType defaultUadpConfiguration;
    UA_UadpDataSetWriterMessageDataType *dsm = NULL;
    UA_JsonDataSetWriterMessageDataType *jsonDsm = NULL;
    const UA_ExtensionObject *ms = &dataSetWriter->config.messageSettings;
    if((ms->encoding == UA_EXTENSIONOBJECT_DECODED ||
        ms->encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) &&
       ms->content.decoded.type == &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE]) {
        dsm = (UA_UadpDataSetWriterMessageDataType*)ms->content.decoded.data; /* type is UADP */
    } else if((ms->encoding == UA_EXTENSIONOBJECT_DECODED ||
               ms->encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) &&
              ms->content.decoded.type == &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE]) {
        jsonDsm = (UA_JsonDataSetWriterMessageDataType*)ms->content.decoded.data; /* type is JSON */
    } else {
        /* Create default flag configuration if no
         * UadpDataSetWriterMessageDataType was passed in */
        memset(&defaultUadpConfiguration, 0, sizeof(UA_UadpDataSetWriterMessageDataType));
        defaultUadpConfiguration.dataSetMessageContentMask = (UA_UadpDataSetMessageContentMask)
            ((u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION);
        dsm = &defaultUadpConfiguration; /* type is UADP */
    }

    /* The field encoding depends on the flags inside the writer config. */
    if(dataSetWriter->config.dataSetFieldContentMask &
       (u64)UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_RAWDATA;
    } else if((u64)dataSetWriter->config.dataSetFieldContentMask &
              ((u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP |
               (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE)) {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    } else {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    }

    if(dsm) {
        /* Sanity-test the configuration */
        if(dsm->networkMessageNumber != 0 ||
           dsm->dataSetOffset != 0 ||
           dsm->configuredSize != 0) {
            UA_LOG_WARNING_WRITER(server->config.logging, dataSetWriter,
                                  "Static DSM configuration not supported, using defaults");
            dsm->networkMessageNumber = 0;
            dsm->dataSetOffset = 0;
          //  dsm->configuredSize = 0;
        }

        /* setting configured size in the dataSetMessage to add padding later on */
        dataSetMessage->configuredSize = dsm->configuredSize;

        /* Std: 'The DataSetMessageContentMask defines the flags for the content
         * of the DataSetMessage header.' */
        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION) {
            dataSetMessage->header.configVersionMajorVersionEnabled = true;
            if(heartbeat){
                dataSetMessage->header.configVersionMajorVersion = 0;
            } else {
                dataSetMessage->header.configVersionMajorVersion =
                    currentDataSet->dataSetMetaData.configurationVersion.majorVersion;
            }
        }
        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION) {
            dataSetMessage->header.configVersionMinorVersionEnabled = true;
            if(heartbeat){
                dataSetMessage->header.configVersionMinorVersion = 0;
            } else {
                dataSetMessage->header.configVersionMinorVersion =
                    currentDataSet->dataSetMetaData.configurationVersion.minorVersion;
            }
        }

        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
            dataSetMessage->header.dataSetMessageSequenceNrEnabled = true;
            dataSetMessage->header.dataSetMessageSequenceNr =
                dataSetWriter->actualDataSetMessageSequenceCount;
        }

        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP) {
            dataSetMessage->header.timestampEnabled = true;
            dataSetMessage->header.timestamp = UA_DateTime_now();
        }

        /* TODO: Picoseconds resolution not supported atm */
        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_PICOSECONDS) {
            dataSetMessage->header.picoSecondsIncluded = false;
        }

        /* TODO: Statuscode not supported yet */
        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_STATUS) {
            dataSetMessage->header.statusEnabled = true;
        }
    } else if(jsonDsm) {
        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION) {
            dataSetMessage->header.configVersionMajorVersionEnabled = true;
            dataSetMessage->header.configVersionMinorVersionEnabled = true;
            if(heartbeat){
                dataSetMessage->header.configVersionMajorVersion = 0;
                dataSetMessage->header.configVersionMinorVersion = 0;
            } else {
                dataSetMessage->header.configVersionMajorVersion =
                    currentDataSet->dataSetMetaData.configurationVersion.majorVersion;
                dataSetMessage->header.configVersionMinorVersion =
                    currentDataSet->dataSetMetaData.configurationVersion.minorVersion;
            }
       }

        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
            dataSetMessage->header.dataSetMessageSequenceNrEnabled = true;
            dataSetMessage->header.dataSetMessageSequenceNr =
                dataSetWriter->actualDataSetMessageSequenceCount;
        }

        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_TIMESTAMP) {
            dataSetMessage->header.timestampEnabled = true;
            dataSetMessage->header.timestamp = UA_DateTime_now();
        }

        /* TODO: Statuscode not supported yet */
        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_STATUS) {
            dataSetMessage->header.statusEnabled = true;
        }
    }

    /* Set the sequence count. Automatically rolls over to zero */
    dataSetWriter->actualDataSetMessageSequenceCount++;

    if(heartbeat) {
        /* Prepare DataSetMessageContent */
        dataSetMessage->header.dataSetMessageValid = true;
        dataSetMessage->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
        dataSetMessage->data.keyFrameData.fieldCount = 0;
        return UA_STATUSCODE_GOOD;
    }

    /* JSON does not differ between deltaframes and keyframes, only keyframes
     * are currently used. */
    if(dsm && server->config.pubSubConfig.enableDeltaFrames) {
        /* Check if the PublishedDataSet version has changed -> if yes flush the
         * lastValue store and send a KeyFrame */
        if(dataSetWriter->connectedDataSetVersion.majorVersion !=
           currentDataSet->dataSetMetaData.configurationVersion.majorVersion ||
           dataSetWriter->connectedDataSetVersion.minorVersion !=
           currentDataSet->dataSetMetaData.configurationVersion.minorVersion) {
            /* Remove old samples */
            for(size_t i = 0; i < dataSetWriter->lastSamplesCount; i++)
                UA_DataValue_clear(&dataSetWriter->lastSamples[i].value);

            /* Realloc PDS dependent memory */
            dataSetWriter->lastSamplesCount = currentDataSet->fieldSize;
            UA_DataSetWriterSample *newSamplesArray = (UA_DataSetWriterSample * )
                UA_realloc(dataSetWriter->lastSamples,
                           sizeof(UA_DataSetWriterSample) * dataSetWriter->lastSamplesCount);
            if(!newSamplesArray)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            dataSetWriter->lastSamples = newSamplesArray;
            memset(dataSetWriter->lastSamples, 0,
                   sizeof(UA_DataSetWriterSample) * dataSetWriter->lastSamplesCount);

            dataSetWriter->connectedDataSetVersion =
                currentDataSet->dataSetMetaData.configurationVersion;
            UA_PubSubDataSetWriter_generateKeyFrameMessage(server, dataSetMessage,
                                                           dataSetWriter);
            dataSetWriter->deltaFrameCounter = 0;
            return UA_STATUSCODE_GOOD;
        }

        /* The standard defines: if a PDS contains only one fields no delta messages
         * should be generated because they need more memory than a keyframe with 1
         * field. */
        if(currentDataSet->fieldSize > 1 && dataSetWriter->deltaFrameCounter > 0 &&
           dataSetWriter->deltaFrameCounter <= dataSetWriter->config.keyFrameCount) {
            UA_PubSubDataSetWriter_generateDeltaFrameMessage(server, dataSetMessage,
                                                             dataSetWriter);
            dataSetWriter->deltaFrameCounter++;
            return UA_STATUSCODE_GOOD;
        }

        dataSetWriter->deltaFrameCounter = 1;
    }

    return UA_PubSubDataSetWriter_generateKeyFrameMessage(server, dataSetMessage,
                                                          dataSetWriter);
}

#endif /* UA_ENABLE_PUBSUB */
