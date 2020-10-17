/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 */

#include <open62541/plugin/log_stdout.h>

#ifdef UA_ENABLE_PUBSUB
#ifdef UA_ENABLE_PUBSUB_ETH_UADP
#include <open62541/plugin/pubsub_ethernet.h>
#endif
#include <open62541/plugin/pubsub_udp.h>
#endif

#include "pubsub/ua_pubsub.h"
#include "pubsub/ua_pubsub_manager.h"
#include "pubsub/ua_pubsub_config.h"

#include "server/ua_server_internal.h"

#include <open62541/types_generated_encoding_binary.h>

#ifdef UA_ENABLE_PUBSUB_FILE_CONFIG

/* Function prototypes: */

static UA_StatusCode 
UA_PubSubManager_createPubSubConnection(UA_Server *server, const UA_PubSubConnectionDataType *const connectionParameters, 
                          const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent);

static UA_StatusCode 
UA_PubSubManager_createWriterGroup(UA_Server *server, const UA_WriterGroupDataType *const writerGroupParameters, 
                     const UA_NodeId connectionIdent, const UA_UInt32 pdsCount, 
                     const UA_NodeId *pdsIdent);

static UA_StatusCode 
UA_PubSubManager_createDataSetWriter(UA_Server *server, const UA_DataSetWriterDataType *const dataSetWriterParameters, 
                       const UA_NodeId writerGroupIdent, const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent);

static UA_StatusCode
UA_PubSubManager_createReaderGroup(UA_Server *server, const UA_ReaderGroupDataType *const readerGroupParameters, 
                     const UA_NodeId connectionIdent);

static UA_StatusCode
UA_PubSubManager_createDataSetReader(UA_Server *server, const UA_DataSetReaderDataType *const dataSetReaderParameters, 
                       const UA_NodeId readerGroupIdent);

static UA_StatusCode 
UA_PubSubManager_createPublishedDataSet(UA_Server *server, const UA_PublishedDataSetDataType *const publishedDataSetParameters, 
                          UA_NodeId *publishedDataSetIdent);

static UA_StatusCode 
UA_PubSubManager_createDataSetFields(UA_Server *server, const UA_NodeId *const publishedDataSetIdent, 
                       const UA_PublishedDataSetDataType *const publishedDataSetParameters);

static UA_StatusCode
UA_PubSubManager_generatePubSubConfigurationDataType(const UA_Server *server,
                                       UA_PubSubConfigurationDataType *pubSubConfiguration);

/* Function implementations: */

/* UA_PubSubManager_extractPubSubConfigFromDecodedObject() */
/**
 *  @brief      gets PubSub Configuration from an encoded ByteString
 * 
 *  @param      src         [in]    Decoded source object of type ExtensionObject
 *  @param      dst         [out]   Pointer on PubSub Configuration
 * 
 *  @return     UA_StatusCode
 */
static UA_StatusCode 
UA_PubSubManager_extractPubSubConfigFromDecodedObject(const UA_ExtensionObject *const src, UA_PubSubConfigurationDataType **dst) {
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    
    *dst = NULL;
    UA_UABinaryFileDataType *binFile;
    if((src->encoding == UA_EXTENSIONOBJECT_DECODED) && 
       UA_NodeId_equal(&src->content.decoded.type->typeId, 
                       &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE].typeId)) {
        binFile = (UA_UABinaryFileDataType*)src->content.decoded.data;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_extractPubSubConfigFromDecodedObject] Reading extensionObject failed");
        statusCode = UA_STATUSCODE_BADINVALIDARGUMENT;
        goto cleanup;
    }

    if(binFile->body.arrayLength != 0 || binFile->body.arrayDimensionsSize != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_PubSubManager_extractPubSubConfigFromDecodedObject] Loading multiple configurations is not supported");
        statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
        goto cleanup;
    }

    if(UA_NodeId_equal(&binFile->body.type->typeId, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE].typeId)) {
        *dst = (UA_PubSubConfigurationDataType*)binFile->body.data;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_PubSubManager_extractPubSubConfigFromDecodedObject] Invalid datatype encoded in the binary file");
        statusCode = UA_STATUSCODE_BADTYPEMISMATCH;
        goto cleanup;
    }

cleanup:

    return statusCode;
}

/* UA_PubSubManager_updatePubSubConfig() */
/**
 *  @brief      Configures a PubSub Server with given PubSubConfigurationDataType object
 * 
 *  @param      server                  [bi]    Server object
 *  @param      configurationParameters [in]    PubSub Configuration parameters
 * 
 *  @return     UA_StatusCode
 */
static UA_StatusCode 
UA_PubSubManager_updatePubSubConfig(UA_Server* server, const UA_PubSubConfigurationDataType *const configurationParameters) {
    if(server == NULL || configurationParameters == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_updatePubSubConfig] Invalid argument");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    UA_PubSubManager_delete(server, &(server->pubSubManager));
        
    /* Configuration of Published DataSets: */
    UA_UInt32 pdsCount = (UA_UInt32)configurationParameters->publishedDataSetsSize;
    UA_NodeId *publishedDataSetIdent = (UA_NodeId*)UA_calloc(pdsCount, sizeof(UA_NodeId));
    if(publishedDataSetIdent == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    for(UA_UInt32 pdsIndex = 0; pdsIndex < pdsCount; pdsIndex++) {
        statusCode = UA_PubSubManager_createPublishedDataSet(server, &configurationParameters->publishedDataSets[pdsIndex], 
                                               &publishedDataSetIdent[pdsIndex]);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_updatePubSubConfig] PDS creation failed");
            UA_free(publishedDataSetIdent);
            return statusCode;
        }
    }

    /* Configuration of PubSub Connections: */
    if(configurationParameters->connectionsSize < 1) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                       "[UA_PubSubManager_updatePubSubConfig] no connection in UA_PubSubConfigurationDataType");
        UA_free(publishedDataSetIdent);
        return UA_STATUSCODE_GOOD;
    }

    for(UA_UInt32 conIndex = 0; 
        conIndex < configurationParameters->connectionsSize && statusCode == UA_STATUSCODE_GOOD; 
        conIndex++) 
    {
        statusCode = UA_PubSubManager_createPubSubConnection(server, &configurationParameters->connections[conIndex], 
                                               pdsCount, publishedDataSetIdent);
    }

    UA_free(publishedDataSetIdent);
    return statusCode;
}

/* UA_PubSubManager_setConnectionPublisherId() */
/**
 * @brief       Function called by UA_PubSubManager_createPubSubConnection to set the PublisherId of a certain connection.
 * 
 * @param       src     [in]    PubSubConnection parameters
 * @param       dst     [out]   PubSubConfiguration
 * 
 * @return      UA_StatusCode
 */
