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
        for(size_t i = 0; i < msg->messageCount; i++) {
            if(dsr->config.dataSetWriterId == msg->dataSetWriterIds[i])
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

static UA_StatusCode
validateDSRConfig(UA_PubSubManager *psm, UA_DataSetReader *dsr) {
    /* Check if used dataSet metaData is valid in context of the rest of the config */
    if(dsr->config.dataSetFieldContentMask & UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        for(size_t i = 0; i < dsr->config.dataSetMetaData.fieldsSize; i++) {
            const UA_FieldMetaData *field = &dsr->config.dataSetMetaData.fields[i];
            if((field->builtInType == UA_NS0ID_STRING || field->builtInType == UA_NS0ID_BYTESTRING) &&
               field->maxStringLength == 0) {
                /* Fields of type String or ByteString need to have defined
                 * MaxStringLength*/
                UA_LOG_ERROR_PUBSUB(psm->logging, dsr,
                                    "Add DataSetReader failed. MaxStringLength must be "
                                    "set in MetaData when using RawData field encoding.");
                return UA_STATUSCODE_BADCONFIGURATIONERROR;
            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

static void
disconnectDSR2Standalone(UA_PubSubManager *psm, UA_DataSetReader *dsr) {
    /* Check if a sds name is defined */
    const UA_String sdsName = dsr->config.linkedStandaloneSubscribedDataSetName;
    if(UA_String_isEmpty(&sdsName))
        return;

    UA_SubscribedDataSet *sds = UA_SubscribedDataSet_findByName(psm, sdsName);
    if(!sds)
        return;

    /* Remove the backpointer from the sds */
    sds->connectedReader = NULL;

    /* Remove the references in the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    disconnectDataSetReaderToDataSet(psm->sc.server, dsr->head.identifier);
#endif
}

/* Connect to StandaloneSubscribedDataSet if a name is defined */
static UA_StatusCode
connectDSR2Standalone(UA_PubSubManager *psm, UA_DataSetReader *dsr) {
    /* Check if a sds name is defined */
    const UA_String sdsName = dsr->config.linkedStandaloneSubscribedDataSetName;
    if(UA_String_isEmpty(&sdsName))
        return UA_STATUSCODE_GOOD;

    UA_SubscribedDataSet *sds = UA_SubscribedDataSet_findByName(psm, sdsName);
    if(!sds)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Already connected? */
    if(sds->connectedReader) {
        if(sds->connectedReader != dsr)
            UA_LOG_ERROR_PUBSUB(psm->logging, dsr,
                                "Configured StandaloneSubscribedDataSet already "
                                "connected to a different DataSetReader");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check supported type */
    if(sds->config.subscribedDataSetType != UA_PUBSUB_SDS_TARGET) {
        UA_LOG_ERROR_PUBSUB(psm->logging, dsr,
                            "Not implemented! Currently only SubscribedDataSet as "
                            "TargetVariables is implemented");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    UA_LOG_DEBUG_PUBSUB(psm->logging, dsr, "Connecting SubscribedDataSet");

    /* Copy the metadata from the sds */
    UA_DataSetMetaDataType metaData;
    UA_StatusCode res = UA_DataSetMetaDataType_copy(&sds->config.dataSetMetaData, &metaData);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Prepare the input for _createTargetVariables and call it */
    UA_TargetVariablesDataType *tvs = &sds->config.subscribedDataSet.target;
    res = DataSetReader_createTargetVariables(psm, dsr, tvs->targetVariablesSize,
                                              tvs->targetVariables);
    if(res != UA_STATUSCODE_GOOD) {
        UA_DataSetMetaDataType_clear(&metaData);
        return res;
    }

    /* Use the metadata from the sds */
    UA_DataSetMetaDataType_clear(&dsr->config.dataSetMetaData);
    dsr->config.dataSetMetaData = metaData;

    /* Set the backpointer from the sds */
    sds->connectedReader = dsr;

    /* Make the connection visible in the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    return connectDataSetReaderToDataSet(psm->sc.server, dsr->head.identifier,
                                         sds->head.identifier);
#else
    return UA_STATUSCODE_GOOD;
#endif
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
                              "ReaderGroup is enabled");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Allocate memory for new DataSetReader */
    UA_DataSetReader *dsr = (UA_DataSetReader *)
        UA_calloc(1, sizeof(UA_DataSetReader));
    if(!dsr)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    dsr->head.componentType = UA_PUBSUBCOMPONENT_DATASETREADER;
    dsr->linkedReaderGroup = rg;

    /* Add the new reader to the group. Add to the end of the linked list to
     * ensure the order for the realtime offsets is as expected. The received
     * DataSetMessages are matched via UA_DataSetReader_checkIdentifier for the
     * non-RT path. */
    UA_DataSetReader *after = LIST_FIRST(&rg->readers);
    if(!after) {
        LIST_INSERT_HEAD(&rg->readers, dsr, listEntry);
    } else {
        while(LIST_NEXT(after, listEntry))
            after = LIST_NEXT(after, listEntry);
        LIST_INSERT_AFTER(after, dsr, listEntry);
    }
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
                dsr->linkedReaderGroup->head.logIdString, dsr->head.identifier);
    dsr->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    /* Connect to StandaloneSubscribedDataSet if a name is defined. Needs to be
     * added in the information model first, as this adds references to the
     * StandaloneSubscribedDataSet. */
    retVal = connectDSR2Standalone(psm, dsr);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_DataSetReader_remove(psm, dsr);
        return retVal;
    }

    /* Validate the config */
    retVal = validateDSRConfig(psm, dsr);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_DataSetReader_remove(psm, dsr);
        return retVal;
    }

    /* Notify the application that a new Reader was created.
     * This may internally adjust the config */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.componentLifecycleCallback) {
        UA_StatusCode res = server->config.pubSubConfig.
            componentLifecycleCallback(server, dsr->head.identifier,
                                       UA_PUBSUBCOMPONENT_DATASETREADER, false);
        if(res != UA_STATUSCODE_GOOD) {
            UA_DataSetReader_remove(psm, dsr);
            return res;
        }
    }

    UA_LOG_INFO_PUBSUB(psm->logging, dsr, "DataSetReader created (State: %s)",
                       UA_PubSubState_name(dsr->head.state));

    if(readerIdentifier)
        UA_NodeId_copy(&dsr->head.identifier, readerIdentifier);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetReader_remove(UA_PubSubManager *psm, UA_DataSetReader *dsr) {
    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    UA_ReaderGroup *rg = dsr->linkedReaderGroup;
    UA_assert(rg);

    /* Check if the ReaderGroup is enabled */
    if(UA_PubSubState_isEnabled(rg->head.state)) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "Removal of the DataSetReader not possible while "
                              "the ReaderGroup is enabled");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Check with the application if we can remove */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.componentLifecycleCallback) {
        UA_StatusCode res = server->config.pubSubConfig.
            componentLifecycleCallback(server, dsr->head.identifier,
                                       UA_PUBSUBCOMPONENT_DATASETREADER, true);
        if(res != UA_STATUSCODE_GOOD)
            return res;
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
    memcpy(dst, src, sizeof(UA_DataSetReaderConfig));
    dst->writerGroupId = src->writerGroupId;
    dst->dataSetWriterId = src->dataSetWriterId;
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
                                UA_PubSubState targetState, UA_StatusCode errorReason) {
    UA_ReaderGroup *rg = dsr->linkedReaderGroup;
    UA_assert(rg);

    /* Callback to modify the WriterGroup config and change the targetState
     * before the state machine executes */
    UA_Server *server = psm->sc.server;
    if(server->config.pubSubConfig.beforeStateChangeCallback) {
        server->config.pubSubConfig.
            beforeStateChangeCallback(server, dsr->head.identifier, &targetState);
    }

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

    /* Received a heartbeat with no fields */
    if(msg->fieldCount == 0)
        return;

    /* Check whether the field count matches the configuration */
    UA_TargetVariablesDataType *tvs = &dsr->config.subscribedDataSet.target;
    if(tvs->targetVariablesSize != msg->fieldCount) {
        UA_LOG_WARNING_PUBSUB(psm->logging, dsr,
                              "Number of fields does not match the "
                              "TargetVariables configuration");
        return;
    }

    /* Write the message fields. RT has the external data value configured. */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < msg->fieldCount; i++) {
        UA_FieldTargetDataType *tv = &tvs->targetVariables[i];
        UA_DataValue *field = &msg->data.keyFrameFields[i];
        if(!field->hasValue)
            continue;

        /* Write via the Write-Service */
        UA_WriteValue writeVal;
        UA_WriteValue_init(&writeVal);
        writeVal.attributeId = tv->attributeId;
        writeVal.indexRange = tv->receiverIndexRange;
        writeVal.nodeId = tv->targetNodeId;
        writeVal.value = *field;
        Operation_Write(psm->sc.server, &psm->sc.server->adminSession, &writeVal, &res);
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
UA_Server_addDataSetReader(UA_Server *server, UA_NodeId readerGroupId,
                           const UA_DataSetReaderConfig *config,
                           UA_NodeId *dsrId) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_StatusCode res =
        UA_DataSetReader_create(getPSM(server), readerGroupId, config, dsrId);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_removeDataSetReader(UA_Server *server, const UA_NodeId readerId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, readerId);
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
                                          size_t targetVariablesSize,
                                          const UA_FieldTargetDataType *targetVariables) {
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

UA_StatusCode
UA_Server_updateDataSetReaderConfig(UA_Server *server, const UA_NodeId dsrId,
                                    const UA_DataSetReaderConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dsrId);
    if(!dsr) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    if(UA_PubSubState_isEnabled(dsr->head.state)) {
        UA_LOG_ERROR_PUBSUB(psm->logging, dsr,
                            "The DataSetReader must be disabled to update the config");
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Store the old config */
    UA_DataSetReaderConfig oldConfig = dsr->config;

    /* Copy the config into the new dataSetReader */
    UA_StatusCode retVal = UA_DataSetReaderConfig_copy(config, &dsr->config);
    if(retVal != UA_STATUSCODE_GOOD)
        goto errout;

    /* Change the connection to a StandaloneSubscribedDataSet */
    if(!UA_String_equal(&dsr->config.linkedStandaloneSubscribedDataSetName,
                        &oldConfig.linkedStandaloneSubscribedDataSetName)) {
        disconnectDSR2Standalone(psm, dsr);
        retVal = connectDSR2Standalone(psm, dsr);
        if(retVal != UA_STATUSCODE_GOOD)
            goto errout;
    }

    /* Validate the new config */
    retVal = validateDSRConfig(psm, dsr);
    if(retVal != UA_STATUSCODE_GOOD)
        goto errout;

    /* Call the state-machine. This can move the connection state from _ERROR to
     * _DISABLED. */
    UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_DISABLED,
                                    UA_STATUSCODE_GOOD);

    /* Clean up and return */
    UA_DataSetReaderConfig_clear(&oldConfig);
    unlockServer(server);
    return UA_STATUSCODE_GOOD;

    /* Fall back to the old config */
 errout:
    UA_DataSetReaderConfig_clear(&dsr->config);
    dsr->config = oldConfig;
    unlockServer(server);
    return retVal;
}

/**********************/
/* Offset Computation */
/**********************/

static UA_StatusCode
UA_PubSubDataSetReader_generateKeyFrameMessage(UA_Server *server,
                                               UA_DataSetMessage *dsm,
                                               UA_DataSetReader *dsr) {
    /* Prepare DataSetMessageContent */
    UA_TargetVariablesDataType *tv = &dsr->config.subscribedDataSet.target;
    UA_DataSetMetaDataType *metaData = &dsr->config.dataSetMetaData;
    if(tv->targetVariablesSize != metaData->fieldsSize)
        metaData = NULL;
    dsm->header.dataSetMessageValid = true;
    dsm->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dsm->fieldCount = (UA_UInt16) tv->targetVariablesSize;
    dsm->data.keyFrameFields = (UA_DataValue *)
            UA_Array_new(tv->targetVariablesSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
    if(!dsm->data.keyFrameFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

     for(size_t counter = 0; counter < tv->targetVariablesSize; counter++) {
        /* Read the value and set the source in the reader config */
        UA_DataValue *dfv = &dsm->data.keyFrameFields[counter];
        UA_FieldTargetDataType *ftv = &tv->targetVariables[counter];

        /* Synthesize the field value from the FieldMetaData. This allows us to
         * prevent a read from the information model during startup. */
        UA_FieldMetaData *fieldMetaData = (metaData) ? &metaData->fields[counter] : NULL;
        if(fieldMetaData && fieldMetaData->valueRank == UA_VALUERANK_SCALAR) {
            const UA_DataType *type =
                UA_findDataTypeWithCustom(&fieldMetaData->dataType,
                                          server->config.customDataTypes);
            if(type == &UA_TYPES[UA_TYPES_STRING] && fieldMetaData->maxStringLength > 0) {
                UA_String *s = UA_String_new();
                if(!s) {
                    UA_DataSetMessage_clear(dsm);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                s->data = (UA_Byte*)
                    UA_calloc(fieldMetaData->maxStringLength, sizeof(UA_Byte));
                if(!s->data) {
                    UA_free(s);
                    UA_DataSetMessage_clear(dsm);
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                }
                s->length = fieldMetaData->maxStringLength;
                UA_Variant_setScalar(&dfv->value, s, type);
                dfv->hasValue = true;
            } else if(type && type->memSize < 512) {
                char buf[512];
                UA_init(buf, type);
                UA_StatusCode res = UA_Variant_setScalarCopy(&dfv->value, buf, type);
                if(res != UA_STATUSCODE_GOOD) {
                    UA_DataSetMessage_clear(dsm);
                    return res;
                }
                dfv->hasValue = true;
            }
        }

        /* Read the value from the information model */
        if(!dfv->hasValue) {
            UA_ReadValueId rvi;
            UA_ReadValueId_init(&rvi);
            rvi.nodeId = ftv->targetNodeId;
            rvi.attributeId = ftv->attributeId;
            rvi.indexRange = ftv->writeIndexRange;
            *dfv = readWithSession(server, &server->adminSession, &rvi,
                                   UA_TIMESTAMPSTORETURN_NEITHER);
        }

        /* Deactivate statuscode? */
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
            dfv->hasStatus = false;

        /* Deactivate timestamps */
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP) == 0)
            dfv->hasSourceTimestamp = false;
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS) == 0)
            dfv->hasSourcePicoseconds = false;
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERTIMESTAMP) == 0)
            dfv->hasServerTimestamp = false;
        if(((u64)dsr->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS) == 0)
            dfv->hasServerPicoseconds = false;
    }

    return UA_STATUSCODE_GOOD;
}

/* Generate a DataSetMessage for the given reader. */
UA_StatusCode
UA_DataSetReader_generateDataSetMessage(UA_Server *server,
                                        UA_DataSetMessage *dsm,
                                        UA_DataSetReader *dsr) {
    /* Support only for UADP configuration
     * TODO: JSON encoding if UA_DataSetReader_generateDataSetMessage used other
     * that RT configuration */

    UA_ExtensionObject *settings = &dsr->config.messageSettings;
    if(settings->content.decoded.type != &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE])
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* The configuration Flags are included inside the std. defined
     * UA_UadpDataSetReaderMessageDataType */
    UA_UadpDataSetReaderMessageDataType defaultUadpConfiguration;
    UA_UadpDataSetReaderMessageDataType *dsrMessageDataType =
        (UA_UadpDataSetReaderMessageDataType*) settings->content.decoded.data;

    if(!(settings->encoding == UA_EXTENSIONOBJECT_DECODED ||
         settings->encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       !dsrMessageDataType->dataSetMessageContentMask) {
        /* Create default flag configuration if no dataSetMessageContentMask or
         * even messageSettings in UadpDataSetWriterMessageDataType was
         * passed. */
        memset(&defaultUadpConfiguration, 0, sizeof(UA_UadpDataSetReaderMessageDataType));
        defaultUadpConfiguration.dataSetMessageContentMask = (UA_UadpDataSetMessageContentMask)
            ((u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION);
        dsrMessageDataType = &defaultUadpConfiguration;
    }

    /* The field encoding depends on the flags inside the reader config. */
    if(dsr->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        dsm->header.fieldEncoding = UA_FIELDENCODING_RAWDATA;
    } else if((u64)dsr->config.dataSetFieldContentMask &
              ((u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP |
               (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE)) {
        dsm->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    } else {
        dsm->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    }

    /* Std: 'The DataSetMessageContentMask defines the flags for the content
     * of the DataSetMessage header.' */
    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION) {
        dsm->header.configVersionMajorVersionEnabled = true;
        dsm->header.configVersionMajorVersion =
            dsr->config.dataSetMetaData.configurationVersion.majorVersion;
    }

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION) {
        dsm->header.configVersionMinorVersionEnabled = true;
        dsm->header.configVersionMinorVersion =
            dsr->config.dataSetMetaData.configurationVersion.minorVersion;
    }

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
        /* Will be modified when subscriber receives new nw msg */
        dsm->header.dataSetMessageSequenceNrEnabled = true;
        dsm->header.dataSetMessageSequenceNr = 1;
    }

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP) {
        dsm->header.timestampEnabled = true;
        dsm->header.timestamp = UA_DateTime_now();
    }

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_PICOSECONDS)
        dsm->header.picoSecondsIncluded = false;

    if((u64)dsrMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_STATUS)
        dsm->header.statusEnabled = true;

    /* Not supported for Delta frames atm */
    return UA_PubSubDataSetReader_generateKeyFrameMessage(server, dsm, dsr);
}


