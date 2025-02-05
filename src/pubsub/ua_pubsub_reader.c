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

#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub_networkmessage.h"

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

#if UA_LOGLEVEL <= 200
static void
printPublisherId(char *out, size_t size, UA_PublisherId *id) {
    switch(id->idType) {
    case UA_PUBLISHERIDTYPE_BYTE:   mp_snprintf(out, size, "(b)%u", (unsigned)id->id.byte); break;
    case UA_PUBLISHERIDTYPE_UINT16: mp_snprintf(out, size, "(u16)%u", (unsigned)id->id.uint16); break;
    case UA_PUBLISHERIDTYPE_UINT32: mp_snprintf(out, size, "(u32)%u", (unsigned)id->id.uint32); break;
    case UA_PUBLISHERIDTYPE_UINT64: mp_snprintf(out, size, "(u64)%lu", id->id.uint64); break;
    case UA_PUBLISHERIDTYPE_STRING: mp_snprintf(out, size, "\"%S\"", id->id.string); break;
    default: out[0] = 0; break;
    }
}
#endif

UA_StatusCode
UA_DataSetReader_checkIdentifier(UA_PubSubManager *psm, UA_DataSetReader *dsr,
                                 UA_NetworkMessage *msg) {
    if(!publisherIdIsMatching(msg, &dsr->config.publisherId)) {
#if UA_LOGLEVEL <= 200
        char idAStr[512];
        char idBStr[512];
        printPublisherId(idAStr, 512, &dsr->config.publisherId);
        printPublisherId(idBStr, 512, &msg->publisherId);
        UA_LOG_DEBUG_PUBSUB(psm->logging, dsr, "PublisherId does not match. "
                            "Expected %s, received %s", idAStr, idBStr);
#endif
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_ReaderGroup *rg = dsr->linkedReaderGroup;
    if(rg->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
        // TODO
        /* if(dsr->config.dataSetWriterId == */
        /*    *msg->payloadHeader.dataSetPayloadHeader.dataSetWriterIds) { */
        /*     return UA_STATUSCODE_GOOD; */
        /* } */

        /* UA_LOG_DEBUG_PUBSUB(psm->logging, dsr, "DataSetWriterId does not match. " */
        /*                     "Expected %u, received %u", dsr->config.dataSetWriterId, */
        /*                     *msg->payloadHeader.dataSetPayloadHeader.dataSetWriterIds); */
        return UA_STATUSCODE_BADNOTFOUND;
    }

    if(msg->groupHeaderEnabled && msg->groupHeader.writerGroupIdEnabled) {
        if(dsr->config.writerGroupId != msg->groupHeader.writerGroupId) {
            UA_LOG_DEBUG_PUBSUB(psm->logging, dsr, "WriterGroupId does not match. "
                                "Expected %u, received %u", dsr->config.writerGroupId,
                                msg->groupHeader.writerGroupId);
            return UA_STATUSCODE_BADNOTFOUND;
        }
    }

    if(msg->payloadHeaderEnabled) {
        size_t totalDataSets = msg->payload.dataSetPayload.dataSetMessagesSize;
        for(size_t i = 0; i < totalDataSets; i++) {
            UA_UInt32 dswId = msg->payload.dataSetPayload.dataSetMessages[i].dataSetWriterId;
            if(dsr->config.dataSetWriterId == dswId)
                return UA_STATUSCODE_GOOD;
        }
        UA_LOG_DEBUG_PUBSUB(psm->logging, dsr,
                            "DataSetWriterIds in the payload do not match");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    return UA_STATUSCODE_GOOD;
}

UA_DataSetReader *
UA_DataSetReader_find(UA_PubSubManager *psm, const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_PubSubConnection *psc;
    TAILQ_FOREACH(psc, &psm->connections, listEntry) {
        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &psc->readerGroups, listEntry) {
            UA_DataSetReader *dsr;
            LIST_FOREACH(dsr, &rg->readers, listEntry) {
                if(UA_NodeId_equal(&id, &dsr->head.identifier))
                    return dsr;
            }
        }
    }
    return NULL;
}

UA_StatusCode
UA_DataSetReader_create(UA_PubSubManager *psm, UA_NodeId readerGroupIdentifier,
                        const UA_DataSetReaderConfig *dataSetReaderConfig,
                        UA_NodeId *readerIdentifier) {
    if(!psm || !dataSetReaderConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    /* Search the reader group by the given readerGroupIdentifier */
    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupIdentifier);
    if(!rg)
        return UA_STATUSCODE_BADNOTFOUND;

    if(UA_PubSubState_isEnabled(rg->head.state)) {
        UA_LOG_WARNING_PUBSUB(psm->logging, rg,
                              "Cannot add a DataSetReader while the "
                              "ReaderGroup with realtime options is enabled");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Allocate memory for new DataSetReader */
    UA_DataSetReader *dsr = (UA_DataSetReader *)
        UA_calloc(1, sizeof(UA_DataSetReader));
    if(!dsr)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dsr->head.componentType = UA_PUBSUBCOMPONENT_DATASETREADER;
    dsr->linkedReaderGroup = rg;

    /* Add the new reader to the group */
    LIST_INSERT_HEAD(&rg->readers, dsr, listEntry);
    rg->readersCount++;

    /* Copy the config into the new dataSetReader */
    UA_StatusCode retVal =
        UA_DataSetReaderConfig_copy(dataSetReaderConfig, &dsr->config);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_DataSetReader_remove(psm, dsr);
        return retVal;
    }

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retVal = addDataSetReaderRepresentation(psm->sc.server, dsr);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(psm->logging, rg,
                            "Adding the DataSetReader to the information model failed");
        UA_DataSetReader_remove(psm, dsr);
        return retVal;
    }
#else
    UA_PubSubManager_generateUniqueNodeId(psm, &dsr->head.identifier);
#endif

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "%SDataSetReader %N\t| ",
                dsr->linkedReaderGroup->head.logIdString,
                dsr->head.identifier);
    dsr->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    UA_LOG_INFO_PUBSUB(psm->logging, dsr, "DataSetReader created (State: %s)",
                       UA_PubSubState_name(dsr->head.state));

    /* Connect to StandaloneSubscribedDataSet if a name is defined */
    const UA_String sdsName = dsr->config.linkedStandaloneSubscribedDataSetName;
    UA_SubscribedDataSet *sds = (UA_String_isEmpty(&sdsName)) ?
        NULL : UA_SubscribedDataSet_findByName(psm, sdsName);
    if(sds) {
        if(sds->config.subscribedDataSetType != UA_PUBSUB_SDS_TARGET) {
            UA_LOG_ERROR_PUBSUB(psm->logging, dsr,
                                "Not implemented! Currently only SubscribedDataSet as "
                                "TargetVariables is implemented");
        } else if(sds->connectedReader) {
            UA_LOG_ERROR_PUBSUB(psm->logging, dsr,
                                "SubscribedDataSet is already connected");
        } else {
            UA_LOG_DEBUG_PUBSUB(psm->logging, dsr,
                                "Found SubscribedDataSet");

            /* Use the MetaData from the sds */
            UA_DataSetMetaDataType_clear(&dsr->config.dataSetMetaData);
            UA_DataSetMetaDataType_copy(&sds->config.dataSetMetaData,
                                        &dsr->config.dataSetMetaData);

            /* Prepare the input for _createTargetVariables and call it */
            UA_TargetVariablesDataType *tvs =
                &sds->config.subscribedDataSet.target;
            DataSetReader_createTargetVariables(psm, dsr, tvs->targetVariablesSize,
                                                tvs->targetVariables);

            sds->connectedReader = dsr; /* Set the backpointer */

            /* Make the connection visible in the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
            connectDataSetReaderToDataSet(psm->sc.server, dsr->head.identifier,
                                          sds->head.identifier);
#endif
        }
    }

    /* Check if used dataSet metaData is valid in context of the rest of the config */
    if(dsr->config.dataSetFieldContentMask & UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        for(size_t fieldIdx = 0;
            fieldIdx < dsr->config.dataSetMetaData.fieldsSize; fieldIdx++) {
            const UA_FieldMetaData *field =
                &dsr->config.dataSetMetaData.fields[fieldIdx];
            if((field->builtInType == UA_NS0ID_STRING ||
                field->builtInType == UA_NS0ID_BYTESTRING) &&
               field->maxStringLength == 0) {
                /* Fields of type String or ByteString need to have defined
                 * MaxStringLength*/
                UA_LOG_ERROR_PUBSUB(psm->logging, dsr,
                                    "Add DataSetReader failed. MaxStringLength must be "
                                    "set in MetaData when using RawData field encoding.");
                UA_DataSetReader_remove(psm, dsr);
                return UA_STATUSCODE_BADCONFIGURATIONERROR;
            }
        }
    }

    if(readerIdentifier)
        UA_NodeId_copy(&dsr->head.identifier, readerIdentifier);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetReader_remove(UA_PubSubManager *psm, UA_DataSetReader *dsr) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    UA_ReaderGroup *rg = dsr->linkedReaderGroup;
    UA_assert(rg);

    /* Check if the ReaderGroup is enabled with realtime options. Disallow
     * removal in that case. The RT path might still have a pointer to the
     * DataSetReader. Or we violate the fixed-size-message configuration.*/
    if(UA_PubSubState_isEnabled(rg->head.state)) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "Removal of DataSetReader not possible while "
                              "the ReaderGroup with realtime options is enabled");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Disable and signal to the application */
    UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_DISABLED,
                                    UA_STATUSCODE_BADSHUTDOWN);

    /* Remove from information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(psm->sc.server, dsr->head.identifier, true);
#endif

    /* Check if a Standalone-SubscribedDataSet is associated with this reader
     * and disconnect it*/
    const UA_String sdsName = dsr->config.linkedStandaloneSubscribedDataSetName;
    UA_SubscribedDataSet *sds = (UA_String_isEmpty(&sdsName)) ?
        NULL : UA_SubscribedDataSet_findByName(psm, sdsName);
    if(sds && sds->connectedReader == dsr)
        sds->connectedReader = NULL;

    /* Remove DataSetReader from group */
    LIST_REMOVE(dsr, listEntry);
    rg->readersCount--;

    UA_LOG_INFO_PUBSUB(psm->logging, dsr, "DataSetReader deleted");

    UA_DataSetReaderConfig_clear(&dsr->config);
    UA_PubSubComponentHead_clear(&dsr->head);
    UA_free(dsr);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetReaderConfig_copy(const UA_DataSetReaderConfig *src,
                            UA_DataSetReaderConfig *dst) {
    memset(dst, 0, sizeof(UA_DataSetReaderConfig));
    dst->writerGroupId = src->writerGroupId;
    dst->dataSetWriterId = src->dataSetWriterId;
    dst->expectedEncoding = src->expectedEncoding;
    dst->dataSetFieldContentMask = src->dataSetFieldContentMask;
    dst->messageReceiveTimeout = src->messageReceiveTimeout;

    UA_StatusCode ret = UA_String_copy(&src->name, &dst->name);
    ret |= UA_PublisherId_copy(&src->publisherId, &dst->publisherId);
    ret |= UA_DataSetMetaDataType_copy(&src->dataSetMetaData, &dst->dataSetMetaData);
    ret |= UA_ExtensionObject_copy(&src->messageSettings, &dst->messageSettings);
    ret |= UA_ExtensionObject_copy(&src->transportSettings, &dst->transportSettings);
    ret |= UA_String_copy(&src->linkedStandaloneSubscribedDataSetName,
                             &dst->linkedStandaloneSubscribedDataSetName);

    if(src->subscribedDataSetType == UA_PUBSUB_SDS_TARGET) {
        ret |= UA_TargetVariablesDataType_copy(&src->subscribedDataSet.target,
                                               &dst->subscribedDataSet.target);
    }

    if(ret != UA_STATUSCODE_GOOD)
        UA_DataSetReaderConfig_clear(dst);

    return ret;
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
        UA_TargetVariablesDataType_clear(&cfg->subscribedDataSet.target);
    }
}

