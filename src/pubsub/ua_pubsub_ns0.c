/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2022 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019-2021 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020-2022 Thomas Fischer, Siemens AG
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 * Copyright (c) 2025 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB_SKS
#include "ua_pubsub_keystorage.h"
#endif

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL /* conditional compilation */

typedef struct {
    UA_NodeId parentNodeId;
    UA_UInt32 parentClassifier;
    UA_UInt32 elementClassiefier;
} UA_NodePropertyContext;

static UA_StatusCode
writePubSubNs0VariableArray(UA_Server *server, const UA_NodeId id, void *v,
                            size_t length, const UA_DataType *type) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_Variant var;
    UA_Variant_init(&var);
    UA_Variant_setArray(&var, v, length, type);
    return writeValueAttribute(server, id, &var);
}

static UA_NodeId
findSingleChildNode(UA_Server *server, UA_QualifiedName targetName,
                    UA_NodeId referenceTypeId, UA_NodeId startingNode){
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_NodeId resultNodeId;
    UA_RelativePathElement rpe;
    UA_RelativePathElement_init(&rpe);
    rpe.referenceTypeId = referenceTypeId;
    rpe.isInverse = false;
    rpe.includeSubtypes = false;
    rpe.targetName = targetName;
    UA_BrowsePath bp;
    UA_BrowsePath_init(&bp);
    bp.startingNode = startingNode;
    bp.relativePath.elementsSize = 1;
    bp.relativePath.elements = &rpe;
    UA_BrowsePathResult bpr = translateBrowsePathToNodeIds(server, &bp);
    if(bpr.statusCode != UA_STATUSCODE_GOOD ||
       bpr.targetsSize < 1)
        return UA_NODEID_NULL;
    UA_StatusCode res = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, &resultNodeId);
    if(res != UA_STATUSCODE_GOOD){
        UA_BrowsePathResult_clear(&bpr);
        return UA_NODEID_NULL;
    }
    UA_BrowsePathResult_clear(&bpr);
    return resultNodeId;
}

static void
onRead(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
       const UA_NodeId *nodeid, void *context,
       const UA_NumericRange *range, const UA_DataValue *data) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return;

    const UA_NodePropertyContext *nodeContext = (const UA_NodePropertyContext*)context;
    const UA_NodeId *myNodeId = &nodeContext->parentNodeId;

    UA_PublishedVariableDataType *pvd = NULL;
    UA_PublishedDataSet *publishedDataSet = NULL;

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Boolean isConnected;

    switch(nodeContext->parentClassifier) {
    case UA_NS0ID_PUBSUBCONNECTIONTYPE: {
        UA_PubSubConnection *pubSubConnection = UA_PubSubConnection_find(psm, *myNodeId);
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_PUBSUBCONNECTIONTYPE_PUBLISHERID:
            UA_PublisherId_toVariant(&pubSubConnection->config.publisherId, &value);
            break;
        default:
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_READERGROUPTYPE: {
        UA_ReaderGroup *readerGroup = UA_ReaderGroup_find(psm, *myNodeId);
        if(!readerGroup)
            return;
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_PUBSUBGROUPTYPE_STATUS_STATE:
            UA_Variant_setScalar(&value, &readerGroup->head.state,
                                 &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
            break;
        default:
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_DATASETREADERTYPE: {
        UA_DataSetReader *dataSetReader = UA_DataSetReader_find(psm, *myNodeId);
        if(!dataSetReader)
            return;

        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_DATASETREADERTYPE_PUBLISHERID:
            UA_PublisherId_toVariant(&dataSetReader->config.publisherId, &value);
            break;
        case UA_NS0ID_DATASETREADERTYPE_STATUS_STATE:
            UA_Variant_setScalar(&value, &dataSetReader->head.state,
                                 &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
            break;
        default:
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_WRITERGROUPTYPE: {
        UA_WriterGroup *writerGroup = UA_WriterGroup_find(psm, *myNodeId);
        if(!writerGroup)
            return;
        switch(nodeContext->elementClassiefier){
        case UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL:
            UA_Variant_setScalar(&value, &writerGroup->config.publishingInterval,
                                 &UA_TYPES[UA_TYPES_DURATION]);
            break;
        case UA_NS0ID_PUBSUBGROUPTYPE_STATUS_STATE:
            UA_Variant_setScalar(&value, &writerGroup->head.state,
                                 &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
            break;
        default:
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_DATASETWRITERTYPE: {
        UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_find(psm, *myNodeId);
        if(!dataSetWriter)
            return;

        switch(nodeContext->elementClassiefier) {
            case UA_NS0ID_DATASETWRITERTYPE_DATASETWRITERID:
                UA_Variant_setScalar(&value, &dataSetWriter->config.dataSetWriterId,
                                     &UA_TYPES[UA_TYPES_UINT16]);
                break;
            case UA_NS0ID_DATASETWRITERTYPE_STATUS_STATE:
                UA_Variant_setScalar(&value, &dataSetWriter->head.state,
                                     &UA_TYPES[UA_TYPES_PUBSUBSTATE]);
                break;
            default:
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                               "Read error! Unknown property.");
        }
        break;
    }
    case UA_NS0ID_PUBLISHEDDATAITEMSTYPE: {
        publishedDataSet = UA_PublishedDataSet_find(psm, *myNodeId);
        if(!publishedDataSet)
            return;
        switch(nodeContext->elementClassiefier) {
        case UA_NS0ID_PUBLISHEDDATAITEMSTYPE_PUBLISHEDDATA: {
            pvd = (UA_PublishedVariableDataType *)
                UA_calloc(publishedDataSet->fieldSize, sizeof(UA_PublishedVariableDataType));
            size_t counter = 0;
            UA_DataSetField *field;
            TAILQ_FOREACH(field, &publishedDataSet->fields, listEntry) {
                pvd[counter].attributeId = UA_ATTRIBUTEID_VALUE;
                pvd[counter].publishedVariable =
                    field->config.field.variable.publishParameters.publishedVariable;
                UA_NodeId_copy(&field->config.field.variable.publishParameters.publishedVariable,
                               &pvd[counter].publishedVariable);
                counter++;
            }
            UA_Variant_setArray(&value, pvd, publishedDataSet->fieldSize,
                                &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
            break;
        }
        case UA_NS0ID_PUBLISHEDDATASETTYPE_DATASETMETADATA: {
            UA_Variant_setScalar(&value, &publishedDataSet->dataSetMetaData,
                                 &UA_TYPES[UA_TYPES_DATASETMETADATATYPE]);
            break;
        }
        case UA_NS0ID_PUBLISHEDDATASETTYPE_CONFIGURATIONVERSION: {
            UA_Variant_setScalar(&value, &publishedDataSet->dataSetMetaData.configurationVersion,
                                     &UA_TYPES[UA_TYPES_CONFIGURATIONVERSIONDATATYPE]);
            break;
        }
        default:
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown property.");
        }
        break;
    }    
    case UA_NS0ID_STANDALONESUBSCRIBEDDATASETREFDATATYPE: {
        UA_SubscribedDataSet *sds = UA_SubscribedDataSet_find(psm, *myNodeId);
        switch(nodeContext->elementClassiefier) {
            case UA_NS0ID_STANDALONESUBSCRIBEDDATASETTYPE_ISCONNECTED: {
                isConnected = (sds->connectedReader != NULL);
                UA_Variant_setScalar(&value, &isConnected,
                                     &UA_TYPES[UA_TYPES_BOOLEAN]);
                break;
            }
            case UA_NS0ID_STANDALONESUBSCRIBEDDATASETTYPE_DATASETMETADATA: {
                UA_Variant_setScalar(&value, &sds->config.dataSetMetaData,
                                     &UA_TYPES[UA_TYPES_DATASETMETADATATYPE]);
                break;
            }
            default:
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                            "Read error! Unknown property.");
        }
        break;
    }
    default:
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Read error! Unknown parent element.");
    }

    writeValueAttribute(server, *nodeid, &value);
    if(pvd && publishedDataSet) {
        UA_Array_delete(pvd, publishedDataSet->fieldSize,
                        &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
    }
}

static void
onWrite(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
        const UA_NodeId *nodeId, void *nodeContext,
        const UA_NumericRange *range, const UA_DataValue *data) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodePropertyContext *npc = (UA_NodePropertyContext *)nodeContext;

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return;
    UA_WriterGroup *writerGroup = NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(npc->parentClassifier) {
        case UA_NS0ID_PUBSUBCONNECTIONTYPE:
            //no runtime writable attributes
            break;
        case UA_NS0ID_WRITERGROUPTYPE: {
            writerGroup = UA_WriterGroup_find(psm, npc->parentNodeId);
            if(!writerGroup) {
                res = UA_STATUSCODE_BADNOTFOUND;
                break;
            }
            switch(npc->elementClassiefier) {
                case UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL:
                    if(!UA_Variant_hasScalarType(&data->value, &UA_TYPES[UA_TYPES_DURATION]) &&
                       !UA_Variant_hasScalarType(&data->value, &UA_TYPES[UA_TYPES_DOUBLE])) {
                        res = UA_STATUSCODE_BADTYPEMISMATCH;
                        break;
                    }
                    UA_Duration interval = *((UA_Duration *) data->value.data);
                    if(interval <= 0.0) {
                        res = UA_STATUSCODE_BADOUTOFRANGE;
                        break;
                    }
                    writerGroup->config.publishingInterval = interval;
                    if(writerGroup->head.state == UA_PUBSUBSTATE_OPERATIONAL) {
                        UA_WriterGroup_removePublishCallback(psm, writerGroup);
                        UA_WriterGroup_addPublishCallback(psm, writerGroup);
                    }
                    break;
                default:
                    UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                                   "Write error! Unknown property element.");
            }
            break;
        }
        default:
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Read error! Unknown parent element.");
    }

    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Changing the ReaderGroupConfig failed with status %s",
                       UA_StatusCode_name(res));
    }
}

static UA_StatusCode
addVariableValueSource(UA_Server *server, UA_ValueCallback valueCallback,
                       UA_NodeId node, UA_NodePropertyContext *context){
    UA_LOCK_ASSERT(&server->serviceMutex);
    setNodeContext(server, node, context);
    return setVariableNode_valueCallback(server, node, valueCallback);
}

static UA_StatusCode
addPubSubConnectionConfig(UA_Server *server, UA_PubSubConnectionDataType *pubsubConnection,
                          UA_NodeId *connectionId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NetworkAddressUrlDataType networkAddressUrl;
    memset(&networkAddressUrl, 0, sizeof(networkAddressUrl));
    UA_ExtensionObject *eo = &pubsubConnection->address;
    if(eo->encoding == UA_EXTENSIONOBJECT_DECODED &&
       eo->content.decoded.type == &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]) {
        void *data = eo->content.decoded.data;
        retVal =
            UA_NetworkAddressUrlDataType_copy((UA_NetworkAddressUrlDataType *)data,
                                              &networkAddressUrl);
        if(retVal != UA_STATUSCODE_GOOD)
            return retVal;
    }

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(UA_PubSubConnectionConfig));
    connectionConfig.transportProfileUri = pubsubConnection->transportProfileUri;
    connectionConfig.name = pubsubConnection->name;
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl,
                         &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    retVal |= UA_PublisherId_fromVariant(&connectionConfig.publisherId,
                                         &pubsubConnection->publisherId);
    retVal |= UA_PubSubConnection_create(psm, &connectionConfig, connectionId);
    UA_NetworkAddressUrlDataType_clear(&networkAddressUrl);
    return retVal;
}

/**
 * **WriterGroup handling**
 *
 * The WriterGroup (WG) is part of the connection and contains the primary
 * configuration parameters for the message creation. */
static UA_StatusCode
addWriterGroupConfig(UA_Server *server, UA_NodeId connectionId,
                     UA_WriterGroupDataType *writerGroup, UA_NodeId *writerGroupId){
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    /* Now we create a new WriterGroupConfig and add the group to the existing
     * PubSubConnection. */
    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = writerGroup->name;
    writerGroupConfig.publishingInterval = writerGroup->publishingInterval;
    writerGroupConfig.writerGroupId = writerGroup->writerGroupId;
    writerGroupConfig.priority = writerGroup->priority;

    UA_ExtensionObject *eoWG = &writerGroup->messageSettings;
    UA_UadpWriterGroupMessageDataType uadpWriterGroupMessage;
    UA_JsonWriterGroupMessageDataType jsonWriterGroupMessage;
    if(eoWG->encoding == UA_EXTENSIONOBJECT_DECODED){
        writerGroupConfig.messageSettings.encoding  = UA_EXTENSIONOBJECT_DECODED;
        if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]){
            writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
            if(UA_UadpWriterGroupMessageDataType_copy(
                    (UA_UadpWriterGroupMessageDataType *)eoWG->content.decoded.data,
                    &uadpWriterGroupMessage) != UA_STATUSCODE_GOOD) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
            writerGroupConfig.messageSettings.content.decoded.data = &uadpWriterGroupMessage;
        } else if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE]) {
            writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
            if(UA_JsonWriterGroupMessageDataType_copy(
                   (UA_JsonWriterGroupMessageDataType *)eoWG->content.decoded.data,
                   &jsonWriterGroupMessage) != UA_STATUSCODE_GOOD) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE];
            writerGroupConfig.messageSettings.content.decoded.data = &jsonWriterGroupMessage;
        }
    }

    eoWG = &writerGroup->transportSettings;
    UA_BrokerWriterGroupTransportDataType brokerWriterGroupTransport;
    UA_DatagramWriterGroupTransportDataType datagramWriterGroupTransport;
    if(eoWG->encoding == UA_EXTENSIONOBJECT_DECODED) {
        writerGroupConfig.transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE]) {
            if(UA_BrokerWriterGroupTransportDataType_copy(
                    (UA_BrokerWriterGroupTransportDataType*)eoWG->content.decoded.data,
                    &brokerWriterGroupTransport) != UA_STATUSCODE_GOOD) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
            writerGroupConfig.transportSettings.content.decoded.data = &brokerWriterGroupTransport;
        } else if(eoWG->content.decoded.type == &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORTDATATYPE]) {
            if(UA_DatagramWriterGroupTransportDataType_copy(
                   (UA_DatagramWriterGroupTransportDataType *)eoWG->content.decoded.data,
                   &datagramWriterGroupTransport) != UA_STATUSCODE_GOOD) {
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            writerGroupConfig.transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_DATAGRAMWRITERGROUPTRANSPORTDATATYPE];
            writerGroupConfig.transportSettings.content.decoded.data = &datagramWriterGroupTransport;
        }
    }
    if (writerGroupConfig.encodingMimeType == UA_PUBSUB_ENCODING_JSON
        && (writerGroupConfig.transportSettings.encoding != UA_EXTENSIONOBJECT_DECODED ||
        writerGroupConfig.transportSettings.content.decoded.type !=
            &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE])) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "JSON encoding is supported only for MQTT transport");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    return UA_WriterGroup_create(psm, connectionId, &writerGroupConfig, writerGroupId);
}

