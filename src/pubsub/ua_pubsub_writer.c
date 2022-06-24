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

/* Forward declaration */
static void
UA_DataSetField_clear(UA_DataSetField *field);

/**********************************************/
/*               Connection                   */
/**********************************************/

UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src,
                               UA_PubSubConnectionConfig *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PubSubConnectionConfig));
    if (src->publisherIdType == UA_PUBLISHERIDTYPE_STRING) {
        res |= UA_String_copy(&src->publisherId.string, &dst->publisherId.string);
    }
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_Variant_copy(&src->address, &dst->address);
    res |= UA_String_copy(&src->transportProfileUri, &dst->transportProfileUri);
    res |= UA_Variant_copy(&src->connectionTransportSettings,
                           &dst->connectionTransportSettings);
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
    if (connectionConfig->publisherIdType == UA_PUBLISHERIDTYPE_STRING) {
        UA_String_clear(&connectionConfig->publisherId.string);
    }
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
    LIST_FOREACH_SAFE(writerGroup, &connection->writerGroups,
                      listEntry, tmpWriterGroup) {
        removeWriterGroup(server, writerGroup->identifier);
    }

    /* Remove ReaderGroups */
    UA_ReaderGroup *readerGroups, *tmpReaderGroup;
    LIST_FOREACH_SAFE(readerGroups, &connection->readerGroups, listEntry, tmpReaderGroup)
        removeReaderGroup(server, readerGroups->identifier);

    UA_NodeId_clear(&connection->identifier);
    if(connection->channel)
        connection->channel->close(connection->channel);

    UA_PubSubConnectionConfig_clear(connection->config);
    UA_free(connection->config);
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
    switch(src->publishedDataSetType) {
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
            //no additional items
            break;

        case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
            if(src->config.itemsTemplate.variablesToAddSize > 0) {
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

            for(size_t i = 0; i < src->config.itemsTemplate.variablesToAddSize; i++) {
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
                                    UA_PublishedDataSetConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_PublishedDataSet *currentPDS = UA_PublishedDataSet_findPDSbyId(server, pds);
    if(!currentPDS)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_PublishedDataSetConfig_copy(&currentPDS->config, config);
}

UA_StatusCode
UA_Server_getPublishedDataSetMetaData(UA_Server *server, const UA_NodeId pds,
                                      UA_DataSetMetaDataType *metaData) {
    if(!metaData)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_PublishedDataSet *currentPDS = UA_PublishedDataSet_findPDSbyId(server, pds);
    if(!currentPDS)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_DataSetMetaDataType_copy(&currentPDS->dataSetMetaData, metaData);
}

UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PublishedDataSet *tmpPDS = NULL;
    TAILQ_FOREACH(tmpPDS, &server->pubSubManager.publishedDataSets, listEntry) {
        if(UA_NodeId_equal(&tmpPDS->identifier, &identifier))
            break;
    }
    return tmpPDS;
}

UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyName(UA_Server *server, UA_String name) {
    UA_PublishedDataSet *tmpPDS = NULL;
    TAILQ_FOREACH(tmpPDS, &server->pubSubManager.publishedDataSets, listEntry) {
        if(UA_String_equal(&name, &tmpPDS->config.name))
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
    UA_DataSetField *field, *tmpField;
    TAILQ_FOREACH_SAFE(field, &publishedDataSet->fields, listEntry, tmpField) {
        removeDataSetField(server, field->identifier);
    }
    UA_PublishedDataSetConfig_clear(&publishedDataSet->config);
    UA_DataSetMetaDataType_clear(&publishedDataSet->dataSetMetaData);
    UA_NodeId_clear(&publishedDataSet->identifier);
}

/* The fieldMetaData variable has to be cleaned up external in case of an error */
static UA_StatusCode
generateFieldMetaData(UA_Server *server, UA_DataSetField *field,
                      UA_FieldMetaData *fieldMetaData) {
    if(field->config.dataSetFieldType != UA_PUBSUB_DATASETFIELD_VARIABLE)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* Set the field identifier */
    fieldMetaData->dataSetFieldId = UA_PubSubManager_generateUniqueGuid(server);

    /* Set the description */
    fieldMetaData->description = UA_LOCALIZEDTEXT_ALLOC("", "");

    /* Set the name */
    const UA_DataSetVariableConfig *var = &field->config.field.variable;
    UA_StatusCode res = UA_String_copy(&var->fieldNameAlias, &fieldMetaData->name);
    UA_CHECK_STATUS(res, return res);

    /* Static value source. ToDo after freeze PR, the value source must be
     * checked (other behavior for static value source) */
    if(var->rtValueSource.rtFieldSourceEnabled &&
       !var->rtValueSource.rtInformationModelNode) {
        const UA_DataValue *svs = *var->rtValueSource.staticValueSource;
        if(svs->value.arrayDimensionsSize > 0) {
            fieldMetaData->arrayDimensions = (UA_UInt32 *)
                UA_calloc(svs->value.arrayDimensionsSize, sizeof(UA_UInt32));
            if(fieldMetaData->arrayDimensions == NULL)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            memcpy(fieldMetaData->arrayDimensions, svs->value.arrayDimensions,
                   sizeof(UA_UInt32) * svs->value.arrayDimensionsSize);
        }
        fieldMetaData->arrayDimensionsSize = svs->value.arrayDimensionsSize;

        res = UA_NodeId_copy(&svs->value.type->typeId, &fieldMetaData->dataType);
        UA_CHECK_STATUS(res, return res);

        //TODO collect value rank for the static field source
        fieldMetaData->properties = NULL;
        fieldMetaData->propertiesSize = 0;
        fieldMetaData->fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        return UA_STATUSCODE_GOOD;
    }

    /* Set the Array Dimensions */
    const UA_PublishedVariableDataType *pp = &var->publishParameters;
    UA_Variant value;
    UA_Variant_init(&value);
    res = readWithReadValue(server, &pp->publishedVariable,
                            UA_ATTRIBUTEID_ARRAYDIMENSIONS, &value);
    UA_CHECK_STATUS_LOG(res, return res,
                        WARNING, &server->config.logger, UA_LOGCATEGORY_SERVER,
                        "PubSub meta data generation. Reading the array dimensions failed.");

    if(value.arrayDimensionsSize > 0) {
        fieldMetaData->arrayDimensions = (UA_UInt32 *)
            UA_calloc(value.arrayDimensionsSize, sizeof(UA_UInt32));
        if(!fieldMetaData->arrayDimensions)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memcpy(fieldMetaData->arrayDimensions, value.arrayDimensions,
               sizeof(UA_UInt32)*value.arrayDimensionsSize);
    }
    fieldMetaData->arrayDimensionsSize = value.arrayDimensionsSize;

    /* Set the DataType */
    res = readWithReadValue(server, &pp->publishedVariable,
                            UA_ATTRIBUTEID_DATATYPE, &fieldMetaData->dataType);
    UA_CHECK_STATUS_LOG(res, return res,
                        WARNING, &server->config.logger, UA_LOGCATEGORY_SERVER,
                        "PubSub meta data generation. Reading the datatype failed.");

    if(!UA_NodeId_isNull(&fieldMetaData->dataType)) {
        const UA_DataType *currentDataType =
            UA_findDataTypeWithCustom(&fieldMetaData->dataType,
                                      server->config.customDataTypes);
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "MetaData creation. Found DataType %s.", currentDataType->typeName);
#endif
        /* Check if the datatype is a builtInType, if yes set the builtinType.
         * TODO: Remove the magic number */
        if(currentDataType->typeKind <= UA_DATATYPEKIND_ENUM)
            fieldMetaData->builtInType = (UA_Byte)currentDataType->typeKind;
    } else {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "PubSub meta data generation. DataType is UA_NODEID_NULL.");
    }

    /* Set the ValueRank */
    UA_Int32 valueRank;
    res = readWithReadValue(server, &pp->publishedVariable,
                            UA_ATTRIBUTEID_VALUERANK, &valueRank);
    UA_CHECK_STATUS_LOG(res, return res,
                        WARNING, &server->config.logger, UA_LOGCATEGORY_SERVER,
                        "PubSub meta data generation. Reading the value rank failed.");
    fieldMetaData->valueRank = valueRank;

    /* PromotedField? */
    if(var->promotedField)
        fieldMetaData->fieldFlags = UA_DATASETFIELDFLAGS_PROMOTEDFIELD;
    else
        fieldMetaData->fieldFlags = UA_DATASETFIELDFLAGS_NONE;

    /* Properties */
    fieldMetaData->properties = NULL;
    fieldMetaData->propertiesSize = 0;

    //TODO collect the following fields*/
    //fieldMetaData.builtInType
    //fieldMetaData.maxStringLength

    return UA_STATUSCODE_GOOD;
}

