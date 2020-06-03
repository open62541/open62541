#include <open62541/types_generated_encoding_binary.h>
#include <open62541/plugin/log_stdout.h>
#include "server/ua_server_internal.h"

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#ifdef UA_ENABLE_PUBSUB_CONFIG

#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/pubsub_ethernet.h>
#include "pubsub/ua_pubsub.h"
#include "pubsub/ua_pubsub_manager.h"
#include "pubsub/ua_pubsub_config.h"

/* Function prototypes: */

static UA_StatusCode 
UA_createPubSubConnection(UA_Server *server, const UA_PubSubConnectionDataType *const connectionParameters, 
                          const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent);

static UA_StatusCode 
UA_createWriterGroup(UA_Server *server, const UA_WriterGroupDataType *const writerGroupParameters, 
                     const UA_NodeId connectionIdent, const UA_UInt32 pdsCount, 
                     const UA_NodeId *pdsIdent);

static UA_StatusCode 
UA_createDataSetWriter(UA_Server *server, const UA_DataSetWriterDataType *const dataSetWriterParameters, 
                       const UA_NodeId writerGroupIdent, const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent);

static UA_StatusCode
UA_createReaderGroup(UA_Server *server, const UA_ReaderGroupDataType *const readerGroupParameters, 
                     const UA_NodeId connectionIdent);

static UA_StatusCode
UA_createDataSetReader(UA_Server *server, const UA_DataSetReaderDataType *const dataSetReaderParameters, 
                       const UA_NodeId readerGroupIdent);

static UA_StatusCode 
UA_createPublishedDataSet(UA_Server *server, const UA_PublishedDataSetDataType *const publishedDataSetParameters, 
                          UA_NodeId *publishedDataSetIdent);

static UA_StatusCode 
UA_createDataSetFields(UA_Server *server, const UA_NodeId *const publishedDataSetIdent, 
                       const UA_PublishedDataSetDataType *const publishedDataSetParameters);

static UA_StatusCode
UA_generatePubSubConfigurationDataType(const UA_Server *server,
                                       UA_PubSubConfigurationDataType *pubSubConfiguration);

/* Function implementations: */

/* createCharArrayFromString() */
/**
 * @brief   Allocates a null-terminated char-array, which contains the the data of a UA_String.
 * @param   str     [in]    UA_String which shall be transformed into char-array
 * @return  on success: pointer to resulting char-array
 *          on failure: NULL
 */
static char* 
createCharArrayFromString(const UA_String str)
{
    if(str.length == 0)
        return NULL;
    char *array = (char *)UA_calloc(str.length + 1, sizeof(char));
    if(array != NULL) {
        memcpy(array, str.data, str.length);
        array[str.length] = '\0';
    }
    return array;
}

/* UA_readFileToBuffer() */
/**
 *  @brief      Writes the content of a file (relative location in the filesystem) into a buffer
 * 
 *  @param      filename    [in]    Name of the file to buffer
 *  @param      buffer      [bi]    Object which shall buffer the file
 * 
 *  @return     UA_StatusCode
 */
static UA_StatusCode 
UA_readFileToBuffer(const UA_String filename, UA_ByteString *buffer)
{
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    FILE *filePointer = NULL;
    char *fileNameChar = NULL;
    
    do{
        if(buffer == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_readFileToBuffer] invalid argument");
            statusCode = UA_STATUSCODE_BADINVALIDARGUMENT; 
            break;
        }
        *buffer = UA_BYTESTRING_NULL;

        /* UA_String is a string without a terminating null-character. 
         * However, this terminating character is needed by fopen. */
        fileNameChar = createCharArrayFromString(filename);
        if(fileNameChar == NULL) {
            statusCode = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }

        filePointer = fopen(fileNameChar, "rb");
        if(filePointer == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                         "[UA_readFileToBuffer] Opening file \"%s\" failed (file does not exist)", fileNameChar);
            statusCode = UA_STATUSCODE_BADINTERNALERROR; 
            break;
        }

        /* Determine file size */
        int ret = fseek(filePointer, 0, SEEK_END);
        if(ret < 0) {
            statusCode = UA_STATUSCODE_BADINTERNALERROR; 
            break;
        }
        long int fileSize = ftell(filePointer);
        if(fileSize < 0) {
            statusCode = UA_STATUSCODE_BADINTERNALERROR; 
            break;
        }
        ret = fseek(filePointer, 0, SEEK_SET);
        if(ret < 0) {
            statusCode = UA_STATUSCODE_BADINTERNALERROR; 
            break;
        }

        buffer->data = (UA_Byte*)UA_calloc((size_t)fileSize, sizeof(UA_Byte));
        if(buffer->data == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_readFileToBuffer] Allocation of buffer failed!");
            statusCode = UA_STATUSCODE_BADOUTOFMEMORY; 
            break;
        }
        buffer->length = (size_t)fileSize;

        size_t bytes = fread(buffer->data, sizeof(UA_Byte), buffer->length, filePointer);
        if(bytes == 0) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_readFileToBuffer] Reading file %s into buffer failed!", fileNameChar);
            free(buffer->data);
            *buffer = UA_STRING_NULL;
            statusCode = UA_STATUSCODE_BADINTERNALERROR; 
            break;
        }
    } while(0);

    if(filePointer != NULL) {
        fclose(filePointer);
    }
    if(fileNameChar != NULL) {
        free(fileNameChar);
        fileNameChar = NULL;
    }

    return statusCode;
}