/**
 * **DataSetWriter handling**
 *
 * A DataSetWriter (DSW) is the glue between the WG and the PDS. The DSW is
 * linked to exactly one PDS and contains additional informations for the
 * message generation. */
static UA_StatusCode
addDataSetWriterConfig(UA_Server *server, const UA_NodeId *writerGroupId,
                       UA_DataSetWriterDataType *dataSetWriter,
                       UA_NodeId *dataSetWriterId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId publishedDataSetId = UA_NODEID_NULL;
    UA_PublishedDataSet *tmpPDS;
    TAILQ_FOREACH(tmpPDS, &psm->publishedDataSets, listEntry){
        if(UA_String_equal(&dataSetWriter->dataSetName, &tmpPDS->config.name)) {
            publishedDataSetId = tmpPDS->head.identifier;
            break;
        }
    }

    if(UA_NodeId_isNull(&publishedDataSetId))
        return UA_STATUSCODE_BADPARENTNODEIDINVALID;

    /* We need now a DataSetWriter within the WriterGroup. This means we must
     * create a new DataSetWriterConfig and add call the addWriterGroup function. */
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = dataSetWriter->name;
    dataSetWriterConfig.dataSetWriterId = dataSetWriter->dataSetWriterId;
    dataSetWriterConfig.keyFrameCount = dataSetWriter->keyFrameCount;
    dataSetWriterConfig.dataSetFieldContentMask =  dataSetWriter->dataSetFieldContentMask;
    return UA_DataSetWriter_create(psm, *writerGroupId, publishedDataSetId,
                                   &dataSetWriterConfig, dataSetWriterId);
}

/**
 * **ReaderGroup**
 *
 * ReaderGroup is used to group a list of DataSetReaders. All ReaderGroups are
 * created within a PubSubConnection and automatically deleted if the connection
 * is removed. All network message related filters are only available in the DataSetReader. */
/* Add ReaderGroup to the created connection */
static UA_StatusCode
addReaderGroupConfig(UA_Server *server, UA_NodeId connectionId,
                     UA_ReaderGroupDataType *readerGroup,
                     UA_NodeId *readerGroupId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
    readerGroupConfig.name = readerGroup->name;
    return UA_ReaderGroup_create(psm, connectionId,
                                 &readerGroupConfig, readerGroupId);
}

/**
 * **SubscribedDataSet**
 *
 * Set SubscribedDataSet type to TargetVariables data type.
 * Add subscribedvariables to the DataSetReader */
static UA_StatusCode
addSubscribedVariables(UA_Server *server, UA_NodeId dataSetReaderId,
                       UA_DataSetReaderDataType *dataSetReader,
                       UA_DataSetMetaDataType *pMetaData) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ExtensionObject *eoTargetVar = &dataSetReader->subscribedDataSet;
    if(eoTargetVar->encoding != UA_EXTENSIONOBJECT_DECODED ||
       eoTargetVar->content.decoded.type != &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE])
        return UA_STATUSCODE_BADUNEXPECTEDERROR;

    const UA_TargetVariablesDataType *targetVars =
        (UA_TargetVariablesDataType*)eoTargetVar->content.decoded.data;

    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dataSetReaderId);
    if(!dsr)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId folderId;
    UA_String folderName = pMetaData->name;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_QualifiedName folderBrowseName;
    if(folderName.length > 0) {
        oAttr.displayName.locale = UA_STRING("");
        oAttr.displayName.text = folderName;
        folderBrowseName.namespaceIndex = 1;
        folderBrowseName.name = folderName;
    } else {
        oAttr.displayName = UA_LOCALIZEDTEXT("", "Subscribed Variables");
        folderBrowseName = UA_QUALIFIEDNAME(1, "Subscribed Variables");
    }

    addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NULL,
            UA_NS0ID(OBJECTSFOLDER), UA_NS0ID(ORGANIZES),
            folderBrowseName, UA_NS0ID(BASEOBJECTTYPE),
            &oAttr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
            NULL, &folderId);

    /* The SubscribedDataSet option TargetVariables defines a list of Variable
     * mappings between received DataSet fields and target Variables in the
     * Subscriber AddressSpace. The values subscribed from the Publisher are
     * updated in the value field of these variables */

    /* Add variable for the fields */
    for(size_t i = 0; i < targetVars->targetVariablesSize; i++) {
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.description = pMetaData->fields[i].description;
        vAttr.displayName.locale = UA_STRING("");
        vAttr.displayName.text = pMetaData->fields[i].name;
        vAttr.dataType = pMetaData->fields[i].dataType;
        UA_QualifiedName varname = {1, pMetaData->fields[i].name};
        addNode(server, UA_NODECLASS_VARIABLE,
                targetVars->targetVariables[i].targetNodeId, folderId,
                UA_NS0ID(HASCOMPONENT), varname, UA_NS0ID(BASEDATAVARIABLETYPE),
                &vAttr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                NULL, &targetVars->targetVariables[i].targetNodeId);
    }

    /* Set the TargetVariables in the DSR config */
    return DataSetReader_createTargetVariables(psm, dsr,
                                               targetVars->targetVariablesSize,
                                               targetVars->targetVariables);
}

/**
 * **DataSetReader**
 *
 * DataSetReader can receive NetworkMessages with the DataSetMessage
 * of interest sent by the Publisher. DataSetReader provides
 * the configuration necessary to receive and process DataSetMessages
 * on the Subscriber side. DataSetReader must be linked with a
 * SubscribedDataSet and be contained within a ReaderGroup. */
