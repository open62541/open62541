/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 */

#include <open62541/server_pubsub.h>
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
#include "ua_types_encoding_binary.h"
#endif

#define UA_MAX_STACKBUF 512 /* Max size of network messages on the stack */

/* Forward declaration */
static void
UA_WriterGroup_clear(UA_Server *server, UA_WriterGroup *writerGroup);
static void
UA_DataSetField_clear(UA_DataSetField *field);
static UA_StatusCode
generateNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                       UA_ExtensionObject *messageSettings,
                       UA_ExtensionObject *transportSettings,
                       UA_NetworkMessage *networkMessage);
static UA_StatusCode
UA_DataSetWriter_generateDataSetMessage(UA_Server *server, UA_DataSetMessage *dataSetMessage,
                                        UA_DataSetWriter *dataSetWriter);

/**********************************************/
/*               Connection                   */
/**********************************************/

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src,
                               UA_PubSubConnectionConfig *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PubSubConnectionConfig));
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_Variant_copy(&src->address, &dst->address);
    res |= UA_String_copy(&src->transportProfileUri, &dst->transportProfileUri);
    res |= UA_Variant_copy(&src->connectionTransportSettings, &dst->connectionTransportSettings);
    if(src->connectionPropertiesSize > 0) {
        dst->connectionProperties = (UA_KeyValuePair *)
            UA_calloc(src->connectionPropertiesSize, sizeof(UA_KeyValuePair));
        if(!dst->connectionProperties) {
            UA_PubSubConnectionConfig_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for(size_t i = 0; i < src->connectionPropertiesSize; i++){
            res |= UA_QualifiedName_copy(&src->connectionProperties[i].key,
                                            &dst->connectionProperties[i].key);
            res |= UA_Variant_copy(&src->connectionProperties[i].value,
                                      &dst->connectionProperties[i].value);
        }
    }
    if(res != UA_STATUSCODE_GOOD)
        UA_PubSubConnectionConfig_clear(dst);
    return res;
}

UA_StatusCode
UA_Server_getPubSubConnectionConfig(UA_Server *server, const UA_NodeId connection,
                                    UA_PubSubConnectionConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubConnection *currentPubSubConnection =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!currentPubSubConnection)
        return UA_STATUSCODE_BADNOTFOUND;

    return UA_PubSubConnectionConfig_copy(currentPubSubConnection->config, config);
}

UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_Server *server, UA_NodeId connectionIdentifier) {
    UA_PubSubConnection *pubSubConnection;
    TAILQ_FOREACH(pubSubConnection, &server->pubSubManager.connections, listEntry){
        if(UA_NodeId_equal(&connectionIdentifier, &pubSubConnection->identifier))
            break;
    }
    return pubSubConnection;
}

void
UA_PubSubConnectionConfig_clear(UA_PubSubConnectionConfig *connectionConfig) {
    UA_String_clear(&connectionConfig->name);
    UA_String_clear(&connectionConfig->transportProfileUri);
    UA_Variant_clear(&connectionConfig->connectionTransportSettings);
    UA_Variant_clear(&connectionConfig->address);
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++){
        UA_QualifiedName_clear(&connectionConfig->connectionProperties[i].key);
        UA_Variant_clear(&connectionConfig->connectionProperties[i].value);
    }
    UA_free(connectionConfig->connectionProperties);
}

void
UA_PubSubConnection_clear(UA_Server *server, UA_PubSubConnection *connection) {
    /* Remove WriterGroups */
    UA_WriterGroup *writerGroup, *tmpWriterGroup;
    LIST_FOREACH_SAFE(writerGroup, &connection->writerGroups, listEntry, tmpWriterGroup)
        UA_Server_removeWriterGroup(server, writerGroup->identifier);

    /* Remove ReaderGroups */
    UA_ReaderGroup *readerGroups, *tmpReaderGroup;
    LIST_FOREACH_SAFE(readerGroups, &connection->readerGroups, listEntry, tmpReaderGroup)
        UA_Server_removeReaderGroup(server, readerGroups->identifier);

    UA_NodeId_clear(&connection->identifier);
    if(connection->channel)
        connection->channel->close(connection->channel);

    UA_PubSubConnectionConfig_clear(connection->config);
    UA_free(connection->config);
}

UA_StatusCode
UA_PubSubConnection_regist(UA_Server *server, UA_NodeId *connectionIdentifier) {
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, *connectionIdentifier);
    if(!connection)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_StatusCode retval = connection->channel->regist(connection->channel, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "register channel failed: 0x%" PRIx32 "!", retval);
    }
    return retval;
}

