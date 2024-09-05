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

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub_networkmessage.h"

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

UA_DataSetWriter *
UA_DataSetWriter_find(UA_PubSubManager *psm, const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            UA_DataSetWriter *dsw;
            LIST_FOREACH(dsw, &wg->writers, listEntry) {
                if(UA_NodeId_equal(&id, &dsw->head.identifier))
                    return dsw;
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
UA_DataSetWriter_freezeConfiguration(UA_DataSetWriter *dsw) {
    if(dsw->configurationFrozen)
        return;
    dsw->configurationFrozen = true;
    UA_PublishedDataSet *pds = dsw->connectedDataSet;
    if(!pds) /* Skip for heartbeat writers */
        return;
    pds->configurationFreezeCounter++;
}

static void
UA_DataSetWriter_unfreezeConfiguration(UA_DataSetWriter *dsw) {
    if(!dsw->configurationFrozen)
        return;
    dsw->configurationFrozen = false;
    UA_PublishedDataSet *pds = dsw->connectedDataSet;
    if(!pds) /* Skip for heartbeat writers */
        return;
    pds->configurationFreezeCounter--;
}

UA_StatusCode
UA_DataSetWriter_setPubSubState(UA_PubSubManager *psm, UA_DataSetWriter *dsw,
                                UA_PubSubState targetState) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_WriterGroup *wg = dsw->linkedWriterGroup;
    UA_assert(wg);

    UA_PubSubState oldState = dsw->head.state;

    switch(targetState) {
        /* Disabled */
    case UA_PUBSUBSTATE_DISABLED:
    case UA_PUBSUBSTATE_ERROR:
        dsw->head.state = targetState;
        UA_DataSetWriter_unfreezeConfiguration(dsw);
        break;

        /* Enabled */
    case UA_PUBSUBSTATE_PAUSED:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
    case UA_PUBSUBSTATE_OPERATIONAL:
        if(wg->head.state == UA_PUBSUBSTATE_DISABLED ||
           wg->head.state == UA_PUBSUBSTATE_ERROR ||
           wg->head.state == UA_PUBSUBSTATE_PAUSED) {
            dsw->head.state = UA_PUBSUBSTATE_PAUSED; /* WG is disabled -> paused */
        } else {
            dsw->head.state = wg->head.state; /* WG is enabled -> same state */
        }
        UA_DataSetWriter_freezeConfiguration(dsw);
        break;

    default:
        dsw->head.state = UA_PUBSUBSTATE_ERROR;
        res = UA_STATUSCODE_BADINTERNALERROR;
        UA_DataSetWriter_unfreezeConfiguration(dsw);
        break;
    }

    /* Inform application about state change */
    if(dsw->head.state != oldState) {
        UA_Server *server = psm->sc.server;
        UA_LOG_INFO_PUBSUB(psm->logging, dsw, "%s -> %s",
                           UA_PubSubState_name(oldState),
                           UA_PubSubState_name(dsw->head.state));
        if(server->config.pubSubConfig.stateChangeCallback != 0) {
            UA_UNLOCK(&server->serviceMutex);
            server->config.pubSubConfig.
                stateChangeCallback(server, dsw->head.identifier, dsw->head.state, res);
            UA_LOCK(&server->serviceMutex);
        }
    }

    return res;
}

UA_StatusCode
UA_DataSetWriter_create(UA_PubSubManager *psm,
                        const UA_NodeId writerGroup, const UA_NodeId dataSet,
                        const UA_DataSetWriterConfig *dswConfig,
                        UA_NodeId *writerIdentifier) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);
    if(!dswConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Make checks for a heartbeat */
    if(UA_NodeId_isNull(&dataSet) && dswConfig->keyFrameCount != 1) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "Adding DataSetWriter failed: DataSet can be null only for "
                              "a heartbeat in which case KeyFrameCount shall be 1");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(wg->config.rtLevel != UA_PUBSUB_RT_NONE &&
       UA_PubSubState_isEnabled(wg->head.state)) {
        UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                              "Cannot add a DataSetWriter while the "
                              "WriterGroup with realtime options is enabled");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PublishedDataSet *pds = NULL;

    if(!UA_NodeId_isNull(&dataSet)) {
        pds = UA_PublishedDataSet_find(psm, dataSet);
        if(!pds) {
            UA_LOG_WARNING_PUBSUB(psm->logging, wg,
                                  "Cannot add the DataSetWriter: "
                                  "The PublishedDataSet was not found");
            return UA_STATUSCODE_BADNOTFOUND;
        }

        if(wg->config.rtLevel != UA_PUBSUB_RT_NONE) {
            UA_DataSetField *tmpDSF;
            TAILQ_FOREACH(tmpDSF, &pds->fields, listEntry) {
                if(!tmpDSF->config.field.variable.rtValueSource.rtFieldSourceEnabled &&
                   !tmpDSF->config.field.variable.rtValueSource.rtInformationModelNode) {
                    UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                                          "Adding DataSetWriter failed: "
                                          "Fields in PDS are not RT capable");
                    return UA_STATUSCODE_BADCONFIGURATIONERROR;
                }
            }
        }
    }

    UA_DataSetWriter *dsw = (UA_DataSetWriter *)
        UA_calloc(1, sizeof(UA_DataSetWriter));
    if(!dsw)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dsw->head.componentType = UA_PUBSUBCOMPONENT_DATASETWRITER;
    dsw->linkedWriterGroup = wg;

    /* Copy the config into the new dataSetWriter */
    UA_StatusCode res = UA_DataSetWriterConfig_copy(dswConfig, &dsw->config);
    UA_CHECK_STATUS(res, UA_free(dsw); return res);

    if(pds) {
        /* Save the current version of the connected PublishedDataSet */
        dsw->connectedDataSetVersion = pds->dataSetMetaData.configurationVersion;

        if(psm->sc.server->config.pubSubConfig.enableDeltaFrames) {
            /* Initialize the queue for the last values */
            if(pds->fieldSize > 0) {
                dsw->lastSamples = (UA_DataSetWriterSample*)
                    UA_calloc(pds->fieldSize, sizeof(UA_DataSetWriterSample));
                if(!dsw->lastSamples) {
                    UA_DataSetWriterConfig_clear(&dsw->config);
                    UA_free(dsw);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                dsw->lastSamplesCount = pds->fieldSize;
                for(size_t i = 0; i < dsw->lastSamplesCount; i++) {
                    UA_DataValue_init(&dsw->lastSamples[i].value);
                    dsw->lastSamples[i].valueChanged = false;
                }
            }
        }
        /* Connect PublishedDataSet with DataSetWriter */
        dsw->connectedDataSet = pds;
    } else {
        /* If the dataSet is NULL, we are adding a heartbeat writer */
        dsw->connectedDataSetVersion.majorVersion = 0;
        dsw->connectedDataSetVersion.minorVersion = 0;
        dsw->connectedDataSet = NULL;
    }

    /* Add the new writer to the group */
    LIST_INSERT_HEAD(&wg->writers, dsw, listEntry);
    wg->writersCount++;

    /* Add to the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    res |= addDataSetWriterRepresentation(psm->sc.server, dsw);
#else
    UA_PubSubManager_generateUniqueNodeId(psm, &dsw->head.identifier);
#endif

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "%SDataSetWriter %N\t| ",
                dsw->linkedWriterGroup->head.logIdString,
                dsw->head.identifier);
    dsw->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    UA_LOG_INFO_PUBSUB(psm->logging, dsw,
                       "DataSetWriter created (State: %s)",
                       UA_PubSubState_name(dsw->head.state));

    if(writerIdentifier)
        UA_NodeId_copy(&dsw->head.identifier, writerIdentifier);
    return res;
}

UA_StatusCode
UA_DataSetWriter_prepareDataSet(UA_PubSubManager *psm, UA_DataSetWriter *dsw,
                                UA_DataSetMessage *dsm) {
    /* No PublishedDataSet defined -> Heartbeat messages only */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_PublishedDataSet *pds = dsw->connectedDataSet;
    if(!pds) {
        res = UA_DataSetWriter_generateDataSetMessage(psm, dsm, dsw);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_PUBSUB(psm->logging, dsw,
                                  "PubSub-RT configuration fail: "
                                  "Heartbeat DataSetMessage creation failed");
        }
        return res;
    }

    UA_WriterGroup *wg = dsw->linkedWriterGroup;
    UA_assert(wg);

    /* Promoted Fields not allowed if RT is enabled */
    if(wg->config.rtLevel > UA_PUBSUB_RT_NONE &&
       pds->promotedFieldsCount > 0) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsw,
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
            UA_NODESTORE_GET(psm->sc.server, publishedVariable);
        if(rtNode && rtNode->head.nodeClass != UA_NODECLASS_VARIABLE) {
            UA_LOG_ERROR_PUBSUB(psm->logging, dsw,
                                "PubSub-RT configuration fail: "
                                "PDS points to a node that is not a variable");
            UA_NODESTORE_RELEASE(psm->sc.server, (const UA_Node *)rtNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
        UA_NODESTORE_RELEASE(psm->sc.server, (const UA_Node *)rtNode);

        /* TODO: Get the External Value Source from the node instead of from the config */

        /* If direct-value-access is enabled, the pointers need to be set */
        if(wg->config.rtLevel & UA_PUBSUB_RT_DIRECT_VALUE_ACCESS &&
           !dsf->config.field.variable.rtValueSource.rtFieldSourceEnabled) {
            UA_LOG_ERROR_PUBSUB(psm->logging, dsw,
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
                UA_LOG_WARNING_PUBSUB(psm->logging, dsw,
                                      "PubSub-RT configuration fail: "
                                      "PDS contains String/ByteString with dynamic length");
                return UA_STATUSCODE_BADNOTSUPPORTED;
            } else if(!UA_DataType_isNumeric(
                          UA_findDataType(&dsf->fieldMetaData.dataType)) &&
                      !UA_NodeId_equal(&dsf->fieldMetaData.dataType,
                                       &UA_TYPES[UA_TYPES_BOOLEAN].typeId)) {
                UA_LOG_WARNING_PUBSUB(psm->logging, dsw,
                                      "PubSub-RT configuration fail: "
                                      "PDS contains variable with dynamic size");
                return UA_STATUSCODE_BADNOTSUPPORTED;
            }
        }
    }

    /* Generate the DSM */
    res = UA_DataSetWriter_generateDataSetMessage(psm, dsm, dsw);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsw,
                              "PubSub-RT configuration fail: "
                              "DataSetMessage buffering failed");
    }

    return res;
}

