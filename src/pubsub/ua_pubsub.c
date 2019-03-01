/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
#include "ua_types_encoding_binary.h"
#endif

#define UA_MAX_STACKBUF 512 /* Max size of network messages on the stack */

/* Forward declaration */
static void
UA_WriterGroup_deleteMembers(UA_Server *server, UA_WriterGroup *writerGroup);
static void
UA_DataSetField_deleteMembers(UA_DataSetField *field);

/**********************************************/
/*               Connection                   */
/**********************************************/

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src,
                               UA_PubSubConnectionConfig *dst) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PubSubConnectionConfig));
    retVal |= UA_String_copy(&src->name, &dst->name);
    retVal |= UA_Variant_copy(&src->address, &dst->address);
    retVal |= UA_String_copy(&src->transportProfileUri, &dst->transportProfileUri);
    retVal |= UA_Variant_copy(&src->connectionTransportSettings, &dst->connectionTransportSettings);
    if(src->connectionPropertiesSize > 0){
        dst->connectionProperties = (UA_KeyValuePair *)
            UA_calloc(src->connectionPropertiesSize, sizeof(UA_KeyValuePair));
        if(!dst->connectionProperties){
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for(size_t i = 0; i < src->connectionPropertiesSize; i++){
            retVal |= UA_QualifiedName_copy(&src->connectionProperties[i].key,
                                            &dst->connectionProperties[i].key);
            retVal |= UA_Variant_copy(&src->connectionProperties[i].value,
                                      &dst->connectionProperties[i].value);
        }
    }
    return retVal;
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

    UA_PubSubConnectionConfig tmpPubSubConnectionConfig;
    //deep copy of the actual config
    UA_PubSubConnectionConfig_copy(currentPubSubConnection->config, &tmpPubSubConnectionConfig);
    *config = tmpPubSubConnectionConfig;
    return UA_STATUSCODE_GOOD;
}

UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_Server *server, UA_NodeId connectionIdentifier) {
    for(size_t i = 0; i < server->pubSubManager.connectionsSize; i++){
        if(UA_NodeId_equal(&connectionIdentifier, &server->pubSubManager.connections[i].identifier)){
            return &server->pubSubManager.connections[i];
        }
    }
    return NULL;
}

void
UA_PubSubConnectionConfig_deleteMembers(UA_PubSubConnectionConfig *connectionConfig) {
    UA_String_deleteMembers(&connectionConfig->name);
    UA_String_deleteMembers(&connectionConfig->transportProfileUri);
    UA_Variant_deleteMembers(&connectionConfig->connectionTransportSettings);
    UA_Variant_deleteMembers(&connectionConfig->address);
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++){
        UA_QualifiedName_deleteMembers(&connectionConfig->connectionProperties[i].key);
        UA_Variant_deleteMembers(&connectionConfig->connectionProperties[i].value);
    }
    UA_free(connectionConfig->connectionProperties);
}

void
UA_PubSubConnection_deleteMembers(UA_Server *server, UA_PubSubConnection *connection) {
    //delete connection config
    UA_PubSubConnectionConfig_deleteMembers(connection->config);
    //remove contained WriterGroups
    UA_WriterGroup *writerGroup, *tmpWriterGroup;
    LIST_FOREACH_SAFE(writerGroup, &connection->writerGroups, listEntry, tmpWriterGroup){
        UA_Server_removeWriterGroup(server, writerGroup->identifier);
    }
    UA_NodeId_deleteMembers(&connection->identifier);
    if(connection->channel){
        connection->channel->close(connection->channel);
    }
    UA_free(connection->config);
}

