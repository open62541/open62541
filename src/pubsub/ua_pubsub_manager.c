/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include <open62541/server_pubsub.h>

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "server/ua_server_internal.h"
#include "ua_pubsub_ns0.h"

#define UA_DATETIMESTAMP_2000 125911584000000000

static UA_PubSubTransportLayer *
getTransportProtocolLayer(const UA_Server *server,
                          const UA_String *transportProfileUri) {
    /* Find the matching UA_PubSubTransportLayers */
    UA_PubSubTransportLayer *tl = NULL;
    for(size_t i = 0; i < server->config.pubSubConfig.transportLayersSize; i++) {
        if(UA_String_equal(&server->config.pubSubConfig.transportLayers[i].transportProfileUri,
                           transportProfileUri)) {
            tl = &server->config.pubSubConfig.transportLayers[i];
        }
    }
    if(!tl) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Requested transport layer not found.");
        return NULL;
    }
    return tl;
}

static void
UA_PubSubConfig_delete(UA_PubSubConnectionConfig *tmpConnectionConfig) {
    UA_PubSubConnectionConfig_clear(tmpConnectionConfig);
    UA_free(tmpConnectionConfig);
}

static UA_StatusCode
copyConnectionConfig(const UA_PubSubConnectionConfig *srcConfig, UA_PubSubConnectionConfig **dstConfig, UA_Logger *logger) {
    /* Create a copy of the connection config */
    *dstConfig = (UA_PubSubConnectionConfig *) UA_calloc(1, sizeof(UA_PubSubConnectionConfig));
    UA_CHECK_MEM_ERROR(dstConfig, return UA_STATUSCODE_BADOUTOFMEMORY,
                       logger, UA_LOGCATEGORY_SERVER,
                       "PubSub Connection creation failed. Out of Memory.");

    UA_StatusCode retval = UA_PubSubConnectionConfig_copy(srcConfig, *dstConfig);
    UA_CHECK_STATUS_ERROR(retval, goto copy_error, logger, UA_LOGCATEGORY_SERVER,
                          "PubSub Connection creation failed. Could not copy the config.");

    return UA_STATUSCODE_GOOD;
copy_error:
    UA_free(*dstConfig);
    return retval;
}

static void
UA_PubSubManager_addConnection(UA_PubSubManager *pubSubManager, UA_PubSubConnection *connection) {
    if (pubSubManager->connectionsSize != 0) {
        TAILQ_INSERT_TAIL(&pubSubManager->connections, connection, listEntry);
    } else {
        TAILQ_INIT(&pubSubManager->connections);
        TAILQ_INSERT_HEAD(&pubSubManager->connections, connection, listEntry);
    }
    pubSubManager->connectionsSize++;
}

static UA_PubSubConnection *
UA_PubSubConnection_new(UA_PubSubConnectionConfig *connectionConfig,
                        UA_Logger *logger) {/* Create new connection and add to UA_PubSubManager */
    UA_PubSubConnection *newConnectionsField = (UA_PubSubConnection *)
        UA_calloc(1, sizeof(UA_PubSubConnection));
    if(!newConnectionsField) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Connection creation failed. Out of Memory.");
        return NULL;
    }
    newConnectionsField->componentType = UA_PUBSUB_COMPONENT_CONNECTION;
    LIST_INIT(&newConnectionsField->writerGroups);
    newConnectionsField->config = connectionConfig;
    return newConnectionsField;
}

static UA_StatusCode
channelErrorHandling(UA_Server *server, UA_PubSubConnection *newConnectionsField) {
    UA_PubSubConnection_clear(server, newConnectionsField);
    TAILQ_REMOVE(&server->pubSubManager.connections, newConnectionsField, listEntry);
    server->pubSubManager.connectionsSize--;
    UA_free(newConnectionsField);
    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "PubSub Connection creation failed. Transport layer creation problem.");
    return UA_STATUSCODE_BADINTERNALERROR;
}