/* UA_decodeBinFile() */
/**
 *  @brief      Decodes buffered binary config file
 * 
 *  @param      buffer      [in]    Object that buffers the bin file
 *  @param      offset      [bi]    Offset in the buffer
 *  @param      dst         [out]   Object that shall contain the decoded data
 * 
 *  @return     UA_StatusCode
 */
UA_StatusCode 
UA_decodeBinFile(const UA_ByteString *buffer, size_t offset, UA_ExtensionObject *dst)
{
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    if(buffer == NULL || dst == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_decodeBinFile] invalid argument");
        statusCode = UA_STATUSCODE_BADINVALIDARGUMENT;
    } else {
        statusCode = UA_ExtensionObject_decodeBinary(buffer, &offset, dst);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_decodeBinFile] decoding UA_Binary failed");
        }
    }
    return statusCode;
}

/* UA_getPubSubConfig() */
/**
 *  @brief      gets PubSub Configuration from decoded Extension Object
 * 
 *  @param      src         [in]    Decoded source object of type ExtensionObject
 *  @param      dst         [out]   Pointer on PubSub Configuration
 * 
 *  @return     UA_StatusCode
 */
UA_StatusCode 
UA_getPubSubConfig(const UA_ExtensionObject *const src, UA_PubSubConfigurationDataType **dst)
{
    if(src == NULL || dst == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    *dst = NULL;

    UA_UABinaryFileDataType *binFile;
    if((src->encoding == UA_EXTENSIONOBJECT_DECODED) && 
       UA_NodeId_equal(&src->content.decoded.type->typeId, 
                       &UA_TYPES[UA_TYPES_UABINARYFILEDATATYPE].typeId)) {
        binFile = (UA_UABinaryFileDataType*)src->content.decoded.data;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_getPubSubConfig] Reading extensionObject failed");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(binFile->body.arrayLength != 0 || binFile->body.arrayDimensionsSize != 0) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_getPubSubConfig] Loading multiple configurations is not supported");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    if(UA_NodeId_equal(&binFile->body.type->typeId, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONDATATYPE].typeId)) {
        *dst = (UA_PubSubConfigurationDataType*)binFile->body.data;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_getPubSubConfig] Invalid datatype encoded in the binary file");
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    return UA_STATUSCODE_GOOD;
}

/* UA_updatePubSubConfig() */
/**
 *  @brief      Configures a PubSub Server with given PubSubConfigurationDataType object
 * 
 *  @param      server                  [bi]    Server object
 *  @param      configurationParameters [in]    PubSub Configuration parameters
 * 
 *  @return     UA_StatusCode
 */
UA_StatusCode 
UA_updatePubSubConfig(UA_Server* server, const UA_PubSubConfigurationDataType *const configurationParameters)
{
    if(server == NULL || configurationParameters == NULL) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_updatePubSubConfig] Invalid argument");
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
        statusCode = UA_createPublishedDataSet(server, &configurationParameters->publishedDataSets[pdsIndex], 
                                               &publishedDataSetIdent[pdsIndex]);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_updatePubSubConfig] PDS creation failed");
            free(publishedDataSetIdent);
            return statusCode;
        }
    }

    /* Configuration of PubSub Connections: */
    if(configurationParameters->connectionsSize < 1) {
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                       "[UA_updatePubSubConfig] no connection in UA_PubSubConfigurationDataType");
        free(publishedDataSetIdent);
        return UA_STATUSCODE_GOOD;
    }

    for(UA_UInt32 conIndex = 0; 
        conIndex < configurationParameters->connectionsSize && statusCode == UA_STATUSCODE_GOOD; 
        conIndex++) 
    {
        statusCode = UA_createPubSubConnection(server, &configurationParameters->connections[conIndex], 
                                               pdsCount, publishedDataSetIdent);
    }

    free(publishedDataSetIdent);
    return statusCode;
}

/* UA_setConnectionPublisherId() */
/**
 * @brief       Function called by UA_createPubSubConnection to set the PublisherId of a certain connection.
 * 
 * @param       src     [in]    PubSubConnection parameters
 * @param       dst     [out]   PubSubConfiguration
 * 
 * @return      UA_StatusCode
 */
static UA_StatusCode
UA_setConnectionPublisherId(const UA_PubSubConnectionDataType *src, UA_PubSubConnectionConfig *dst)
{
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
                    "[UA_setConnectionPublisherId] PublisherId is UInt64 (not implemented); Recommended dataType for PublisherId: UInt32");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_setConnectionPublisherId] PublisherId is not valid.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