UA_DataSetFieldResult
addDataSetField(UA_Server *server, const UA_NodeId publishedDataSet,
                const UA_DataSetFieldConfig *fieldConfig,
                UA_NodeId *fieldIdentifier) {
    UA_DataSetFieldResult result = {0};
    if(!fieldConfig) {
        result.result = UA_STATUSCODE_BADINVALIDARGUMENT;
        return result;
    }

    UA_PublishedDataSet *currDS =
        UA_PublishedDataSet_findPDSbyId(server, publishedDataSet);
    if(!currDS) {
        result.result = UA_STATUSCODE_BADNOTFOUND;
        return result;
    }

    if(currDS->configurationFrozen) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding DataSetField failed. PublishedDataSet is frozen.");
        result.result = UA_STATUSCODE_BADCONFIGURATIONERROR;
        return result;
    }

    if(currDS->config.publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS) {
        result.result = UA_STATUSCODE_BADNOTIMPLEMENTED;
        return result;
    }

    UA_DataSetField *newField = (UA_DataSetField*)UA_calloc(1, sizeof(UA_DataSetField));
    if(!newField) {
        result.result = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    result.result = UA_DataSetFieldConfig_copy(fieldConfig, &newField->config);
    if(result.result != UA_STATUSCODE_GOOD) {
        UA_free(newField);
        return result;
    }

    result.result = UA_NodeId_copy(&currDS->identifier, &newField->publishedDataSet);
    if(result.result != UA_STATUSCODE_GOOD) {
        UA_DataSetFieldConfig_clear(&newField->config);
        UA_free(newField);
        return result;
    }

    /* Initialize the field metadata. Also generates a FieldId */
    UA_FieldMetaData fmd;
    UA_FieldMetaData_init(&fmd);
    result.result = generateFieldMetaData(server, newField, &fmd);
    if(result.result != UA_STATUSCODE_GOOD) {
        UA_FieldMetaData_clear(&fmd);
        UA_DataSetFieldConfig_clear(&newField->config);
        UA_NodeId_clear(&newField->publishedDataSet);
        UA_free(newField);
        return result;
    }

    /* Append to the metadata fields array. Point of last return. */
    result.result = UA_Array_append((void**)&currDS->dataSetMetaData.fields,
                                    &currDS->dataSetMetaData.fieldsSize,
                                    &fmd, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    if(result.result != UA_STATUSCODE_GOOD) {
        UA_FieldMetaData_clear(&fmd);
        UA_DataSetFieldConfig_clear(&newField->config);
        UA_NodeId_clear(&newField->publishedDataSet);
        UA_free(newField);
        return result;
    }

    /* Copy the identifier from the metadata. Cannot fail with a guid NodeId. */
    newField->identifier = UA_NODEID_GUID(1, fmd.dataSetFieldId);
    if(fieldIdentifier)
        UA_NodeId_copy(&newField->identifier, fieldIdentifier);

    /* Register the field. The order of DataSetFields should be the same in both
     * creating and publishing. So adding DataSetFields at the the end of the
     * DataSets using the TAILQ structure. */
    TAILQ_INSERT_TAIL(&currDS->fields, newField, listEntry);
    currDS->fieldSize++;

    if(newField->config.field.variable.promotedField)
        currDS->promotedFieldsCount++;

    /* The values of the metadata are "borrowed" in a mirrored structure in the
     * pds. Reset them after resizing the array. */
    size_t counter = 0;
    UA_DataSetField *dsf;
    TAILQ_FOREACH(dsf, &currDS->fields, listEntry) {
        dsf->fieldMetaData = currDS->dataSetMetaData.fields[counter++];
    }

    /* Update major version of parent published data set */
    currDS->dataSetMetaData.configurationVersion.majorVersion =
        UA_PubSubConfigurationVersionTimeDifference();

    result.configurationVersion.majorVersion =
        currDS->dataSetMetaData.configurationVersion.majorVersion;
    result.configurationVersion.minorVersion =
        currDS->dataSetMetaData.configurationVersion.minorVersion;
    return result;
}

UA_DataSetFieldResult
UA_Server_addDataSetField(UA_Server *server, const UA_NodeId publishedDataSet,
                          const UA_DataSetFieldConfig *fieldConfig,
                          UA_NodeId *fieldIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_DataSetFieldResult res =
        addDataSetField(server, publishedDataSet, fieldConfig, fieldIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_DataSetFieldResult
removeDataSetField(UA_Server *server, const UA_NodeId dsf) {
    UA_DataSetFieldResult result = {0};

    UA_DataSetField *currentField = UA_DataSetField_findDSFbyId(server, dsf);
    if(!currentField) {
        result.result = UA_STATUSCODE_BADNOTFOUND;
        return result;
    }

    if(currentField->configurationFrozen) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove DataSetField failed. DataSetField is frozen.");
        result.result = UA_STATUSCODE_BADCONFIGURATIONERROR;
        return result;
    }

    UA_PublishedDataSet *pds =
        UA_PublishedDataSet_findPDSbyId(server, currentField->publishedDataSet);
    if(!pds) {
        result.result = UA_STATUSCODE_BADNOTFOUND;
        return result;
    }

    if(pds->configurationFrozen) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove DataSetField failed. PublishedDataSet is frozen.");
        result.result = UA_STATUSCODE_BADCONFIGURATIONERROR;
        return result;
    }

    /* Reduce the counters before the config is cleaned up */
    if(currentField->config.field.variable.promotedField)
        pds->promotedFieldsCount--;
    pds->fieldSize--;

    /* Update major version of PublishedDataSet */
    pds->dataSetMetaData.configurationVersion.majorVersion =
        UA_PubSubConfigurationVersionTimeDifference();

    /* Clean up */
    currentField->fieldMetaData.arrayDimensions = NULL;
    currentField->fieldMetaData.properties = NULL;
    currentField->fieldMetaData.name = UA_STRING_NULL;
    currentField->fieldMetaData.description.locale = UA_STRING_NULL;
    currentField->fieldMetaData.description.text = UA_STRING_NULL;
    UA_DataSetField_clear(currentField);

    /* Remove */
    TAILQ_REMOVE(&pds->fields, currentField, listEntry);
    UA_free(currentField);

    /* Regenerate DataSetMetaData */
    pds->dataSetMetaData.fieldsSize--;
    if(pds->dataSetMetaData.fieldsSize > 0) {
        for(size_t i = 0; i < pds->dataSetMetaData.fieldsSize+1; i++) {
            UA_FieldMetaData_clear(&pds->dataSetMetaData.fields[i]);
        }
        UA_free(pds->dataSetMetaData.fields);
        UA_FieldMetaData *fieldMetaData = (UA_FieldMetaData *)
            UA_calloc(pds->dataSetMetaData.fieldsSize, sizeof(UA_FieldMetaData));
        if(!fieldMetaData) {
            result.result =  UA_STATUSCODE_BADOUTOFMEMORY;
            return result;
        }
        UA_DataSetField *tmpDSF;
        size_t counter = 0;
        TAILQ_FOREACH(tmpDSF, &pds->fields, listEntry){
            result.result = generateFieldMetaData(server, tmpDSF, &fieldMetaData[counter]);
            if(result.result != UA_STATUSCODE_GOOD) {
                UA_FieldMetaData_clear(&fieldMetaData[counter]);
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub MetaData generation failed!");
                break;
            }
            counter++;
        }
        pds->dataSetMetaData.fields = fieldMetaData;
    } else {
        UA_FieldMetaData_delete(pds->dataSetMetaData.fields);
        pds->dataSetMetaData.fields = NULL;
    }

    result.configurationVersion.majorVersion =
        pds->dataSetMetaData.configurationVersion.majorVersion;
    result.configurationVersion.minorVersion =
        pds->dataSetMetaData.configurationVersion.minorVersion;
    return result;
}

UA_DataSetFieldResult
UA_Server_removeDataSetField(UA_Server *server, const UA_NodeId dsf) {
    UA_LOCK(&server->serviceMutex);
    UA_DataSetFieldResult res = removeDataSetField(server, dsf);
    UA_UNLOCK(&server->serviceMutex);
    return res;
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
    if(src->dataSetWriterPropertiesSize > 0) {
        dst->dataSetWriterProperties = (UA_KeyValuePair *)
            UA_calloc(src->dataSetWriterPropertiesSize, sizeof(UA_KeyValuePair));
        if(!dst->dataSetWriterProperties)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        for(size_t i = 0; i < src->dataSetWriterPropertiesSize; i++){
            retVal |= UA_KeyValuePair_copy(&src->dataSetWriterProperties[i],
                                           &dst->dataSetWriterProperties[i]);
        }
    }
    return retVal;
}

UA_StatusCode
UA_Server_getDataSetWriterConfig(UA_Server *server, const UA_NodeId dsw,
                                 UA_DataSetWriterConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_DataSetWriter *currentDataSetWriter = UA_DataSetWriter_findDSWbyId(server, dsw);
    if(!currentDataSetWriter)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_DataSetWriterConfig_copy(&currentDataSetWriter->config, config);
}

UA_StatusCode
UA_Server_DataSetWriter_getState(UA_Server *server, UA_NodeId dataSetWriterIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_DataSetWriter *currentDataSetWriter =
        UA_DataSetWriter_findDSWbyId(server, dataSetWriterIdentifier);
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
    for(size_t i = 0; i < pdsConfig->dataSetWriterPropertiesSize; i++) {
        UA_KeyValuePair_clear(&pdsConfig->dataSetWriterProperties[i]);
    }
    UA_free(pdsConfig->dataSetWriterProperties);
    UA_ExtensionObject_clear(&pdsConfig->messageSettings);
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
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
                    break;
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
                    break;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Received unknown PubSub state!");
            }
            break;
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                           "Received unknown PubSub state!");
    }
    if (state != oldState) {
        /* inform application about state change */
        UA_ServerConfig *pConfig = UA_Server_getConfig(server);
        if(pConfig->pubSubConfig.stateChangeCallback != 0) {
            pConfig->pubSubConfig.
                stateChangeCallback(server, &dataSetWriter->identifier, state, cause);
        }
    }
    return ret;
}