static UA_StatusCode
UA_PubSubManager_setConnectionPublisherId(const UA_PubSubConnectionDataType *src, UA_PubSubConnectionConfig *dst) {
    if(UA_NodeId_equal(&src->publisherId.type->typeId, &UA_TYPES[UA_TYPES_STRING].typeId)) {
        dst->publisherIdType = UA_PUBSUB_PUBLISHERID_STRING;
        dst->publisherId.string = *(UA_String*)src->publisherId.data;
    } else if(UA_NodeId_equal(&src->publisherId.type->typeId, &UA_TYPES[UA_TYPES_BYTE].typeId) || 
              UA_NodeId_equal(&src->publisherId.type->typeId, &UA_TYPES[UA_TYPES_UINT16].typeId) || 
              UA_NodeId_equal(&src->publisherId.type->typeId, &UA_TYPES[UA_TYPES_UINT32].typeId)) {
        dst->publisherIdType = UA_PUBSUB_PUBLISHERID_NUMERIC;
        dst->publisherId.numeric =  *(UA_UInt32*)src->publisherId.data;
    } else if(UA_NodeId_equal(&src->publisherId.type->typeId, &UA_TYPES[UA_TYPES_UINT64].typeId)) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_PubSubManager_setConnectionPublisherId] PublisherId is UInt64 (not implemented); Recommended dataType for PublisherId: UInt32");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_setConnectionPublisherId] PublisherId is not valid.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/* UA_PubSubManager_createComponentsForConnection() */
/**
 * @brief   Function called by UA_PubSubManager_createPubSubConnection to create all WriterGroups and ReaderGroups
 *          that belong to a certain connection.
 * 
 * @param   server                  [bi]
 * @param   connectionParameters    [in]
 * @param   connectionIdent         [in]
 * @param   pdsCount                [in]
 * @param   pdsIdent                [in]
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_createComponentsForConnection(UA_Server *server, const UA_PubSubConnectionDataType *const connectionParameters, 
                                 UA_NodeId connectionIdent, 
                                 const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* WriterGroups configuration */
    for(UA_UInt32 wGroupIndex=0; 
        wGroupIndex < connectionParameters->writerGroupsSize; 
        wGroupIndex++)
    {
        retVal = UA_PubSubManager_createWriterGroup(server, &connectionParameters->writerGroups[wGroupIndex], 
                                      connectionIdent, pdsCount, pdsIdent);
        if(retVal == UA_STATUSCODE_GOOD) {
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                         "[UA_PubSubManager_createComponentsForConnection] Error occured during %d.WriterGroup Creation", wGroupIndex+1);
            return retVal;
        }
    }

    /* ReaderGroups configuration */
    for(UA_UInt32 rGroupIndex=0; 
        rGroupIndex < connectionParameters->readerGroupsSize; 
        rGroupIndex++) 
    {
        retVal = UA_PubSubManager_createReaderGroup(server, &connectionParameters->readerGroups[rGroupIndex], connectionIdent);
        if(retVal == UA_STATUSCODE_GOOD) {

            retVal |= UA_PubSubConnection_regist(server, &connectionIdent);
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_PubSubManager_createComponentsForConnection] Error occured during %d.ReaderGroup Creation", rGroupIndex+1);
            return retVal;
        }
    }
    return retVal;
}

/**
 * @brief       Checks if transportLayer for the specified transportProfileUri exists.
 * 
 * @param       server                  [bi]    Server object that shall be configured
 * @param       transportProfileUri     [in]    String that specifies the transport protocol
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_Boolean
UA_PubSubManager_transportLayerExists(UA_Server *server, UA_String transportProfileUri) {
    UA_Boolean tlExists= UA_FALSE;
    for(UA_UInt32 tlIndex=0; tlIndex < server->config.pubsubTransportLayersSize; tlIndex++) {
        if(UA_String_equal(&server->config.pubsubTransportLayers[tlIndex].transportProfileUri, &transportProfileUri)) {
            tlExists = UA_TRUE;
            break;
        }
    }
    return tlExists;
}

/**
 *  @brief      Creates transportlayer for specified transport protocol if this layer doesn't exist yet.
 * 
 *  @param      server                  [bi]    Server object that shall be configured
 *  @param      transportProfileUri     [in]    String that specifies the transport protocol
 * 
 *  @return     UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_createTransportLayer(UA_Server *server, const UA_String transportProfileUri) {
    if(UA_PubSubManager_transportLayerExists(server, transportProfileUri)) {
        return UA_STATUSCODE_GOOD;
    }

    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_PubSubTransportLayer tl;

    do {
        UA_String strUDP = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
        if(UA_String_equal(&transportProfileUri, &strUDP)) {
            tl = UA_PubSubTransportLayerUDPMP();
            break;
        }

    #ifdef UA_ENABLE_PUBSUB_ETH_UADP
        UA_String strETH = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp");
        if(UA_String_equal(&transportProfileUri, &strETH)) {
            tl = UA_PubSubTransportLayerEthernet();
            break;
        }
    #endif

        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_createTransportLayer] invalid transportProfileUri");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    } while(0);

    if(config->pubsubTransportLayersSize > 0) {
        config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_realloc(config->pubsubTransportLayers,
                                                                    (config->pubsubTransportLayersSize + 1) * sizeof(UA_PubSubTransportLayer));
    } else {
        config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_calloc(1, sizeof(UA_PubSubTransportLayer));
    }

    if(config->pubsubTransportLayers == NULL) {
        UA_Server_delete(server);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    config->pubsubTransportLayers[config->pubsubTransportLayersSize] = tl;
    config->pubsubTransportLayersSize++;
    return UA_STATUSCODE_GOOD;
}

/* UA_PubSubManager_createPubSubConnection() */
/**
 *  @brief      Creates PubSubConnection configuration from PubSubConnectionDataType object
 * 
 *  @param      server      [bi]    Server object that shall be configured
 *  @param      connectionParameters  [in]    PubSub connection configuration
 *  @param      pdsCount    [in]    Number of published DataSets
 *  @param      pdsIdent    [in]    Array of NodeIds of the published DataSets
 * 
 *  @return     UA_StatusCode
 */
static UA_StatusCode 
UA_PubSubManager_createPubSubConnection(UA_Server *server, const UA_PubSubConnectionDataType *const connectionParameters, 
                          const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent) {
    UA_PubSubConnectionConfig config;
    memset(&config, 0, sizeof(UA_PubSubConnectionConfig));

    config.name =                       connectionParameters->name;
    config.enabled =                    connectionParameters->enabled;
    config.transportProfileUri =        connectionParameters->transportProfileUri;
    config.connectionPropertiesSize =   connectionParameters->connectionPropertiesSize;
    if(config.connectionPropertiesSize > 0) {
        config.connectionProperties = connectionParameters->connectionProperties;
    }

    UA_StatusCode statusCode = UA_PubSubManager_setConnectionPublisherId(connectionParameters, &config);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_createPubSubConnection] Setting PublisherId failed");
        return statusCode;
    }

    if(connectionParameters->address.encoding == UA_EXTENSIONOBJECT_DECODED) {
        UA_Variant_setScalar(&(config.address), 
                             connectionParameters->address.content.decoded.data, 
                             connectionParameters->address.content.decoded.type);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_PubSubManager_createPubSubConnection] Reading connection address failed");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(connectionParameters->transportSettings.encoding == UA_EXTENSIONOBJECT_DECODED) {
        UA_Variant_setScalar(&(config.connectionTransportSettings), 
                             connectionParameters->transportSettings.content.decoded.data, 
                             connectionParameters->transportSettings.content.decoded.type);
    } else { 
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                       "[UA_PubSubManager_createPubSubConnection] TransportSettings can not be read");
    }

    statusCode = UA_PubSubManager_createTransportLayer(server, connectionParameters->transportProfileUri);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_createPubSubConnection] Creating transportLayer failed");
        return statusCode;
    }

    /* Load connection config into server: */
    UA_NodeId connectionIdent;
    statusCode = UA_Server_addPubSubConnection(server, &config, &connectionIdent);
    if(statusCode == UA_STATUSCODE_GOOD) {
        /* Configuration of all Components that belong to this connection: */
        statusCode = UA_PubSubManager_createComponentsForConnection(server, connectionParameters, connectionIdent, pdsCount, pdsIdent);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_PubSubManager_createPubSubConnection] Connection creation failed");
    }

    return statusCode;
}

