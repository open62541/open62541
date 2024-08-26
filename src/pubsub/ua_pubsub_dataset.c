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
#include "ua_pubsub.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

static void
UA_DataSetField_clear(UA_DataSetField *field) {
    UA_DataSetFieldConfig_clear(&field->config);
    UA_NodeId_clear(&field->identifier);
    UA_NodeId_clear(&field->publishedDataSet);
    UA_FieldMetaData_clear(&field->fieldMetaData);
}

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

UA_PublishedDataSet *
UA_PublishedDataSet_find(UA_PubSubManager *psm, const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_PublishedDataSet *tmpPDS = NULL;
    TAILQ_FOREACH(tmpPDS, &psm->publishedDataSets, listEntry) {
        if(UA_NodeId_equal(&id, &tmpPDS->head.identifier))
            break;
    }
    return tmpPDS;
}

UA_PublishedDataSet *
UA_PublishedDataSet_findByName(UA_PubSubManager *psm, const UA_String name) {
    if(!psm)
        return NULL;
    UA_PublishedDataSet *tmpPDS = NULL;
    TAILQ_FOREACH(tmpPDS, &psm->publishedDataSets, listEntry) {
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

/* The fieldMetaData variable has to be cleaned up external in case of an error */
static UA_StatusCode
generateFieldMetaData(UA_PubSubManager *psm, UA_PublishedDataSet *pds,
                      UA_DataSetField *field, UA_FieldMetaData *fieldMetaData) {
    if(field->config.dataSetFieldType != UA_PUBSUB_DATASETFIELD_VARIABLE)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* Set the field identifier */
    fieldMetaData->dataSetFieldId = UA_PubSubManager_generateUniqueGuid(psm);

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

        if(svs->value.type)
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
    res = readWithReadValue(psm->sc.server, &pp->publishedVariable,
                            UA_ATTRIBUTEID_ARRAYDIMENSIONS, &value);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                              "PubSub meta data generation: Reading the array dimensions failed");
        return res;
    }

    if(value.arrayDimensionsSize > 0) {
        fieldMetaData->arrayDimensions = (UA_UInt32 *)
            UA_calloc(value.arrayDimensionsSize, sizeof(UA_UInt32));
        if(!fieldMetaData->arrayDimensions)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        memcpy(fieldMetaData->arrayDimensions, value.arrayDimensions,
               sizeof(UA_UInt32)*value.arrayDimensionsSize);
    }
    fieldMetaData->arrayDimensionsSize = value.arrayDimensionsSize;

    UA_Variant_clear(&value);

    /* Set the DataType */
    res = readWithReadValue(psm->sc.server, &pp->publishedVariable,
                            UA_ATTRIBUTEID_DATATYPE, &fieldMetaData->dataType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                              "PubSub meta data generation: Reading the datatype failed");
        return res;
    }

    if(!UA_NodeId_isNull(&fieldMetaData->dataType)) {
        const UA_DataType *currentDataType =
            UA_findDataTypeWithCustom(&fieldMetaData->dataType,
                                      psm->sc.server->config.customDataTypes);
#ifdef UA_ENABLE_TYPEDESCRIPTION
        UA_LOG_DEBUG_PUBSUB(psm->logging, pds,
                            "MetaData creation: Found DataType %s",
                            currentDataType->typeName);
#endif
        /* Check if the datatype is a builtInType, if yes set the builtinType. */
        if(currentDataType->typeKind <= UA_DATATYPEKIND_ENUM)
            fieldMetaData->builtInType = (UA_Byte)currentDataType->typeId.identifier.numeric;
        /* set the maxStringLength attribute */
        if(field->config.field.variable.maxStringLength != 0){
            if(currentDataType->typeKind == UA_DATATYPEKIND_BYTESTRING ||
            currentDataType->typeKind == UA_DATATYPEKIND_STRING ||
            currentDataType->typeKind == UA_DATATYPEKIND_LOCALIZEDTEXT) {
                fieldMetaData->maxStringLength = field->config.field.variable.maxStringLength;
            } else {
                UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                                      "PubSub meta data generation: MaxStringLength with incompatible DataType configured.");
            }
        }
    } else {
        UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                              "PubSub meta data generation: DataType is UA_NODEID_NULL");
    }

    /* Set the ValueRank */
    UA_Int32 valueRank;
    res = readWithReadValue(psm->sc.server, &pp->publishedVariable,
                            UA_ATTRIBUTEID_VALUERANK, &valueRank);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                              "PubSub meta data generation: Reading the value rank failed");
        return res;
    }
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
UA_DataSetField_create(UA_PubSubManager *psm, const UA_NodeId publishedDataSet,
                       const UA_DataSetFieldConfig *fieldConfig,
                       UA_NodeId *fieldIdentifier) {
    UA_DataSetFieldResult result;
    memset(&result, 0, sizeof(UA_DataSetFieldResult));
    if(!fieldConfig) {
        result.result = UA_STATUSCODE_BADINVALIDARGUMENT;
        return result;
    }

    UA_PublishedDataSet *currDS =
        UA_PublishedDataSet_find(psm, publishedDataSet);
    if(!currDS) {
        result.result = UA_STATUSCODE_BADNOTFOUND;
        return result;
    }

    /* If currDS was found, psm != NULL */
    if(currDS->configurationFreezeCounter > 0) {
        UA_LOG_WARNING_PUBSUB(psm->logging, currDS,
                              "Adding DataSetField failed: PublishedDataSet is frozen");
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

    result.result = UA_NodeId_copy(&currDS->head.identifier, &newField->publishedDataSet);
    if(result.result != UA_STATUSCODE_GOOD) {
        UA_DataSetFieldConfig_clear(&newField->config);
        UA_free(newField);
        return result;
    }

    /* Initialize the field metadata. Also generates a FieldId */
    UA_FieldMetaData fmd;
    UA_FieldMetaData_init(&fmd);
    result.result = generateFieldMetaData(psm, currDS, newField, &fmd);
    if(result.result != UA_STATUSCODE_GOOD) {
        UA_FieldMetaData_clear(&fmd);
        UA_DataSetFieldConfig_clear(&newField->config);
        UA_NodeId_clear(&newField->publishedDataSet);
        UA_free(newField);
        return result;
    }

    /* Append to the metadata fields array. Point of last return. */
    result.result = UA_Array_appendCopy((void**)&currDS->dataSetMetaData.fields,
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
    UA_FieldMetaData_clear(&fmd);

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
    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    currDS->dataSetMetaData.configurationVersion.majorVersion =
        UA_PubSubConfigurationVersionTimeDifference(el->dateTime_now(el));

    result.configurationVersion.majorVersion =
        currDS->dataSetMetaData.configurationVersion.majorVersion;
    result.configurationVersion.minorVersion =
        currDS->dataSetMetaData.configurationVersion.minorVersion;
    return result;
}

UA_DataSetFieldResult
UA_DataSetField_remove(UA_PubSubManager *psm, UA_DataSetField *currentField) {
    UA_DataSetFieldResult result;
    memset(&result, 0, sizeof(UA_DataSetFieldResult));

    if(!currentField) {
        result.result = UA_STATUSCODE_BADNOTFOUND;
        return result;
    }

    UA_PublishedDataSet *pds =
        UA_PublishedDataSet_find(psm, currentField->publishedDataSet);
    if(!pds) {
        result.result = UA_STATUSCODE_BADNOTFOUND;
        return result;
    }

    if(currentField->configurationFrozen) {
        UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                              "Remove DataSetField failed: DataSetField is frozen");
        result.result = UA_STATUSCODE_BADCONFIGURATIONERROR;
        return result;
    }

    if(pds->configurationFreezeCounter > 0) {
        UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                              "Remove DataSetField failed: PublishedDataSet is frozen");
        result.result = UA_STATUSCODE_BADCONFIGURATIONERROR;
        return result;
    }

    /* Reduce the counters before the config is cleaned up */
    if(currentField->config.field.variable.promotedField)
        pds->promotedFieldsCount--;
    pds->fieldSize--;

    /* Update major version of PublishedDataSet */
    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    pds->dataSetMetaData.configurationVersion.majorVersion =
        UA_PubSubConfigurationVersionTimeDifference(el->dateTime_now(el));

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
        TAILQ_FOREACH(tmpDSF, &pds->fields, listEntry) {
            result.result = generateFieldMetaData(psm, pds, tmpDSF, &fieldMetaData[counter]);
            if(result.result != UA_STATUSCODE_GOOD) {
                UA_FieldMetaData_clear(&fieldMetaData[counter]);
                UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                                      "PubSub MetaData regeneration failed "
                                      "after removing a field!");
                break;
            }
            /* The contents of the metadata is shared between the PDS and its fields */
            tmpDSF->fieldMetaData = fieldMetaData[counter++];
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

UA_DataSetField *
UA_DataSetField_find(UA_PubSubManager *psm, UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_PublishedDataSet *tmpPDS;
    TAILQ_FOREACH(tmpPDS, &psm->publishedDataSets, listEntry) {
        UA_DataSetField *tmpField;
        TAILQ_FOREACH(tmpField, &tmpPDS->fields, listEntry) {
            if(UA_NodeId_equal(&id, &tmpField->identifier))
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

/* Obtain the latest value for a specific DataSetField. This method is currently
 * called inside the DataSetMessage generation process. */
void
UA_PubSubDataSetField_sampleValue(UA_PubSubManager *psm, UA_DataSetField *field,
                                  UA_DataValue *value) {
    UA_PublishedVariableDataType *params = &field->config.field.variable.publishParameters;

    /* Read the value */
    if(field->config.field.variable.rtValueSource.rtInformationModelNode) {
        const UA_VariableNode *rtNode = (const UA_VariableNode *)
            UA_NODESTORE_GET(psm->sc.server, &params->publishedVariable);
        *value = **rtNode->valueBackend.backend.external.value;
        value->value.storageType = UA_VARIANT_DATA_NODELETE;
        UA_NODESTORE_RELEASE(psm->sc.server, (const UA_Node *) rtNode);
    } else if(field->config.field.variable.rtValueSource.rtFieldSourceEnabled == false){
        UA_ReadValueId rvid;
        UA_ReadValueId_init(&rvid);
        rvid.nodeId = params->publishedVariable;
        rvid.attributeId = params->attributeId;
        rvid.indexRange = params->indexRange;
        *value = readWithSession(psm->sc.server, &psm->sc.server->adminSession,
                                 &rvid, UA_TIMESTAMPSTORETURN_BOTH);
    } else {
        *value = **field->config.field.variable.rtValueSource.staticValueSource;
        value->value.storageType = UA_VARIANT_DATA_NODELETE;
    }
}

UA_AddPublishedDataSetResult
UA_PublishedDataSet_create(UA_PubSubManager *psm,
                           const UA_PublishedDataSetConfig *publishedDataSetConfig,
                           UA_NodeId *pdsIdentifier) {
    UA_AddPublishedDataSetResult result = {UA_STATUSCODE_BADINVALIDARGUMENT, 0, NULL, {0, 0}};
    if(!psm) {
        result.addResult = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    if(!publishedDataSetConfig) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PublishedDataSet creation failed. No config passed in.");
        return result;
    }

    if(publishedDataSetConfig->publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS){
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PublishedDataSet creation failed. Unsupported PublishedDataSet type.");
        return result;
    }

    if(UA_String_isEmpty(&publishedDataSetConfig->name)) {
        // DataSet has to have a valid name
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PublishedDataSet creation failed. Invalid name.");
        return result;
    }

    if(UA_PublishedDataSet_findByName(psm, publishedDataSetConfig->name)) {
        // DataSet name has to be unique in the publisher
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PublishedDataSet creation failed. DataSet with the same name already exists.");
        result.addResult = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        return result;
    }

    /* Create new PDS and add to UA_PubSubManager */
    UA_PublishedDataSet *newPDS = (UA_PublishedDataSet *)
        UA_calloc(1, sizeof(UA_PublishedDataSet));
    if(!newPDS) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PublishedDataSet creation failed. Out of Memory.");
        result.addResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return result;
    }
    TAILQ_INIT(&newPDS->fields);

    newPDS->head.componentType = UA_PUBSUBCOMPONENT_PUBLISHEDDATASET;

    UA_PublishedDataSetConfig *newConfig = &newPDS->config;

    /* Deep copy the given connection config */
    UA_StatusCode res = UA_PublishedDataSetConfig_copy(publishedDataSetConfig, newConfig);
    if(res != UA_STATUSCODE_GOOD){
        UA_free(newPDS);
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PublishedDataSet creation failed. Configuration copy failed.");
        result.addResult = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    /* TODO: Parse template config and add fields (later PubSub batch) */
    if(newConfig->publishedDataSetType == UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE) {
    }

    /* Fill the DataSetMetaData */
    UA_EventLoop *el = psm->sc.server->config.eventLoop;
    result.configurationVersion.majorVersion =
        UA_PubSubConfigurationVersionTimeDifference(el->dateTime_now(el));
    result.configurationVersion.minorVersion =
        UA_PubSubConfigurationVersionTimeDifference(el->dateTime_now(el));
    switch(newConfig->publishedDataSetType) {
    case UA_PUBSUB_DATASET_PUBLISHEDEVENTS_TEMPLATE:
        res = UA_STATUSCODE_BADNOTSUPPORTED;
        break;
    case UA_PUBSUB_DATASET_PUBLISHEDEVENTS:
        res = UA_STATUSCODE_BADNOTSUPPORTED;
        break;
    case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
        newPDS->dataSetMetaData.configurationVersion.majorVersion =
            UA_PubSubConfigurationVersionTimeDifference(el->dateTime_now(el));
        newPDS->dataSetMetaData.configurationVersion.minorVersion =
            UA_PubSubConfigurationVersionTimeDifference(el->dateTime_now(el));
        newPDS->dataSetMetaData.description = UA_LOCALIZEDTEXT_ALLOC("", "");
        newPDS->dataSetMetaData.dataSetClassId = UA_GUID_NULL;
        res = UA_String_copy(&newConfig->name, &newPDS->dataSetMetaData.name);
        break;
    case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
        res = UA_DataSetMetaDataType_copy(&newConfig->config.itemsTemplate.metaData,
                                          &newPDS->dataSetMetaData);
        break;
    default:
        res = UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Abort? */
    result.addResult = res;
    if(result.addResult != UA_STATUSCODE_GOOD) {
        UA_PublishedDataSetConfig_clear(newConfig);
        UA_free(newPDS);
        return result;
    }

    /* Insert into the queue of the manager */
    TAILQ_INSERT_TAIL(&psm->publishedDataSets, newPDS, listEntry);
    psm->publishedDataSetsSize++;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* Create representation and unique id */
    addPublishedDataItemsRepresentation(psm->sc.server, newPDS);
#else
    /* Generate unique nodeId */
    UA_PubSubManager_generateUniqueNodeId(psm, &newPDS->identifier);
#endif

    /* Cache the log string */
    char tmpLogIdStr[128];
    mp_snprintf(tmpLogIdStr, 128, "PublishedDataset %N\t| ", newPDS->head.identifier);
    newPDS->head.logIdString = UA_STRING_ALLOC(tmpLogIdStr);

    UA_LOG_INFO_PUBSUB(psm->logging, newPDS, "DataSet created");

    /* Return the created identifier */
    if(pdsIdentifier)
        UA_NodeId_copy(&newPDS->head.identifier, pdsIdentifier);
    return result;
}

UA_StatusCode
UA_PublishedDataSet_remove(UA_PubSubManager *psm, UA_PublishedDataSet *pds) {
    if(pds->configurationFreezeCounter > 0) {
        UA_LOG_WARNING_PUBSUB(psm->logging, pds,
                              "Remove PublishedDataSet failed. PublishedDataSet is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Search for referenced writers -> delete this writers. (Standard: writer
     * must be connected with PDS) */
    UA_PubSubConnection *conn;
    TAILQ_FOREACH(conn, &psm->connections, listEntry) {
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &conn->writerGroups, listEntry) {
            UA_DataSetWriter *dsw, *tmpWriter;
            LIST_FOREACH_SAFE(dsw, &wg->writers, listEntry, tmpWriter) {
                if(dsw->connectedDataSet == pds)
                    UA_DataSetWriter_remove(psm, dsw);
            }
        }
    }

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(psm->sc.server, pds->head.identifier, true);
#endif

    UA_LOG_INFO_PUBSUB(psm->logging, pds, "DataSet deleted");

    /* Unlink from the server */
    TAILQ_REMOVE(&psm->publishedDataSets, pds, listEntry);
    psm->publishedDataSetsSize--;

    /* Clean up the PublishedDataSet */
    UA_DataSetField *field, *tmpField;
    TAILQ_FOREACH_SAFE(field, &pds->fields, listEntry, tmpField) {
        /* Code in this block is a duplication of similar code in UA_DataSetField_remove, but
         * this is intentional. We don't want to call UA_DataSetField_remove here as that
         * function regenerates DataSetMetaData, which is not necessary if we want to
         * clear the whole PDS anyway. */
        if(field->configurationFrozen)
            UA_LOG_WARNING_PUBSUB(psm->logging, pds, "Clearing a frozen field.");
        field->fieldMetaData.arrayDimensions = NULL;
        field->fieldMetaData.properties = NULL;
        field->fieldMetaData.name = UA_STRING_NULL;
        field->fieldMetaData.description.locale = UA_STRING_NULL;
        field->fieldMetaData.description.text = UA_STRING_NULL;
        UA_DataSetField_clear(field);
        TAILQ_REMOVE(&pds->fields, field, listEntry);
        UA_free(field);
    }
    UA_PublishedDataSetConfig_clear(&pds->config);
    UA_DataSetMetaDataType_clear(&pds->dataSetMetaData);
    UA_PubSubComponentHead_clear(&pds->head);
    UA_free(pds);

    return UA_STATUSCODE_GOOD;
}

UA_SubscribedDataSet *
UA_SubscribedDataSet_find(UA_PubSubManager *psm, const UA_NodeId id) {
    if(!psm)
        return NULL;
    UA_SubscribedDataSet *sds;
    TAILQ_FOREACH(sds, &psm->subscribedDataSets, listEntry) {
        if(UA_NodeId_equal(&id, &sds->head.identifier))
            return sds;
    }
    return NULL;
}

UA_SubscribedDataSet *
UA_SubscribedDataSet_findByName(UA_PubSubManager *psm, const UA_String name) {
    if(!psm)
        return NULL;
    UA_SubscribedDataSet *sds;
    TAILQ_FOREACH(sds, &psm->subscribedDataSets, listEntry) {
        if(UA_String_equal(&name, &sds->config.name))
            return sds;
    }
    return NULL;
}

UA_StatusCode
UA_SubscribedDataSetConfig_copy(const UA_SubscribedDataSetConfig *src,
                                UA_SubscribedDataSetConfig *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_SubscribedDataSetConfig));
    res = UA_DataSetMetaDataType_copy(&src->dataSetMetaData, &dst->dataSetMetaData);
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_TargetVariablesDataType_copy(&src->subscribedDataSet.target,
                                           &dst->subscribedDataSet.target);

    if(res != UA_STATUSCODE_GOOD)
        UA_SubscribedDataSetConfig_clear(dst);
    return res;
}

void
UA_SubscribedDataSetConfig_clear(UA_SubscribedDataSetConfig *sdsConfig) {
    UA_String_clear(&sdsConfig->name);
    UA_DataSetMetaDataType_clear(&sdsConfig->dataSetMetaData);
    UA_TargetVariablesDataType_clear(&sdsConfig->subscribedDataSet.target);
}

static UA_StatusCode
addSubscribedDataSet(UA_PubSubManager *psm,
                     const UA_SubscribedDataSetConfig *sdsConfig,
                     UA_NodeId *sdsIdentifier) {
    if(!psm)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex, 1);

    if(!sdsConfig) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "SubscribedDataSet creation failed. No config passed in.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_SubscribedDataSetConfig tmpSubscribedDataSetConfig;
    memset(&tmpSubscribedDataSetConfig, 0, sizeof(UA_SubscribedDataSetConfig));
    UA_StatusCode res = UA_SubscribedDataSetConfig_copy(sdsConfig,
                                                        &tmpSubscribedDataSetConfig);
    if(res != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "SubscribedDataSet creation failed. Configuration copy failed.");
        return res;
    }

    /* Create new PDS and add to UA_PubSubManager */
    UA_SubscribedDataSet *newSubscribedDataSet = (UA_SubscribedDataSet *)
            UA_calloc(1, sizeof(UA_SubscribedDataSet));
    if(!newSubscribedDataSet) {
        UA_SubscribedDataSetConfig_clear(&tmpSubscribedDataSetConfig);
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "SubscribedDataSet creation failed. Out of Memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    newSubscribedDataSet->head.componentType = UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET;
    newSubscribedDataSet->config = tmpSubscribedDataSetConfig;
    newSubscribedDataSet->connectedReader = NULL;

    TAILQ_INSERT_TAIL(&psm->subscribedDataSets, newSubscribedDataSet, listEntry);
    psm->subscribedDataSetsSize++;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addSubscribedDataSetRepresentation(psm->sc.server, newSubscribedDataSet);
#else
    UA_PubSubManager_generateUniqueNodeId(psm, &newSubscribedDataSet->identifier);
#endif

    if(sdsIdentifier)
        UA_NodeId_copy(&newSubscribedDataSet->head.identifier, sdsIdentifier);

    return UA_STATUSCODE_GOOD;
}

void
UA_SubscribedDataSet_remove(UA_PubSubManager *psm, UA_SubscribedDataSet *sds) {
    /* Search for referenced readers */
    UA_PubSubConnection *conn;
    TAILQ_FOREACH(conn, &psm->connections, listEntry) {
        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &conn->readerGroups, listEntry) {
            UA_DataSetReader *dsr, *tmpReader;
            LIST_FOREACH_SAFE(dsr, &rg->readers, listEntry, tmpReader) {
                /* TODO: What if the reader is still operational?
                 * This should be checked before calling _remove. */
                if(dsr == sds->connectedReader)
                    UA_DataSetReader_remove(psm, dsr);
            }
        }
    }

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(psm->sc.server, sds->head.identifier, true);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&psm->subscribedDataSets, sds, listEntry);
    psm->subscribedDataSetsSize--;

    /* Clean up */
    UA_SubscribedDataSetConfig_clear(&sds->config);
    UA_PubSubComponentHead_clear(&sds->head);
    UA_free(sds);
}