UA_StatusCode
UA_Server_addWriterGroup(UA_Server *server, const UA_NodeId connection,
                         const UA_WriterGroupConfig *writerGroupConfig,
                         UA_NodeId *writerGroupIdentifier) {
    if(!writerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    //search the connection by the given connectionIdentifier
    UA_PubSubConnection *currentConnectionContext =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!currentConnectionContext)
        return UA_STATUSCODE_BADNOTFOUND;

    if(currentConnectionContext->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding WriterGroup failed. PubSubConnection is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Validate messageSettings type */
    if (writerGroupConfig->messageSettings.content.decoded.type) {
        if (writerGroupConfig->encodingMimeType == UA_PUBSUB_ENCODING_JSON &&
                (writerGroupConfig->messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED
                || writerGroupConfig->messageSettings.content.decoded.type->typeIndex != UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE) ) {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }

        if (writerGroupConfig->encodingMimeType == UA_PUBSUB_ENCODING_UADP &&
                (writerGroupConfig->messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED
                        || writerGroupConfig->messageSettings.content.decoded.type->typeIndex != UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE) ) {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }

    //allocate memory for new WriterGroup
    UA_WriterGroup *newWriterGroup = (UA_WriterGroup *) UA_calloc(1, sizeof(UA_WriterGroup));
    if(!newWriterGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newWriterGroup->componentType = UA_PUBSUB_COMPONENT_WRITERGROUP;
    newWriterGroup->linkedConnection = currentConnectionContext;
    UA_PubSubManager_generateUniqueNodeId(server, &newWriterGroup->identifier);
    if(writerGroupIdentifier)
        UA_NodeId_copy(&newWriterGroup->identifier, writerGroupIdentifier);

    //deep copy of the config
    UA_WriterGroupConfig tmpWriterGroupConfig;
    UA_StatusCode retVal = UA_WriterGroupConfig_copy(writerGroupConfig, &tmpWriterGroupConfig);

    if(!tmpWriterGroupConfig.messageSettings.content.decoded.type) {
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        tmpWriterGroupConfig.messageSettings.content.decoded.data = wgm;
        tmpWriterGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        tmpWriterGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    }
    newWriterGroup->config = tmpWriterGroupConfig;
    LIST_INSERT_HEAD(&currentConnectionContext->writerGroups, newWriterGroup, listEntry);
    currentConnectionContext->writerGroupsSize++;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addWriterGroupRepresentation(server, newWriterGroup);
#endif
    return retVal;
}

UA_StatusCode
UA_Server_removeWriterGroup(UA_Server *server, const UA_NodeId writerGroup) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    if(wg->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Delete WriterGroup failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PubSubConnection *connection = wg->linkedConnection;
    if(!connection)
        return UA_STATUSCODE_BADNOTFOUND;

    if(connection->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Delete WriterGroup failed. PubSubConnection is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }
    if(wg->state == UA_PUBSUBSTATE_OPERATIONAL){
        //unregister the publish callback
        if(wg->config.pubsubManagerCallback.removeCustomCallback)
            wg->config.pubsubManagerCallback.removeCustomCallback(server, wg->identifier, wg->publishCallbackId);
        else
            UA_PubSubManager_removeRepeatedPubSubCallback(server, wg->publishCallbackId);

    }

    connection->writerGroupsSize--;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeGroupRepresentation(server, wg);
#endif

    UA_WriterGroup_clear(server, wg);
    LIST_REMOVE(wg, listEntry);
    UA_free(wg);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_freezeWriterGroupConfiguration(UA_Server *server, const UA_NodeId writerGroup) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    //PubSubConnection freezeCounter++
    UA_PubSubConnection *pubSubConnection =  wg->linkedConnection;
    pubSubConnection->configurationFreezeCounter++;
    pubSubConnection->configurationFrozen = UA_TRUE;
    //WriterGroup freeze
    wg->configurationFrozen = UA_TRUE;
    //DataSetWriter freeze
    UA_DataSetWriter *dataSetWriter;
    LIST_FOREACH(dataSetWriter, &wg->writers, listEntry){
        dataSetWriter->configurationFrozen = UA_TRUE;
        //PublishedDataSet freezeCounter++
        UA_PublishedDataSet *publishedDataSet =
            UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
        publishedDataSet->configurationFreezeCounter++;
        publishedDataSet->configurationFrozen = UA_TRUE;
        //DataSetFields freeze
        UA_DataSetField *dataSetField;
        TAILQ_FOREACH(dataSetField, &publishedDataSet->fields, listEntry){
            dataSetField->configurationFrozen = UA_TRUE;
        }
    }

    if(wg->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE){
        size_t dsmCount = 0;
        if(wg->config.encodingMimeType != UA_PUBSUB_ENCODING_UADP) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub-RT configuration fail: Non-RT capable encoding.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }
        //TODO Clarify: should we only allow = maxEncapsulatedDataSetMessageCount == 1 with RT?
        //TODO Clarify: Behaviour if the finale size is more than MTU
        /* Generate data set messages  */
        UA_STACKARRAY(UA_UInt16, dsWriterIds, wg->writersCount);
        UA_STACKARRAY(UA_DataSetMessage, dsmStore, wg->writersCount);
        UA_DataSetWriter *dsw;
        LIST_FOREACH(dsw, &wg->writers, listEntry) {
            /* Find the dataset */
            UA_PublishedDataSet *pds =
                    UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
            if(!pds) {
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub Publish: PublishedDataSet not found");
                continue;
            }
            if(pds->promotedFieldsCount > 0) {
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub-RT configuration fail: PDS contains promoted fields.");
                return UA_STATUSCODE_BADNOTSUPPORTED;
            }
            UA_DataSetField *dsf;
            TAILQ_FOREACH(dsf, &pds->fields, listEntry){
                const UA_VariableNode *rtNode = (const UA_VariableNode *) UA_NODESTORE_GET(server, &dsf->config.field.variable.publishParameters.publishedVariable);
                if(rtNode != NULL && rtNode->valueBackend.backendType != UA_VALUEBACKENDTYPE_EXTERNAL){
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "PubSub-RT configuration fail: PDS contains field without external data source.");
                    UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
                    return UA_STATUSCODE_BADNOTSUPPORTED;
                }
                UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
                if((UA_NodeId_equal(&dsf->fieldMetaData.dataType, &UA_TYPES[UA_TYPES_STRING].typeId) ||
                    UA_NodeId_equal(&dsf->fieldMetaData.dataType,
                                    &UA_TYPES[UA_TYPES_BYTESTRING].typeId)) &&
                   dsf->fieldMetaData.maxStringLength == 0) {
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "PubSub-RT configuration fail: "
                                   "PDS contains String/ByteString with dynamic length.");
                    return UA_STATUSCODE_BADNOTSUPPORTED;
                } else if(!UA_DataType_isNumeric(UA_findDataType(&dsf->fieldMetaData.dataType))){
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "PubSub-RT configuration fail: "
                                   "PDS contains variable with dynamic size.");
                    return UA_STATUSCODE_BADNOTSUPPORTED;
                }
            }
            /* Generate the DSM */
            UA_StatusCode res =
                    UA_DataSetWriter_generateDataSetMessage(server, &dsmStore[dsmCount], dsw);
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub RT Offset calculation: DataSetMessage buffering failed");
                continue;
            }
            dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
            dsmCount++;
        }
        UA_NetworkMessage networkMessage;
        memset(&networkMessage, 0, sizeof(networkMessage));
        UA_StatusCode  res =
            generateNetworkMessage(pubSubConnection, wg, dsmStore, dsWriterIds, (UA_Byte) dsmCount,
                                   &wg->config.messageSettings, &wg->config.transportSettings,
                                   &networkMessage);
        if(res != UA_STATUSCODE_GOOD)
        {
            return UA_STATUSCODE_BADINTERNALERROR;
        }

        memset(&wg->bufferedMessage, 0, sizeof(UA_NetworkMessageOffsetBuffer));
        UA_NetworkMessage_calcSizeBinary(&networkMessage, &wg->bufferedMessage);
        /* Allocate the buffer. Allocate on the stack if the buffer is small. */
        UA_ByteString buf;
        size_t msgSize = UA_NetworkMessage_calcSizeBinary(&networkMessage, NULL);
        res = UA_ByteString_allocBuffer(&buf, msgSize);
        if(res != UA_STATUSCODE_GOOD)
        {
            UA_free(networkMessage.payload.dataSetPayload.sizes);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        wg->bufferedMessage.buffer = buf;
        const UA_Byte *bufEnd = &wg->bufferedMessage.buffer.data[wg->bufferedMessage.buffer.length];
        UA_Byte *bufPos = wg->bufferedMessage.buffer.data;
        UA_NetworkMessage_encodeBinary(&networkMessage, &bufPos, bufEnd);
        
        UA_free(networkMessage.payload.dataSetPayload.sizes);
        /* Clean up DSM */
        for(size_t i = 0; i < dsmCount; i++){
            UA_free(dsmStore[i].data.keyFrameData.dataSetFields);
#ifdef UA_ENABLE_JSON_ENCODING
            UA_free(dsmStore[i].data.keyFrameData.fieldNames);
#endif
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_unfreezeWriterGroupConfiguration(UA_Server *server, const UA_NodeId writerGroup){
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;
    //if(wg->config.rtLevel == UA_PUBSUB_RT_NONE){
    //    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
    //                   "PubSub configuration freeze without RT configuration has no effect.");
    //    return UA_STATUSCODE_BADCONFIGURATIONERROR;
    //}
    //PubSubConnection freezeCounter--
    UA_PubSubConnection *pubSubConnection =  wg->linkedConnection;
    pubSubConnection->configurationFreezeCounter--;
    if(pubSubConnection->configurationFreezeCounter == 0){
        pubSubConnection->configurationFrozen = UA_FALSE;
    }
    //WriterGroup unfreeze
    wg->configurationFrozen = UA_FALSE;
    //DataSetWriter unfreeze
    UA_DataSetWriter *dataSetWriter;
    LIST_FOREACH(dataSetWriter, &wg->writers, listEntry) {
        UA_PublishedDataSet *publishedDataSet =
            UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
        //PublishedDataSet freezeCounter--
        publishedDataSet->configurationFreezeCounter--;
        if(publishedDataSet->configurationFreezeCounter == 0){
            publishedDataSet->configurationFrozen = UA_FALSE;
            UA_DataSetField *dataSetField;
            TAILQ_FOREACH(dataSetField, &publishedDataSet->fields, listEntry){
                dataSetField->configurationFrozen = UA_FALSE;
            }
        }
        dataSetWriter->configurationFrozen = UA_FALSE;
    }
    if(wg->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE)
        UA_ByteString_clear(&wg->bufferedMessage.buffer);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UA_EXPORT
UA_Server_setWriterGroupOperational(UA_Server *server, const UA_NodeId writerGroup){
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_OPERATIONAL, wg);
}

UA_StatusCode UA_EXPORT
UA_Server_setWriterGroupDisabled(UA_Server *server, const UA_NodeId writerGroup){
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_DISABLED, wg);
}

/**********************************************/
/*               PublishedDataSet             */
/**********************************************/
UA_StatusCode
UA_PublishedDataSetConfig_copy(const UA_PublishedDataSetConfig *src,
                               UA_PublishedDataSetConfig *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PublishedDataSetConfig));
    res |= UA_String_copy(&src->name, &dst->name);
    switch(src->publishedDataSetType){
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
            //no additional items
            break;

        case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
            if(src->config.itemsTemplate.variablesToAddSize > 0){
                dst->config.itemsTemplate.variablesToAdd = (UA_PublishedVariableDataType *)
                    UA_calloc(src->config.itemsTemplate.variablesToAddSize,
                              sizeof(UA_PublishedVariableDataType));
                if(!dst->config.itemsTemplate.variablesToAdd) {
                    res = UA_STATUSCODE_BADOUTOFMEMORY;
                    break;
                }
                dst->config.itemsTemplate.variablesToAddSize =
                    src->config.itemsTemplate.variablesToAddSize;
            }

            for(size_t i = 0; i < src->config.itemsTemplate.variablesToAddSize; i++){
                res |= UA_PublishedVariableDataType_copy(&src->config.itemsTemplate.variablesToAdd[i],
                                                         &dst->config.itemsTemplate.variablesToAdd[i]);
            }
            res |= UA_DataSetMetaDataType_copy(&src->config.itemsTemplate.metaData,
                                               &dst->config.itemsTemplate.metaData);
            break;

        default:
            res = UA_STATUSCODE_BADINVALIDARGUMENT;
            break;
    }

    if(res != UA_STATUSCODE_GOOD)
        UA_PublishedDataSetConfig_clear(dst);
    return res;
}