static UA_StatusCode
createAndAddConnection(UA_Server *server, const UA_PubSubConnectionConfig *connectionConfig,
                       UA_PubSubConnection **connection) {
    /* Create a copy of the connection config */
    UA_PubSubConnectionConfig *tmpConnectionConfig = NULL;
    UA_StatusCode retval = copyConnectionConfig(connectionConfig, &tmpConnectionConfig, &server->config.logger);
    UA_CHECK_STATUS(retval, return retval);

    *connection = UA_PubSubConnection_new(tmpConnectionConfig, &server->config.logger);
    UA_CHECK_MEM(*connection, UA_PubSubConfig_delete(tmpConnectionConfig); return UA_STATUSCODE_BADOUTOFMEMORY;);

    UA_PubSubManager *pubSubManager = &server->pubSubManager;
    UA_PubSubManager_addConnection(pubSubManager, *connection);
    return UA_STATUSCODE_GOOD;
}

static void
assignConnectionIdentifier(UA_Server *server, UA_PubSubConnection *newConnectionsField,
                           UA_NodeId *connectionIdentifier) {
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* Internally createa a unique id */
    addPubSubConnectionRepresentation(server, newConnectionsField);
#else
    /* Create a unique NodeId that does not correspond to a Node */
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newConnectionsField->identifier);
#endif
    if(connectionIdentifier) {
        UA_NodeId_copy(&newConnectionsField->identifier, connectionIdentifier);
    }
}

UA_StatusCode
UA_Server_addPubSubConnection(UA_Server *server,
                              const UA_PubSubConnectionConfig *connectionConfig,
                              UA_NodeId *connectionIdentifier) {

    /* validate preconditions */
    UA_CHECK_MEM(server, return UA_STATUSCODE_BADINTERNALERROR);
    UA_CHECK_MEM_ERROR(connectionConfig, return UA_STATUSCODE_BADINTERNALERROR, &server->config.logger,
                       UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. No connection configuration supplied.");

    /* Retrieve the transport layer for the given profile uri */
    UA_PubSubTransportLayer *tl = getTransportProtocolLayer(server, &connectionConfig->transportProfileUri);
    UA_CHECK_MEM_ERROR(tl, return UA_STATUSCODE_BADNOTFOUND, &server->config.logger,
                       UA_LOGCATEGORY_SERVER, "PubSub Connection creation failed. Requested transport layer not found.");

    /* create and add new connection from connection config */
    UA_PubSubConnection *newConnectionsField = NULL;
    UA_StatusCode retval = createAndAddConnection(server, connectionConfig, &newConnectionsField);
    UA_CHECK_STATUS(retval, return retval);

    /* Open the communication channel */
    newConnectionsField->channel = tl->createPubSubChannel(newConnectionsField->config);
    UA_CHECK_MEM(newConnectionsField->channel, return channelErrorHandling(server, newConnectionsField));

    assignConnectionIdentifier(server, newConnectionsField, connectionIdentifier);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    /* Find the connection */
    UA_PubSubConnection *c =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Stop, unfreeze and delete all WriterGroups attached to the Connection */
    UA_WriterGroup *writerGroup, *tmpWriterGroup;
    LIST_FOREACH_SAFE(writerGroup, &c->writerGroups, listEntry, tmpWriterGroup) {
        UA_WriterGroup_setPubSubState(server, writerGroup, UA_PUBSUBSTATE_DISABLED,
                                      UA_STATUSCODE_BADSHUTDOWN);
        UA_Server_unfreezeWriterGroupConfiguration(server, writerGroup->identifier);
        removeWriterGroup(server, writerGroup->identifier);
    }

    /* Stop, unfreeze and delete all ReaderGroups attached to the Connection */
    UA_ReaderGroup *readerGroup, *tmpReaderGroup;
    LIST_FOREACH_SAFE(readerGroup, &c->readerGroups, listEntry, tmpReaderGroup) {
        UA_ReaderGroup_setPubSubState(server, readerGroup, UA_PUBSUBSTATE_DISABLED,
                                      UA_STATUSCODE_BADSHUTDOWN);
        UA_Server_unfreezeReaderGroupConfiguration(server, readerGroup->identifier);
        removeReaderGroup(server, readerGroup->identifier);
    }

    /* Remove from the information model */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removePubSubConnectionRepresentation(server, c);
#endif

    /* Unlink from the server */
    TAILQ_REMOVE(&server->pubSubManager.connections, c, listEntry);
    server->pubSubManager.connectionsSize--;

    /* Clean up the connection structure */
    UA_PubSubConnection_clear(server, c);
    UA_free(c);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = removePubSubConnection(server, connection);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

UA_StatusCode
UA_PubSubConnection_regist(UA_Server *server, UA_NodeId *connectionIdentifier) {
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, *connectionIdentifier);
    if(!connection)
        return UA_STATUSCODE_BADNOTFOUND;

    if(connection->isRegistered) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Connection already registered");
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode retval = connection->channel->regist(connection->channel, NULL, NULL);
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "register channel failed: 0x%" PRIx32 "!", retval);

    connection->isRegistered = UA_TRUE;
    return retval;
}

