/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 * Copyright (c) 2025 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2025 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include <open62541/server_pubsub.h>

#if defined(UA_ENABLE_PUBSUB) && defined(UA_ENABLE_PUBSUB_FILE_CONFIG)

#include "ua_pubsub_internal.h"

static UA_StatusCode
createPubSubConnection(UA_PubSubManager *psm,
                       const UA_PubSubConnectionDataType *connection,
                       UA_UInt32 pdsCount, UA_NodeId *pdsIdent, UA_NodeId *connectionIdent);

static UA_StatusCode
createWriterGroup(UA_PubSubManager *psm,
                  const UA_WriterGroupDataType *writerGroupParameters,
                  UA_NodeId connectionIdent, UA_UInt32 pdsCount,
                  const UA_NodeId *pdsIdent, UA_NodeId *writerGroupIdent);

static UA_StatusCode
createDataSetWriter(UA_PubSubManager *psm,
                    const UA_DataSetWriterDataType *dataSetWriterParameters,
                    UA_NodeId writerGroupIdent, UA_UInt32 pdsCount,
                    const UA_NodeId *pdsIdent, UA_NodeId *dataSetWriterIdent);

static UA_StatusCode
createReaderGroup(UA_PubSubManager *psm,
                  const UA_ReaderGroupDataType *readerGroupParameters,
                  UA_NodeId connectionIdent, UA_NodeId *readerGroupIdent);

static UA_StatusCode
createDataSetReader(UA_PubSubManager *psm,
                    const UA_DataSetReaderDataType *dataSetReaderParameters,
                    UA_NodeId readerGroupIdent, UA_NodeId *dataSetReaderIdent);

static UA_StatusCode
createPublishedDataSet(UA_PubSubManager *psm,
                       const UA_PublishedDataSetDataType *publishedDataSetParameters,
                       UA_NodeId *publishedDataSetIdent);

static UA_StatusCode
createDataSetFields(UA_PubSubManager *psm,
                    const UA_NodeId *publishedDataSetIdent,
                    const UA_PublishedDataSetDataType *publishedDataSetParameters);

static UA_StatusCode
generatePubSubConfigurationDataType(UA_PubSubManager *psm,
                                    UA_PubSubConfigurationDataType *pubSubConfiguration);

/* Gets PubSub Configuration from an ExtensionObject */
static UA_StatusCode
extractPubSubConfigFromExtensionObject(UA_PubSubManager *psm,
                                       const UA_ExtensionObject *src,
                                       UA_PubSubConfigurationDataType **dst) {
    if(!src || !dst || src->encoding != UA_EXTENSIONOBJECT_DECODED ||
       src->content.decoded.type != &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE] ||
       !src->content.decoded.data) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_extractPubSubConfigFromDecodedObject] "
                     "Reading extensionObject failed");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_UABinaryFileDataType *binFile = (UA_UABinaryFileDataType*)src->content.decoded.data;

    if(binFile->body.arrayLength != 0 || binFile->body.arrayDimensionsSize != 0) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_extractPubSubConfigFromDecodedObject] "
                     "Loading multiple configurations is not supported");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    if(binFile->body.type != &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE]) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_extractPubSubConfigFromDecodedObject] "
                     "Invalid datatype encoded in the binary file");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }

    *dst = (UA_PubSubConfigurationDataType*)binFile->body.data;
    return UA_STATUSCODE_GOOD;
}

/* Configures with given PubSubConfigurationDataType object */
static UA_StatusCode
updatePubSubConfig(UA_PubSubManager *psm,
                   const UA_PubSubConfigurationDataType *configurationParameters) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    if(configurationParameters == NULL) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_updatePubSubConfig] Invalid argument");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Check if the PubSubManager is in an active state and has connections attached. */
    if(psm->drv.state != UA_LIFECYCLESTATE_STOPPED && psm->connectionsSize > 0) {
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "[UA_PubSubManager_updatePubSubConfig] PubSub configured and active. "
                       "Disable the PublishSubscribe state before loading a pubsub configuration");
        return UA_STATUSCODE_BADINVALIDSTATE;
    }

    /* Ensure the PubSubManager is stopped before clearing */
    if(psm->drv.state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_INFO(psm->logging, UA_LOGCATEGORY_PUBSUB,
                    "[UA_PubSubManager_updatePubSubConfig] Stopping PubSubManager before loading configuration");
        UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STOPPED);
    }

    /* Clear the PubSubManager to load a new config.
     * The PubSubManager is now guaranteed to be stopped. */
    UA_StatusCode res = UA_PubSubManager_clear(psm);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Configuration of Published DataSets: */
    UA_UInt32 pdsCount = (UA_UInt32)configurationParameters->publishedDataSetsSize;
    UA_NodeId *publishedDataSetIdent = NULL;
    if(pdsCount > 0) {
        publishedDataSetIdent = (UA_NodeId*)UA_calloc(pdsCount, sizeof(UA_NodeId));
        if(!publishedDataSetIdent)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(UA_UInt32 i = 0; i < pdsCount; i++) {
        res = createPublishedDataSet(psm,
                                     &configurationParameters->publishedDataSets[i],
                                     &publishedDataSetIdent[i]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "[UA_PubSubManager_updatePubSubConfig] PDS creation failed");
            UA_free(publishedDataSetIdent);
            UA_PubSubManager_clear(psm);
            return res;
        }
    }

    /* Configuration of PubSub Connections: */
    if(configurationParameters->connectionsSize < 1) {
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "[UA_PubSubManager_updatePubSubConfig] no connection in "
                       "UA_PubSubConfigurationDataType");
        UA_free(publishedDataSetIdent);
        return UA_STATUSCODE_GOOD;
    }

    /* Phase 1: Create all components with enabled = false to avoid premature state changes */
    UA_LOG_INFO(psm->logging, UA_LOGCATEGORY_PUBSUB,
                "[UA_PubSubManager_updatePubSubConfig] START CONNECTIONS (Phase 1: Creation)");

    /* Store connection NodeIds for Phase 2 */
    UA_NodeId *connectionIdents = (UA_NodeId*)UA_calloc(configurationParameters->connectionsSize, sizeof(UA_NodeId));
    if(!connectionIdents) {
        UA_free(publishedDataSetIdent);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    for(size_t i = 0; i < configurationParameters->connectionsSize; i++) {
        res = createPubSubConnection(psm, &configurationParameters->connections[i],
                                     pdsCount, publishedDataSetIdent, &connectionIdents[i]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_free(connectionIdents);
            break;
        }
    }
    
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(publishedDataSetIdent);
        UA_PubSubManager_clear(psm);
        return res;
    }
    
    /* Phase 2: Enable components based on original configuration */
    UA_LOG_INFO(psm->logging, UA_LOGCATEGORY_PUBSUB,
                "[UA_PubSubManager_updatePubSubConfig] START COMPONENTS (Phase 2: Enabling)");
    
    /* Apply the configured desired state by component identity. The runtime
     * lists do not preserve the order of the decoded arrays: groups are
     * inserted at the list head and writers are sorted by DataSetWriterId. */
    for(size_t i = 0; i < configurationParameters->connectionsSize; i++) {
        const UA_PubSubConnectionDataType *connParams = &configurationParameters->connections[i];
        UA_PubSubConnection *conn =
            UA_PubSubConnection_find(psm, connectionIdents[i]);
        if(!conn) {
            res = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }
        conn->config.enabled = connParams->enabled;
    }

    if(res != UA_STATUSCODE_GOOD)
        UA_PubSubManager_clear(psm);

    /* Enable PubSubManager if specified */
    if(res == UA_STATUSCODE_GOOD && configurationParameters->enabled) {
        UA_LOG_INFO(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "[UA_PubSubManager_updatePubSubConfig] PubSubManager is enabled");
        UA_assert(psm->drv.state == UA_LIFECYCLESTATE_STOPPED);
        psm->pubSubInitialSetupMode = true;
        UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STARTED);
        psm->pubSubInitialSetupMode = false;
    }
    
    UA_free(connectionIdents);
    UA_free(publishedDataSetIdent);
    return res;
}

