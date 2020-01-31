/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "server/ua_server_internal.h"
#include "ua_pubsub_ns0.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#define UA_DATETIMESTAMP_2000 125911584000000000

UA_StatusCode
UA_Server_addPubSubConnection(UA_Server *server,
                              const UA_PubSubConnectionConfig *connectionConfig,
                              UA_NodeId *connectionIdentifier) {
    /* Find the matching UA_PubSubTransportLayers */
    UA_PubSubTransportLayer *tl = NULL;
    for(size_t i = 0; i < server->config.pubsubTransportLayersSize; i++) {
        if(connectionConfig &&
           UA_String_equal(&server->config.pubsubTransportLayers[i].transportProfileUri,
                           &connectionConfig->transportProfileUri)) {
            tl = &server->config.pubsubTransportLayers[i];
        }
    }
    if(!tl) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Requested transport layer not found.");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Create a copy of the connection config */
    UA_PubSubConnectionConfig *tmpConnectionConfig = (UA_PubSubConnectionConfig *)
        UA_calloc(1, sizeof(UA_PubSubConnectionConfig));
    if(!tmpConnectionConfig){
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of Memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_StatusCode retval = UA_PubSubConnectionConfig_copy(connectionConfig, tmpConnectionConfig);
    if(retval != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Could not copy the config.");
        return retval;
    }

    /* Create new connection and add to UA_PubSubManager */
    UA_PubSubConnection *newConnectionsField = (UA_PubSubConnection *)
        UA_calloc(1, sizeof(UA_PubSubConnection));
    if(!newConnectionsField) {
        UA_PubSubConnectionConfig_clear(tmpConnectionConfig);
        UA_free(tmpConnectionConfig);
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of Memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    if (server->pubSubManager.connectionsSize != 0)
        TAILQ_INSERT_TAIL(&server->pubSubManager.connections, newConnectionsField, listEntry);
    else {
        TAILQ_INIT(&server->pubSubManager.connections);
        TAILQ_INSERT_HEAD(&server->pubSubManager.connections, newConnectionsField, listEntry);
    }

    server->pubSubManager.connectionsSize++;

    LIST_INIT(&newConnectionsField->writerGroups);
    newConnectionsField->config = tmpConnectionConfig;

    /* Open the channel */
    newConnectionsField->channel = tl->createPubSubChannel(newConnectionsField->config);
    if(!newConnectionsField->channel) {
        UA_PubSubConnection_clear(server, newConnectionsField);
        TAILQ_REMOVE(&server->pubSubManager.connections, newConnectionsField, listEntry);
        server->pubSubManager.connectionsSize--;
        UA_free(newConnectionsField);
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Transport layer creation problem.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_PubSubManager_generateUniqueNodeId(server, &newConnectionsField->identifier);

    if(connectionIdentifier)
        UA_NodeId_copy(&newConnectionsField->identifier, connectionIdentifier);

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addPubSubConnectionRepresentation(server, newConnectionsField);
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    //search the identified Connection and store the Connection index
    UA_PubSubConnection *currentConnection = UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!currentConnection)
        return UA_STATUSCODE_BADNOTFOUND;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removePubSubConnectionRepresentation(server, currentConnection);
#endif
    server->pubSubManager.connectionsSize--;

    UA_PubSubConnection_clear(server, currentConnection);
    TAILQ_REMOVE(&server->pubSubManager.connections, currentConnection, listEntry);
    UA_free(currentConnection);
    return UA_STATUSCODE_GOOD;
}

UA_AddPublishedDataSetResult
UA_Server_addPublishedDataSet(UA_Server *server, const UA_PublishedDataSetConfig *publishedDataSetConfig,
                              UA_NodeId *pdsIdentifier) {
    UA_AddPublishedDataSetResult result = {UA_STATUSCODE_BADINVALIDARGUMENT, 0, NULL, {0, 0}};
    if(!publishedDataSetConfig){
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. No config passed in.");
        return result;
    }
    if(publishedDataSetConfig->publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS){
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Unsupported PublishedDataSet type.");
        return result;
    }
    //deep copy the given connection config
    UA_PublishedDataSetConfig tmpPublishedDataSetConfig;
    memset(&tmpPublishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    if(UA_PublishedDataSetConfig_copy(publishedDataSetConfig, &tmpPublishedDataSetConfig) != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Configuration copy failed.");
        result.addResult = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }
    //create new PDS and add to UA_PubSubManager
    UA_PublishedDataSet *newPubSubDataSetField = (UA_PublishedDataSet *)
            UA_calloc(1, sizeof(UA_PublishedDataSet));
    if(!newPubSubDataSetField) {
        UA_PublishedDataSetConfig_clear(&tmpPublishedDataSetConfig);
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Out of Memory.");
        result.addResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return result;
    }
    memset(newPubSubDataSetField, 0, sizeof(UA_PublishedDataSet));
    TAILQ_INIT(&newPubSubDataSetField->fields);
    newPubSubDataSetField->config = tmpPublishedDataSetConfig;

    if (server->pubSubManager.publishedDataSetsSize != 0)
        TAILQ_INSERT_TAIL(&server->pubSubManager.publishedDataSets, newPubSubDataSetField, listEntry);
    else {
        TAILQ_INIT(&server->pubSubManager.publishedDataSets);
        TAILQ_INSERT_HEAD(&server->pubSubManager.publishedDataSets, newPubSubDataSetField, listEntry);
    }
    if(tmpPublishedDataSetConfig.publishedDataSetType == UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE){
        //parse template config and add fields (later PubSub batch)
    }
    //generate unique nodeId
    UA_PubSubManager_generateUniqueNodeId(server, &newPubSubDataSetField->identifier);
    if(pdsIdentifier != NULL){
        UA_NodeId_copy(&newPubSubDataSetField->identifier, pdsIdentifier);
    }

    result.addResult = UA_STATUSCODE_GOOD;
    result.fieldAddResults = NULL;
    result.fieldAddResultsSize = 0;

    //fill the DataSetMetaData
    switch(tmpPublishedDataSetConfig.publishedDataSetType){
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
            if(UA_DataSetMetaDataType_copy(&tmpPublishedDataSetConfig.config.itemsTemplate.metaData,
                    &newPubSubDataSetField->dataSetMetaData) != UA_STATUSCODE_GOOD){
                UA_Server_removeDataSetField(server, newPubSubDataSetField->identifier);
                result.addResult = UA_STATUSCODE_BADINTERNALERROR;
            }
            break;
        case UA_PUBSUB_DATASET_PUBLISHEDEVENTS_TEMPLATE:
            if(UA_DataSetMetaDataType_copy(&tmpPublishedDataSetConfig.config.eventTemplate.metaData,
                    &newPubSubDataSetField->dataSetMetaData) != UA_STATUSCODE_GOOD){
                UA_Server_removeDataSetField(server, newPubSubDataSetField->identifier);
                result.addResult = UA_STATUSCODE_BADINTERNALERROR;
            }
            break;
        case UA_PUBSUB_DATASET_PUBLISHEDEVENTS:
            newPubSubDataSetField->dataSetMetaData.configurationVersion.majorVersion = UA_PubSubConfigurationVersionTimeDifference();
            newPubSubDataSetField->dataSetMetaData.configurationVersion.minorVersion = UA_PubSubConfigurationVersionTimeDifference();
            newPubSubDataSetField->dataSetMetaData.dataSetClassId = UA_GUID_NULL;
            if(UA_String_copy(&tmpPublishedDataSetConfig.name, &newPubSubDataSetField->dataSetMetaData.name) != UA_STATUSCODE_GOOD){
                UA_Server_removeDataSetField(server, newPubSubDataSetField->identifier);
                result.addResult = UA_STATUSCODE_BADINTERNALERROR;
            }
            newPubSubDataSetField->dataSetMetaData.description = UA_LOCALIZEDTEXT_ALLOC("", "");
            break;
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
            newPubSubDataSetField->dataSetMetaData.configurationVersion.majorVersion = UA_PubSubConfigurationVersionTimeDifference();
            newPubSubDataSetField->dataSetMetaData.configurationVersion.minorVersion = UA_PubSubConfigurationVersionTimeDifference();
            if(UA_String_copy(&tmpPublishedDataSetConfig.name, &newPubSubDataSetField->dataSetMetaData.name) != UA_STATUSCODE_GOOD){
                UA_Server_removeDataSetField(server, newPubSubDataSetField->identifier);
                result.addResult = UA_STATUSCODE_BADINTERNALERROR;
            }
            newPubSubDataSetField->dataSetMetaData.description = UA_LOCALIZEDTEXT_ALLOC("", "");
            newPubSubDataSetField->dataSetMetaData.dataSetClassId = UA_GUID_NULL;
            break;
    }

    server->pubSubManager.publishedDataSetsSize++;
    result.configurationVersion.majorVersion = UA_PubSubConfigurationVersionTimeDifference();
    result.configurationVersion.minorVersion = UA_PubSubConfigurationVersionTimeDifference();
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addPublishedDataItemsRepresentation(server, newPubSubDataSetField);
#endif
    return result;
}

UA_StatusCode
UA_Server_removePublishedDataSet(UA_Server *server, const UA_NodeId pds) {
    //search the identified PublishedDataSet and store the PDS index
    UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_findPDSbyId(server, pds);
    if(!publishedDataSet){
        return UA_STATUSCODE_BADNOTFOUND;
    }
    if(publishedDataSet->config.configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Remove PublishedDataSet failed. PublishedDataSet is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    //search for referenced writers -> delete this writers. (Standard: writer must be connected with PDS)
    UA_PubSubConnection *tmpConnectoin;
    TAILQ_FOREACH(tmpConnectoin, &server->pubSubManager.connections, listEntry){
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &tmpConnectoin->writerGroups, listEntry){
            UA_DataSetWriter *currentWriter, *tmpWriterGroup;
            LIST_FOREACH_SAFE(currentWriter, &writerGroup->writers, listEntry, tmpWriterGroup){
                if(UA_NodeId_equal(&currentWriter->connectedDataSet, &publishedDataSet->identifier)){
                    UA_Server_removeDataSetWriter(server, currentWriter->identifier);
                }
            }
        }
    }
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removePublishedDataSetRepresentation(server, publishedDataSet);
#endif
    UA_PublishedDataSet_clear(server, publishedDataSet);
    server->pubSubManager.publishedDataSetsSize--;

    TAILQ_REMOVE(&server->pubSubManager.publishedDataSets, publishedDataSet, listEntry);
    UA_free(publishedDataSet);
    return UA_STATUSCODE_GOOD;
}

/* Calculate the time difference between current time and UTC (00:00) on January
 * 1, 2000. */
UA_UInt32
UA_PubSubConfigurationVersionTimeDifference() {
    UA_UInt32 timeDiffSince2000 = (UA_UInt32) (UA_DateTime_now() - UA_DATETIMESTAMP_2000);
    return timeDiffSince2000;
}

/* Generate a new unique NodeId. This NodeId will be used for the information
 * model representation of PubSub entities. */
void
UA_PubSubManager_generateUniqueNodeId(UA_Server *server, UA_NodeId *nodeId) {
    UA_NodeId newNodeId = UA_NODEID_NUMERIC(0, 0);
    UA_Node *newNode = UA_NODESTORE_NEW(server, UA_NODECLASS_OBJECT);
    UA_NODESTORE_INSERT(server, newNode, &newNodeId);
    UA_NodeId_copy(&newNodeId, nodeId);
}

/* Delete the current PubSub configuration including all nested members. This
 * action also delete the configured PubSub transport Layers. */
void
UA_PubSubManager_delete(UA_Server *server, UA_PubSubManager *pubSubManager) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "PubSub cleanup was called.");

    /* Stop and unfreeze all WriterGroups */
    UA_PubSubConnection *tmpConnection;
    TAILQ_FOREACH(tmpConnection, &server->pubSubManager.connections, listEntry){
        for(size_t i = 0; i < pubSubManager->connectionsSize; i++) {
            UA_WriterGroup *writerGroup;
            LIST_FOREACH(writerGroup, &tmpConnection->writerGroups, listEntry) {
                UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_DISABLED, writerGroup);
                UA_Server_unfreezeWriterGroupConfiguration(server, writerGroup->identifier);
            }
        }
    }

    //free the currently configured transport layers
    if (server->config.pubsubTransportLayersSize > 0) {
        UA_free(server->config.pubsubTransportLayers);
        server->config.pubsubTransportLayersSize = 0;
    }

    //remove Connections and WriterGroups
    UA_PubSubConnection *tmpConnection1, *tmpConnection2;
    TAILQ_FOREACH_SAFE(tmpConnection1, &server->pubSubManager.connections, listEntry, tmpConnection2){
        UA_Server_removePubSubConnection(server, tmpConnection1->identifier);
    }
    UA_PublishedDataSet *tmpPDS1, *tmpPDS2;
    TAILQ_FOREACH_SAFE(tmpPDS1, &server->pubSubManager.publishedDataSets, listEntry, tmpPDS2){
        UA_Server_removePublishedDataSet(server, tmpPDS1->identifier);
    }
}