UA_AddPublishedDataSetResult
UA_Server_addPublishedDataSet(UA_Server *server,
                              const UA_PublishedDataSetConfig *publishedDataSetConfig,
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

    if(UA_String_isEmpty(&publishedDataSetConfig->name))
    {
        // DataSet has to have a valid name
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Invalid name.");
        return result;
    }

    if(UA_PublishedDataSet_findPDSbyName(server, publishedDataSetConfig->name))
    {
        // DataSet name has to be unique in the publisher
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. DataSet with the same name already exists.");
        result.addResult = UA_STATUSCODE_BADBROWSENAMEDUPLICATED;
        return result;
    }

    /* Create new PDS and add to UA_PubSubManager */
    UA_PublishedDataSet *newPDS = (UA_PublishedDataSet *)
        UA_calloc(1, sizeof(UA_PublishedDataSet));
    if(!newPDS) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Out of Memory.");
        result.addResult = UA_STATUSCODE_BADOUTOFMEMORY;
        return result;
    }
    TAILQ_INIT(&newPDS->fields);

    UA_PublishedDataSetConfig *newConfig = &newPDS->config;

    /* Deep copy the given connection config */
    UA_StatusCode res = UA_PublishedDataSetConfig_copy(publishedDataSetConfig, newConfig);
    if(res != UA_STATUSCODE_GOOD){
        UA_free(newPDS);
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Configuration copy failed.");
        result.addResult = UA_STATUSCODE_BADINTERNALERROR;
        return result;
    }

    /* TODO: Parse template config and add fields (later PubSub batch) */
    if(newConfig->publishedDataSetType == UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE) {
    }

    /* Fill the DataSetMetaData */
    result.configurationVersion.majorVersion = UA_PubSubConfigurationVersionTimeDifference();
    result.configurationVersion.minorVersion = UA_PubSubConfigurationVersionTimeDifference();
    switch(newConfig->publishedDataSetType) {
    case UA_PUBSUB_DATASET_PUBLISHEDEVENTS_TEMPLATE:
        res = UA_STATUSCODE_BADNOTSUPPORTED;
        break;
    case UA_PUBSUB_DATASET_PUBLISHEDEVENTS:
        res = UA_STATUSCODE_BADNOTSUPPORTED;
        break;
    case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
        newPDS->dataSetMetaData.configurationVersion.majorVersion =
            UA_PubSubConfigurationVersionTimeDifference();
        newPDS->dataSetMetaData.configurationVersion.minorVersion =
            UA_PubSubConfigurationVersionTimeDifference();
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
    if(server->pubSubManager.publishedDataSetsSize != 0) {
        TAILQ_INSERT_TAIL(&server->pubSubManager.publishedDataSets,
                          newPDS, listEntry);
    } else {
        TAILQ_INIT(&server->pubSubManager.publishedDataSets);
        TAILQ_INSERT_HEAD(&server->pubSubManager.publishedDataSets,
                          newPDS, listEntry);
    }
    server->pubSubManager.publishedDataSetsSize++;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* Create representation and unique id */
    addPublishedDataItemsRepresentation(server, newPDS);
#else
    /* Generate unique nodeId */
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager, &newPDS->identifier);
#endif
    if(pdsIdentifier)
        UA_NodeId_copy(&newPDS->identifier, pdsIdentifier);

    return result;
}