/* UA_PubSubManager_setWriterGroupEncodingType() */
/**
 * @brief   Function called by UA_PubSubManager_createWriterGroup to configure the messageSettings of a writerGroup.
 * 
 * @param   writerGroupParameters   [in]
 * @param   config                  [bi]
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode 
UA_PubSubManager_setWriterGroupEncodingType(const UA_WriterGroupDataType *writerGroupParameters, 
                              UA_WriterGroupConfig *config) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(writerGroupParameters->messageSettings.encoding == UA_EXTENSIONOBJECT_DECODED) {
        if(UA_NodeId_equal(&writerGroupParameters->messageSettings.content.decoded.type->typeId, 
                           &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE].typeId)) {
            config->encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        } else if(writerGroupParameters->messageSettings.content.decoded.type->typeId.identifier.numeric == 
                  UA_NS0ID_JSONWRITERGROUPMESSAGEDATATYPE) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_PubSubManager_setWriterGroupEncodingType] encoding type: JSON (not implemented!)");
            retVal = UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_PubSubManager_setWriterGroupEncodingType] invalid message encoding type");
            retVal = UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_PubSubManager_setWriterGroupEncodingType] getting message type information failed");
        retVal = UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return retVal;
}

/* UA_PubSubManager_createWriterGroup() */
/**
 *  @brief      Creates WriterGroup configuration from WriterGroup object
 * 
 *  @param      server                  [bi]    Server object that shall be configured
 *  @param      writerGroupParameters   [in]    WriterGroup configuration
 *  @param      connectionIdent         [in]    NodeId of the PubSub connection, the WriterGroup belongs to
 *  @param      pdsCount                [in]    Number of published DataSets
 *  @param      pdsIdent                [in]    Array of NodeIds of the published DataSets
 * 
 *  @return     UA_StatusCode
 */
static UA_StatusCode 
UA_PubSubManager_createWriterGroup(UA_Server *server, const UA_WriterGroupDataType *const writerGroupParameters, 
                     const UA_NodeId connectionIdent,
                     const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent) {
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    UA_WriterGroupConfig config;
    
    memset(&config, 0, sizeof(UA_WriterGroupConfig));

    config.name = writerGroupParameters->name;
    config.enabled =               writerGroupParameters->enabled;
    config.writerGroupId =         writerGroupParameters->writerGroupId;
    config.publishingInterval =    writerGroupParameters->publishingInterval;
    config.keepAliveTime =         writerGroupParameters->keepAliveTime;
    config.priority =              writerGroupParameters->priority;
    config.securityMode =          writerGroupParameters->securityMode;

    config.transportSettings = writerGroupParameters->transportSettings;
    config.messageSettings = writerGroupParameters->messageSettings;
    config.groupPropertiesSize =   writerGroupParameters->groupPropertiesSize;
    if(config.groupPropertiesSize > 0) {
        config.groupProperties = writerGroupParameters->groupProperties;
    }

    config.maxEncapsulatedDataSetMessageCount = 255; /* non std parameter */

    statusCode = UA_PubSubManager_setWriterGroupEncodingType(writerGroupParameters, &config);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_createWriterGroup] Setting message settings failed");
        return statusCode;
    }

    /* Load config into server: */
    UA_NodeId writerGroupIdent;
    statusCode = UA_Server_addWriterGroup(server, connectionIdent, &config, &writerGroupIdent);
    UA_Server_setWriterGroupOperational(server, writerGroupIdent);
    if(statusCode == UA_STATUSCODE_GOOD) {    
        /* Configuration of all DataSetWriters that belong to this WriterGroup */
        for(UA_UInt32 dsWriterIndex=0; 
            dsWriterIndex < writerGroupParameters->dataSetWritersSize && statusCode == UA_STATUSCODE_GOOD; 
            dsWriterIndex++)
        {
            statusCode = UA_PubSubManager_createDataSetWriter(server, &writerGroupParameters->dataSetWriters[dsWriterIndex],
                                                     writerGroupIdent, pdsCount, pdsIdent);
            if(statusCode != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                            "[UA_PubSubManager_createWriterGroup] DataSetWriter Creation failed.");
            }
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_PubSubManager_createWriterGroup] Adding WriterGroup to server failed: 0x%x", statusCode);
    }

    return statusCode;
}

/* UA_PubSubManager_addDataSetWriterWithPdsReference() */
/**
 * @brief   Function called by UA_PubSubManager_createDataSetWriter. It searches for a PublishedDataSet that is referenced by
 *          the DataSetWriter. If a related PDS is found, the DSWriter will be added to the server, 
 *          otherwise, no DSWriter will be added.
 * 
 * @param   server              [bi]    UA_Server object that shall be configured
 * @param   writerGroupIdent    [in]    NodeId of writerGroup, the DataSetWriter belongs to
 * @param   dsWriterConfig      [in]    WriterGroup configuration
 * @param   pdsCount            [in]    Number of published DataSets
 * @param   pdsIdent            [in]    Array of NodeIds of the published DataSets
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_addDataSetWriterWithPdsReference(UA_Server *server, const UA_NodeId writerGroupIdent, 
                                    const UA_DataSetWriterConfig *dsWriterConfig,
                                    const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent) {
    UA_NodeId dataSetWriterIdent;
    UA_PublishedDataSetConfig pdsConfig;
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    UA_Boolean pdsFound = UA_FALSE;

    for(UA_UInt32 pdsIndex = 0; 
        pdsIndex < pdsCount && retVal == UA_STATUSCODE_GOOD; 
        pdsIndex++) 
    {
        retVal = UA_Server_getPublishedDataSetConfig(server, pdsIdent[pdsIndex], &pdsConfig);
        /* members of pdsConfig must be deleted manually */
        if(retVal == UA_STATUSCODE_GOOD) {
            if(dsWriterConfig->dataSetName.length == pdsConfig.name.length &&
               0 == strncmp((const char *)dsWriterConfig->dataSetName.data, (const char *)pdsConfig.name.data, 
                             dsWriterConfig->dataSetName.length)) 
            {
                /* DSWriter will only be created, if a matching PDS is found: */
                retVal = UA_Server_addDataSetWriter(server, writerGroupIdent, pdsIdent[pdsIndex], 
                                                    dsWriterConfig, &dataSetWriterIdent);
                if(retVal != UA_STATUSCODE_GOOD) {
                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                                 "[UA_PubSubManager_addDataSetWriterWithPdsReference] Adding DataSetWriter failed");
                } else {
                    pdsFound = UA_TRUE;
                }
            }

            UA_PublishedDataSetConfig_clear(&pdsConfig);
            if(pdsFound)
                break; /* break loop if corresponding publishedDataSet was found */
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_PubSubManager_addDataSetWriterWithPdsReference] Getting pdsConfig from NodeId failed.");
        }
    }

    if(!pdsFound) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_PubSubManager_addDataSetWriterWithPdsReference] No matching DataSet found; no DataSetWriter created");
    }

    return retVal;
}