/* UA_createComponentsForConnection() */
/**
 * @brief   Function called by UA_createPubSubConnection to create all WriterGroups and ReaderGroups
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
UA_createComponentsForConnection(UA_Server *server, const UA_PubSubConnectionDataType *const connectionParameters, 
                                 UA_NodeId connectionIdent, 
                                 const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    /* WriterGroups configuration */
    for(UA_UInt32 wGroupIndex=0; 
        wGroupIndex < connectionParameters->writerGroupsSize; 
        wGroupIndex++)
    {
        retVal = UA_createWriterGroup(server, &connectionParameters->writerGroups[wGroupIndex], 
                                      connectionIdent, pdsCount, pdsIdent);
        if(retVal == UA_STATUSCODE_GOOD) {
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                         "[UA_createComponentsForConnection] Error occured during %d.WriterGroup Creation", wGroupIndex+1);
            return retVal;
        }
    }

    /* ReaderGroups configuration */
    for(UA_UInt32 rGroupIndex=0; 
        rGroupIndex < connectionParameters->readerGroupsSize; 
        rGroupIndex++) 
    {
        retVal = UA_createReaderGroup(server, &connectionParameters->readerGroups[rGroupIndex], connectionIdent);
        if(retVal == UA_STATUSCODE_GOOD) {

            retVal |= UA_PubSubConnection_regist(server, &connectionIdent);
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_createComponentsForConnection] Error occured during %d.ReaderGroup Creation", rGroupIndex+1);
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
UA_transportLayerExists(UA_Server *server, UA_String transportProfileUri)
{
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
UA_createTransportLayer(UA_Server *server, const UA_String transportProfileUri)
{
    if(UA_transportLayerExists(server, transportProfileUri)) {
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

        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_createTransportLayer] invalid transportProfileUri");
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

/* UA_createPubSubConnection() */
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
UA_createPubSubConnection(UA_Server *server, const UA_PubSubConnectionDataType *const connectionParameters, 
                          const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent)
{
    UA_PubSubConnectionConfig config;
    memset(&config, 0, sizeof(UA_PubSubConnectionConfig));

    config.name =                       connectionParameters->name;
    config.enabled =                    connectionParameters->enabled;
    config.transportProfileUri =        connectionParameters->transportProfileUri;
    config.connectionPropertiesSize =   connectionParameters->connectionPropertiesSize;
    if(config.connectionPropertiesSize > 0) {
        config.connectionProperties = connectionParameters->connectionProperties;
    }

    UA_StatusCode statusCode = UA_setConnectionPublisherId(connectionParameters, &config);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_createPubSubConnection] Setting PublisherId failed");
        return statusCode;
    }

    if(connectionParameters->address.encoding == UA_EXTENSIONOBJECT_DECODED) {
        UA_Variant_setScalar(&(config.address), 
                             connectionParameters->address.content.decoded.data, 
                             connectionParameters->address.content.decoded.type);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_createPubSubConnection] Reading connection address failed");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(connectionParameters->transportSettings.encoding == UA_EXTENSIONOBJECT_DECODED) {
        UA_Variant_setScalar(&(config.connectionTransportSettings), 
                             connectionParameters->transportSettings.content.decoded.data, 
                             connectionParameters->transportSettings.content.decoded.type);
    } else { 
        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                       "[UA_createPubSubConnection] TransportSettings can not be read");
    }

    statusCode = UA_createTransportLayer(server, connectionParameters->transportProfileUri);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_createPubSubConnection] Creating transportLayer failed");
        return statusCode;
    }

    /* Load connection config into server: */
    UA_NodeId connectionIdent;
    statusCode = UA_Server_addPubSubConnection(server, &config, &connectionIdent);
    if(statusCode == UA_STATUSCODE_GOOD) {
        /* Configuration of all Components that belong to this connection: */
        statusCode = UA_createComponentsForConnection(server, connectionParameters, connectionIdent, pdsCount, pdsIdent);
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_createPubSubConnection] Connection creation failed");
    }

    return statusCode;
}

/* UA_setWriterGroupEncodingType() */
/**
 * @brief   Function called by UA_createWriterGroup to configure the messageSettings of a writerGroup.
 * 
 * @param   writerGroupParameters   [in]
 * @param   config                  [bi]
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode 
UA_setWriterGroupEncodingType(const UA_WriterGroupDataType *writerGroupParameters, 
                              UA_WriterGroupConfig *config)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    if(writerGroupParameters->messageSettings.encoding == UA_EXTENSIONOBJECT_DECODED) {
        if(UA_NodeId_equal(&writerGroupParameters->messageSettings.content.decoded.type->typeId, 
                           &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE].typeId)) {
            config->encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        } else if(writerGroupParameters->messageSettings.content.decoded.type->typeId.identifier.numeric == 
                  UA_NS0ID_JSONWRITERGROUPMESSAGEDATATYPE) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_setWriterGroupEncodingType] encoding type: JSON (not implemented!)");
            retVal = UA_STATUSCODE_BADNOTIMPLEMENTED;
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_setWriterGroupEncodingType] invalid message encoding type");
            retVal = UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_setWriterGroupEncodingType] getting message type information failed");
        retVal = UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return retVal;
}

/* UA_createWriterGroup() */
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
UA_createWriterGroup(UA_Server *server, const UA_WriterGroupDataType *const writerGroupParameters, 
                     const UA_NodeId connectionIdent,
                     const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent)
{
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    UA_WriterGroupConfig config;
    
    memset(&config, 0, sizeof(UA_WriterGroupConfig));

    /* TODO: create client structure */
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

    statusCode = UA_setWriterGroupEncodingType(writerGroupParameters, &config);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_createWriterGroup] Setting message settings failed");
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
            statusCode = UA_createDataSetWriter(server, &writerGroupParameters->dataSetWriters[dsWriterIndex],
                                                     writerGroupIdent, pdsCount, pdsIdent);
            if(statusCode != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                            "[UA_createWriterGroup] DataSetWriter Creation failed.");
            }
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_createWriterGroup] Adding WriterGroup to server failed: 0x%x", statusCode);
    }

    return UA_STATUSCODE_GOOD;
}