static UA_StatusCode
createPubSubConnection(UA_PubSubManager *psm, const UA_PubSubConnectionDataType *connParams,
                       UA_UInt32 pdsCount, UA_NodeId *pdsIdent, UA_NodeId *connectionIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PubSubConnectionConfig config;
    memset(&config, 0, sizeof(UA_PubSubConnectionConfig));

    config.name =                       connParams->name;
    config.transportProfileUri =        connParams->transportProfileUri;
    config.connectionProperties.map =   connParams->connectionProperties;
    config.connectionProperties.mapSize = connParams->connectionPropertiesSize;
    config.enabled = false;  /* Always create disabled, enabling during the last stage of updatePubSubConfig */

    UA_StatusCode res = UA_PublisherId_fromVariant(&config.publisherId,
                                                   &connParams->publisherId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createPubSubConnection] "
                     "Setting PublisherId failed");
        return res;
    }

    if(connParams->address.encoding == UA_EXTENSIONOBJECT_DECODED) {
        UA_Variant_setScalar(&(config.address),
                             connParams->address.content.decoded.data,
                             connParams->address.content.decoded.type);
    } else {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createPubSubConnection] "
                     "Reading connection address failed");
        UA_PublisherId_clear(&config.publisherId);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(connParams->transportSettings.encoding == UA_EXTENSIONOBJECT_DECODED) {
        UA_Variant_setScalar(&(config.connectionTransportSettings),
                             connParams->transportSettings.content.decoded.data,
                             connParams->transportSettings.content.decoded.type);
    } else if (connParams->transportSettings.encoding != UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
        UA_LOG_WARNING(psm->logging, UA_LOGCATEGORY_PUBSUB,
                       "[UA_PubSubManager_createPubSubConnection] "
                       "TransportSettings can not be read");
    }

    res = UA_PubSubConnection_create(psm, &config, connectionIdent);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createPubSubConnection] "
                     "Connection creation failed");
        UA_PublisherId_clear(&config.publisherId);
        return res;
    }

    for(size_t i = 0; i < connParams->writerGroupsSize; i++) {
        UA_NodeId writerGroupIdent;
        res = createWriterGroup(psm, &connParams->writerGroups[i],
                                *connectionIdent, pdsCount, pdsIdent,
                                &writerGroupIdent);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PubSubConnection *created =
                UA_PubSubConnection_find(psm, *connectionIdent);
            if(created)
                UA_PubSubComponent_freeWithoutLifecycleCallback(
                    psm, created, UA_PUBSUBCOMPONENT_CONNECTION);
            UA_PublisherId_clear(&config.publisherId);
            return res;
        }
        UA_WriterGroup *wg = UA_WriterGroup_find(psm, writerGroupIdent);
        if(!wg) {
            UA_PubSubConnection *created =
                UA_PubSubConnection_find(psm, *connectionIdent);
            if(created)
                UA_PubSubComponent_freeWithoutLifecycleCallback(
                    psm, created, UA_PUBSUBCOMPONENT_CONNECTION);
            UA_PublisherId_clear(&config.publisherId);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        wg->config.enabled = connParams->writerGroups[i].enabled;
    }

    for(size_t j = 0; j < connParams->readerGroupsSize; j++) {
        UA_NodeId readerGroupIdent;
        res = createReaderGroup(psm, &connParams->readerGroups[j],
                                *connectionIdent, &readerGroupIdent);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PubSubConnection *created =
                UA_PubSubConnection_find(psm, *connectionIdent);
            if(created)
                UA_PubSubComponent_freeWithoutLifecycleCallback(
                    psm, created, UA_PUBSUBCOMPONENT_CONNECTION);
            UA_PublisherId_clear(&config.publisherId);
            return res;
        }
        UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, readerGroupIdent);
        if(!rg) {
            UA_PubSubConnection *created =
                UA_PubSubConnection_find(psm, *connectionIdent);
            if(created)
                UA_PubSubComponent_freeWithoutLifecycleCallback(
                    psm, created, UA_PUBSUBCOMPONENT_CONNECTION);
            UA_PublisherId_clear(&config.publisherId);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        rg->config.enabled = connParams->readerGroups[j].enabled;
    }

    UA_PublisherId_clear(&config.publisherId);
    return UA_STATUSCODE_GOOD;
}

/* Function called by UA_PubSubManager_createWriterGroup to configure the messageSettings
 * of a writerGroup */
static UA_StatusCode
setWriterGroupEncodingType(UA_PubSubManager *psm,
                           const UA_WriterGroupDataType *writerGroupParameters,
                           UA_WriterGroupConfig *config) {
    if(writerGroupParameters->messageSettings.encoding != UA_EXTENSIONOBJECT_DECODED) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_setWriterGroupEncodingType] "
                     "getting message type information failed");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(writerGroupParameters->messageSettings.content.decoded.type ==
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]) {
        config->encodingMimeType = UA_PUBSUB_ENCODING_UADP;
    } else if(writerGroupParameters->messageSettings.content.decoded.type ==
              &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE]) {
#ifdef UA_ENABLE_JSON_ENCODING
        config->encodingMimeType = UA_PUBSUB_ENCODING_JSON;
#else
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_setWriterGroupEncodingType] "
                     "encoding type: JSON (not implemented!)");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