void
UA_DataSetReader_setPubSubState(UA_PubSubManager *psm, UA_DataSetReader *dsr,
                                UA_PubSubState targetState,
                                UA_StatusCode errorReason) {
    UA_ReaderGroup *rg = dsr->linkedReaderGroup;
    UA_assert(rg);

    UA_Server *server = psm->sc.server;
    UA_PubSubState oldState = dsr->head.state;

    /* Custom state machine */
    if(dsr->config.customStateMachine) {
        errorReason = dsr->config.customStateMachine(server, dsr->head.identifier,
                                                     dsr->config.context,
                                                     &dsr->head.state, targetState);
        goto finalize_state_machine;
    }

    /* Internal state machine */
    switch(targetState) {
        /* Disabled */
    case UA_PUBSUBSTATE_DISABLED:
    case UA_PUBSUBSTATE_ERROR:
        dsr->head.state = targetState;
        break;

        /* Enabled */
    case UA_PUBSUBSTATE_PAUSED:
    case UA_PUBSUBSTATE_PREOPERATIONAL:
    case UA_PUBSUBSTATE_OPERATIONAL:
        if(rg->head.state == UA_PUBSUBSTATE_DISABLED ||
           rg->head.state == UA_PUBSUBSTATE_ERROR ||
           rg->head.state == UA_PUBSUBSTATE_PAUSED) {
            dsr->head.state = UA_PUBSUBSTATE_PAUSED; /* RG is disabled -> paused */
        } else {
            dsr->head.state = rg->head.state; /* RG is enabled -> same state */
        }
        break;

    default:
        dsr->head.state = UA_PUBSUBSTATE_ERROR;
        errorReason = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }

    /* Only keep the timeout callback if the reader is operational */
    if(dsr->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
       dsr->msgRcvTimeoutTimerId != 0) {
        UA_EventLoop *el = psm->sc.server->config.eventLoop;
        el->removeTimer(el, dsr->msgRcvTimeoutTimerId);
        dsr->msgRcvTimeoutTimerId = 0;
    }

 finalize_state_machine:

    /* Inform application about state change */
    if(dsr->head.state != oldState) {
        UA_LOG_INFO_PUBSUB(psm->logging, dsr, "%s -> %s",
                           UA_PubSubState_name(oldState),
                           UA_PubSubState_name(dsr->head.state));
        if(server->config.pubSubConfig.stateChangeCallback != 0) {
            server->config.pubSubConfig.
                stateChangeCallback(server, dsr->head.identifier,
                                    dsr->head.state, errorReason);
        }
    }
}