/* UA_addDataSetWriterWithPdsReference() */
/**
 * @brief   Function called by UA_createDataSetWriter. It searches for a PublishedDataSet that is referenced by
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
UA_addDataSetWriterWithPdsReference(UA_Server *server, const UA_NodeId writerGroupIdent, 
                                    const UA_DataSetWriterConfig *dsWriterConfig,
                                    const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent)
{
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
                                 "[UA_addDataSetWriterWithPdsReference] Adding DataSetWriter failed");
                } else {
                    pdsFound = UA_TRUE;
                }
            }

            UA_PublishedDataSetConfig_clear(&pdsConfig);
            if(pdsFound)
                break; /* break loop if corresponding publishedDataSet was found */
        } else {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_addDataSetWriterWithPdsReference] Getting pdsConfig from NodeId failed.");
        }
    }

    if(!pdsFound) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_addDataSetWriterWithPdsReference] No matching DataSet found; no DataSetWriter created");
    }

    return retVal;
}

/* UA_createDataSetWriter() */
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
UA_createDataSetWriter(UA_Server *server, const UA_DataSetWriterDataType *const dataSetWriterParameters, 
                       const UA_NodeId writerGroupIdent, const UA_UInt32 pdsCount, const UA_NodeId *pdsIdent)
{
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

    UA_StatusCode statusCode = UA_addDataSetWriterWithPdsReference(server, writerGroupIdent, 
                                                                   &config, pdsCount, pdsIdent);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_createDataSetWriter] Referencing related PDS failed");
    }
    
    return statusCode;
}

/* UA_createReaderGroup() */
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
UA_createReaderGroup(UA_Server *server, const UA_ReaderGroupDataType *const readerGroupParameters, 
                     const UA_NodeId connectionIdent)
{    
    UA_ReaderGroupConfig config;
    
    memset(&config, 0, sizeof(UA_ReaderGroupConfig));

    config.name                                 = readerGroupParameters->name;
    config.securityParameters.securityMode      = readerGroupParameters->securityMode;

    UA_NodeId readerGroupIdent;

    UA_StatusCode statusCode = UA_Server_addReaderGroup(server, connectionIdent, &config, &readerGroupIdent);
    if(statusCode == UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_createReaderGroup] ReaderGroup successfully added.");
        for(UA_UInt32 dsReaderIndex=0; 
            dsReaderIndex < readerGroupParameters->dataSetReadersSize && statusCode == UA_STATUSCODE_GOOD; 
            dsReaderIndex++)
        {
            statusCode = UA_createDataSetReader(server, &readerGroupParameters->dataSetReaders[dsReaderIndex], readerGroupIdent);
            if(statusCode != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                             "[UA_createReaderGroup] Creating DataSetReader failed");
            }
        }
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_createReaderGroup] Adding ReaderGroup to server failed: 0x%x", statusCode);
    }

    UA_Server_setReaderGroupOperational(server, readerGroupIdent);
    return statusCode;
}

/* UA_setDataSetReaderTargetVariables() */
/**
 * @brief   Sets TargetVariables in DataSetSReaderConfig if necessary
 * 
 * @param   dataSetReaderParameters [in]    Information for DataSetReader configuration
 * @param   config                  [bi]    DataSetReader configuration object
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_setDataSetReaderTargetVariables(const UA_DataSetReaderDataType *dataSetReaderParameters, 
                                   UA_DataSetReaderConfig *config)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    do {
        if(dataSetReaderParameters->subscribedDataSet.encoding != UA_EXTENSIONOBJECT_DECODED) {
            retVal = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }

        if(UA_NodeId_equal(&dataSetReaderParameters->subscribedDataSet.content.decoded.type->typeId, 
                           &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE].typeId)) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_setDataSetReaderTargetVariables] Setting TargetVariables");
            config->subscribedDataSetTarget = *(UA_TargetVariablesDataType*)dataSetReaderParameters->
                                                                            subscribedDataSet.content.decoded.data;
            if(config->dataSetMetaData.fieldsSize != config->subscribedDataSetTarget.targetVariablesSize) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                             "[UA_setDataSetReaderTargetVariables] ERROR: invalid configuration");
                retVal = UA_STATUSCODE_BADINTERNALERROR;
            }

            break;
        } 

        if(UA_NodeId_equal(&dataSetReaderParameters->subscribedDataSet.content.decoded.type->typeId,
                           &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE].typeId)) {
            /* Nothing to do here */
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_setDataSetReaderTargetVariables] Type is SubscribedDataSetMirrorDataType");
            break;
        } 

        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_setDataSetReaderTargetVariables] creating subscribedDataSet failed");
        retVal = UA_STATUSCODE_BADINTERNALERROR;

    } while(0);

    return retVal;
}