#endif
    } else {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_setWriterGroupEncodingType] "
                     "invalid message encoding type");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
createWriterGroup(UA_PubSubManager *psm,
                  const UA_WriterGroupDataType *writerGroupParameters,
                  UA_NodeId connectionIdent, UA_UInt32 pdsCount,
                  const UA_NodeId *pdsIdent, UA_NodeId *writerGroupIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_WriterGroupConfig config;
    memset(&config, 0, sizeof(UA_WriterGroupConfig));
    config.name =                  writerGroupParameters->name;
    config.writerGroupId =         writerGroupParameters->writerGroupId;
    config.publishingInterval =    writerGroupParameters->publishingInterval;
    config.keepAliveTime =         writerGroupParameters->keepAliveTime;
    config.priority =              writerGroupParameters->priority;
    config.securityMode =          writerGroupParameters->securityMode;
    config.securityGroupId =       writerGroupParameters->securityGroupId;
    config.securityKeyServicesSize = writerGroupParameters->securityKeyServicesSize;
    config.securityKeyServices =   writerGroupParameters->securityKeyServices;
    config.maxNetworkMessageSize = writerGroupParameters->maxNetworkMessageSize;
    config.transportSettings =     writerGroupParameters->transportSettings;
    config.messageSettings =       writerGroupParameters->messageSettings;
    config.groupProperties.mapSize =   writerGroupParameters->groupPropertiesSize;
    config.groupProperties.map =   writerGroupParameters->groupProperties;
    config.maxEncapsulatedDataSetMessageCount = 255; /* non std parameter */
    config.enabled = false;  /* Always create disabled, enabling during the last stage of updatePubSubConfig */

    UA_StatusCode res = setWriterGroupEncodingType(psm, writerGroupParameters, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createWriterGroup] "
                     "Setting message settings failed");
        return res;
    }

    /* Load config. The enabled flag is "false" here.
     * Auto-enable only after adding the DataSetWriters. */
    res = UA_WriterGroup_create(psm, connectionIdent, &config,
                                writerGroupIdent);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createWriterGroup] "
                     "Adding WriterGroup failed: 0x%x", res);
        return res;
    }

    /* Configuration of all DataSetWriters that belong to this WriterGroup - all created disabled */
    for(size_t dsw = 0; dsw < writerGroupParameters->dataSetWritersSize; dsw++) {
        UA_NodeId dataSetWriterIdent;
        res = createDataSetWriter(psm,
                                  &writerGroupParameters->dataSetWriters[dsw],
                                  *writerGroupIdent, pdsCount, pdsIdent,
                                  &dataSetWriterIdent);
        if(res != UA_STATUSCODE_GOOD)
            goto rollback;
        UA_DataSetWriter *writer =
            UA_DataSetWriter_find(psm, dataSetWriterIdent);
        if(!writer)
            goto rollbackInternal;
        writer->config.enabled =
            writerGroupParameters->dataSetWriters[dsw].enabled;
    }

    return res;

rollbackInternal:
    res = UA_STATUSCODE_BADINTERNALERROR;
rollback:
    {
        UA_WriterGroup *wg = UA_WriterGroup_find(psm, *writerGroupIdent);
        if(wg)
            UA_PubSubComponent_freeWithoutLifecycleCallback(
                psm, wg, UA_PUBSUBCOMPONENT_WRITERGROUP);
    }
    return res;
}

static UA_StatusCode
createDataSetWriter(UA_PubSubManager *psm,
                    const UA_DataSetWriterDataType *dataSetWriterParameters,
                    UA_NodeId writerGroupIdent, UA_UInt32 pdsCount,
                    const UA_NodeId *pdsIdent, UA_NodeId *dataSetWriterIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    /* Search the PDS among the supplied PDS-NodeIds.
     * The name must match the configured one. */
    UA_PublishedDataSet *pds = NULL;
    size_t i = 0;
    for(; i < pdsCount; i++) {
        pds = UA_PublishedDataSet_find(psm, pdsIdent[i]);
        if(!pds)
            continue;
        if(!UA_String_equal(&dataSetWriterParameters->dataSetName,
                            &pds->config.name))
           continue;
        break;
    }

    /* No matching PDS found */
    if(i == pdsCount) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_addDataSetWriterWithPdsReference] "
                     "No matching PDS with name %S found",
                     dataSetWriterParameters->name);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_DataSetWriterConfig config;
    memset(&config, 0, sizeof(UA_DataSetWriterConfig));
    config.name = dataSetWriterParameters->name;
    config.dataSetWriterId = dataSetWriterParameters->dataSetWriterId;
    config.keyFrameCount = dataSetWriterParameters->keyFrameCount;
    config.dataSetFieldContentMask = dataSetWriterParameters->dataSetFieldContentMask;
    config.messageSettings = dataSetWriterParameters->messageSettings;
    config.transportSettings = dataSetWriterParameters->transportSettings;
    config.dataSetName = dataSetWriterParameters->dataSetName;
    config.dataSetWriterProperties.mapSize = dataSetWriterParameters->dataSetWriterPropertiesSize;
    config.dataSetWriterProperties.map = dataSetWriterParameters->dataSetWriterProperties;
    config.enabled = false;  /* Always create disabled, enabling during the last stage of updatePubSubConfig */

    /* Create the DataSetWriter disabled. Enable later in Phase 2. */
    UA_NodeId localIdent;
    UA_NodeId *ident = dataSetWriterIdent ? dataSetWriterIdent : &localIdent;
    UA_StatusCode res =
        UA_DataSetWriter_create(psm, writerGroupIdent, pdsIdent[i],
                                &config, ident);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_addDataSetWriterWithPdsReference] "
                     "Creating the DataSetWriter for %S failed",
                     dataSetWriterParameters->name);
    }
    return res;
}

