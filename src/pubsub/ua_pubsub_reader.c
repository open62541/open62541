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

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
#include "ua_types_encoding_binary.h"
#endif

#ifdef UA_ENABLE_PUBSUB_BUFMALLOC
#include "ua_pubsub_bufmalloc.h"
#endif

#ifdef UA_ENABLE_PUBSUB_MONITORING
static void
UA_DataSetReader_checkMessageReceiveTimeout(UA_Server *server, UA_DataSetReader *dsr);

static void
UA_DataSetReader_handleMessageReceiveTimeout(UA_Server *server, UA_DataSetReader *dsr);
#endif

/* This functionality of this API will be used in future to create mirror Variables - TODO */
/* #define UA_MAX_SIZENAME           64 */ /* Max size of Qualified Name of Subscribed Variable */

static void
UA_PubSubDSRDataSetField_sampleValue(UA_Server *server, UA_DataSetReader *dataSetReader,
                                     UA_DataValue *value, UA_FieldTargetVariable *ftv) {
    /* TODO: Static value source without RT information model
     * This API supports only to external datasource in RT configutation
     * TODO: Extend to support other configuration if required */

    /* Get the Node */
    const UA_VariableNode *rtNode = (const UA_VariableNode *)
        UA_NODESTORE_GET(server, &ftv->targetVariable.targetNodeId);
    if(!rtNode)
        return;

    if(rtNode->valueBackend.backendType == UA_VALUEBACKENDTYPE_EXTERNAL) {
        /* Set the external source in the dataset reader config */
        ftv->externalDataValue = rtNode->valueBackend.backend.external.value;

        /* Get the value to compute the offsets */
        *value = **rtNode->valueBackend.backend.external.value;
        value->value.storageType = UA_VARIANT_DATA_NODELETE;
    }

    UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
}

