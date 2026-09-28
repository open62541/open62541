/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server_pubsub.h>

#ifdef UA_ENABLE_PUBSUB

#include "ua_pubsub_internal.h"

/* Mapping between the Part 14 configuration DataTypes and the internal
 * UA_*Config structures.
 *
 * The _fromDataType converters return a view that borrows the memory of the
 * source DataType and must not outlive it. That suffices for the UA_*_create
 * functions, which deep-copy the config. Only a String PublisherId is
 * allocated. Clear the view with the matching _clearView function, never with
 * _clear (that would free borrowed memory).
 *
 * Part 14 fields without an internal counterpart are marked "TODO Part14". */

/* DECODED when read from a file, DECODED_NODELETE when built in code */
static UA_Boolean
eoDecoded(const UA_ExtensionObject *eo) {
    return (eo->encoding == UA_EXTENSIONOBJECT_DECODED ||
            eo->encoding == UA_EXTENSIONOBJECT_DECODED_NODELETE);
}

UA_StatusCode
UA_PubSubConnectionConfig_fromDataType(const UA_PubSubConnectionDataType *src,
                                       UA_PubSubConnectionConfig *dst) {
    memset(dst, 0, sizeof(UA_PubSubConnectionConfig));

    dst->name = src->name;
    dst->enabled = src->enabled;
    dst->transportProfileUri = src->transportProfileUri;
    dst->connectionProperties.map = src->connectionProperties;
    dst->connectionProperties.mapSize = src->connectionPropertiesSize;

    /* The internal address is a Variant, the ExtensionObject must be decoded */
    if(!eoDecoded(&src->address))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_Variant_setScalar(&dst->address, src->address.content.decoded.data,
                         src->address.content.decoded.type);

    /* Optional TransportSettings */
    if(eoDecoded(&src->transportSettings)) {
        UA_Variant_setScalar(&dst->connectionTransportSettings,
                             src->transportSettings.content.decoded.data,
                             src->transportSettings.content.decoded.type);
    }

    /* An empty PublisherId keeps Byte 0, the caller may set the default */
    if(!UA_Variant_isEmpty(&src->publisherId))
        return UA_PublisherId_fromVariant(&dst->publisherId, &src->publisherId);

    return UA_STATUSCODE_GOOD;
}

void
UA_PubSubConnectionConfig_clearView(UA_PubSubConnectionConfig *config) {
    UA_PublisherId_clear(&config->publisherId);
    memset(config, 0, sizeof(UA_PubSubConnectionConfig));
}

