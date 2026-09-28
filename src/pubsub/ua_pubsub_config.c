/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 * Copyright (c) 2025 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2025 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server_pubsub.h>

#if defined(UA_ENABLE_PUBSUB) && defined(UA_ENABLE_PUBSUB_FILE_CONFIG)

#include "ua_pubsub_internal.h"

/*********************/
/* Namespace Mapping */
/*********************/

/* Namespace index i+1 in the file body refers to entry [i] of the namespaces
 * array of the UABinaryFileDataType (ns0 is skipped, Part 14 v1.05 Table 88).
 * The indices are remapped to the server NamespaceArray, unknown namespaces
 * are added. Larger indices are kept and must match the server. */

static void
remapNodeId(UA_NodeId *id, const UA_UInt16 *map, size_t mapEntries) {
    if(id->namespaceIndex > 0 && id->namespaceIndex < mapEntries)
        id->namespaceIndex = map[id->namespaceIndex];
}

static void
remapQualifiedName(UA_QualifiedName *qn, const UA_UInt16 *map, size_t mapEntries) {
    if(qn->namespaceIndex > 0 && qn->namespaceIndex < mapEntries)
        qn->namespaceIndex = map[qn->namespaceIndex];
}

static void
remapKeyValuePairs(UA_KeyValuePair *kvp, size_t kvpSize,
                   const UA_UInt16 *map, size_t mapEntries) {
    for(size_t i = 0; i < kvpSize; i++)
        remapQualifiedName(&kvp[i].key, map, mapEntries);
}

static void
remapMetaData(UA_DataSetMetaDataType *md, const UA_UInt16 *map, size_t mapEntries) {
    /* Metadata with its own namespaces array is not remapped */
    if(md->namespacesSize > 0)
        return;
    for(size_t i = 0; i < md->fieldsSize; i++)
        remapNodeId(&md->fields[i].dataType, map, mapEntries);
}

static void
remapSubscribedDataSet(UA_ExtensionObject *sds,
                       const UA_UInt16 *map, size_t mapEntries) {
    if(!UA_ExtensionObject_hasDecodedType(sds,
           &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]))
        return;
    UA_TargetVariablesDataType *tvs =
        (UA_TargetVariablesDataType*)sds->content.decoded.data;
    for(size_t i = 0; i < tvs->targetVariablesSize; i++)
        remapNodeId(&tvs->targetVariables[i].targetNodeId, map, mapEntries);
}