/* UA_PubSubManager_createDataSetWriter() */
/**
 *  @brief      Creates DataSetWriter configuration from DataSetWriter object
 * 
 *  @param      server                  [bi]    UA_Server object that shall be configured
 *  @param      dataSetWriterParameters [in]    DataSetWriter Configuration
 *  @param      writerGroupIdent        [in]    NodeId of writerGroup, the DataSetWriter belongs to
 *  @param      pdsCount                [in]    Number of published DataSets
 *  @param      pdsIdent                [in]    Array of NodeIds of the published DataSets
 * 
 *  @return     UA_StatusCode
 */
static UA_StatusCode 
UA_PubSubManager_createDataSetWriter(UA_Server *server, const UA_DataSetWriterDataType *const dataSetWriterParameters, 
                       const UA_NodeId writerGroupIdent, const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent) {
    UA_DataSetWriterConfig config;
    memset(&config, 0, sizeof(UA_DataSetWriterConfig));

    config.name =                          dataSetWriterParameters->name;
    config.dataSetWriterId =               dataSetWriterParameters->dataSetWriterId;
    config.keyFrameCount =                 dataSetWriterParameters->keyFrameCount;
    config.dataSetFieldContentMask =       dataSetWriterParameters->dataSetFieldContentMask;
    config.messageSettings =               dataSetWriterParameters->messageSettings;
    config.dataSetName =                   dataSetWriterParameters->dataSetName;
    config.dataSetWriterPropertiesSize =   dataSetWriterParameters->dataSetWriterPropertiesSize;
    if(config.dataSetWriterPropertiesSize > 0) {
        config.dataSetWriterProperties = dataSetWriterParameters->dataSetWriterProperties;
    }

    UA_StatusCode statusCode = UA_PubSubManager_addDataSetWriterWithPdsReference(server, writerGroupIdent, 
                                                                   &config, pdsCount, pdsIdent);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_createDataSetWriter] Referencing related PDS failed");
    }
    
    return statusCode;
}

/* UA_PubSubManager_createReaderGroup() */
/**
 * @brief       Creates ReaderGroup configuration from ReaderGroup object
 * 
 * @param       server                  [bi]    UA_Server object that shall be configured
 * @param       readerGroupParameters   [in]    ReaderGroup configuration
 * @param       connectionIdent         [in]    NodeId of the PubSub connection, the ReaderGroup belongs to
 * 
 * @return      UA_StatusCode
 */
static UA_StatusCode
UA_PubSubManager_createReaderGroup(UA_Server *server, const UA_ReaderGroupDataType *const readerGroupParameters, 
                     const UA_NodeId connectionIdent) {    
    UA_ReaderGroupConfig config;
    
    memset(&config, 0, sizeof(UA_ReaderGroupConfig));

    config.name                                 = readerGroupParameters->name;
    config.securityParameters.securityMode      = readerGroupParameters->securityMode;

    UA_NodeId readerGroupIdent;

    UA_StatusCode statusCode = UA_Server_addReaderGroup(server, connectionIdent, &config, &readerGroupIdent);
    if(statusCode == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_createReaderGroup] ReaderGroup successfully added.");
        for(UA_UInt32 dsReaderIndex=0; 
            dsReaderIndex < readerGroupParameters->dataSetReadersSize && statusCode == UA_STATUSCODE_GOOD; 
            dsReaderIndex++)
        {
            statusCode = UA_PubSubManager_createDataSetReader(server, &readerGroupParameters->dataSetReaders[dsReaderIndex], readerGroupIdent);
            if(statusCode != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                             "[UA_PubSubManager_createReaderGroup] Creating DataSetReader failed");
            }
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_PubSubManager_createReaderGroup] Adding ReaderGroup to server failed: 0x%x", statusCode);
    }
    
    if(statusCode == UA_STATUSCODE_GOOD)
        UA_Server_setReaderGroupOperational(server, readerGroupIdent);

    return statusCode;
}