static UA_StatusCode
removePublishedDataSet(UA_Server *server, const UA_NodeId pds) {
    //search the identified PublishedDataSet and store the PDS index
    UA_PublishedDataSet *publishedDataSet = UA_PublishedDataSet_findPDSbyId(server, pds);
    if(!publishedDataSet){
        return UA_STATUSCODE_BADNOTFOUND;
    }
    if(publishedDataSet->configurationFrozen){
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
                    removeDataSetWriter(server, currentWriter->identifier);
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

UA_StatusCode
UA_Server_removePublishedDataSet(UA_Server *server, const UA_NodeId pds) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = removePublishedDataSet(server, pds);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

/* Calculate the time difference between current time and UTC (00:00) on January
 * 1, 2000. */
UA_UInt32
UA_PubSubConfigurationVersionTimeDifference(void) {
    UA_UInt32 timeDiffSince2000 = (UA_UInt32) (UA_DateTime_now() - UA_DATETIMESTAMP_2000);
    return timeDiffSince2000;
}

/* Generate a new unique NodeId. This NodeId will be used for the information
 * model representation of PubSub entities. */
#ifndef UA_ENABLE_PUBSUB_INFORMATIONMODEL
void
UA_PubSubManager_generateUniqueNodeId(UA_PubSubManager *psm, UA_NodeId *nodeId) {
    *nodeId = UA_NODEID_NUMERIC(1, ++psm->uniqueIdCount);
}
#endif

UA_Guid
UA_PubSubManager_generateUniqueGuid(UA_Server *server) {
    while(true) {
        UA_NodeId testId = UA_NODEID_GUID(1, UA_Guid_random());
        const UA_Node *testNode = UA_NODESTORE_GET(server, &testId);
        if(!testNode)
            return testId.identifier.guid;
        UA_NODESTORE_RELEASE(server, testNode);
    }
}

/* Delete the current PubSub configuration including all nested members. This
 * action also delete the configured PubSub transport Layers. */
void
UA_PubSubManager_delete(UA_Server *server, UA_PubSubManager *pubSubManager) {
    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "PubSub cleanup was called.");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Remove Connections - this also remove WriterGroups and ReaderGroups */
    UA_PubSubConnection *tmpConnection1, *tmpConnection2;
    TAILQ_FOREACH_SAFE(tmpConnection1, &server->pubSubManager.connections,
                       listEntry, tmpConnection2) {
        removePubSubConnection(server, tmpConnection1->identifier);
    }

    /* Remove the DataSets */
    UA_PublishedDataSet *tmpPDS1, *tmpPDS2;
    TAILQ_FOREACH_SAFE(tmpPDS1, &server->pubSubManager.publishedDataSets,
                       listEntry, tmpPDS2){
        removePublishedDataSet(server, tmpPDS1->identifier);
    }


    /* Free the list of transport layers */
    if(server->config.pubSubConfig.transportLayersSize > 0) {
        UA_free(server->config.pubSubConfig.transportLayers);
        server->config.pubSubConfig.transportLayersSize = 0;
    }
}

/***********************************/
/*      PubSub Jobs abstraction    */
/***********************************/

/* Default Timer based PubSub Callbacks */

UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_Double interval_ms, UA_DateTime *baseTime,
                                     UA_TimerPolicy timerPolicy, UA_UInt64 *callbackId) {
    return server->config.eventLoop->
        addCyclicCallback(server->config.eventLoop, (UA_Callback)callback, server, data,
                          interval_ms, baseTime, timerPolicy, callbackId);
}