/* Remap the namespace indices in-place */
static UA_StatusCode
remapNamespaces(UA_PubSubManager *psm, UA_PubSubConfiguration2DataType *config,
                UA_String *namespaces, size_t namespacesSize) {
    if(namespacesSize == 0)
        return UA_STATUSCODE_GOOD;

    /* Entry [0] is the OPC UA namespace */
    size_t mapEntries = namespacesSize + 1;
    UA_UInt16 *map = (UA_UInt16*)UA_calloc(mapEntries, sizeof(UA_UInt16));
    if(!map)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_Boolean identity = true;
    for(size_t i = 1; i < mapEntries; i++) {
        map[i] = addNamespace(psm->drv.server, namespaces[i - 1]);
        if(map[i] != i)
            identity = false;
    }

    /* The file namespaces already match the server NamespaceArray */
    if(identity) {
        UA_free(map);
        return UA_STATUSCODE_GOOD;
    }

    UA_LOG_INFO(psm->logging, UA_LOGCATEGORY_PUBSUB,
                "PubSub configuration file: Remapping the namespace "
                "indices to the server NamespaceArray");

    for(size_t i = 0; i < config->publishedDataSetsSize; i++) {
        UA_PublishedDataSetDataType *pds = &config->publishedDataSets[i];
        remapMetaData(&pds->dataSetMetaData, map, mapEntries);
        remapKeyValuePairs(pds->extensionFields, pds->extensionFieldsSize,
                           map, mapEntries);
        if(UA_ExtensionObject_hasDecodedType(&pds->dataSetSource,
               &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE])) {
            UA_PublishedDataItemsDataType *pdi = (UA_PublishedDataItemsDataType*)
                pds->dataSetSource.content.decoded.data;
            for(size_t j = 0; j < pdi->publishedDataSize; j++) {
                UA_PublishedVariableDataType *pv = &pdi->publishedData[j];
                remapNodeId(&pv->publishedVariable, map, mapEntries);
                for(size_t k = 0; k < pv->metaDataPropertiesSize; k++)
                    remapQualifiedName(&pv->metaDataProperties[k], map, mapEntries);
            }
        }
    }

    for(size_t i = 0; i < config->connectionsSize; i++) {
        UA_PubSubConnectionDataType *c = &config->connections[i];
        remapKeyValuePairs(c->connectionProperties, c->connectionPropertiesSize,
                           map, mapEntries);
        for(size_t j = 0; j < c->writerGroupsSize; j++) {
            UA_WriterGroupDataType *wg = &c->writerGroups[j];
            remapKeyValuePairs(wg->groupProperties, wg->groupPropertiesSize,
                               map, mapEntries);
            for(size_t k = 0; k < wg->dataSetWritersSize; k++) {
                UA_DataSetWriterDataType *dsw = &wg->dataSetWriters[k];
                remapKeyValuePairs(dsw->dataSetWriterProperties,
                                   dsw->dataSetWriterPropertiesSize,
                                   map, mapEntries);
            }
        }
        for(size_t j = 0; j < c->readerGroupsSize; j++) {
            UA_ReaderGroupDataType *rg = &c->readerGroups[j];
            remapKeyValuePairs(rg->groupProperties, rg->groupPropertiesSize,
                               map, mapEntries);
            for(size_t k = 0; k < rg->dataSetReadersSize; k++) {
                UA_DataSetReaderDataType *dsr = &rg->dataSetReaders[k];
                remapKeyValuePairs(dsr->dataSetReaderProperties,
                                   dsr->dataSetReaderPropertiesSize,
                                   map, mapEntries);
                remapMetaData(&dsr->dataSetMetaData, map, mapEntries);
                remapSubscribedDataSet(&dsr->subscribedDataSet, map, mapEntries);
            }
        }
    }

    for(size_t i = 0; i < config->subscribedDataSetsSize; i++) {
        UA_StandaloneSubscribedDataSetDataType *sds = &config->subscribedDataSets[i];
        remapMetaData(&sds->dataSetMetaData, map, mapEntries);
        remapSubscribedDataSet(&sds->subscribedDataSet, map, mapEntries);
    }

    UA_free(map);
    return UA_STATUSCODE_GOOD;
}

/******************/
/* Configuration  */
/******************/


/* Gets the PubSub configuration from an ExtensionObject containing a
 * UABinaryFileDataType. The body is either the legacy
 * PubSubConfigurationDataType or its subtype PubSubConfiguration2DataType.
 * Returns a shallow view of the configuration that borrows from src. */
static UA_StatusCode
extractPubSubConfig2FromExtensionObject(UA_PubSubManager *psm,
                                        const UA_ExtensionObject *src,
                                        UA_PubSubConfiguration2DataType *dst,
                                        UA_String **namespaces,
                                        size_t *namespacesSize) {
    if(!UA_ExtensionObject_hasDecodedType(src,
           &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE])) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration file: The file content is not a "
                     "UABinaryFileDataType");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    UA_UABinaryFileDataType *binFile =
        (UA_UABinaryFileDataType*)src->content.decoded.data;

    if(binFile->body.arrayLength != 0 || binFile->body.arrayDimensionsSize != 0 ||
       !binFile->body.data) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration file: The body must contain a "
                     "single configuration");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    UA_PubSubConfiguration2DataType_init(dst);
    if(binFile->body.type == &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATION2DATATYPE]) {
        *dst = *(UA_PubSubConfiguration2DataType*)binFile->body.data;
    } else if(binFile->body.type == &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE]) {
        /* Upgrade the legacy PubSubConfigurationDataType to a
         * PubSubConfiguration2DataType view */
        UA_PubSubConfigurationDataType *legacy =
            (UA_PubSubConfigurationDataType*)binFile->body.data;
        dst->publishedDataSetsSize = legacy->publishedDataSetsSize;
        dst->publishedDataSets = legacy->publishedDataSets;
        dst->connectionsSize = legacy->connectionsSize;
        dst->connections = legacy->connections;
        dst->enabled = legacy->enabled;
    } else {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration file: The body is not a "
                     "PubSubConfiguration2DataType or PubSubConfigurationDataType");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    *namespaces = binFile->namespaces;
    *namespacesSize = binFile->namespacesSize;
    return UA_STATUSCODE_GOOD;
}

/*********************/
/* Element Creation  */
/*********************/