static UA_StatusCode
UA_PubSubDataSetReader_generateKeyFrameMessage(UA_Server *server,
                                               UA_DataSetMessage *dataSetMessage,
                                               UA_DataSetReader *dataSetReader) {
    /* Prepare DataSetMessageContent */
    UA_TargetVariables *tv = &dataSetReader->config.subscribedDataSet.subscribedDataSetTarget;
    dataSetMessage->header.dataSetMessageValid = true;
    dataSetMessage->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
    dataSetMessage->data.keyFrameData.fieldCount = (UA_UInt16) tv->targetVariablesSize;
    dataSetMessage->data.keyFrameData.dataSetFields = (UA_DataValue *)
            UA_Array_new(tv->targetVariablesSize, &UA_TYPES[UA_TYPES_DATAVALUE]);
    dataSetMessage->data.keyFrameData.dataSetMetaDataType = &dataSetReader->config.dataSetMetaData;
    if(!dataSetMessage->data.keyFrameData.dataSetFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

     for(size_t counter = 0; counter < tv->targetVariablesSize; counter++) {
        /* Sample the value and set the source in the reader config */
        UA_DataValue *dfv = &dataSetMessage->data.keyFrameData.dataSetFields[counter];
        UA_FieldTargetVariable *ftv = &tv->targetVariables[counter];
        UA_PubSubDSRDataSetField_sampleValue(server, dataSetReader, dfv, ftv);

        /* Deactivate statuscode? */
        if(((u64)dataSetReader->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
            dfv->hasStatus = false;

        /* Deactivate timestamps */
        if(((u64)dataSetReader->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP) == 0)
            dfv->hasSourceTimestamp = false;
        if(((u64)dataSetReader->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS) == 0)
            dfv->hasSourcePicoseconds = false;
        if(((u64)dataSetReader->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERTIMESTAMP) == 0)
            dfv->hasServerTimestamp = false;
        if(((u64)dataSetReader->config.dataSetFieldContentMask &
            (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS) == 0)
            dfv->hasServerPicoseconds = false;
    }

    return UA_STATUSCODE_GOOD;
}

/* Generate a DataSetMessage for the given reader. */
UA_StatusCode
UA_DataSetReader_generateDataSetMessage(UA_Server *server,
                                        UA_DataSetMessage *dataSetMessage,
                                        UA_DataSetReader *dataSetReader) {
    /* Reset the message */
    memset(dataSetMessage, 0, sizeof(UA_DataSetMessage));

    /* Support only for UADP configuration
     * TODO: JSON encoding if UA_DataSetReader_generateDataSetMessage used other
     * that RT configuration */

    UA_ExtensionObject *settings = &dataSetReader->config.messageSettings;
    if(settings->content.decoded.type != &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE]) {
        UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                              "Only UADP encoding is supported");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    /* The configuration Flags are included inside the std. defined UA_UadpDataSetReaderMessageDataType */
    UA_UadpDataSetReaderMessageDataType defaultUadpConfiguration;
    UA_UadpDataSetReaderMessageDataType *dataSetReaderMessageDataType =
        (UA_UadpDataSetReaderMessageDataType*) settings->content.decoded.data;

    if(!(settings->encoding == UA_EXTENSIONOBJECT_DECODED ||
         settings->encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       !dataSetReaderMessageDataType->dataSetMessageContentMask) {
        /* create default flag configuration if no dataSetMessageContentMask or even messageSettings in
         * UadpDataSetWriterMessageDataType was passed in */
        memset(&defaultUadpConfiguration, 0, sizeof(UA_UadpDataSetReaderMessageDataType));
        defaultUadpConfiguration.dataSetMessageContentMask = (UA_UadpDataSetMessageContentMask)
            ((u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION);
        dataSetReaderMessageDataType = &defaultUadpConfiguration;
    }

    /* Sanity-test the configuration */
    if(dataSetReaderMessageDataType &&
       (dataSetReaderMessageDataType->networkMessageNumber != 0 ||
        dataSetReaderMessageDataType->dataSetOffset != 0)) {
        UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                              "Static DSM configuration not supported, using defaults");
        dataSetReaderMessageDataType->networkMessageNumber = 0;
        dataSetReaderMessageDataType->dataSetOffset = 0;
    }

    /* The field encoding depends on the flags inside the reader config. */
    if(dataSetReader->config.dataSetFieldContentMask &
       (u64)UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_RAWDATA;
    } else if((u64)dataSetReader->config.dataSetFieldContentMask &
              ((u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP |
               (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE)) {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    } else {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    }

    /* Std: 'The DataSetMessageContentMask defines the flags for the content
     * of the DataSetMessage header.' */
    if((u64)dataSetReaderMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION) {
        dataSetMessage->header.configVersionMajorVersionEnabled = true;
        dataSetMessage->header.configVersionMajorVersion =
            dataSetReader->config.dataSetMetaData.configurationVersion.majorVersion;
    }

    if((u64)dataSetReaderMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION) {
        dataSetMessage->header.configVersionMinorVersionEnabled = true;
        dataSetMessage->header.configVersionMinorVersion =
            dataSetReader->config.dataSetMetaData.configurationVersion.minorVersion;
    }

    if((u64)dataSetReaderMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
        /* Will be modified when subscriber receives new nw msg */
        dataSetMessage->header.dataSetMessageSequenceNrEnabled = true;
        dataSetMessage->header.dataSetMessageSequenceNr = 1;
    }

    if((u64)dataSetReaderMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP) {
        dataSetMessage->header.timestampEnabled = true;
        dataSetMessage->header.timestamp = UA_DateTime_now();
    }

    /* TODO: Picoseconds resolution not supported atm */
    if((u64)dataSetReaderMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_PICOSECONDS) {
        dataSetMessage->header.picoSecondsIncluded = false;
    }

    if((u64)dataSetReaderMessageDataType->dataSetMessageContentMask &
       (u64)UA_UADPDATASETMESSAGECONTENTMASK_STATUS) {
        dataSetMessage->header.statusEnabled = true;
    }

    /* Not supported for Delta frames atm */
    return UA_PubSubDataSetReader_generateKeyFrameMessage(server, dataSetMessage,
                                                          dataSetReader);
}

UA_StatusCode
UA_DataSetReader_generateNetworkMessage(UA_PubSubConnection *pubSubConnection,
                                        UA_ReaderGroup *readerGroup,
                                        UA_DataSetReader *dataSetReader,
                                        UA_DataSetMessage *dsm, UA_UInt16 *writerId,
                                        UA_Byte dsmCount, UA_NetworkMessage *nm) {
    UA_ExtensionObject *settings = &dataSetReader->config.messageSettings;
    if(settings->content.decoded.type != &UA_TYPES[UA_TYPES_UADPDATASETREADERMESSAGEDATATYPE])
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_UadpDataSetReaderMessageDataType *dsrm =
        (UA_UadpDataSetReaderMessageDataType*)settings->content.decoded.data;
    nm->publisherIdEnabled = ((u64)dsrm->networkMessageContentMask &
                              (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID) != 0;
    nm->groupHeaderEnabled = ((u64)dsrm->networkMessageContentMask &
                              (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER) != 0;
    nm->groupHeader.writerGroupIdEnabled = ((u64)dsrm->networkMessageContentMask &
                                            (u64)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID) != 0;
    nm->groupHeader.groupVersionEnabled = ((u64)dsrm->networkMessageContentMask &
                                           (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION) != 0;
    nm->groupHeader.networkMessageNumberEnabled = ((u64)dsrm->networkMessageContentMask &
                                                   (u64)UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER) != 0;
    nm->groupHeader.sequenceNumberEnabled = ((u64)dsrm->networkMessageContentMask &
                                             (u64)UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER) != 0;
    nm->payloadHeaderEnabled = ((u64)dsrm->networkMessageContentMask &
                                (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER) != 0;
    nm->timestampEnabled = ((u64)dsrm->networkMessageContentMask &
                            (u64)UA_UADPNETWORKMESSAGECONTENTMASK_TIMESTAMP) != 0;
    nm->picosecondsEnabled = ((u64)dsrm->networkMessageContentMask &
                              (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PICOSECONDS) != 0;
    nm->dataSetClassIdEnabled = ((u64)dsrm->networkMessageContentMask &
                                 (u64)UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID) != 0;
    nm->promotedFieldsEnabled = ((u64)dsrm->networkMessageContentMask &
                                 (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PROMOTEDFIELDS) != 0;
    /* Set the SecurityHeader */
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(readerGroup->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        nm->securityEnabled = true;
        nm->securityHeader.networkMessageSigned = true;
        if(readerGroup->config.securityMode >= UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            nm->securityHeader.networkMessageEncrypted = true;
        nm->securityHeader.securityTokenId = readerGroup->securityTokenId;

        /* Generate the MessageNonce starting with four random bytes */
        UA_ByteString nonce = {4, nm->securityHeader.messageNonce};
        UA_StatusCode rv = readerGroup->config.securityPolicy->symmetricModule.
            generateNonce(readerGroup->config.securityPolicy->policyContext,
                          &nonce);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
        nm->securityHeader.messageNonceSize = 8;
    }
#endif
    nm->version = 1;
    nm->networkMessageType = UA_NETWORKMESSAGE_DATASET;

    if(!UA_DataType_isNumeric(dataSetReader->config.publisherId.type))
        return UA_STATUSCODE_BADNOTSUPPORTED;

    if (nm->publisherIdEnabled) {
        switch(dataSetReader->config.publisherId.type->typeKind) {
        case UA_DATATYPEKIND_BYTE:
            nm->publisherIdType = UA_PUBLISHERIDTYPE_BYTE;
            nm->publisherId.byte = *(UA_Byte *) dataSetReader->config.publisherId.data;
            break;
        case UA_DATATYPEKIND_UINT16:
            nm->publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
            nm->publisherId.uint16 = *(UA_UInt16 *) dataSetReader->config.publisherId.data;
            break;
        case UA_DATATYPEKIND_UINT32:
            nm->publisherIdType = UA_PUBLISHERIDTYPE_UINT32;
            nm->publisherId.uint32 = *(UA_UInt32 *) dataSetReader->config.publisherId.data;
            break;
        case UA_DATATYPEKIND_UINT64:
            nm->publisherIdType = UA_PUBLISHERIDTYPE_UINT64;
            nm->publisherId.uint64 = *(UA_UInt64 *) dataSetReader->config.publisherId.data;
            break;
        default:
            // UA_PUBLISHERIDTYPE_STRING is not supported because of UA_PUBSUB_RT_FIXED_SIZE
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
    }

    if(nm->groupHeader.sequenceNumberEnabled)
        nm->groupHeader.sequenceNumber = 1; /* Will be modified when subscriber receives new nw msg. */

    if(nm->groupHeader.groupVersionEnabled)
        nm->groupHeader.groupVersion = dsrm->groupVersion;

    /* Compute the length of the dsm separately for the header */
    UA_UInt16 *dsmLengths = (UA_UInt16 *) UA_calloc(dsmCount, sizeof(UA_UInt16));
    if(!dsmLengths)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(UA_Byte i = 0; i < dsmCount; i++){
        dsmLengths[i] = (UA_UInt16) UA_DataSetMessage_calcSizeBinary(&dsm[i], NULL, 0);
        switch(dataSetReader->config.expectedEncoding) {
            case UA_PUBSUB_RT_UNKNOWN:
                break;
            case UA_PUBSUB_RT_VARIANT:
                dsm[i].header.fieldEncoding = UA_FIELDENCODING_VARIANT;
                break;
            case UA_PUBSUB_RT_DATA_VALUE:
                dsm[i].header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
                break;
            case UA_PUBSUB_RT_RAW:
                dsm[i].header.fieldEncoding = UA_FIELDENCODING_RAWDATA;
                break;
        }
    }
    nm->payloadHeader.dataSetPayloadHeader.count = dsmCount;
    nm->payloadHeader.dataSetPayloadHeader.dataSetWriterIds = writerId;
    nm->groupHeader.writerGroupId = dataSetReader->config.writerGroupId;
    nm->groupHeader.networkMessageNumber = 1; /* number of the NetworkMessage inside a PublishingInterval */
    nm->payload.dataSetPayload.sizes = dsmLengths;
    nm->payload.dataSetPayload.dataSetMessages = dsm;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
publisherIdIsMatching(UA_NetworkMessage *msg, UA_Variant publisherId) {
    switch(msg->publisherIdType) {
        case UA_PUBLISHERIDTYPE_BYTE:
            return (publisherId.type == &UA_TYPES[UA_TYPES_BYTE] &&
               msg->publisherId.byte == *(UA_Byte*)publisherId.data);
        case UA_PUBLISHERIDTYPE_UINT16:
            return (publisherId.type == &UA_TYPES[UA_TYPES_UINT16] &&
               msg->publisherId.uint16 == *(UA_UInt16*)publisherId.data);
        case UA_PUBLISHERIDTYPE_UINT32:
            return (publisherId.type == &UA_TYPES[UA_TYPES_UINT32] &&
               msg->publisherId.uint32 == *(UA_UInt32*)publisherId.data);
        case UA_PUBLISHERIDTYPE_UINT64:
            return (publisherId.type == &UA_TYPES[UA_TYPES_UINT64] &&
               msg->publisherId.uint64 == *(UA_UInt64*)publisherId.data);
        case UA_PUBLISHERIDTYPE_STRING:
            return (publisherId.type == &UA_TYPES[UA_TYPES_STRING] &&
               UA_String_equal(&msg->publisherId.string, (UA_String*)publisherId.data));
        default:
            return false;
    }
    return true;
}

static UA_StatusCode
checkReaderIdentifier(UA_Server *server, UA_NetworkMessage *msg,
                      UA_DataSetReader *reader, UA_ReaderGroupConfig readerGroupConfig) {


    if(readerGroupConfig.encodingMimeType != UA_PUBSUB_ENCODING_JSON){
        if(!publisherIdIsMatching(msg, reader->config.publisherId)) {
            return UA_STATUSCODE_BADNOTFOUND;
        }
        if(msg->groupHeaderEnabled && msg->groupHeader.writerGroupIdEnabled) {
            if(reader->config.writerGroupId != msg->groupHeader.writerGroupId) {
                UA_LOG_INFO_READER(&server->config.logger, reader,
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
                UA_LOG_INFO_READER(&server->config.logger, reader, "DataSetWriterId doesn't match");
                return UA_STATUSCODE_BADNOTFOUND;
            }
        }
        return UA_STATUSCODE_GOOD;
    } else {
        if(reader->config.publisherId.type != &UA_TYPES[UA_TYPES_UINT32] &&
           msg->publisherId.uint32 != *(UA_UInt32*)reader->config.publisherId.data)
            return UA_STATUSCODE_BADNOTFOUND;

        if(reader->config.dataSetWriterId == *msg->payloadHeader.dataSetPayloadHeader.dataSetWriterIds) {
            UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
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

    /* Search the reader group by the given readerGroupIdentifier */
    UA_ReaderGroup *readerGroup = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(readerGroup == NULL)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!dataSetReaderConfig)
        return UA_STATUSCODE_BADNOTFOUND;

    if(readerGroup->configurationFrozen) {
        UA_LOG_WARNING_READERGROUP(&server->config.logger, readerGroup,
                                   "Add DataSetReader failed, Subscriber configuration is frozen");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Allocate memory for new DataSetReader */
    UA_DataSetReader *newDataSetReader = (UA_DataSetReader *)
        UA_calloc(1, sizeof(UA_DataSetReader));
    if(!newDataSetReader)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    newDataSetReader->componentType = UA_PUBSUB_COMPONENT_DATASETREADER;
    if(readerGroup->state == UA_PUBSUBSTATE_OPERATIONAL || readerGroup->state == UA_PUBSUBSTATE_PREOPERATIONAL) {
        retVal = UA_DataSetReader_setPubSubState(server, newDataSetReader, UA_PUBSUBSTATE_PREOPERATIONAL,
                                                 UA_STATUSCODE_GOOD);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_READERGROUP(&server->config.logger, readerGroup,
                                     "Add DataSetReader failed, setPubSubState failed");
            UA_free(newDataSetReader);
            newDataSetReader = 0;
            return retVal;
        }
    }

    /* Copy the config into the new dataSetReader */
    UA_DataSetReaderConfig_copy(dataSetReaderConfig, &newDataSetReader->config);
    newDataSetReader->linkedReaderGroup = readerGroup->identifier;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    retVal = addDataSetReaderRepresentation(server, newDataSetReader);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READERGROUP(&server->config.logger, readerGroup,
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
        UA_LOG_ERROR_READERGROUP(&server->config.logger, readerGroup,
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
                UA_LOG_ERROR_READER(&server->config.logger, newDataSetReader,
                                    "Not implemented! Currently only SubscribedDataSet as "
                                    "TargetVariables is implemented");
            } else {
                if(subscribedDataSet->config.isConnected) {
                    UA_LOG_ERROR_READER(&server->config.logger, newDataSetReader,
                                        "SubscribedDataSet is already connected");
                } else {
                    UA_LOG_DEBUG_READER(&server->config.logger, newDataSetReader,
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
                    retVal |= connectDataSetReaderToDataSet(server, newDataSetReader->identifier,
                                                            subscribedDataSet->identifier);
#endif
                }
            }
        }
    }

    if(readerIdentifier)
        UA_NodeId_copy(&newDataSetReader->identifier, readerIdentifier);

    return retVal;
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
removeDataSetReader(UA_Server *server, UA_NodeId readerIdentifier) {
    /* Remove datasetreader given by the identifier */
    UA_DataSetReader *dsr = UA_ReaderGroup_findDSRbyId(server, readerIdentifier);
    if(!dsr)
        return UA_STATUSCODE_BADNOTFOUND;

    if(dsr->configurationFrozen) {
        UA_LOG_WARNING_READER(&server->config.logger, dsr,
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
            UA_LOG_ERROR_READER(&server->config.logger, dsr,
                                "Remove DataSetReader failed. Stop message "
                                "receive timeout timer of DataSetReader '%.*s' failed.",
                                (int) dsr->config.name.length, dsr->config.name.data);
        }
    }

    res |= server->config.pubSubConfig.monitoringInterface.
        deleteMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                         UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READER(&server->config.logger, dsr,
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

    /* Remove DataSetReader from group */
    LIST_REMOVE(dsr, listEntry);
    UA_ReaderGroup *rg = UA_ReaderGroup_findRGbyId(server, dsr->linkedReaderGroup);
    if(rg)
        rg->readersCount--;

    /* THe offset buffer is only set when the dsr is frozen
     * UA_NetworkMessageOffsetBuffer_clear(&dsr->bufferedMessage); */

    UA_NodeId_clear(&dsr->identifier);
    UA_NodeId_clear(&dsr->linkedReaderGroup);

    /* Free memory allocated for DataSetReader */
    UA_free(dsr);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeDataSetReader(UA_Server *server, UA_NodeId readerIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = removeDataSetReader(server, readerIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
DataSetReader_updateConfig(UA_Server *server, UA_ReaderGroup *rg, UA_DataSetReader *dsr,
                           const UA_DataSetReaderConfig *config) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(dsr->configurationFrozen) {
        UA_LOG_WARNING_READER(&server->config.logger, dsr,
                              "Update DataSetReader config failed. "
                              "Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(rg->configurationFrozen) {
        UA_LOG_WARNING_READER(&server->config.logger, dsr,
                              "Update DataSetReader config failed. "
                              "Subscriber configuration is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(dsr->config.subscribedDataSetType != UA_PUBSUB_SDS_TARGET) {
        UA_LOG_WARNING_READER(&server->config.logger, dsr,
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
        res = server->config.pubSubConfig.monitoringInterface.
            updateMonitoringInterval(server, dsr->identifier,
                                     UA_PUBSUB_COMPONENT_DATASETREADER,
                                     UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT,
                                     dsr);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR_READER(&server->config.logger, dsr,
                                "Update DataSetReader message receive timeout timer failed.");
        }
    }
#endif /* UA_ENABLE_PUBSUB_MONITORING */
    return res;
}

UA_StatusCode
UA_Server_DataSetReader_updateConfig(UA_Server *server, UA_NodeId dataSetReaderIdentifier,
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
UA_Server_DataSetReader_getConfig(UA_Server *server, UA_NodeId dataSetReaderIdentifier,
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

    retVal = UA_Variant_copy(&src->publisherId, &dst->publisherId);
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
    UA_Variant_clear(&cfg->publisherId);
    UA_DataSetMetaDataType_clear(&cfg->dataSetMetaData);
    UA_ExtensionObject_clear(&cfg->messageSettings);
    UA_ExtensionObject_clear(&cfg->transportSettings);
    if(cfg->subscribedDataSetType == UA_PUBSUB_SDS_TARGET) {
        UA_TargetVariables_clear(&cfg->subscribedDataSet.subscribedDataSetTarget);
    }
}

UA_StatusCode
UA_Server_DataSetReader_getState(UA_Server *server, UA_NodeId dataSetReaderIdentifier,
                                 UA_PubSubState *state) {
    if(!server || !state)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    UA_DataSetReader *dsr = UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    if(dsr) {
        res = UA_STATUSCODE_GOOD;
        *state = dsr->state;
    }
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
UA_DataSetReader_setState_disabled(UA_Server *server, UA_DataSetReader *dsr) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch(dsr->state) {
    case UA_PUBSUBSTATE_DISABLED:
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_PAUSED:
        dsr->state = UA_PUBSUBSTATE_DISABLED;
        return UA_STATUSCODE_GOOD;
    case UA_PUBSUBSTATE_OPERATIONAL:
#ifdef UA_ENABLE_PUBSUB_MONITORING
        /* Stop MessageReceiveTimeout timer */
        if(dsr->msgRcvTimeoutTimerRunning == true) {
            ret = server->config.pubSubConfig.monitoringInterface.
                stopMonitoring(server, dsr->identifier,
                               UA_PUBSUB_COMPONENT_DATASETREADER,
                               UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
            if(ret == UA_STATUSCODE_GOOD) {
                dsr->msgRcvTimeoutTimerRunning = false;
            } else {
                UA_LOG_ERROR_READER(&server->config.logger, dsr,
                                    "Disable ReaderGroup failed. Stop message receive "
                                    "timeout timer of DataSetReader '%.*s' failed.",
                                    (int) dsr->config.name.length, dsr->config.name.data);
            }
        }
#endif /* UA_ENABLE_PUBSUB_MONITORING */
        if(ret == UA_STATUSCODE_GOOD)
            dsr->state = UA_PUBSUBSTATE_DISABLED;
        return ret;
    case UA_PUBSUBSTATE_ERROR:
        break;
    default:
        UA_LOG_WARNING_READER(&server->config.logger, dsr,
                              "Received unknown PubSub state!");
    }
    return UA_STATUSCODE_BADINVALIDARGUMENT;
}

/* State machine methods not part of the open62541 state machine API */
UA_StatusCode
UA_DataSetReader_setPubSubState(UA_Server *server,
                                UA_DataSetReader *dataSetReader,
                                UA_PubSubState state,
                                UA_StatusCode cause) {
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_PubSubState oldState = dataSetReader->state;
    switch(state) {
        case UA_PUBSUBSTATE_DISABLED:
            ret = UA_DataSetReader_setState_disabled(server, dataSetReader);
            break;
        case UA_PUBSUBSTATE_PAUSED:
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
        case UA_PUBSUBSTATE_PREOPERATIONAL:
            dataSetReader->state = UA_PUBSUBSTATE_PREOPERATIONAL;
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            dataSetReader->state = UA_PUBSUBSTATE_OPERATIONAL;
            break;
        case UA_PUBSUBSTATE_ERROR:
            dataSetReader->state = UA_PUBSUBSTATE_ERROR;
            break;
        default:
            UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                                  "Received unknown PubSub state!");
            ret = UA_STATUSCODE_BADINVALIDARGUMENT;
            break;
    }
    if (state != oldState) {
        /* inform application about state change */
        UA_ServerConfig *config = &server->config;
        if(config->pubSubConfig.stateChangeCallback != 0) {
            config->pubSubConfig.
                stateChangeCallback(server, &dataSetReader->identifier, state, cause);
        }
    }
    return ret;
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
        UA_LOG_WARNING_READER(&server->config.logger, dsr,
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
                                              UA_NodeId dataSetReaderIdentifier,
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
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
                UA_snprintf(szTmpName, sizeof(szTmpName), "%.*s", (int)slen,
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
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                        "addVariableNode %s succeeded", szTmpName);
        }
        else {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
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
DataSetReader_processRaw(UA_Server *server, UA_ReaderGroup *rg,
                         UA_DataSetReader *dsr, UA_DataSetMessage* msg) {
    UA_LOG_TRACE_READER(&server->config.logger, dsr, "Received RAW Frame");
    msg->data.keyFrameData.fieldCount = (UA_UInt16)
        dsr->config.dataSetMetaData.fieldsSize;

    size_t offset = 0;
    for(size_t i = 0; i < dsr->config.dataSetMetaData.fieldsSize; i++) {
        /* TODO The datatype reference should be part of the internal
         * pubsub configuration to avoid the time-expensive lookup */
        const UA_DataType *type =
            UA_findDataTypeWithCustom(&dsr->config.dataSetMetaData.fields[i].dataType,
                                      server->config.customDataTypes);
        msg->data.keyFrameData.rawFields.length += type->memSize;
        UA_STACKARRAY(UA_Byte, value, type->memSize);
        UA_StatusCode res =
            UA_decodeBinaryInternal(&msg->data.keyFrameData.rawFields,
                                    &offset, value, type, NULL);
        if(dsr->config.dataSetMetaData.fields[i].maxStringLength != 0) {
            if(type->typeKind == UA_DATATYPEKIND_STRING ||
                type->typeKind == UA_DATATYPEKIND_BYTESTRING) {
                UA_ByteString *bs = (UA_ByteString *) value;
                //check if length < maxStringLength, The types ByteString and String are equal in their base definition
                size_t lengthDifference = dsr->config.dataSetMetaData.fields[i].maxStringLength - bs->length;
                offset += lengthDifference;
            }
        }
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_READER(&server->config.logger, dsr,
                               "Error during Raw-decode KeyFrame field %u: %s",
                               (unsigned)i, UA_StatusCode_name(res));
            return;
        }

        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];

        if(rg->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE) {
            if (tv->beforeWrite) {
                void *pData = (**tv->externalDataValue).value.data;
                (**tv->externalDataValue).value.data = value;   // set raw data as "preview"
                tv->beforeWrite(server,
                                &dsr->identifier,
                                &dsr->linkedReaderGroup,
                                &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariable.targetNodeId,
                                dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariableContext,
                                tv->externalDataValue);
                (**tv->externalDataValue).value.data = pData;  // restore previous data pointer
            }
            memcpy((**tv->externalDataValue).value.data, value, type->memSize);
            if(tv->afterWrite)
                tv->afterWrite(server, &dsr->identifier,
                                &dsr->linkedReaderGroup,
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
        UA_Variant_setScalar(&writeVal.value.value, value, type);
        writeVal.value.hasValue = true;
        Operation_Write(server, &server->adminSession, NULL, &writeVal, &res);
        UA_clear(value, type);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_INFO_READER(&server->config.logger, dsr,
                               "Error writing KeyFrame field %u: %s",
                               (unsigned)i, UA_StatusCode_name(res));
        }
    }
}

static void
DataSetReader_processFixedSize(UA_Server *server, UA_ReaderGroup *rg,
                               UA_DataSetReader *dsr, UA_DataSetMessage *msg,
                               size_t fieldCount) {
    for(size_t i = 0; i < fieldCount; i++) {
        if(!msg->data.keyFrameData.dataSetFields[i].hasValue)
            continue;

        UA_FieldTargetVariable *tv =
            &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i];
        if(tv->targetVariable.attributeId != UA_ATTRIBUTEID_VALUE)
            continue;
        if (tv->beforeWrite) {
            UA_DataValue *tmp = &msg->data.keyFrameData.dataSetFields[i];
            tv->beforeWrite(server,
                      &dsr->identifier,
                      &dsr->linkedReaderGroup,
                      &dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariable.targetNodeId,
                      dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[i].targetVariableContext,
                      &tmp);
        }
        memcpy((**tv->externalDataValue).value.data,
                    msg->data.keyFrameData.dataSetFields[i].value.data,
                    msg->data.keyFrameData.dataSetFields[i].value.type->memSize);
        if(tv->afterWrite)
            tv->afterWrite(server, &dsr->identifier, &dsr->linkedReaderGroup,
                        &tv->targetVariable.targetNodeId,
                        tv->targetVariableContext, tv->externalDataValue);
    }
}

void
UA_DataSetReader_process(UA_Server *server, UA_ReaderGroup *rg,
                         UA_DataSetReader *dsr, UA_DataSetMessage *msg) {
    if(!dsr || !rg || !msg || !server)
        return;

    /* Check the metadata, to see if this reader is configured for a heartbeat */
    if(dsr->config.dataSetMetaData.fieldsSize == 0 &&
       dsr->config.dataSetMetaData.configurationVersion.majorVersion == 0 &&
       dsr->config.dataSetMetaData.configurationVersion.minorVersion == 0) {
        /* Expecting a heartbeat, check the message */
        if(msg->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME ||
            msg->header.configVersionMajorVersion != 0 ||
            msg->header.configVersionMinorVersion != 0 ||
            msg->data.keyFrameData.fieldCount != 0) {
            UA_LOG_INFO_READER(&server->config.logger, dsr,
                               "This DSR expects heartbeat, but the received "
                               "message doesn't seem to be so.");
        }
#ifdef UA_ENABLE_PUBSUB_MONITORING
        UA_DataSetReader_checkMessageReceiveTimeout(server, dsr);
#endif
        dsr->lastHeartbeatReceived = UA_DateTime_nowMonotonic();
        return;
    }

    UA_LOG_DEBUG_READER(&server->config.logger, dsr,
                        "DataSetReader '%.*s': received a network message",
                        (int)dsr->config.name.length, dsr->config.name.data);

    if(!msg->header.dataSetMessageValid) {
        UA_LOG_INFO_READER(&server->config.logger, dsr,
                           "DataSetMessage is discarded: message is not valid");
        /* To Do check ConfigurationVersion */
        /* if(msg->header.configVersionMajorVersionEnabled) {
         *     if(msg->header.configVersionMajorVersion !=
         *            dsr->config.dataSetMetaData.configurationVersion.majorVersion) {
         *         UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER,
         *                        "DataSetMessage is discarded: ConfigurationVersion "
         *                        "MajorVersion does not match");
         *         return;
         *     }
         * } */
        return;
    }

    /* Message is valid, if DSR is preoperational, we can change it to operational*/
    if(dsr->state == UA_PUBSUBSTATE_PREOPERATIONAL){
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_OPERATIONAL, UA_STATUSCODE_GOOD);
    }

    if(msg->header.dataSetMessageType != UA_DATASETMESSAGE_DATAKEYFRAME) {
        UA_LOG_WARNING_READER(&server->config.logger, dsr,
                       "DataSetMessage is discarded: Only keyframes are supported");
        return;
    }

    /* Process message with raw encoding (realtime and non-realtime) */
    if(msg->header.fieldEncoding == UA_FIELDENCODING_RAWDATA) {
        DataSetReader_processRaw(server, rg, dsr, msg);
#ifdef UA_ENABLE_PUBSUB_MONITORING
        UA_DataSetReader_checkMessageReceiveTimeout(server, dsr);
#endif
        return;
    }

    /* Check and adjust the field count
     * TODO Throw an error if non-matching? */
    size_t fieldCount = msg->data.keyFrameData.fieldCount;
    if(dsr->config.dataSetMetaData.fieldsSize < fieldCount)
        fieldCount = dsr->config.dataSetMetaData.fieldsSize;

    if(dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize < fieldCount)
        fieldCount = dsr->config.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize;

    /* Process message with fixed size fields (realtime capable) */
    if(rg->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE) {
        DataSetReader_processFixedSize(server, rg, dsr, msg, fieldCount);
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
            UA_LOG_INFO_READER(&server->config.logger, dsr,
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
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_OPERATIONAL,
                                        UA_STATUSCODE_GOOD);
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
            UA_LOG_ERROR_READER(&server->config.logger, dsr,
                                "DataSetReader '%.*s': stop receive timeout timer failed",
                                (int)dsr->config.name.length, dsr->config.name.data);
            UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR,
                                            UA_STATUSCODE_BADINTERNALERROR);
        }
    }

    /* Start message receive timeout timer */
    res = server->config.pubSubConfig.monitoringInterface.
        startMonitoring(server, dsr->identifier, UA_PUBSUB_COMPONENT_DATASETREADER,
                        UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT, dsr);
    if(res == UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG_READER(&server->config.logger, dsr,
                            "Info: DataSetReader '%.*s': start receive timeout timer",
                            (int)dsr->config.name.length, dsr->config.name.data);
        dsr->msgRcvTimeoutTimerRunning = true;
    } else {
        UA_LOG_ERROR_READER(&server->config.logger, dsr,
                            "Starting Message Receive Timeout timer failed.");
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR,
                                        UA_STATUSCODE_BADINTERNALERROR);
    }
}

/* Timeout callback for DataSetReader MessageReceiveTimeout handling */
static void
UA_DataSetReader_handleMessageReceiveTimeout(UA_Server *server, UA_DataSetReader *dsr) {
    UA_assert(server);
    UA_assert(dsr);

    if(dsr->componentType != UA_PUBSUB_COMPONENT_DATASETREADER) {
        UA_LOG_ERROR_READER(&server->config.logger, dsr,
                            "UA_DataSetReader_handleMessageReceiveTimeout(): "
                            "input param is not of type DataSetReader");
        return;
    }

    UA_LOG_DEBUG_READER(&server->config.logger, dsr,
                        "UA_DataSetReader_handleMessageReceiveTimeout(): "
                        "MessageReceiveTimeout occurred at DataSetReader "
                        "'%.*s': MessageReceiveTimeout = %f Timer Id = %u ",
                        (int)dsr->config.name.length, dsr->config.name.data,
                        dsr->config.messageReceiveTimeout,
                        (UA_UInt32) dsr->msgRcvTimeoutTimerId);

    UA_StatusCode res =
        UA_DataSetReader_setPubSubState(server, dsr, UA_PUBSUBSTATE_ERROR,
                                        UA_STATUSCODE_BADTIMEOUT);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_READER(&server->config.logger, dsr,
                            "UA_DataSetReader_handleMessageReceiveTimeout(): "
                            "setting pubsub state failed");
    }
}
#endif /* UA_ENABLE_PUBSUB_MONITORING */

static void
processMessageWithReader(UA_Server *server, UA_ReaderGroup *readerGroup,
                         UA_DataSetReader *reader, UA_NetworkMessage *msg) {
    UA_Byte totalDataSets = 1;
    if(msg->payloadHeaderEnabled)
        totalDataSets = msg->payloadHeader.dataSetPayloadHeader.count;
    for(UA_Byte i = 0; i < totalDataSets; i++) {
        /* map dataset reader to dataset message since multiple dataset reader may read this network message
           otherwise the dataset message may be written to the wrong dataset reader.  */
        if (!msg->payloadHeaderEnabled ||
            (reader->config.dataSetWriterId == msg->payloadHeader.dataSetPayloadHeader.dataSetWriterIds[i])) {
            UA_LOG_DEBUG_READER(&server->config.logger, reader,
                            "Process Msg with DataSetReader!");
            UA_DataSetReader_process(server, readerGroup, reader,
                                 &msg->payload.dataSetPayload.dataSetMessages[i]);
        }
    }
}

/* Process Network Message for a ReaderGroup. But we the ReaderGroup needs to be
 * identified first. */
static UA_StatusCode
processNetworkMessage(UA_Server *server, UA_PubSubConnection *connection,
                      UA_NetworkMessage* msg) {
    if(!msg || !connection)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* To Do The condition pMsg->dataSetClassIdEnabled
     * Here some filtering is possible */

    if(!msg->publisherIdEnabled) {
        UA_LOG_INFO_CONNECTION(&server->config.logger, connection,
                               "Cannot process DataSetReader without PublisherId");
        return UA_STATUSCODE_BADNOTIMPLEMENTED; /* TODO: Handle DSR without PublisherId */
    }

    /* There can be several readers listening for the same network message */
    UA_Boolean processed = false;
    UA_ReaderGroup *readerGroup;
    UA_DataSetReader *reader;
    LIST_FOREACH(readerGroup, &connection->readerGroups, listEntry) {
        LIST_FOREACH(reader, &readerGroup->readers, listEntry) {
            UA_StatusCode retval = checkReaderIdentifier(server, msg, reader, readerGroup->config);
            if(retval == UA_STATUSCODE_GOOD) {
                processed = true;
                processMessageWithReader(server, readerGroup, reader, msg);
            }
        }
    }

    if(!processed) {
        UA_LOG_INFO_CONNECTION(&server->config.logger, connection,
                               "Dataset reader not found. Check PublisherID, "
                               "WriterGroupID and DatasetWriterID");
    }

    return UA_STATUSCODE_GOOD;
}

/********************************************************************************
 * Functionality related to decoding, decrypting and processing network messages
 * as a subscriber
 ********************************************************************************/

#define MIN_PAYLOAD_SIZE_ETHERNET 46

/* Delete the payload value of every decoded DataSet field */
static void UA_DataSetMessage_freeDecodedPayload(UA_DataSetMessage *dsm) {
    if(dsm->header.fieldEncoding == UA_FIELDENCODING_VARIANT) {
        for(size_t i = 0; i < dsm->data.keyFrameData.fieldCount; i++) {
#ifdef UA_ENABLE_PUBSUB_BUFMALLOC
            UA_Variant_init(&dsm->data.keyFrameData.dataSetFields[i].value);
#else
            UA_Variant_clear(&dsm->data.keyFrameData.dataSetFields[i].value);
#endif
        }
    }
    else if(dsm->header.fieldEncoding == UA_FIELDENCODING_DATAVALUE) {
        for(size_t i = 0; i < dsm->data.keyFrameData.fieldCount; i++) {
#ifdef UA_ENABLE_PUBSUB_BUFMALLOC
            UA_DataValue_init(&dsm->data.keyFrameData.dataSetFields[i]);
#else
            UA_DataValue_clear(&dsm->data.keyFrameData.dataSetFields[i]);
#endif
        }
    }
}

UA_StatusCode
decodeNetworkMessage(UA_Server *server, UA_ByteString *buffer, size_t *pos,
                     UA_NetworkMessage *nm, UA_PubSubConnection *connection) {
#ifdef UA_DEBUG_DUMP_PKGS
    UA_dump_hex_pkg(buffer->data, buffer->length);
#endif

    UA_StatusCode rv = UA_NetworkMessage_decodeHeaders(buffer, pos, nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                  "PubSub receive. decoding headers failed");
        return rv;
    }

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_Boolean processed = false;
    UA_ReaderGroup *readerGroup;
    UA_DataSetReader *reader;

    /* Choose a correct readergroup for decrypt/verify this message
     * (there could be multiple) */
    LIST_FOREACH(readerGroup, &connection->readerGroups, listEntry) {
        LIST_FOREACH(reader, &readerGroup->readers, listEntry) {
            UA_StatusCode retval = checkReaderIdentifier(server, nm, reader, readerGroup->config);
            if(retval == UA_STATUSCODE_GOOD) {
                processed = true;
                rv = verifyAndDecryptNetworkMessage(&server->config.logger, buffer, pos,
                                                    nm, readerGroup);
                if(rv != UA_STATUSCODE_GOOD) {
                    UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                              "Subscribe failed, verify and decrypt "
                                              "network message failed.");
                    return rv;
                }

#ifdef UA_DEBUG_DUMP_PKGS
                UA_dump_hex_pkg(buffer->data, buffer->length);
#endif
                /* break out of all loops when first verify & decrypt was successful */
                goto loops_exit;
            }
        }
    }

loops_exit:
    if(!processed) {
        UA_LOG_INFO_CONNECTION(&server->config.logger, connection,
                               "Dataset reader not found. Check PublisherId, "
                               "WriterGroupId and DatasetWriterId");
        /* Possible multicast scenario: there are multiple connections (with one
         * or more ReaderGroups) within a multicast group every connection
         * receives all network messages, even if some of them are not meant for
         * the connection currently processed -> therefore it is ok if the
         * connection does not have a DataSetReader for every received network
         * message. We must not return an error here, but continue with the
         * buffer decoding and see if we have a matching DataSetReader for the
         * next network message. */
    }
#endif

    rv = UA_NetworkMessage_decodePayload(buffer, pos, nm);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_decodeFooters(buffer, pos, nm);
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_JSON_ENCODING
UA_StatusCode
decodeNetworkMessageJson(UA_Server *server, UA_ByteString *buffer, size_t *pos,
                     UA_NetworkMessage *nm, UA_PubSubConnection *connection) {

    UA_StatusCode rv = UA_NetworkMessage_decodeJson(nm, buffer);
    UA_CHECK_STATUS(rv, return rv);

    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
decodeAndProcessNetworkMessage(UA_Server *server, UA_PubSubConnection *connection,
                                  UA_PubSubEncodingType encodingMimeType, UA_ByteString *buf) {
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));

    UA_ReaderGroup *readerGroup = LIST_FIRST(&connection->readerGroups);
    if(!readerGroup) {
        UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                  "Received message, but no reader configured");
        return UA_STATUSCODE_GOOD;
    }

    size_t currentPosition = 0;
    UA_StatusCode rv;
    if(encodingMimeType == UA_PUBSUB_ENCODING_UADP){
        rv = decodeNetworkMessage(server, buf, &currentPosition,
                                  &nm, connection);
    } else { /* if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) */
#ifdef UA_ENABLE_JSON_ENCODING
        rv = decodeNetworkMessageJson(server, buf, &currentPosition,
                                  &nm, connection);
#else
        rv = UA_STATUSCODE_BADNOTSUPPORTED;
#endif
    }
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                  "Verify, decrypt and decode network message failed.");
        goto cleanup;
    }

    rv = processNetworkMessage(server, connection, &nm);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                                  "Processing the network message failed.");
    }

cleanup:
    UA_NetworkMessage_clear(&nm);
    return rv;
}

UA_StatusCode
UA_decodeAndProcessNetworkMessage(UA_Server *server,
                                  UA_PubSubConnection *connection,
                                  UA_ByteString *buffer) {
    return decodeAndProcessNetworkMessage(server, connection, UA_PUBSUB_ENCODING_UADP, buffer);
}

static UA_StatusCode
decodeAndProcessNetworkMessageRT(UA_Server *server, UA_ReaderGroup *readerGroup,
                                 UA_PubSubConnection *connection, UA_ByteString *buf) {
#ifdef UA_ENABLE_PUBSUB_BUFMALLOC
    useMembufAlloc();
#endif

    /* Considering max DSM as 1
    * TODO: Process with the static value source */
    UA_StatusCode rv = UA_STATUSCODE_GOOD;
    size_t currentPosition = 0;
    UA_DataSetReader *dataSetReader = LIST_FIRST(&readerGroup->readers);
    UA_NetworkMessage *nm = dataSetReader->bufferedMessage.nm;

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_NetworkMessage currentNetworkMessage;
    memset(&currentNetworkMessage, 0, sizeof(UA_NetworkMessage));

    size_t payLoadPosition = 0;
    rv = UA_NetworkMessage_decodeHeaders(buf, &payLoadPosition, &currentNetworkMessage);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                           "PubSub receive. decoding headers failed");
        return rv;
    }

    rv = verifyAndDecryptNetworkMessage(&server->config.logger, buf, &payLoadPosition,
                                        &currentNetworkMessage, readerGroup);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_READER(&server->config.logger, dataSetReader,
                              "Subscribe failed. verify and decrypt network message failed.");
        return rv;
    }
    UA_NetworkMessage_clear(&currentNetworkMessage);
#endif

    /* Decode only the necessary offset and update the networkMessage */
    rv = UA_NetworkMessage_updateBufferedNwMessage(&dataSetReader->bufferedMessage,
                                                   buf, &currentPosition);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_READER(&server->config.logger, dataSetReader,
                           "PubSub receive. Unknown field type.");
        rv = UA_STATUSCODE_UNCERTAIN;
        goto cleanup;
    }

    /* Check the decoded message is the expected one */
    rv = checkReaderIdentifier(server, nm, dataSetReader, readerGroup->config);
    if(rv != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO_READER(&server->config.logger, dataSetReader,
                           "PubSub receive. Unknown message received. Will not be processed.");
        rv = UA_STATUSCODE_UNCERTAIN;
        goto cleanup;
    }

    UA_DataSetReader_process(server, readerGroup, dataSetReader,
                             nm->payload.dataSetPayload.dataSetMessages);

 cleanup:
    UA_DataSetMessage_freeDecodedPayload(nm->payload.dataSetPayload.dataSetMessages);
#ifdef UA_ENABLE_PUBSUB_BUFMALLOC
    useNormalAlloc();
#endif
    return rv;
}

typedef struct {
    UA_Server *server;
    UA_PubSubConnection *connection;
} UA_RGContext;

static UA_StatusCode
decodeAndProcessFun(UA_PubSubChannel *channel, void *cbContext,
                    const UA_ByteString *buf) {
    UA_RGContext *ctx = (UA_RGContext*)cbContext;
    UA_ByteString mutableBuffer = {buf->length, buf->data};

    /* If there is no ReaderGroup attached -> nothing to do.
     * Otherwise assume they all have the same encoding mime-type. */
    UA_ReaderGroup *rg = LIST_FIRST(&ctx->connection->readerGroups);
    if(!rg)
        return UA_STATUSCODE_BADNOTHINGTODO;

    return decodeAndProcessNetworkMessage(ctx->server, ctx->connection,
                                          rg->config.encodingMimeType,
                                          &mutableBuffer);
}

static UA_StatusCode
decodeAndProcessFunRT(UA_PubSubChannel *channel, void *cbContext,
                      const UA_ByteString *buf) {
    UA_RGContext *ctx = (UA_RGContext*)cbContext;
    UA_ByteString mutableBuffer = {buf->length, buf->data};

    /* Process for all ReaderGroups that are operational and linked to the connection */
    UA_ReaderGroup *rg;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    LIST_FOREACH(rg, &ctx->connection->readerGroups, listEntry) {
        res |= decodeAndProcessNetworkMessageRT(ctx->server, rg, ctx->connection,
                                                &mutableBuffer);
    }
    return res;
}

void processMqttSubscriberCallback(UA_Server *server, UA_PubSubConnection *connection,
                                   UA_ByteString *msg) {
    UA_RGContext ctx = {server, connection};
    decodeAndProcessFun(connection->channel, &ctx, msg);
}

UA_StatusCode
receiveBufferedNetworkMessage(UA_Server *server, UA_ReaderGroup *readerGroup,
                              UA_PubSubConnection *connection) {
    UA_RGContext ctx = {server, connection};
    UA_PubSubReceiveCallback receiveCB;
    if(readerGroup->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE)
        receiveCB = decodeAndProcessFunRT;
    else
        receiveCB = decodeAndProcessFun;

    /* TODO: Move the TransportSettings to to the readerGroupConfig. So we can
     * use it here instead of a NULL pointer. */
    UA_StatusCode rv =
        connection->channel->receive(connection->channel, &readerGroup->config.transportSettings,
                                     receiveCB, &ctx,
                                     readerGroup->config.timeout);

    // TODO attention: here rv is ok if UA_STATUSCODE_GOOD != rv
    if(UA_StatusCode_isBad(rv)) {
        UA_LOG_WARNING_CONNECTION(&server->config.logger, connection,
                              "SubscribeCallback(): Connection receive failed!");
        return rv;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StandaloneSubscribedDataSet *
UA_StandaloneSubscribedDataSet_findSDSbyId(UA_Server *server, UA_NodeId identifier) {
    UA_StandaloneSubscribedDataSet *subscribedDataSet;
    TAILQ_FOREACH(subscribedDataSet, &server->pubSubManager.subscribedDataSets,
                  listEntry) {
        if(UA_NodeId_equal(&identifier, &subscribedDataSet->identifier))
            return subscribedDataSet;
    }
    return NULL;
}

UA_StandaloneSubscribedDataSet *
UA_StandaloneSubscribedDataSet_findSDSbyName(UA_Server *server, UA_String identifier) {
    UA_StandaloneSubscribedDataSet *subscribedDataSet;
    TAILQ_FOREACH(subscribedDataSet, &server->pubSubManager.subscribedDataSets,
                  listEntry) {
        if(UA_String_equal(&identifier, &subscribedDataSet->config.name))
            return subscribedDataSet;
    }
    return NULL;
}

UA_StatusCode
UA_StandaloneSubscribedDataSetConfig_copy(const UA_StandaloneSubscribedDataSetConfig *src,
                                          UA_StandaloneSubscribedDataSetConfig *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_StandaloneSubscribedDataSetConfig));
    res = UA_DataSetMetaDataType_copy(&src->dataSetMetaData, &dst->dataSetMetaData);
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_Boolean_copy(&src->isConnected, &dst->isConnected);
    res |= UA_TargetVariablesDataType_copy(&src->subscribedDataSet.target,
                                           &dst->subscribedDataSet.target);

    if(res != UA_STATUSCODE_GOOD)
        UA_StandaloneSubscribedDataSetConfig_clear(dst);
    return res;
}

void
UA_StandaloneSubscribedDataSetConfig_clear(
    UA_StandaloneSubscribedDataSetConfig *sdsConfig) {
    // delete sds config
    UA_String_clear(&sdsConfig->name);
    UA_DataSetMetaDataType_clear(&sdsConfig->dataSetMetaData);
    UA_TargetVariablesDataType_clear(&sdsConfig->subscribedDataSet.target);
}

void
UA_StandaloneSubscribedDataSet_clear(UA_Server *server,
                                     UA_StandaloneSubscribedDataSet *subscribedDataSet) {
    UA_StandaloneSubscribedDataSetConfig_clear(&subscribedDataSet->config);
    UA_NodeId_clear(&subscribedDataSet->identifier);
    UA_NodeId_clear(&subscribedDataSet->connectedReader);
}

#endif /* UA_ENABLE_PUBSUB */