UA_StatusCode
UA_WriterGroupConfig_fromDataType(const UA_WriterGroupDataType *src,
                                  UA_WriterGroupConfig *dst) {
    memset(dst, 0, sizeof(UA_WriterGroupConfig));

    dst->name = src->name;
    dst->enabled = src->enabled;
    dst->writerGroupId = src->writerGroupId;
    dst->publishingInterval = src->publishingInterval;
    dst->keepAliveTime = src->keepAliveTime;
    dst->priority = src->priority;
    dst->securityMode = src->securityMode;
    dst->securityGroupId = src->securityGroupId;
    dst->securityKeyServices = src->securityKeyServices;
    dst->securityKeyServicesSize = src->securityKeyServicesSize;
    dst->maxNetworkMessageSize = src->maxNetworkMessageSize;
    dst->headerLayoutUri = src->headerLayoutUri;
    dst->localeIds = src->localeIds;
    dst->localeIdsSize = src->localeIdsSize;
    dst->transportSettings = src->transportSettings;
    dst->messageSettings = src->messageSettings;
    dst->groupProperties.map = src->groupProperties;
    dst->groupProperties.mapSize = src->groupPropertiesSize;

    /* Non-standard. Max DataSetMessages per NetworkMessage (size permitting) */
    dst->maxEncapsulatedDataSetMessageCount = 255;

    /* The type of the MessageSettings defines the encoding, default UADP */
    dst->encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    if(UA_ExtensionObject_hasDecodedType(&src->messageSettings,
           &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE])) {
#ifdef UA_ENABLE_JSON_ENCODING
        dst->encodingMimeType = UA_PUBSUB_ENCODING_JSON;
#else
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
#endif
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetWriterConfig_fromDataType(const UA_DataSetWriterDataType *src,
                                    UA_DataSetWriterConfig *dst) {
    memset(dst, 0, sizeof(UA_DataSetWriterConfig));

    dst->name = src->name;
    dst->enabled = src->enabled;
    dst->dataSetWriterId = src->dataSetWriterId;
    dst->dataSetFieldContentMask = src->dataSetFieldContentMask;
    dst->keyFrameCount = src->keyFrameCount;
    dst->dataSetName = src->dataSetName;
    dst->messageSettings = src->messageSettings;
    dst->transportSettings = src->transportSettings;
    dst->dataSetWriterProperties.map = src->dataSetWriterProperties;
    dst->dataSetWriterProperties.mapSize = src->dataSetWriterPropertiesSize;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ReaderGroupConfig_fromDataType(const UA_ReaderGroupDataType *src,
                                  UA_ReaderGroupConfig *dst) {
    memset(dst, 0, sizeof(UA_ReaderGroupConfig));

    dst->name = src->name;
    dst->enabled = src->enabled;
    dst->securityMode = src->securityMode;
    dst->securityGroupId = src->securityGroupId;
    dst->securityKeyServices = src->securityKeyServices;
    dst->securityKeyServicesSize = src->securityKeyServicesSize;
    dst->maxNetworkMessageSize = src->maxNetworkMessageSize;
    dst->transportSettings = src->transportSettings;
    dst->messageSettings = src->messageSettings;
    dst->groupProperties.map = src->groupProperties;
    dst->groupProperties.mapSize = src->groupPropertiesSize;

    /* The encoding follows the MessageSettings of the readers, default UADP */
    dst->encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    if(src->dataSetReadersSize > 0 &&
       UA_ExtensionObject_hasDecodedType(&src->dataSetReaders[0].messageSettings,
           &UA_TYPES[UA_TYPES_JSONDATASETREADERMESSAGEDATATYPE]))
        dst->encodingMimeType = UA_PUBSUB_ENCODING_JSON;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetReaderConfig_fromDataType(const UA_DataSetReaderDataType *src,
                                    UA_DataSetReaderConfig *dst) {
    memset(dst, 0, sizeof(UA_DataSetReaderConfig));

    dst->name = src->name;
    dst->enabled = src->enabled;
    dst->writerGroupId = src->writerGroupId;
    dst->dataSetWriterId = src->dataSetWriterId;
    dst->dataSetMetaData = src->dataSetMetaData;
    dst->dataSetFieldContentMask = src->dataSetFieldContentMask;
    dst->messageReceiveTimeout = src->messageReceiveTimeout;
    dst->messageSettings = src->messageSettings;
    dst->transportSettings = src->transportSettings;
    dst->keyFrameCount = src->keyFrameCount;
    dst->headerLayoutUri = src->headerLayoutUri;
    dst->securityMode = src->securityMode;
    dst->securityGroupId = src->securityGroupId;
    dst->securityKeyServices = src->securityKeyServices;
    dst->securityKeyServicesSize = src->securityKeyServicesSize;
    dst->dataSetReaderProperties.map = src->dataSetReaderProperties;
    dst->dataSetReaderProperties.mapSize = src->dataSetReaderPropertiesSize;

    /* Inline TargetVariables or a StandaloneSubscribedDataSet referenced by
     * name. SubscribedDataSetMirror is not supported. */
    const UA_ExtensionObject *sds = &src->subscribedDataSet;
    if(eoDecoded(sds)) {
        if(sds->content.decoded.type == &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]) {
            dst->subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
            dst->subscribedDataSet.target =
                *(UA_TargetVariablesDataType*)sds->content.decoded.data;
        } else if(sds->content.decoded.type ==
                  &UA_TYPES[UA_TYPES_STANDALONESUBSCRIBEDDATASETREFDATATYPE]) {
            UA_StandaloneSubscribedDataSetRefDataType *ref =
                (UA_StandaloneSubscribedDataSetRefDataType*)sds->content.decoded.data;
            dst->linkedStandaloneSubscribedDataSetName = ref->dataSetName;
            dst->subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
        } else if(sds->content.decoded.type ==
                  &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE]) {
            return UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else {
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    }

    /* A given PublisherId filters the received messages (String ids are
     * allocated). Empty leaves Byte 0 without the filter. */
    if(!UA_Variant_isEmpty(&src->publisherId)) {
        dst->publisherIdFilterEnabled = true;
        return UA_PublisherId_fromVariant(&dst->publisherId, &src->publisherId);
    }

    return UA_STATUSCODE_GOOD;
}

void
UA_DataSetReaderConfig_clearView(UA_DataSetReaderConfig *config) {
    UA_PublisherId_clear(&config->publisherId);
    memset(config, 0, sizeof(UA_DataSetReaderConfig));
}

UA_StatusCode
UA_PublishedDataSetConfig_fromDataType(const UA_PublishedDataSetDataType *src,
                                       UA_PublishedDataSetConfig *dst) {
    memset(dst, 0, sizeof(UA_PublishedDataSetConfig));

    dst->name = src->name;
    dst->dataSetFolder = src->dataSetFolder;
    dst->dataSetFolderSize = src->dataSetFolderSize;
    dst->extensionFields.map = src->extensionFields;
    dst->extensionFields.mapSize = src->extensionFieldsSize;

    /* Only PublishedDataItems are supported so far */
    if(!eoDecoded(&src->dataSetSource))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    const UA_DataType *sourceType = src->dataSetSource.content.decoded.type;
    if(sourceType == &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]) {
        dst->publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    } else if(sourceType == &UA_TYPES[UA_TYPES_PUBLISHEDEVENTSDATATYPE]) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    } else {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* One published variable per metadata field */
    UA_PublishedDataItemsDataType *pdi = (UA_PublishedDataItemsDataType*)
        src->dataSetSource.content.decoded.data;
    if(pdi->publishedDataSize != src->dataSetMetaData.fieldsSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* TODO Part14: the config drops the DataSetMetaData description and
     * dataSetClassId; the PDS rebuilds its metadata from the field configs */

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetFieldConfig_fromDataType(const UA_PublishedDataSetDataType *src,
                                   size_t fieldIndex, UA_DataSetFieldConfig *dst) {
    memset(dst, 0, sizeof(UA_DataSetFieldConfig));

    if(!UA_ExtensionObject_hasDecodedType(&src->dataSetSource,
           &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PublishedDataItemsDataType *pdi = (UA_PublishedDataItemsDataType*)
        src->dataSetSource.content.decoded.data;
    if(fieldIndex >= pdi->publishedDataSize ||
       fieldIndex >= src->dataSetMetaData.fieldsSize)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    const UA_FieldMetaData *fmd = &src->dataSetMetaData.fields[fieldIndex];
    dst->dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dst->field.variable.configurationVersion =
        src->dataSetMetaData.configurationVersion;
    dst->field.variable.fieldNameAlias = fmd->name;
    dst->field.variable.promotedField =
        (fmd->fieldFlags & UA_DATASETFIELDFLAGS_PROMOTEDFIELD) != 0;
    dst->field.variable.publishParameters = pdi->publishedData[fieldIndex];
    dst->field.variable.maxStringLength = fmd->maxStringLength;
    dst->field.variable.description = fmd->description;
    dst->field.variable.dataSetFieldId = fmd->dataSetFieldId;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_PublishedDataSet_addFieldsFromDataType(UA_PubSubManager *psm,
                                          const UA_NodeId pdsId,
                                          const UA_PublishedDataSetDataType *src) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    for(size_t i = 0; i < src->dataSetMetaData.fieldsSize; i++) {
        UA_DataSetFieldConfig fc;
        res = UA_DataSetFieldConfig_fromDataType(src, i, &fc);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        UA_NodeId fieldId;
        res = UA_DataSetField_create(psm, pdsId, &fc, &fieldId).result;
        if(res != UA_STATUSCODE_GOOD)
            return res;

        UA_DataSetField *field = UA_DataSetField_find(psm, fieldId);
        if(!field)
            return UA_STATUSCODE_BADINTERNALERROR;
        UA_FieldMetaData_clear(&field->fieldMetaData);
        res = UA_FieldMetaData_copy(&src->dataSetMetaData.fields[i],
                                    &field->fieldMetaData);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, pdsId);
    if(!pds)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_DataSetMetaDataType md;
    res = UA_DataSetMetaDataType_copy(&src->dataSetMetaData, &md);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_DataSetMetaDataType_clear(&pds->dataSetMetaData);
    pds->dataSetMetaData = md;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_SubscribedDataSetConfig_fromDataType(const UA_StandaloneSubscribedDataSetDataType *src,
                                        UA_SubscribedDataSetConfig *dst) {
    memset(dst, 0, sizeof(UA_SubscribedDataSetConfig));

    dst->name = src->name;
    dst->dataSetMetaData = src->dataSetMetaData;
    dst->dataSetFolder = src->dataSetFolder;
    dst->dataSetFolderSize = src->dataSetFolderSize;

    const UA_ExtensionObject *sds = &src->subscribedDataSet;
    if(UA_ExtensionObject_hasDecodedType(sds,
           &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE])) {
        dst->subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
        dst->subscribedDataSet.target =
            *(UA_TargetVariablesDataType*)sds->content.decoded.data;
    } else if(UA_ExtensionObject_hasDecodedType(sds,
                  &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE])) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    } else {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    return UA_STATUSCODE_GOOD;
}

/* The _toDataType converters deep-copy a view that borrows the config. The
 * config's enabled flag only controls auto-enabling at creation, so the
 * caller sets dst->enabled from the component state. */

static void
setEoView(UA_ExtensionObject *eo, const UA_Variant *v) {
    if(v->data && UA_Variant_isScalar(v))
        UA_ExtensionObject_setValueNoDelete(eo, v->data, v->type);
}

UA_StatusCode
UA_PubSubConnectionConfig_toDataType(const UA_PubSubConnectionConfig *src,
                                     UA_PubSubConnectionDataType *dst) {
    UA_PubSubConnectionDataType view;
    UA_PubSubConnectionDataType_init(&view);
    view.name = src->name;
    view.transportProfileUri = src->transportProfileUri;
    view.connectionProperties = src->connectionProperties.map;
    view.connectionPropertiesSize = src->connectionProperties.mapSize;
    UA_PublisherId_toVariant(&src->publisherId, &view.publisherId);
    setEoView(&view.address, &src->address);
    setEoView(&view.transportSettings, &src->connectionTransportSettings);
    return UA_PubSubConnectionDataType_copy(&view, dst);
}

UA_StatusCode
UA_WriterGroupConfig_toDataType(const UA_WriterGroupConfig *src,
                                UA_WriterGroupDataType *dst) {
    UA_WriterGroupDataType view;
    UA_WriterGroupDataType_init(&view);
    view.name = src->name;
    view.writerGroupId = src->writerGroupId;
    view.publishingInterval = src->publishingInterval;
    view.keepAliveTime = src->keepAliveTime;
    view.priority = src->priority;
    view.securityMode = src->securityMode;
    view.maxNetworkMessageSize = src->maxNetworkMessageSize;
    view.securityGroupId = src->securityGroupId;
    view.headerLayoutUri = src->headerLayoutUri;
    view.transportSettings = src->transportSettings;
    view.messageSettings = src->messageSettings;
    view.groupProperties = src->groupProperties.map;
    view.groupPropertiesSize = src->groupProperties.mapSize;
    view.localeIds = src->localeIds;
    view.localeIdsSize = src->localeIdsSize;
    view.securityKeyServices = src->securityKeyServices;
    view.securityKeyServicesSize = src->securityKeyServicesSize;
    return UA_WriterGroupDataType_copy(&view, dst);
}

UA_StatusCode
UA_DataSetWriterConfig_toDataType(const UA_DataSetWriterConfig *src,
                                  UA_DataSetWriterDataType *dst) {
    UA_DataSetWriterDataType view;
    UA_DataSetWriterDataType_init(&view);
    view.name = src->name;
    view.dataSetWriterId = src->dataSetWriterId;
    view.keyFrameCount = src->keyFrameCount;
    view.dataSetFieldContentMask = src->dataSetFieldContentMask;
    view.dataSetName = src->dataSetName;
    view.messageSettings = src->messageSettings;
    view.transportSettings = src->transportSettings;
    view.dataSetWriterProperties = src->dataSetWriterProperties.map;
    view.dataSetWriterPropertiesSize = src->dataSetWriterProperties.mapSize;
    return UA_DataSetWriterDataType_copy(&view, dst);
}

UA_StatusCode
UA_ReaderGroupConfig_toDataType(const UA_ReaderGroupConfig *src,
                                UA_ReaderGroupDataType *dst) {
    UA_ReaderGroupDataType view;
    UA_ReaderGroupDataType_init(&view);
    view.name = src->name;
    view.securityMode = src->securityMode;
    view.maxNetworkMessageSize = src->maxNetworkMessageSize;
    view.securityGroupId = src->securityGroupId;
    view.transportSettings = src->transportSettings;
    view.messageSettings = src->messageSettings;
    view.groupProperties = src->groupProperties.map;
    view.groupPropertiesSize = src->groupProperties.mapSize;
    view.securityKeyServices = src->securityKeyServices;
    view.securityKeyServicesSize = src->securityKeyServicesSize;
    return UA_ReaderGroupDataType_copy(&view, dst);
}

UA_StatusCode
UA_DataSetReaderConfig_toDataType(const UA_DataSetReaderConfig *src,
                                  UA_DataSetReaderDataType *dst) {
    UA_DataSetReaderDataType view;
    UA_DataSetReaderDataType_init(&view);
    view.name = src->name;
    view.writerGroupId = src->writerGroupId;
    view.dataSetWriterId = src->dataSetWriterId;
    view.dataSetFieldContentMask = src->dataSetFieldContentMask;
    view.messageReceiveTimeout = src->messageReceiveTimeout;
    view.keyFrameCount = src->keyFrameCount;
    view.securityMode = src->securityMode;
    view.dataSetMetaData = src->dataSetMetaData;
    view.messageSettings = src->messageSettings;
    view.transportSettings = src->transportSettings;
    view.headerLayoutUri = src->headerLayoutUri;
    view.securityGroupId = src->securityGroupId;
    view.securityKeyServices = src->securityKeyServices;
    view.securityKeyServicesSize = src->securityKeyServicesSize;
    view.dataSetReaderProperties = src->dataSetReaderProperties.map;
    view.dataSetReaderPropertiesSize = src->dataSetReaderProperties.mapSize;
    UA_PublisherId_toVariant(&src->publisherId, &view.publisherId);

    /* A standalone SubscribedDataSet takes precedence over TargetVariables */
    UA_StandaloneSubscribedDataSetRefDataType ref;
    UA_StandaloneSubscribedDataSetRefDataType_init(&ref);
    if(!UA_String_isEmpty(&src->linkedStandaloneSubscribedDataSetName)) {
        ref.dataSetName = src->linkedStandaloneSubscribedDataSetName;
        UA_ExtensionObject_setValueNoDelete(&view.subscribedDataSet, &ref,
            &UA_TYPES[UA_TYPES_STANDALONESUBSCRIBEDDATASETREFDATATYPE]);
    } else if(src->subscribedDataSetType == UA_PUBSUB_SDS_TARGET) {
        UA_ExtensionObject_setValueNoDelete(&view.subscribedDataSet,
            (void*)(uintptr_t)&src->subscribedDataSet.target,
            &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]);
    }
    return UA_DataSetReaderDataType_copy(&view, dst);
}

/* Export from the component, not only the config: its DataSetMetaData and
 * DataSetFields define the dataSetMetaData and dataSetSource. A template only
 * defines the initial fields. */
UA_StatusCode
UA_PublishedDataSet_toDataType(const UA_PublishedDataSet *pds,
                               UA_PublishedDataSetDataType *dst) {
    if(pds->config.publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS &&
       pds->config.publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    UA_PublishedDataSetDataType_init(dst);

    UA_StatusCode res = UA_String_copy(&pds->config.name, &dst->name);
    res |= UA_DataSetMetaDataType_copy(&pds->dataSetMetaData, &dst->dataSetMetaData);
    res |= UA_Array_copy(pds->config.dataSetFolder, pds->config.dataSetFolderSize,
                         (void**)&dst->dataSetFolder, &UA_TYPES[UA_TYPES_STRING]);
    if(res == UA_STATUSCODE_GOOD)
        dst->dataSetFolderSize = pds->config.dataSetFolderSize;
    res |= UA_Array_copy(pds->config.extensionFields.map,
                         pds->config.extensionFields.mapSize,
                         (void**)&dst->extensionFields,
                         &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    if(res == UA_STATUSCODE_GOOD)
        dst->extensionFieldsSize = pds->config.extensionFields.mapSize;

    UA_PublishedDataItemsDataType *pdi = UA_PublishedDataItemsDataType_new();
    if(!pdi) {
        UA_PublishedDataSetDataType_clear(dst);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    pdi->publishedData = (UA_PublishedVariableDataType*)
        UA_Array_new(pds->fieldSize, &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
    if(pds->fieldSize > 0 && !pdi->publishedData) {
        UA_free(pdi);
        UA_PublishedDataSetDataType_clear(dst);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    pdi->publishedDataSize = pds->fieldSize;

    size_t i = 0;
    UA_DataSetField *dsf;
    TAILQ_FOREACH(dsf, &pds->fields, listEntry) {
        res |= UA_PublishedVariableDataType_copy(
            &dsf->config.field.variable.publishParameters, &pdi->publishedData[i]);
        i++;
    }
    UA_ExtensionObject_setValue(&dst->dataSetSource, pdi,
                                &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);

    if(res != UA_STATUSCODE_GOOD)
        UA_PublishedDataSetDataType_clear(dst);
    return res;
}

UA_StatusCode
UA_SubscribedDataSetConfig_toDataType(const UA_SubscribedDataSetConfig *src,
                                      UA_StandaloneSubscribedDataSetDataType *dst) {
    if(src->subscribedDataSetType != UA_PUBSUB_SDS_TARGET)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    UA_StandaloneSubscribedDataSetDataType view;
    UA_StandaloneSubscribedDataSetDataType_init(&view);
    view.name = src->name;
    view.dataSetMetaData = src->dataSetMetaData;
    view.dataSetFolder = src->dataSetFolder;
    view.dataSetFolderSize = src->dataSetFolderSize;
    UA_ExtensionObject_setValueNoDelete(&view.subscribedDataSet,
        (void*)(uintptr_t)&src->subscribedDataSet.target,
        &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]);
    return UA_StandaloneSubscribedDataSetDataType_copy(&view, dst);
}

#endif /* UA_ENABLE_PUBSUB */