/* UA_PubSubManager_addSubscribedDataSet() */
/**
 * @brief   Creates TargetVariables or SubscribedDataSetMirror for a given DataSetReader
 * 
 * @param   server                  [bi]    UA_Server object that shall be configured
 * @param   dsReaderIdent           [in]    NodeId of the DataSetReader the SubscribedDataSet belongs to
 * @param   dataSetReaderParameters [in]    Configuration Parameters of the DataSetReader 
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_addSubscribedDataSet(UA_Server *server, const UA_NodeId dsReaderIdent, 
                        const UA_ExtensionObject *const subscribedDataSet) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    if(UA_NodeId_equal(&subscribedDataSet->content.decoded.type->typeId, 
                        &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE].typeId)) {
        UA_TargetVariablesDataType *tmpTargetVars = 
            (UA_TargetVariablesDataType*)subscribedDataSet->content.decoded.data;
        UA_FieldTargetVariable *targetVars = 
                        (UA_FieldTargetVariable *)UA_calloc(tmpTargetVars->targetVariablesSize, sizeof(UA_FieldTargetVariable));
        memset(targetVars, 0, sizeof(UA_FieldTargetVariable));
        
        for(size_t index = 0; index < tmpTargetVars->targetVariablesSize; index++) {
            UA_FieldTargetDataType_copy(&tmpTargetVars->targetVariables[index] ,&targetVars[index].targetVariable);
        }

        retVal = UA_Server_DataSetReader_createTargetVariables(server, dsReaderIdent, tmpTargetVars->targetVariablesSize, targetVars);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_PubSubManager_addSubscribedDataSet] create TargetVariables failed");
        }

        for(size_t index = 0; index < tmpTargetVars->targetVariablesSize; index++) {
            UA_FieldTargetDataType_clear(&targetVars[index].targetVariable);
        }

        UA_free(targetVars);
        return retVal;
    } 
    
    if(UA_NodeId_equal(&subscribedDataSet->content.decoded.type->typeId,
                        &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE].typeId)) {
        
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_addSubscribedDataSet] DataSetMirror is currently not supported");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    } 

    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_addSubscribedDataSet] Invalid Type of SubscribedDataSet");
    return UA_STATUSCODE_BADINTERNALERROR;
}

/* UA_PubSubManager_createDataSetReader() */
/**
 * @brief       Creates DataSetReader configuration from DataSetReader object
 * 
 * @param       server                  [bi]    UA_Server object that shall be configured
 * @param       dataSetReaderParameters [in]    DataSetReader configuration
 * @param       writerGroupIdent        [in]    NodeId of readerGroupParameters, the DataSetReader belongs to
 *
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_createDataSetReader(UA_Server *server, const UA_DataSetReaderDataType *const dataSetReaderParameters, 
                       const UA_NodeId readerGroupIdent) {
    UA_DataSetReaderConfig config;
    memset(&config, 0, sizeof(UA_DataSetReaderConfig));

    config.name =                   dataSetReaderParameters->name;
    config.publisherId  =           dataSetReaderParameters->publisherId;
    config.writerGroupId =          dataSetReaderParameters->writerGroupId;
    config.dataSetWriterId =        dataSetReaderParameters->dataSetWriterId;
    config.dataSetMetaData =        dataSetReaderParameters->dataSetMetaData;
    config.dataSetFieldContentMask = dataSetReaderParameters->dataSetFieldContentMask;
    config.messageReceiveTimeout =  dataSetReaderParameters->messageReceiveTimeout;
    config.messageSettings = dataSetReaderParameters->messageSettings;

    UA_NodeId dsReaderIdent;
    UA_StatusCode statusCode = UA_Server_addDataSetReader (server, readerGroupIdent, &config, &dsReaderIdent);
    if(statusCode == UA_STATUSCODE_GOOD) {
        statusCode = UA_PubSubManager_addSubscribedDataSet(server, dsReaderIdent, &dataSetReaderParameters->subscribedDataSet);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_PubSubManager_createDataSetReader] create subscribedDataSet failed");
        }
    }

    return statusCode;
}

/* UA_PubSubManager_setPublishedDataSetType() */
/**
 * @brief   Determines whether PublishedDataSet is of type PublishedItems or PublishedEvents.
 *          (PublishedEvents are currently not supported!) 
 * 
 * @param   publishedDataSetParameters  [in]    PublishedDataSet parameters
 * @param   config                      [bi]    PublishedDataSet configuration object
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_setPublishedDataSetType(const UA_PublishedDataSetDataType *const publishedDataSetParameters, 
                           UA_PublishedDataSetConfig *config) {
    if(publishedDataSetParameters->dataSetSource.encoding != UA_EXTENSIONOBJECT_DECODED) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(UA_NodeId_equal(&publishedDataSetParameters->dataSetSource.content.decoded.type->typeId, 
                       &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE].typeId)) { 
        config->publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    } else if(UA_NodeId_equal(&publishedDataSetParameters->dataSetSource.content.decoded.type->typeId,
                              &UA_TYPES[UA_TYPES_PUBLISHEDEVENTSDATATYPE].typeId)) {
        /* config.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDEVENTS; */
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_PubSubManager_setPublishedDataSetType] Published events not supported.");
        retVal = UA_STATUSCODE_BADNOTIMPLEMENTED;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_PubSubManager_setPublishedDataSetType] Invalid DataSetSourceDataType.");
        retVal = UA_STATUSCODE_BADINTERNALERROR;
    }

    return retVal;
}

/* UA_PubSubManager_createPublishedDataSet() */
/**
 *  @brief      Creates PublishedDataSetConfig object from PublishedDataSet object
 * 
 *  @param      server                      [bi]    UA_Server object that shall be configured
 *  @param      publishedDataSetParameters  [in]    publishedDataSet configuration
 *  @param      publishedDataSetIdent       [out]   NodeId of the publishedDataSet
 * 
 *  @return     UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode 
UA_PubSubManager_createPublishedDataSet(UA_Server *server, const UA_PublishedDataSetDataType *const publishedDataSetParameters, 
                          UA_NodeId *publishedDataSetIdent) {
    UA_PublishedDataSetConfig config;
    memset(&config, 0, sizeof(UA_PublishedDataSetConfig));

    config.name = publishedDataSetParameters->name;
    UA_StatusCode statusCode = UA_PubSubManager_setPublishedDataSetType(publishedDataSetParameters, &config);
    if(statusCode != UA_STATUSCODE_GOOD) {
        return statusCode;
    }

    statusCode = UA_Server_addPublishedDataSet(server, &config, publishedDataSetIdent).addResult;
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_PubSubManager_createPublishedDataSet] Adding PublishedDataSet failed.");
        return statusCode;
    }

    /* DataSetField configuration for this publishedDataSet: */
    statusCode = UA_PubSubManager_createDataSetFields(server, publishedDataSetIdent, publishedDataSetParameters);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_PubSubManager_createPublishedDataSet] Creating DataSetFieldConfig failed.");
    }

    return statusCode;
}

/* UA_PubSubManager_addDataSetFieldVariables */
/**
 * @brief   Adds DataSetField Variables bound to a certain PublishedDataSet.
 *          This method does NOT check, whether the PublishedDataSet actually contains Variables instead of Events!
 * 
 * @param   server                      [bi]    UA_Server object that shall be configured
 * @param   publishedDataSetIdent       [in]    NodeId of the publishedDataSet, the DataSetField belongs to
 * @param   publishedDataSetParameters  [in]    publishedDataSet configuration
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_addDataSetFieldVariables(UA_Server *server, const UA_NodeId *publishedDataSetIdent,
                            const UA_PublishedDataSetDataType *const publishedDataSetParameters) {
    UA_PublishedDataItemsDataType *publishedDataItems = 
        (UA_PublishedDataItemsDataType *)publishedDataSetParameters->dataSetSource.content.decoded.data;
    if(publishedDataItems->publishedDataSize != publishedDataSetParameters->dataSetMetaData.fieldsSize){
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    for(UA_Int32 dsFieldIndex = 0;
        dsFieldIndex < (UA_Int32)publishedDataItems->publishedDataSize && statusCode == UA_STATUSCODE_GOOD;
        dsFieldIndex++)
    {
        UA_DataSetFieldConfig fieldConfig;
        memset(&fieldConfig, 0, sizeof(UA_DataSetFieldConfig));
        fieldConfig.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
        fieldConfig.field.variable.configurationVersion = publishedDataSetParameters->dataSetMetaData.configurationVersion;
        fieldConfig.field.variable.fieldNameAlias = publishedDataSetParameters->dataSetMetaData.fields[dsFieldIndex].name;
        fieldConfig.field.variable.promotedField = publishedDataSetParameters->dataSetMetaData.
                                                   fields[dsFieldIndex].fieldFlags & 0x0001;
        fieldConfig.field.variable.publishParameters = publishedDataItems->publishedData[dsFieldIndex];

        UA_NodeId fieldIdent;
        statusCode = UA_Server_addDataSetField(server, *publishedDataSetIdent, &fieldConfig, &fieldIdent).result;
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_PubSubManager_addDataSetFieldVariables] Adding DataSetField Variable failed.");
        }
    }   

    return UA_STATUSCODE_GOOD;
}

/* UA_PubSubManager_createDataSetFields() */
/**
 *  @brief      Checks if PublishedDataSet contains event or variable fields and calls the corresponding method
 *              to add these fields to the server.
 * 
 *  @param      server                      [bi]    UA_Server object that shall be configured
 *  @param      publishedDataSetIdent       [in]    NodeId of the publishedDataSet, the DataSetFields belongs to
 *  @param      publishedDataSetParameters  [in]    publishedDataSet configuration
 * 
 *  @return     UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode 
UA_PubSubManager_createDataSetFields(UA_Server *server, const UA_NodeId *const publishedDataSetIdent, 
                       const UA_PublishedDataSetDataType *const publishedDataSetParameters) {
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    
    if(publishedDataSetParameters->dataSetSource.encoding != UA_EXTENSIONOBJECT_DECODED) {
        statusCode = UA_STATUSCODE_BADINTERNALERROR;
        goto cleanup;
    }

    if(UA_NodeId_equal(&publishedDataSetParameters->dataSetSource.content.decoded.type->typeId, 
                        &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE].typeId)) { 
        statusCode = UA_PubSubManager_addDataSetFieldVariables(server, publishedDataSetIdent, publishedDataSetParameters);
        goto cleanup;
    } 

    if(publishedDataSetParameters->dataSetSource.content.decoded.type->typeId.identifier.numeric == 
        UA_NS0ID_PUBLISHEDEVENTSDATATYPE) {
        /* This is a placeholder; TODO: Implement Routine for adding Event DataSetFields */
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_PubSubManager_createDataSetFields] Published events not supported.");
        statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
        goto cleanup;
    } 

    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_createDataSetFields] Invalid DataSetSourceDataType.");
    statusCode = UA_STATUSCODE_BADINTERNALERROR;