/**************/
/* Server API */
/**************/

UA_StatusCode
UA_Server_getPublishedDataSetConfig(UA_Server *server, const UA_NodeId pdsId,
                                    UA_PublishedDataSetConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(getPSM(server), pdsId);
    UA_StatusCode res = (pds) ?
        UA_PublishedDataSetConfig_copy(&pds->config, config) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getPublishedDataSetMetaData(UA_Server *server, const UA_NodeId pdsId,
                                      UA_DataSetMetaDataType *metaData) {
    if(!server || !metaData)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(getPSM(server), pdsId);
    UA_StatusCode res = (pds) ?
        UA_DataSetMetaDataType_copy(&pds->dataSetMetaData, metaData) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_DataSetFieldResult
UA_Server_addDataSetField(UA_Server *server, const UA_NodeId publishedDataSet,
                          const UA_DataSetFieldConfig *fieldConfig,
                          UA_NodeId *fieldIdentifier) {
    UA_DataSetFieldResult res;
    if(!server || !fieldConfig) {
        memset(&res, 0, sizeof(UA_DataSetFieldResult));
        res.result = UA_STATUSCODE_BADINVALIDARGUMENT;
        return res;
    }
    UA_LOCK(&server->serviceMutex);
    res = UA_DataSetField_create(getPSM(server), publishedDataSet, fieldConfig, fieldIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_DataSetFieldResult
UA_Server_removeDataSetField(UA_Server *server, const UA_NodeId dsf) {
    UA_DataSetFieldResult res;
    if(!server) {
        memset(&res, 0, sizeof(UA_DataSetFieldResult));
        res.result = UA_STATUSCODE_BADINVALIDARGUMENT;
        return res;
    }
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_DataSetField *field = UA_DataSetField_find(psm, dsf);
    res = UA_DataSetField_remove(psm, field);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_getDataSetFieldConfig(UA_Server *server, const UA_NodeId dsfId,
                                UA_DataSetFieldConfig *config) {
    if(!server || !config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_DataSetField *dsf = UA_DataSetField_find(getPSM(server), dsfId);
    UA_StatusCode res = (dsf) ?
        UA_DataSetFieldConfig_copy(&dsf->config, config) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_AddPublishedDataSetResult
UA_Server_addPublishedDataSet(UA_Server *server,
                              const UA_PublishedDataSetConfig *publishedDataSetConfig,
                              UA_NodeId *pdsIdentifier) {
    UA_AddPublishedDataSetResult res;
    if(!server || !publishedDataSetConfig) {
        memset(&res, 0, sizeof(UA_AddPublishedDataSetResult));
        res.addResult = UA_STATUSCODE_BADINTERNALERROR;
        return res;
    }
    UA_LOCK(&server->serviceMutex);
    res = UA_PublishedDataSet_create(getPSM(server), publishedDataSetConfig, pdsIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_removePublishedDataSet(UA_Server *server, const UA_NodeId pdsId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, pdsId);
    UA_StatusCode res = (pds) ?
        UA_PublishedDataSet_remove(psm, pds) : UA_STATUSCODE_BADNOTFOUND;
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_addSubscribedDataSet(UA_Server *server,
                               const UA_SubscribedDataSetConfig *sdsConfig,
                               UA_NodeId *sdsIdentifier) {
    if(!server || !sdsConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = addSubscribedDataSet(getPSM(server), sdsConfig, sdsIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_Server_removeSubscribedDataSet(UA_Server *server, const UA_NodeId sdsId) {
    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK(&server->serviceMutex);
    UA_PubSubManager *psm = getPSM(server);
    UA_SubscribedDataSet *sds = UA_SubscribedDataSet_find(psm, sdsId);
    UA_StatusCode res = UA_STATUSCODE_BADNOTFOUND;
    if(sds) {
        UA_SubscribedDataSet_remove(psm, sds);
        res = UA_STATUSCODE_GOOD;
    }
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

#endif /* UA_ENABLE_PUBSUB */