/* UA_addSubscribedDataSetMirrolrObject() */
/**
 * @brief   Adds object node that shall contain the variables of a SubscribedDataSetMirror
 *          This method will not be needed anymore, as soon as DataSetMirror is supported by the Stack.
 * 
 * @param   server      [bi]    UA_Server object that shall be configured
 * @param   objectName  [in]    Name of the object
 * 
 * @return  on success: NodeId of the created object
 *          on failure: UA_NODEID_NULL;
 */
static UA_NodeId 
UA_addSubscribedDataSetMirrorObject(UA_Server *server, const UA_String objectName)
{
    UA_NodeId subscribedDataSetObjectId = UA_NODEID_STRING(0, "PubSubObject");;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName.locale = UA_STRING("en-US");
    oAttr.displayName.text = objectName; /*Necessary to copy string? -> Test with Valgrind */
    if(UA_Server_addObjectNode(server,UA_NODEID_NULL,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), /* TODO: this should be the NodeId of the
                                                                             corresponding DataSetReader (currently,
                                                                             there exists no representation for DSReader) */
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(0, "SucribedDataSetMirror"), UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            oAttr, NULL, &subscribedDataSetObjectId) != UA_STATUSCODE_GOOD) {
        return UA_NODEID_NULL;
    }

    return subscribedDataSetObjectId;
}

/* UA_addSubscribedDataSet() */
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
UA_addSubscribedDataSet(UA_Server *server, const UA_NodeId dsReaderIdent, 
                        const UA_ExtensionObject *const subscribedDataSet)
{
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;

    if(UA_NodeId_equal(&subscribedDataSet->content.decoded.type->typeId, 
                        &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE].typeId)) {
        UA_TargetVariablesDataType *targetVars = 
            (UA_TargetVariablesDataType*)subscribedDataSet->content.decoded.data;
        retVal = UA_Server_DataSetReader_createTargetVariables(server, dsReaderIdent, targetVars);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_addSubscribedDataSet] create TargetVariables failed");
        }

        return retVal;
    } 

    /* The following operations for creating a SubscribedDataSetMirror is only a provisional solution, as it is 
       currently not possible to create this type of SubscribedDataSet in the dsReader.
       As a provisional implementation, an object will be created under the Objects folder.
       Inside this object, TargetVariables will be created according to the DataSetMetaData of the dsReader. */
    if(UA_NodeId_equal(&subscribedDataSet->content.decoded.type->typeId,
                        &UA_TYPES[UA_TYPES_SUBSCRIBEDDATASETMIRRORDATATYPE].typeId)) {
                            
        UA_SubscribedDataSetMirrorDataType *dsMirror =
            (UA_SubscribedDataSetMirrorDataType*)subscribedDataSet->content.decoded.data;

        UA_NodeId subscribedDataSetObjectId = UA_addSubscribedDataSetMirrorObject(server, dsMirror->parentNodeName);
        if(UA_NodeId_equal(&subscribedDataSetObjectId, &UA_NODEID_NULL)){
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        retVal = UA_Server_DataSetReader_addTargetVariables(server, &subscribedDataSetObjectId, 
                                                            dsReaderIdent, UA_PUBSUB_SDS_TARGET);
        if(retVal != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_addSubscribedDataSet] create DataSetMirror failed");
        }
        
        return retVal;
    } 

    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_addSubscribedDataSet] Invalid Type of SubscribedDataSet");
    return UA_STATUSCODE_BADINTERNALERROR;
}

/* UA_createDataSetReader() */
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
UA_createDataSetReader(UA_Server *server, const UA_DataSetReaderDataType *const dataSetReaderParameters, 
                       const UA_NodeId readerGroupIdent)
{
    UA_DataSetReaderConfig config;
    memset(&config, 0, sizeof(UA_DataSetReaderConfig));

    config.name =                   dataSetReaderParameters->name;
    UA_Variant_copy(&dataSetReaderParameters->publisherId, &config.publisherId);
    config.writerGroupId =          dataSetReaderParameters->writerGroupId;
    config.dataSetWriterId =        dataSetReaderParameters->dataSetWriterId;
    UA_DataSetMetaDataType_copy(&dataSetReaderParameters->dataSetMetaData, &config.dataSetMetaData);
    config.dataSetFieldContentMask = dataSetReaderParameters->dataSetFieldContentMask;
    config.messageReceiveTimeout =  dataSetReaderParameters->messageReceiveTimeout;
    config.messageSettings = dataSetReaderParameters->messageSettings;

    /* The DataSetReader contains information about TargetVariables twice:
       directly and in the DataSetReaderConfig. 
       The following operation sets the TargetVariables in the DSRaderConfig. */
    UA_StatusCode statusCode = UA_setDataSetReaderTargetVariables(dataSetReaderParameters, &config);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_createDataSetReader] Configuring SubscribedDataSet failed");
        return statusCode;
    }

    UA_NodeId dsReaderIdent;
    statusCode = UA_Server_addDataSetReader (server, readerGroupIdent, &config, &dsReaderIdent);
    if(statusCode == UA_STATUSCODE_GOOD) {
        statusCode = UA_addSubscribedDataSet(server, dsReaderIdent, &dataSetReaderParameters->subscribedDataSet);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_createDataSetReader] create subscribedDataSet failed");
        }
    }

    return statusCode;
}