cleanup:
    return statusCode;
}

/* UA_PubSubManager_loadPubSubConfigFromByteString() */
/**
 * @brief       Decodes the information from the ByteString. If the decoded content is a PubSubConfiguration object
 *              it will overwrite the current PubSub configuration from the server.
 * 
 * @param       server      [bi]    Pointer to Server object that shall be configured
 * @param       buffer      [in]    Relative path and name of the file that contains the PubSub configuration
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode 
UA_PubSubManager_loadPubSubConfigFromByteString(UA_Server *server, const UA_ByteString buffer) {
    UA_StatusCode statusCode;
    UA_ExtensionObject decodedFile;
    memset(&decodedFile, 0, sizeof(UA_ExtensionObject)); /* Prevents valgrind errors in case of invalid filename */

    size_t offset = 0;
    statusCode = UA_ExtensionObject_decodeBinary(&buffer, &offset, &decodedFile);
    if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_decodeBinFile] decoding UA_Binary failed");
            goto cleanup;
    }
        
    UA_PubSubConfigurationDataType *pubSubConfig;
    statusCode = UA_PubSubManager_extractPubSubConfigFromDecodedObject(&decodedFile, &pubSubConfig);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_PubSubManager_loadPubSubConfigFromByteString] Extracting PubSub Configuration failed");
        goto cleanup;
    }

    statusCode = UA_PubSubManager_updatePubSubConfig(server, pubSubConfig);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_PubSubManager_loadPubSubConfigFromByteString] Loading PubSub configuration into server failed");
        goto cleanup;
    }

cleanup:
    UA_ExtensionObject_clear(&decodedFile);
    return statusCode;
}

/* UA_PubSubManager_encodePubSubConfiguration() */
/**
 * @brief       Encodes a PubSubConfigurationDataType object as ByteString using the UA Binary Data Encoding.
 * @param       configurationParameters     [in]
 * @param       buffer                      [out]
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_encodePubSubConfiguration(UA_PubSubConfigurationDataType *configurationParameters,
                             UA_ByteString *buffer) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    UA_UABinaryFileDataType binFile;
    memset(&binFile, 0, sizeof(UA_UABinaryFileDataType));
    /*Perhaps, additional initializations of binFile are necessary here.*/
    
    UA_Variant_setScalar(&binFile.body, configurationParameters, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE]);
    
    UA_ExtensionObject container;
    memset(&container, 0, sizeof(UA_ExtensionObject));
    container.encoding = UA_EXTENSIONOBJECT_DECODED;
    container.content.decoded.type = &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE];
    container.content.decoded.data = &binFile;

    size_t fileSize = UA_ExtensionObject_calcSizeBinary(&container);
    buffer->data = (UA_Byte*)UA_calloc(fileSize, sizeof(UA_Byte));
    if(buffer->data == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_encodePubSubConfiguration] Allocating buffer failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    buffer->length = fileSize;

    UA_Byte *bufferPos = buffer->data;
    retVal = UA_ExtensionObject_encodeBinary(&container, &bufferPos, bufferPos + fileSize);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_PubSubManager_encodePubSubConfiguration] Encoding failed");
    }

    return retVal;
}

