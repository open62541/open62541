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
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
#include "ua_types_encoding_binary.h"
#endif

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
    UA_LOCK(&server->serviceMutex);
    UA_DataSetWriter *currentDataSetWriter = UA_DataSetWriter_findDSWbyId(server, dsw);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    if(currentDataSetWriter)
        res = UA_DataSetWriterConfig_copy(&currentDataSetWriter->config, config);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_DataSetWriter_getState(UA_Server *server, UA_NodeId dataSetWriterIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_DataSetWriter *currentDataSetWriter =
        UA_DataSetWriter_findDSWbyId(server, dataSetWriterIdentifier);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(currentDataSetWriter) {
        *state = currentDataSetWriter->state;
    } else {
        res = UA_STATUSCODE_BADNOTFOUND;
    }
    UA_UNLOCK(&server->serviceMutex);
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

static void
UA_DataSetWriter_clear(UA_Server *server, UA_DataSetWriter *dataSetWriter) {
    UA_DataSetWriterConfig_clear(&dataSetWriter->config);
    UA_NodeId_clear(&dataSetWriter->identifier);
    UA_NodeId_clear(&dataSetWriter->linkedWriterGroup);
    UA_NodeId_clear(&dataSetWriter->connectedDataSet);

    /* Delete lastSamples store */
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
    for(size_t i = 0; i < dataSetWriter->lastSamplesCount; i++) {
        UA_DataValue_clear(&dataSetWriter->lastSamples[i].value);
    }
    UA_free(dataSetWriter->lastSamples);
    dataSetWriter->lastSamples = NULL;
    dataSetWriter->lastSamplesCount = 0;
#endif
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
                    UA_LOG_WARNING_WRITER(&server->config.logger, dataSetWriter,
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
                    UA_LOG_WARNING_WRITER(&server->config.logger, dataSetWriter,
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
                    UA_LOG_WARNING_WRITER(&server->config.logger, dataSetWriter,
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
                    UA_LOG_WARNING_WRITER(&server->config.logger, dataSetWriter,
                                          "Received unknown PubSub state!");
            }
            break;
        default:
            UA_LOG_WARNING_WRITER(&server->config.logger, dataSetWriter,
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
        UA_LOG_WARNING_WRITERGROUP(&server->config.logger, wg,
                                   "Adding DataSetWriter failed: DataSet can be null only for "
                                   "a heartbeat in which case KeyFrameCount shall be 1");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(wg->configurationFrozen) {
        UA_LOG_WARNING_WRITERGROUP(&server->config.logger, wg,
                                   "Adding DataSetWriter failed: WriterGroup is frozen");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PublishedDataSet *currentDataSetContext = NULL;

    if(!UA_NodeId_isNull(&dataSet)) {
        currentDataSetContext = UA_PublishedDataSet_findPDSbyId(server, dataSet);
        if(!currentDataSetContext)
            return UA_STATUSCODE_BADNOTFOUND;

        if(currentDataSetContext->configurationFrozen) {
            UA_LOG_WARNING_DATASET(&server->config.logger, currentDataSetContext,
                                   "Adding DataSetWriter failed: PublishedDataSet is frozen");
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }

        if(wg->config.rtLevel != UA_PUBSUB_RT_NONE) {
            UA_DataSetField *tmpDSF;
            TAILQ_FOREACH(tmpDSF, &currentDataSetContext->fields, listEntry) {
                if(!tmpDSF->config.field.variable.rtValueSource.rtFieldSourceEnabled &&
                   !tmpDSF->config.field.variable.rtValueSource.rtInformationModelNode) {
                    UA_LOG_WARNING_DATASET(&server->config.logger, currentDataSetContext,
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
            UA_LOG_ERROR_WRITERGROUP(&server->config.logger, wg,
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

    #ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
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
    #endif

        /* Connect PublishedDataSet with DataSetWriter */
        newDataSetWriter->connectedDataSet = currentDataSetContext->identifier;
    } else {
        /* If the dataSet is NULL, we are adding a heartbeat writer */
        newDataSetWriter->connectedDataSetVersion.majorVersion = 0;
        newDataSetWriter->connectedDataSetVersion.minorVersion = 0;
        newDataSetWriter->connectedDataSet = UA_NODEID_NULL;
    }

    newDataSetWriter->linkedWriterGroup = wg->identifier;

    /* Add the new writer to the group */
    LIST_INSERT_HEAD(&wg->writers, newDataSetWriter, listEntry);
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
    UA_LOCK(&server->serviceMutex);
    /* Delete the reserved IDs if the related session no longer exists. */
    UA_PubSubManager_freeIds(server);
    UA_StatusCode res = UA_DataSetWriter_create(server, writerGroup, dataSet,
                                                dataSetWriterConfig, writerIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_DataSetWriter_remove(UA_Server *server, UA_WriterGroup *linkedWriterGroup,
                        UA_DataSetWriter *dataSetWriter) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Frozen? */
    if(linkedWriterGroup->configurationFrozen) {
        UA_LOG_WARNING_WRITER(&server->config.logger, dataSetWriter,
                              "Remove DataSetWriter failed: WriterGroup is frozen");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Remove from information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, dataSetWriter->identifier, true);
#endif

    /* Remove DataSetWriter from group */
    UA_DataSetWriter_clear(server, dataSetWriter);
    LIST_REMOVE(dataSetWriter, listEntry);
    linkedWriterGroup->writersCount--;
    UA_free(dataSetWriter);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
removeDataSetWriter(UA_Server *server, const UA_NodeId dsw) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_findDSWbyId(server, dsw);
    if(!dataSetWriter)
        return UA_STATUSCODE_BADNOTFOUND;

    if(dataSetWriter->configurationFrozen) {
        UA_LOG_WARNING_WRITER(&server->config.logger, dataSetWriter,
                              "Remove DataSetWriter failed: DataSetWriter is frozen");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_WriterGroup *linkedWriterGroup =
        UA_WriterGroup_findWGbyId(server, dataSetWriter->linkedWriterGroup);
    if(!linkedWriterGroup)
        return UA_STATUSCODE_BADNOTFOUND;

    return UA_DataSetWriter_remove(server, linkedWriterGroup, dataSetWriter);
}

UA_StatusCode
UA_Server_removeDataSetWriter(UA_Server *server, const UA_NodeId dsw) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = removeDataSetWriter(server, dsw);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

/*********************************************************/
/*               PublishValues handling                  */
/*********************************************************/

/* Compare two variants. Internally used for value change detection. */
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
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
#endif

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

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
        /* Update lastValue store */
        UA_DataValue_clear(&dataSetWriter->lastSamples[counter].value);
        UA_DataValue_copy(dfv, &dataSetWriter->lastSamples[counter].value);
#endif

        counter++;
    }
    return UA_STATUSCODE_GOOD;
}

/* the input message is already initialized and that the method 
 * must not be called twice for the same message */
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
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
    size_t counter = 0;
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
        UA_calloc(dataSetMessage->data.deltaFrameData.fieldCount,
                  sizeof(UA_DataSetMessage_DeltaFrameField));
    if(!deltaFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dataSetMessage->data.deltaFrameData.deltaFrameFields = deltaFields;
    size_t currentDeltaField = 0;
    for(size_t i = 0; i < currentDataSet->fieldSize; i++) {
        if(!dataSetWriter->lastSamples[i].valueChanged)
            continue;

        UA_DataSetMessage_DeltaFrameField *dff = &deltaFields[currentDeltaField];

        dff->fieldIndex = (UA_UInt16) i;
        UA_DataValue_copy(&dataSetWriter->lastSamples[i].value, &dff->fieldValue);
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
#endif

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
            UA_LOG_WARNING_WRITER(&server->config.logger, dataSetWriter,
                                  "Static DSM configuration not supported, using defaults");
            dsm->networkMessageNumber = 0;
            dsm->dataSetOffset = 0;
            dsm->configuredSize = 0;
        }

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
            if(heartbeat){
                dataSetMessage->header.configVersionMajorVersion = 0;
            } else {
                dataSetMessage->header.configVersionMajorVersion =
                currentDataSet->dataSetMetaData.configurationVersion.majorVersion;
            }
       }
        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION) {
            dataSetMessage->header.configVersionMinorVersionEnabled = true;
            if(heartbeat){
                dataSetMessage->header.configVersionMinorVersion = 0;
            } else {
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
    if(dsm) {
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
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
#endif
    }

<<<<<<< HEAD
    return UA_PubSubDataSetWriter_generateKeyFrameMessage(server, dataSetMessage,
                                                          dataSetWriter);
=======
    return UA_PubSubDataSetWriter_generateKeyFrameMessage(server, dataSetMessage, dataSetWriter);
}

static UA_StatusCode
sendNetworkMessageJson(UA_PubSubConnection *connection, UA_DataSetMessage *dsm,
                       UA_UInt16 *writerIds, UA_Byte dsmCount,
                       UA_ExtensionObject *transportSettings) {
   UA_StatusCode retval = UA_STATUSCODE_BADNOTSUPPORTED;
#ifdef UA_ENABLE_JSON_ENCODING
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    nm.version = 1;
    nm.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    nm.payloadHeaderEnabled = true;

    nm.payloadHeader.dataSetPayloadHeader.count = dsmCount;
    nm.payloadHeader.dataSetPayloadHeader.dataSetWriterIds = writerIds;
    nm.payload.dataSetPayload.dataSetMessages = dsm;

    /* Allocate the buffer. Allocate on the stack if the buffer is small. */
    UA_ByteString buf;
    size_t msgSize = UA_NetworkMessage_calcSizeJson(&nm, NULL, 0, NULL, 0, true);
    size_t stackSize = 1;
    if(msgSize <= UA_MAX_STACKBUF)
        stackSize = msgSize;
    UA_STACKARRAY(UA_Byte, stackBuf, stackSize);
    buf.data = stackBuf;
    buf.length = msgSize;
    if(msgSize > UA_MAX_STACKBUF) {
        retval = UA_ByteString_allocBuffer(&buf, msgSize);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* Encode the message */
    UA_Byte *bufPos = buf.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &buf.data[buf.length];
    retval = UA_NetworkMessage_encodeJson(&nm, &bufPos, &bufEnd, NULL, 0, NULL, 0, true);
    if(retval != UA_STATUSCODE_GOOD) {
        if(msgSize > UA_MAX_STACKBUF)
            UA_ByteString_clear(&buf);
        return retval;
    }

    /* Send the prepared messages */
    retval = connection->channel->send(connection->channel, transportSettings, &buf);
    if(msgSize > UA_MAX_STACKBUF)
        UA_ByteString_clear(&buf);
#endif
    return retval;
}

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
    networkMessage->version = 1;
    networkMessage->networkMessageType = UA_NETWORKMESSAGE_DATASET;
    if(connection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_NUMERIC) {
        networkMessage->publisherIdType = UA_PUBLISHERDATATYPE_UINT16;
        networkMessage->publisherId.publisherIdUInt32 = connection->config->publisherId.numeric;
    } else if(connection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_STRING){
        networkMessage->publisherIdType = UA_PUBLISHERDATATYPE_STRING;
        networkMessage->publisherId.publisherIdString = connection->config->publisherId.string;
    }
    if(networkMessage->groupHeader.sequenceNumberEnabled)
        networkMessage->groupHeader.sequenceNumber = wg->sequenceNumber;
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
sendBufferedNetworkMessage(UA_Server *server, UA_PubSubConnection *connection,
                           UA_NetworkMessageOffsetBuffer *buffer,
                           UA_ExtensionObject *transportSettings) {
    if(UA_NetworkMessage_updateBufferedMessage(buffer) != UA_STATUSCODE_GOOD)
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub sending. Unknown field type.");
    return connection->channel->send(connection->channel,
                                     transportSettings, &buffer->buffer);
}

static UA_StatusCode
sendNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                   UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                   UA_ExtensionObject *messageSettings,
                   UA_ExtensionObject *transportSettings) {
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    generateNetworkMessage(connection, wg, dsm, writerIds, dsmCount,
                           messageSettings, transportSettings, &nm);

    /* Allocate the buffer. Allocate on the stack if the buffer is small. */
    UA_ByteString buf;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&nm, NULL);
    size_t stackSize = 1;
    if(msgSize <= UA_MAX_STACKBUF)
        stackSize = msgSize;
    UA_STACKARRAY(UA_Byte, stackBuf, stackSize);
    buf.data = stackBuf;
    buf.length = msgSize;
    UA_StatusCode retval;
    if(msgSize > UA_MAX_STACKBUF) {
        retval = UA_ByteString_allocBuffer(&buf, msgSize);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
    }

    /* Encode the message */
    UA_Byte *bufPos = buf.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &buf.data[buf.length];
    retval = UA_NetworkMessage_encodeBinary(&nm, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        if(msgSize > UA_MAX_STACKBUF)
            UA_ByteString_clear(&buf);
        goto cleanup;
    }

    /* Send the prepared messages */
    retval = connection->channel->send(connection->channel, transportSettings, &buf);
    if(msgSize > UA_MAX_STACKBUF)
        UA_ByteString_clear(&buf);

cleanup:
    UA_free(nm.payload.dataSetPayload.sizes);
    return retval;
}

/* This callback triggers the collection and publish of NetworkMessages and the
 * contained DataSetMessages. */
void
UA_WriterGroup_publishCallback(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER, "Publish Callback");
    UA_DateTime currentTime = UA_DateTime_nowMonotonic();

    if(!writerGroup) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Publish failed. WriterGroup not found");
        return;
    }

    /* Nothing to do? */
    if(writerGroup->writersCount <= 0)
        return;

    /* Binary or Json encoding?  */
    if(writerGroup->config.encodingMimeType != UA_PUBSUB_ENCODING_UADP &&
       writerGroup->config.encodingMimeType != UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Publish failed: Unknown encoding type.");
        UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_ERROR, writerGroup);
        return;
    }

    /* Find the connection associated with the writer */
    UA_PubSubConnection *connection = writerGroup->linkedConnection;
    if(!connection) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Publish failed. PubSubConnection invalid.");
        UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_ERROR, writerGroup);
        return;
    }

    UA_Int64 interval = (UA_Int64)(writerGroup->config.publishingInterval * UA_DATETIME_MSEC);
    if(writerGroup->config.baseTime) {
        if(writerGroup->callbackTime == NULL) {
            // First packet - Setting up the callback time
            writerGroup->callbackTime = UA_DateTime_new();
            *writerGroup->callbackTime = currentTime - ((currentTime - *writerGroup->config.baseTime) % interval);
        }
        else if((currentTime > (*writerGroup->callbackTime + interval)) &&
                (writerGroup->config.timerPolicy == UA_TIMER_HANDLE_CYCLEMISS_WITH_BASETIME)) {
            // Assuming a packet miss occured and handling it with the base time
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Packet miss occured. Handling the packet miss with the base time");
            *writerGroup->callbackTime = currentTime - ((currentTime - *writerGroup->config.baseTime) % interval);
        }
    }

    if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP){
        UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType *) writerGroup->config.messageSettings.content.decoded.data;
        if(wgm->publishingOffset) {
            /* Check the current time with the first value of publishingOffset array */
            if((*writerGroup->callbackTime + (UA_DateTime)((*wgm->publishingOffset) * UA_DATETIME_MSEC)) < currentTime)
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "Delayed publish. Publish may fail, modify PublishingOffset configuration. Use custom callback implementation for better accuracy");
        }
    }

    if(writerGroup->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE) {
        UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType *) writerGroup->config.messageSettings.content.decoded.data;
        if(wgm->publishingOffsetSize > 1)
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PublishingOffset array not supported in RT pubsub path as it has only one DSM. Initial publishingOffset value taken");

        if(wgm->publishingOffset) {
            // TODO: PublishingOffset calculation and passing it to the structure
            // UA_DateTime publishingTime = *writerGroup->callbackTime + (UA_DateTime)((*wgm->publishingOffset) * UA_DATETIME_MSEC);
        }

        UA_StatusCode res =
            sendBufferedNetworkMessage(server, connection, &writerGroup->bufferedMessage,
                                       &writerGroup->config.transportSettings);
        if(res == UA_STATUSCODE_GOOD) {
            writerGroup->sequenceNumber++;
            if(writerGroup->config.baseTime)
                *writerGroup->callbackTime = *writerGroup->callbackTime + (UA_DateTime)(writerGroup->config.publishingInterval * UA_DATETIME_MSEC);
        } else {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Publish failed. RT fixed size. sendBufferedNetworkMessage failed");
            UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_ERROR, writerGroup);
        }

        return;
    }

    /* How many DSM can be sent in one NM? */
    UA_Byte maxDSM = (UA_Byte)writerGroup->config.maxEncapsulatedDataSetMessageCount;
    if(writerGroup->config.maxEncapsulatedDataSetMessageCount > UA_BYTE_MAX)
        maxDSM = UA_BYTE_MAX;
    /* If the maxEncapsulatedDataSetMessageCount is set to 0->1 */
    if(maxDSM == 0)
        maxDSM = 1;

    /* It is possible to put several DataSetMessages into one NetworkMessage.
     * But only if they do not contain promoted fields. NM with only DSM are
     * sent out right away. The others are kept in a buffer for "batching". */
    size_t dsmCount = 0;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_STACKARRAY(UA_UInt16, dsWriterIds, writerGroup->writersCount);
    UA_STACKARRAY(UA_DataSetMessage, dsmStore, writerGroup->writersCount);
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &writerGroup->writers, listEntry) {
        if(dsw->state != UA_PUBSUBSTATE_OPERATIONAL)
            continue;

        /* Find the dataset */
        UA_PublishedDataSet *pds =
            UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
        if(!pds) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "PubSub Publish: PublishedDataSet not found");
            UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
            continue;
        }

        /* Generate the DSM */
        res = UA_DataSetWriter_generateDataSetMessage(server, &dsmStore[dsmCount], dsw);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "PubSub Publish: DataSetMessage creation failed");
            UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
            continue;
        }

        /* There is no promoted field and we can batch dsm. So do the batching. */
        if(pds->promotedFieldsCount == 0 && maxDSM > 1) {
            dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
            dsmCount++;
            continue;
        }

        /* Send right away */
        if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP){
            UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType *) writerGroup->config.messageSettings.content.decoded.data;
            if(wgm->publishingOffset) {
                if((dsmCount > 1) && (wgm->publishingOffsetSize > 1))
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "PublishingOffset array not implemented. Initial publishingOffset value taken");

                // TODO: PublishingOffset array calculation with multiple dsm counts and pass them into the structure
                // UA_DateTime publishingTime = *writerGroup->callbackTime  + (UA_DateTime)((*wgm->publishingOffset) * UA_DATETIME_MSEC);
            }

            res = sendNetworkMessage(connection, writerGroup, &dsmStore[dsmCount],
                                     &dsw->config.dataSetWriterId, 1,
                                     &writerGroup->config.messageSettings,
                                     &writerGroup->config.transportSettings);
        } else { /* if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) */
            res = sendNetworkMessageJson(connection, &dsmStore[dsmCount],
                                         &dsw->config.dataSetWriterId, 1,
                                         &writerGroup->config.transportSettings);
        }

        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "PubSub Publish: Could not send a NetworkMessage");
            UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
        }

        /* Clean up */
        if(writerGroup->config.rtLevel == UA_PUBSUB_RT_DIRECT_VALUE_ACCESS) {
            for(size_t i = 0; i < dsmStore[dsmCount].data.keyFrameData.fieldCount; ++i) {
                dsmStore[dsmCount].data.keyFrameData.dataSetFields[i].value.data = NULL;
            }
        }
        UA_DataSetMessage_clear(&dsmStore[dsmCount]);
    }

    /* Send the NetworkMessages with batched DataSetMessages */
    size_t i = 0;
    while(i < dsmCount) {
        /* How many dsm in this iteration? */
        UA_Byte nmDsmCount = maxDSM;
        if(i + nmDsmCount > dsmCount) {
            nmDsmCount = (UA_Byte)(dsmCount - i);
        }

        if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP) {
            UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType *) writerGroup->config.messageSettings.content.decoded.data;
            if(wgm->publishingOffset) {
                if((dsmCount > 1) && (wgm->publishingOffsetSize > 1))
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                    "PublishingOffset array not implemented. Initial publishingOffset value taken");

                // TODO: PublishingOffset array calculation with multiple dsm counts and pass them into the structure
                // UA_DateTime publishingTime = *writerGroup->callbackTime  + (UA_DateTime)((*wgm->publishingOffset) * UA_DATETIME_MSEC);
            }

            res = sendNetworkMessage(connection, writerGroup, &dsmStore[i],
                                     &dsWriterIds[i], nmDsmCount,
                                     &writerGroup->config.messageSettings,
                                     &writerGroup->config.transportSettings);
        } else { /* if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) */
            res = sendNetworkMessageJson(connection, &dsmStore[i],
                                         &dsWriterIds[i], nmDsmCount,
                                         &writerGroup->config.transportSettings);
        }

        if(res == UA_STATUSCODE_GOOD) {
            writerGroup->sequenceNumber++; /* TODO: Why not in the direct-send case? */
        } else {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "PubSub Publish: Sending a NetworkMessage failed");
            LIST_FOREACH(dsw, &writerGroup->writers, listEntry) {
                if(dsWriterIds[i * maxDSM] != dsw->config.dataSetWriterId)
                    continue;
                UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
            }
        }

        /* Forward the position for the next iteration */
        i += nmDsmCount;
    }

    if(writerGroup->config.baseTime)
        *writerGroup->callbackTime = *writerGroup->callbackTime + (UA_DateTime)(writerGroup->config.publishingInterval * UA_DATETIME_MSEC);

    /* Clean up DSM */
    for(i = 0; i < dsmCount; i++)
        UA_DataSetMessage_clear(&dsmStore[i]);
}