UA_StatusCode
UA_DataSetWriter_remove(UA_PubSubManager *psm, UA_DataSetWriter *dsw) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    UA_WriterGroup *wg = dsw->linkedWriterGroup;
    UA_assert(wg);

    /* Check if the WriterGroup is enabled with RT options. Disallow removal in
     * that case. The RT path might still have a pointer to the DataSetWriter.
     * Or we violate the fixed-size-message configuration.*/
    if(wg->config.rtLevel != UA_PUBSUB_RT_NONE &&
       UA_PubSubState_isEnabled(wg->head.state)) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsw,
                              "Removal of DataSetWriter not possible while "
                              "the WriterGroup with realtime options is enabled");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Disable and signal to the application */
    UA_DataSetWriter_setPubSubState(psm, dsw, UA_PUBSUBSTATE_DISABLED);

    /* Remove from information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(psm->sc.server, dsw->head.identifier, true);
#endif

    /* Remove DataSetWriter from group */
    LIST_REMOVE(dsw, listEntry);
    wg->writersCount--;

    UA_LOG_INFO_PUBSUB(psm->logging, dsw, "Writer deleted");

    if(psm->sc.server->config.pubSubConfig.enableDeltaFrames) {
        /* Delete lastSamples store */
        for(size_t i = 0; i < dsw->lastSamplesCount; i++) {
            UA_DataValue_clear(&dsw->lastSamples[i].value);
        }
        UA_free(dsw->lastSamples);
        dsw->lastSamples = NULL;
        dsw->lastSamplesCount = 0;
    }

    UA_DataSetWriterConfig_clear(&dsw->config);
    UA_PubSubComponentHead_clear(&dsw->head);
    UA_free(dsw);

    return UA_STATUSCODE_GOOD;
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
UA_PubSubDataSetWriter_generateKeyFrameMessage(UA_PubSubManager *psm,
                                               UA_DataSetMessage *dataSetMessage,
                                               UA_DataSetWriter *dsw) {
    UA_PublishedDataSet *pds = dsw->connectedDataSet;
    if(!pds)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Prepare DataSetMessageContent */
    dataSetMessage->header.dataSetMessageValid = true;
    dataSetMessage->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dataSetMessage->data.keyFrameData.fieldCount = pds->fieldSize;
    dataSetMessage->data.keyFrameData.dataSetFields = (UA_DataValue *)
            UA_Array_new(pds->fieldSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
    dataSetMessage->data.keyFrameData.dataSetMetaDataType = &pds->dataSetMetaData;
    if(!dataSetMessage->data.keyFrameData.dataSetFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

#ifdef UA_ENABLE_JSON_ENCODING
    dataSetMessage->data.keyFrameData.fieldNames = (UA_String *)
        UA_Array_new(pds->fieldSize, &UA_TYPES[UA_TYPES_STRING]);
    if(!dataSetMessage->data.keyFrameData.fieldNames) {
        UA_DataSetMessage_clear(dataSetMessage);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
#endif

    /* Loop over the fields */
    size_t counter = 0;
    UA_DataSetField *dsf;
    TAILQ_FOREACH(dsf, &pds->fields, listEntry) {
#ifdef UA_ENABLE_JSON_ENCODING
        /* Set the field name alias */
        UA_String_copy(&dsf->config.field.variable.fieldNameAlias,
                       &dataSetMessage->data.keyFrameData.fieldNames[counter]);
#endif

        /* Sample the value */
        UA_DataValue *dfv = &dataSetMessage->data.keyFrameData.dataSetFields[counter];
        UA_PubSubDataSetField_sampleValue(psm, dsf, dfv);

        /* Deactivate statuscode? */
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
            dfv->hasStatus = false;

        /* Deactivate timestamps */
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP) == 0)
            dfv->hasSourceTimestamp = false;
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS) == 0)
            dfv->hasSourcePicoseconds = false;
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERTIMESTAMP) == 0)
            dfv->hasServerTimestamp = false;
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS) == 0)
            dfv->hasServerPicoseconds = false;

        if(psm->sc.server->config.pubSubConfig.enableDeltaFrames) {
            /* Update lastValue store */
            UA_DataValue_clear(&dsw->lastSamples[counter].value);
            UA_DataValue_copy(dfv, &dsw->lastSamples[counter].value);
        }
        counter++;
    }
    return UA_STATUSCODE_GOOD;
}