static UA_StatusCode
addDataSetReaderConfig(UA_Server *server, UA_NodeId readerGroupId,
                       UA_DataSetReaderDataType *dataSetReader,
                       UA_NodeId *dataSetReaderId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_DataSetReaderConfig readerConfig;
    memset(&readerConfig, 0, sizeof(UA_DataSetReaderConfig));

    UA_StatusCode retVal =
        UA_PublisherId_fromVariant(&readerConfig.publisherId,
                                   &dataSetReader->publisherId);
    readerConfig.name = dataSetReader->name;
    readerConfig.writerGroupId = dataSetReader->writerGroupId;
    readerConfig.dataSetWriterId = dataSetReader->dataSetWriterId;

    /* Setting up Meta data configuration in DataSetReader */
    UA_DataSetMetaDataType *pMetaData;
    pMetaData = &readerConfig.dataSetMetaData;
    UA_DataSetMetaDataType_init (pMetaData);
    pMetaData->name =  dataSetReader->dataSetMetaData.name;
    pMetaData->fieldsSize = dataSetReader->dataSetMetaData.fieldsSize;
    pMetaData->fields = (UA_FieldMetaData*)UA_Array_new (pMetaData->fieldsSize,
                        &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    for(size_t i = 0; i < pMetaData->fieldsSize; i++){
        UA_FieldMetaData_init (&pMetaData->fields[i]);
        UA_NodeId_copy(&dataSetReader->dataSetMetaData.fields[i].dataType,
                       &pMetaData->fields[i].dataType);
        pMetaData->fields[i].builtInType = dataSetReader->dataSetMetaData.fields[i].builtInType;
        pMetaData->fields[i].name = dataSetReader->dataSetMetaData.fields[i].name;
        pMetaData->fields[i].valueRank = dataSetReader->dataSetMetaData.fields[i].valueRank;
    }

    retVal |= UA_DataSetReader_create(psm, readerGroupId,
                                      &readerConfig, dataSetReaderId);
    UA_PublisherId_clear(&readerConfig.publisherId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_free(pMetaData->fields);
        return retVal;
    }

    retVal |= addSubscribedVariables(server, *dataSetReaderId, dataSetReader, pMetaData);
    UA_free(pMetaData->fields);
    return retVal;
}

/*************************************************/
/*            PubSubConnection                   */
/*************************************************/

UA_StatusCode
addPubSubConnectionRepresentation(UA_Server *server, UA_PubSubConnection *connection) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(connection->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    char connectionName[513];
    memcpy(connectionName, connection->config.name.data, connection->config.name.length);
    connectionName[connection->config.name.length] = '\0';

    UA_ObjectAttributes attr = UA_ObjectAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("", connectionName);
    retVal |= addNode_begin(server, UA_NODECLASS_OBJECT,
                            UA_NODEID_NUMERIC(1, 0), /* Generate a new id */
                            UA_NS0ID(PUBLISHSUBSCRIBE),
                            UA_NS0ID(HASPUBSUBCONNECTION),
                            UA_QUALIFIEDNAME(0, connectionName),
                            UA_NS0ID(PUBSUBCONNECTIONTYPE),
                            (const UA_NodeAttributes*)&attr,
                            &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                            NULL, &connection->head.identifier);

    attr.displayName = UA_LOCALIZEDTEXT("", "Address");
    retVal |= addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0),
                      connection->head.identifier, UA_NS0ID(HASCOMPONENT),
                      UA_QUALIFIEDNAME(0, "Address"), UA_NS0ID(NETWORKADDRESSURLTYPE),
                      &attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, NULL);

    retVal |= addNode_finish(server, &server->adminSession, &connection->head.identifier);

    UA_NodeId addressNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Address"),
                            UA_NS0ID(HASCOMPONENT), connection->head.identifier);
    UA_NodeId urlNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Url"),
                            UA_NS0ID(HASCOMPONENT), addressNode);
    UA_NodeId interfaceNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "NetworkInterface"),
                            UA_NS0ID(HASCOMPONENT), addressNode);
    UA_NodeId publisherIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                            UA_NS0ID(HASPROPERTY), connection->head.identifier);
    UA_NodeId connectionPropertyNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConnectionProperties"),
                            UA_NS0ID(HASPROPERTY), connection->head.identifier);
    UA_NodeId transportProfileUri =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "TransportProfileUri"),
                            UA_NS0ID(HASCOMPONENT), connection->head.identifier);

    if(UA_NodeId_isNull(&addressNode) || UA_NodeId_isNull(&urlNode) ||
       UA_NodeId_isNull(&interfaceNode) || UA_NodeId_isNull(&publisherIdNode) ||
       UA_NodeId_isNull(&connectionPropertyNode) ||
       UA_NodeId_isNull(&transportProfileUri)) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    retVal |= writePubSubNs0VariableArray(server, connectionPropertyNode,
                                          connection->config.connectionProperties.map,
                                          connection->config.connectionProperties.mapSize,
                                          &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);

    UA_NetworkAddressUrlDataType *networkAddressUrl=
        ((UA_NetworkAddressUrlDataType*)connection->config.address.data);
    UA_Variant value;
    UA_Variant_init(&value);

    UA_Variant_setScalar(&value, &networkAddressUrl->url, &UA_TYPES[UA_TYPES_STRING]);
    writeValueAttribute(server, urlNode, &value);

    UA_Variant_setScalar(&value, &networkAddressUrl->networkInterface, &UA_TYPES[UA_TYPES_STRING]);
    writeValueAttribute(server, interfaceNode, &value);

    UA_Variant_setScalar(&value, &connection->config.transportProfileUri, &UA_TYPES[UA_TYPES_STRING]);
    writeValueAttribute(server, transportProfileUri, &value);

    UA_NodePropertyContext *connectionPublisherIdContext =
        (UA_NodePropertyContext *)UA_malloc(sizeof(UA_NodePropertyContext));
    connectionPublisherIdContext->parentNodeId = connection->head.identifier;
    connectionPublisherIdContext->parentClassifier = UA_NS0ID_PUBSUBCONNECTIONTYPE;
    connectionPublisherIdContext->elementClassiefier = UA_NS0ID_PUBSUBCONNECTIONTYPE_PUBLISHERID;

    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;
    retVal |= addVariableValueSource(server, valueCallback, publisherIdNode,
                                     connectionPublisherIdContext);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, connection->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBSUBCONNECTIONTYPE_ADDWRITERGROUP), true);
        retVal |= addRef(server, connection->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBSUBCONNECTIONTYPE_ADDREADERGROUP), true);
        retVal |= addRef(server, connection->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBSUBCONNECTIONTYPE_REMOVEGROUP), true);
    }
    return retVal;
}

static UA_StatusCode
addPubSubConnectionAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_PubSubConnectionDataType *pubSubConnection =
        (UA_PubSubConnectionDataType *) input[0].data;

    //call API function and create the connection
    UA_NodeId connectionId;
    retVal |= addPubSubConnectionConfig(server, pubSubConnection, &connectionId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "addPubSubConnection failed");
        return retVal;
    }

    for(size_t i = 0; i < pubSubConnection->writerGroupsSize; i++) {
        UA_NodeId writerGroupId;
        UA_WriterGroupDataType *writerGroup = &pubSubConnection->writerGroups[i];
        retVal |= addWriterGroupConfig(server, connectionId, writerGroup, &writerGroupId);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "addWriterGroup failed");
            return retVal;
        }

        for(size_t j = 0; j < writerGroup->dataSetWritersSize; j++) {
            UA_DataSetWriterDataType *dataSetWriter = &writerGroup->dataSetWriters[j];
            retVal |= addDataSetWriterConfig(server, &writerGroupId, dataSetWriter, NULL);
            if(retVal != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                             "addDataSetWriter failed");
                return retVal;
            }
        }

        /* TODO: Need to handle the UA_Server_setWriterGroupOperational based on
         * the status variable in information model */
        UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroupId);
        if(!wg)
            continue;
        if(pubSubConnection->enabled) {
            UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_OPERATIONAL);
        } else {
            UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED);
        }
    }

    for(size_t i = 0; i < pubSubConnection->readerGroupsSize; i++){
        UA_NodeId readerGroupId;
        UA_ReaderGroupDataType *readerGroup = &pubSubConnection->readerGroups[i];
        retVal |= addReaderGroupConfig(server, connectionId, readerGroup, &readerGroupId);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "addReaderGroup failed");
            return retVal;
        }

        for(size_t j = 0; j < readerGroup->dataSetReadersSize; j++) {
            UA_NodeId dataSetReaderId;
            UA_DataSetReaderDataType *dataSetReader = &readerGroup->dataSetReaders[j];
            retVal |= addDataSetReaderConfig(server, readerGroupId,
                                             dataSetReader, &dataSetReaderId);
            if(retVal != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                             "addDataSetReader failed");
                return retVal;
            }

        }

        /* TODO: Need to handle the UA_Server_setReaderGroupOperational based on
         * the status variable in information model */
        UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupId);
        if(!rg)
            continue;
        if(pubSubConnection->enabled) {
            UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_OPERATIONAL);
        } else {
            UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED);
        }
    }

    /* Set ouput value */
    UA_Variant_setScalarCopy(output, &connectionId, &UA_TYPES[UA_TYPES_NODEID]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeConnectionAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    retVal |= UA_Server_removePubSubConnection(server, nodeToRemove);
    if(retVal == UA_STATUSCODE_BADNOTFOUND)
        retVal = UA_STATUSCODE_BADNODEIDUNKNOWN;
    return retVal;
}

/**********************************************/
/*               DataSetReader                */
/**********************************************/

UA_StatusCode
addDataSetReaderRepresentation(UA_Server *server, UA_DataSetReader *dataSetReader){
    UA_LOCK_ASSERT(&server->serviceMutex);

    if(dataSetReader->config.name.length > 512)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;

    char dsrName[513];
    memcpy(dsrName, dataSetReader->config.name.data, dataSetReader->config.name.length);
    dsrName[dataSetReader->config.name.length] = '\0';

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_NodeId publisherIdNode, writerGroupIdNode, dataSetwriterIdNode, statusIdNode, stateIdNode;

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", dsrName);
    retVal = addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* create an id */
                     dataSetReader->linkedReaderGroup->head.identifier,
                     UA_NODEID_NUMERIC(0, UA_NS0ID_HASDATASETREADER),
                     UA_QUALIFIEDNAME(0, dsrName),
                     UA_NODEID_NUMERIC(0, UA_NS0ID_DATASETREADERTYPE),
                     &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                     NULL, &dataSetReader->head.identifier);

    /* Add childNodes such as PublisherId, WriterGroupId and DataSetWriterId in
     * DataSetReader object */
    publisherIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                                          UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                          dataSetReader->head.identifier);
    writerGroupIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "WriterGroupId"),
                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                            dataSetReader->head.identifier);
    dataSetwriterIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY),
                                              dataSetReader->head.identifier);
    statusIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              dataSetReader->head.identifier);

    if(UA_NodeId_isNull(&statusIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    stateIdNode = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                                              UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                              statusIdNode);

    if(UA_NodeId_isNull(&publisherIdNode) ||
       UA_NodeId_isNull(&writerGroupIdNode) ||
       UA_NodeId_isNull(&dataSetwriterIdNode) ||
       UA_NodeId_isNull(&stateIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *dataSetReaderPublisherIdContext =
        (UA_NodePropertyContext *) UA_malloc(sizeof(UA_NodePropertyContext));
    dataSetReaderPublisherIdContext->parentNodeId = dataSetReader->head.identifier;
    dataSetReaderPublisherIdContext->parentClassifier = UA_NS0ID_DATASETREADERTYPE;
    dataSetReaderPublisherIdContext->elementClassiefier = UA_NS0ID_DATASETREADERTYPE_PUBLISHERID;
    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;
    retVal |= addVariableValueSource(server, valueCallback, publisherIdNode,
                                     dataSetReaderPublisherIdContext);

    UA_NodePropertyContext *dataSetReaderStateContext =
        (UA_NodePropertyContext *) UA_malloc(sizeof(UA_NodePropertyContext));
    UA_CHECK_MEM(dataSetReaderStateContext, return UA_STATUSCODE_BADOUTOFMEMORY);
    dataSetReaderStateContext->parentNodeId = dataSetReader->head.identifier;
    dataSetReaderStateContext->parentClassifier = UA_NS0ID_DATASETREADERTYPE;
    dataSetReaderStateContext->elementClassiefier = UA_NS0ID_DATASETREADERTYPE_STATUS_STATE;

    retVal |= addVariableValueSource(server, valueCallback, stateIdNode,
                                     dataSetReaderStateContext);

    /* Update childNode with values from Publisher */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &dataSetReader->config.writerGroupId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    writeValueAttribute(server, writerGroupIdNode, &value);
    UA_Variant_setScalar(&value, &dataSetReader->config.dataSetWriterId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    writeValueAttribute(server, dataSetwriterIdNode, &value);
    return retVal;
}

static UA_StatusCode
addDataSetReaderAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodeId dataSetReaderId;
    UA_DataSetReaderDataType *dataSetReader= (UA_DataSetReaderDataType *) input[0].data;
    UA_StatusCode retVal =
        addDataSetReaderConfig(server, *objectId, dataSetReader, &dataSetReaderId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "AddDataSetReader failed");
        return retVal;
    }

    UA_Variant_setScalarCopy(output, &dataSetReaderId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}

static UA_StatusCode
removeDataSetReaderAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_NodeId nodeToRemove = *((UA_NodeId *)input[0].data);
    return UA_Server_removeDataSetReader(server, nodeToRemove);
}

/*************************************************/
/*                PublishedDataSet               */
/*************************************************/

static UA_StatusCode
addDataSetFolderAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output){
    /* defined in R 1.04 9.1.4.5.7 */
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String newFolderName = *((UA_String *) input[0].data);
    UA_NodeId generatedId;
    UA_ObjectAttributes objectAttributes = UA_ObjectAttributes_default;
    UA_LocalizedText name = {UA_STRING(""), newFolderName};
    objectAttributes.displayName = name;
    retVal |= UA_Server_addObjectNode(server, UA_NODEID_NULL, *objectId,
                                      UA_NS0ID(ORGANIZES),
                                      UA_QUALIFIEDNAME(0, "DataSetFolder"),
                                      UA_NS0ID(DATASETFOLDERTYPE),
                                      objectAttributes, NULL, &generatedId);
    UA_Variant_setScalarCopy(output, &generatedId, &UA_TYPES[UA_TYPES_NODEID]);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= UA_Server_addReference(server, generatedId, UA_NS0ID(HASCOMPONENT),
                                         UA_NS0EXID(DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), true);
        retVal |= UA_Server_addReference(server, generatedId, UA_NS0ID(HASCOMPONENT),
                                         UA_NS0EXID(DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), true);
        retVal |= UA_Server_addReference(server, generatedId, UA_NS0ID(HASCOMPONENT),
                                         UA_NS0EXID(DATASETFOLDERTYPE_ADDDATASETFOLDER), true);
        retVal |= UA_Server_addReference(server, generatedId, UA_NS0ID(HASCOMPONENT),
                                         UA_NS0EXID(DATASETFOLDERTYPE_REMOVEDATASETFOLDER), true);
    }
    return retVal;
}

