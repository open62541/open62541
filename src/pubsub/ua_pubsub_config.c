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


/* Get the PubSubConfiguration2DataType body of the UABinaryFileDataType in
 * src (Part 14 v1.05 9.1.3.7.1). dst is a shallow view that borrows from src.
 * Sets the reason on failure. */
static UA_StatusCode
configFromFileContent(const UA_ExtensionObject *src,
                      UA_PubSubConfiguration2DataType *dst,
                      UA_String **namespaces, size_t *namespacesSize,
                      const char **reason) {
    if(!UA_ExtensionObject_hasDecodedType(src,
           &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE])) {
        *reason = "The file content is not a UABinaryFileDataType";
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    UA_UABinaryFileDataType *binFile =
        (UA_UABinaryFileDataType*)src->content.decoded.data;

    if(binFile->body.arrayLength != 0 || binFile->body.arrayDimensionsSize != 0 ||
       !binFile->body.data) {
        *reason = "The body must contain a single configuration";
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    if(binFile->body.type != &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATION2DATATYPE]) {
        *reason = "The body is not a PubSubConfiguration2DataType";
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    *dst = *(UA_PubSubConfiguration2DataType*)binFile->body.data;

    *namespaces = binFile->namespaces;
    *namespacesSize = binFile->namespacesSize;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
extractPubSubConfig2FromExtensionObject(UA_PubSubManager *psm,
                                        const UA_ExtensionObject *src,
                                        UA_PubSubConfiguration2DataType *dst,
                                        UA_String **namespaces,
                                        size_t *namespacesSize) {
    const char *reason = "";
    UA_StatusCode res =
        configFromFileContent(src, dst, namespaces, namespacesSize, &reason);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration file: %s", reason);
    return res;
}

/**************/
/* References */
/**************/

/* Only count if refs is NULL. Match only applies to connections and groups. */
static void
addReference(UA_PubSubConfigurationRefDataType *refs, size_t *refsSize,
             UA_UInt32 mask, UA_UInt32 refbit, size_t elementIndex,
             size_t connectionIndex, size_t groupIndex) {
    if(refbit != UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION &&
       refbit != UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP &&
       refbit != UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP)
        mask &= ~(UA_UInt32)UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH;
    if(mask == 0)
        return;
    if(refs) {
        UA_PubSubConfigurationRefDataType *ref = &refs[*refsSize];
        UA_PubSubConfigurationRefDataType_init(ref);
        ref->configurationMask = mask | refbit;
        ref->elementIndex = (UA_UInt16)elementIndex;
        ref->connectionIndex = (UA_UInt16)connectionIndex;
        ref->groupIndex = (UA_UInt16)groupIndex;
    }
    (*refsSize)++;
}

/* References for all elements, in file order */
static void
addReferences(const UA_PubSubConfiguration2DataType *cfg, UA_UInt32 mask,
              UA_PubSubConfigurationRefDataType *refs, size_t *refsSize) {
    *refsSize = 0;
    for(size_t i = 0; i < cfg->publishedDataSetsSize; i++)
        addReference(refs, refsSize, mask,
                     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET, i, 0, 0);
    for(size_t i = 0; i < cfg->subscribedDataSetsSize; i++)
        addReference(refs, refsSize, mask,
                     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESUBDATASET, i, 0, 0);
    for(size_t c = 0; c < cfg->connectionsSize; c++) {
        const UA_PubSubConnectionDataType *conn = &cfg->connections[c];
        addReference(refs, refsSize, mask,
                     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION, 0, c, 0);
        for(size_t g = 0; g < conn->writerGroupsSize; g++) {
            addReference(refs, refsSize, mask,
                         UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP, 0, c, g);
            for(size_t k = 0; k < conn->writerGroups[g].dataSetWritersSize; k++)
                addReference(refs, refsSize, mask,
                             UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER, k, c, g);
        }
        for(size_t g = 0; g < conn->readerGroupsSize; g++) {
            addReference(refs, refsSize, mask,
                         UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP, 0, c, g);
            for(size_t k = 0; k < conn->readerGroups[g].dataSetReadersSize; k++)
                addReference(refs, refsSize, mask,
                             UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER, k, c, g);
        }
    }
    for(size_t i = 0; i < cfg->securityGroupsSize; i++)
        addReference(refs, refsSize, mask,
                     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESECURITYGROUP, i, 0, 0);
    for(size_t i = 0; i < cfg->pubSubKeyPushTargetsSize; i++)
        addReference(refs, refsSize, mask,
                     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUSHTARGET, i, 0, 0);
}

/* The indices of the references are 16 bit */
static UA_Boolean
indicesFit(const UA_PubSubConfiguration2DataType *cfg) {
    if(cfg->publishedDataSetsSize > UA_UINT16_MAX ||
       cfg->subscribedDataSetsSize > UA_UINT16_MAX ||
       cfg->connectionsSize > UA_UINT16_MAX ||
       cfg->securityGroupsSize > UA_UINT16_MAX ||
       cfg->pubSubKeyPushTargetsSize > UA_UINT16_MAX)
        return false;
    for(size_t c = 0; c < cfg->connectionsSize; c++) {
        const UA_PubSubConnectionDataType *conn = &cfg->connections[c];
        if(conn->writerGroupsSize > UA_UINT16_MAX ||
           conn->readerGroupsSize > UA_UINT16_MAX)
            return false;
        for(size_t g = 0; g < conn->writerGroupsSize; g++) {
            if(conn->writerGroups[g].dataSetWritersSize > UA_UINT16_MAX)
                return false;
        }
        for(size_t g = 0; g < conn->readerGroupsSize; g++) {
            if(conn->readerGroups[g].dataSetReadersSize > UA_UINT16_MAX)
                return false;
        }
    }
    return true;
}

UA_StatusCode
UA_PubSubConfiguration_createReferences(const UA_ByteString *file,
                                        UA_PubSubConfigurationRefMask mask,
                                        size_t *referencesSize,
                                        UA_PubSubConfigurationRefDataType **references) {
    if(!file || !referencesSize || !references)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    *referencesSize = 0;
    *references = NULL;

    /* The operations allowed by CloseAndUpdate, without reference bits */
    if(mask != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD &&
       mask != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH &&
       mask != (UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH) &&
       mask != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY &&
       mask != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_ExtensionObject eo;
    UA_StatusCode res =
        UA_decodeBinary(file, &eo, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT], NULL);
    if(res != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    UA_PubSubConfiguration2DataType cfg;
    UA_String *namespaces = NULL;
    size_t namespacesSize = 0;
    const char *reason = "";
    res = configFromFileContent(&eo, &cfg, &namespaces, &namespacesSize, &reason);
    if(res == UA_STATUSCODE_GOOD && !indicesFit(&cfg))
        res = UA_STATUSCODE_BADENCODINGLIMITSEXCEEDED;
    if(res != UA_STATUSCODE_GOOD) {
        UA_ExtensionObject_clear(&eo);
        return res;
    }

    /* Count, then fill the references */
    size_t size = 0;
    addReferences(&cfg, mask, NULL, &size);
    if(size > 0) {
        *references = (UA_PubSubConfigurationRefDataType*)
            UA_Array_new(size, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONREFDATATYPE]);
        if(!*references) {
            UA_ExtensionObject_clear(&eo);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        addReferences(&cfg, mask, *references, referencesSize);
    }
    UA_ExtensionObject_clear(&eo);
    return UA_STATUSCODE_GOOD;
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
UA_PubSubManager_encodeConfig2Blob(UA_PubSubManager *psm, UA_ByteString *buf) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PubSubConfiguration2DataType config;
    UA_StatusCode res = generatePubSubConfiguration2DataType(psm, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "Retrieving the PubSub configuration failed");
        return res;
    }

    res = encodePubSubConfiguration2(psm, &config, buf);
    UA_PubSubConfiguration2DataType_clear(&config);
    return res;
}

UA_StatusCode
UA_PubSubManager_decodeConfig2Blob(UA_PubSubManager *psm, const UA_ByteString *buf,
                                   UA_ExtensionObject *eo,
                                   UA_PubSubConfiguration2DataType *cfg) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    size_t offset = 0;
    UA_StatusCode res = UA_ExtensionObject_decodeBinary(buf, &offset, eo);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "PubSub configuration file: Decoding failed");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    UA_String *namespaces = NULL;
    size_t namespacesSize = 0;
    res = extractPubSubConfig2FromExtensionObject(psm, eo, cfg,
                                                  &namespaces, &namespacesSize);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ExtensionObject_clear(eo);
        UA_ExtensionObject_init(eo);
        return res;
    }

    res = remapNamespaces(psm, cfg, namespaces, namespacesSize);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ExtensionObject_clear(eo);
        UA_ExtensionObject_init(eo);
    }
    return res;
}

UA_StatusCode
UA_Server_readPubSubConfiguration(UA_Server *server, UA_ByteString *file) {
    if(server == NULL || file == NULL)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_ByteString_init(file);

    lockServer(server);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode res = UA_PubSubManager_encodeConfig2Blob(psm, file);
    unlockServer(server);
    return res;
}

#endif /* UA_ENABLE_PUBSUB && UA_ENABLE_PUBSUB_FILE_CONFIG */