UA_StatusCode
UA_Server_computeDataSetReaderOffsetTable(UA_Server *server,
                                          const UA_NodeId dataSetReaderId,
                                          UA_PubSubOffsetTable *ot) {
    /* Validate the arguments */
    if(!server || !ot)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    /* Get the DataSetReader */
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dataSetReaderId);
    if(!dsr) {
        unlockServer(server);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Generate the DataSetMessage */
    UA_DataSetMessage dsm;
    memset(&dsm, 0, sizeof(UA_DataSetMessage));
    UA_StatusCode res = UA_DataSetReader_generateDataSetMessage(server, &dsm, dsr);
    if(res != UA_STATUSCODE_GOOD) {
        unlockServer(server);
        return res;
    }

    /* Reset the OffsetTable */
    memset(ot, 0, sizeof(UA_PubSubOffsetTable));

    /* Prepare the encoding context */
    UA_DataSetMessage_EncodingMetaData emd;
    memset(&emd, 0, sizeof(UA_DataSetMessage_EncodingMetaData));
    emd.dataSetWriterId = dsr->config.dataSetWriterId;
    emd.fields = dsr->config.dataSetMetaData.fields;
    emd.fieldsSize = dsr->config.dataSetMetaData.fieldsSize;

    PubSubEncodeCtx ctx;
    memset(&ctx, 0, sizeof(PubSubEncodeCtx));
    ctx.ot = ot;
    ctx.eo.metaData = &emd;
    ctx.eo.metaDataSize = 1;

    /* Compute the offset */
    size_t fieldindex = 0;
    UA_FieldTargetDataType *tv = NULL;
    size_t msgSize = UA_DataSetMessage_calcSizeBinary(&ctx, &emd, &dsm, 0);
    if(msgSize == 0) {
        res = UA_STATUSCODE_BADINTERNALERROR;
        goto errout;
    }

    /* Allocate the message */
    res = UA_ByteString_allocBuffer(&ot->networkMessage, msgSize);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Create the ByteString of the encoded DataSetMessage */
    ctx.ctx.pos = ot->networkMessage.data;
    ctx.ctx.end = ot->networkMessage.data + ot->networkMessage.length;
    res = UA_DataSetMessage_encodeBinary(&ctx, &emd, &dsm);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;

    /* Pick up the component NodeIds */
    for(size_t i = 0; i < ot->offsetsSize; i++) {
        UA_PubSubOffset *o = &ot->offsets[i];
        switch(o->offsetType) {
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_SEQUENCENUMBER:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_STATUS:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_TIMESTAMP:
        case UA_PUBSUBOFFSETTYPE_DATASETMESSAGE_PICOSECONDS:
            res |= UA_NodeId_copy(&dsr->head.identifier, &o->component);
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_DATAVALUE:
            tv = &dsr->config.subscribedDataSet.target.targetVariables[fieldindex];
            res |= UA_NodeId_copy(&tv->targetNodeId, &o->component);
            fieldindex++;
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_VARIANT:
            tv = &dsr->config.subscribedDataSet.target.targetVariables[fieldindex];
            res |= UA_NodeId_copy(&tv->targetNodeId, &o->component);
            fieldindex++;
            break;
        case UA_PUBSUBOFFSETTYPE_DATASETFIELD_RAW:
            tv = &dsr->config.subscribedDataSet.target.targetVariables[fieldindex];
            res |= UA_NodeId_copy(&tv->targetNodeId, &o->component);
            fieldindex++;
            break;
        default:
            res = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }
    }

    /* Clean up */
 errout:
    UA_DataSetMessage_clear(&dsm);
    if(res != UA_STATUSCODE_GOOD)
        UA_PubSubOffsetTable_clear(ot);
    unlockServer(server);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