static UA_StatusCode
removeDataSetFolderAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output) {
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    return UA_Server_deleteNode(server, nodeToRemove, true);
}

UA_StatusCode
addPublishedDataItemsRepresentation(UA_Server *server,
                                    UA_PublishedDataSet *publishedDataSet) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(publishedDataSet->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    char pdsName[513];
    memcpy(pdsName, publishedDataSet->config.name.data, publishedDataSet->config.name.length);
    pdsName[publishedDataSet->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", pdsName);
    retVal = addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* Create a new id */
                     UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                     UA_NS0ID(HASCOMPONENT),
                     UA_QUALIFIEDNAME(0, pdsName),
                     UA_NS0ID(PUBLISHEDDATAITEMSTYPE),
                     &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                     NULL, &publishedDataSet->head.identifier);
    UA_CHECK_STATUS(retVal, return retVal);

    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;
    //ToDo: Need to move the browse name from namespaceindex 0 to 1
    UA_NodeId configurationVersionNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConfigurationVersion"),
                            UA_NS0ID(HASPROPERTY), publishedDataSet->head.identifier);
    if(UA_NodeId_isNull(&configurationVersionNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *configurationVersionContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    configurationVersionContext->parentNodeId = publishedDataSet->head.identifier;
    configurationVersionContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    configurationVersionContext->elementClassiefier =
        UA_NS0ID_PUBLISHEDDATASETTYPE_CONFIGURATIONVERSION;
    retVal |= addVariableValueSource(server, valueCallback, configurationVersionNode,
                                     configurationVersionContext);

    UA_NodeId publishedDataNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishedData"),
                            UA_NS0ID(HASPROPERTY), publishedDataSet->head.identifier);
    if(UA_NodeId_isNull(&publishedDataNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext * publishingIntervalContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    publishingIntervalContext->parentNodeId = publishedDataSet->head.identifier;
    publishingIntervalContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    publishingIntervalContext->elementClassiefier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE_PUBLISHEDDATA;
    retVal |= addVariableValueSource(server, valueCallback, publishedDataNode,
                                     publishingIntervalContext);

    UA_NodeId dataSetMetaDataNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), publishedDataSet->head.identifier);
    if(UA_NodeId_isNull(&dataSetMetaDataNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *metaDataContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    metaDataContext->parentNodeId = publishedDataSet->head.identifier;
    metaDataContext->parentClassifier = UA_NS0ID_PUBLISHEDDATAITEMSTYPE;
    metaDataContext->elementClassiefier = UA_NS0ID_PUBLISHEDDATASETTYPE_DATASETMETADATA;
    retVal |= addVariableValueSource(server, valueCallback,
                                     dataSetMetaDataNode, metaDataContext);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, publishedDataSet->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBLISHEDDATAITEMSTYPE_ADDVARIABLES), true);
        retVal |= addRef(server, publishedDataSet->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(PUBLISHEDDATAITEMSTYPE_REMOVEVARIABLES), true);
    }
    return retVal;
}

static UA_StatusCode
addPublishedDataItemsAction(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    size_t fieldNameAliasesSize = input[1].arrayLength;
    UA_String * fieldNameAliases = (UA_String *) input[1].data;
    size_t fieldFlagsSize = input[2].arrayLength;
    UA_DataSetFieldFlags * fieldFlags = (UA_DataSetFieldFlags *) input[2].data;
    size_t variablesToAddSize = input[3].arrayLength;
    UA_PublishedVariableDataType *eoAddVar =
        (UA_PublishedVariableDataType *)input[3].data;

    if(fieldNameAliasesSize != fieldFlagsSize ||
       fieldFlagsSize != variablesToAddSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(publishedDataSetConfig));
    publishedDataSetConfig.name = *((UA_String *) input[0].data);
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;

    UA_NodeId dataSetItemsNodeId;
    retVal |= UA_Server_addPublishedDataSet(server, &publishedDataSetConfig,
                                            &dataSetItemsNodeId).addResult;
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "addPublishedDataset failed");
        return retVal;
    }

    UA_DataSetFieldConfig dataSetFieldConfig;
    for(size_t j = 0; j < variablesToAddSize; ++j) {
        /* Prepare the config */
        memset(&dataSetFieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        dataSetFieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        dataSetFieldConfig.field.variable.fieldNameAlias = fieldNameAliases[j];
        dataSetFieldConfig.field.variable.publishParameters = eoAddVar[j];
        if(fieldFlags[j] == UA_DATASETFIELDFLAGS_PROMOTEDFIELD)
            dataSetFieldConfig.field.variable.promotedField = true;
        retVal |= UA_Server_addDataSetField(server, dataSetItemsNodeId,
                                            &dataSetFieldConfig, NULL).result;
        if(retVal != UA_STATUSCODE_GOOD) {
           UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                        "addDataSetField failed");
           return retVal;
        }
    }

    UA_Variant_setScalarCopy(output, &dataSetItemsNodeId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}

static UA_StatusCode
addVariablesAction(UA_Server *server,
                   const UA_NodeId *sessionId, void *sessionContext,
                   const UA_NodeId *methodId, void *methodContext,
                   const UA_NodeId *objectId, void *objectContext,
                   size_t inputSize, const UA_Variant *input,
                   size_t outputSize, UA_Variant *output){
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeVariablesAction(UA_Server *server,
                      const UA_NodeId *sessionId, void *sessionContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext,
                      size_t inputSize, const UA_Variant *input,
                      size_t outputSize, UA_Variant *output){
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removePublishedDataSetAction(UA_Server *server,
                             const UA_NodeId *sessionId, void *sessionContext,
                             const UA_NodeId *methodId, void *methodContext,
                             const UA_NodeId *objectId, void *objectContext,
                             size_t inputSize, const UA_Variant *input,
                             size_t outputSize, UA_Variant *output){
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    return UA_Server_removePublishedDataSet(server, nodeToRemove);
}

/*********************/
/* SubscribedDataSet */
/*********************/

UA_StatusCode
addSubscribedDataSetRepresentation(UA_Server *server,
                                   UA_SubscribedDataSet *subscribedDataSet) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    if(subscribedDataSet->config.name.length > 512)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;

    UA_STACKARRAY(char, sdsName, sizeof(char) * subscribedDataSet->config.name.length +1);
    memcpy(sdsName, subscribedDataSet->config.name.data, subscribedDataSet->config.name.length);
    sdsName[subscribedDataSet->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", sdsName);
    addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* Create a new id */
            UA_NS0ID(PUBLISHSUBSCRIBE_SUBSCRIBEDDATASETS),
            UA_NS0ID(HASCOMPONENT),
            UA_QUALIFIEDNAME(0, sdsName),
            UA_NS0ID(STANDALONESUBSCRIBEDDATASETTYPE),
            &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
            NULL, &subscribedDataSet->head.identifier);
    UA_NodeId sdsObjectNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "SubscribedDataSet"),
                            UA_NS0ID(HASCOMPONENT), subscribedDataSet->head.identifier);
    UA_NodeId metaDataId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), subscribedDataSet->head.identifier);
    UA_NodeId connectedId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "IsConnected"),
                            UA_NS0ID(HASPROPERTY), subscribedDataSet->head.identifier);

    if(UA_NodeId_equal(&sdsObjectNode, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&metaDataId, &UA_NODEID_NULL) ||
       UA_NodeId_equal(&connectedId, &UA_NODEID_NULL)) {
        return UA_STATUSCODE_BADNOTFOUND;
    }
    if(subscribedDataSet->config.subscribedDataSetType == UA_PUBSUB_SDS_TARGET){
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        UA_NodeId targetVarsId;
        attr.displayName = UA_LOCALIZEDTEXT("", "TargetVariables");
        attr.dataType = UA_TYPES[UA_TYPES_FIELDTARGETDATATYPE].typeId;
        attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
        attr.arrayDimensionsSize = 1;
        UA_UInt32 arrayDimensions[1];
        arrayDimensions[0] = (UA_UInt32)
            subscribedDataSet->config.subscribedDataSet.target.targetVariablesSize;
        attr.arrayDimensions = arrayDimensions;
        attr.accessLevel = UA_ACCESSLEVELMASK_READ;
        UA_Variant_setArray(&attr.value,
                            subscribedDataSet->config.subscribedDataSet.target.targetVariables,
                            subscribedDataSet->config.subscribedDataSet.target.targetVariablesSize,
                            &UA_TYPES[UA_TYPES_FIELDTARGETDATATYPE]);
        ret |= addNode(server, UA_NODECLASS_VARIABLE, UA_NODEID_NULL, sdsObjectNode,
                       UA_NS0ID(HASPROPERTY), UA_QUALIFIEDNAME(0, "TargetVariables"),
                       UA_NS0ID(PROPERTYTYPE), &attr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                       NULL, &targetVarsId);
    }

    UA_NodePropertyContext *isConnectedNodeContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    isConnectedNodeContext->parentNodeId = subscribedDataSet->head.identifier;
    isConnectedNodeContext->parentClassifier = UA_NS0ID_STANDALONESUBSCRIBEDDATASETREFDATATYPE;
    isConnectedNodeContext->elementClassiefier = UA_NS0ID_STANDALONESUBSCRIBEDDATASETTYPE_ISCONNECTED;

    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;
    ret |= addVariableValueSource(server, valueCallback, connectedId, isConnectedNodeContext);

    UA_NodePropertyContext *metaDataContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    metaDataContext->parentNodeId = subscribedDataSet->head.identifier;
    metaDataContext->parentClassifier = UA_NS0ID_STANDALONESUBSCRIBEDDATASETREFDATATYPE;
    metaDataContext->elementClassiefier = UA_NS0ID_STANDALONESUBSCRIBEDDATASETTYPE_DATASETMETADATA;
    ret |= addVariableValueSource(server, valueCallback, metaDataId, metaDataContext);

    return ret;
}

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