/* UA_setPublishedDataSetType() */
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
UA_setPublishedDataSetType(const UA_PublishedDataSetDataType *const publishedDataSetParameters, 
                           UA_PublishedDataSetConfig *config)
{
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
                     "[UA_setPublishedDataSetType] Published events not supported.");
        retVal = UA_STATUSCODE_BADNOTIMPLEMENTED;
    } else {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_setPublishedDataSetType] Invalid DataSetSourceDataType.");
        retVal = UA_STATUSCODE_BADINTERNALERROR;
    }

    return retVal;
}

/* UA_createPublishedDataSet() */
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
UA_createPublishedDataSet(UA_Server *server, const UA_PublishedDataSetDataType *const publishedDataSetParameters, 
                          UA_NodeId *publishedDataSetIdent)
{
    UA_PublishedDataSetConfig config;
    memset(&config, 0, sizeof(UA_PublishedDataSetConfig));

    config.name = publishedDataSetParameters->name;
    UA_StatusCode statusCode = UA_setPublishedDataSetType(publishedDataSetParameters, &config);
    if(statusCode != UA_STATUSCODE_GOOD) {
        return statusCode;
    }

    statusCode = UA_Server_addPublishedDataSet(server, &config, publishedDataSetIdent).addResult;
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "[UA_createPublishedDataSet] Adding PublishedDataSet failed.");
        return statusCode;
    }

    /* DataSetField configuration for this publishedDataSet: */
    statusCode = UA_createDataSetFields(server, publishedDataSetIdent, publishedDataSetParameters);
    if(statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                    "[UA_createPublishedDataSet] Creating DataSetFieldConfig failed.");
    }

    return statusCode;
}

/* UA_addDataSetFieldVariables */
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
UA_addDataSetFieldVariables(UA_Server *server, const UA_NodeId *publishedDataSetIdent,
                            const UA_PublishedDataSetDataType *const publishedDataSetParameters)
{
    UA_PublishedDataItemsDataType *publishedDataItems = 
        (UA_PublishedDataItemsDataType *)publishedDataSetParameters->dataSetSource.content.decoded.data;
    if(publishedDataItems->publishedDataSize != publishedDataSetParameters->dataSetMetaData.fieldsSize){
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    /* Last in first out configuration: */
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
                        "[UA_addDataSetFieldVariables] Adding DataSetField Variable failed.");
        }
    }   

    return UA_STATUSCODE_GOOD;
}

/* UA_createDataSetFields() */
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
UA_createDataSetFields(UA_Server *server, const UA_NodeId *const publishedDataSetIdent, 
                       const UA_PublishedDataSetDataType *const publishedDataSetParameters)
{
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    do {
        if(publishedDataSetParameters->dataSetSource.encoding != UA_EXTENSIONOBJECT_DECODED) {
            statusCode = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }

        if(UA_NodeId_equal(&publishedDataSetParameters->dataSetSource.content.decoded.type->typeId, 
                           &UA_TYPES[UA_TYPES_PUBLISHEDDATAITEMSDATATYPE].typeId)) { 
            statusCode = UA_addDataSetFieldVariables(server, publishedDataSetIdent, publishedDataSetParameters);
            break;
        } 

        if(publishedDataSetParameters->dataSetSource.content.decoded.type->typeId.identifier.numeric == 
           UA_NS0ID_PUBLISHEDEVENTSDATATYPE) {
            /* This is a placeholder; TODO: Implement Routine for adding Event DataSetFields */
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                         "[UA_createDataSetFields] Published events not supported.");
            statusCode = UA_STATUSCODE_BADNOTIMPLEMENTED;
            break;
        } 

        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_createDataSetFields] Invalid DataSetSourceDataType.");
        statusCode = UA_STATUSCODE_BADINTERNALERROR;
    } while(0);
    
    return statusCode;
}

