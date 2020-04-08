/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2018 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 */

#include <open62541/server_pubsub.h>
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#ifdef UA_ENABLE_PUBSUB_DELTAFRAMES
#include "ua_types_encoding_binary.h"
#endif

#define UA_MAX_SIZENAME 64  /* Max size of Qualified Name of Subscribed Variable */

/***************/
/* ReaderGroup */
/***************/

UA_StatusCode
UA_Server_addReaderGroup(UA_Server *server, UA_NodeId connectionIdentifier,
                         const UA_ReaderGroupConfig *readerGroupConfig,
                         UA_NodeId *readerGroupIdentifier) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Check for valid readergroup configuration */
    if(!readerGroupConfig) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *currentConnectionContext =
        UA_PubSubConnection_findConnectionbyId(server, connectionIdentifier);
    if(!currentConnectionContext) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Allocate memory for new reader group */
    UA_ReaderGroup *newGroup = (UA_ReaderGroup *)UA_calloc(1, sizeof(UA_ReaderGroup));
    if(!newGroup) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Generate nodeid for the readergroup identifier */
    newGroup->linkedConnection = currentConnectionContext->identifier;
    UA_PubSubManager_generateUniqueNodeId(server, &newGroup->identifier);
    if(readerGroupIdentifier) {
        UA_NodeId_copy(&newGroup->identifier, readerGroupIdentifier);
    }

    /* Deep copy of the config */
    retval |= UA_ReaderGroupConfig_copy(readerGroupConfig, &newGroup->config);
    retval |= UA_ReaderGroup_addSubscribeCallback(server, newGroup);
    LIST_INSERT_HEAD(&currentConnectionContext->readerGroups, newGroup, listEntry);
    currentConnectionContext->readerGroupsSize++;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addReaderGroupRepresentation(server, newGroup);
#endif

    return retval;
}

UA_StatusCode
UA_Server_removeReaderGroup(UA_Server *server, UA_NodeId groupIdentifier) {
    UA_ReaderGroup* readerGroup = UA_ReaderGroup_findRGbyId(server, groupIdentifier);
    if(readerGroup == NULL) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Search the connection to which the given readergroup is connected to */
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, readerGroup->linkedConnection);
    if(connection == NULL) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Unregister subscribe callback */
    UA_PubSubManager_removeRepeatedPubSubCallback(server, readerGroup->subscribeCallbackId);
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* To Do:RemoveGroupRepresentation(server, &readerGroup->identifier) */
#endif

    /* UA_Server_ReaderGroup_delete also removes itself from the list */
    UA_Server_ReaderGroup_delete(server, readerGroup);
    /* Remove readerGroup from Connection */
    LIST_REMOVE(readerGroup, listEntry);
    UA_free(readerGroup);
    return UA_STATUSCODE_GOOD;
}

/* TODO: Implement
UA_StatusCode
UA_Server_ReaderGroup_updateConfig(UA_Server *server, UA_NodeId readerGroupIdentifier,
                                   const UA_ReaderGroupConfig *config) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}
*/

UA_StatusCode
UA_Server_ReaderGroup_getConfig(UA_Server *server, UA_NodeId readerGroupIdentifier,
                                UA_ReaderGroupConfig *config) {
    if(!config) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Identify the readergroup through the readerGroupIdentifier */
    UA_ReaderGroup *currentReaderGroup = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(!currentReaderGroup) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_ReaderGroupConfig tmpReaderGroupConfig;
    /* deep copy of the actual config */
    UA_ReaderGroupConfig_copy(&currentReaderGroup->config, &tmpReaderGroupConfig);
    *config = tmpReaderGroupConfig;
    return UA_STATUSCODE_GOOD;
}

void
UA_Server_ReaderGroup_delete(UA_Server* server, UA_ReaderGroup *readerGroup) {
    /* To Do Call UA_ReaderGroupConfig_delete */
    UA_DataSetReader *dataSetReader, *tmpDataSetReader;
    LIST_FOREACH_SAFE(dataSetReader, &readerGroup->readers, listEntry, tmpDataSetReader) {
        UA_DataSetReader_delete(server, dataSetReader);
    }
    UA_PubSubConnection* pConn =
        UA_PubSubConnection_findConnectionbyId(server, readerGroup->linkedConnection);
    if(pConn != NULL) {
        pConn->readerGroupsSize--;
    }

    /* Delete ReaderGroup and its members */
    UA_String_deleteMembers(&readerGroup->config.name);
    UA_NodeId_deleteMembers(&readerGroup->linkedConnection);
    UA_NodeId_deleteMembers(&readerGroup->identifier);
}