static UA_StatusCode
readContentMask(UA_Server *server, const UA_NodeId *sessionId,
                void *sessionContext, const UA_NodeId *nodeId,
                void *nodeContext, UA_Boolean includeSourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    UA_WriterGroup *writerGroup = (UA_WriterGroup*)nodeContext;
    if((writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       writerGroup->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        writerGroup->config.messageSettings.content.decoded.data;

    UA_Variant_setScalarCopy(&value->value, &wgm->networkMessageContentMask,
                             &UA_TYPES[UA_TYPES_UADPNETWORKMESSAGECONTENTMASK]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeContentMask(UA_Server *server, const UA_NodeId *sessionId,
                 void *sessionContext, const UA_NodeId *nodeId,
                 void *nodeContext, const UA_NumericRange *range,
                 const UA_DataValue *value) {
    UA_WriterGroup *writerGroup = (UA_WriterGroup*)nodeContext;
    if((writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       writerGroup->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        writerGroup->config.messageSettings.content.decoded.data;

    if(!value->value.type)
        return UA_STATUSCODE_BADTYPEMISMATCH;
    if(value->value.type->typeKind != UA_DATATYPEKIND_ENUM &&
       value->value.type->typeKind != UA_DATATYPEKIND_INT32)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    wgm->networkMessageContentMask = *(UA_UadpNetworkMessageContentMask*)value->value.data;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
readGroupVersion(UA_Server *server, const UA_NodeId *sessionId,
                void *sessionContext, const UA_NodeId *nodeId,
                void *nodeContext, UA_Boolean includeSourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    UA_WriterGroup *writerGroup = (UA_WriterGroup*)nodeContext;
    if((writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED &&
        writerGroup->config.messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED_NODELETE) ||
       writerGroup->config.messageSettings.content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        writerGroup->config.messageSettings.content.decoded.data;

    UA_Variant_setScalarCopy(&value->value, &wgm->groupVersion,
                             &UA_TYPES[UA_DATATYPEKIND_UINT32]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
addWriterGroupRepresentation(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(writerGroup->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    char wgName[513];
    memcpy(wgName, writerGroup->config.name.data, writerGroup->config.name.length);
    wgName[writerGroup->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", wgName);
    retVal = addNode(server, UA_NODECLASS_OBJECT,
                     UA_NODEID_NUMERIC(1, 0), /* create a new id */
                     writerGroup->linkedConnection->head.identifier, UA_NS0ID(HASCOMPONENT),
                     UA_QUALIFIEDNAME(0, wgName), UA_NS0ID(WRITERGROUPTYPE), &object_attr,
                     &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, &writerGroup->head.identifier);

    UA_NodeId keepAliveNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "KeepAliveTime"),
                            UA_NS0ID(HASPROPERTY), writerGroup->head.identifier);
    UA_NodeId publishingIntervalNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                            UA_NS0ID(HASPROPERTY), writerGroup->head.identifier);
    UA_NodeId statusIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), writerGroup->head.identifier);

    if(UA_NodeId_isNull(&statusIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodeId stateIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusIdNode);

    if(UA_NodeId_isNull(&keepAliveNode) ||
       UA_NodeId_isNull(&publishingIntervalNode) ||
       UA_NodeId_isNull(&stateIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext * publishingIntervalContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    publishingIntervalContext->parentNodeId = writerGroup->head.identifier;
    publishingIntervalContext->parentClassifier = UA_NS0ID_WRITERGROUPTYPE;
    publishingIntervalContext->elementClassiefier = UA_NS0ID_WRITERGROUPTYPE_PUBLISHINGINTERVAL;
    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = onWrite;
    retVal |= addVariableValueSource(server, valueCallback,
                                     publishingIntervalNode, publishingIntervalContext);
    writeAccessLevelAttribute(server, publishingIntervalNode,
                              UA_ACCESSLEVELMASK_READ ^ UA_ACCESSLEVELMASK_WRITE);

    UA_NodePropertyContext * stateContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    UA_CHECK_MEM(stateContext, return UA_STATUSCODE_BADOUTOFMEMORY);
    stateContext->parentNodeId = writerGroup->head.identifier;
    stateContext->parentClassifier = UA_NS0ID_WRITERGROUPTYPE;
    stateContext->elementClassiefier = UA_NS0ID_PUBSUBGROUPTYPE_STATUS_STATE;
    UA_ValueCallback stateValueCallback;
    stateValueCallback.onRead = onRead;
    stateValueCallback.onWrite = NULL;
    retVal |= addVariableValueSource(server, stateValueCallback,
                                     stateIdNode, stateContext);

    UA_NodeId priorityNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Priority"),
                            UA_NS0ID(HASPROPERTY), writerGroup->head.identifier);
    UA_NodeId writerGroupIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "WriterGroupId"),
                            UA_NS0ID(HASPROPERTY), writerGroup->head.identifier);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &writerGroup->config.publishingInterval, &UA_TYPES[UA_TYPES_DURATION]);
    writeValueAttribute(server, publishingIntervalNode, &value);
    UA_Variant_setScalar(&value, &writerGroup->config.keepAliveTime, &UA_TYPES[UA_TYPES_DURATION]);
    writeValueAttribute(server, keepAliveNode, &value);
    UA_Variant_setScalar(&value, &writerGroup->config.priority, &UA_TYPES[UA_TYPES_BYTE]);
    writeValueAttribute(server, priorityNode, &value);
    UA_Variant_setScalar(&value, &writerGroup->config.writerGroupId, &UA_TYPES[UA_TYPES_UINT16]);
    writeValueAttribute(server, writerGroupIdNode, &value);

    object_attr.displayName = UA_LOCALIZEDTEXT("", "MessageSettings");
    retVal |= addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0),
                      writerGroup->head.identifier, UA_NS0ID(HASCOMPONENT),
                      UA_QUALIFIEDNAME(0, "MessageSettings"),
                      UA_NS0ID(UADPWRITERGROUPMESSAGETYPE), &object_attr,
                      &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                      NULL, NULL);

    /* Find the variable with the content mask */

    UA_NodeId messageSettingsId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "MessageSettings"),
                            UA_NS0ID(HASCOMPONENT), writerGroup->head.identifier);
    UA_NodeId contentMaskId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "NetworkMessageContentMask"),
                            UA_NS0ID(HASPROPERTY), messageSettingsId);
    if(!UA_NodeId_isNull(&contentMaskId)) {
        /* Set the callback */
        UA_DataSource ds;
        ds.read = readContentMask;
        ds.write = writeContentMask;
        setVariableNode_dataSource(server, contentMaskId, ds);
        setNodeContext(server, contentMaskId, writerGroup);

        /* Make writable */
        writeAccessLevelAttribute(server, contentMaskId,
                                  UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_READ);

    }
    UA_NodeId groupVersionId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "GroupVersion"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), messageSettingsId);
    if(!UA_NodeId_isNull(&groupVersionId)) {
        /* Set the callback */
        UA_DataSource ds;
        ds.read = readGroupVersion;
        ds.write = NULL;
        setVariableNode_dataSource(server, groupVersionId, ds);
        setNodeContext(server, groupVersionId, writerGroup);

        /* Read only */
        writeAccessLevelAttribute(server, groupVersionId,
                                  UA_ACCESSLEVELMASK_READ);

    }

    /* Add reference to methods */
    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, writerGroup->head.identifier,
                         UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(WRITERGROUPTYPE_ADDDATASETWRITER), true);
        retVal |= addRef(server, writerGroup->head.identifier,
                         UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(WRITERGROUPTYPE_REMOVEDATASETWRITER), true);
    }
    return retVal;
}

static UA_StatusCode
addWriterGroupAction(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodeId writerGroupId;
    UA_WriterGroupDataType *writerGroup = (UA_WriterGroupDataType *)input->data;
    UA_StatusCode retVal = addWriterGroupConfig(server, *objectId, writerGroup, &writerGroupId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER, "addWriterGroup failed");
        return retVal;
    }
    // TODO: Need to handle the UA_Server_setWriterGroupOperational based on the
    // status variable in information model

    UA_Variant_setScalarCopy(output, &writerGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}

static UA_StatusCode
removeGroupAction(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *methodId, void *methodContext,
                  const UA_NodeId *objectId, void *objectContext,
                  size_t inputSize, const UA_Variant *input,
                  size_t outputSize, UA_Variant *output){
    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_NodeId nodeToRemove = *((UA_NodeId *)input->data);
    if(UA_WriterGroup_find(psm, nodeToRemove)) {
        return UA_Server_removeWriterGroup(server, nodeToRemove);
    } else {
        return UA_Server_removeReaderGroup(server, nodeToRemove);
    }
}

/**********************************************/
/*               ReserveIds                   */
/**********************************************/

static UA_StatusCode
addReserveIdsAction(UA_Server *server,
                    const UA_NodeId *sessionId, void *sessionContext,
                    const UA_NodeId *methodId, void *methodContext,
                    const UA_NodeId *objectId, void *objectContext,
                    size_t inputSize, const UA_Variant *input,
                    size_t outputSize, UA_Variant *output){
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String transportProfileUri = *((UA_String *)input[0].data);
    UA_UInt16 numRegWriterGroupIds = *((UA_UInt16 *)input[1].data);
    UA_UInt16 numRegDataSetWriterIds = *((UA_UInt16 *)input[2].data);

    UA_UInt16 *writerGroupIds;
    UA_UInt16 *dataSetWriterIds;

    retVal |= UA_PubSubManager_reserveIds(psm, *sessionId, numRegWriterGroupIds,
                                          numRegDataSetWriterIds, transportProfileUri,
                                          &writerGroupIds, &dataSetWriterIds);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER, "addReserveIds failed");
        return retVal;
    }

    /* Check the transportProfileUri */
    UA_String profile_1 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    UA_String profile_2 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json");

    if(UA_String_equal(&transportProfileUri, &profile_1) ||
       UA_String_equal(&transportProfileUri, &profile_2)) {
        UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER, "ApplicationUri: %S",
                    server->config.applicationDescription.applicationUri);
        retVal |= UA_Variant_setScalarCopy(&output[0],
                                           &server->config.applicationDescription.applicationUri,
                                           &UA_TYPES[UA_TYPES_STRING]);
    } else {
        retVal |= UA_Variant_setScalarCopy(&output[0], &psm->defaultPublisherId,
                                           &UA_TYPES[UA_TYPES_UINT64]);
    }

    UA_Variant_setArray(&output[1], writerGroupIds,
                        numRegWriterGroupIds, &UA_TYPES[UA_TYPES_UINT16]);
    UA_Variant_setArray(&output[2], dataSetWriterIds,
                        numRegDataSetWriterIds, &UA_TYPES[UA_TYPES_UINT16]);

    return retVal;
}

/**********************************************/
/*               ReaderGroup                  */
/**********************************************/