UA_StatusCode
UA_Server_getPublishedDataSetConfig(UA_Server *server, const UA_NodeId pds,
                                    UA_PublishedDataSetConfig *config){
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PublishedDataSet *currentPublishedDataSet = UA_PublishedDataSet_findPDSbyId(server, pds);
    if(!currentPublishedDataSet)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_PublishedDataSetConfig tmpPublishedDataSetConfig;
    //deep copy of the actual config
    UA_PublishedDataSetConfig_copy(&currentPublishedDataSet->config, &tmpPublishedDataSetConfig);
    *config = tmpPublishedDataSetConfig;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getPublishedDataSetMetaData(UA_Server *server, const UA_NodeId pds,
                                      UA_DataSetMetaDataType *metaData) {
    if(!metaData)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PublishedDataSet *currentPublishedDataSet = UA_PublishedDataSet_findPDSbyId(server, pds);
    if(!currentPublishedDataSet)
        return UA_STATUSCODE_BADNOTFOUND;

    return UA_DataSetMetaDataType_copy(&currentPublishedDataSet->dataSetMetaData, metaData);
}

UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PublishedDataSet *tmpPDS;
    TAILQ_FOREACH(tmpPDS, &server->pubSubManager.publishedDataSets, listEntry) {
        if(UA_NodeId_equal(&tmpPDS->identifier, &identifier))
            break;
    }
    return tmpPDS;
}

void
UA_PublishedDataSetConfig_clear(UA_PublishedDataSetConfig *pdsConfig) {
    //delete pds config
    UA_String_clear(&pdsConfig->name);
    switch (pdsConfig->publishedDataSetType){
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
            //no additional items
            break;
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
            if(pdsConfig->config.itemsTemplate.variablesToAddSize > 0){
                for(size_t i = 0; i < pdsConfig->config.itemsTemplate.variablesToAddSize; i++){
                    UA_PublishedVariableDataType_clear(&pdsConfig->config.itemsTemplate.variablesToAdd[i]);
                }
                UA_free(pdsConfig->config.itemsTemplate.variablesToAdd);
            }
            UA_DataSetMetaDataType_clear(&pdsConfig->config.itemsTemplate.metaData);
            break;
        default:
            break;
    }
}

void
UA_PublishedDataSet_clear(UA_Server *server, UA_PublishedDataSet *publishedDataSet) {
    UA_PublishedDataSetConfig_clear(&publishedDataSet->config);
    //delete PDS
    UA_DataSetField *field, *tmpField;
    TAILQ_FOREACH_SAFE(field, &publishedDataSet->fields, listEntry, tmpField) {
        UA_Server_removeDataSetField(server, field->identifier);
    }
    UA_DataSetMetaDataType_clear(&publishedDataSet->dataSetMetaData);
    UA_NodeId_clear(&publishedDataSet->identifier);
}

static UA_StatusCode
generateFieldMetaData(UA_Server *server, UA_DataSetField *field, UA_FieldMetaData *fieldMetaData) {
    switch (field->config.dataSetFieldType){
        case UA_PUBSUB_DATASETFIELD_VARIABLE:
            if(UA_String_copy(&field->config.field.variable.fieldNameAlias, &fieldMetaData->name) != UA_STATUSCODE_GOOD)
                return UA_STATUSCODE_BADINTERNALERROR;
            fieldMetaData->description = UA_LOCALIZEDTEXT_ALLOC("", "");
            fieldMetaData->dataSetFieldId = UA_GUID_NULL;

            //ToDo after freeze PR, the value source must be checked (other behavior for static value source)
            if(field->config.field.variable.rtValueSource.rtFieldSourceEnabled &&
               !field->config.field.variable.rtValueSource.rtInformationModelNode) {
                if((**(field->config.field.variable.rtValueSource.staticValueSource)).value.arrayDimensionsSize > 0) {
                    fieldMetaData->arrayDimensions = (UA_UInt32 *) UA_calloc(
                        (*(*(field->config.field.variable.rtValueSource.staticValueSource))).value.arrayDimensionsSize, sizeof(UA_UInt32));
                    if(fieldMetaData->arrayDimensions == NULL)
                        return UA_STATUSCODE_BADOUTOFMEMORY;
                    memcpy(fieldMetaData->arrayDimensions,
                           (*(field->config.field.variable.rtValueSource.staticValueSource))->value.arrayDimensions,
                            sizeof(UA_UInt32) * ((*(*(field->config.field.variable.rtValueSource.staticValueSource))).value.arrayDimensionsSize));
                }
                fieldMetaData->arrayDimensionsSize = (**(field->config.field.variable.rtValueSource.staticValueSource)).value.arrayDimensionsSize;
                if(UA_NodeId_copy(&(**field->config.field.variable.rtValueSource.staticValueSource).value.type->typeId,
                        &fieldMetaData->dataType) != UA_STATUSCODE_GOOD){
                    if(fieldMetaData->arrayDimensions){
                        UA_free(fieldMetaData->arrayDimensions);
                        return UA_STATUSCODE_BADINTERNALERROR;
                    }
                }
                fieldMetaData->properties = NULL;
                fieldMetaData->propertiesSize = 0;
                //TODO collect value rank for the static field source
                fieldMetaData->fieldFlags = UA_DATASETFIELDFLAGS_NONE;
                return UA_STATUSCODE_GOOD;
            }
            UA_Variant value;
            UA_Variant_init(&value);
            if(UA_Server_readArrayDimensions(server, field->config.field.variable.publishParameters.publishedVariable,
                                             &value) != UA_STATUSCODE_GOOD){
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub meta data generation. Reading ArrayDimension failed.");
            } else {
                if (value.arrayDimensionsSize > 0) {
                    fieldMetaData->arrayDimensions = (UA_UInt32 *) UA_calloc(value.arrayDimensionsSize, sizeof(UA_UInt32));
                    if(fieldMetaData->arrayDimensions == NULL)
                        return UA_STATUSCODE_BADOUTOFMEMORY;
                    memcpy(fieldMetaData->arrayDimensions, value.arrayDimensions, sizeof(UA_UInt32)*value.arrayDimensionsSize);
                }
                fieldMetaData->arrayDimensionsSize = value.arrayDimensionsSize;
            }
            if(UA_Server_readDataType(server, field->config.field.variable.publishParameters.publishedVariable,
                                      &fieldMetaData->dataType) != UA_STATUSCODE_GOOD){
                if(fieldMetaData->arrayDimensions){
                    UA_free(fieldMetaData->arrayDimensions);
                    return UA_STATUSCODE_BADINTERNALERROR;
                }
            }
            fieldMetaData->properties = NULL;
            fieldMetaData->propertiesSize = 0;
            UA_Int32 valueRank;
            if(UA_Server_readValueRank(server, field->config.field.variable.publishParameters.publishedVariable,
                                       &valueRank) != UA_STATUSCODE_GOOD){
                if(fieldMetaData->arrayDimensions){
                    UA_free(fieldMetaData->arrayDimensions);
                    return UA_STATUSCODE_BADINTERNALERROR;
                }
            }
            fieldMetaData->valueRank = valueRank;
            if(field->config.field.variable.promotedField){
                fieldMetaData->fieldFlags = UA_DATASETFIELDFLAGS_PROMOTEDFIELD;
            } else {
                fieldMetaData->fieldFlags = UA_DATASETFIELDFLAGS_NONE;
            }
            //TODO collect the following fields*/
            //fieldMetaData.builtInType
            //fieldMetaData.maxStringLength
            return UA_STATUSCODE_GOOD;
        case UA_PUBSUB_DATASETFIELD_EVENT:
            return UA_STATUSCODE_BADNOTSUPPORTED;
        default:
            return UA_STATUSCODE_BADNOTSUPPORTED;
    }
}