UA_StatusCode
UA_Server_addWriterGroup(UA_Server *server, const UA_NodeId connection,
                         const UA_WriterGroupConfig *writerGroupConfig,
                         UA_NodeId *writerGroupIdentifier) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(!writerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    //search the connection by the given connectionIdentifier
    UA_PubSubConnection *currentConnectionContext =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!currentConnectionContext)
        return UA_STATUSCODE_BADNOTFOUND;

    //allocate memory for new WriterGroup
    UA_WriterGroup *newWriterGroup = (UA_WriterGroup *) UA_calloc(1, sizeof(UA_WriterGroup));
    if (!newWriterGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newWriterGroup->linkedConnection = currentConnectionContext->identifier;
    UA_PubSubManager_generateUniqueNodeId(server, &newWriterGroup->identifier);
    if(writerGroupIdentifier){
        UA_NodeId_copy(&newWriterGroup->identifier, writerGroupIdentifier);
    }

    //deep copy of the config
    UA_WriterGroupConfig tmpWriterGroupConfig;
    retVal |= UA_WriterGroupConfig_copy(writerGroupConfig, &tmpWriterGroupConfig);

    if(!tmpWriterGroupConfig.messageSettings.content.decoded.type) {
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        tmpWriterGroupConfig.messageSettings.content.decoded.data = wgm;
        tmpWriterGroupConfig.messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        tmpWriterGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    }

    newWriterGroup->config = tmpWriterGroupConfig;
    retVal |= UA_WriterGroup_addPublishCallback(server, newWriterGroup);
    LIST_INSERT_HEAD(&currentConnectionContext->writerGroups, newWriterGroup, listEntry);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addWriterGroupRepresentation(server, newWriterGroup);
#endif
    return retVal;
}

UA_StatusCode
UA_Server_removeWriterGroup(UA_Server *server, const UA_NodeId writerGroup){
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, wg->linkedConnection);
    if(!connection)
        return UA_STATUSCODE_BADNOTFOUND;

    //unregister the publish callback
    UA_PubSubManager_removeRepeatedPubSubCallback(server, wg->publishCallbackId);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeGroupRepresentation(server, wg);
#endif

    UA_WriterGroup_deleteMembers(server, wg);
    LIST_REMOVE(wg, listEntry);
    UA_free(wg);
    return UA_STATUSCODE_GOOD;
}

/**********************************************/
/*               PublishedDataSet             */
/**********************************************/