static UA_StatusCode
addDataSetWriter(UA_Server *server,
                 const UA_NodeId writerGroup, const UA_NodeId dataSet,
                 const UA_DataSetWriterConfig *dataSetWriterConfig,
                 UA_NodeId *writerIdentifier) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(!dataSetWriterConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Make checks for a heartbeat */
    if(UA_NodeId_isNull(&dataSet) && dataSetWriterConfig->keyFrameCount != 1) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding DataSetWriter failed. DataSet can be null only for a heartbeat, "
                       "in which case KeyFrameCount shall be 1.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    if(wg->configurationFrozen) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding DataSetWriter failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PublishedDataSet *currentDataSetContext = NULL;

    if(!UA_NodeId_isNull(&dataSet)) {
        currentDataSetContext = UA_PublishedDataSet_findPDSbyId(server, dataSet);
        if(!currentDataSetContext)
            return UA_STATUSCODE_BADNOTFOUND;

        if(currentDataSetContext->configurationFrozen){
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                        "Adding DataSetWriter failed. PublishedDataSet is frozen.");
            return UA_STATUSCODE_BADCONFIGURATIONERROR;
        }

        if(wg->config.rtLevel != UA_PUBSUB_RT_NONE) {
            UA_DataSetField *tmpDSF;
            TAILQ_FOREACH(tmpDSF, &currentDataSetContext->fields, listEntry) {
                if(!tmpDSF->config.field.variable.rtValueSource.rtFieldSourceEnabled &&
                !tmpDSF->config.field.variable.rtValueSource.rtInformationModelNode) {
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                "Adding DataSetWriter failed. Fields in PDS are not RT capable.");
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
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "Add DataSetWriter failed. setPubSubState failed.");
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
    UA_StatusCode res =
        addDataSetWriter(server, writerGroup, dataSet,
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
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove DataSetWriter failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Remove from information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeDataSetWriterRepresentation(server, dataSetWriter);
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
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove DataSetWriter failed. DataSetWriter is frozen.");
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

/**********************************************/
/*                DataSetField                */
/**********************************************/

static void
UA_DataSetField_clear(UA_DataSetField *field) {
    UA_DataSetFieldConfig_clear(&field->config);
    UA_NodeId_clear(&field->identifier);
    UA_NodeId_clear(&field->publishedDataSet);
    UA_FieldMetaData_clear(&field->fieldMetaData);
}

UA_StatusCode
UA_DataSetFieldConfig_copy(const UA_DataSetFieldConfig *src,
                           UA_DataSetFieldConfig *dst) {
    if(src->dataSetFieldType != UA_PUBSUB_DATASETFIELD_VARIABLE)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    memcpy(dst, src, sizeof(UA_DataSetFieldConfig));
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_String_copy(&src->field.variable.fieldNameAlias,
                          &dst->field.variable.fieldNameAlias);
    res |= UA_PublishedVariableDataType_copy(&src->field.variable.publishParameters,
                                             &dst->field.variable.publishParameters);
    if(res != UA_STATUSCODE_GOOD)
        UA_DataSetFieldConfig_clear(dst);
    return res;
}

UA_StatusCode
UA_Server_getDataSetFieldConfig(UA_Server *server, const UA_NodeId dsf,
                                UA_DataSetFieldConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_DataSetField *currentDataSetField = UA_DataSetField_findDSFbyId(server, dsf);
    if(!currentDataSetField)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_DataSetFieldConfig_copy(&currentDataSetField->config, config);
}

UA_DataSetField *
UA_DataSetField_findDSFbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PublishedDataSet *tmpPDS;
    TAILQ_FOREACH(tmpPDS, &server->pubSubManager.publishedDataSets, listEntry) {
        UA_DataSetField *tmpField;
        TAILQ_FOREACH(tmpField, &tmpPDS->fields, listEntry) {
            if(UA_NodeId_equal(&tmpField->identifier, &identifier))
                return tmpField;
        }
    }
    return NULL;
}

void
UA_DataSetFieldConfig_clear(UA_DataSetFieldConfig *dataSetFieldConfig) {
    if(dataSetFieldConfig->dataSetFieldType == UA_PUBSUB_DATASETFIELD_VARIABLE) {
        UA_String_clear(&dataSetFieldConfig->field.variable.fieldNameAlias);
        UA_PublishedVariableDataType_clear(&dataSetFieldConfig->field.variable.publishParameters);
    }
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

    UA_ByteString oldValueEncoding = {0};
    UA_StatusCode res = UA_ByteString_allocBuffer(&oldValueEncoding, oldValueEncodingSize);
    if(res != UA_STATUSCODE_GOOD)
        return false;

    UA_ByteString newValueEncoding = {0};
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

/* Obtain the latest value for a specific DataSetField. This method is currently
 * called inside the DataSetMessage generation process. */
static void
UA_PubSubDataSetField_sampleValue(UA_Server *server, UA_DataSetField *field,
                                  UA_DataValue *value) {
    UA_PublishedVariableDataType *params = &field->config.field.variable.publishParameters;

    /* Read the value */
    if(field->config.field.variable.rtValueSource.rtInformationModelNode) {
        const UA_VariableNode *rtNode = (const UA_VariableNode *)
            UA_NODESTORE_GET(server, &params->publishedVariable);
        *value = **rtNode->valueBackend.backend.external.value;
        value->value.storageType = UA_VARIANT_DATA_NODELETE;
        UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
    } else if(field->config.field.variable.rtValueSource.rtFieldSourceEnabled == UA_FALSE){
        UA_ReadValueId rvid;
        UA_ReadValueId_init(&rvid);
        rvid.nodeId = params->publishedVariable;
        rvid.attributeId = params->attributeId;
        rvid.indexRange = params->indexRange;
        *value = readAttribute(server, &rvid, UA_TIMESTAMPSTORETURN_BOTH);
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
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Static DSM configuration not supported. Using defaults");
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
            dataSetMessage->header.statusEnabled = false;
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
            dataSetMessage->header.statusEnabled = false;
        }
    }

    /* Set the sequence count. Automatically rolls over to zero */
    dataSetWriter->actualDataSetMessageSequenceCount++;

    if(heartbeat){
        /* Prepare DataSetMessageContent */
        dataSetMessage->header.dataSetMessageValid = true;
        dataSetMessage->header.dataSetMessageType = UA_DATASETMESSAGE_DATAKEYFRAME;
        dataSetMessage->data.keyFrameData.fieldCount = 0; // Heartbeat

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

    return UA_PubSubDataSetWriter_generateKeyFrameMessage(server, dataSetMessage,
                                                          dataSetWriter);
}

#endif /* UA_ENABLE_PUBSUB */