UA_StatusCode
UA_ReaderGroupConfig_copy(const UA_ReaderGroupConfig *src,
                          UA_ReaderGroupConfig *dst) {
    /* Currently simple memcpy only */
    memcpy(&dst->securityParameters, &src->securityParameters, sizeof(UA_PubSubSecurityParameters));
    UA_String_copy(&src->name, &dst->name);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
checkReaderIdentifier(UA_Server *server, UA_NetworkMessage *pMsg, UA_DataSetReader *reader) {
    if(!pMsg->groupHeaderEnabled &&
       !pMsg->groupHeader.writerGroupIdEnabled &&
       !pMsg->payloadHeaderEnabled) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Cannot process DataSetReader without WriterGroup"
                    "and DataSetWriter identifiers");
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }

    if((reader->config.writerGroupId == pMsg->groupHeader.writerGroupId) &&
       (reader->config.dataSetWriterId == *pMsg->payloadHeader.dataSetPayloadHeader.dataSetWriterIds)) {
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "DataSetReader found. Process NetworkMessage");
        return UA_STATUSCODE_GOOD;
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_StatusCode
getReaderFromIdentifier(UA_Server *server, UA_NetworkMessage *pMsg,
                        UA_DataSetReader **dataSetReader, UA_PubSubConnection *pConnection) {
    UA_StatusCode retval = UA_STATUSCODE_BADNOTFOUND;
    if(!pMsg->publisherIdEnabled) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "Cannot process DataSetReader without PublisherId");
        return UA_STATUSCODE_BADNOTIMPLEMENTED; /* TODO: Handle DSR without PublisherId */
    }

    UA_ReaderGroup* readerGroup;
    LIST_FOREACH(readerGroup, &pConnection->readerGroups, listEntry) {
        UA_DataSetReader *tmpReader;
        LIST_FOREACH(tmpReader, &readerGroup->readers, listEntry) {
            switch (pMsg->publisherIdType) {
            case UA_PUBLISHERDATATYPE_BYTE:
                if(tmpReader->config.publisherId.type == &UA_TYPES[UA_TYPES_BYTE] &&
                   pMsg->publisherIdType == UA_PUBLISHERDATATYPE_BYTE &&
                   pMsg->publisherId.publisherIdByte == *(UA_Byte*)tmpReader->config.publisherId.data) {
                    retval = checkReaderIdentifier(server, pMsg, tmpReader);
                }
                break;
            case UA_PUBLISHERDATATYPE_UINT16:
                if(tmpReader->config.publisherId.type == &UA_TYPES[UA_TYPES_UINT16] &&
                   pMsg->publisherIdType == UA_PUBLISHERDATATYPE_UINT16 &&
                   pMsg->publisherId.publisherIdUInt16 == *(UA_UInt16*) tmpReader->config.publisherId.data) {
                    retval = checkReaderIdentifier(server, pMsg, tmpReader);
                }
                break;
            case UA_PUBLISHERDATATYPE_UINT32:
                if(tmpReader->config.publisherId.type == &UA_TYPES[UA_TYPES_UINT32] &&
                   pMsg->publisherIdType == UA_PUBLISHERDATATYPE_UINT32 &&
                   pMsg->publisherId.publisherIdUInt32 == *(UA_UInt32*)tmpReader->config.publisherId.data) {
                    retval = checkReaderIdentifier(server, pMsg, tmpReader);
                }
                break;
            case UA_PUBLISHERDATATYPE_UINT64:
                if(tmpReader->config.publisherId.type == &UA_TYPES[UA_TYPES_UINT64] &&
                   pMsg->publisherIdType == UA_PUBLISHERDATATYPE_UINT64 &&
                   pMsg->publisherId.publisherIdUInt64 == *(UA_UInt64*)tmpReader->config.publisherId.data) {
                    retval = checkReaderIdentifier(server, pMsg, tmpReader);
                }
                break;
            case UA_PUBLISHERDATATYPE_STRING:
                if(tmpReader->config.publisherId.type == &UA_TYPES[UA_TYPES_STRING] &&
                   pMsg->publisherIdType == UA_PUBLISHERDATATYPE_STRING &&
                   UA_String_equal(&pMsg->publisherId.publisherIdString,
                                   (UA_String*)tmpReader->config.publisherId.data)) {
                    retval = checkReaderIdentifier(server, pMsg, tmpReader);
                }
                break;
            default:
                return UA_STATUSCODE_BADINTERNALERROR;
            }

            if(retval == UA_STATUSCODE_GOOD) {
                *dataSetReader = tmpReader;
                return UA_STATUSCODE_GOOD;
            }
        }
    }

    UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                "Dataset reader not found. Check PublisherID, WriterGroupID and DatasetWriterID");
    return UA_STATUSCODE_BADNOTFOUND;
}