UA_StatusCode
UA_PublishedDataSetConfig_copy(const UA_PublishedDataSetConfig *src,
                               UA_PublishedDataSetConfig *dst) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PublishedDataSetConfig));
    retVal |= UA_String_copy(&src->name, &dst->name);
    switch(src->publishedDataSetType){
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
            //no additional items
            break;
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
            if (src->config.itemsTemplate.variablesToAddSize > 0){
                dst->config.itemsTemplate.variablesToAdd = (UA_PublishedVariableDataType *) UA_calloc(
                        src->config.itemsTemplate.variablesToAddSize, sizeof(UA_PublishedVariableDataType));
            }
            for(size_t i = 0; i < src->config.itemsTemplate.variablesToAddSize; i++){
                retVal |= UA_PublishedVariableDataType_copy(&src->config.itemsTemplate.variablesToAdd[i],
                                                            &dst->config.itemsTemplate.variablesToAdd[i]);
            }
            retVal |= UA_DataSetMetaDataType_copy(&src->config.itemsTemplate.metaData,
                                                  &dst->config.itemsTemplate.metaData);
            break;
        default:
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return retVal;
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

UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyId(UA_Server *server, UA_NodeId identifier){
    for(size_t i = 0; i < server->pubSubManager.publishedDataSetsSize; i++){
        if(UA_NodeId_equal(&server->pubSubManager.publishedDataSets[i].identifier, &identifier)){
            return &server->pubSubManager.publishedDataSets[i];
        }
    }
    return NULL;
}

void
UA_PublishedDataSetConfig_deleteMembers(UA_PublishedDataSetConfig *pdsConfig){
    //delete pds config
    UA_String_deleteMembers(&pdsConfig->name);
    switch (pdsConfig->publishedDataSetType){
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
            //no additional items
            break;
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
            if (pdsConfig->config.itemsTemplate.variablesToAddSize > 0){
                for(size_t i = 0; i < pdsConfig->config.itemsTemplate.variablesToAddSize; i++){
                    UA_PublishedVariableDataType_deleteMembers(&pdsConfig->config.itemsTemplate.variablesToAdd[i]);
                }
                UA_free(pdsConfig->config.itemsTemplate.variablesToAdd);
            }
            UA_DataSetMetaDataType_deleteMembers(&pdsConfig->config.itemsTemplate.metaData);
            break;
        default:
            break;
    }
}

void
UA_PublishedDataSet_deleteMembers(UA_Server *server, UA_PublishedDataSet *publishedDataSet){
    UA_PublishedDataSetConfig_deleteMembers(&publishedDataSet->config);
    //delete PDS
    UA_DataSetMetaDataType_deleteMembers(&publishedDataSet->dataSetMetaData);
    UA_DataSetField *field, *tmpField;
    LIST_FOREACH_SAFE(field, &publishedDataSet->fields, listEntry, tmpField) {
        UA_Server_removeDataSetField(server, field->identifier);
    }
    UA_NodeId_deleteMembers(&publishedDataSet->identifier);
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
    LIST_INSERT_HEAD(&currentDataSet->fields, newField, listEntry);
    if(newField->config.field.variable.promotedField)
        currentDataSet->promotedFieldsCount++;
    currentDataSet->fieldSize++;
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

    UA_PublishedDataSet *parentPublishedDataSet =
        UA_PublishedDataSet_findPDSbyId(server, currentField->publishedDataSet);
    if(!parentPublishedDataSet)
        return result;

    parentPublishedDataSet->fieldSize--;
    if(currentField->config.field.variable.promotedField)
        parentPublishedDataSet->promotedFieldsCount--;
    
    /* update major version of PublishedDataSet */
    parentPublishedDataSet->dataSetMetaData.configurationVersion.majorVersion =
        UA_PubSubConfigurationVersionTimeDifference();

    UA_DataSetField_deleteMembers(currentField);
    LIST_REMOVE(currentField, listEntry);
    UA_free(currentField);

    result.result = UA_STATUSCODE_GOOD;
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
    dst->dataSetWriterProperties = (UA_KeyValuePair *)
        UA_calloc(src->dataSetWriterPropertiesSize, sizeof(UA_KeyValuePair));
    if(!dst->dataSetWriterProperties)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < src->dataSetWriterPropertiesSize; i++){
        retVal |= UA_KeyValuePair_copy(&src->dataSetWriterProperties[i], &dst->dataSetWriterProperties[i]);
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

UA_DataSetWriter *
UA_DataSetWriter_findDSWbyId(UA_Server *server, UA_NodeId identifier) {
    for(size_t i = 0; i < server->pubSubManager.connectionsSize; i++){
        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &server->pubSubManager.connections[i].writerGroups, listEntry){
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
UA_DataSetWriterConfig_deleteMembers(UA_DataSetWriterConfig *pdsConfig) {
    UA_String_deleteMembers(&pdsConfig->name);
    UA_String_deleteMembers(&pdsConfig->dataSetName);
    for(size_t i = 0; i < pdsConfig->dataSetWriterPropertiesSize; i++){
        UA_KeyValuePair_deleteMembers(&pdsConfig->dataSetWriterProperties[i]);
    }
    UA_free(pdsConfig->dataSetWriterProperties);
    UA_ExtensionObject_deleteMembers(&pdsConfig->messageSettings);
}

static void
UA_DataSetWriter_deleteMembers(UA_Server *server, UA_DataSetWriter *dataSetWriter) {
    UA_DataSetWriterConfig_deleteMembers(&dataSetWriter->config);
    //delete DataSetWriter
    UA_NodeId_deleteMembers(&dataSetWriter->identifier);
    UA_NodeId_deleteMembers(&dataSetWriter->linkedWriterGroup);
    UA_NodeId_deleteMembers(&dataSetWriter->connectedDataSet);
#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
    //delete lastSamples store
    for(size_t i = 0; i < dataSetWriter->lastSamplesCount; i++) {
        UA_DataValue_deleteMembers(&dataSetWriter->lastSamples[i].value);
    }
    UA_free(dataSetWriter->lastSamples);
    dataSetWriter->lastSamples = NULL;
    dataSetWriter->lastSamplesCount = 0;
#endif
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
    dst->groupProperties = (UA_KeyValuePair *) UA_calloc(src->groupPropertiesSize, sizeof(UA_KeyValuePair));
    if(!dst->groupProperties)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < src->groupPropertiesSize; i++){
        retVal |= UA_KeyValuePair_copy(&src->groupProperties[i], &dst->groupProperties[i]);
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
    //The update functionality will be extended during the next PubSub batches.
    //Currently is only a change of the publishing interval possible.
    if(currentWriterGroup->config.publishingInterval != config->publishingInterval) {
        UA_PubSubManager_removeRepeatedPubSubCallback(server, currentWriterGroup->publishCallbackId);
        currentWriterGroup->config.publishingInterval = config->publishingInterval;
        UA_WriterGroup_addPublishCallback(server, currentWriterGroup);
    } else if(currentWriterGroup->config.priority != config->priority) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "No or unsupported WriterGroup update.");
    }
    return UA_STATUSCODE_GOOD;
}

UA_WriterGroup *
UA_WriterGroup_findWGbyId(UA_Server *server, UA_NodeId identifier){
    for(size_t i = 0; i < server->pubSubManager.connectionsSize; i++){
        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &server->pubSubManager.connections[i].writerGroups, listEntry) {
            if(UA_NodeId_equal(&identifier, &tmpWriterGroup->identifier)){
                return tmpWriterGroup;
            }
        }
    }
    return NULL;
}

void
UA_WriterGroupConfig_deleteMembers(UA_WriterGroupConfig *writerGroupConfig){
    //delete writerGroup config
    UA_String_deleteMembers(&writerGroupConfig->name);
    UA_ExtensionObject_deleteMembers(&writerGroupConfig->transportSettings);
    UA_ExtensionObject_deleteMembers(&writerGroupConfig->messageSettings);
    for(size_t i = 0; i < writerGroupConfig->groupPropertiesSize; i++){
        UA_KeyValuePair_deleteMembers(&writerGroupConfig->groupProperties[i]);
    }
    UA_free(writerGroupConfig->groupProperties);
}

static void
UA_WriterGroup_deleteMembers(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_WriterGroupConfig_deleteMembers(&writerGroup->config);
    //delete WriterGroup
    //delete all writers. Therefore removeDataSetWriter is called from PublishedDataSet
    UA_DataSetWriter *dataSetWriter, *tmpDataSetWriter;
    LIST_FOREACH_SAFE(dataSetWriter, &writerGroup->writers, listEntry, tmpDataSetWriter){
        UA_Server_removeDataSetWriter(server, dataSetWriter->identifier);
    }
    UA_NodeId_deleteMembers(&writerGroup->linkedConnection);
    UA_NodeId_deleteMembers(&writerGroup->identifier);
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

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_DataSetWriter *newDataSetWriter = (UA_DataSetWriter *) UA_calloc(1, sizeof(UA_DataSetWriter));
    if(!newDataSetWriter)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    //copy the config into the new dataSetWriter
    UA_DataSetWriterConfig tmpDataSetWriterConfig;
    retVal |= UA_DataSetWriterConfig_copy(dataSetWriterConfig, &tmpDataSetWriterConfig);
    newDataSetWriter->config = tmpDataSetWriterConfig;
    //save the current version of the connected PublishedDataSet
    newDataSetWriter->connectedDataSetVersion = currentDataSetContext->dataSetMetaData.configurationVersion;

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
    //initialize the queue for the last values
    newDataSetWriter->lastSamples = (UA_DataSetWriterSample * )
        UA_calloc(currentDataSetContext->fieldSize, sizeof(UA_DataSetWriterSample));
    if(!newDataSetWriter->lastSamples) {
        UA_DataSetWriterConfig_deleteMembers(&newDataSetWriter->config);
        UA_free(newDataSetWriter);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    newDataSetWriter->lastSamplesCount = currentDataSetContext->fieldSize;
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

    UA_WriterGroup *linkedWriterGroup = UA_WriterGroup_findWGbyId(server, dataSetWriter->linkedWriterGroup);
    if(!linkedWriterGroup)
        return UA_STATUSCODE_BADNOTFOUND;

    linkedWriterGroup->writersCount--;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeDataSetWriterRepresentation(server, dataSetWriter);
#endif

    //remove DataSetWriter from group
    UA_DataSetWriter_deleteMembers(server, dataSetWriter);
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
    for(size_t i = 0; i < server->pubSubManager.publishedDataSetsSize; i++){
        UA_DataSetField *tmpField;
        LIST_FOREACH(tmpField, &server->pubSubManager.publishedDataSets[i].fields, listEntry){
            if(UA_NodeId_equal(&tmpField->identifier, &identifier)){
                return tmpField;
            }
        }
    }
    return NULL;
}

void
UA_DataSetFieldConfig_deleteMembers(UA_DataSetFieldConfig *dataSetFieldConfig){
    if(dataSetFieldConfig->dataSetFieldType == UA_PUBSUB_DATASETFIELD_VARIABLE){
        UA_String_deleteMembers(&dataSetFieldConfig->field.variable.fieldNameAlias);
        UA_PublishedVariableDataType_deleteMembers(&dataSetFieldConfig->field.variable.publishParameters);
    }
}

static void
UA_DataSetField_deleteMembers(UA_DataSetField *field) {
    UA_DataSetFieldConfig_deleteMembers(&field->config);
    //delete DataSetField
    UA_NodeId_deleteMembers(&field->identifier);
    UA_NodeId_deleteMembers(&field->publishedDataSet);
    UA_FieldMetaData_deleteMembers(&field->fieldMetaData);
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
    UA_ReadValueId rvid;
    UA_ReadValueId_init(&rvid);
    rvid.nodeId = field->config.field.variable.publishParameters.publishedVariable;
    rvid.attributeId = field->config.field.variable.publishParameters.attributeId;
    rvid.indexRange = field->config.field.variable.publishParameters.indexRange;
    *value = UA_Server_read(server, &rvid, UA_TIMESTAMPSTORETURN_BOTH);
}

static UA_StatusCode
UA_PubSubDataSetWriter_generateKeyFrameMessage(UA_Server *server, UA_DataSetMessage *dataSetMessage,
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
    /* json: insert fieldnames used as json keys */
       dataSetMessage->data.keyFrameData.fieldNames =
               (UA_String *)UA_Array_new(currentDataSet->fieldSize, &UA_TYPES[UA_TYPES_STRING]);
       if(!dataSetMessage->data.keyFrameData.fieldNames)
           return UA_STATUSCODE_BADOUTOFMEMORY;
#endif

    /* Loop over the fields */
    size_t counter = 0;
    UA_DataSetField *dsf;
    LIST_FOREACH(dsf, &currentDataSet->fields, listEntry) {

#ifdef UA_ENABLE_JSON_ENCODING
        /* json: store the fieldNameAlias*/
        UA_String_copy(&dsf->config.field.variable.fieldNameAlias,
              &dataSetMessage->data.keyFrameData.fieldNames[counter]);
#endif

        /* Sample the value */
        UA_DataValue *dfv = &dataSetMessage->data.keyFrameData.dataSetFields[counter];
        UA_PubSubDataSetField_sampleValue(server, dsf, dfv);

        /* Deactivate statuscode? */
        if(((u64)dataSetWriter->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
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
        UA_DataValue_deleteMembers(&dataSetWriter->lastSamples[counter].value);
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

    UA_DataSetField *dsf;
    size_t counter = 0;
    LIST_FOREACH(dsf, &currentDataSet->fields, listEntry) {
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
            UA_DataValue_deleteMembers(&dataSetWriter->lastSamples[counter].value);
            dataSetWriter->lastSamples[counter].value = value;
        } else {
            UA_DataValue_deleteMembers(&value);
            dataSetWriter->lastSamples[counter].valueChanged = false;
        }

        counter++;
    }

    /* Allocate DeltaFrameFields */
    UA_DataSetMessage_DeltaFrameField *deltaFields = (UA_DataSetMessage_DeltaFrameField *)
            UA_calloc(dataSetMessage->data.deltaFrameData.fieldCount, sizeof(UA_DataSetMessage_DeltaFrameField));
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
        if(((u64)dataSetWriter->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE) == 0)
            dff->fieldValue.hasStatus = false;

        /* Deactivate timestamps? */
        if(((u64)dataSetWriter->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP) == 0)
            dff->fieldValue.hasSourceTimestamp = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS) ==
           0)
            dff->fieldValue.hasServerPicoseconds = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_SERVERTIMESTAMP) == 0)
            dff->fieldValue.hasServerTimestamp = false;
        if(((u64)dataSetWriter->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS) ==
           0)
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
            ((u64)UA_UADPDATASETMESSAGECONTENTMASK_TIMESTAMP | (u64)UA_UADPDATASETMESSAGECONTENTMASK_MAJORVERSION |
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
    if(dataSetWriter->config.dataSetFieldContentMask & (u64)UA_DATASETFIELDCONTENTMASK_RAWDATA) {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_RAWDATA;
    } else if((u64)dataSetWriter->config.dataSetFieldContentMask &
              ((u64)UA_DATASETFIELDCONTENTMASK_SOURCETIMESTAMP | (u64)UA_DATASETFIELDCONTENTMASK_SERVERPICOSECONDS |
               (u64)UA_DATASETFIELDCONTENTMASK_SOURCEPICOSECONDS | (u64)UA_DATASETFIELDCONTENTMASK_STATUSCODE)) {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_DATAVALUE;
    } else {
        dataSetMessage->header.fieldEncoding = UA_FIELDENCODING_VARIANT;
    }

    if(messageType == UA_TYPES_UADPDATASETWRITERMESSAGEDATATYPE) {
        /* Std: 'The DataSetMessageContentMask defines the flags for the content of the DataSetMessage header.' */
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
    /* Check if the PublishedDataSet version has changed -> if yes flush the lastValue store and send a KeyFrame */
    if(dataSetWriter->connectedDataSetVersion.majorVersion != currentDataSet->dataSetMetaData.configurationVersion.majorVersion ||
       dataSetWriter->connectedDataSetVersion.minorVersion != currentDataSet->dataSetMetaData.configurationVersion.minorVersion) {
        /* Remove old samples */
        for(size_t i = 0; i < dataSetWriter->lastSamplesCount; i++)
            UA_DataValue_deleteMembers(&dataSetWriter->lastSamples[i].value);

        /* Realloc pds dependent memory */
        dataSetWriter->lastSamplesCount = currentDataSet->fieldSize;
        UA_DataSetWriterSample *newSamplesArray = (UA_DataSetWriterSample * )
            UA_realloc(dataSetWriter->lastSamples, sizeof(UA_DataSetWriterSample) * dataSetWriter->lastSamplesCount);
        if(!newSamplesArray)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dataSetWriter->lastSamples = newSamplesArray;
        memset(dataSetWriter->lastSamples, 0, sizeof(UA_DataSetWriterSample) * dataSetWriter->lastSamplesCount);

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

    UA_PubSubDataSetWriter_generateKeyFrameMessage(server, dataSetMessage, dataSetWriter);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sendNetworkMessageJson(UA_PubSubConnection *connection, UA_DataSetMessage *dsm,
                   UA_UInt16 *writerIds, UA_Byte dsmCount, UA_ExtensionObject *transportSettings) {
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
            UA_ByteString_deleteMembers(&buf);
        return retval;
    }

    /* Send the prepared messages */
    retval = connection->channel->send(connection->channel, transportSettings, &buf);
    if(msgSize > UA_MAX_STACKBUF)
        UA_ByteString_deleteMembers(&buf);
#endif
    return retval;
}

static UA_StatusCode
sendNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                   UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                   UA_ExtensionObject *messageSettings,
                   UA_ExtensionObject *transportSettings) {

    if(messageSettings->content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
        messageSettings->content.decoded.data;

    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));

    nm.publisherIdEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID) != 0;
    nm.groupHeaderEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER) != 0;
    nm.groupHeader.writerGroupIdEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID) != 0;
    nm.groupHeader.groupVersionEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION) != 0;
    nm.groupHeader.networkMessageNumberEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER) != 0;
    nm.groupHeader.sequenceNumberEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER) != 0;
    nm.payloadHeaderEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER) != 0;
    nm.timestampEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_TIMESTAMP) != 0;
    nm.picosecondsEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PICOSECONDS) != 0;
    nm.dataSetClassIdEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID) != 0;
    nm.promotedFieldsEnabled =
        ((u64)wgm->networkMessageContentMask & (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PROMOTEDFIELDS) != 0;

    nm.version = 1;
    nm.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    if(connection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_NUMERIC) {
        nm.publisherIdType = UA_PUBLISHERDATATYPE_UINT16;
        nm.publisherId.publisherIdUInt32 = connection->config->publisherId.numeric;
    } else if (connection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_STRING){
        nm.publisherIdType = UA_PUBLISHERDATATYPE_STRING;
        nm.publisherId.publisherIdString = connection->config->publisherId.string;
    }

    /* Compute the length of the dsm separately for the header */
    UA_STACKARRAY(UA_UInt16, dsmLengths, dsmCount);
    for(UA_Byte i = 0; i < dsmCount; i++)
        dsmLengths[i] = (UA_UInt16)UA_DataSetMessage_calcSizeBinary(&dsm[i]);

    nm.payloadHeader.dataSetPayloadHeader.count = dsmCount;
    nm.payloadHeader.dataSetPayloadHeader.dataSetWriterIds = writerIds;
    nm.groupHeader.writerGroupId = wg->config.writerGroupId;
    nm.groupHeader.networkMessageNumber = 1;
    nm.payload.dataSetPayload.sizes = dsmLengths;
    nm.payload.dataSetPayload.dataSetMessages = dsm;

    /* Allocate the buffer. Allocate on the stack if the buffer is small. */
    UA_ByteString buf;
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&nm);
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
            return retval;
    }
        
    /* Encode the message */
    UA_Byte *bufPos = buf.data;
    memset(bufPos, 0, msgSize);
    const UA_Byte *bufEnd = &buf.data[buf.length];
    retval = UA_NetworkMessage_encodeBinary(&nm, &bufPos, bufEnd);
    if(retval != UA_STATUSCODE_GOOD) {
        if(msgSize > UA_MAX_STACKBUF)
            UA_ByteString_deleteMembers(&buf);
        return retval;
    }

    /* Send the prepared messages */
    retval = connection->channel->send(connection->channel, transportSettings, &buf);
    if(msgSize > UA_MAX_STACKBUF)
        UA_ByteString_deleteMembers(&buf);
    return retval;
}

