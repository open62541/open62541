/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_server_pubsub.h"
#include "server/ua_server_internal.h"
#include "ua_pubsub.h"
#include "ua_pubsub_manager.h"

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
    UA_WriterGroupConfig tmpWriterGroupConfig;
    //deep copy of the config
    retVal |= UA_WriterGroupConfig_copy(writerGroupConfig, &tmpWriterGroupConfig);
    newWriterGroup->config = tmpWriterGroupConfig;
    LIST_INSERT_HEAD(&currentConnectionContext->writerGroups, newWriterGroup, listEntry);
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

    UA_WriterGroup_deleteMembers(server, wg);
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
    if(!fieldConfig)
        return (UA_DataSetFieldResult) {UA_STATUSCODE_BADINVALIDARGUMENT, {0, 0}};

    UA_PublishedDataSet *currentDataSet = UA_PublishedDataSet_findPDSbyId(server, publishedDataSet);
    if(currentDataSet == NULL)
        return (UA_DataSetFieldResult) {UA_STATUSCODE_BADNOTFOUND, {0, 0}};

    if(currentDataSet->config.publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS)
        return (UA_DataSetFieldResult) {UA_STATUSCODE_BADNOTIMPLEMENTED, {0, 0}};

    UA_DataSetField *newField = (UA_DataSetField *) UA_calloc(1, sizeof(UA_DataSetField));
    if(!newField)
        return (UA_DataSetFieldResult) {UA_STATUSCODE_BADINTERNALERROR, {0, 0}};

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
    UA_DataSetFieldResult result =
        {retVal, {currentDataSet->dataSetMetaData.configurationVersion.majorVersion,
                  currentDataSet->dataSetMetaData.configurationVersion.minorVersion}};
    return result;
}

UA_DataSetFieldResult
UA_Server_removeDataSetField(UA_Server *server, const UA_NodeId dsf) {
    UA_DataSetField *currentField = UA_DataSetField_findDSFbyId(server, dsf);
    if(!currentField)
        return (UA_DataSetFieldResult) {UA_STATUSCODE_BADNOTFOUND, {0, 0}};

    UA_PublishedDataSet *parentPublishedDataSet =
        UA_PublishedDataSet_findPDSbyId(server, currentField->publishedDataSet);
    if(!parentPublishedDataSet)
        return (UA_DataSetFieldResult) {UA_STATUSCODE_BADNOTFOUND, {0, 0}};

    parentPublishedDataSet->fieldSize--;
    if(currentField->config.field.variable.promotedField)
        parentPublishedDataSet->promotedFieldsCount--;
    
    /* update major version of PublishedDataSet */
    parentPublishedDataSet->dataSetMetaData.configurationVersion.majorVersion =
        UA_PubSubConfigurationVersionTimeDifference();
    UA_DataSetField_deleteMembers(currentField);
    UA_free(currentField);
    UA_DataSetFieldResult result =
        {UA_STATUSCODE_GOOD, {parentPublishedDataSet->dataSetMetaData.configurationVersion.majorVersion,
                              parentPublishedDataSet->dataSetMetaData.configurationVersion.minorVersion}};
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

void
UA_DataSetWriter_deleteMembers(UA_Server *server, UA_DataSetWriter *dataSetWriter){
    UA_DataSetWriterConfig_deleteMembers(&dataSetWriter->config);
    //delete DataSetWriter
    UA_NodeId_deleteMembers(&dataSetWriter->identifier);
    UA_NodeId_deleteMembers(&dataSetWriter->linkedWriterGroup);
    UA_NodeId_deleteMembers(&dataSetWriter->connectedDataSet);
    LIST_REMOVE(dataSetWriter, listEntry);
    //delete lastSamples store
    for(size_t i = 0; i < dataSetWriter->lastSamplesCount; i++){
        UA_DataValue_delete(dataSetWriter->lastSamples[i].value);
    }
    LIST_REMOVE(dataSetWriter, listEntry);
    UA_free(dataSetWriter->lastSamples);
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

void
UA_WriterGroup_deleteMembers(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_WriterGroupConfig_deleteMembers(&writerGroup->config);
    //delete WriterGroup
    //delete all writers. Therefore removeDataSetWriter is called from PublishedDataSet
    UA_DataSetWriter *dataSetWriter, *tmpDataSetWriter;
    LIST_FOREACH_SAFE(dataSetWriter, &writerGroup->writers, listEntry, tmpDataSetWriter){
        UA_Server_removeDataSetWriter(server, dataSetWriter->identifier);
    }
    LIST_REMOVE(writerGroup, listEntry);
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
    //initialize the queue for the last values
    newDataSetWriter->lastSamplesCount = currentDataSetContext->fieldSize;
    newDataSetWriter->lastSamples = (UA_DataSetWriterSample * )
        UA_calloc(newDataSetWriter->lastSamplesCount, sizeof(UA_DataSetWriterSample));
    if(!newDataSetWriter->lastSamples) {
        UA_DataSetWriterConfig_deleteMembers(&newDataSetWriter->config);
        UA_free(newDataSetWriter);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    for(size_t i = 0; i < newDataSetWriter->lastSamplesCount; i++) {
        newDataSetWriter->lastSamples[i].value = (UA_DataValue *) UA_calloc(1, sizeof(UA_DataValue));
        if(!newDataSetWriter->lastSamples[i].value) {
            for(size_t j = 0; j < i; j++)
                UA_free(newDataSetWriter->lastSamples[j].value);
            UA_DataSetWriterConfig_deleteMembers(&newDataSetWriter->config);
            UA_free(newDataSetWriter);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    //connect PublishedDataSet with DataSetWriter
    newDataSetWriter->connectedDataSet = currentDataSetContext->identifier;
    newDataSetWriter->linkedWriterGroup = wg->identifier;
    UA_PubSubManager_generateUniqueNodeId(server, &newDataSetWriter->identifier);
    if(writerIdentifier != NULL)
        UA_NodeId_copy(&newDataSetWriter->identifier, writerIdentifier);
    //add the new writer to the group
    LIST_INSERT_HEAD(&wg->writers, newDataSetWriter, listEntry);
    wg->writersCount++;
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
    //remove DataSetWriter from group
    UA_DataSetWriter_deleteMembers(server, dataSetWriter);
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

void UA_DataSetField_deleteMembers(UA_DataSetField *field) {
    UA_DataSetFieldConfig_deleteMembers(&field->config);
    //delete DataSetField
    UA_NodeId_deleteMembers(&field->identifier);
    UA_NodeId_deleteMembers(&field->publishedDataSet);
    UA_FieldMetaData_deleteMembers(&field->fieldMetaData);
    UA_DataValue_deleteMembers(&field->lastValue);
    LIST_REMOVE(field, listEntry);
}