static UA_StatusCode
createPublishedDataSet(UA_PubSubManager *psm,
                       const UA_PublishedDataSetDataType *pdsParams,
                       UA_NodeId *pdsIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PublishedDataSetConfig config;
    UA_StatusCode res = UA_PublishedDataSetConfig_fromDataType(pdsParams, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Invalid PublishedDataSet %S",
                     pdsParams->name);
        return res;
    }

    res = UA_PublishedDataSet_create(psm, &config, pdsIdent).addResult;
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Adding PublishedDataSet %S failed",
                     pdsParams->name);
        return res;
    }

    res = UA_PublishedDataSet_addFieldsFromDataType(psm, *pdsIdent, pdsParams);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Adding the DataSetFields to %S failed",
                     pdsParams->name);
    return res;
}

static UA_StatusCode
createSubscribedDataSet(UA_PubSubManager *psm,
                        const UA_StandaloneSubscribedDataSetDataType *sdsParams) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_SubscribedDataSetConfig config;
    UA_StatusCode res = UA_SubscribedDataSetConfig_fromDataType(sdsParams, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Invalid SubscribedDataSet %S",
                     sdsParams->name);
        return res;
    }

    res = UA_SubscribedDataSet_create(psm, &config, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Adding SubscribedDataSet %S failed",
                     sdsParams->name);
    }
    return res;
}