/* UA_PubSubManager_generatePublishedDataSetDataType() */
/**
 * @brief       Generates a PublishedDataSetDataType object from a PublishedDataSet.
 * @param       dst     [out]   PublishedDataSetDataType
 * @param       src     [in]    PublishedDataSet
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_generatePublishedDataSetDataType(UA_PublishedDataSetDataType *dst,
                                    const UA_PublishedDataSet *src) {
    if(src->config.publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS) {
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    memset(dst, 0, sizeof(UA_PublishedDataSetDataType));
    
    UA_PublishedDataItemsDataType *tmp = UA_PublishedDataItemsDataType_new();
    UA_String_copy(&src->config.name, &dst->name);
    dst->dataSetMetaData.fieldsSize = src->fieldSize;
    
    size_t index = 0;
    tmp->publishedDataSize = src->fieldSize;
    tmp->publishedData = (UA_PublishedVariableDataType*)UA_Array_new(tmp->publishedDataSize, &UA_TYPES[UA_TYPES_PUBLISHEDVARIABLEDATATYPE]);
    if(tmp->publishedData == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Allocation memory failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    dst->dataSetMetaData.fields = (UA_FieldMetaData*)UA_Array_new(dst->dataSetMetaData.fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    if(dst->dataSetMetaData.fields == NULL) {
        UA_free(tmp->publishedData);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Allocation memory failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_DataSetField *dsf, *dsf_tmp = NULL;
    TAILQ_FOREACH_SAFE(dsf ,&src->fields, listEntry, dsf_tmp) {
        UA_String_copy(&dsf->config.field.variable.fieldNameAlias, &dst->dataSetMetaData.fields[index].name);
        UA_PublishedVariableDataType_copy(&dsf->config.field.variable.publishParameters, &tmp->publishedData[index]);
        UA_ConfigurationVersionDataType_copy(&dsf->config.field.variable.configurationVersion, &dst->dataSetMetaData.configurationVersion);
        dst->dataSetMetaData.fields[index].fieldFlags = dsf->config.field.variable.promotedField;
        index++;
    }
    
    dst->dataSetSource.encoding = UA_EXTENSIONOBJECT_DECODED;
    dst->dataSetSource.content.decoded.data = tmp;
    dst->dataSetSource.content.decoded.type = &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE];

    return UA_STATUSCODE_GOOD;
}

/* UA_PubSubManager_generateDataSetWriterDataType() */
/**
 * @brief       Generates a DataSetWriterDataType object from a DataSetWriter.
 * @param       dst     [out]   DataSetWriterDataType
 * @param       src     [in]    DataSetWriter
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_generateDataSetWriterDataType(UA_DataSetWriterDataType *dst,
                                 const UA_DataSetWriter *src) {
    size_t index;

    memset(dst, 0, sizeof(UA_DataSetWriterDataType));

    UA_String_copy(&src->config.name, &dst->name);
    dst->dataSetWriterId = src->config.dataSetWriterId;
    dst->keyFrameCount = src->config.keyFrameCount;
    dst->dataSetFieldContentMask = src->config.dataSetFieldContentMask;
    UA_ExtensionObject_copy(&src->config.messageSettings, &dst->messageSettings);
    UA_String_copy(&src->config.dataSetName, &dst->dataSetName);

    dst->dataSetWriterPropertiesSize = src->config.dataSetWriterPropertiesSize;
    for(index = 0; index < src->config.dataSetWriterPropertiesSize; index++) {
        UA_KeyValuePair_copy(&src->config.dataSetWriterProperties[index], &dst->dataSetWriterProperties[index]);
    }

    return UA_STATUSCODE_GOOD;
}

/* UA_PubSubManager_generateWriterGroupDataType() */
/**
 * @brief       Generates a WriterGroupDataType object from a WriterGroup.
 * @param       dst     [out]   WriterGroupDataType
 * @param       src     [in]    WriterGroup
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_generateWriterGroupDataType(UA_WriterGroupDataType *dst,
                               const UA_WriterGroup *src) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    size_t index = 0;
    memset(dst, 0, sizeof(UA_WriterGroupDataType));

    UA_String_copy(&src->config.name, &dst->name);
    dst->enabled = src->config.enabled;
    dst->writerGroupId = src->config.writerGroupId;
    dst->publishingInterval = src->config.publishingInterval;
    dst->keepAliveTime = src->config.keepAliveTime;
    dst->priority = src->config.priority;
    dst->securityMode = src->config.securityMode;
    

    UA_ExtensionObject_copy(&src->config.transportSettings, &dst->transportSettings);
    UA_ExtensionObject_copy(&src->config.messageSettings, &dst->messageSettings);

    dst->groupPropertiesSize = src->config.groupPropertiesSize;
    dst->groupProperties = (UA_KeyValuePair*)UA_Array_new(dst->groupPropertiesSize, &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    for(index = 0; index < dst->groupPropertiesSize; index++) {
        UA_KeyValuePair_copy(&src->config.groupProperties[index], &dst->groupProperties[index]);
    }

    dst->dataSetWriters = (UA_DataSetWriterDataType*)UA_calloc(src->writersCount, sizeof(UA_DataSetWriterDataType));
    if(dst->dataSetWriters == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    dst->dataSetWritersSize = src->writersCount;

    UA_DataSetWriter *dsw, *dsw_tmp = NULL;
    size_t dsWriterIndex = 0;
    LIST_FOREACH_SAFE(dsw ,&src->writers, listEntry, dsw_tmp) {
        retVal = UA_PubSubManager_generateDataSetWriterDataType(&dst->dataSetWriters[dsWriterIndex], dsw);
        if(retVal != UA_STATUSCODE_GOOD) {
            break;
        }

        dsWriterIndex++;
    }

    return retVal;
}

/* UA_PubSubManager_generateDataSetReaderDataType() */
/**
 * @brief       Generates a DataSetReaderDataType object from a DataSetReader.
 * @param       dst     [out]   DataSetReaderDataType
 * @param       src     [in]    DataSetReader
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_generateDataSetReaderDataType(UA_DataSetReaderDataType *dst,
                                 UA_DataSetReader *src) {
    UA_StatusCode retVal;

    memset(dst, 0 , sizeof(UA_DataSetReaderDataType));
    retVal = UA_String_copy(&src->config.name, &dst->name);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;
    UA_Variant_copy(&src->config.publisherId, &dst->publisherId);
    dst->writerGroupId = src->config.writerGroupId;
    dst->dataSetWriterId = src->config.dataSetWriterId;
    UA_DataSetMetaDataType_copy(&src->config.dataSetMetaData, &dst->dataSetMetaData);
    dst->dataSetFieldContentMask = src->config.dataSetFieldContentMask;
    dst->messageReceiveTimeout = src->config.messageReceiveTimeout;
    UA_ExtensionObject_copy(&src->config.messageSettings, &dst->messageSettings);
    dst->subscribedDataSet.encoding = UA_EXTENSIONOBJECT_DECODED;
    dst->subscribedDataSet.content.decoded.type = &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE];
    dst->subscribedDataSet.content.decoded.data = (void*)UA_TargetVariablesDataType_new();
    UA_TargetVariablesDataType *tmpTarget = UA_TargetVariablesDataType_new();
    tmpTarget->targetVariablesSize = src->config.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize;
    tmpTarget->targetVariables = (UA_FieldTargetDataType *)UA_calloc(tmpTarget->targetVariablesSize, sizeof(UA_FieldTargetDataType));
    for(size_t index = 0; index < tmpTarget->targetVariablesSize; index++) {
        UA_FieldTargetDataType_copy(&src->config.subscribedDataSet.subscribedDataSetTarget.targetVariables[index].targetVariable, &tmpTarget->targetVariables[index]);
    }

    retVal = UA_copy(tmpTarget, 
                    dst->subscribedDataSet.content.decoded.data, 
                    &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]);

    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    return UA_STATUSCODE_GOOD;
}

/* UA_PubSubManager_generateReaderGroupDataType() */
/**
 * @brief       Generates a ReaderGroupDataType object from a ReaderGroup.
 * @param       dst     [out]   ReaderGroupDataType
 * @param       src     [in]    ReaderGroup
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_generateReaderGroupDataType(UA_ReaderGroupDataType *dst,
                               const UA_ReaderGroup *src) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    memset(dst, 0, sizeof(UA_ReaderGroupDataType));

    UA_String_copy(&src->config.name, &dst->name);
    dst->dataSetReaders = (UA_DataSetReaderDataType*)UA_calloc(src->readersCount, sizeof(UA_DataSetReaderDataType));
    if(dst->dataSetReaders == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    dst->dataSetReadersSize = src->readersCount;
    size_t dsReaderIndex = 0;
    UA_DataSetReader *dsr, *dsr_tmp = NULL;
    LIST_FOREACH_SAFE(dsr ,&src->readers, listEntry, dsr_tmp) {
        retVal = UA_PubSubManager_generateDataSetReaderDataType(&dst->dataSetReaders[dsReaderIndex], dsr);
        if(retVal != UA_STATUSCODE_GOOD) {
            break;
        }

        dsReaderIndex++;
    }

    return retVal;
}

/* UA_PubSubManager_generatePubSubConnectionDataType() */
/**
 * @brief       Generates a PubSubConnectionDataType object from a PubSubConnection.
 * @param       dst     [out]   PubSubConnectionDataType
 * @param       src     [in]    PubSubConnection
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_PubSubManager_generatePubSubConnectionDataType(UA_PubSubConnectionDataType *dst,
                                    const UA_PubSubConnection *src) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    size_t index;
    memset(dst, 0, sizeof(UA_PubSubConnectionDataType));
    
    UA_String_copy(&src->config->name, &dst->name);
    UA_String_copy(&src->config->transportProfileUri, &dst->transportProfileUri);
    dst->enabled = src->config->enabled;

    dst->connectionPropertiesSize = src->config->connectionPropertiesSize;
    for(index = 0; index < src->config->connectionPropertiesSize; index++) {
        UA_KeyValuePair_copy(&src->config->connectionProperties[index], &dst->connectionProperties[index]);
    }

    if(src->config->publisherIdType == UA_PUBSUB_PUBLISHERID_NUMERIC) {
        UA_Variant_setScalarCopy(&dst->publisherId, &src->config->publisherId.numeric, &UA_TYPES[UA_TYPES_UINT32]);
    } else if(src->config->publisherIdType == UA_PUBSUB_PUBLISHERID_STRING) {
        UA_Variant_setScalarCopy(&dst->publisherId, &src->config->publisherId.string, &UA_TYPES[UA_TYPES_STRING]);
    }

    /* Possibly, array size and dimensions of src->config->address and src->config->connectionTransportSettings 
       should be checked beforehand. */
    dst->address.encoding = UA_EXTENSIONOBJECT_DECODED;
    dst->address.content.decoded.type = src->config->address.type;
    retVal = UA_Array_copy(src->config->address.data, 1, &dst->address.content.decoded.data, src->config->address.type);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;

    if(src->config->connectionTransportSettings.data) {

        dst->transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        dst->transportSettings.content.decoded.type = src->config->connectionTransportSettings.type;
        retVal = UA_Array_copy(src->config->connectionTransportSettings.data, 
                                1, 
                                &dst->transportSettings.content.decoded.data,
                                src->config->connectionTransportSettings.type);

        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }
    }
    
    dst->writerGroups = (UA_WriterGroupDataType*)UA_calloc(src->writerGroupsSize, sizeof(UA_WriterGroupDataType));
    if(dst->writerGroups == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    dst->writerGroupsSize = src->writerGroupsSize;
    UA_WriterGroup *wg, *wg_tmp = NULL;
    size_t wgIndex = 0;
    LIST_FOREACH_SAFE(wg ,&src->writerGroups, listEntry, wg_tmp) {
        retVal = UA_PubSubManager_generateWriterGroupDataType(&dst->writerGroups[wgIndex], wg);
        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }

        wgIndex++;
    }

    dst->readerGroups = (UA_ReaderGroupDataType*)UA_calloc(src->readerGroupsSize, sizeof(UA_ReaderGroupDataType));
    if(dst->readerGroups == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    dst->readerGroupsSize = src->readerGroupsSize;
    UA_ReaderGroup *rg = NULL;
    size_t rgIndex = 0;
    LIST_FOREACH(rg, &src->readerGroups, listEntry) {
        retVal = UA_PubSubManager_generateReaderGroupDataType(&dst->readerGroups[rgIndex], rg);
        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }

        rgIndex++;
    }

    return UA_STATUSCODE_GOOD;
}