UA_StatusCode
addReaderGroupRepresentation(UA_Server *server, UA_ReaderGroup *readerGroup) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(readerGroup->config.name.length > 512)
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    char rgName[513];
    memcpy(rgName, readerGroup->config.name.data, readerGroup->config.name.length);
    rgName[readerGroup->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", rgName);
    UA_StatusCode retVal =
        addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* create an id */
                readerGroup->linkedConnection->head.identifier,
                UA_NS0ID(HASCOMPONENT),
                UA_QUALIFIEDNAME(0, rgName), UA_NS0ID(READERGROUPTYPE),
                &object_attr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                NULL, &readerGroup->head.identifier);

    UA_NodeId statusIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), readerGroup->head.identifier);

    if(UA_NodeId_isNull(&statusIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodeId stateIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusIdNode);

    if(UA_NodeId_isNull(&stateIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext * stateContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    UA_CHECK_MEM(stateContext, return UA_STATUSCODE_BADOUTOFMEMORY);
    stateContext->parentNodeId = readerGroup->head.identifier;
    stateContext->parentClassifier = UA_NS0ID_READERGROUPTYPE;
    stateContext->elementClassiefier = UA_NS0ID_PUBSUBGROUPTYPE_STATUS_STATE;
    UA_ValueCallback stateValueCallback;
    stateValueCallback.onRead = onRead;
    stateValueCallback.onWrite = NULL;
    retVal |= addVariableValueSource(server, stateValueCallback,
                                     stateIdNode, stateContext);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        retVal |= addRef(server, readerGroup->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(READERGROUPTYPE_ADDDATASETREADER), true);
        retVal |= addRef(server, readerGroup->head.identifier, UA_NS0ID(HASCOMPONENT),
                         UA_NS0ID(READERGROUPTYPE_REMOVEDATASETREADER), true);
    }
    return retVal;
}

static UA_StatusCode
addReaderGroupAction(UA_Server *server,
                     const UA_NodeId *sessionId, void *sessionContext,
                     const UA_NodeId *methodId, void *methodContext,
                     const UA_NodeId *objectId, void *objectContext,
                     size_t inputSize, const UA_Variant *input,
                     size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_ReaderGroupDataType *readerGroup = ((UA_ReaderGroupDataType *) input->data);
    UA_NodeId readerGroupId;
    retVal |= addReaderGroupConfig(server, *objectId, readerGroup, &readerGroupId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER, "addReaderGroup failed");
        return retVal;
    }
    // TODO: Need to handle the UA_Server_setReaderGroupOperational based on the
    // status variable in information model

    UA_Variant_setScalarCopy(output, &readerGroupId, &UA_TYPES[UA_TYPES_NODEID]);
    return retVal;
}

#ifdef UA_ENABLE_PUBSUB_SKS
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
static UA_Boolean
isValidParentNode(UA_Server *server, UA_NodeId parentId) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_Boolean retval = true;
    const UA_Node *parentNodeType;
    const UA_NodeId parentNodeTypeId = UA_NS0ID(SECURITYGROUPFOLDERTYPE);
    const UA_Node *parentNode = UA_NODESTORE_GET(server, &parentId);

    if(parentNode) {
        parentNodeType = getNodeType(server, &parentNode->head);
        if(parentNodeType) {
            retval = UA_NodeId_equal(&parentNodeType->head.nodeId, &parentNodeTypeId);
            UA_NODESTORE_RELEASE(server, parentNodeType);
        }
        UA_NODESTORE_RELEASE(server, parentNode);
    }
    return retval;
}

static UA_StatusCode
updateSecurityGroupProperties(UA_Server *server, UA_NodeId *securityGroupNodeId,
                              UA_SecurityGroupConfig *config) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &config->securityGroupName, &UA_TYPES[UA_TYPES_STRING]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "SecurityGroupId"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /*AddValueCallback*/
    UA_Variant_setScalar(&value, &config->securityPolicyUri, &UA_TYPES[UA_TYPES_STRING]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "SecurityPolicyUri"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Variant_setScalar(&value, &config->keyLifeTime, &UA_TYPES[UA_TYPES_DURATION]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "KeyLifetime"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Variant_setScalar(&value, &config->maxFutureKeyCount, &UA_TYPES[UA_TYPES_UINT32]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "MaxFutureKeyCount"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_Variant_setScalar(&value, &config->maxPastKeyCount, &UA_TYPES[UA_TYPES_UINT32]);
    retval = writeObjectProperty(server, *securityGroupNodeId,
                                 UA_QUALIFIEDNAME(0, "MaxPastKeyCount"), value);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    return retval;
}

UA_StatusCode
addSecurityGroupRepresentation(UA_Server *server, UA_SecurityGroup *securityGroup) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_BAD;

    UA_SecurityGroupConfig *securityGroupConfig = &securityGroup->config;
    if(!isValidParentNode(server, securityGroup->securityGroupFolderId))
        return UA_STATUSCODE_BADPARENTNODEIDINVALID;

    if(securityGroupConfig->securityGroupName.length <= 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_QualifiedName browseName;
    UA_QualifiedName_init(&browseName);
    browseName.name = securityGroupConfig->securityGroupName;

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName.text = securityGroupConfig->securityGroupName;
    UA_NodeId refType = UA_NS0ID(HASCOMPONENT);
    UA_NodeId nodeType = UA_NS0ID(SECURITYGROUPTYPE);
    retval = addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NULL,
                     securityGroup->securityGroupFolderId, refType,
                     browseName, nodeType, &object_attr,
                     &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL,
                     &securityGroup->securityGroupNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Add SecurityGroup failed with error: %s.",
                     UA_StatusCode_name(retval));
        return retval;
    }

    retval = updateSecurityGroupProperties(server,
                                           &securityGroup->securityGroupNodeId,
                                           securityGroupConfig);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "Add SecurityGroup failed with error: %s.",
                     UA_StatusCode_name(retval));
        deleteNode(server, securityGroup->securityGroupNodeId, true);
    }
    return retval;
}

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL*/
#endif /* UA_ENABLE_PUBSUB_SKS */

/**********************************************/
/*               DataSetWriter                */
/**********************************************/

UA_StatusCode
addDataSetWriterRepresentation(UA_Server *server, UA_DataSetWriter *dataSetWriter) {

    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(dataSetWriter->config.name.length > 512)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    char dswName[513];
    memcpy(dswName, dataSetWriter->config.name.data, dataSetWriter->config.name.length);
    dswName[dataSetWriter->config.name.length] = '\0';

    UA_ObjectAttributes object_attr = UA_ObjectAttributes_default;
    object_attr.displayName = UA_LOCALIZEDTEXT("", dswName);
    retVal =
        addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0), /* create an id */
                dataSetWriter->linkedWriterGroup->head.identifier, UA_NS0ID(HASDATASETWRITER),
                UA_QUALIFIEDNAME(0, dswName), UA_NS0ID(DATASETWRITERTYPE), &object_attr,
                &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                     NULL, &dataSetWriter->head.identifier);
    //if connected dataset is null this means it's configured for heartbeats
    if(dataSetWriter->connectedDataSet) {
        retVal |= addRef(server, dataSetWriter->connectedDataSet->head.identifier,
                         UA_NS0ID(DATASETTOWRITER), dataSetWriter->head.identifier, true);
    }

    UA_NodeId dataSetWriterIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                            UA_NS0ID(HASPROPERTY), dataSetWriter->head.identifier);
    UA_NodeId keyFrameNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "KeyFrameCount"),
                            UA_NS0ID(HASPROPERTY), dataSetWriter->head.identifier);
    UA_NodeId dataSetFieldContentMaskNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetFieldContentMask"),
                            UA_NS0ID(HASPROPERTY), dataSetWriter->head.identifier);

    UA_NodeId statusIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), dataSetWriter->head.identifier);
    
    if(UA_NodeId_isNull(&statusIdNode))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NodeId stateIdNode = 
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusIdNode);

    // TODO: The keyFrameNode is NULL here, should be check
    // does not depend on the pubsub changes
    if(UA_NodeId_isNull(&dataSetWriterIdNode) ||
       UA_NodeId_isNull(&dataSetFieldContentMaskNode) ||
       UA_NodeId_isNull(&stateIdNode))
            return UA_STATUSCODE_BADNOTFOUND;

    UA_NodePropertyContext *dataSetWriterIdContext = (UA_NodePropertyContext *)
        UA_malloc(sizeof(UA_NodePropertyContext));
    dataSetWriterIdContext->parentNodeId = dataSetWriter->head.identifier;
    dataSetWriterIdContext->parentClassifier = UA_NS0ID_DATASETWRITERTYPE;
    dataSetWriterIdContext->elementClassiefier = UA_NS0ID_DATASETWRITERTYPE_DATASETWRITERID;
    UA_ValueCallback valueCallback;
    valueCallback.onRead = onRead;
    valueCallback.onWrite = NULL;
    retVal |= addVariableValueSource(server, valueCallback,
                                     dataSetWriterIdNode, dataSetWriterIdContext);

    UA_NodePropertyContext *dataSetWriterStateContext =
        (UA_NodePropertyContext *) UA_malloc(sizeof(UA_NodePropertyContext));
    UA_CHECK_MEM(dataSetWriterStateContext, return UA_STATUSCODE_BADOUTOFMEMORY);
    dataSetWriterStateContext->parentNodeId = dataSetWriter->head.identifier;
    dataSetWriterStateContext->parentClassifier = UA_NS0ID_DATASETWRITERTYPE;
    dataSetWriterStateContext->elementClassiefier = UA_NS0ID_DATASETWRITERTYPE_STATUS_STATE;
    retVal |= addVariableValueSource(server, valueCallback,
                                     stateIdNode, dataSetWriterStateContext);

    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &dataSetWriter->config.dataSetWriterId,
                         &UA_TYPES[UA_TYPES_UINT16]);
    writeValueAttribute(server, dataSetWriterIdNode, &value);

    UA_Variant_setScalar(&value, &dataSetWriter->config.keyFrameCount,
                         &UA_TYPES[UA_TYPES_UINT32]);
    writeValueAttribute(server, keyFrameNode, &value);

    UA_Variant_setScalar(&value, &dataSetWriter->config.dataSetFieldContentMask,
                         &UA_TYPES[UA_TYPES_DATASETFIELDCONTENTMASK]);
    writeValueAttribute(server, dataSetFieldContentMaskNode, &value);

    object_attr.displayName = UA_LOCALIZEDTEXT("", "MessageSettings");
    retVal |= addNode(server, UA_NODECLASS_OBJECT, UA_NODEID_NUMERIC(1, 0),
                      dataSetWriter->head.identifier, UA_NS0ID(HASCOMPONENT),
                      UA_QUALIFIEDNAME(0, "MessageSettings"),
                      UA_NS0ID(UADPDATASETWRITERMESSAGETYPE), &object_attr,
                      &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                      NULL, NULL);

    return retVal;
}

static UA_StatusCode
addDataSetWriterAction(UA_Server *server,
                       const UA_NodeId *sessionId, void *sessionContext,
                       const UA_NodeId *methodId, void *methodContext,
                       const UA_NodeId *objectId, void *objectContext,
                       size_t inputSize, const UA_Variant *input,
                       size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_NodeId dataSetWriterId;
    UA_DataSetWriterDataType *dataSetWriterData = (UA_DataSetWriterDataType *)input->data;
    UA_StatusCode retVal =
        addDataSetWriterConfig(server, objectId, dataSetWriterData, &dataSetWriterId);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "addDataSetWriter failed");
        return retVal;
    }

    UA_Variant_setScalarCopy(output, &dataSetWriterId, &UA_TYPES[UA_TYPES_NODEID]);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
removeDataSetWriterAction(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *methodId, void *methodContext,
                          const UA_NodeId *objectId, void *objectContext,
                          size_t inputSize, const UA_Variant *input,
                          size_t outputSize, UA_Variant *output){
    UA_NodeId nodeToRemove = *((UA_NodeId *) input[0].data);
    return UA_Server_removeDataSetWriter(server, nodeToRemove);
}