/* This Method is used to initially set the SubscribedDataSet to
 * TargetVariablesType and to create the list of target Variables of a
 * SubscribedDataSetType. */
UA_StatusCode
DataSetReader_createTargetVariables(UA_PubSubManager *psm, UA_DataSetReader *dsr,
                                    size_t tvsSize, const UA_FieldTargetDataType *tvs) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    if(UA_PubSubState_isEnabled(dsr->head.state)) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "Cannot create Target Variables failed while "
                              "the DataSetReader is enabled");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_TargetVariablesDataType newVars;
    UA_TargetVariablesDataType tmp = {tvsSize, (UA_FieldTargetDataType*)(uintptr_t)tvs};
    UA_StatusCode res = UA_TargetVariablesDataType_copy(&tmp, &newVars);   
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_TargetVariablesDataType_clear(&dsr->config.subscribedDataSet.target);
    dsr->config.subscribedDataSet.target = newVars;
    return UA_STATUSCODE_GOOD;
}

static void
DataSetReader_processRaw(UA_PubSubManager *psm, UA_DataSetReader *dsr,
                         UA_DataSetMessage* msg) {
    UA_LOG_TRACE_PUBSUB(psm->logging, dsr, "Received RAW Frame");

    if(dsr->config.dataSetMetaData.fieldsSize !=
       dsr->config.subscribedDataSet.target.targetVariablesSize) {
        UA_LOG_ERROR_PUBSUB(psm->logging, dsr, "Inconsistent number of fields configured");
        return;
    }

    msg->data.keyFrameData.fieldCount = (UA_UInt16)
        dsr->config.dataSetMetaData.fieldsSize;

    /* Start iteration from beginning of rawFields buffer */
    size_t offset = 0;
    UA_TargetVariablesDataType *tvs = &dsr->config.subscribedDataSet.target;
    for(size_t i = 0; i < tvs->targetVariablesSize ; i++) {
        UA_FieldTargetDataType *tv = &tvs->targetVariables[i];

        /* TODO The datatype reference should be part of the internal
         * pubsub configuration to avoid the time-expensive lookup */
        const UA_DataType *type =
            UA_findDataTypeWithCustom(&dsr->config.dataSetMetaData.fields[i].dataType,
                                      psm->sc.server->config.customDataTypes);
        if(!type) {
            UA_LOG_ERROR_PUBSUB(psm->logging, dsr, "Type not found");
            return;
        }

        /* For arrays the length of the array is encoded before the actual data */
        size_t elementCount = 1;
        for(int cnt = 0; cnt < dsr->config.dataSetMetaData.fields[i].valueRank; cnt++) {
            UA_UInt32 dimSize =
                *(UA_UInt32 *)&msg->data.keyFrameData.rawFields.data[offset];
            if(dimSize != dsr->config.dataSetMetaData.fields[i].arrayDimensions[cnt]) {
                UA_LOG_INFO_PUBSUB(psm->logging, dsr,
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
                UA_LOG_INFO_PUBSUB(psm->logging, dsr,
                                   "Error during Raw-decode KeyFrame field %u: %s",
                                   (unsigned)i, UA_StatusCode_name(res));
                return;
            }
            valPtr += type->memSize;
        }

        /* Write the value */
        UA_WriteValue writeVal;
        UA_WriteValue_init(&writeVal);
        writeVal.attributeId = tv->attributeId;
        writeVal.indexRange = tv->receiverIndexRange;
        writeVal.nodeId = tv->targetNodeId;
        if(dsr->config.dataSetMetaData.fields[i].valueRank > 0) {
            UA_Variant_setArray(&writeVal.value.value, value, elementCount, type);
        } else {
            UA_Variant_setScalar(&writeVal.value.value, value, type);
        }
        writeVal.value.hasValue = true;
        Operation_Write(psm->sc.server, &psm->sc.server->adminSession, NULL, &writeVal, &res);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                                  "Error writing KeyFrame field %u: %s",
                                  (unsigned)i, UA_StatusCode_name(res));
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

static void
UA_DataSetReader_handleMessageReceiveTimeout(UA_PubSubManager *psm,
                                             UA_DataSetReader *dsr) {
    UA_assert(dsr->head.componentType == UA_PUBSUBCOMPONENT_DATASETREADER);

    /* Don't signal an error if we don't expect messages to arrive */
    if(dsr->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
       dsr->head.state != UA_PUBSUBSTATE_PREOPERATIONAL)
        return;

    UA_LOG_DEBUG_PUBSUB(psm->logging, dsr, "Message receive timeout occurred");

    lockServer(psm->sc.server);
    UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_ERROR,
                                    UA_STATUSCODE_BADTIMEOUT);
    unlockServer(psm->sc.server);
}

void
UA_DataSetReader_process(UA_PubSubManager *psm, UA_DataSetReader *dsr,
                         UA_DataSetMessage *msg) {
    if(!dsr || !msg || !psm)
        return;

    UA_LOG_DEBUG_PUBSUB(psm->logging, dsr, "Received a network message");

    /* Received a (first) message for the Reader.
     * Transition from PreOperational to Operational. */
    if(dsr->head.state == UA_PUBSUBSTATE_PREOPERATIONAL)
        UA_DataSetReader_setPubSubState(psm, dsr, dsr->head.state, UA_STATUSCODE_GOOD);

    if(dsr->head.state != UA_PUBSUBSTATE_OPERATIONAL &&
       dsr->head.state != UA_PUBSUBSTATE_PREOPERATIONAL) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "Received a network message but not operational");
        return;
    }

    if(!msg->header.dataSetMessageValid) {
        UA_LOG_INFO_PUBSUB(psm->logging, dsr,
                           "DataSetMessage is discarded: message is not valid");
        return;
    }

    /* TODO: Check ConfigurationVersion */
    /* if(msg->header.configVersionMajorVersionEnabled) {
     *     if(msg->header.configVersionMajorVersion !=
     *            dsr->config.dataSetMetaData.configurationVersion.majorVersion) {
     *         UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_SERVER,
     *                        "DataSetMessage is discarded: ConfigurationVersion "
     *                        "MajorVersion does not match");
     *         return;
     *     }
     * } */

    if(msg->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "DataSetMessage is discarded: Only keyframes are supported");
        return;
    }

    /* Configure / Update the timeout callback */
    if(dsr->config.messageReceiveTimeout > 0.0) {
        UA_EventLoop *el = psm->sc.server->config.eventLoop;
        if(dsr->msgRcvTimeoutTimerId == 0) {
            el->addTimer(el, (UA_Callback)UA_DataSetReader_handleMessageReceiveTimeout,
                         psm, dsr, dsr->config.messageReceiveTimeout, NULL,
                         UA_TIMERPOLICY_CURRENTTIME, &dsr->msgRcvTimeoutTimerId);
        } else {
            /* Reset the next execution time to now + interval */
            el->modifyTimer(el, dsr->msgRcvTimeoutTimerId,
                            dsr->config.messageReceiveTimeout, NULL,
                            UA_TIMERPOLICY_CURRENTTIME);
        }
    }

    /* Process message with raw encoding. We have no field-count information for
     * the message. */
    if(msg->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
        DataSetReader_processRaw(psm, dsr, msg);
        return;
    }

    /* Received a heartbeat with no fields */
    if(msg->data.keyFrameData.fieldCount == 0)
        return;

    /* Check whether the field count matches the configuration */
    size_t fieldCount = msg->data.keyFrameData.fieldCount;
    if(dsr->config.dataSetMetaData.fieldsSize != fieldCount) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "Number of fields does not match the "
                              "DataSetMetaData configuration");
        return;
    }

    UA_TargetVariablesDataType *tvs = &dsr->config.subscribedDataSet.target;;
    if(tvs->targetVariablesSize != fieldCount) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "Number of fields does not match the "
                              "TargetVariables configuration");
        return;
    }

    /* Write the message fields. RT has the external data value configured. */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < fieldCount; i++) {
        UA_FieldTargetDataType *tv = &tvs->targetVariables[i];
        UA_DataValue *field = &msg->data.keyFrameData.dataSetFields[i];
        if(!field->hasValue)
            continue;

        /* Write via the Write-Service */
        UA_WriteValue writeVal;
        UA_WriteValue_init(&writeVal);
        writeVal.attributeId = tv->attributeId;
        writeVal.indexRange = tv->receiverIndexRange;
        writeVal.nodeId = tv->targetNodeId;
        writeVal.value = *field;
        Operation_Write(psm->sc.server, &psm->sc.server->adminSession,
                        NULL, &writeVal, &res);
        if(res != UA_STATUSCODE_GOOD)
            UA_LOG_INFO_PUBSUB(psm->logging, dsr,
                               "Error writing KeyFrame field %u: %s",
                               (unsigned)i, UA_StatusCode_name(res));
    }
}