/* the input message is already initialized and that the method 
 * must not be called twice for the same message */
static UA_StatusCode
UA_PubSubDataSetWriter_generateDeltaFrameMessage(UA_PubSubManager *psm,
                                                 UA_DataSetMessage *dsm,
                                                 UA_DataSetWriter *dsw) {
    UA_PublishedDataSet *pds = dsw->connectedDataSet;
    if(!pds)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Prepare DataSetMessageContent */
    dsm->header.dataSetMessageValid = true;
    dsm->header.dataSetMessageType = UA_DATASETMESSAGE_DATADELTAFRAME;
    if(pds->fieldSize == 0)
        return UA_STATUSCODE_GOOD;

    UA_DataSetField *dsf;
    UA_UInt16 counter = 0;
    TAILQ_FOREACH(dsf, &pds->fields, listEntry) {
        /* Sample the value */
        UA_DataValue value;
        UA_DataValue_init(&value);
        UA_PubSubDataSetField_sampleValue(psm, dsf, &value);

        /* Check if the value has changed */
        UA_DataSetWriterSample *ls = &dsw->lastSamples[counter];
        if(valueChangedVariant(&ls->value.value, &value.value)) {
            /* increase fieldCount for current delta message */
            dsm->data.deltaFrameData.fieldCount++;
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

    dsm->data.deltaFrameData.deltaFrameFields = deltaFields;
    dsm->data.deltaFrameData.fieldCount = counter;

    size_t currentDeltaField = 0;
    for(size_t i = 0; i < pds->fieldSize; i++) {
        if(!dsw->lastSamples[i].valueChanged)
            continue;

        UA_DataSetMessage_DeltaFrameField *dff = &deltaFields[currentDeltaField];

        dff->fieldIndex = (UA_UInt16) i;
        UA_DataValue_copy(&dsw->lastSamples[i].value, &dff->fieldValue);

        /* Reset the changed flag */
        dsw->lastSamples[i].valueChanged = false;

        /* Deactivate statuscode? */
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
            dff->fieldValue.hasStatus = false;

        /* Deactivate timestamps? */
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP) == 0)
            dff->fieldValue.hasSourceTimestamp = false;
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS) == 0)
            dff->fieldValue.hasServerPicoseconds = false;
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERTIMESTAMP) == 0)
            dff->fieldValue.hasServerTimestamp = false;
        if(((u64)dsw->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS) == 0)
            dff->fieldValue.hasServerPicoseconds = false;

        currentDeltaField++;
    }
    return UA_STATUSCODE_GOOD;
}