UA_ReaderGroup *
UA_ReaderGroup_findRGbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PubSubConnection *pubSubConnection;
    TAILQ_FOREACH(pubSubConnection, &server->pubSubManager.connections, listEntry){
        UA_ReaderGroup* readerGroup = NULL;
        LIST_FOREACH(readerGroup, &pubSubConnection->readerGroups, listEntry) {
            if(UA_NodeId_equal(&identifier, &readerGroup->identifier)) {
                return readerGroup;
            }

        }
    }
    return NULL;
}

UA_DataSetReader *UA_ReaderGroup_findDSRbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PubSubConnection *pubSubConnection;
    TAILQ_FOREACH(pubSubConnection, &server->pubSubManager.connections, listEntry){
        UA_ReaderGroup* readerGroup = NULL;
        LIST_FOREACH(readerGroup, &pubSubConnection->readerGroups, listEntry) {
            UA_DataSetReader *tmpReader;
            LIST_FOREACH(tmpReader, &readerGroup->readers, listEntry) {
                if(UA_NodeId_equal(&tmpReader->identifier, &identifier)) {
                    return tmpReader;
                }
            }
        }
    }
    return NULL;
}

/* This callback triggers the collection and reception of NetworkMessages and the
 * contained DataSetMessages. */
void UA_ReaderGroup_subscribeCallback(UA_Server *server, UA_ReaderGroup *readerGroup) {
    UA_PubSubConnection *connection =
        UA_PubSubConnection_findConnectionbyId(server, readerGroup->linkedConnection);
    UA_ByteString buffer;
    if(UA_ByteString_allocBuffer(&buffer, 512) != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Message buffer alloc failed!");
        return;
    }

    connection->channel->receive(connection->channel, &buffer, NULL, 1000);
    if(buffer.length > 0) {
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_USERLAND, "Message received:");
        UA_NetworkMessage currentNetworkMessage;
        memset(&currentNetworkMessage, 0, sizeof(UA_NetworkMessage));
        size_t currentPosition = 0;
        UA_NetworkMessage_decodeBinary(&buffer, &currentPosition, &currentNetworkMessage);
        UA_Server_processNetworkMessage(server, &currentNetworkMessage, connection);
        UA_NetworkMessage_deleteMembers(&currentNetworkMessage);
    }

    UA_ByteString_deleteMembers(&buffer);
}

/* Add new subscribeCallback. The first execution is triggered directly after
 * creation. */
UA_StatusCode
UA_ReaderGroup_addSubscribeCallback(UA_Server *server, UA_ReaderGroup *readerGroup) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_PubSubManager_addRepeatedCallback(server,
                                                   (UA_ServerCallback) UA_ReaderGroup_subscribeCallback,
                                                   readerGroup, 5, &readerGroup->subscribeCallbackId);

    if(retval == UA_STATUSCODE_GOOD) {
        readerGroup->subscribeCallbackIsRegistered = true;
    }

    /* Run once after creation */
    UA_ReaderGroup_subscribeCallback(server, readerGroup);
    return retval;
}

/**********/
/* Reader */
/**********/