/**************/
/* Server API */
/**************/

UA_StatusCode
UA_Server_addDataSetReader(UA_Server *server, UA_NodeId readerGroupIdentifier,
                           const UA_DataSetReaderConfig *dataSetReaderConfig,
                           UA_NodeId *readerIdentifier) {
    if(!server || !dataSetReaderConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_StatusCode res =
        UA_DataSetReader_create(getPSM(server), readerGroupIdentifier,
                                dataSetReaderConfig, readerIdentifier);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_removeDataSetReader(UA_Server *server, UA_NodeId readerIdentifier) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, readerIdentifier);
    UA_StatusCode res = (dsr) ?
        UA_DataSetReader_remove(psm, dsr) : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getDataSetReaderConfig(UA_Server *server, const UA_NodeId dsrId,
                                 UA_DataSetReaderConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dsrId);
    UA_StatusCode res = (dsr) ?
        UA_DataSetReaderConfig_copy(&dsr->config, config) : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getDataSetReaderState(UA_Server *server, const UA_NodeId dsrId,
                                UA_PubSubState *state) {
    if(!server || !state)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(getPSM(server), dsrId);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    if(dsr) {
        res = UA_STATUSCODE_GOOD;
        *state = dsr->head.state;
    }
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_enableDataSetReader(UA_Server *server, const UA_NodeId dsrId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dsrId);
    if(dsr)
        UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_OPERATIONAL,
                                        UA_STATUSCODE_GOOD);
    else
        ret = UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return ret;
}

UA_StatusCode
UA_Server_disableDataSetReader(UA_Server *server, const UA_NodeId dsrId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dsrId);
    if(dsr)
        UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_DISABLED,
                                        UA_STATUSCODE_GOOD);
    else
        ret = UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return ret;
}

UA_StatusCode
UA_Server_setDataSetReaderTargetVariables(UA_Server *server, const UA_NodeId dsrId,
    size_t targetVariablesSize, const UA_FieldTargetDataType *targetVariables) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dsrId);
    UA_StatusCode res = (dsr) ?
        DataSetReader_createTargetVariables(psm, dsr, targetVariablesSize,
                                            targetVariables) : UA_STATUSCODE_BADNOTFOUND;
    unlockServer(server);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