/* This callback triggers the collection and publish of NetworkMessages and the
 * contained DataSetMessages. */
void
UA_WriterGroup_publishCallback(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER, "Publish Callback");

    if(!writerGroup) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Publish failed. WriterGroup not found");
        return;
    }

    /* Nothing to do? */
    if(writerGroup->writersCount <= 0)
        return;

    /* Binary or Json encoding?  */
    if(writerGroup->config.encodingMimeType != UA_PUBSUB_ENCODING_UADP &&
       writerGroup->config.encodingMimeType != UA_PUBSUB_ENCODING_JSON) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Publish failed: Unknown encoding type.");
        return;
    }

    /* Find the connection associated with the writer */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, writerGroup->linkedConnection);
    if(!connection) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Publish failed. PubSubConnection invalid.");
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
        /* Find the dataset */
        UA_PublishedDataSet *pds =
            UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
        if(!pds) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Publish: PublishedDataSet not found");
            continue;
        }

        /* Generate the DSM */
        UA_StatusCode res =
            UA_DataSetWriter_generateDataSetMessage(server, &dsmStore[dsmCount], dsw);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Publish: DataSetMessage creation failed");
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
            }else if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON){
                res = sendNetworkMessageJson(connection, &dsmStore[dsmCount],
                        &dsw->config.dataSetWriterId, 1, &writerGroup->config.transportSettings);
            }

            if(res != UA_STATUSCODE_GOOD)
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub Publish: Could not send a NetworkMessage");
            UA_DataSetMessage_free(&dsmStore[dsmCount]);
            continue;
        }

        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        dsmCount++;
    }

    /* Send the NetworkMessages with batched DataSetMessages */
    size_t nmCount = (dsmCount / maxDSM) + ((dsmCount % maxDSM) == 0 ? 0 : 1);
    for(UA_UInt32 i = 0; i < nmCount; i++) {
        UA_Byte nmDsmCount = maxDSM;
        if(i == nmCount - 1)
            nmDsmCount = (UA_Byte)dsmCount % maxDSM;

        UA_StatusCode res3 = UA_STATUSCODE_GOOD;
        if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_UADP){
            res3 = sendNetworkMessage(connection, writerGroup, &dsmStore[i * maxDSM],
                                      &dsWriterIds[i * maxDSM], nmDsmCount,
                                      &writerGroup->config.messageSettings,
                                      &writerGroup->config.transportSettings);
        }else if(writerGroup->config.encodingMimeType == UA_PUBSUB_ENCODING_JSON){
            res3 = sendNetworkMessageJson(connection, &dsmStore[i * maxDSM],
                    &dsWriterIds[i * maxDSM], nmDsmCount, &writerGroup->config.transportSettings);
        }

        if(res3 != UA_STATUSCODE_GOOD)
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Publish: Sending a NetworkMessage failed");
    }

    /* Clean up DSM */
    for(size_t i = 0; i < dsmCount; i++)
        UA_DataSetMessage_free(&dsmStore[i]);
}

/* Add new publishCallback. The first execution is triggered directly after
 * creation. */
UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_StatusCode retval =
            UA_PubSubManager_addRepeatedCallback(server,
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