#ifdef UA_ENABLE_PUBSUB_SKS

/**
 * @note The user credentials and permissions are checked in the AccessControl plugin
 * before this callback is executed.
 */
static UA_StatusCode
setSecurityKeysAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Validate the arguments */
    if(!server || !input)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(inputSize < 7)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 7 || outputSize > 0)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    /* Check whether the channel is encrypted according to specification */
    UA_Session *session = getSessionById(server, sessionId);
    if(!session || !session->channel)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(session->channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;

    /* Check for types */
    if(!UA_Variant_hasScalarType(&input[0], &UA_TYPES[UA_TYPES_STRING]) || /*SecurityGroupId*/
        !UA_Variant_hasScalarType(&input[1], &UA_TYPES[UA_TYPES_STRING]) || /*SecurityPolicyUri*/
        !UA_Variant_hasScalarType(&input[2], &UA_TYPES[UA_TYPES_INTEGERID]) || /*CurrentTokenId*/
        !UA_Variant_hasScalarType(&input[3], &UA_TYPES[UA_TYPES_BYTESTRING]) || /*CurrentKey*/
        !UA_Variant_hasArrayType(&input[4], &UA_TYPES[UA_TYPES_BYTESTRING]) || /*FutureKeys*/
        (!UA_Variant_hasScalarType(&input[5], &UA_TYPES[UA_TYPES_DURATION]) &&
        !UA_Variant_hasScalarType(&input[5], &UA_TYPES[UA_TYPES_DOUBLE])) || /*TimeToNextKey*/
        (!UA_Variant_hasScalarType(&input[6], &UA_TYPES[UA_TYPES_DURATION]) &&
        !UA_Variant_hasScalarType(&input[6], &UA_TYPES[UA_TYPES_DOUBLE]))) /*TimeToNextKey*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_Duration callbackTime;
    UA_String *securityGroupId = (UA_String *)input[0].data;
    UA_String *securityPolicyUri = (UA_String *)input[1].data;
    UA_UInt32 currentKeyId = *(UA_UInt32 *)input[2].data;
    UA_ByteString *currentKey = (UA_ByteString *)input[3].data;
    UA_ByteString *futureKeys = (UA_ByteString *)input[4].data;
    size_t futureKeySize = input[4].arrayLength;
    UA_Duration msTimeToNextKey = *(UA_Duration *)input[5].data;
    UA_Duration msKeyLifeTime = *(UA_Duration *)input[6].data;

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubKeyStorage *ks =
        UA_PubSubKeyStorage_find(psm, *securityGroupId);
    if(!ks)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!UA_String_equal(securityPolicyUri, &ks->policy->policyUri))
        return UA_STATUSCODE_BADSECURITYPOLICYREJECTED;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_PubSubKeyListItem *current = UA_PubSubKeyStorage_getKeyByKeyId(ks, currentKeyId);
    if(!current) {
        UA_PubSubKeyStorage_clearKeyList(ks);
        retval |= (UA_PubSubKeyStorage_push(ks, currentKey, currentKeyId)) ?
            UA_STATUSCODE_GOOD : UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_PubSubKeyStorage_setCurrentKey(ks, currentKeyId);
    retval |= UA_PubSubKeyStorage_addSecurityKeys(ks, futureKeySize,
                                                  futureKeys, currentKeyId);
    ks->keyLifeTime = msKeyLifeTime;
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    retval = UA_PubSubKeyStorage_activateKeyToChannelContext(psm, UA_NODEID_NULL,
                                                             ks->securityGroupID);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(
            server->config.logging, UA_LOGCATEGORY_SERVER,
            "Failed to import Symmetric Keys into PubSub Channel Context with %s \n",
            UA_StatusCode_name(retval));
        return retval;
    }

    callbackTime = msKeyLifeTime;
    if(msTimeToNextKey > 0)
        callbackTime = msTimeToNextKey;

    /* Move to setSecurityKeysAction */
    return UA_PubSubKeyStorage_addKeyRolloverCallback(
        psm, ks, (UA_Callback)UA_PubSubKeyStorage_keyRolloverCallback,
        callbackTime, &ks->callBackId);
}

static UA_StatusCode
getSecurityKeysAction(UA_Server *server, const UA_NodeId *sessionId, void *sessionContext,
                      const UA_NodeId *methodId, void *methodContext,
                      const UA_NodeId *objectId, void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Validate the arguments */
    if(!server || !input)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(inputSize < 3 || outputSize < 5)
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    if(inputSize > 3 || outputSize > 5)
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;

    /* Check whether the channel is encrypted according to specification */
    UA_Session *session = getSessionById(server, sessionId);
    if(!session || !session->channel)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(session->channel->securityMode != UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
        return UA_STATUSCODE_BADSECURITYMODEINSUFFICIENT;

    /* Check for types */
    if(!UA_Variant_hasScalarType(&input[0],
                                 &UA_TYPES[UA_TYPES_STRING]) || /*SecurityGroupId*/
       !UA_Variant_hasScalarType(&input[1],
                                 &UA_TYPES[UA_TYPES_INTEGERID]) || /*StartingTokenId*/
       !UA_Variant_hasScalarType(&input[2],
                                 &UA_TYPES[UA_TYPES_UINT32])) /*RequestedKeyCount*/
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_StatusCode retval = UA_STATUSCODE_BAD;
    UA_UInt32 currentKeyCount = 1;

    /* Input */
    UA_String *securityGroupId = (UA_String *)input[0].data;
    UA_UInt32 startingTokenId = *(UA_UInt32 *)input[1].data;
    UA_UInt32 requestedKeyCount = *(UA_UInt32 *)input[2].data;

    UA_PubSubManager *psm = getPSM(server);
    UA_PubSubKeyStorage *ks = UA_PubSubKeyStorage_find(psm, *securityGroupId);
    if(!ks)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_Boolean executable = false;
    UA_SecurityGroup *sg = UA_SecurityGroup_findByName(psm, *securityGroupId);
    void *sgNodeCtx;
    getNodeContext(server, sg->securityGroupNodeId, (void **)&sgNodeCtx);
    executable = server->config.accessControl.getUserExecutableOnObject(
        server, &server->config.accessControl, sessionId, sessionContext, methodId,
        methodContext, &sg->securityGroupNodeId, sgNodeCtx);

    if(!executable)
        return UA_STATUSCODE_BADUSERACCESSDENIED;

    /* If the caller requests a number larger than the Security Key Service
     * permits, then the SKS shall return the maximum it allows.*/
    if(requestedKeyCount > sg->config.maxFutureKeyCount)
        requestedKeyCount =(UA_UInt32) sg->keyStorage->keyListSize;
    else
        requestedKeyCount = requestedKeyCount + currentKeyCount; /* Add Current keyCount */

    /* The current token is requested by passing 0. */
    UA_PubSubKeyListItem *startingItem = NULL;
    if(startingTokenId == 0) {
        /* currentItem is always set by the server when a security group is added */
        UA_assert(sg->keyStorage->currentItem != NULL);
        startingItem = sg->keyStorage->currentItem;
    } else {
        startingItem = UA_PubSubKeyStorage_getKeyByKeyId(sg->keyStorage, startingTokenId);
        /* If the StartingTokenId is unknown, the oldest (firstItem) available
         * tokens are returned. */
        if(!startingItem)
            startingItem = TAILQ_FIRST(&sg->keyStorage->keyList);
    }

    /* SecurityPolicyUri */
    retval = UA_Variant_setScalarCopy(&output[0], &(sg->keyStorage->policy->policyUri),
                         &UA_TYPES[UA_TYPES_STRING]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* FirstTokenId */
    retval = UA_Variant_setScalarCopy(&output[1], &startingItem->keyID,
                                      &UA_TYPES[UA_TYPES_INTEGERID]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* TimeToNextKey */
    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime baseTime = sg->baseTime;
    UA_DateTime currentTime = el->dateTime_nowMonotonic(el);
    UA_Duration interval = sg->config.keyLifeTime;
    UA_Duration timeToNextKey =
        (UA_Duration)((currentTime - baseTime) / UA_DATETIME_MSEC);
    timeToNextKey = interval - timeToNextKey;
    retval = UA_Variant_setScalarCopy(&output[3], &timeToNextKey,
                                      &UA_TYPES[UA_TYPES_DURATION]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* KeyLifeTime */
    retval = UA_Variant_setScalarCopy(&output[4], &sg->config.keyLifeTime,
                         &UA_TYPES[UA_TYPES_DURATION]);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Keys */
    UA_PubSubKeyListItem *iterator = startingItem;
    output[2].data = (UA_ByteString *)
        UA_calloc(requestedKeyCount, startingItem->key.length);
    if(!output[2].data)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_ByteString *requestedKeys = (UA_ByteString *)output[2].data;
    UA_UInt32 retkeyCount = 0;
    for(size_t i = 0; i < requestedKeyCount; i++) {
        UA_ByteString_copy(&iterator->key, &requestedKeys[i]);
        ++retkeyCount;
        iterator = TAILQ_NEXT(iterator, keyListEntry);
        if(!iterator) {
            requestedKeyCount = retkeyCount;
            break;
        }
    }

    UA_Variant_setArray(&output[2], requestedKeys, requestedKeyCount,
                        &UA_TYPES[UA_TYPES_BYTESTRING]);
    return retval;
}

#endif

/**********************************************/
/*                Destructors                 */
/**********************************************/

static void
connectionTypeDestructor(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionContext,
                         const UA_NodeId *typeId, void *typeContext,
                         const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "Connection destructor called!");
    UA_NodeId publisherIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                            UA_NS0ID(HASPROPERTY), *nodeId);
    UA_NodePropertyContext *ctx;
    getNodeContext(server, publisherIdNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&publisherIdNode))
        UA_free(ctx);
}

static void
writerGroupTypeDestructor(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *typeId, void *typeContext,
                          const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "WriterGroup destructor called!");
    UA_NodeId intervalNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishingInterval"),
                            UA_NS0ID(HASPROPERTY), *nodeId);

    UA_NodeId statusNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), *nodeId);
    UA_NodeId stateNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusNode);

    UA_NodePropertyContext *ctx;
    getNodeContext(server, intervalNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&intervalNode))
        UA_free(ctx);

    getNodeContext(server, stateNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&stateNode))
        UA_free(ctx);
}

static void
readerGroupTypeDestructor(UA_Server *server,
                          const UA_NodeId *sessionId, void *sessionContext,
                          const UA_NodeId *typeId, void *typeContext,
                          const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_NodeId statusNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), *nodeId);
    UA_NodeId stateNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusNode);

    UA_NodePropertyContext *ctx;
    getNodeContext(server, stateNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&stateNode))
        UA_free(ctx);
}

static void
dataSetWriterTypeDestructor(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *typeId, void *typeContext,
                            const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "DataSetWriter destructor called!");

    UA_NodeId dataSetWriterIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetWriterId"),
                            UA_NS0ID(HASPROPERTY), *nodeId);
    UA_NodeId statusNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), *nodeId);
    UA_NodeId stateNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusNode);

    UA_NodePropertyContext *ctx;
    getNodeContext(server, dataSetWriterIdNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&dataSetWriterIdNode))
        UA_free(ctx);

    getNodeContext(server, stateNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&stateNode))
        UA_free(ctx);
}