UA_StatusCode
UA_PubSubManager_changeRepeatedCallback(UA_Server *server, UA_UInt64 callbackId,
                                        UA_Double interval_ms, UA_DateTime *baseTime,
                                        UA_TimerPolicy timerPolicy) {
    return server->config.eventLoop->
        modifyCyclicCallback(server->config.eventLoop, callbackId, interval_ms,
                             baseTime, timerPolicy);
}

void
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
    server->config.eventLoop->removeCyclicCallback(server->config.eventLoop, callbackId);
}


#ifdef UA_ENABLE_PUBSUB_MONITORING

static UA_StatusCode
UA_PubSubComponent_createMonitoring(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType, void *data, UA_ServerCallback callback) {

    if ((!server) || (!data)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Error UA_PubSubComponent_createMonitoring(): "
            "null pointer param");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT:
                    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER, "UA_PubSubComponent_createMonitoring(): DataSetReader '%.*s' "
                        "- MessageReceiveTimeout", (UA_Int32) reader->config.name.length, reader->config.name.data);
                    reader->msgRcvTimeoutTimerCallback = callback;
                    break;
                default:
                    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "UA_PubSubComponent_createMonitoring(): DataSetReader '%.*s' "
                        "DataSetReader does not support timeout type '%i'", (UA_Int32) reader->config.name.length, reader->config.name.data,
                        eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Error UA_PubSubComponent_createMonitoring(): PubSub component type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

static UA_StatusCode
UA_PubSubComponent_startMonitoring(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                   UA_PubSubMonitoringType eMonitoringType, void *data) {

    if ((!server) || (!data)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Error UA_PubSubComponent_startMonitoring(): "
            "null pointer param");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT: {
                    /* use a timed callback, because one notification is enough,
                    we assume that MessageReceiveTimeout configuration is in [ms], we do not handle or check fractions */
                    UA_UInt64 interval = (UA_UInt64)(reader->config.messageReceiveTimeout * UA_DATETIME_MSEC);
                    ret = server->config.eventLoop->
                        addTimedCallback(server->config.eventLoop, (UA_Callback) reader->msgRcvTimeoutTimerCallback,
                                         server, reader, UA_DateTime_nowMonotonic() + (UA_DateTime) interval,
                                         &(reader->msgRcvTimeoutTimerId));
                    if (ret == UA_STATUSCODE_GOOD) {
                        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "UA_PubSubComponent_startMonitoring(): DataSetReader '%.*s'- MessageReceiveTimeout: MessageReceiveTimeout = '%f' "
                            "Timer Id = '%u'", (UA_Int32) reader->config.name.length, reader->config.name.data,
                                reader->config.messageReceiveTimeout, (UA_UInt32) reader->msgRcvTimeoutTimerId);
                    } else {
                        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Error UA_PubSubComponent_startMonitoring(): DataSetReader '%.*s' - MessageReceiveTimeout: start timer failed",
                                (UA_Int32) reader->config.name.length, reader->config.name.data);
                    }
                    break;
                }
                default:
                    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "UA_PubSubComponent_startMonitoring(): DataSetReader '%.*s' "
                        "DataSetReader does not support timeout type '%i'", (UA_Int32) reader->config.name.length, reader->config.name.data,
                            eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Error UA_PubSubComponent_startMonitoring(): PubSub component type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