/* UA_loadPubSubConfigFromFile() */
/**
 * @brief       Opens file, decodes the information and loads the PubSub configuration into server.
 * 
 * @param       server      [bi]    Server object that shall be configured
 * @param       filename    [in]    Relative path and name of the file that contains the PubSub configuration
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode UA_loadPubSubConfigFromFile(UA_Server *server, const UA_String filename)
{
    UA_ByteString buffer = UA_BYTESTRING_NULL;
    UA_ExtensionObject decodedBinFile;
    memset(&decodedBinFile, 0, sizeof(UA_ExtensionObject)); /* Prevents valgrind errors in case of invalid filename */
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;

    do {
        statusCode = UA_readFileToBuffer(filename, &buffer);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_loadPubSubConfigFromFile] Buffering file failed");
            break;
        }

        statusCode = UA_decodeBinFile(&buffer, 0, &decodedBinFile);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_loadPubSubConfigFromFile] Decoding failed");
            break;
        }

        UA_PubSubConfigurationDataType *pubSubConfig;
        statusCode = UA_getPubSubConfig(&decodedBinFile, &pubSubConfig);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_loadPubSubConfigFromFile] Extracting PubSub Configuration failed");
            break;
        }

        statusCode = UA_updatePubSubConfig(server, pubSubConfig);
        if(statusCode != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                        "[UA_loadPubSubConfigFromFile] Loading PubSub configuration into server failed");
            break;
        }
    } while(0);

    free(buffer.data);
    UA_ExtensionObject_deleteMembers(&decodedBinFile);
    return statusCode;
}

/* UA_writeBufferToFile() */
/**
 * @brief   Writes a buffer (ByteString) to a file in the filesystem.
 * 
 * @param   filename    [in]    Name of the file that shall be written.
 * @param   buffer      [in]    ByteString that contains the data that shall be written.
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_writeBufferToFile(const UA_String filename, const UA_ByteString buffer)
{
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    FILE *filePointer = NULL;
    char *fileNameChar = NULL;

    do {
        fileNameChar = createCharArrayFromString(filename);
        if(fileNameChar == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_writeBufferToFile] transforming string failed");
            statusCode = UA_STATUSCODE_BADINTERNALERROR;
            break;
        }

        filePointer = fopen(fileNameChar, "wb");
        if(filePointer == NULL) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_writeBufferToFile] Opening file %s failed", fileNameChar);
            statusCode = UA_STATUSCODE_BADINTERNALERROR; 
            break;
        }

        for(UA_UInt32 bufIndex = 0; bufIndex < buffer.length; bufIndex++) {
            int retVal = fputc(buffer.data[bufIndex], filePointer);
            if(retVal == EOF) {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_writeBufferToFile] writing into file failed");
                statusCode = UA_STATUSCODE_BADINTERNALERROR;
                break;
            }
        }
    } while(0);

    if(filePointer != NULL) {
        fclose(filePointer);
    }

    if(fileNameChar != NULL) {
        free(fileNameChar);
        fileNameChar = NULL;
    }

    return statusCode;
}

/* UA_encodePubSubConfiguration() */
/**
 * @brief       Encodes a PubSubConfigurationDataType object as ByteString using the UA Binary Data Encoding.
 * @param       configurationParameters     [in]
 * @param       buffer                      [out]
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_encodePubSubConfiguration(UA_PubSubConfigurationDataType *configurationParameters,
                             UA_ByteString *buffer)
{
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
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_encodePubSubConfiguration] Allocating buffer failed");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    buffer->length = fileSize;

    UA_Byte *bufferPos = buffer->data;
    retVal = UA_ExtensionObject_encodeBinary(&container, &bufferPos, bufferPos + fileSize);
    if(retVal != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "[UA_encodePubSubConfiguration] Encoding failed");
    }

    return retVal;
}

/* UA_generatePublishedDataSetDataType() */
/**
 * @brief       Generates a PublishedDataSetDataType object from a PublishedDataSet.
 * @param       dst     [out]   PublishedDataSetDataType
 * @param       src     [in]    PublishedDataSet
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_generatePublishedDataSetDataType(UA_PublishedDataSetDataType *dst,
                                    const UA_PublishedDataSet *src)
{
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
        /* todo err; */
    }

    dst->dataSetMetaData.fields = (UA_FieldMetaData*)UA_Array_new(dst->dataSetMetaData.fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);
    if(dst->dataSetMetaData.fields == NULL) {
        /* todo err; */
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

/* UA_generateDataSetWriterDataType() */
/**
 * @brief       Generates a DataSetWriterDataType object from a DataSetWriter.
 * @param       dst     [out]   DataSetWriterDataType
 * @param       src     [in]    DataSetWriter
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_generateDataSetWriterDataType(UA_DataSetWriterDataType *dst,
                                 const UA_DataSetWriter *src)
{
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

/* UA_generateWriterGroupDataType() */
/**
 * @brief       Generates a WriterGroupDataType object from a WriterGroup.
 * @param       dst     [out]   WriterGroupDataType
 * @param       src     [in]    WriterGroup
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_generateWriterGroupDataType(UA_WriterGroupDataType *dst,
                               const UA_WriterGroup *src)
{
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
        retVal = UA_generateDataSetWriterDataType(&dst->dataSetWriters[dsWriterIndex], dsw);
        if(retVal != UA_STATUSCODE_GOOD) {
            break;
        }

        dsWriterIndex++;
    }

    return retVal;
}

/* UA_generateDataSetReaderDataType() */
/**
 * @brief       Generates a DataSetReaderDataType object from a DataSetReader.
 * @param       dst     [out]   DataSetReaderDataType
 * @param       src     [in]    DataSetReader
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_generateDataSetReaderDataType(UA_DataSetReaderDataType *dst,
                                 UA_DataSetReader *src)
{
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
    retVal = UA_copy(&src->subscribedDataSetTarget, 
                    &dst->subscribedDataSet.content.decoded.data, 
                    &UA_TYPES[UA_TYPES_TARGETVARIABLESDATATYPE]);
    if(retVal != UA_STATUSCODE_GOOD)
        return retVal;


    return UA_STATUSCODE_GOOD;
}

/* UA_generateReaderGroupDataType() */
/**
 * @brief       Generates a ReaderGroupDataType object from a ReaderGroup.
 * @param       dst     [out]   ReaderGroupDataType
 * @param       src     [in]    ReaderGroup
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_generateReaderGroupDataType(UA_ReaderGroupDataType *dst,
                               const UA_ReaderGroup *src)
{
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
        retVal = UA_generateDataSetReaderDataType(&dst->dataSetReaders[dsReaderIndex], dsr);
        if(retVal != UA_STATUSCODE_GOOD) {
            break;
        }

        dsReaderIndex++;
    }

    return retVal;
}

/* UA_generatePubSubConnectionDataType() */
/**
 * @brief       Generates a PubSubConnectionDataType object from a PubSubConnection.
 * @param       dst     [out]   PubSubConnectionDataType
 * @param       src     [in]    PubSubConnection
 * @return      UA_STATUSCODE_GOOD on success
 */
static UA_StatusCode
UA_generatePubSubConnectionDataType(UA_PubSubConnectionDataType *dst,
                                    const UA_PubSubConnection *src)
{
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
        retVal = UA_generateWriterGroupDataType(&dst->writerGroups[wgIndex], wg);
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
        retVal = UA_generateReaderGroupDataType(&dst->readerGroups[rgIndex], rg);
        if(retVal != UA_STATUSCODE_GOOD) {
            return retVal;
        }

        rgIndex++;
    }

    return UA_STATUSCODE_GOOD;
}