static UA_StatusCode
createDataSetWriter(UA_PubSubManager *psm,
                    const UA_DataSetWriterDataType *dswParams,
                    UA_NodeId writerGroupIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_DataSetWriterConfig config;
    UA_StatusCode res = UA_DataSetWriterConfig_fromDataType(dswParams, &config);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Find the PublishedDataSet by name. An empty DataSetName indicates a
     * heartbeat DataSetWriter without a connected PublishedDataSet. */
    UA_NodeId pdsIdent = UA_NODEID_NULL;
    if(!UA_String_isEmpty(&dswParams->dataSetName)) {
        UA_PublishedDataSet *pds =
            UA_PublishedDataSet_findByName(psm, dswParams->dataSetName);
        if(!pds) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "PubSub configuration: No matching PublishedDataSet %S "
                         "for DataSetWriter %S",
                         dswParams->dataSetName, dswParams->name);
            return UA_STATUSCODE_BADNOTFOUND;
        }
        pdsIdent = pds->head.identifier;
    }

    /* Create disabled, the enabled flag is applied after loading completes */
    UA_Boolean enabled = config.enabled;
    config.enabled = false;

    UA_NodeId dswIdent;
    res = UA_DataSetWriter_create(psm, writerGroupIdent, pdsIdent, &config, &dswIdent);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Creating DataSetWriter %S failed",
                     dswParams->name);
        return res;
    }

    UA_DataSetWriter *dsw = UA_DataSetWriter_find(psm, dswIdent);
    if(dsw)
        dsw->config.enabled = enabled;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createWriterGroup(UA_PubSubManager *psm,
                  const UA_WriterGroupDataType *wgParams,
                  UA_NodeId connectionIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_WriterGroupConfig config;
    UA_StatusCode res = UA_WriterGroupConfig_fromDataType(wgParams, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Invalid WriterGroup %S",
                     wgParams->name);
        return res;
    }

    /* Create disabled, the enabled flag is applied after loading completes */
    UA_Boolean enabled = config.enabled;
    config.enabled = false;

    UA_NodeId wgIdent;
    res = UA_WriterGroup_create(psm, connectionIdent, &config, &wgIdent);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Adding WriterGroup %S failed",
                     wgParams->name);
        return res;
    }

    UA_WriterGroup *wg = UA_WriterGroup_find(psm, wgIdent);
    if(wg)
        wg->config.enabled = enabled;

    for(size_t i = 0; i < wgParams->dataSetWritersSize; i++) {
        res = createDataSetWriter(psm, &wgParams->dataSetWriters[i], wgIdent);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createDataSetReader(UA_PubSubManager *psm,
                    const UA_DataSetReaderDataType *dsrParams,
                    UA_NodeId readerGroupIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_DataSetReaderConfig config;
    UA_StatusCode res = UA_DataSetReaderConfig_fromDataType(dsrParams, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Invalid DataSetReader %S",
                     dsrParams->name);
        return res;
    }

    /* Create disabled, the enabled flag is applied after loading completes */
    UA_Boolean enabled = config.enabled;
    config.enabled = false;

    UA_NodeId dsrIdent;
    res = UA_DataSetReader_create(psm, readerGroupIdent, &config, &dsrIdent);
    UA_DataSetReaderConfig_clearView(&config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Creating DataSetReader %S failed",
                     dsrParams->name);
        return res;
    }

    UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dsrIdent);
    if(dsr)
        dsr->config.enabled = enabled;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createReaderGroup(UA_PubSubManager *psm,
                  const UA_ReaderGroupDataType *rgParams,
                  UA_NodeId connectionIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_ReaderGroupConfig config;
    UA_StatusCode res = UA_ReaderGroupConfig_fromDataType(rgParams, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Invalid ReaderGroup %S",
                     rgParams->name);
        return res;
    }

    /* Create disabled, the enabled flag is applied after loading completes */
    UA_Boolean enabled = config.enabled;
    config.enabled = false;

    UA_NodeId rgIdent;
    res = UA_ReaderGroup_create(psm, connectionIdent, &config, &rgIdent);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Adding ReaderGroup %S failed",
                     rgParams->name);
        return res;
    }

    UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, rgIdent);
    if(rg)
        rg->config.enabled = enabled;

    for(size_t i = 0; i < rgParams->dataSetReadersSize; i++) {
        res = createDataSetReader(psm, &rgParams->dataSetReaders[i], rgIdent);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createPubSubConnection(UA_PubSubManager *psm,
                       const UA_PubSubConnectionDataType *connParams) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PubSubConnectionConfig config;
    UA_StatusCode res = UA_PubSubConnectionConfig_fromDataType(connParams, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Invalid PubSubConnection %S",
                     connParams->name);
        return res;
    }

    /* Create disabled, the enabled flag is applied after loading completes */
    UA_Boolean enabled = config.enabled;
    config.enabled = false;

    UA_NodeId connectionIdent;
    res = UA_PubSubConnection_create(psm, &config, &connectionIdent);
    UA_PubSubConnectionConfig_clearView(&config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration: Creating PubSubConnection %S failed",
                     connParams->name);
        return res;
    }

    UA_PubSubConnection *c = UA_PubSubConnection_find(psm, connectionIdent);
    if(c)
        c->config.enabled = enabled;

    for(size_t i = 0; i < connParams->writerGroupsSize; i++) {
        res = createWriterGroup(psm, &connParams->writerGroups[i], connectionIdent);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    for(size_t i = 0; i < connParams->readerGroupsSize; i++) {
        res = createReaderGroup(psm, &connParams->readerGroups[i], connectionIdent);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    return UA_STATUSCODE_GOOD;
}

/* Replaces the PubSub configuration with the given
 * PubSubConfiguration2DataType */
static UA_StatusCode
updatePubSubConfig(UA_PubSubManager *psm,
                   const UA_PubSubConfiguration2DataType *config) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    /* Check if the PubSubManager is in an active state and has connections
     * attached */
    if(psm->drv.state != UA_LIFECYCLESTATE_STOPPED && psm->connectionsSize > 0) {
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "PubSub configuration: PubSub configured and active. "
                       "Disable the PublishSubscribe state before loading a "
                       "PubSub configuration");
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* Ensure the PubSubManager is stopped before clearing */
    if(psm->drv.state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_INFO(psm->logging, UA_LOGCATEGORY_PUBSUB,
                    "PubSub configuration: Stopping the PubSubManager before "
                    "loading the configuration");
        UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STOPPED);
    }

    /* Clear the PubSubManager to load a new config.
     * The PubSubManager is now guaranteed to be stopped. */
    UA_StatusCode res = UA_PubSubManager_clear(psm);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Log unsupported parts of the configuration. The DataSetClasses and the
     * ConfigurationVersion are read-only information and always ignored. */
    if(config->securityGroupsSize > 0)
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "PubSub configuration: SecurityGroups in the "
                       "configuration are not supported and ignored");
    if(config->pubSubKeyPushTargetsSize > 0)
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "PubSub configuration: PubSubKeyPushTargets in the "
                       "configuration are not supported and ignored");

    /* Store the top-level configuration metadata. The ConfigurationVersion is
     * set to the current time (the version from the file is ignored). */
    UA_KeyValueMap propertiesMap = {config->configurationPropertiesSize,
                                    config->configurationProperties};
    res = UA_KeyValueMap_copy(&propertiesMap, &psm->configurationProperties);
    res |= UA_Array_copy(config->defaultSecurityKeyServices,
                         config->defaultSecurityKeyServicesSize,
                         (void**)&psm->defaultSecurityKeyServices,
                         &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;
    psm->defaultSecurityKeyServicesSize = config->defaultSecurityKeyServicesSize;
    psm->configurationVersion =
        UA_PubSubConfigurationVersionTimeDifference(UA_DateTime_now());

    /* Create the PublishedDataSets */
    for(size_t i = 0; i < config->publishedDataSetsSize; i++) {
        UA_NodeId pdsIdent;
        res = createPublishedDataSet(psm, &config->publishedDataSets[i], &pdsIdent);
        if(res != UA_STATUSCODE_GOOD)
            goto errout;
    }

    /* Create the standalone SubscribedDataSets. They must exist before the
     * DataSetReaders that reference them by name. */
    for(size_t i = 0; i < config->subscribedDataSetsSize; i++) {
        res = createSubscribedDataSet(psm, &config->subscribedDataSets[i]);
        if(res != UA_STATUSCODE_GOOD)
            goto errout;
    }

    /* Create the PubSubConnections with the contained groups. All components
     * are created disabled. The enabled flag from the configuration is written
     * into the component configs for the activation below. */
    for(size_t i = 0; i < config->connectionsSize; i++) {
        res = createPubSubConnection(psm, &config->connections[i]);
        if(res != UA_STATUSCODE_GOOD)
            goto errout;
    }

    /* Enable the PubSub subsystem. The initial setup mode lets the state
     * machine cascade enable all components with the enabled flag set. */
    if(config->enabled) {
        UA_assert(psm->drv.state == UA_LIFECYCLESTATE_STOPPED);
        psm->pubSubInitialSetupMode = true;
        UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STARTED);
        psm->pubSubInitialSetupMode = false;
    }

    return UA_STATUSCODE_GOOD;

 errout:
    /* Do not leave a partially loaded configuration behind */
    UA_PubSubManager_clear(psm);
    return res;
}