/* UA_PubSubManager_generatePubSubConfigurationDataType() */
/**
 * @brief       Generates a PubSubConfigurationDataType object from the current server configuration.
 * 
 * @param       server                  [in]    server, that contains the PubSub configuration
 * @param       pubSubConfiguration     [out]   target object
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_PubSubManager_generatePubSubConfigurationDataType(const UA_Server* server,
                                       UA_PubSubConfigurationDataType *pubSubConfiguration) {
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    UA_PubSubManager manager = server->pubSubManager;
    memset(pubSubConfiguration, 0, sizeof(UA_PubSubConfigurationDataType));

    pubSubConfiguration->publishedDataSets = (UA_PublishedDataSetDataType*)UA_calloc(manager.publishedDataSetsSize, 
                                                                                     sizeof(UA_PublishedDataSetDataType));
    if(pubSubConfiguration->publishedDataSets == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    pubSubConfiguration->publishedDataSetsSize = manager.publishedDataSetsSize;
    
    UA_PublishedDataSet *pds;
    UA_UInt32 pdsIndex = 0;
    TAILQ_FOREACH(pds, &manager.publishedDataSets, listEntry) {
        statusCode = UA_PubSubManager_generatePublishedDataSetDataType(&pubSubConfiguration->publishedDataSets[pdsIndex],
                                                         pds);
        
        pdsIndex++;
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                "[UA_PubSubManager_generatePubSubConfigurationDataType] retrieving PublishedDataSet configuration failed");
            return statusCode;
        }
    }

    pubSubConfiguration->connections = (UA_PubSubConnectionDataType*)UA_calloc(manager.connectionsSize, 
                                                                               sizeof(UA_PubSubConnectionDataType));
    if(pubSubConfiguration->connections == NULL) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    pubSubConfiguration->connectionsSize = manager.connectionsSize;
    UA_UInt32 connectionIndex = 0;
    UA_PubSubConnection *connection;
    TAILQ_FOREACH(connection, &manager.connections, listEntry) {
        statusCode = UA_PubSubManager_generatePubSubConnectionDataType(&pubSubConfiguration->connections[connectionIndex], 
                                                         connection);
        connectionIndex++;                                                         
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                "[UA_PubSubManager_generatePubSubConfigurationDataType] retrieving PubSubConnection configuration failed");
            return statusCode;
        }
    }

    return UA_STATUSCODE_GOOD;
}

/* UA_PubSubManager_getEncodedPubSubConfiguration() */
/**
 * @brief       Saves the current PubSub configuration of a server in a byteString.
 * 
 * @param       server  [in]    Pointer to server object, that contains the PubSubConfiguration
 * @param       buffer  [out]    Pointer to a byteString object
 *
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_PubSubManager_getEncodedPubSubConfiguration(UA_Server *server, UA_ByteString *buffer) {
    UA_StatusCode statusCode;
    UA_PubSubConfigurationDataType config;
    memset(&config, 0, sizeof(UA_PubSubConfigurationDataType));

    statusCode = UA_PubSubManager_generatePubSubConfigurationDataType(server, &config);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "retrieving PubSub configuration from server failed");
        goto cleanup;
    }

    statusCode = UA_PubSubManager_encodePubSubConfiguration(&config, buffer);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "encoding PubSub configuration failed");
        goto cleanup;
    }

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Saving PubSub config was successful");
    
cleanup:
    UA_PubSubConfigurationDataType_clear(&config);

    return statusCode;
}

#endif /* UA_ENABLE_PUBSUB_FILE_CONFIG */