/* Generate a DataSetMessage for the given writer. */
UA_StatusCode
UA_DataSetWriter_generateDataSetMessage(UA_PubSubManager *psm,
                                        UA_DataSetMessage *dataSetMessage,
                                        UA_DataSetWriter *dsw) {
    /* Heartbeat message if no pds is connected */
    UA_PublishedDataSet *pds = dsw->connectedDataSet;

    UA_WriterGroup *wg = dsw->linkedWriterGroup;
    UA_EventLoop *el = UA_PubSubConnection_getEL(psm, wg->linkedConnection);

    /* Reset the message */
    memset(dataSetMessage, 0, sizeof(UA_DataSetMessage));

    /* The configuration Flags are included
     * inside the std. defined UA_UadpDataSetWriterMessageDataType */
    UA_UadpDataSetWriterMessageDataType defaultUadpConfiguration;
    UA_UadpDataSetWriterMessageDataType *dsm = NULL;
    UA_JsonDataSetWriterMessageDataType *jsonDsm = NULL;
    const UA_ExtensionObject *ms = &dsw->config.messageSettings;
    if(ms->content.decoded.type == &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE]) {
        dsm = (UA_UadpDataSetWriterMessageDataType*)ms->content.decoded.data; /* type is UADP */
    } else if(ms->content.decoded.type == &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE]) {
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
    if(dsw->config.dataSetFieldContentMask &
       (u64)UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_RAWDATA;
    } else if((u64)dsw->config.dataSetFieldContentMask &
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
        if(dsm->networkMessageNumber != 0 || dsm->dataSetOffset != 0 ||
           dsm->configuredSize != 0) {
            UA_LOG_WARNING_PUBSUB(psm->logging, dsw,
                                  "Static DSM configuration not supported, using defaults");
            dsm->networkMessageNumber = 0;
            dsm->dataSetOffset = 0;
            // dsm->configuredSize = 0;
        }

        /* setting configured size in the dataSetMessage to add padding later on */
        dataSetMessage->configuredSize = dsm->configuredSize;

        /* Std: 'The DataSetMessageContentMask defines the flags for the content
         * of the DataSetMessage header.' */
        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION) {
            dataSetMessage->header.configVersionMajorVersionEnabled = true;
            if(!pds) {
                /* Heartbeat */
                dataSetMessage->header.configVersionMajorVersion = 0;
            } else {
                dataSetMessage->header.configVersionMajorVersion =
                    pds->dataSetMetaData.configurationVersion.majorVersion;
            }
        }
        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION) {
            dataSetMessage->header.configVersionMinorVersionEnabled = true;
            if(!pds) {
                /* Heartbeat */
                dataSetMessage->header.configVersionMinorVersion = 0;
            } else {
                dataSetMessage->header.configVersionMinorVersion =
                    pds->dataSetMetaData.configurationVersion.minorVersion;
            }
        }

        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
            dataSetMessage->header.dataSetMessageSequenceNrEnabled = true;
            dataSetMessage->header.dataSetMessageSequenceNr =
                dsw->actualDataSetMessageSequenceCount;
        }

        if((u64)dsm->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP) {
            dataSetMessage->header.timestampEnabled = true;
            dataSetMessage->header.timestamp = el->dateTime_now(el);
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
            if(!pds) {
                /* Heartbeat */
                dataSetMessage->header.configVersionMajorVersion = 0;
            } else {
                dataSetMessage->header.configVersionMajorVersion =
                pds->dataSetMetaData.configurationVersion.majorVersion;
            }
       }
        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION) {
            dataSetMessage->header.configVersionMinorVersionEnabled = true;
            if(!pds) {
                /* Heartbeat */
                dataSetMessage->header.configVersionMinorVersion = 0;
            } else {
                dataSetMessage->header.configVersionMinorVersion =
                pds->dataSetMetaData.configurationVersion.minorVersion;
            }
       }

        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
            dataSetMessage->header.dataSetMessageSequenceNrEnabled = true;
            dataSetMessage->header.dataSetMessageSequenceNr =
                dsw->actualDataSetMessageSequenceCount;
        }

        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_TIMESTAMP) {
            dataSetMessage->header.timestampEnabled = true;
            dataSetMessage->header.timestamp = el->dateTime_now(el);
        }

        /* TODO: Statuscode not supported yet */
        if((u64)jsonDsm->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_STATUS) {
            dataSetMessage->header.statusEnabled = true;
        }
    }

    /* Set the sequence count. Automatically rolls over to zero */
    dsw->actualDataSetMessageSequenceCount++;

    if(!pds) {
        /* Prepare DataSetMessageContent for the heartbeat message */
        dataSetMessage->header.dataSetMessageValid = true;
        dataSetMessage->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
        dataSetMessage->data.keyFrameData.fieldCount = 0;
        return UA_STATUSCODE_GOOD;
    }

    /* JSON does not differ between deltaframes and keyframes, only keyframes
     * are currently used. */
    if(dsm && psm->sc.server->config.pubSubConfig.enableDeltaFrames) {
        /* Check if the PublishedDataSet version has changed -> if yes flush the
         * lastValue store and send a KeyFrame */
        if(dsw->connectedDataSetVersion.majorVersion !=
           pds->dataSetMetaData.configurationVersion.majorVersion ||
           dsw->connectedDataSetVersion.minorVersion !=
           pds->dataSetMetaData.configurationVersion.minorVersion) {
            /* Remove old samples */
            for(size_t i = 0; i < dsw->lastSamplesCount; i++)
                UA_DataValue_clear(&dsw->lastSamples[i].value);

            /* Realloc PDS dependent memory */
            dsw->lastSamplesCount = pds->fieldSize;
            UA_DataSetWriterSample *newSamplesArray = (UA_DataSetWriterSample * )
                UA_realloc(dsw->lastSamples,
                           sizeof(UA_DataSetWriterSample) * dsw->lastSamplesCount);
            if(!newSamplesArray)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            dsw->lastSamples = newSamplesArray;
            memset(dsw->lastSamples, 0,
                   sizeof(UA_DataSetWriterSample) * dsw->lastSamplesCount);

            dsw->connectedDataSetVersion =
                pds->dataSetMetaData.configurationVersion;
            UA_PubSubDataSetWriter_generateKeyFrameMessage(psm, dataSetMessage, dsw);
            dsw->deltaFrameCounter = 0;
            return UA_STATUSCODE_GOOD;
        }

        /* The standard defines: if a PDS contains only one fields no delta messages
         * should be generated because they need more memory than a keyframe with 1
         * field. */
        if(pds->fieldSize > 1 && dsw->deltaFrameCounter > 0 &&
           dsw->deltaFrameCounter <= dsw->config.keyFrameCount) {
            UA_PubSubDataSetWriter_generateDeltaFrameMessage(psm, dataSetMessage, dsw);
            dsw->deltaFrameCounter++;
            return UA_STATUSCODE_GOOD;
        }

        dsw->deltaFrameCounter = 1;
    }

    return UA_PubSubDataSetWriter_generateKeyFrameMessage(psm, dataSetMessage, dsw);
}