static UA_StatusCode
createReaderGroup(UA_PubSubManager *psm,
                  const UA_ReaderGroupDataType *readerGroupParameters,
                  UA_NodeId connectionIdent, UA_NodeId *readerGroupIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_ReaderGroupConfig config;
    memset(&config, 0, sizeof(UA_ReaderGroupConfig));

    config.name = readerGroupParameters->name;
    config.securityMode = readerGroupParameters->securityMode;
    config.securityGroupId = readerGroupParameters->securityGroupId;
    config.securityKeyServicesSize = readerGroupParameters->securityKeyServicesSize;
    config.securityKeyServices = readerGroupParameters->securityKeyServices;
    config.maxNetworkMessageSize = readerGroupParameters->maxNetworkMessageSize;
    config.transportSettings = readerGroupParameters->transportSettings;
    config.messageSettings = readerGroupParameters->messageSettings;
    config.groupProperties.mapSize = readerGroupParameters->groupPropertiesSize;
    config.groupProperties.map = readerGroupParameters->groupProperties;
    config.enabled = false;  /* Always create disabled, enabling during the last stage of updatePubSubConfig */

    UA_StatusCode res = UA_ReaderGroup_create(psm, connectionIdent,
                                              &config, readerGroupIdent);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createReaderGroup] Adding ReaderGroup "
                     "failed: 0x%x", res);
        return res;
    }

    for(UA_UInt32 i = 0; i < readerGroupParameters->dataSetReadersSize; i++) {
        UA_NodeId dataSetReaderIdent;
        res = createDataSetReader(psm,
                                  &readerGroupParameters->dataSetReaders[i],
                                  *readerGroupIdent, &dataSetReaderIdent);
        if(res != UA_STATUSCODE_GOOD)
            goto rollback;
        UA_DataSetReader *reader =
            UA_DataSetReader_find(psm, dataSetReaderIdent);
        if(!reader)
            goto rollbackInternal;
        reader->config.enabled =
            readerGroupParameters->dataSetReaders[i].enabled;
    }

    return UA_STATUSCODE_GOOD;

rollbackInternal:
    res = UA_STATUSCODE_BADINTERNALERROR;
rollback:
    {
        UA_ReaderGroup *rg = UA_ReaderGroup_find(psm, *readerGroupIdent);
        if(rg)
            UA_PubSubComponent_freeWithoutLifecycleCallback(
                psm, rg, UA_PUBSUBCOMPONENT_READERGROUP);
    }
    return res;
}

/* Creates TargetVariables or SubscribedDataSetMirror for a given DataSetReader */
static UA_StatusCode
addSubscribedDataSet(UA_PubSubManager *psm, const UA_NodeId dsReaderIdent,
                     const UA_ExtensionObject *subscribedDataSet) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    if(!subscribedDataSet ||
       subscribedDataSet->encoding != UA_EXTENSIONOBJECT_DECODED ||
       !subscribedDataSet->content.decoded.type ||
       !subscribedDataSet->content.decoded.data) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_addSubscribedDataSet] "
                     "SubscribedDataSet is not a decoded value");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(subscribedDataSet->content.decoded.type ==
       &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]) {
        UA_TargetVariablesDataType *targetVars = (UA_TargetVariablesDataType*)
            subscribedDataSet->content.decoded.data;
        UA_StatusCode res = UA_STATUSCODE_BADINTERNALERROR;
        UA_DataSetReader *dsr = UA_DataSetReader_find(psm, dsReaderIdent);
        if(dsr)
            res = DataSetReader_createTargetVariables(psm, dsr,
                                                      targetVars->targetVariablesSize,
                                                      targetVars->targetVariables);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "[UA_PubSubManager_addSubscribedDataSet] "
                         "create TargetVariables failed");
        }
        return res;
    }

    if(subscribedDataSet->content.decoded.type ==
       &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE]) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_addSubscribedDataSet] "
                     "DataSetMirror is currently not supported");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                 "[UA_PubSubManager_addSubscribedDataSet] "
                 "Invalid Type of SubscribedDataSet");
    return UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
createDataSetReader(UA_PubSubManager *psm, const UA_DataSetReaderDataType *dsrParams,
                    UA_NodeId readerGroupIdent, UA_NodeId *dataSetReaderIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    /* Prepare the config parameters */
    UA_DataSetReaderConfig config;
    memset(&config, 0, sizeof(UA_DataSetReaderConfig));
    config.name = dsrParams->name;
    config.writerGroupId = dsrParams->writerGroupId;
    config.dataSetWriterId = dsrParams->dataSetWriterId;
    config.dataSetMetaData = dsrParams->dataSetMetaData;
    config.dataSetFieldContentMask = dsrParams->dataSetFieldContentMask;
    config.messageReceiveTimeout =  dsrParams->messageReceiveTimeout;
    config.messageSettings = dsrParams->messageSettings;
    config.transportSettings = dsrParams->transportSettings;
    config.enabled = false;  /* Always create disabled, enabling during the last stage of updatePubSubConfig */
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(dsrParams->publisherId.type) {
        res = UA_PublisherId_fromVariant(&config.publisherId,
                                         &dsrParams->publisherId);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        config.publisherIdFilterEnabled = true;
    }

    UA_NodeId localIdent;
    UA_NodeId *ident = dataSetReaderIdent ? dataSetReaderIdent : &localIdent;
    res = UA_DataSetReader_create(psm, readerGroupIdent, &config, ident);
    UA_PublisherId_clear(&config.publisherId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createDataSetReader] "
                     "Could not create the DataSetReader");
        return res;
    }

    /* Create the SubscribedDataSet */
    res = addSubscribedDataSet(psm, *ident, &dsrParams->subscribedDataSet);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createDataSetReader] "
                     "Create subscribedDataSet failed");
        UA_DataSetReader *dsr = UA_DataSetReader_find(psm, *ident);
        if(dsr)
            UA_DataSetReader_remove(psm, dsr);
        return res;
    }

    return UA_STATUSCODE_GOOD;
}

/* Determines whether PublishedDataSet is of type PublishedItems or PublishedEvents.
 * (PublishedEvents are currently not supported!)
 *
 * @param publishedDataSetParameters PublishedDataSet parameters
 * @param config PublishedDataSet configuration object */
static UA_StatusCode
setPublishedDataSetType(UA_PubSubManager *psm,
                        const UA_PublishedDataSetDataType *pdsParams,
                        UA_PublishedDataSetConfig *config) {
    if(pdsParams->dataSetSource.encoding != UA_EXTENSIONOBJECT_DECODED)
        return UA_STATUSCODE_BADINTERNALERROR;

    const UA_DataType *sourceType = pdsParams->dataSetSource.content.decoded.type;
    if(sourceType == &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]) {
        config->publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        return UA_STATUSCODE_GOOD;
    } else if(sourceType == &UA_TYPES[UA_TYPES_PUBLISHEDEVENTSDATATYPE]) {
        /* config.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS; */
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_setPublishedDataSetType] Published events not supported.");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                 "[UA_PubSubManager_setPublishedDataSetType] Invalid DataSetSourceDataType.");
    return UA_STATUSCODE_BADINTERNALERROR;
}

/* Creates PublishedDataSetConfig object from PublishedDataSet object
 *
 * @param psm PubSubManager that shall be configured
 * @param pdsParams publishedDataSet configuration
 * @param pdsIdent NodeId of the publishedDataSet */
