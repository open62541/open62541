/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_server_pubsub.h"
#include "server/ua_server_internal.h"
#include "ua_pubsub.h"
#include "ua_pubsub_manager.h"

/**********************************************/
/*               Connection                   */
/**********************************************/
UA_StatusCode
UA_PubSubConnectionConfig_copy(const UA_PubSubConnectionConfig *src, UA_PubSubConnectionConfig *dst) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PubSubConnectionConfig));
    retVal |= UA_String_copy(&src->name, &dst->name);
    retVal |= UA_Variant_copy(&src->address, &dst->address);
    retVal |= UA_String_copy(&src->transportProfileUri, &dst->transportProfileUri);
    retVal |= UA_Variant_copy(&src->connectionTransportSettings, &dst->connectionTransportSettings);
    if(src->connectionPropertiesSize > 0){
        dst->connectionProperties = (UA_KeyValuePair *) UA_calloc(src->connectionPropertiesSize, sizeof(UA_KeyValuePair));
        if(!dst->connectionProperties){
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for(size_t i = 0; i < src->connectionPropertiesSize; i++){
            retVal |= UA_QualifiedName_copy(&src->connectionProperties[i].key, &dst->connectionProperties[i].key);
            retVal |= UA_Variant_copy(&src->connectionProperties[i].value, &dst->connectionProperties[i].value);
        }
    }
    return retVal;
}

/**
 * Get the current config of the Connection.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_PubSubConnection_getConfig(UA_Server *server, UA_NodeId connectionIdentifier,
                              UA_PubSubConnectionConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PubSubConnection *currentPubSubConnection = UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    if(!currentPubSubConnection)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_PubSubConnectionConfig tmpPubSubConnectionConfig;
    //deep copy of the actual config
    UA_PubSubConnectionConfig_copy(currentPubSubConnection->config, &tmpPubSubConnectionConfig);
    *config = tmpPubSubConnectionConfig;
    return UA_STATUSCODE_GOOD;
}

/**
 * Find a Connection by the connectionIdentifier.
 *
 * @return ptr to Connection or NULL if not found
 */
UA_PubSubConnection *
UA_PubSubConnection_findConnectionbyId(UA_Server *server, UA_NodeId connectionIdentifier) {
    for(size_t i = 0; i < server->pubSubManager.connectionsSize; i++){
        if(UA_NodeId_equal(&connectionIdentifier, &server->pubSubManager.connections[i].identifier)){
            return &server->pubSubManager.connections[i];
        }
    }
    return NULL;
}

void
UA_PubSubConnectionConfig_deleteMembers(UA_PubSubConnectionConfig *connectionConfig) {
    UA_String_deleteMembers(&connectionConfig->name);
    UA_String_deleteMembers(&connectionConfig->transportProfileUri);
    UA_Variant_deleteMembers(&connectionConfig->connectionTransportSettings);
    UA_Variant_deleteMembers(&connectionConfig->address);
    for(size_t i = 0; i < connectionConfig->connectionPropertiesSize; i++){
        UA_QualifiedName_deleteMembers(&connectionConfig->connectionProperties[i].key);
        UA_Variant_deleteMembers(&connectionConfig->connectionProperties[i].value);
    }
    UA_free(connectionConfig->connectionProperties);
}

void
UA_PubSubConnection_delete(UA_PubSubConnection *connection) {
    //delete connection config
    UA_PubSubConnectionConfig_deleteMembers(connection->config);
    UA_NodeId_deleteMembers(&connection->identifier);
    if(connection->channel){
        connection->channel->close(connection->channel);
    }
    UA_free(connection->config);
}

/**********************************************/
/*               PublishedDataSet             */
/**********************************************/

UA_StatusCode
UA_PublishedDataSetConfig_copy(const UA_PublishedDataSetConfig *src,
                               UA_PublishedDataSetConfig *dst) {
    UA_StatusCode retVal = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_PublishedDataSetConfig));
    retVal |= UA_String_copy(&src->name, &dst->name);
    switch(src->publishedDataSetType){
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
            //no additional items
            break;
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
            if (src->config.itemsTemplate.variablesToAddSize > 0){
                dst->config.itemsTemplate.variablesToAdd = (UA_PublishedVariableDataType *) UA_calloc(
                        src->config.itemsTemplate.variablesToAddSize, sizeof(UA_PublishedVariableDataType));
            }
            for(size_t i = 0; i < src->config.itemsTemplate.variablesToAddSize; i++){
                retVal |= UA_PublishedVariableDataType_copy(&src->config.itemsTemplate.variablesToAdd[i], &dst->config.itemsTemplate.variablesToAdd[i]);
            }
            retVal |= UA_DataSetMetaDataType_copy(&src->config.itemsTemplate.metaData, &dst->config.itemsTemplate.metaData);
            break;
        default:
            return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    return retVal;
}

/**
 * Get the current config of the PublishedDataSetField.
 *
 * @return UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_PublishedDataSet_getConfig(UA_Server *server, UA_NodeId publishedDataSetIdentifier,
                              UA_PublishedDataSetConfig *config){
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_PublishedDataSet *currentPublishedDataSet = UA_PublishedDataSet_findPDSbyId(server,
                                                                                         publishedDataSetIdentifier);
    if(!currentPublishedDataSet)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_PublishedDataSetConfig tmpPublishedDataSetConfig;
    //deep copy of the actual config
    UA_PublishedDataSetConfig_copy(&currentPublishedDataSet->config, &tmpPublishedDataSetConfig);
    *config = tmpPublishedDataSetConfig;
    return UA_STATUSCODE_GOOD;
}

/**
 * Get PDS by the dataSetIdentifier.
 *
 * @return ptr to PDS or NULL if not found
 */
UA_PublishedDataSet *
UA_PublishedDataSet_findPDSbyId(UA_Server *server, UA_NodeId identifier){
    for(size_t i = 0; i < server->pubSubManager.publishedDataSetsSize; i++){
        if(UA_NodeId_equal(&server->pubSubManager.publishedDataSets[i].identifier, &identifier)){
            return &server->pubSubManager.publishedDataSets[i];
        }
    }
    return NULL;
}

void UA_PublishedDataSetConfig_deleteMembers(UA_PublishedDataSetConfig *pdsConfig){
    //delete pds config
    UA_String_deleteMembers(&pdsConfig->name);
    switch (pdsConfig->publishedDataSetType){
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS:
            //no additional items
            break;
        case UA_PUBSUB_DATASET_PUBLISHEDITEMS_TEMPLATE:
            if (pdsConfig->config.itemsTemplate.variablesToAddSize > 0){
                for(size_t i = 0; i < pdsConfig->config.itemsTemplate.variablesToAddSize; i++){
                    UA_PublishedVariableDataType_deleteMembers(&pdsConfig->config.itemsTemplate.variablesToAdd[i]);
                }
                UA_free(pdsConfig->config.itemsTemplate.variablesToAdd);
            }
            UA_DataSetMetaDataType_deleteMembers(&pdsConfig->config.itemsTemplate.metaData);
            break;
        default:
            break;
    }
}

void UA_PublishedDataSet_delete(UA_PublishedDataSet *publishedDataSet){
    UA_PublishedDataSetConfig_deleteMembers(&publishedDataSet->config);
    //delete PDS
    UA_DataSetMetaDataType_deleteMembers(&publishedDataSet->dataSetMetaData);
}