/* Add new publishCallback. The first execution is triggered directly after
 * creation. */
UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(writerGroup->config.pubsubManagerCallback.addCustomCallback)
        retval |= writerGroup->config.pubsubManagerCallback.addCustomCallback(server, writerGroup->identifier,
                                                                              (UA_ServerCallback) UA_WriterGroup_publishCallback,
                                                                              writerGroup, writerGroup->config.publishingInterval,
                                                                              NULL,                                        // TODO: Send base time from writer group config
                                                                              UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,  // TODO: Send timer policy from writer group config
                                                                              &writerGroup->publishCallbackId);
    else
        retval |= UA_PubSubManager_addRepeatedCallback(server,
                                                       (UA_ServerCallback) UA_WriterGroup_publishCallback,
                                                       writerGroup, writerGroup->config.publishingInterval,
                                                       NULL,                                        // TODO: Send base time from writer group config
                                                       UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,  // TODO: Send timer policy from writer group config
                                                       &writerGroup->publishCallbackId);

    if(retval == UA_STATUSCODE_GOOD)
        writerGroup->publishCallbackIsRegistered = true;

    /* Run once after creation */
    UA_WriterGroup_publishCallback(server, writerGroup);
    return retval;
>>>>>>> bfc5f39da (feat(pubsub): Support base time and timer policy in WriterGroup and ReaderGroup configuration)
}

#endif /* UA_ENABLE_PUBSUB */