static UA_StatusCode
createPublishedDataSet(UA_PubSubManager *psm,
                       const UA_PublishedDataSetDataType *pdsParams,
                       UA_NodeId *pdsIdent) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PublishedDataSetConfig config;
    memset(&config, 0, sizeof(UA_PublishedDataSetConfig));

    config.name = pdsParams->name;
    UA_StatusCode res = setPublishedDataSetType(psm, pdsParams, &config);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_PublishedDataSet_create(psm, &config, pdsIdent).addResult;
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createPublishedDataSet] "
                     "Adding PublishedDataSet failed.");
        return res;
    }

    /* DataSetField configuration for this publishedDataSet: */
    res = createDataSetFields(psm, pdsIdent, pdsParams);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createPublishedDataSet] "
                     "Creating DataSetFieldConfig failed.");
        UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, *pdsIdent);
        if(pds)
            UA_PublishedDataSet_remove(psm, pds);
    }

    return res;
}

/* Adds DataSetField Variables bound to a certain PublishedDataSet. This method does NOT
 * check, whether the PublishedDataSet actually contains Variables instead of Events!
 *
 * @param psm PubSubManager that shall be configured
 * @param pdsIdent NodeId of the publishedDataSet, the DataSetField belongs to
 * @param publishedDataSetParameters publishedDataSet configuration */
static UA_StatusCode
addDataSetFieldVariables(UA_PubSubManager *psm, const UA_NodeId *pdsIdent,
                         const UA_PublishedDataSetDataType *pdsParams) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PublishedDataItemsDataType *pdItems = (UA_PublishedDataItemsDataType *)
        pdsParams->dataSetSource.content.decoded.data;
    if(pdItems->publishedDataSize != pdsParams->dataSetMetaData.fieldsSize)
        return UA_STATUSCODE_BADINTERNALERROR;

    for(size_t i = 0; i < pdItems->publishedDataSize; i++) {
        UA_DataSetFieldConfig fc;
        memset(&fc, 0, sizeof(UA_DataSetFieldConfig));
        fc.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fc.field.variable.configurationVersion =
            pdsParams->dataSetMetaData.configurationVersion;
        fc.field.variable.fieldNameAlias = pdsParams->dataSetMetaData.fields[i].name;
        fc.field.variable.promotedField = pdsParams->dataSetMetaData.
            fields[i].fieldFlags & 0x0001;
        fc.field.variable.publishParameters = pdItems->publishedData[i];
        fc.field.variable.maxStringLength =
            pdsParams->dataSetMetaData.fields[i].maxStringLength;
        fc.field.variable.description =
            pdsParams->dataSetMetaData.fields[i].description;
        fc.field.variable.dataSetFieldId =
            pdsParams->dataSetMetaData.fields[i].dataSetFieldId;

        UA_NodeId fieldIdent;
        UA_StatusCode res = UA_DataSetField_create(psm, *pdsIdent, &fc, &fieldIdent).result;
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "[UA_PubSubManager_addDataSetFieldVariables] "
                         "Adding DataSetField Variable failed.");
            return res;
        }

        /* The public field configuration does not carry every FieldMetaData
         * member. Preserve the serialized schema verbatim after validating
         * that the source node can be used to create the field. */
        UA_DataSetField *field = UA_DataSetField_find(psm, fieldIdent);
        if(!field)
            return UA_STATUSCODE_BADINTERNALERROR;
        UA_FieldMetaData_clear(&field->fieldMetaData);
        res = UA_FieldMetaData_copy(&pdsParams->dataSetMetaData.fields[i],
                                    &field->fieldMetaData);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, *pdsIdent);
    if(!pds)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_DataSetMetaDataType replacement;
    UA_DataSetMetaDataType_init(&replacement);
    UA_StatusCode res = UA_DataSetMetaDataType_copy(&pdsParams->dataSetMetaData,
                                                    &replacement);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_DataSetMetaDataType_clear(&pds->dataSetMetaData);
    pds->dataSetMetaData = replacement;

    return UA_STATUSCODE_GOOD;
}

/* Checks if PublishedDataSet contains event or variable fields and calls the
 * corresponding method to add these fields.
 *
 * @param psm PubSubManager that shall be configured
 * @param pdsIdent NodeId of the publishedDataSet, the DataSetFields belongs to
 * @param pdsParams publishedDataSet configuration */
static UA_StatusCode
createDataSetFields(UA_PubSubManager *psm, const UA_NodeId *pdsIdent,
                    const UA_PublishedDataSetDataType *pdsParams) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    if(pdsParams->dataSetSource.encoding != UA_EXTENSIONOBJECT_DECODED)
        return UA_STATUSCODE_BADINTERNALERROR;

    if(pdsParams->dataSetSource.content.decoded.type ==
       &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE])
        return addDataSetFieldVariables(psm, pdsIdent, pdsParams);

    /* TODO: Implement Routine for adding Event DataSetFields */
    if(pdsParams->dataSetSource.content.decoded.type ==
       &UA_TYPES[UA_TYPES_PUBLISHEDEVENTSDATATYPE]) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_createDataSetFields] "
                     "Published events not supported.");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                 "[UA_PubSubManager_createDataSetFields] "
                 "Invalid DataSetSourceDataType.");
    return UA_STATUSCODE_BADINTERNALERROR;
}

UA_StatusCode
UA_Server_loadPubSubConfigFromByteString(UA_Server *server, const UA_ByteString buffer) {
    size_t offset = 0;
    UA_ExtensionObject decodedFile;
    UA_ExtensionObject_init(&decodedFile);

    if(!server)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode res =
        UA_ExtensionObject_decodeBinary(&buffer, &offset, &decodedFile);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_decodeBinFile] decoding UA_Binary failed");
        goto cleanup;
    }

    UA_PubSubConfigurationDataType *pubSubConfig = NULL;
    res = extractPubSubConfigFromExtensionObject(psm, &decodedFile, &pubSubConfig);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_loadPubSubConfigFromByteString] "
                     "Extracting PubSub Configuration failed");
        goto cleanup;
    }

    res = updatePubSubConfig(psm, pubSubConfig);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_loadPubSubConfigFromByteString] "
                     "Loading PubSub configuration failed");
        goto cleanup;
    }

 cleanup:
    unlockServer(server);
    UA_ExtensionObject_clear(&decodedFile);
    return res;
}

/* Encodes a PubSubConfigurationDataType object as ByteString using the UA Binary Data
 * Encoding */