UA_DataSetFieldResult
UA_Server_addDataSetField(UA_Server *server, const UA_NodeId publishedDataSet,
                          const UA_DataSetFieldConfig *fieldConfig,
                          UA_NodeId *fieldIdentifier) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_DataSetFieldResult result = {UA_STATUSCODE_BADINVALIDARGUMENT, {0, 0}};
    if(!fieldConfig)
        return result;

    UA_PublishedDataSet *currentDataSet = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet);
    if(currentDataSet == NULL){
        result.result = UA_STATUSCODE_BADNOTFOUND;
        return result;
    }

    if(currentDataSet->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding DataSetField failed. PublishedDataSet is frozen.");
        result.result = UA_STATUSCODE_BADCONFIGURATIONERROR;
        return result;
    }

    if(currentDataSet->config.publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS){
        result.result = UA_STATUSCODE_BADNOTIMPLEMENTED;
        return result;
    }

    UA_DataSetField *newField = (UA_DataSetField *) UA_calloc(1, sizeof(UA_DataSetField));
    if(!newField){
        result.result = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    UA_DataSetFieldConfig tmpFieldConfig;
    retVal |= UA_DataSetFieldConfig_copy(fieldConfig, &tmpFieldConfig);
    newField->config = tmpFieldConfig;
    UA_PubSubManager_generateUniqueNodeId(server, &newField->identifier);
    if(fieldIdentifier != NULL){
        UA_NodeId_copy(&newField->identifier, fieldIdentifier);
    }
    newField->publishedDataSet = currentDataSet->identifier;
    //update major version of parent published data set
    currentDataSet->dataSetMetaData.configurationVersion.majorVersion = UA_PubSubConfigurationVersionTimeDifference();
    /* The order of DataSetFields should be the same in both creating and publishing.
     * So adding DataSetFields at the the end of the DataSets using TAILQ structure */
    if (currentDataSet->fieldSize != 0)
        TAILQ_INSERT_TAIL(&currentDataSet->fields, newField, listEntry);
    else
        TAILQ_INSERT_HEAD(&currentDataSet->fields, newField, listEntry);

    if(newField->config.field.variable.promotedField)
        currentDataSet->promotedFieldsCount++;
    currentDataSet->fieldSize++;

    //generate fieldMetadata within the DataSetMetaData
    currentDataSet->dataSetMetaData.fieldsSize++;
    UA_FieldMetaData *fieldMetaData = (UA_FieldMetaData *)
        UA_realloc(currentDataSet->dataSetMetaData.fields, currentDataSet->dataSetMetaData.fieldsSize *
            sizeof(UA_FieldMetaData));
    if(!fieldMetaData){
        result.result =  UA_STATUSCODE_BADOUTOFMEMORY;
        return result;
    }
    currentDataSet->dataSetMetaData.fields = fieldMetaData;

    UA_FieldMetaData_init(&fieldMetaData[currentDataSet->fieldSize-1]);
    if(generateFieldMetaData(server, newField, &fieldMetaData[currentDataSet->fieldSize-1]) != UA_STATUSCODE_GOOD){
        UA_Server_removeDataSetField(server, newField->identifier);
        result.result =  UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }
    UA_DataSetField *dsf;
    size_t counter = 0;
    TAILQ_FOREACH(dsf, &currentDataSet->fields, listEntry){
        dsf->fieldMetaData = fieldMetaData[counter++];
    }
    result.result = retVal;
    result.configurationVersion.majorVersion = currentDataSet->dataSetMetaData.configurationVersion.majorVersion;
    result.configurationVersion.minorVersion = currentDataSet->dataSetMetaData.configurationVersion.minorVersion;
    return result;
}

UA_DataSetFieldResult
UA_Server_removeDataSetField(UA_Server *server, const UA_NodeId dsf) {
    UA_DataSetField *currentField = UA_DataSetField_findDSFbyId(server, dsf);
    UA_DataSetFieldResult result = {UA_STATUSCODE_BADNOTFOUND, {0, 0}};
    if(!currentField)
        return result;

    if(currentField->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove DataSetField failed. DataSetField is frozen.");
        result.result = UA_STATUSCODE_BADCONFIGURATIONERROR;
        return result;
    }

    UA_PublishedDataSet *parentPublishedDataSet =
        UA_PublishedDataSet_findPDSbyId(server, currentField->publishedDataSet);
    if(!parentPublishedDataSet)
        return result;

    if(parentPublishedDataSet->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove DataSetField failed. PublishedDataSet is frozen.");
        result.result = UA_STATUSCODE_BADCONFIGURATIONERROR;
        return result;
    }

    parentPublishedDataSet->fieldSize--;
    if(currentField->config.field.variable.promotedField)
        parentPublishedDataSet->promotedFieldsCount--;
    /* update major version of PublishedDataSet */
    parentPublishedDataSet->dataSetMetaData.configurationVersion.majorVersion =
        UA_PubSubConfigurationVersionTimeDifference();

    currentField->fieldMetaData.arrayDimensions = NULL;
    currentField->fieldMetaData.properties = NULL;
    currentField->fieldMetaData.name = UA_STRING_NULL;
    currentField->fieldMetaData.description.locale = UA_STRING_NULL;
    currentField->fieldMetaData.description.text = UA_STRING_NULL;
    UA_DataSetField_clear(currentField);
    TAILQ_REMOVE(&parentPublishedDataSet->fields, currentField, listEntry);
    UA_free(currentField);

    result.result = UA_STATUSCODE_GOOD;
    //regenerate DataSetMetaData
    parentPublishedDataSet->dataSetMetaData.fieldsSize--;
    if(parentPublishedDataSet->dataSetMetaData.fieldsSize > 0){
        for(size_t i = 0; i < parentPublishedDataSet->dataSetMetaData.fieldsSize+1; i++) {
            UA_FieldMetaData_clear(&parentPublishedDataSet->dataSetMetaData.fields[i]);
        }
        UA_free(parentPublishedDataSet->dataSetMetaData.fields);
        UA_FieldMetaData *fieldMetaData = (UA_FieldMetaData *)
            UA_calloc(parentPublishedDataSet->dataSetMetaData.fieldsSize,
                      sizeof(UA_FieldMetaData));
        if(!fieldMetaData){
            result.result =  UA_STATUSCODE_BADOUTOFMEMORY;
            return result;
        }
        UA_DataSetField *tmpDSF;
        size_t counter = 0;
        TAILQ_FOREACH(tmpDSF, &parentPublishedDataSet->fields, listEntry){
            UA_FieldMetaData tmpFieldMetaData;
            UA_FieldMetaData_init(&tmpFieldMetaData);
            if(generateFieldMetaData(server, tmpDSF, &tmpFieldMetaData) != UA_STATUSCODE_GOOD){
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub MetaData generation failed!");
                //TODO how to ensure consistency if the metadata regeneration fails
                result.result = UA_STATUSCODE_BADINTERNALERROR;
            }
            fieldMetaData[counter++] = tmpFieldMetaData;
        }
        parentPublishedDataSet->dataSetMetaData.fields = fieldMetaData;
    } else {
        UA_FieldMetaData_delete(parentPublishedDataSet->dataSetMetaData.fields);
        parentPublishedDataSet->dataSetMetaData.fields = NULL;
    }

    result.configurationVersion.majorVersion = parentPublishedDataSet->dataSetMetaData.configurationVersion.majorVersion;
    result.configurationVersion.minorVersion = parentPublishedDataSet->dataSetMetaData.configurationVersion.minorVersion;
    return result;
}

/**********************************************/
/*               DataSetWriter                */
/**********************************************/

UA_StatusCode
UA_DataSetWriterConfig_copy(const UA_DataSetWriterConfig *src,
                            UA_DataSetWriterConfig *dst){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_DataSetWriterConfig));
    retVal |= UA_String_copy(&src->name, &dst->name);
    retVal |= UA_String_copy(&src->dataSetName, &dst->dataSetName);
    retVal |= UA_ExtensionObject_copy(&src->messageSettings, &dst->messageSettings);
    if (src->dataSetWriterPropertiesSize > 0) {
        dst->dataSetWriterProperties = (UA_KeyValuePair *)
            UA_calloc(src->dataSetWriterPropertiesSize, sizeof(UA_KeyValuePair));
        if(!dst->dataSetWriterProperties)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        for(size_t i = 0; i < src->dataSetWriterPropertiesSize; i++){
            retVal |= UA_KeyValuePair_copy(&src->dataSetWriterProperties[i], &dst->dataSetWriterProperties[i]);
        }
    }
    return retVal;
}

UA_StatusCode
UA_Server_getDataSetWriterConfig(UA_Server *server, const UA_NodeId dsw,
                                 UA_DataSetWriterConfig *config){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_DataSetWriter *currentDataSetWriter = UA_DataSetWriter_findDSWbyId(server, dsw);
    if(!currentDataSetWriter)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_DataSetWriterConfig tmpWriterConfig;
    //deep copy of the actual config
    retVal |= UA_DataSetWriterConfig_copy(&currentDataSetWriter->config, &tmpWriterConfig);
    *config = tmpWriterConfig;
    return retVal;
}