/**************/
/* Server API */
/**************/

UA_StatusCode
UA_Server_getDataSetWriterConfig(UA_Server *server, const UA_NodeId dswId,
                                 UA_DataSetWriterConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_DataSetWriter *dsw = UA_DataSetWriter_find(getPSM(server), dswId);
    UA_StatusCode res = (dsw) ?
        UA_DataSetWriterConfig_copy(&dsw->config, config) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getDataSetWriterState(UA_Server *server, const UA_NodeId dswId,
                               UA_PubSubState *state) {
    if(!server || !state)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_DataSetWriter *dsw = UA_DataSetWriter_find(getPSM(server), dswId);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;;
    if(dsw) {
        *state = dsw->head.state;
        res = UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_enableDataSetWriter(UA_Server *server, const UA_NodeId dswId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetWriter *dsw = UA_DataSetWriter_find(psm, dswId);
    UA_StatusCode ret = (dsw) ?
        UA_DataSetWriter_setPubSubState(psm, dsw, UA_PUBSUBSTATE_OPERATIONAL) :
        UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_disableDataSetWriter(UA_Server *server, const UA_NodeId dswId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetWriter *dsw = UA_DataSetWriter_find(psm, dswId);
    UA_StatusCode ret = (dsw) ?
        UA_DataSetWriter_setPubSubState(psm, dsw, UA_PUBSUBSTATE_DISABLED) :
        UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_addDataSetWriter(UA_Server *server,
                           const UA_NodeId writerGroup, const UA_NodeId dataSet,
                           const UA_DataSetWriterConfig *dataSetWriterConfig,
                           UA_NodeId *writerIdentifier) {
    if(!server || !dataSetWriterConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    /* Delete the reserved IDs if the related session no longer exists. */
    UA_PubSubManager_freeIds(psm);
    UA_StatusCode res = UA_DataSetWriter_create(psm, writerGroup, dataSet,
                                                dataSetWriterConfig, writerIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_removeDataSetWriter(UA_Server *server, const UA_NodeId dswId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetWriter *dsw = UA_DataSetWriter_find(psm, dswId);
    UA_StatusCode res = (dsw) ?
        UA_DataSetWriter_remove(psm, dsw) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