static UA_StatusCode
encodePubSubConfiguration(UA_PubSubManager *psm,
                          UA_PubSubConfigurationDataType *configurationParameters,
                          UA_ByteString *buffer) {
    UA_UABinaryFileDataType binFile;
    memset(&binFile, 0, sizeof(UA_UABinaryFileDataType));
    /*Perhaps, additional initializations of binFile are necessary here.*/

    UA_Variant_setScalar(&binFile.body, configurationParameters,
                         &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE]);

    UA_ExtensionObject container;
    memset(&container, 0, sizeof(UA_ExtensionObject));
    container.encoding = UA_EXTENSIONOBJECT_DECODED;
    container.content.decoded.type = &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE];
    container.content.decoded.data = &binFile;

    size_t fileSize = UA_ExtensionObject_calcSizeBinary(&container);
    buffer->data = (UA_Byte*)UA_calloc(fileSize, sizeof(UA_Byte));
    if(buffer->data == NULL) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_encodePubSubConfiguration] Allocating buffer failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    buffer->length = fileSize;

    UA_Byte *bufferPos = buffer->data;
    UA_StatusCode res =
        UA_ExtensionObject_encodeBinary(&container, &bufferPos, bufferPos + fileSize);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "[UA_PubSubManager_encodePubSubConfiguration] Encoding failed");
    }
    return res;
}