UA_StatusCode
UA_Server_DataSetWriter_getState(UA_Server *server, UA_NodeId dataSetWriterIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_DataSetWriter *currentDataSetWriter = UA_DataSetWriter_findDSWbyId(server, dataSetWriterIdentifier);
    if(currentDataSetWriter == NULL)
        return UA_STATUSCODE_BADNOTFOUND;
    *state = currentDataSetWriter->state;
    return UA_STATUSCODE_GOOD;
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
    for(size_t i = 0; i < pdsConfig->dataSetWriterPropertiesSize; i++){
        UA_KeyValuePair_clear(&pdsConfig->dataSetWriterProperties[i]);
    }
    UA_free(pdsConfig->dataSetWriterProperties);
    UA_ExtensionObject_clear(&pdsConfig->messageSettings);
}

static void
UA_DataSetWriter_clear(UA_Server *server, UA_DataSetWriter *dataSetWriter) {
    UA_DataSetWriterConfig_clear(&dataSetWriter->config);
    //delete DataSetWriter
    UA_NodeId_clear(&dataSetWriter->identifier);
    UA_NodeId_clear(&dataSetWriter->linkedWriterGroup);
    UA_NodeId_clear(&dataSetWriter->connectedDataSet);
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
    //delete lastSamples store
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
UA_DataSetWriter_setPubSubState(UA_Server *server, UA_PubSubState state, UA_DataSetWriter *dataSetWriter) {
    switch(state){
        case UA_PUBSUBSTATE_DISABLED:
            switch (dataSetWriter->state){
                case UA_PUBSUBSTATE_DISABLED:
                    return UA_STATUSCODE_GOOD;
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
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_PAUSED:
            switch (dataSetWriter->state){
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
                    return UA_STATUSCODE_GOOD;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
                    return UA_STATUSCODE_GOOD;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Received unknown PubSub state!");
            }
            break;
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                           "Received unknown PubSub state!");
    }
    return UA_STATUSCODE_GOOD;
}

/**********************************************/
/*               WriterGroup                  */
/**********************************************/

UA_StatusCode
UA_WriterGroupConfig_copy(const UA_WriterGroupConfig *src,
                          UA_WriterGroupConfig *dst){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_WriterGroupConfig));
    retVal |= UA_String_copy(&src->name, &dst->name);
    retVal |= UA_ExtensionObject_copy(&src->transportSettings, &dst->transportSettings);
    retVal |= UA_ExtensionObject_copy(&src->messageSettings, &dst->messageSettings);
    if (src->groupPropertiesSize > 0) {
        dst->groupProperties = (UA_KeyValuePair *) UA_calloc(src->groupPropertiesSize, sizeof(UA_KeyValuePair));
        if(!dst->groupProperties)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        for(size_t i = 0; i < src->groupPropertiesSize; i++){
            retVal |= UA_KeyValuePair_copy(&src->groupProperties[i], &dst->groupProperties[i]);
        }
    }
    return retVal;
}

UA_StatusCode
UA_Server_getWriterGroupConfig(UA_Server *server, const UA_NodeId writerGroup,
                               UA_WriterGroupConfig *config){
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_WriterGroup *currentWriterGroup = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!currentWriterGroup){
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_WriterGroupConfig tmpWriterGroupConfig;
    //deep copy of the actual config
    retVal |= UA_WriterGroupConfig_copy(&currentWriterGroup->config, &tmpWriterGroupConfig);
    *config = tmpWriterGroupConfig;
    return retVal;
}

UA_StatusCode
UA_Server_updateWriterGroupConfig(UA_Server *server, UA_NodeId writerGroupIdentifier,
                                  const UA_WriterGroupConfig *config){
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_WriterGroup *currentWriterGroup = UA_WriterGroup_findWGbyId(server, writerGroupIdentifier);
    if(!currentWriterGroup)
        return UA_STATUSCODE_BADNOTFOUND;

    if(currentWriterGroup->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Modify WriterGroup failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    //The update functionality will be extended during the next PubSub batches.
    //Currently is only a change of the publishing interval possible.
    if(currentWriterGroup->config.maxEncapsulatedDataSetMessageCount != config->maxEncapsulatedDataSetMessageCount){
        currentWriterGroup->config.maxEncapsulatedDataSetMessageCount = config->maxEncapsulatedDataSetMessageCount;
        if(currentWriterGroup->config.messageSettings.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "MaxEncapsulatedDataSetMessag need enabled 'PayloadHeader' within the message settings.");
        }
    }
    if(currentWriterGroup->config.publishingInterval != config->publishingInterval) {
        if(currentWriterGroup->config.rtLevel == UA_PUBSUB_RT_NONE && currentWriterGroup->state == UA_PUBSUBSTATE_OPERATIONAL){
            if(currentWriterGroup->config.pubsubManagerCallback.removeCustomCallback)
                currentWriterGroup->config.pubsubManagerCallback.removeCustomCallback(server, currentWriterGroup->identifier, currentWriterGroup->publishCallbackId);
            else
                UA_PubSubManager_removeRepeatedPubSubCallback(server, currentWriterGroup->publishCallbackId);

            currentWriterGroup->config.publishingInterval = config->publishingInterval;
            UA_WriterGroup_addPublishCallback(server, currentWriterGroup);
        } else {
            currentWriterGroup->config.publishingInterval = config->publishingInterval;
        }
    }
    if(currentWriterGroup->config.priority != config->priority) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "No or unsupported WriterGroup update.");
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_WriterGroup_getState(UA_Server *server, UA_NodeId writerGroupIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_WriterGroup *currentWriterGroup = UA_WriterGroup_findWGbyId(server, writerGroupIdentifier);
    if(currentWriterGroup == NULL)
        return UA_STATUSCODE_BADNOTFOUND;
    *state = currentWriterGroup->state;
    return UA_STATUSCODE_GOOD;
}

UA_WriterGroup *
UA_WriterGroup_findWGbyId(UA_Server *server, UA_NodeId identifier){
    UA_PubSubConnection *tmpConnection;
    TAILQ_FOREACH(tmpConnection, &server->pubSubManager.connections, listEntry){
        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &tmpConnection->writerGroups, listEntry) {
            if(UA_NodeId_equal(&identifier, &tmpWriterGroup->identifier)){
                return tmpWriterGroup;
            }
        }
    }
    return NULL;
}

void
UA_WriterGroupConfig_clear(UA_WriterGroupConfig *writerGroupConfig){
    //delete writerGroup config
    UA_String_clear(&writerGroupConfig->name);
    UA_ExtensionObject_clear(&writerGroupConfig->transportSettings);
    UA_ExtensionObject_clear(&writerGroupConfig->messageSettings);
    UA_Array_delete(writerGroupConfig->groupProperties,
                    writerGroupConfig->groupPropertiesSize,
                    &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    writerGroupConfig->groupProperties = NULL;
}

static void
UA_WriterGroup_clear(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_WriterGroupConfig_clear(&writerGroup->config);
    //delete WriterGroup
    //delete all writers. Therefore removeDataSetWriter is called from PublishedDataSet
    UA_DataSetWriter *dataSetWriter, *tmpDataSetWriter;
    LIST_FOREACH_SAFE(dataSetWriter, &writerGroup->writers, listEntry, tmpDataSetWriter){
        UA_Server_removeDataSetWriter(server, dataSetWriter->identifier);
    }
    if(writerGroup->bufferedMessage.offsetsSize > 0){
        for (size_t i = 0; i < writerGroup->bufferedMessage.offsetsSize; i++) {
            if(writerGroup->bufferedMessage.offsets[i].contentType == UA_PUBSUB_OFFSETTYPE_PAYLOAD_VARIANT){
                UA_DataValue_delete(writerGroup->bufferedMessage.offsets[i].offsetData.value.value);
            }
        }
        UA_ByteString_clear(&writerGroup->bufferedMessage.buffer);
        UA_free(writerGroup->bufferedMessage.offsets);
    }
    UA_NodeId_clear(&writerGroup->identifier);
}

UA_StatusCode
UA_WriterGroup_setPubSubState(UA_Server *server, UA_PubSubState state, UA_WriterGroup *writerGroup){
    UA_DataSetWriter *dataSetWriter;
    switch(state){
        case UA_PUBSUBSTATE_DISABLED:
            switch (writerGroup->state){
                case UA_PUBSUBSTATE_DISABLED:
                    return UA_STATUSCODE_GOOD;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    if(writerGroup->config.pubsubManagerCallback.removeCustomCallback)
                        writerGroup->config.pubsubManagerCallback.removeCustomCallback(server, writerGroup->identifier, writerGroup->publishCallbackId);
                    else
                        UA_PubSubManager_removeRepeatedPubSubCallback(server, writerGroup->publishCallbackId);

                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                        UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_DISABLED, dataSetWriter);
                    }
                    writerGroup->state = UA_PUBSUBSTATE_DISABLED;
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_PAUSED:
            switch (writerGroup->state){
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
                                   "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            switch (writerGroup->state){
                case UA_PUBSUBSTATE_DISABLED:
                    writerGroup->state = UA_PUBSUBSTATE_OPERATIONAL;
                    if(writerGroup->config.pubsubManagerCallback.removeCustomCallback)
                        writerGroup->config.pubsubManagerCallback.removeCustomCallback(server, writerGroup->identifier, writerGroup->publishCallbackId);
                    else
                        UA_PubSubManager_removeRepeatedPubSubCallback(server, writerGroup->publishCallbackId);

                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                        UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_OPERATIONAL, dataSetWriter);
                    }
                    UA_WriterGroup_addPublishCallback(server, writerGroup);
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    return UA_STATUSCODE_GOOD;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_ERROR: {
            switch (writerGroup->state){
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    UA_PubSubManager_removeRepeatedPubSubCallback(server, writerGroup->publishCallbackId);
                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                        UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dataSetWriter);
                    }      
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    return UA_STATUSCODE_GOOD;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                    "Received unknown PubSub state!");
            }
            writerGroup->state = UA_PUBSUBSTATE_ERROR;
            /* TODO: WIP - example usage of pubsubStateChangeCallback -> inform application about error state, reason param necessary */
            UA_ServerConfig *pConfig = UA_Server_getConfig(server);
            if (pConfig->pubsubConfiguration->pubsubStateChangeCallback != 0) {
                pConfig->pubsubConfiguration->pubsubStateChangeCallback(&writerGroup->identifier, UA_PUBSUBSTATE_ERROR, UA_STATUSCODE_BADINTERNALERROR);
            }
            break;
        }
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Received unknown PubSub state!");
    }
    return UA_STATUSCODE_GOOD;
}