/* UA_generatePubSubConfigurationDataType() */
/**
 * @brief       Generates a PubSubConfigurationDataType object from the current server configuration.
 * 
 * @param       server                  [in]    server, that contains the PubSub configuration
 * @param       pubSubConfiguration     [out]   target object
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_generatePubSubConfigurationDataType(const UA_Server* server,
                                       UA_PubSubConfigurationDataType *pubSubConfiguration)
{
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
    TAILQ_FOREACH(pds, &manager.publishedDataSets, listEntry){
        statusCode = UA_generatePublishedDataSetDataType(&pubSubConfiguration->publishedDataSets[pdsIndex],
                                                         pds);
        
        pdsIndex++;
        if(statusCode != UA_STATUSCODE_GOOD) {
            /*ToDo ERROR Log*/
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
        statusCode = UA_generatePubSubConnectionDataType(&pubSubConfiguration->connections[connectionIndex], 
                                                         connection);
        connectionIndex++;                                                         
        if(statusCode != UA_STATUSCODE_GOOD) {
            /*ToDo ERROR Log*/
            return statusCode;
        }
    }

    return UA_STATUSCODE_GOOD;
}

/* UA_savePubSubConfigToFile() */
/**
 * @brief       Saves the current PubSub configuration of a server in a file in the filesystem.
 * 
 * @param       server  [in]    Pointer to server object, that contains the PubSubConfiguration
 *
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_savePubSubConfigToFile(UA_Server *server)
{
    UA_StatusCode statusCode = UA_STATUSCODE_GOOD;
    UA_ByteString buffer = UA_STRING_NULL;
    UA_PubSubConfigurationDataType config;
    memset(&config, 0, sizeof(UA_PubSubConfigurationDataType));

    do {
        statusCode = UA_generatePubSubConfigurationDataType(server, &config);
        if(statusCode != UA_STATUSCODE_GOOD) {
            /*ToDo: ERROR Log*/
            break;
        }

        statusCode = UA_encodePubSubConfiguration(&config, &buffer);
        if(statusCode != UA_STATUSCODE_GOOD) {
            /*ToDo: ERROR Log*/
            break;
        }
        
        statusCode = UA_writeBufferToFile(server->pubSubManager.pubSubConfigFilename, buffer);
        if(statusCode != UA_STATUSCODE_GOOD) {
            /*ToDo: ERROR Log*/
            break;
        }

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Saving PubSub config was successful");
    } while(0);

    UA_ByteString_deleteMembers(&buffer);
    UA_PubSubConfigurationDataType_deleteMembers(&config);

    return statusCode;
}

/* Stores the name of PubSub configuration file in the PubSub manager of the specified server. */
UA_StatusCode 
UA_Server_setPubSubConfigFilename(UA_Server *server, const char *filename) {
    if(server == NULL || filename == NULL){
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_String_deleteMembers(&server->pubSubManager.pubSubConfigFilename);
    server->pubSubManager.pubSubConfigFilename = UA_String_fromChars(filename);
    if(server->pubSubManager.pubSubConfigFilename.length == 0) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_PUBSUB */