static UA_StatusCode
generatePublishedDataSetDataType(UA_PubSubManager *psm,
                                 const UA_PublishedDataSet *src,
                                 UA_PublishedDataSetDataType *dst) {
    if(src->config.publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS)
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    memset(dst, 0, sizeof(UA_PublishedDataSetDataType));

    UA_PublishedDataItemsDataType *tmp = UA_PublishedDataItemsDataType_new();
    if(!tmp)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    UA_StatusCode res = UA_String_copy(&src->config.name, &dst->name);

    /* Copy the full DataSetMetaData from the runtime PDS metadata rather than
     * reconstructing it field-by-field. The previous save path only wrote
     * fields[].name, fieldFlags, and configurationVersion, dropping the
     * DataSetMetaData name, description, dataSetClassId, namespaces, and
     * per-field builtInType, dataType, valueRank, arrayDimensions,
     * maxStringLength, dataSetFieldId, and properties. A tool reloading a
     * saved PDS could not reconstruct the dataset schema. */
    res |= UA_DataSetMetaDataType_copy(&src->dataSetMetaData, &dst->dataSetMetaData);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "generatePublishedDataSetDataType: DataSetMetaData copy failed");
        UA_PublishedDataSetDataType_clear(dst);
        UA_PublishedDataItemsDataType_delete(tmp);
        return res;
    }

    size_t index = 0;
    tmp->publishedDataSize = src->fieldSize;
    if(tmp->publishedDataSize > 0) {
        tmp->publishedData = (UA_PublishedVariableDataType*)
            UA_Array_new(tmp->publishedDataSize,
                         &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
        if(!tmp->publishedData) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "Allocation memory failed");
            UA_PublishedDataItemsDataType_delete(tmp);
            UA_PublishedDataSetDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }

    UA_DataSetField *dsf, *dsf_tmp = NULL;
    TAILQ_FOREACH_SAFE(dsf ,&src->fields, listEntry, dsf_tmp) {
        res = UA_PublishedVariableDataType_copy(
            &dsf->config.field.variable.publishParameters,
            &tmp->publishedData[index]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PublishedDataItemsDataType_delete(tmp);
            UA_PublishedDataSetDataType_clear(dst);
            return res;
        }
        index++;
    }
    UA_ExtensionObject_setValue(&dst->dataSetSource, tmp,
                                &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE]);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateDataSetWriterDataType(const UA_DataSetWriter *src,
                               UA_DataSetWriterDataType *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memset(dst, 0, sizeof(UA_DataSetWriterDataType));
    res |= UA_String_copy(&src->config.name, &dst->name);
    dst->dataSetWriterId = src->config.dataSetWriterId;
    dst->keyFrameCount = src->config.keyFrameCount;
    dst->dataSetFieldContentMask = src->config.dataSetFieldContentMask;
    res |= UA_ExtensionObject_copy(&src->config.messageSettings, &dst->messageSettings);
    res |= UA_ExtensionObject_copy(&src->config.transportSettings, &dst->transportSettings);
    res |= UA_String_copy(&src->config.dataSetName, &dst->dataSetName);
    /* Preserve the enabled flag so a save→load round-trip keeps components
     * enabled instead of silently disabling everything. */
    dst->enabled = src->config.enabled;
    if(res != UA_STATUSCODE_GOOD) {
        UA_DataSetWriterDataType_clear(dst);
        return res;
    }

    res = UA_Array_copy(src->config.dataSetWriterProperties.map,
                        src->config.dataSetWriterProperties.mapSize,
                        (void**)&dst->dataSetWriterProperties,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    if(res == UA_STATUSCODE_GOOD)
        dst->dataSetWriterPropertiesSize = src->config.dataSetWriterProperties.mapSize;
    else
        UA_DataSetWriterDataType_clear(dst);

    return res;
}

static UA_StatusCode
generateWriterGroupDataType(const UA_WriterGroup *src,
                             UA_WriterGroupDataType *dst) {
    memset(dst, 0, sizeof(UA_WriterGroupDataType));

    UA_StatusCode res = UA_String_copy(&src->config.name, &dst->name);
    dst->writerGroupId = src->config.writerGroupId;
    dst->publishingInterval = src->config.publishingInterval;
    dst->keepAliveTime = src->config.keepAliveTime;
    dst->priority = src->config.priority;
    dst->securityMode = src->config.securityMode;
    dst->enabled = src->config.enabled;
    dst->maxNetworkMessageSize = src->config.maxNetworkMessageSize;
    res |= UA_String_copy(&src->config.securityGroupId, &dst->securityGroupId);
    res |= UA_ExtensionObject_copy(&src->config.transportSettings,
                                   &dst->transportSettings);
    res |= UA_ExtensionObject_copy(&src->config.messageSettings,
                                   &dst->messageSettings);
    res |= UA_Array_copy(src->config.securityKeyServices,
                         src->config.securityKeyServicesSize,
                         (void**)&dst->securityKeyServices,
                         &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    res |= UA_Array_copy(src->config.groupProperties.map,
                         src->config.groupProperties.mapSize,
                         (void**)&dst->groupProperties,
                         &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_WriterGroupDataType_clear(dst);
        return res;
    }
    dst->securityKeyServicesSize = src->config.securityKeyServicesSize;
    dst->groupPropertiesSize = src->config.groupProperties.mapSize;

    if(src->writersCount > 0) {
        dst->dataSetWriters = (UA_DataSetWriterDataType*)
            UA_calloc(src->writersCount, sizeof(UA_DataSetWriterDataType));
        if(!dst->dataSetWriters) {
            UA_WriterGroupDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    dst->dataSetWritersSize = src->writersCount;

    UA_DataSetWriter *dsw;
    size_t dsWriterIndex = 0;
    LIST_FOREACH(dsw, &src->writers, listEntry) {
        res = generateDataSetWriterDataType(dsw,
                                             &dst->dataSetWriters[dsWriterIndex]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_WriterGroupDataType_clear(dst);
            return res;
        }
        dsWriterIndex++;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateDataSetReaderDataType(const UA_DataSetReader *src,
                              UA_DataSetReaderDataType *dst) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memset(dst, 0, sizeof(UA_DataSetReaderDataType));
    dst->writerGroupId = src->config.writerGroupId;
    dst->dataSetWriterId = src->config.dataSetWriterId;
    dst->dataSetFieldContentMask = src->config.dataSetFieldContentMask;
    dst->messageReceiveTimeout = src->config.messageReceiveTimeout;
    /* Preserve the enabled flag for round-trip consistency. */
    dst->enabled = src->config.enabled;
    res |= UA_String_copy(&src->config.name, &dst->name);
    res |= UA_DataSetMetaDataType_copy(&src->config.dataSetMetaData,
                                       &dst->dataSetMetaData);
    res |= UA_ExtensionObject_copy(&src->config.messageSettings, &dst->messageSettings);
    res |= UA_ExtensionObject_copy(&src->config.transportSettings, &dst->transportSettings);

    if(src->config.publisherIdFilterEnabled ||
       src->config.publisherId.idType != UA_PUBLISHERIDTYPE_BYTE ||
       src->config.publisherId.id.byte != 0) {
        UA_Variant var;
        UA_Variant_init(&var);
        UA_PublisherId_toVariant(&src->config.publisherId, &var);
        res |= UA_Variant_copy(&var, &dst->publisherId);
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_DataSetReaderDataType_clear(dst);
        return res;
    }

    UA_TargetVariablesDataType *tmpTarget = UA_TargetVariablesDataType_new();
    if(!tmpTarget) {
        UA_DataSetReaderDataType_clear(dst);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    const UA_TargetVariablesDataType *targets = &src->config.subscribedDataSet.target;
    if(targets->targetVariablesSize > 0) {
        tmpTarget->targetVariables = (UA_FieldTargetDataType *)
            UA_calloc(targets->targetVariablesSize, sizeof(UA_FieldTargetDataType));
        if(!tmpTarget->targetVariables) {
            UA_TargetVariablesDataType_delete(tmpTarget);
            UA_DataSetReaderDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    tmpTarget->targetVariablesSize = targets->targetVariablesSize;
    for(size_t i = 0; i < tmpTarget->targetVariablesSize; i++) {
        res = UA_FieldTargetDataType_copy(&targets->targetVariables[i],
                                          &tmpTarget->targetVariables[i]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_TargetVariablesDataType_delete(tmpTarget);
            UA_DataSetReaderDataType_clear(dst);
            return res;
        }
    }

    UA_ExtensionObject_setValue(&dst->subscribedDataSet, tmpTarget,
                                &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generateReaderGroupDataType(const UA_ReaderGroup *src,
                             UA_ReaderGroupDataType *dst) {
    memset(dst, 0, sizeof(UA_ReaderGroupDataType));

    UA_StatusCode res = UA_String_copy(&src->config.name, &dst->name);
    dst->enabled = src->config.enabled;
    dst->securityMode = src->config.securityMode;
    dst->maxNetworkMessageSize = src->config.maxNetworkMessageSize;
    res |= UA_String_copy(&src->config.securityGroupId, &dst->securityGroupId);
    res |= UA_ExtensionObject_copy(&src->config.transportSettings,
                                   &dst->transportSettings);
    res |= UA_ExtensionObject_copy(&src->config.messageSettings,
                                   &dst->messageSettings);
    res |= UA_Array_copy(src->config.securityKeyServices,
                         src->config.securityKeyServicesSize,
                         (void**)&dst->securityKeyServices,
                         &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    res |= UA_Array_copy(src->config.groupProperties.map,
                         src->config.groupProperties.mapSize,
                         (void**)&dst->groupProperties,
                         &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ReaderGroupDataType_clear(dst);
        return res;
    }
    dst->securityKeyServicesSize = src->config.securityKeyServicesSize;
    dst->groupPropertiesSize = src->config.groupProperties.mapSize;

    if(src->readersCount > 0) {
        dst->dataSetReaders = (UA_DataSetReaderDataType*)
            UA_calloc(src->readersCount, sizeof(UA_DataSetReaderDataType));
        if(!dst->dataSetReaders) {
            UA_ReaderGroupDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    dst->dataSetReadersSize = src->readersCount;

    size_t i = 0;
    UA_DataSetReader *dsr, *dsr_tmp = NULL;
    LIST_FOREACH_SAFE(dsr, &src->readers, listEntry, dsr_tmp) {
        res = generateDataSetReaderDataType(dsr, &dst->dataSetReaders[i]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_ReaderGroupDataType_clear(dst);
            return res;
        }
        i++;
    }

    return UA_STATUSCODE_GOOD;
}

/* Generates a PubSubConnectionDataType object from a PubSubConnection. */
static UA_StatusCode
generatePubSubConnectionDataType(UA_PubSubManager *psm,
                                 const UA_PubSubConnection *src,
                                 UA_PubSubConnectionDataType *dst) {
    const UA_DataType *publisherIdType;
    memset(dst, 0, sizeof(UA_PubSubConnectionDataType));

    UA_StatusCode res = UA_String_copy(&src->config.name, &dst->name);
    res |= UA_String_copy(&src->config.transportProfileUri,
                          &dst->transportProfileUri);
    if(res != UA_STATUSCODE_GOOD) {
        UA_PubSubConnectionDataType_clear(dst);
        return res;
    }
    dst->enabled = src->config.enabled;

    res = UA_Array_copy(src->config.connectionProperties.map,
                        src->config.connectionProperties.mapSize,
                        (void**)&dst->connectionProperties,
                        &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    if(res != UA_STATUSCODE_GOOD) {
        UA_PubSubConnectionDataType_clear(dst);
        return res;
    }
    dst->connectionPropertiesSize = src->config.connectionProperties.mapSize;

    switch (src->config.publisherId.idType) {
        case UA_PUBLISHERIDTYPE_BYTE:
            publisherIdType = &UA_TYPES[UA_TYPES_BYTE];
            break;
        case UA_PUBLISHERIDTYPE_UINT16:
            publisherIdType = &UA_TYPES[UA_TYPES_UINT16];
            break;
        case UA_PUBLISHERIDTYPE_UINT32:
            publisherIdType = &UA_TYPES[UA_TYPES_UINT32];
            break;
        case UA_PUBLISHERIDTYPE_UINT64:
            publisherIdType = &UA_TYPES[UA_TYPES_UINT64];
            break;
        case UA_PUBLISHERIDTYPE_STRING:
            publisherIdType = &UA_TYPES[UA_TYPES_STRING];
            break;
        default:
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "generatePubSubConnectionDataType(): publisher Id type is not supported");
            UA_PubSubConnectionDataType_clear(dst);
            return UA_STATUSCODE_BADINTERNALERROR;
    }
    res = UA_Variant_setScalarCopy(&dst->publisherId,
                                   &src->config.publisherId.id,
                                   publisherIdType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_PubSubConnectionDataType_clear(dst);
        return res;
    }

    /* Possibly, array size and dimensions of src->config->address and
     * src->config->connectionTransportSettings should be checked beforehand. */
    dst->address.encoding = UA_EXTENSIONOBJECT_DECODED;
    dst->address.content.decoded.type = src->config.address.type;
    res = UA_Array_copy(src->config.address.data, 1,
                        &dst->address.content.decoded.data,
                        src->config.address.type);
    if(res != UA_STATUSCODE_GOOD) {
        UA_PubSubConnectionDataType_clear(dst);
        return res;
    }

    if(src->config.connectionTransportSettings.data) {
        dst->transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        dst->transportSettings.content.decoded.type =
            src->config.connectionTransportSettings.type;
        res = UA_Array_copy(src->config.connectionTransportSettings.data, 1,
                            &dst->transportSettings.content.decoded.data,
                            src->config.connectionTransportSettings.type);

        if(res != UA_STATUSCODE_GOOD) {
            UA_PubSubConnectionDataType_clear(dst);
            return res;
        }
    }

    if(src->writerGroupsSize > 0) {
        dst->writerGroups = (UA_WriterGroupDataType*)
            UA_calloc(src->writerGroupsSize, sizeof(UA_WriterGroupDataType));
        if(!dst->writerGroups) {
            UA_PubSubConnectionDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }

    dst->writerGroupsSize = src->writerGroupsSize;
    UA_WriterGroup *wg, *wg_tmp = NULL;
    size_t wgIndex = 0;
    LIST_FOREACH_SAFE(wg, &src->writerGroups, listEntry, wg_tmp) {
        res = generateWriterGroupDataType(wg, &dst->writerGroups[wgIndex]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PubSubConnectionDataType_clear(dst);
            return res;
        }
        wgIndex++;
    }

    if(src->readerGroupsSize > 0) {
        dst->readerGroups = (UA_ReaderGroupDataType*)
            UA_calloc(src->readerGroupsSize, sizeof(UA_ReaderGroupDataType));
        if(!dst->readerGroups) {
            UA_PubSubConnectionDataType_clear(dst);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }

    dst->readerGroupsSize = src->readerGroupsSize;
    UA_ReaderGroup *rg = NULL;
    size_t rgIndex = 0;
    LIST_FOREACH(rg, &src->readerGroups, listEntry) {
        res = generateReaderGroupDataType(rg, &dst->readerGroups[rgIndex]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PubSubConnectionDataType_clear(dst);
            return res;
        }
        rgIndex++;
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
generatePubSubConfigurationDataType(UA_PubSubManager *psm,
                                    UA_PubSubConfigurationDataType *configDT) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    UA_PubSubConfigurationDataType_init(configDT);
    /* Preserve the global enabled flag (PubSubManager started/stopped state)
     * so a save→load round-trip auto-starts the PubSubManager if it was
     * running. Previously this flag was read on load but never written on
     * save, so every round-trip disabled everything. */
    configDT->enabled = (psm->drv.state == UA_LIFECYCLESTATE_STARTED);
    configDT->publishedDataSetsSize = psm->publishedDataSetsSize;
    if(configDT->publishedDataSetsSize > 0) {
        configDT->publishedDataSets = (UA_PublishedDataSetDataType*)
            UA_calloc(configDT->publishedDataSetsSize,
                      sizeof(UA_PublishedDataSetDataType));
        if(configDT->publishedDataSets == NULL)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_PublishedDataSet *pds;
    UA_UInt32 pdsIndex = 0;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    TAILQ_FOREACH(pds, &psm->publishedDataSets, listEntry) {
        UA_PublishedDataSetDataType *dst = &configDT->publishedDataSets[pdsIndex];
        res = generatePublishedDataSetDataType(psm, pds, dst);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "[UA_PubSubManager_generatePubSubConfigurationDataType] "
                         "retrieving PublishedDataSet configuration failed");
            return res;
        }
        pdsIndex++;
    }

    configDT->connectionsSize = psm->connectionsSize;
    if(configDT->connectionsSize > 0) {
        configDT->connections = (UA_PubSubConnectionDataType*)
            UA_calloc(configDT->connectionsSize,
                      sizeof(UA_PubSubConnectionDataType));
        if(configDT->connections == NULL)
            return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_UInt32 connectionIndex = 0;
    UA_PubSubConnection *connection;
    TAILQ_FOREACH(connection, &psm->connections, listEntry) {
        UA_PubSubConnectionDataType *cdt = &configDT->connections[connectionIndex];
        res = generatePubSubConnectionDataType(psm, connection, cdt);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "[UA_PubSubManager_generatePubSubConfigurationDataType] "
                         "retrieving PubSubConnection configuration failed");
            return res;
        }
        connectionIndex++;
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_writePubSubConfigurationToByteString(UA_Server *server,
                                               UA_ByteString *buffer) {
    if(!server || !buffer)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    lockServer(server);

    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_PubSubConfigurationDataType config;
    UA_PubSubConfigurationDataType_init(&config);

    UA_StatusCode res = generatePubSubConfigurationDataType(psm, &config);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "Retrieving PubSub configuration failed");
        goto cleanup;
    }

    res = encodePubSubConfiguration(psm, &config, buffer);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "encoding PubSub configuration failed");
        goto cleanup;
    }

    UA_LOG_INFO(psm->logging, UA_LOGCATEGORY_PUBSUB,
                "Saving PubSub config was successful");

 cleanup:
    unlockServer(server);
    UA_PubSubConfigurationDataType_clear(&config);
    return res;
}

#endif /* UA_ENABLE_PUBSUB && UA_ENABLE_PUBSUB_FILE_CONFIG */
