/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "server/ua_server_internal.h"
#include "ua_pubsub_ns0.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#define UA_DATETIMESTAMP_2000 125911584000000000

UA_StatusCode
UA_Server_addPubSubConnection(UA_Server *server, const UA_PubSubConnectionConfig *connectionConfig,
                              UA_NodeId *connectionIdentifier) {
    //iterate over the available UA_PubSubTransportLayers
    for(size_t i = 0; i < server->config.pubsubTransportLayersSize; i++) {
        if(connectionConfig && UA_String_equal(&server->config.pubsubTransportLayers[i].transportProfileUri,
                                               &connectionConfig->transportProfileUri)){
            //create new connection config
            UA_PubSubConnectionConfig *tmpConnectionConfig = (UA_PubSubConnectionConfig *)
                    UA_calloc(1, sizeof(UA_PubSubConnectionConfig));
            if(!tmpConnectionConfig){
                UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                             "PubSub Connection creation failed. Out of Memory.");
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            //deep copy the given connection config
            if(UA_PubSubConnectionConfig_copy(connectionConfig, tmpConnectionConfig) != UA_STATUSCODE_GOOD){
                UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                             "PubSub Connection creation failed. Config copy problem.");
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            //create new connection and add to UA_PubSubManager
            UA_PubSubConnection *newConnectionsField = (UA_PubSubConnection *)
                    UA_realloc(server->pubSubManager.connections,
                               sizeof(UA_PubSubConnection) * (server->pubSubManager.connectionsSize + 1));
            if(!newConnectionsField) {
                UA_PubSubConnectionConfig_deleteMembers(tmpConnectionConfig);
                UA_free(tmpConnectionConfig);
                UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                             "PubSub Connection creation failed. Out of Memory.");
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
            server->pubSubManager.connections = newConnectionsField;
            UA_PubSubConnection *newConnection = &server->pubSubManager.connections[server->pubSubManager.connectionsSize];
            memset(newConnection, 0, sizeof(UA_PubSubConnection));
            LIST_INIT(&newConnection->writerGroups);
            //workaround - fixing issue with queue.h and realloc.
            for(size_t n = 0; n < server->pubSubManager.connectionsSize; n++){
                if(server->pubSubManager.connections[n].writerGroups.lh_first){
                    server->pubSubManager.connections[n].writerGroups.lh_first->listEntry.le_prev = &server->pubSubManager.connections[n].writerGroups.lh_first;
                }
            }
            newConnection->config = tmpConnectionConfig;
            newConnection->channel = server->config.pubsubTransportLayers[i].createPubSubChannel(newConnection->config);
            if(!newConnection->channel){
                UA_PubSubConnection_deleteMembers(server, newConnection);
                if(server->pubSubManager.connectionsSize > 0){
                    newConnectionsField = (UA_PubSubConnection *)
                            UA_realloc(server->pubSubManager.connections,
                                       sizeof(UA_PubSubConnection) * (server->pubSubManager.connectionsSize));
                    if(!newConnectionsField) {
                        return UA_STATUSCODE_BADINTERNALERROR;
                    }
                    server->pubSubManager.connections = newConnectionsField;
                } else  {
                    UA_free(newConnectionsField);
                    server->pubSubManager.connections = NULL;
                }
                UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                             "PubSub Connection creation failed. Transport layer creation problem.");
                return UA_STATUSCODE_BADINTERNALERROR;
            }
            UA_PubSubManager_generateUniqueNodeId(server, &newConnection->identifier);
            if(connectionIdentifier != NULL){
                UA_NodeId_copy(&newConnection->identifier, connectionIdentifier);
            }
            server->pubSubManager.connectionsSize++;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
        addPubSubConnectionRepresentation(server, newConnection);
#endif
            return UA_STATUSCODE_GOOD;
        }
    }
    UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                 "PubSub Connection creation failed. Requested transport layer not found.");
    return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_removePubSubConnection(UA_Server *server, const UA_NodeId connection) {
    //search the identified Connection and store the Connection index
    size_t connectionIndex;
    UA_PubSubConnection *currentConnection = NULL;
    for(connectionIndex = 0; connectionIndex < server->pubSubManager.connectionsSize; connectionIndex++){
        if(UA_NodeId_equal(&connection, &server->pubSubManager.connections[connectionIndex].identifier)){
            currentConnection = &server->pubSubManager.connections[connectionIndex];
            break;
        }
    }
    if(!currentConnection)
        return UA_STATUSCODE_BADNOTFOUND;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removePubSubConnectionRepresentation(server, currentConnection);
#endif
    UA_PubSubConnection_deleteMembers(server, currentConnection);
    server->pubSubManager.connectionsSize--;
    //remove the connection from the pubSubManager, move the last connection
    //into the allocated memory of the deleted connection
    if(server->pubSubManager.connectionsSize != connectionIndex){
        memcpy(&server->pubSubManager.connections[connectionIndex],
               &server->pubSubManager.connections[server->pubSubManager.connectionsSize],
               sizeof(UA_PubSubConnection));
    }

    if(server->pubSubManager.connectionsSize <= 0){
        UA_free(server->pubSubManager.connections);
        server->pubSubManager.connections = NULL;
    } else {
        server->pubSubManager.connections = (UA_PubSubConnection *)
                UA_realloc(server->pubSubManager.connections, sizeof(UA_PubSubConnection) * server->pubSubManager.connectionsSize);
        if(!server->pubSubManager.connections){
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        //workaround - fixing issue with queue.h and realloc.
        for(size_t n = 0; n < server->pubSubManager.connectionsSize; n++){
            if(server->pubSubManager.connections[n].writerGroups.lh_first){
                server->pubSubManager.connections[n].writerGroups.lh_first->listEntry.le_prev = &server->pubSubManager.connections[n].writerGroups.lh_first;
            }
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_AddPublishedDataSetResult
UA_Server_addPublishedDataSet(UA_Server *server, const UA_PublishedDataSetConfig *publishedDataSetConfig,
                              UA_NodeId *pdsIdentifier) {
    if(!publishedDataSetConfig){
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. No config passed in.");
        return (UA_AddPublishedDataSetResult) {UA_STATUSCODE_BADINVALIDARGUMENT, 0, NULL, {0, 0}};
    }
    if(publishedDataSetConfig->publishedDataSetType != UA_PUBSUB_DATASET_PUBLISHEDITEMS){
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Unsupported PublishedDataSet type.");
        return (UA_AddPublishedDataSetResult) {UA_STATUSCODE_BADINVALIDARGUMENT, 0, NULL, {0, 0}};
    }
    //deep copy the given connection config
    UA_PublishedDataSetConfig tmpPublishedDataSetConfig;
    memset(&tmpPublishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    if(UA_PublishedDataSetConfig_copy(publishedDataSetConfig, &tmpPublishedDataSetConfig) != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Configuration copy failed.");
        return (UA_AddPublishedDataSetResult) {UA_STATUSCODE_BADINTERNALERROR, 0, NULL, {0, 0}};
    }
    //create new PDS and add to UA_PubSubManager
    UA_PublishedDataSet *newPubSubDataSetField = (UA_PublishedDataSet *)
            UA_realloc(server->pubSubManager.publishedDataSets,
                       sizeof(UA_PublishedDataSet) * (server->pubSubManager.publishedDataSetsSize + 1));
    if(!newPubSubDataSetField) {
        UA_PublishedDataSetConfig_deleteMembers(&tmpPublishedDataSetConfig);
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PublishedDataSet creation failed. Out of Memory.");
        return (UA_AddPublishedDataSetResult) {UA_STATUSCODE_BADOUTOFMEMORY, 0, NULL, {0, 0}};
    }
    server->pubSubManager.publishedDataSets = newPubSubDataSetField;
    UA_PublishedDataSet *newPubSubDataSet = &server->pubSubManager.publishedDataSets[server->pubSubManager.publishedDataSetsSize];
    memset(newPubSubDataSet, 0, sizeof(UA_PublishedDataSet));
    LIST_INIT(&newPubSubDataSet->fields);
    //workaround - fixing issue with queue.h and realloc.
    for(size_t n = 0; n < server->pubSubManager.publishedDataSetsSize; n++){
        if(server->pubSubManager.publishedDataSets[n].fields.lh_first){
            server->pubSubManager.publishedDataSets[n].fields.lh_first->listEntry.le_prev = &server->pubSubManager.publishedDataSets[n].fields.lh_first;
        }
    }
    newPubSubDataSet->config = tmpPublishedDataSetConfig;
    if(tmpPublishedDataSetConfig.publishedDataSetType == UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE){
        //parse template config and add fields (later PubSub batch)
    }
    //generate unique nodeId
    UA_PubSubManager_generateUniqueNodeId(server, &newPubSubDataSet->identifier);
    if(pdsIdentifier != NULL){
        UA_NodeId_copy(&newPubSubDataSet->identifier, pdsIdentifier);
    }
    server->pubSubManager.publishedDataSetsSize++;
    UA_AddPublishedDataSetResult result = {UA_STATUSCODE_GOOD, 0, NULL,
                                                 {UA_PubSubConfigurationVersionTimeDifference(),
                                                  UA_PubSubConfigurationVersionTimeDifference()}};
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addPublishedDataItemsRepresentation(server, newPubSubDataSet);
#endif
    return result;
}

UA_StatusCode
UA_Server_removePublishedDataSet(UA_Server *server, const UA_NodeId pds) {
    //search the identified PublishedDataSet and store the PDS index
    UA_PublishedDataSet *publishedDataSet = NULL;
    size_t publishedDataSetIndex;
    for(publishedDataSetIndex = 0; publishedDataSetIndex < server->pubSubManager.publishedDataSetsSize; publishedDataSetIndex++){
        if(UA_NodeId_equal(&server->pubSubManager.publishedDataSets[publishedDataSetIndex].identifier, &pds)){
            publishedDataSet = &server->pubSubManager.publishedDataSets[publishedDataSetIndex];
            break;
        };
    }
    if(!publishedDataSet){
        return UA_STATUSCODE_BADNOTFOUND;
    }
    //search for referenced writers -> delete this writers. (Standard: writer must be connected with PDS)
    for(size_t i = 0; i < server->pubSubManager.connectionsSize; i++){
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &server->pubSubManager.connections[i].writerGroups, listEntry){
            UA_DataSetWriter *currentWriter, *tmpWriterGroup;
            LIST_FOREACH_SAFE(currentWriter, &writerGroup->writers, listEntry, tmpWriterGroup){
                if(UA_NodeId_equal(&currentWriter->identifier, &publishedDataSet->identifier)){
                    UA_Server_removeDataSetWriter(server, currentWriter->identifier);
                }
            }
        }
    }
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removePublishedDataSetRepresentation(server, publishedDataSet);
#endif
    UA_PublishedDataSet_deleteMembers(server, publishedDataSet);
    server->pubSubManager.publishedDataSetsSize--;
    //copy the last PDS to the removed PDS inside the allocated memory block
    if(server->pubSubManager.publishedDataSetsSize != publishedDataSetIndex){
        memcpy(&server->pubSubManager.publishedDataSets[publishedDataSetIndex],
               &server->pubSubManager.publishedDataSets[server->pubSubManager.publishedDataSetsSize],
               sizeof(UA_PublishedDataSet));
    }
    if(server->pubSubManager.publishedDataSetsSize <= 0){
        UA_free(server->pubSubManager.publishedDataSets);
        server->pubSubManager.publishedDataSets = NULL;
    } else {
        server->pubSubManager.publishedDataSets = (UA_PublishedDataSet *)
                UA_realloc(server->pubSubManager.publishedDataSets, sizeof(UA_PublishedDataSet) * server->pubSubManager.publishedDataSetsSize);
        if(!server->pubSubManager.publishedDataSets){
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        //workaround - fixing issue with queue.h and realloc.
        for(size_t n = 0; n < server->pubSubManager.publishedDataSetsSize; n++){
            if(server->pubSubManager.publishedDataSets[n].fields.lh_first){
                server->pubSubManager.publishedDataSets[n].fields.lh_first->listEntry.le_prev = &server->pubSubManager.publishedDataSets[n].fields.lh_first;
            }
        }
    }
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
    UA_Node *newNode = UA_Nodestore_new(server, UA_NODECLASS_OBJECT);
    UA_Nodestore_insert(server, newNode, &newNodeId);
    UA_NodeId_copy(&newNodeId, nodeId);
}

/* Delete the current PubSub configuration including all nested members. This
 * action also delete the configured PubSub transport Layers. */
void
UA_PubSubManager_delete(UA_Server *server, UA_PubSubManager *pubSubManager) {
    UA_LOG_INFO(server->config.logger, UA_LOGCATEGORY_SERVER, "PubSub cleanup was called.");
    //remove Connections and WriterGroups
    while(pubSubManager->connectionsSize > 0){
        UA_Server_removePubSubConnection(server, pubSubManager->connections[pubSubManager->connectionsSize-1].identifier);
    }
    while(pubSubManager->publishedDataSetsSize > 0){
        UA_Server_removePublishedDataSet(server, pubSubManager->publishedDataSets[pubSubManager->publishedDataSetsSize-1].identifier);
    }
    //free the currently configured transport layers
    for(size_t i = 0; i < server->config.pubsubTransportLayersSize; i++){
        UA_free(&server->config.pubsubTransportLayers[i]);
    }
}

/***********************************/
/*      PubSub Jobs abstraction    */
/***********************************/
UA_StatusCode
UA_PubSubManager_addRepeatedCallback(UA_Server *server, UA_ServerCallback callback,
                                     void *data, UA_UInt32 interval, UA_UInt64 *callbackId) {
    return UA_Timer_addRepeatedCallback(&server->timer, (UA_TimerCallback)callback,
                                        data, interval, callbackId);
}

UA_StatusCode
UA_PubSubManager_changeRepeatedCallbackInterval(UA_Server *server, UA_UInt64 callbackId,
                                                UA_UInt32 interval) {
    return UA_Timer_changeRepeatedCallbackInterval(&server->timer, callbackId, interval);
}

UA_StatusCode
UA_PubSubManager_removeRepeatedPubSubCallback(UA_Server *server, UA_UInt64 callbackId) {
    return UA_Timer_removeRepeatedCallback(&server->timer, callbackId);
}

#endif /* UA_ENABLE_PUBSUB */