/* Information model RT handling */
/*
* @param server The server executing the callback
* @param sessionId The identifier of the session
* @param sessionContext Additional data attached to the session in the
*        access control layer
* @param nodeId The identifier of the node being read from
* @param nodeContext Additional data attached to the node by the user
* @param includeSourceTimeStamp If true, then the datasource is expected to
*        set the source timestamp in the returned value
* @param range If not null, then the datasource shall return only a
*        selection of the (nonscalar) data. Set
*        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not
        *        apply
* @param value The (non-null) DataValue that is returned to the client. The
        *        data source sets the read data, the result status and optionally a
*        sourcetimestamp.
* @return Returns a status code for logging. Error codes intended for the
        *         original caller are set in the value. If an error is returned,
*         then no releasing of the value is done
*/
static UA_StatusCode
readInformationModelVariableMemory(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *nodeId,
                      void *nodeContext, UA_Boolean includeSourceTimeStamp,
                      const UA_NumericRange *range, UA_DataValue *value){
    UA_Variant *internalValue = (UA_Variant *) nodeContext;
    UA_Variant_setScalarCopy(&value->value, internalValue->data, internalValue->type);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

/* Write into a data source. This method pointer can be NULL if the
 * operation is unsupported.
 *
 * @param server The server executing the callback
 * @param sessionId The identifier of the session
 * @param sessionContext Additional data attached to the session in the
 *        access control layer
 * @param nodeId The identifier of the node being written to
 * @param nodeContext Additional data attached to the node by the user
 * @param range If not NULL, then the datasource shall return only a
 *        selection of the (nonscalar) data. Set
 *        UA_STATUSCODE_BADINDEXRANGEINVALID in the value if this does not
 *        apply
 * @param value The (non-NULL) DataValue that has been written by the client.
 *        The data source contains the written data, the result status and
 *        optionally a sourcetimestamp
 * @return Returns a status code for logging. Error codes intended for the
 *         original caller are set in the value. If an error is returned,
 *         then no releasing of the value is done
 */
static UA_StatusCode
writeInformationMdelVariableMemory(UA_Server *server, const UA_NodeId *sessionId,
                       void *sessionContext, const UA_NodeId *nodeId,
                       void *nodeContext, const UA_NumericRange *range,
                       const UA_DataValue *value){
    UA_Variant *internalValue = (UA_Variant *) nodeContext;
    *(UA_Int32 *) internalValue->data = *(UA_Int32 *) value->value.data;
    //UA_Variant_setScalarCopy(&dataValue.value, internalValue->data, internalValue->type);
    //dataValue.hasValue = true;

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addPubSubRTvariableNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                                       const UA_NodeId parentNodeId, const UA_NodeId referenceTypeId,
                                       const UA_QualifiedName browseName, const UA_NodeId typeDefinition,
                                       const UA_VariableAttributes attr, UA_Variant *staticValue,
                                       void *nodeContext, UA_NodeId *outNewNodeId){
    UA_StatusCode retVal;
    if(!UA_DataType_isNumeric(UA_findDataType(&staticValue->type->typeId)))
        return UA_STATUSCODE_BADNOTSUPPORTED;
    /* add var + data set and read callback */
    UA_DataSource dataSourceField;
    memset(&dataSourceField, 0, sizeof(UA_DataSource));
    dataSourceField.read = readInformationModelVariableMemory;
    dataSourceField.write = writeInformationMdelVariableMemory;
    retVal = UA_Server_addDataSourceVariableNode(server, requestedNewNodeId, parentNodeId, referenceTypeId,
                                        browseName, typeDefinition, attr, dataSourceField, staticValue, outNewNodeId);
    /* ToDo add destructor to clean up structures */
    return retVal;
}

UA_StatusCode
UA_Server_swapExistingVariableNodeToRT(UA_Server *server, UA_NodeId targetNodeId, UA_Variant *staticValue){
    UA_StatusCode retVal;
    UA_NodeId nodeDataType;
    UA_Server_readDataType(server, targetNodeId, &nodeDataType);
    if(!UA_DataType_isNumeric(UA_findDataType(&nodeDataType)))
        return UA_STATUSCODE_BADNOTSUPPORTED;
    UA_Variant lastValue;
    UA_Server_readValue(server, targetNodeId, &lastValue);
    //check if DataType of static value is equal with target Node in the information model
    if(!UA_NodeId_equal(&nodeDataType, &staticValue->type->typeId))
        return UA_STATUSCODE_BADNOTSUPPORTED;
    //set copy of the value
    UA_copy(lastValue.data, staticValue->data, UA_findDataType(&nodeDataType));
    //set dataSource methods to the specified field
    UA_DataSource dataSourceField;
    memset(&dataSourceField, 0, sizeof(UA_DataSource));
    dataSourceField.read = readInformationModelVariableMemory;
    dataSourceField.write = writeInformationMdelVariableMemory;
    UA_Server_setNodeContext(server, targetNodeId, staticValue);
    retVal = UA_Server_setVariableNode_dataSource(server, targetNodeId, dataSourceField);
    return retVal;
}

/***********************************/
/*      PubSub Jobs abstraction    */
/***********************************/

#ifndef UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING

/* If UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING is enabled, a custom callback
 * management must be linked to the application */

UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&server->timer, (UA_ApplicationCallback)callback,
                                        server, data, interval_ms, callbackId);
}

UA_StatusCode
UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                                UA_Double interval_ms) {
    return UA_Timer_changeRepeatedCallbackInterval(&server->timer, callbackId, interval_ms);
}

void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
    UA_Timer_removeCallback(&server->timer, callbackId);
}

#endif /* UA_ENABLE_PUBSUB_CUSTOM_PUBLISH_HANDLING */

#endif /* UA_ENABLE_PUBSUB */