static UA_StatusCode
UA_PubSubComponent_stopMonitoring(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                  UA_PubSubMonitoringType eMonitoringType, void *data) {

    if ((!server) || (!data)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Error UA_PubSubComponent_stopMonitoring(): "
            "null pointer param");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT: {
                    server->config.eventLoop->removeCyclicCallback(server->config.eventLoop, reader->msgRcvTimeoutTimerId);
                    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                        "UA_PubSubComponent_stopMonitoring(): DataSetReader '%.*s' - MessageReceiveTimeout: MessageReceiveTimeout = '%f' "
                            "Timer Id = '%u'", (UA_Int32) reader->config.name.length, reader->config.name.data,
                                reader->config.messageReceiveTimeout, (UA_UInt32) reader->msgRcvTimeoutTimerId);
                    break;
                }
                default:
                    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "UA_PubSubComponent_stopMonitoring(): DataSetReader '%.*s' "
                        "DataSetReader does not support timeout type '%i'", (UA_Int32) reader->config.name.length, reader->config.name.data,
                        eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Error UA_PubSubComponent_stopMonitoring(): PubSub component type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

static UA_StatusCode
UA_PubSubComponent_updateMonitoringInterval(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                            UA_PubSubMonitoringType eMonitoringType, void *data)
{
    if ((!server) || (!data)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Error UA_PubSubComponent_updateMonitoringInterval(): "
            "null pointer param");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT: {
                    ret = server->config.eventLoop->
                        modifyCyclicCallback(server->config.eventLoop, reader->msgRcvTimeoutTimerId,
                                             reader->config.messageReceiveTimeout, NULL,
                                             UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME);
                    if (ret == UA_STATUSCODE_GOOD) {
                        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "UA_PubSubComponent_updateMonitoringInterval(): DataSetReader '%.*s' - MessageReceiveTimeout: new MessageReceiveTimeout = '%f' "
                            "Timer Id = '%u'", (UA_Int32) reader->config.name.length, reader->config.name.data,
                                reader->config.messageReceiveTimeout, (UA_UInt32) reader->msgRcvTimeoutTimerId);
                    } else {
                        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                            "Error UA_PubSubComponent_updateMonitoringInterval(): DataSetReader '%.*s': update timer interval failed",
                                (UA_Int32) reader->config.name.length, reader->config.name.data);
                    }
                    break;
                }
                default:
                    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "UA_PubSubComponent_createMonitoring(): DataSetReader '%.*s' "
                        "DataSetReader does not support timeout type '%i'", (UA_Int32) reader->config.name.length, reader->config.name.data,
                        eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Error UA_PubSubComponent_updateMonitoringInterval(): PubSub component type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

static UA_StatusCode
UA_PubSubComponent_deleteMonitoring(UA_Server *server, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType, void *data) {

    if ((!server) || (!data)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "Error UA_PubSubComponent_deleteMonitoring(): "
            "null pointer param");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT:
                    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                        "UA_PubSubComponent_deleteMonitoring(): DataSetReader '%.*s' - MessageReceiveTimeout: Timer Id = '%u'",
                        (UA_Int32) reader->config.name.length, reader->config.name.data, (UA_UInt32) reader->msgRcvTimeoutTimerId);
                    break;
                default:
                    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER, "UA_PubSubComponent_deleteMonitoring(): DataSetReader '%.*s' "
                        "DataSetReader does not support timeout type '%i'", (UA_Int32) reader->config.name.length, reader->config.name.data,
                        eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Error UA_PubSubComponent_deleteMonitoring(): PubSub component type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

UA_StatusCode
UA_PubSubManager_setDefaultMonitoringCallbacks(UA_PubSubMonitoringInterface *monitoringInterface) {
    if (monitoringInterface == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    monitoringInterface->createMonitoring = UA_PubSubComponent_createMonitoring;
    monitoringInterface->startMonitoring = UA_PubSubComponent_startMonitoring;
    monitoringInterface->stopMonitoring = UA_PubSubComponent_stopMonitoring;
    monitoringInterface->updateMonitoringInterval = UA_PubSubComponent_updateMonitoringInterval;
    monitoringInterface->deleteMonitoring = UA_PubSubComponent_deleteMonitoring;
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_PUBSUB_MONITORING */

#endif /* UA_ENABLE_PUBSUB */