UA_StatusCode
UA_Server_addDataSetReader(UA_Server *server, UA_NodeId readerGroupIdentifier,
                                      const UA_DataSetReaderConfig *dataSetReaderConfig,
                                      UA_NodeId *readerIdentifier) {
    /* Search the reader group by the given readerGroupIdentifier */
    UA_ReaderGroup *readerGroup = UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);

    if(!dataSetReaderConfig) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    if(readerGroup == NULL) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Allocate memory for new DataSetReader */
    UA_DataSetReader *newDataSetReader = (UA_DataSetReader *)UA_calloc(1, sizeof(UA_DataSetReader));
    /* Copy the config into the new dataSetReader */
    UA_DataSetReaderConfig_copy(dataSetReaderConfig, &newDataSetReader->config);
    newDataSetReader->linkedReaderGroup = readerGroup->identifier;
    UA_PubSubManager_generateUniqueNodeId(server, &newDataSetReader->identifier);
    if(readerIdentifier != NULL) {
        UA_NodeId_copy(&newDataSetReader->identifier, readerIdentifier);
    }

    /* Add the new reader to the group */
    LIST_INSERT_HEAD(&readerGroup->readers, newDataSetReader, listEntry);
    readerGroup->readersCount++;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addDataSetReaderRepresentation(server, newDataSetReader);