static void
dataSetReaderTypeDestructor(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionContext,
                            const UA_NodeId *typeId, void *typeContext,
                            const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "DataSetReader destructor called!");
    UA_NodeId publisherIdNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublisherId"),
                            UA_NS0ID(HASPROPERTY), *nodeId);
    UA_NodeId statusNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "Status"),
                            UA_NS0ID(HASCOMPONENT), *nodeId);
    UA_NodeId stateNode =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "State"),
                            UA_NS0ID(HASCOMPONENT), statusNode);

    UA_NodePropertyContext *ctx;
    getNodeContext(server, publisherIdNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&publisherIdNode))
        UA_free(ctx);

    getNodeContext(server, stateNode, (void **)&ctx);
    if(!UA_NodeId_isNull(&stateNode))
        UA_free(ctx);
}

static void
publishedDataItemsTypeDestructor(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *typeId, void *typeContext,
                                 const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "PublishedDataItems destructor called!");
    void *childContext;
    UA_NodeId node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "PublishedData"),
                                         UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_isNull(&node))
        UA_free(childContext);

    node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "ConfigurationVersion"),
                               UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_isNull(&node))
        UA_free(childContext);

    node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                               UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_isNull(&node))
        UA_free(childContext);
}

static void
subscribedDataSetTypeDestructor(UA_Server *server,
                                const UA_NodeId *sessionId, void *sessionContext,
                                const UA_NodeId *typeId, void *typeContext,
                                const UA_NodeId *nodeId, void **nodeContext) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_USERLAND,
                "Standalone SubscribedDataSet destructor called!");
    void *childContext;
    UA_NodeId node =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &node))
        UA_free(childContext);
    node = findSingleChildNode(server, UA_QUALIFIEDNAME(0, "IsConnected"),
                               UA_NS0ID(HASPROPERTY), *nodeId);
    getNodeContext(server, node, (void**)&childContext);
    if(!UA_NodeId_equal(&UA_NODEID_NULL , &node))
        UA_free(childContext);
}

/*************************************/
/*         PubSub configurator       */
/*************************************/

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG

/* Callback function that will be executed when the method "PubSub configurator
 * (replace config)" is called. */
static UA_StatusCode
UA_loadPubSubConfigMethodCallback(UA_Server *server,
                                  const UA_NodeId *sessionId, void *sessionContext,
                                  const UA_NodeId *methodId, void *methodContext,
                                  const UA_NodeId *objectId, void *objectContext,
                                  size_t inputSize, const UA_Variant *input,
                                  size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(inputSize == 1) {
        UA_ByteString *inputStr = (UA_ByteString*)input->data;
        return UA_Server_loadPubSubConfigFromByteString(server, *inputStr);
    } else if(inputSize > 1) {
        return UA_STATUSCODE_BADTOOMANYARGUMENTS;
    } else {
        return UA_STATUSCODE_BADARGUMENTSMISSING;
    }
}

/* Callback function that will be executed when the method "PubSub configurator
 *  (delete config)" is called. */
static UA_StatusCode
UA_deletePubSubConfigMethodCallback(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *methodId, void *methodContext,
                                    const UA_NodeId *objectId, void *objectContext,
                                    size_t inputSize, const UA_Variant *input,
                                    size_t outputSize, UA_Variant *output) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    if(psm)
        UA_PubSubManager_clear(psm);
    return UA_STATUSCODE_GOOD;
}

#endif

UA_StatusCode
initPubSubNS0(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_String profileArray[1];
    profileArray[0] = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");

    retVal |= writePubSubNs0VariableArray(server,
                                          UA_NS0ID(PUBLISHSUBSCRIBE_SUPPORTEDTRANSPORTPROFILES),
                                          profileArray, 1, &UA_TYPES[UA_TYPES_STRING]);

    if(server->config.pubSubConfig.enableInformationModelMethods) {
        /* Add missing references */
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(DATASETFOLDERTYPE_ADDDATASETFOLDER), true);
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), true);
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), true);
        retVal |= addRef(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBLISHEDDATASETS),
                         UA_NS0ID(HASCOMPONENT), UA_NS0ID(DATASETFOLDERTYPE_REMOVEDATASETFOLDER), true);

        /* Set method callbacks */
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_ADDCONNECTION), addPubSubConnectionAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_REMOVECONNECTION), removeConnectionAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(DATASETFOLDERTYPE_ADDDATASETFOLDER), addDataSetFolderAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(DATASETFOLDERTYPE_REMOVEDATASETFOLDER), removeDataSetFolderAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(DATASETFOLDERTYPE_ADDPUBLISHEDDATAITEMS), addPublishedDataItemsAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(DATASETFOLDERTYPE_REMOVEPUBLISHEDDATASET), removePublishedDataSetAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHEDDATAITEMSTYPE_ADDVARIABLES), addVariablesAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHEDDATAITEMSTYPE_REMOVEVARIABLES), removeVariablesAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBSUBCONNECTIONTYPE_ADDWRITERGROUP), addWriterGroupAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBSUBCONNECTIONTYPE_ADDREADERGROUP), addReaderGroupAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBSUBCONNECTIONTYPE_REMOVEGROUP), removeGroupAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(WRITERGROUPTYPE_ADDDATASETWRITER), addDataSetWriterAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(WRITERGROUPTYPE_REMOVEDATASETWRITER), removeDataSetWriterAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(READERGROUPTYPE_ADDDATASETREADER), addDataSetReaderAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(READERGROUPTYPE_REMOVEDATASETREADER), removeDataSetReaderAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_PUBSUBCONFIGURATION_RESERVEIDS), addReserveIdsAction);
#ifdef UA_ENABLE_PUBSUB_SKS
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_SETSECURITYKEYS), setSecurityKeysAction);
        retVal |= setMethodNode_callback(server, UA_NS0ID(PUBLISHSUBSCRIBE_GETSECURITYKEYS), getSecurityKeysAction);
#endif

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG
        /* Adds method node to server. This method is used to load binary files for
         * PubSub configuration and delete / replace old PubSub configurations. */
        UA_Argument inputArgument;
        UA_Argument_init(&inputArgument);
        inputArgument.description = UA_LOCALIZEDTEXT("", "PubSub config binfile");
        inputArgument.name = UA_STRING("BinFile");
        inputArgument.dataType = UA_TYPES[UA_TYPES_BYTESTRING].typeId;
        inputArgument.valueRank = UA_VALUERANK_SCALAR;

        UA_MethodAttributes configAttr = UA_MethodAttributes_default;
        configAttr.description = UA_LOCALIZEDTEXT("","Load binary configuration file");
        configAttr.displayName = UA_LOCALIZEDTEXT("","LoadPubSubConfigurationFile");
        configAttr.executable = true;
        configAttr.userExecutable = true;
        retVal |= addMethodNode(server, UA_NODEID_NULL,
                                UA_NS0ID(PUBLISHSUBSCRIBE), UA_NS0ID(HASORDEREDCOMPONENT),
                                UA_QUALIFIEDNAME(1, "PubSub configuration"),
                                &configAttr, UA_loadPubSubConfigMethodCallback,
                                1, &inputArgument, UA_NODEID_NULL, NULL,
                                0, NULL, UA_NODEID_NULL, NULL,
                                NULL, NULL);

        /* Adds method node to server. This method is used to delete the current
         * PubSub configuration. */
        configAttr.description = UA_LOCALIZEDTEXT("","Delete current PubSub configuration");
        configAttr.displayName = UA_LOCALIZEDTEXT("","DeletePubSubConfiguration");
        configAttr.executable = true;
        configAttr.userExecutable = true;
        retVal |= addMethodNode(server, UA_NODEID_NULL,
                                UA_NS0ID(PUBLISHSUBSCRIBE), UA_NS0ID(HASORDEREDCOMPONENT),
                                UA_QUALIFIEDNAME(1, "Delete PubSub config"),
                                &configAttr, UA_deletePubSubConfigMethodCallback,
                                0, NULL, UA_NODEID_NULL, NULL,
                                0, NULL, UA_NODEID_NULL, NULL,
                                NULL, NULL);
#endif
    } else {
        /* Remove methods */
        retVal |= deleteReference(server, UA_NS0ID(PUBLISHSUBSCRIBE),
                                  UA_NS0ID(HASCOMPONENT), true,
                                  UA_NS0EXID(PUBLISHSUBSCRIBE_ADDCONNECTION), false);
        retVal |= deleteReference(server, UA_NS0ID(PUBLISHSUBSCRIBE),
                                  UA_NS0ID(HASCOMPONENT), true,
                                  UA_NS0EXID(PUBLISHSUBSCRIBE_REMOVECONNECTION), false);
    }

    /* Set the object-type destructors */
    UA_NodeTypeLifecycle lifeCycle;
    lifeCycle.constructor = NULL;

    lifeCycle.destructor = connectionTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(PUBSUBCONNECTIONTYPE), lifeCycle);

    lifeCycle.destructor = writerGroupTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(WRITERGROUPTYPE), lifeCycle);

    lifeCycle.destructor = readerGroupTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(READERGROUPTYPE), lifeCycle);

    lifeCycle.destructor = dataSetWriterTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(DATASETWRITERTYPE), lifeCycle);

    lifeCycle.destructor = publishedDataItemsTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(PUBLISHEDDATAITEMSTYPE), lifeCycle);

    lifeCycle.destructor = dataSetReaderTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(DATASETREADERTYPE), lifeCycle);

    lifeCycle.destructor = subscribedDataSetTypeDestructor;
    retVal |= setNodeTypeLifecycle(server, UA_NS0ID(STANDALONESUBSCRIBEDDATASETTYPE), lifeCycle);

    return retVal;
}

UA_StatusCode
connectDataSetReaderToDataSet(UA_Server *server, UA_NodeId dsrId, UA_NodeId sdsId) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    UA_NodeId dataSetMetaDataOnDsrId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), dsrId);
    UA_NodeId subscribedDataSetOnDsrId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "SubscribedDataSet"),
                            UA_NS0ID(HASCOMPONENT), dsrId);
    UA_NodeId dataSetMetaDataOnSdsId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "DataSetMetaData"),
                            UA_NS0ID(HASPROPERTY), sdsId);
    UA_NodeId subscribedDataSetOnSdsId =
        findSingleChildNode(server, UA_QUALIFIEDNAME(0, "SubscribedDataSet"),
                            UA_NS0ID(HASCOMPONENT), sdsId);

    if(UA_NodeId_isNull(&dataSetMetaDataOnDsrId) ||
       UA_NodeId_isNull(&subscribedDataSetOnDsrId) ||
       UA_NodeId_isNull(&dataSetMetaDataOnSdsId) ||
       UA_NodeId_isNull(&subscribedDataSetOnSdsId))
        return UA_STATUSCODE_BADNOTFOUND;

    UA_NODESTORE_REMOVE(server, &dataSetMetaDataOnDsrId);
    UA_NODESTORE_REMOVE(server, &subscribedDataSetOnDsrId);

    retVal |= addRef(server, dsrId, UA_NS0ID(HASPROPERTY),
                     UA_NODEID_NUMERIC(dataSetMetaDataOnSdsId.namespaceIndex,
                                       dataSetMetaDataOnSdsId.identifier.numeric), true);
    retVal |= addRef(server, dsrId, UA_NS0ID(HASPROPERTY),
                     UA_NODEID_NUMERIC(subscribedDataSetOnSdsId.namespaceIndex,
                                       subscribedDataSetOnSdsId.identifier.numeric), true);

    return retVal;
}

#endif /* UA_ENABLE_PUBSUB_INFORMATIONMODEL */