UA_StatusCode
UA_Server_loadPubSubConfigFromByteString(UA_Server *server, const UA_ByteString buffer) {
    if(server == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    size_t offset = 0;
    UA_ExtensionObject decodedFile;
    UA_StatusCode res =
        UA_ExtensionObject_decodeBinary(&buffer, &offset, &decodedFile);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration file: Decoding failed");
        goto cleanup;
    }

    UA_PubSubConfiguration2DataType config;
    UA_String *namespaces = NULL;
    size_t namespacesSize = 0;
    res = extractPubSubConfig2FromExtensionObject(psm, &decodedFile, &config,
                                                  &namespaces, &namespacesSize);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Remap the namespace indices to the server NamespaceArray */
    res = remapNamespaces(psm, &config, namespaces, namespacesSize);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    res = updatePubSubConfig(psm, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration file: Loading failed");
        goto cleanup;
    }

 cleanup:
    unlockServer(server);
    UA_ExtensionObject_clear(&decodedFile);
    return res;
}

/******************/
/* Export         */
/******************/

static UA_StatusCode
generateWriterGroupDataType(const UA_WriterGroup *wg,
                            UA_WriterGroupDataType *dst) {
    UA_StatusCode res = UA_WriterGroupConfig_toDataType(&wg->config, dst);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    dst->enabled = wg->config.enabled;

    if(wg->writersCount > 0) {
        dst->dataSetWriters = (UA_DataSetWriterDataType*)
            UA_calloc(wg->writersCount, sizeof(UA_DataSetWriterDataType));
        if(!dst->dataSetWriters) {
            UA_WriterGroupDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->dataSetWritersSize = wg->writersCount;
    }

    size_t i = 0;
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        res = UA_DataSetWriterConfig_toDataType(&dsw->config,
                                                &dst->dataSetWriters[i]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_WriterGroupDataType_clear(dst);
            return res;
        }
        dst->dataSetWriters[i].enabled = dsw->config.enabled;

        /* The file links writer and PDS by name (empty for heartbeat). A
         * writer created via the API may lack it, use the connected PDS. */
        if(UA_String_isEmpty(&dst->dataSetWriters[i].dataSetName) &&
           dsw->connectedDataSet) {
            res = UA_String_copy(&dsw->connectedDataSet->config.name,
                                 &dst->dataSetWriters[i].dataSetName);
            if(res != UA_STATUSCODE_GOOD) {
                UA_WriterGroupDataType_clear(dst);
                return res;
            }
        }
        i++;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateReaderGroupDataType(const UA_ReaderGroup *rg,
                            UA_ReaderGroupDataType *dst) {
    UA_StatusCode res = UA_ReaderGroupConfig_toDataType(&rg->config, dst);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    dst->enabled = rg->config.enabled;

    if(rg->readersCount > 0) {
        dst->dataSetReaders = (UA_DataSetReaderDataType*)
            UA_calloc(rg->readersCount, sizeof(UA_DataSetReaderDataType));
        if(!dst->dataSetReaders) {
            UA_ReaderGroupDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->dataSetReadersSize = rg->readersCount;
    }

    size_t i = 0;
    UA_DataSetReader *dsr;
    LIST_FOREACH(dsr, &rg->readers, listEntry) {
        res = UA_DataSetReaderConfig_toDataType(&dsr->config,
                                                &dst->dataSetReaders[i]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_ReaderGroupDataType_clear(dst);
            return res;
        }
        dst->dataSetReaders[i].enabled = dsr->config.enabled;
        i++;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generatePubSubConnectionDataType(const UA_PubSubConnection *c,
                                 UA_PubSubConnectionDataType *dst) {
    UA_StatusCode res = UA_PubSubConnectionConfig_toDataType(&c->config, dst);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    dst->enabled = c->config.enabled;

    if(c->writerGroupsSize > 0) {
        dst->writerGroups = (UA_WriterGroupDataType*)
            UA_calloc(c->writerGroupsSize, sizeof(UA_WriterGroupDataType));
        if(!dst->writerGroups) {
            UA_PubSubConnectionDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->writerGroupsSize = c->writerGroupsSize;
    }

    size_t i = 0;
    UA_WriterGroup *wg;
    LIST_FOREACH(wg, &c->writerGroups, listEntry) {
        res = generateWriterGroupDataType(wg, &dst->writerGroups[i]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PubSubConnectionDataType_clear(dst);
            return res;
        }
        i++;
    }

    if(c->readerGroupsSize > 0) {
        dst->readerGroups = (UA_ReaderGroupDataType*)
            UA_calloc(c->readerGroupsSize, sizeof(UA_ReaderGroupDataType));
        if(!dst->readerGroups) {
            UA_PubSubConnectionDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        dst->readerGroupsSize = c->readerGroupsSize;
    }

    i = 0;
    UA_ReaderGroup *rg;
    LIST_FOREACH(rg, &c->readerGroups, listEntry) {
        res = generateReaderGroupDataType(rg, &dst->readerGroups[i]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PubSubConnectionDataType_clear(dst);
            return res;
        }
        i++;
    }

    return UA_STATUSCODE_GOOD;
}

/* Export the current configuration of the PubSubManager */
static UA_StatusCode
generatePubSubConfiguration2DataType(UA_PubSubManager *psm,
                                     UA_PubSubConfiguration2DataType *dst) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PubSubConfiguration2DataType_init(dst);
    UA_StatusCode res = UA_STATUSCODE_GOOD;

    /* PublishedDataSets */
    if(psm->publishedDataSetsSize > 0) {
        dst->publishedDataSets = (UA_PublishedDataSetDataType*)
            UA_calloc(psm->publishedDataSetsSize, sizeof(UA_PublishedDataSetDataType));
        if(!dst->publishedDataSets)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        dst->publishedDataSetsSize = psm->publishedDataSetsSize;

        size_t i = 0;
        UA_PublishedDataSet *pds;
        TAILQ_FOREACH(pds, &psm->publishedDataSets, listEntry) {
            res = UA_PublishedDataSet_toDataType(pds, &dst->publishedDataSets[i]);
            if(res != UA_STATUSCODE_GOOD)
                goto errout;
            i++;
        }
    }

    /* Connections */
    if(psm->connectionsSize > 0) {
        dst->connections = (UA_PubSubConnectionDataType*)
            UA_calloc(psm->connectionsSize, sizeof(UA_PubSubConnectionDataType));
        if(!dst->connections) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto errout;
        }
        dst->connectionsSize = psm->connectionsSize;

        size_t i = 0;
        UA_PubSubConnection *c;
        TAILQ_FOREACH(c, &psm->connections, listEntry) {
            res = generatePubSubConnectionDataType(c, &dst->connections[i]);
            if(res != UA_STATUSCODE_GOOD)
                goto errout;
            i++;
        }
    }

    /* Standalone SubscribedDataSets */
    if(psm->subscribedDataSetsSize > 0) {
        dst->subscribedDataSets = (UA_StandaloneSubscribedDataSetDataType*)
            UA_calloc(psm->subscribedDataSetsSize,
                      sizeof(UA_StandaloneSubscribedDataSetDataType));
        if(!dst->subscribedDataSets) {
            res = UA_STATUSCODE_BADOUTOFMEMORY;
            goto errout;
        }
        dst->subscribedDataSetsSize = psm->subscribedDataSetsSize;

        size_t i = 0;
        UA_SubscribedDataSet *sds;
        TAILQ_FOREACH(sds, &psm->subscribedDataSets, listEntry) {
            res = UA_SubscribedDataSetConfig_toDataType(&sds->config,
                                                        &dst->subscribedDataSets[i]);
            if(res != UA_STATUSCODE_GOOD)
                goto errout;
            i++;
        }
    }

    /* Top-level fields */
    dst->enabled = (psm->drv.state == UA_LIFECYCLESTATE_STARTED);
    dst->configurationVersion = psm->configurationVersion;
    res = UA_Array_copy(psm->configurationProperties.map,
                        psm->configurationProperties.mapSize,
                        (void**)&dst->configurationProperties,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;
    dst->configurationPropertiesSize = psm->configurationProperties.mapSize;

    res = UA_Array_copy(psm->defaultSecurityKeyServices,
                        psm->defaultSecurityKeyServicesSize,
                        (void**)&dst->defaultSecurityKeyServices,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    if(res != UA_STATUSCODE_GOOD)
        goto errout;
    dst->defaultSecurityKeyServicesSize = psm->defaultSecurityKeyServicesSize;

    /* TODO Part14: Export of the SecurityGroups (SKS) */

    return UA_STATUSCODE_GOOD;

 errout:
    UA_PubSubConfiguration2DataType_clear(dst);
    return res;
}

/* Encode the config wrapped in a UABinaryFileDataType with UA Binary */
static UA_StatusCode
encodePubSubConfiguration2(UA_PubSubManager *psm,
                           UA_PubSubConfiguration2DataType *config,
                           UA_ByteString *buffer) {
    UA_UABinaryFileDataType binFile;
    UA_UABinaryFileDataType_init(&binFile);

    /* The file lists the server NamespaceArray without ns0, so the indices
     * in the body need no remapping (Part 14 v1.05 Table 88). The array is
     * borrowed and binFile is not cleared. */
    UA_Server *server = psm->drv.server;
    if(server->namespacesSize > 1) {
        binFile.namespaces = &server->namespaces[1];
        binFile.namespacesSize = server->namespacesSize - 1;
    }

    UA_Variant_setScalar(&binFile.body, config,
                         &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATION2DATATYPE]);

    UA_ExtensionObject container;
    UA_ExtensionObject_init(&container);
    container.encoding = UA_EXTENSIONOBJECT_DECODED_NODELETE;
    container.content.decoded.type = &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE];
    container.content.decoded.data = &binFile;

    UA_StatusCode res = UA_encodeBinary(&container,
                                        &UA_TYPES[UA_TYPES_EXTENSIONOBJECT],
                                        buffer, NULL);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration file: Encoding failed");
    }
    return res;
}

UA_StatusCode
UA_Server_writePubSubConfigurationToByteString(UA_Server *server,
                                               UA_ByteString *buffer) {
    if(server == NULL || buffer == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_PubSubConfiguration2DataType config;
    UA_StatusCode res = generatePubSubConfiguration2DataType(psm, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "Retrieving the PubSub configuration failed");
        unlockServer(server);
        return res;
    }

    res = encodePubSubConfiguration2(psm, &config, buffer);
    UA_PubSubConfiguration2DataType_clear(&config);
    unlockServer(server);
    return res;
}

UA_StatusCode
UA_Server_getPubSubConfig2(UA_Server *server,
                           UA_PubSubConfiguration2DataType *config) {
    if(server == NULL || config == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode res = generatePubSubConfiguration2DataType(psm, config);
    unlockServer(server);
    return res;
}

#endif /* UA_ENABLE_PUBSUB && UA_ENABLE_PUBSUB_FILE_CONFIG */