UA_StatusCode
UA_Server_addDataSetWriter(UA_Server *server,
                           const UA_NodeId writerGroup, const UA_NodeId dataSet,
                           const UA_DataSetWriterConfig *dataSetWriterConfig,
                           UA_NodeId *writerIdentifier) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(!dataSetWriterConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PublishedDataSet *currentDataSetContext = UA_PublishedDataSet_findPDSbyId(server, dataSet);
    if(!currentDataSetContext)
        return UA_STATUSCODE_BADNOTFOUND;

    if(currentDataSetContext->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding DataSetWriter failed. PublishedDataSet is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    if(wg->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding DataSetWriter failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(wg->config.rtLevel != UA_PUBSUB_RT_NONE){
        UA_DataSetField *tmpDSF;
        TAILQ_FOREACH(tmpDSF, &currentDataSetContext->fields, listEntry){
            if(tmpDSF->config.field.variable.rtValueSource.rtFieldSourceEnabled != UA_TRUE &&
               tmpDSF->config.field.variable.rtValueSource.rtInformationModelNode != UA_TRUE){
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "Adding DataSetWriter failed. Fields in PDS are not RT capable.");
                return UA_STATUSCODE_BADCONFIGURATIONERROR;
            }
        }
    }

    UA_DataSetWriter *newDataSetWriter = (UA_DataSetWriter *) UA_calloc(1, sizeof(UA_DataSetWriter));
    if(!newDataSetWriter)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newDataSetWriter->componentType = UA_PUBSUB_COMPONENT_DATASETWRITER;

    if (wg->state == UA_PUBSUBSTATE_OPERATIONAL) {
        retVal = UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_OPERATIONAL, newDataSetWriter);
        if (retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Add DataSetWriter failed. setPubSubState failed.");
            return retVal;
        }
    }

    //copy the config into the new dataSetWriter
    UA_DataSetWriterConfig tmpDataSetWriterConfig;
    retVal |= UA_DataSetWriterConfig_copy(dataSetWriterConfig, &tmpDataSetWriterConfig);
    newDataSetWriter->config = tmpDataSetWriterConfig;
    //save the current version of the connected PublishedDataSet
    newDataSetWriter->connectedDataSetVersion = currentDataSetContext->dataSetMetaData.configurationVersion;

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
    //initialize the queue for the last values
    if (currentDataSetContext->fieldSize > 0) {
        newDataSetWriter->lastSamples = (UA_DataSetWriterSample * )
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

    //connect PublishedDataSet with DataSetWriter
    newDataSetWriter->connectedDataSet = currentDataSetContext->identifier;
    newDataSetWriter->linkedWriterGroup = wg->identifier;
    UA_PubSubManager_generateUniqueNodeId(server, &newDataSetWriter->identifier);
    if(writerIdentifier != NULL)
        UA_NodeId_copy(&newDataSetWriter->identifier, writerIdentifier);
    //add the new writer to the group
    LIST_INSERT_HEAD(&wg->writers, newDataSetWriter, listEntry);
    wg->writersCount++;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addDataSetWriterRepresentation(server, newDataSetWriter);
#endif
    return retVal;
}

UA_StatusCode
UA_Server_removeDataSetWriter(UA_Server *server, const UA_NodeId dsw){
    UA_DataSetWriter *dataSetWriter = UA_DataSetWriter_findDSWbyId(server, dsw);
    if(!dataSetWriter)
        return UA_STATUSCODE_BADNOTFOUND;

    if(dataSetWriter->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove DataSetWriter failed. DataSetWriter is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_WriterGroup *linkedWriterGroup =
        UA_WriterGroup_findWGbyId(server, dataSetWriter->linkedWriterGroup);
    if(!linkedWriterGroup)
        return UA_STATUSCODE_BADNOTFOUND;

    if(linkedWriterGroup->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove DataSetWriter failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PublishedDataSet *publishedDataSet =
        UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
    if(!publishedDataSet)
        return UA_STATUSCODE_BADNOTFOUND;

    linkedWriterGroup->writersCount--;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeDataSetWriterRepresentation(server, dataSetWriter);
#endif

    //remove DataSetWriter from group
    UA_DataSetWriter_clear(server, dataSetWriter);
    LIST_REMOVE(dataSetWriter, listEntry);
    UA_free(dataSetWriter);
    return UA_STATUSCODE_GOOD;
}

/**********************************************/
/*                DataSetField                */
/**********************************************/

UA_StatusCode
UA_DataSetFieldConfig_copy(const UA_DataSetFieldConfig *src, UA_DataSetFieldConfig *dst){
    memcpy(dst, src, sizeof(UA_DataSetFieldConfig));
    if(src->dataSetFieldType == UA_PUBSUB_DATASETFIELD_VARIABLE) {
        UA_String_copy(&src->field.variable.fieldNameAlias, &dst->field.variable.fieldNameAlias);
        UA_PublishedVariableDataType_copy(&src->field.variable.publishParameters,
                                          &dst->field.variable.publishParameters);
    } else {
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_getDataSetFieldConfig(UA_Server *server, const UA_NodeId dsf,
                                UA_DataSetFieldConfig *config) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_DataSetField *currentDataSetField = UA_DataSetField_findDSFbyId(server, dsf);
    if(!currentDataSetField)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_DataSetFieldConfig tmpFieldConfig;
    //deep copy of the actual config
    retVal |= UA_DataSetFieldConfig_copy(&currentDataSetField->config, &tmpFieldConfig);
    *config = tmpFieldConfig;
    return retVal;
}

UA_DataSetField *
UA_DataSetField_findDSFbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PublishedDataSet *tmpPDS;
    TAILQ_FOREACH(tmpPDS, &server->pubSubManager.publishedDataSets, listEntry){
        UA_DataSetField *tmpField;
        TAILQ_FOREACH(tmpField, &tmpPDS->fields, listEntry){
            if(UA_NodeId_equal(&tmpField->identifier, &identifier)){
                return tmpField;
            }
        }
    }
    return NULL;
}

void
UA_DataSetFieldConfig_clear(UA_DataSetFieldConfig *dataSetFieldConfig){
    if(dataSetFieldConfig->dataSetFieldType == UA_PUBSUB_DATASETFIELD_VARIABLE){
        UA_String_clear(&dataSetFieldConfig->field.variable.fieldNameAlias);
        UA_PublishedVariableDataType_clear(&dataSetFieldConfig->field.variable.publishParameters);
    }
}

static void
UA_DataSetField_clear(UA_DataSetField *field) {
    UA_DataSetFieldConfig_clear(&field->config);
    //delete DataSetField
    UA_NodeId_clear(&field->identifier);
    UA_NodeId_clear(&field->publishedDataSet);
    UA_FieldMetaData_clear(&field->fieldMetaData);
}

/*********************************************************/
/*               PublishValues handling                  */
/*********************************************************/

/**
 * Compare two variants. Internally used for value change detection.
 *
 * @return true if the value has changed
 */
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
static UA_Boolean
valueChangedVariant(UA_Variant *oldValue, UA_Variant *newValue){
    if(! (oldValue && newValue))
        return false;

    UA_ByteString *oldValueEncoding = UA_ByteString_new(), *newValueEncoding = UA_ByteString_new();
    size_t oldValueEncodingSize, newValueEncodingSize;
    oldValueEncodingSize = UA_calcSizeBinary(oldValue, &UA_TYPES[UA_TYPES_VARIANT]);
    newValueEncodingSize = UA_calcSizeBinary(newValue, &UA_TYPES[UA_TYPES_VARIANT]);
    if((oldValueEncodingSize == 0) || (newValueEncodingSize == 0))
        return false;

    if(oldValueEncodingSize != newValueEncodingSize)
        return true;

    if(UA_ByteString_allocBuffer(oldValueEncoding, oldValueEncodingSize) != UA_STATUSCODE_GOOD)
        return false;

    if(UA_ByteString_allocBuffer(newValueEncoding, newValueEncodingSize) != UA_STATUSCODE_GOOD)
        return false;

    UA_Byte *bufPosOldValue = oldValueEncoding->data;
    const UA_Byte *bufEndOldValue = &oldValueEncoding->data[oldValueEncoding->length];
    UA_Byte *bufPosNewValue = newValueEncoding->data;
    const UA_Byte *bufEndNewValue = &newValueEncoding->data[newValueEncoding->length];
    if(UA_encodeBinary(oldValue, &UA_TYPES[UA_TYPES_VARIANT],
                       &bufPosOldValue, &bufEndOldValue, NULL, NULL) != UA_STATUSCODE_GOOD){
        return false;
    }
    if(UA_encodeBinary(newValue, &UA_TYPES[UA_TYPES_VARIANT],
                       &bufPosNewValue, &bufEndNewValue, NULL, NULL) != UA_STATUSCODE_GOOD){
        return false;
    }
    oldValueEncoding->length = (uintptr_t)bufPosOldValue - (uintptr_t)oldValueEncoding->data;
    newValueEncoding->length = (uintptr_t)bufPosNewValue - (uintptr_t)newValueEncoding->data;
    UA_Boolean compareResult = !UA_ByteString_equal(oldValueEncoding, newValueEncoding);
    UA_ByteString_delete(oldValueEncoding);
    UA_ByteString_delete(newValueEncoding);
    return compareResult;
}
#endif

/**
 * Obtain the latest value for a specific DataSetField. This method is currently
 * called inside the DataSetMessage generation process.
 */
static void
UA_PubSubDataSetField_sampleValue(UA_Server *server, UA_DataSetField *field,
                                  UA_DataValue *value) {
    /* Read the value */
    if(field->config.field.variable.rtValueSource.rtInformationModelNode) {
        const UA_VariableNode *rtNode = (const UA_VariableNode *) UA_NODESTORE_GET(server,
                          &field->config.field.variable.publishParameters.publishedVariable);
        *value = **rtNode->valueBackend.backend.external.value;
        value->value.storageType = UA_VARIANT_DATA_NODELETE;
        UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
    } else if(field->config.field.variable.rtValueSource.rtFieldSourceEnabled == UA_FALSE){
        UA_ReadValueId rvid;
        UA_ReadValueId_init(&rvid);
        rvid.nodeId = field->config.field.variable.publishParameters.publishedVariable;
        rvid.attributeId = field->config.field.variable.publishParameters.attributeId;
        rvid.indexRange = field->config.field.variable.publishParameters.indexRange;
        *value = UA_Server_read(server, &rvid, UA_TIMESTAMPSTORETURN_BOTH);
    } else {
        *value = **field->config.field.variable.rtValueSource.staticValueSource;
        value->value.storageType = UA_VARIANT_DATA_NODELETE;
    }
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
    if(!dataSetMessage->data.keyFrameData.dataSetFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;

#ifdef UA_ENABLE_JSON_ENCODING
    dataSetMessage->data.keyFrameData.fieldNames = (UA_String *)
        UA_Array_new(currentDataSet->fieldSize, &UA_TYPES[UA_TYPES_STRING]);
    if(!dataSetMessage->data.keyFrameData.fieldNames) {
        UA_DataSetMessage_free(dataSetMessage);
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
    memset(dataSetMessage, 0, sizeof(UA_DataSetMessage));
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
        if(valueChangedVariant(&dataSetWriter->lastSamples[counter].value.value, &value.value)) {
            /* increase fieldCount for current delta message */
            dataSetMessage->data.deltaFrameData.fieldCount++;
            dataSetWriter->lastSamples[counter].valueChanged = true;

            /* Update last stored sample */
            UA_DataValue_clear(&dataSetWriter->lastSamples[counter].value);
            dataSetWriter->lastSamples[counter].value = value;
        } else {
            UA_DataValue_clear(&value);
            dataSetWriter->lastSamples[counter].valueChanged = false;
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

/**
 * Generate a DataSetMessage for the given writer.
 *
 * @param dataSetWriter ptr to corresponding writer
 * @return ptr to generated DataSetMessage
 */
static UA_StatusCode
UA_DataSetWriter_generateDataSetMessage(UA_Server *server, UA_DataSetMessage *dataSetMessage,
                                        UA_DataSetWriter *dataSetWriter) {
    UA_PublishedDataSet *currentDataSet =
        UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
    if(!currentDataSet)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Reset the message */
    memset(dataSetMessage, 0, sizeof(UA_DataSetMessage));

    /* store messageType to switch between json or uadp (default) */
    UA_UInt16 messageType = UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE;
    UA_JsonDataSetWriterMessageDataType *jsonDataSetWriterMessageDataType = NULL;

    /* The configuration Flags are included
     * inside the std. defined UA_UadpDataSetWriterMessageDataType */
    UA_UadpDataSetWriterMessageDataType defaultUadpConfiguration;
    UA_UadpDataSetWriterMessageDataType *dataSetWriterMessageDataType = NULL;
    if((dataSetWriter->config.messageSettings.encoding == UA_EXTENSIONOBJECT_DECODED ||
        dataSetWriter->config.messageSettings.encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) &&
       (dataSetWriter->config.messageSettings.content.decoded.type ==
        &UA_TYPES[UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE])) {
        dataSetWriterMessageDataType = (UA_UadpDataSetWriterMessageDataType *)
            dataSetWriter->config.messageSettings.content.decoded.data;

        /* type is UADP */
        messageType = UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE;
    } else if((dataSetWriter->config.messageSettings.encoding == UA_EXTENSIONOBJECT_DECODED ||
               dataSetWriter->config.messageSettings.encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE) &&
              (dataSetWriter->config.messageSettings.content.decoded.type ==
               &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE])) {
        jsonDataSetWriterMessageDataType = (UA_JsonDataSetWriterMessageDataType *)
            dataSetWriter->config.messageSettings.content.decoded.data;

        /* type is JSON */
        messageType = UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE;
    } else {
        /* create default flag configuration if no
         * UadpDataSetWriterMessageDataType was passed in */
        memset(&defaultUadpConfiguration, 0, sizeof(UA_UadpDataSetWriterMessageDataType));
        defaultUadpConfiguration.dataSetMessageContentMask = (UA_UadpDataSetMessageContentMask)
            ((u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION |
             (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION);
        dataSetWriterMessageDataType = &defaultUadpConfiguration;
    }

    /* Sanity-test the configuration */
    if(dataSetWriterMessageDataType &&
       (dataSetWriterMessageDataType->networkMessageNumber != 0 ||
        dataSetWriterMessageDataType->dataSetOffset != 0 ||
        dataSetWriterMessageDataType->configuredSize != 0)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Static DSM configuration not supported. Using defaults");
        dataSetWriterMessageDataType->networkMessageNumber = 0;
        dataSetWriterMessageDataType->dataSetOffset = 0;
        dataSetWriterMessageDataType->configuredSize = 0;
    }

    /* The field encoding depends on the flags inside the writer config.
     * TODO: This can be moved to the encoding layer. */
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

    if(messageType == UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE) {
        /* Std: 'The DataSetMessageContentMask defines the flags for the content
         * of the DataSetMessage header.' */
        if((u64)dataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION) {
            dataSetMessage->header.configVersionMajorVersionEnabled = true;
            dataSetMessage->header.configVersionMajorVersion =
                currentDataSet->dataSetMetaData.configurationVersion.majorVersion;
        }
        if((u64)dataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_MINORVERSION) {
            dataSetMessage->header.configVersionMinorVersionEnabled = true;
            dataSetMessage->header.configVersionMinorVersion =
                currentDataSet->dataSetMetaData.configurationVersion.minorVersion;
        }

        if((u64)dataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
            dataSetMessage->header.dataSetMessageSequenceNrEnabled = true;
            dataSetMessage->header.dataSetMessageSequenceNr =
                dataSetWriter->actualDataSetMessageSequenceCount;
        }

        if((u64)dataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP) {
            dataSetMessage->header.timestampEnabled = true;
            dataSetMessage->header.timestamp = UA_DateTime_now();
        }
        /* TODO: Picoseconds resolution not supported atm */
        if((u64)dataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_PICOSECONDS) {
            dataSetMessage->header.picoSecondsIncluded = false;
        }

        /* TODO: Statuscode not supported yet */
        if((u64)dataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_UADPDATASETMESSAGECONTENTMASK_STATUS) {
            dataSetMessage->header.statusEnabled = false;
        }
    } else if(messageType == UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE) {
        if((u64)jsonDataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION) {
            dataSetMessage->header.configVersionMajorVersionEnabled = true;
            dataSetMessage->header.configVersionMajorVersion =
                currentDataSet->dataSetMetaData.configurationVersion.majorVersion;
        }
        if((u64)jsonDataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION) {
            dataSetMessage->header.configVersionMinorVersionEnabled = true;
            dataSetMessage->header.configVersionMinorVersion =
                currentDataSet->dataSetMetaData.configurationVersion.minorVersion;
        }

        if((u64)jsonDataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER) {
            dataSetMessage->header.dataSetMessageSequenceNrEnabled = true;
            dataSetMessage->header.dataSetMessageSequenceNr =
                dataSetWriter->actualDataSetMessageSequenceCount;
        }

        if((u64)jsonDataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_TIMESTAMP) {
            dataSetMessage->header.timestampEnabled = true;
            dataSetMessage->header.timestamp = UA_DateTime_now();
        }

        /* TODO: Statuscode not supported yet */
        if((u64)jsonDataSetWriterMessageDataType->dataSetMessageContentMask &
           (u64)UA_JSONDATASETMESSAGECONTENTMASK_STATUS) {
            dataSetMessage->header.statusEnabled = false;
        }
    }

    /* Set the sequence count. Automatically rolls over to zero */
    dataSetWriter->actualDataSetMessageSequenceCount++;

    /* JSON does not differ between deltaframes and keyframes, only keyframes are currently used. */
    if(messageType != UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE){
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

        /* Realloc pds dependent memory */
        dataSetWriter->lastSamplesCount = currentDataSet->fieldSize;
        UA_DataSetWriterSample *newSamplesArray = (UA_DataSetWriterSample * )
            UA_realloc(dataSetWriter->lastSamples,
                       sizeof(UA_DataSetWriterSample) * dataSetWriter->lastSamplesCount);
        if(!newSamplesArray)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dataSetWriter->lastSamples = newSamplesArray;
        memset(dataSetWriter->lastSamples, 0,
               sizeof(UA_DataSetWriterSample) * dataSetWriter->lastSamplesCount);

        dataSetWriter->connectedDataSetVersion = currentDataSet->dataSetMetaData.configurationVersion;
        UA_PubSubDataSetWriter_generateKeyFrameMessage(server, dataSetMessage, dataSetWriter);
        dataSetWriter->deltaFrameCounter = 0;
        return UA_STATUSCODE_GOOD;
    }

    /* The standard defines: if a PDS contains only one fields no delta messages
     * should be generated because they need more memory than a keyframe with 1
     * field. */
    if(currentDataSet->fieldSize > 1 && dataSetWriter->deltaFrameCounter > 0 &&
       dataSetWriter->deltaFrameCounter <= dataSetWriter->config.keyFrameCount) {
        UA_PubSubDataSetWriter_generateDeltaFrameMessage(server, dataSetMessage, dataSetWriter);
        dataSetWriter->deltaFrameCounter++;
        return UA_STATUSCODE_GOOD;
    }

    dataSetWriter->deltaFrameCounter = 1;
#endif
    }

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

    if(writerGroup->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE) {
        UA_StatusCode res =
            sendBufferedNetworkMessage(server, connection, &writerGroup->bufferedMessage,
                                       &writerGroup->config.transportSettings);
        if(res == UA_STATUSCODE_GOOD) {
            writerGroup->sequenceNumber++;
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
    UA_DataSetWriter *dsw;
    UA_STACKARRAY(UA_UInt16, dsWriterIds, writerGroup->writersCount);
    UA_STACKARRAY(UA_DataSetMessage, dsmStore, writerGroup->writersCount);
    LIST_FOREACH(dsw, &writerGroup->writers, listEntry) {
        if (dsw->state == UA_PUBSUBSTATE_OPERATIONAL) {
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
            UA_StatusCode res =
                UA_DataSetWriter_generateDataSetMessage(server, &dsmStore[dsmCount], dsw);
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "PubSub Publish: DataSetMessage creation failed");
                UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
                continue;
            }

            /* Send right away if there is only this DSM in a NM. If promoted fields
            * are contained in the PublishedDataSet, then this DSM must go into a
            * dedicated NM as well. */
            if(pds->promotedFieldsCount > 0 || maxDSM == 1) {
                if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP){
                    res = sendNetworkMessage(connection, writerGroup, &dsmStore[dsmCount],
                                            &dsw->config.dataSetWriterId, 1,
                                            &writerGroup->config.messageSettings,
                                            &writerGroup->config.transportSettings);
                } else if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON) {
                    res = sendNetworkMessageJson(connection, &dsmStore[dsmCount],
                                                &dsw->config.dataSetWriterId, 1,
                                                &writerGroup->config.transportSettings);
                }

                if(res != UA_STATUSCODE_GOOD) {
                    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                "PubSub Publish: Could not send a NetworkMessage");
                    UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
                }

                if(writerGroup->config.rtLevel == UA_PUBSUB_RT_DIRECT_VALUE_ACCESS) {
                    for(size_t i = 0; i < dsmStore[dsmCount].data.keyFrameData.fieldCount; ++i) {
                        dsmStore[dsmCount].data.keyFrameData.dataSetFields[i].value.data = NULL;
                    }
                }

                UA_DataSetMessage_free(&dsmStore[dsmCount]);
                continue;
            }

            dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
            dsmCount++;
        }
    }

    /* Send the NetworkMessages with batched DataSetMessages */
    size_t nmCount = (dsmCount / maxDSM) + ((dsmCount % maxDSM) == 0 ? 0 : 1);
    for(UA_UInt32 i = 0; i < nmCount; i++) {
        UA_Byte nmDsmCount = maxDSM;
        if(i == nmCount - 1  && (dsmCount % maxDSM))
            nmDsmCount = (UA_Byte)dsmCount % maxDSM;
        UA_StatusCode res3 = UA_STATUSCODE_GOOD;
        if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP){
            res3 = sendNetworkMessage(connection, writerGroup, &dsmStore[i * maxDSM],
                                      &dsWriterIds[i * maxDSM], nmDsmCount,
                                      &writerGroup->config.messageSettings,
                                      &writerGroup->config.transportSettings);
        } else if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON){
            res3 = sendNetworkMessageJson(connection, &dsmStore[i * maxDSM],
                                          &dsWriterIds[i * maxDSM], nmDsmCount,
                                          &writerGroup->config.transportSettings);
        }

        if(res3 != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Publish: Sending a NetworkMessage failed");
            LIST_FOREACH(dsw, &writerGroup->writers, listEntry) {
                if (dsWriterIds[i * maxDSM] == dsw->config.dataSetWriterId) {
                    if (UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw) != 
                            UA_STATUSCODE_GOOD) {
                        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Publish: Setting DataSetWriter state to error failed");
                    }
                }
            }
            continue;
        }

        writerGroup->sequenceNumber++;
    }

    /* Clean up DSM */
    for(size_t i = 0; i < dsmCount; i++)
        UA_DataSetMessage_free(&dsmStore[i]);
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
                                                                              &writerGroup->publishCallbackId);
    else
        retval |= UA_PubSubManager_addRepeatedCallback(server,
                                                       (UA_ServerCallback) UA_WriterGroup_publishCallback,
                                                       writerGroup, writerGroup->config.publishingInterval,
                                                       &writerGroup->publishCallbackId);

    if(retval == UA_STATUSCODE_GOOD)
        writerGroup->publishCallbackIsRegistered = true;

    /* Run once after creation */
    UA_WriterGroup_publishCallback(server, writerGroup);
    return retval;
}

#endif /* UA_ENABLE_PUBSUB */