#endif

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeDataSetReader(UA_Server *server, UA_NodeId readerIdentifier) {
    /* Remove datasetreader given by the identifier */
    UA_DataSetReader *dataSetReader = UA_ReaderGroup_findDSRbyId(server, readerIdentifier);
    if(!dataSetReader) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeDataSetReaderRepresentation(server, dataSetReader);
#endif

    UA_DataSetReader_delete(server, dataSetReader);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_DataSetReader_updateConfig(UA_Server *server, UA_NodeId dataSetReaderIdentifier,
                                     UA_NodeId readerGroupIdentifier,
                                     const UA_DataSetReaderConfig *config) {
    if(config == NULL) {
       return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_DataSetReader *currentDataSetReader =
        UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    UA_ReaderGroup *currentReaderGroup =
        UA_ReaderGroup_findRGbyId(server, readerGroupIdentifier);
    if(!currentDataSetReader) {
       return UA_STATUSCODE_BADNOTFOUND;
    }

    /* The update functionality will be extended during the next PubSub batches.
     * Currently is only a change of the publishing interval possible. */
    if(currentDataSetReader->config.writerGroupId != config->writerGroupId) {
       UA_PubSubManager_removeRepeatedPubSubCallback(server, currentReaderGroup->subscribeCallbackId);
       currentDataSetReader->config.writerGroupId = config->writerGroupId;
       UA_ReaderGroup_subscribeCallback(server, currentReaderGroup);
    }
    else {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "No or unsupported ReaderGroup update.");
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_DataSetReader_getConfig(UA_Server *server, UA_NodeId dataSetReaderIdentifier,
                                 UA_DataSetReaderConfig *config) {
    if(!config) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_DataSetReader *currentDataSetReader =
        UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    if(!currentDataSetReader) {
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_DataSetReaderConfig tmpReaderConfig;
    /* Deep copy of the actual config */
    UA_DataSetReaderConfig_copy(&currentDataSetReader->config, &tmpReaderConfig);
    *config = tmpReaderConfig;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_DataSetReaderConfig_copy(const UA_DataSetReaderConfig *src,
                            UA_DataSetReaderConfig *dst) {
    memset(dst, 0, sizeof(UA_DataSetReaderConfig));
    UA_StatusCode retVal = UA_String_copy(&src->name, &dst->name);
    if(retVal != UA_STATUSCODE_GOOD) {
        return retVal;
    }

    retVal = UA_Variant_copy(&src->publisherId, &dst->publisherId);
    if(retVal != UA_STATUSCODE_GOOD) {
        return retVal;
    }

    dst->writerGroupId = src->writerGroupId;
    dst->dataSetWriterId = src->dataSetWriterId;
    retVal = UA_DataSetMetaDataType_copy(&src->dataSetMetaData, &dst->dataSetMetaData);
    if(retVal != UA_STATUSCODE_GOOD) {
        return retVal;
    }

    dst->dataSetFieldContentMask = src->dataSetFieldContentMask;
    dst->messageReceiveTimeout = src->messageReceiveTimeout;

    /* Currently memcpy is used to copy the securityParameters */
    memcpy(&dst->securityParameters, &src->securityParameters, sizeof(UA_PubSubSecurityParameters));
    retVal = UA_UadpDataSetReaderMessageDataType_copy(&src->messageSettings, &dst->messageSettings);
    if(retVal != UA_STATUSCODE_GOOD) {
        return retVal;
    }

    retVal = UA_ExtensionObject_copy(&src->transportSettings, &dst->transportSettings);
    if (retVal != UA_STATUSCODE_GOOD) {
    	return retVal;
    }

    return UA_STATUSCODE_GOOD;
}

/* This Method is used to initially set the SubscribedDataSet to
 * TargetVariablesType and to create the list of target Variables of a
 * SubscribedDataSetType. */
UA_StatusCode
UA_Server_DataSetReader_createTargetVariables(UA_Server *server,
                                              UA_NodeId dataSetReaderIdentifier,
                                              UA_TargetVariablesDataType *targetVariables) {
    UA_StatusCode retval = UA_STATUSCODE_BADUNEXPECTEDERROR;
    UA_DataSetReader* pDS = UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    if(pDS == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    if(pDS->subscribedDataSetTarget.targetVariablesSize > 0) {
        UA_TargetVariablesDataType_deleteMembers(&pDS->subscribedDataSetTarget);
        pDS->subscribedDataSetTarget.targetVariablesSize = 0;
        pDS->subscribedDataSetTarget.targetVariables = NULL;
    }

    /* Set subscribed dataset to TargetVariableType */
    pDS->subscribedDataSetType = UA_PUBSUB_SDS_TARGET;
    retval = UA_TargetVariablesDataType_copy(targetVariables, &pDS->subscribedDataSetTarget);
    return retval;
}

/* Adds Subscribed Variables from the DataSetMetaData for the given DataSet into
 * the given parent node and creates the corresponding data in the
 * targetVariables of the DataSetReader */
UA_StatusCode
UA_Server_DataSetReader_addTargetVariables(UA_Server *server, UA_NodeId *parentNode,
                                           UA_NodeId dataSetReaderIdentifier,
                                           UA_SubscribedDataSetEnumType sdsType) {
    if((server == NULL) || (parentNode == NULL)) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_DataSetReader* pDataSetReader = UA_ReaderGroup_findDSRbyId(server, dataSetReaderIdentifier);
    if(pDataSetReader == NULL) {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_TargetVariablesDataType targetVars;
    targetVars.targetVariablesSize = pDataSetReader->config.dataSetMetaData.fieldsSize;
    targetVars.targetVariables = (UA_FieldTargetDataType*)
        UA_calloc(targetVars.targetVariablesSize, sizeof(UA_FieldTargetDataType));

    for (size_t i = 0; i < pDataSetReader->config.dataSetMetaData.fieldsSize; i++) {
        UA_VariableAttributes vAttr = UA_VariableAttributes_default;
        vAttr.valueRank = pDataSetReader->config.dataSetMetaData.fields[i].valueRank;
        if(pDataSetReader->config.dataSetMetaData.fields[i].arrayDimensionsSize > 0) {
            retval = UA_Array_copy(pDataSetReader->config.dataSetMetaData.fields[i].arrayDimensions,
                                   pDataSetReader->config.dataSetMetaData.fields[i].arrayDimensionsSize,
                                   (void**)&vAttr.arrayDimensions, &UA_TYPES[UA_TYPES_UINT32]);
            if(retval == UA_STATUSCODE_GOOD) {
                vAttr.arrayDimensionsSize =
                    pDataSetReader->config.dataSetMetaData.fields[i].arrayDimensionsSize;
            }

        }

        vAttr.dataType = pDataSetReader->config.dataSetMetaData.fields[i].dataType;

        vAttr.accessLevel = UA_ACCESSLEVELMASK_READ;
        UA_LocalizedText_copy(&pDataSetReader->config.dataSetMetaData.fields[i].description,
                              &vAttr.description);
        UA_QualifiedName qn;
        UA_QualifiedName_init(&qn);
        char szTmpName[UA_MAX_SIZENAME];
        if(pDataSetReader->config.dataSetMetaData.fields[i].name.length > 0) {
            UA_UInt16 slen = UA_MAX_SIZENAME -1;
            vAttr.displayName.locale = UA_STRING("en-US");
            vAttr.displayName.text = pDataSetReader->config.dataSetMetaData.fields[i].name;
            if(pDataSetReader->config.dataSetMetaData.fields[i].name.length < slen) {
                slen = (UA_UInt16)pDataSetReader->config.dataSetMetaData.fields[i].name.length;
                UA_snprintf(szTmpName, sizeof(szTmpName), "%.*s", (int)slen,
                            (const char*)pDataSetReader->config.dataSetMetaData.fields[i].name.data);
            }

            szTmpName[slen] = '\0';
            qn = UA_QUALIFIEDNAME(1, szTmpName);
        }
        else {
            strcpy(szTmpName, "SubscribedVariable");
            vAttr.displayName = UA_LOCALIZEDTEXT("en-US", szTmpName);
            qn = UA_QUALIFIEDNAME(1, "SubscribedVariable");
        }

        /* Add variable to the given parent node */
        UA_NodeId newNode;
        retval = UA_Server_addVariableNode(server, UA_NODEID_NULL, *parentNode,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), qn,
                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           vAttr, NULL, &newNode);
        if(retval == UA_STATUSCODE_GOOD) {
            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                        "addVariableNode %s succeeded", szTmpName);
        }
        else {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                         "addVariableNode: error 0x%" PRIx32, retval);
        }

        UA_FieldTargetDataType_init(&targetVars.targetVariables[i]);
        targetVars.targetVariables[i].attributeId = UA_ATTRIBUTEID_VALUE;
        UA_NodeId_copy(&newNode, &targetVars.targetVariables[i].targetNodeId);
        UA_NodeId_deleteMembers(&newNode);
        if(vAttr.arrayDimensionsSize > 0) {
            UA_Array_delete(vAttr.arrayDimensions, vAttr.arrayDimensionsSize,
                            &UA_TYPES[UA_TYPES_UINT32]);
        }
    }

    if(sdsType == UA_PUBSUB_SDS_TARGET) {
        retval = UA_Server_DataSetReader_createTargetVariables(server, pDataSetReader->identifier,
                                                               &targetVars);
    }

    UA_TargetVariablesDataType_deleteMembers(&targetVars);
    return retval;
}

void
UA_Server_DataSetReader_process(UA_Server *server, UA_DataSetReader *dataSetReader,
                                UA_DataSetMessage* dataSetMsg) {
    if((dataSetReader == NULL) || (dataSetMsg == NULL) || (server == NULL)) {
        return;
    }

    if(!dataSetMsg->header.dataSetMessageValid) {
        UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER,
                    "DataSetMessage is discarded: message is not valid");
         /* To Do check ConfigurationVersion*/
         /*if(dataSetMsg->header.configVersionMajorVersionEnabled)
         * {
         * if(dataSetMsg->header.configVersionMajorVersion != dataSetReader->config.dataSetMetaData.configurationVersion.majorVersion)
         * {
         * UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_SERVER, "DataSetMessage is discarded: ConfigurationVersion MajorVersion does not match");
         * return;
         * }
         } */
        return;
    }

    if(dataSetMsg->header.dataSetMessageType == UA_DATASETMESSAGE_DATAKEYFRAME) {
        if(dataSetMsg->header.fieldEncoding != UA_FIELDENCODING_RAWDATA) {
            size_t anzFields = dataSetMsg->data.keyFrameData.fieldCount;
            if(dataSetReader->config.dataSetMetaData.fieldsSize < anzFields) {
                anzFields = dataSetReader->config.dataSetMetaData.fieldsSize;
            }

            if(dataSetReader->subscribedDataSetTarget.targetVariablesSize < anzFields) {
                anzFields = dataSetReader->subscribedDataSetTarget.targetVariablesSize;
            }

            UA_StatusCode retVal = UA_STATUSCODE_GOOD;
            for(UA_UInt16 i = 0; i < anzFields; i++) {
                if(dataSetMsg->data.keyFrameData.dataSetFields[i].hasValue) {
                    if(dataSetReader->subscribedDataSetTarget.targetVariables[i].attributeId == UA_ATTRIBUTEID_VALUE) {
                        retVal = UA_Server_writeValue(server, dataSetReader->subscribedDataSetTarget.targetVariables[i].targetNodeId, dataSetMsg->data.keyFrameData.dataSetFields[i].value);
                        if(retVal != UA_STATUSCODE_GOOD) {
                            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Error Write Value KF %" PRIu16 ": 0x%"PRIx32, i, retVal);
                        }

                    }
                    else {
                        UA_WriteValue writeVal;
                        UA_WriteValue_init(&writeVal);
                        writeVal.attributeId = dataSetReader->subscribedDataSetTarget.targetVariables[i].attributeId;
                        writeVal.indexRange = dataSetReader->subscribedDataSetTarget.targetVariables[i].receiverIndexRange;
                        writeVal.nodeId = dataSetReader->subscribedDataSetTarget.targetVariables[i].targetNodeId;
                        UA_DataValue_copy(&dataSetMsg->data.keyFrameData.dataSetFields[i], &writeVal.value);
                        retVal = UA_Server_write(server, &writeVal);
                        if(retVal != UA_STATUSCODE_GOOD) {
                            UA_LOG_INFO(&server->config.logger, UA_LOGCATEGORY_SERVER, "Error Write KF %" PRIu16 ": 0x%" PRIx32, i, retVal);
                        }

                    }

                }

            }

        }

    }
}

void UA_DataSetReader_delete(UA_Server *server, UA_DataSetReader *dataSetReader) {
    /* Delete DataSetReader config */
    UA_String_deleteMembers(&dataSetReader->config.name);
    UA_Variant_deleteMembers(&dataSetReader->config.publisherId);
    UA_DataSetMetaDataType_deleteMembers(&dataSetReader->config.dataSetMetaData);
    UA_UadpDataSetReaderMessageDataType_deleteMembers(&dataSetReader->config.messageSettings);
    UA_ExtensionObject_clear(&dataSetReader->config.transportSettings);
    UA_TargetVariablesDataType_deleteMembers(&dataSetReader->subscribedDataSetTarget);

    /* Delete DataSetReader */
    UA_ReaderGroup* pGroup = UA_ReaderGroup_findRGbyId(server, dataSetReader->linkedReaderGroup);
    if(pGroup != NULL) {
        pGroup->readersCount--;
    }

    UA_NodeId_deleteMembers(&dataSetReader->identifier);
    UA_NodeId_deleteMembers(&dataSetReader->linkedReaderGroup);
    /* Remove DataSetReader from group */
    LIST_REMOVE(dataSetReader, listEntry);
    /* Free memory allocated for DataSetReader */
    UA_free(dataSetReader);
}

UA_StatusCode
UA_Server_processNetworkMessage(UA_Server *server, UA_NetworkMessage *pMsg,
                                UA_PubSubConnection *pConnection) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(!pMsg || !pConnection)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* To Do Handle multiple DataSetMessage for one NetworkMessage */
    /* To Do The condition pMsg->dataSetClassIdEnabled
     * Here some filtering is possible */

    UA_DataSetReader *dataSetReader;
    retval = getReaderFromIdentifier(server, pMsg, &dataSetReader, pConnection);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }

    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                 "DataSetReader found with PublisherId");

    UA_Byte anzDataSets = 1;
    if(pMsg->payloadHeaderEnabled)
        anzDataSets = pMsg->payloadHeader.dataSetPayloadHeader.count;
    for(UA_Byte iterator = 0; iterator < anzDataSets; iterator++) {
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER, "Process Msg with DataSetReader!");
        UA_Server_DataSetReader_process(server, dataSetReader,
                                        &pMsg->payload.dataSetPayload.dataSetMessages[iterator]);
    }

    /* To Do Handle when dataSetReader parameters are null for publisherId
     * and zero for WriterGroupId and DataSetWriterId */
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_PUBSUB */
