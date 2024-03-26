/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2019 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2019 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2019 Kalycito Infotech Private Limited
 * Copyright (c) 2020 Yannick Wallerer, Siemens AG
 * Copyright (c) 2020 Thomas Fischer, Siemens AG
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 */

#include <open62541/server_pubsub.h>
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#include "ua_pubsub.h"
#include "ua_pubsub_networkmessage.h"

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
#include "ua_pubsub_ns0.h"
#endif

#define UA_MAX_STACKBUF 128 /* Max size of network messages on the stack */

static void
UA_WriterGroup_clear(UA_Server *server, UA_WriterGroup *writerGroup);

static UA_StatusCode
generateNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                       UA_ExtensionObject *messageSettings,
                       UA_ExtensionObject *transportSettings,
                       UA_NetworkMessage *networkMessage);

UA_StatusCode
UA_Server_addWriterGroup(UA_Server *server, const UA_NodeId connection,
                         const UA_WriterGroupConfig *writerGroupConfig,
                         UA_NodeId *writerGroupIdentifier) {
    if(!writerGroupConfig)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Search the connection by the given connectionIdentifier */
    UA_PubSubConnection *currentConnectionContext =
        UA_PubSubConnection_findConnectionbyId(server, connection);
    if(!currentConnectionContext)
        return UA_STATUSCODE_BADNOTFOUND;

    if(currentConnectionContext->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Adding WriterGroup failed. PubSubConnection is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    /* Validate messageSettings type */
    const UA_ExtensionObject *ms = &writerGroupConfig->messageSettings;
    if(ms->content.decoded.type) {
        if(writerGroupConfig->encodingMimeType == UA_PUBSUB_ENCODING_JSON &&
           (ms->encoding != UA_EXTENSIONOBJECT_DECODED ||
            ms->content.decoded.type != &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE])) {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }

        if(writerGroupConfig->encodingMimeType == UA_PUBSUB_ENCODING_UADP &&
           (ms->encoding != UA_EXTENSIONOBJECT_DECODED ||
            ms->content.decoded.type != &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])) {
            return UA_STATUSCODE_BADTYPEMISMATCH;
        }
    }

    /* Allocate new WriterGroup */
    UA_WriterGroup *newWriterGroup = (UA_WriterGroup*)UA_calloc(1, sizeof(UA_WriterGroup));
    if(!newWriterGroup)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    newWriterGroup->componentType = UA_PUBSUB_COMPONENT_WRITERGROUP;
    newWriterGroup->linkedConnection = currentConnectionContext;

    /* Deep copy of the config */
    UA_WriterGroupConfig *newConfig = &newWriterGroup->config;
    UA_StatusCode res = UA_WriterGroupConfig_copy(writerGroupConfig, newConfig);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(newWriterGroup);
        return res;
    }

    /* Create the datatype value if not present */
    if(!newConfig->messageSettings.content.decoded.type) {
        UA_UadpWriterGroupMessageDataType *wgm = UA_UadpWriterGroupMessageDataType_new();
        newConfig->messageSettings.content.decoded.data = wgm;
        newConfig->messageSettings.content.decoded.type =
            &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        newConfig->messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
    }

    /* Attach to the connection */
    LIST_INSERT_HEAD(&currentConnectionContext->writerGroups, newWriterGroup, listEntry);
    currentConnectionContext->writerGroupsSize++;

    /* Add representation / create unique identifier */
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    res = addWriterGroupRepresentation(server, newWriterGroup);
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager,
                                          &newWriterGroup->identifier);
#endif
    if(writerGroupIdentifier)
        UA_NodeId_copy(&newWriterGroup->identifier, writerGroupIdentifier);
    return res;
}

UA_StatusCode
UA_Server_removeWriterGroup(UA_Server *server, const UA_NodeId writerGroup) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    if(wg->configurationFrozen) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Delete WriterGroup failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_PubSubConnection *connection = wg->linkedConnection;
    if(!connection)
        return UA_STATUSCODE_BADNOTFOUND;

    if(connection->configurationFrozen) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Delete WriterGroup failed. PubSubConnection is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    if(wg->state == UA_PUBSUBSTATE_OPERATIONAL) {
        /* Unregister the publish callback */
        if(wg->config.pubsubManagerCallback.removeCustomCallback)
            wg->config.pubsubManagerCallback.
                removeCustomCallback(server, wg->identifier, wg->publishCallbackId);
        else
            UA_PubSubManager_removeRepeatedPubSubCallback(server, wg->publishCallbackId);

    }

    UA_DataSetWriter *dsw, *dsw_tmp;
    LIST_FOREACH_SAFE(dsw, &wg->writers, listEntry, dsw_tmp) {
        UA_DataSetWriter_remove(server, wg, dsw);
    }

    connection->writerGroupsSize--;
#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    removeGroupRepresentation(server, wg);
#endif

    UA_WriterGroup_clear(server, wg);
    LIST_REMOVE(wg, listEntry);
    UA_free(wg);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_freezeWriterGroupConfiguration(UA_Server *server,
                                         const UA_NodeId writerGroup) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    /* PubSubConnection freezeCounter++ */
    UA_PubSubConnection *pubSubConnection =  wg->linkedConnection;
    pubSubConnection->configurationFreezeCounter++;
    pubSubConnection->configurationFrozen = true;

    /* WriterGroup freeze */
    wg->configurationFrozen = true;

    /* DataSetWriter freeze */
    UA_DataSetWriter *dataSetWriter;
    LIST_FOREACH(dataSetWriter, &wg->writers, listEntry) {
        dataSetWriter->configurationFrozen = true;
        /* PublishedDataSet freezeCounter++ */
        UA_PublishedDataSet *publishedDataSet =
            UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
        publishedDataSet->configurationFreezeCounter++;
        publishedDataSet->configurationFrozen = true;
        /* DataSetFields freeze */
        UA_DataSetField *dataSetField;
        TAILQ_FOREACH(dataSetField, &publishedDataSet->fields, listEntry) {
            dataSetField->configurationFrozen = true;
        }
    }

    if(wg->config.rtLevel != UA_PUBSUB_RT_FIXED_SIZE)
        return UA_STATUSCODE_GOOD;

    /* Freeze the RT writer configuration */
    size_t dsmCount = 0;
    if(wg->config.encodingMimeType != UA_PUBSUB_ENCODING_UADP) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "PubSub-RT configuration fail: Non-RT capable encoding.");
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    //TODO Clarify: should we only allow = maxEncapsulatedDataSetMessageCount == 1 with RT?
    //TODO Clarify: Behaviour if the finale size is more than MTU

    /* Generate data set messages  */
    UA_STACKARRAY(UA_UInt16, dsWriterIds, wg->writersCount);
    UA_STACKARRAY(UA_DataSetMessage, dsmStore, wg->writersCount);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        /* Find the dataset */
        UA_PublishedDataSet *pds =
            UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
        if(!pds) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub Publish: PublishedDataSet not found");
            continue;
        }
        if(pds->promotedFieldsCount > 0) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub-RT configuration fail: PDS contains promoted fields.");
            return UA_STATUSCODE_BADNOTSUPPORTED;
        }

        /* Test the DataSetFields */
        UA_DataSetField *dsf;
        TAILQ_FOREACH(dsf, &pds->fields, listEntry) {
            const UA_VariableNode *rtNode = (const UA_VariableNode *)
                UA_NODESTORE_GET(server, &dsf->config.field.variable.publishParameters.publishedVariable);
            if(rtNode != NULL && rtNode->valueBackend.backendType != UA_VALUEBACKENDTYPE_EXTERNAL) {
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub-RT configuration fail: PDS contains field without external data source.");
                UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
                return UA_STATUSCODE_BADNOTSUPPORTED;
            }
            UA_NODESTORE_RELEASE(server, (const UA_Node *) rtNode);
            if((UA_NodeId_equal(&dsf->fieldMetaData.dataType, &UA_TYPES[UA_TYPES_STRING].typeId) ||
                UA_NodeId_equal(&dsf->fieldMetaData.dataType,
                                &UA_TYPES[UA_TYPES_BYTESTRING].typeId)) &&
               dsf->fieldMetaData.maxStringLength == 0) {
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub-RT configuration fail: "
                               "PDS contains String/ByteString with dynamic length.");
                return UA_STATUSCODE_BADNOTSUPPORTED;
            } else if(!UA_DataType_isNumeric(UA_findDataType(&dsf->fieldMetaData.dataType)) &&
                      !UA_NodeId_equal(&dsf->fieldMetaData.dataType, &UA_TYPES[UA_TYPES_BOOLEAN].typeId)) {
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                               "PubSub-RT configuration fail: "
                               "PDS contains variable with dynamic size.");
                return UA_STATUSCODE_BADNOTSUPPORTED;
            }
        }

        /* Generate the DSM */
        res = UA_DataSetWriter_generateDataSetMessage(server, &dsmStore[dsmCount], dsw);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "PubSub RT Offset calculation: DataSetMessage buffering failed");
            continue;
        }

        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        dsmCount++;
    }

    /* Define variables here for goto */
    size_t msgSize;
    UA_ByteString buf;
    UA_NetworkMessage networkMessage;
    const UA_Byte *bufEnd;
    UA_Byte *bufPos;

    if(res != UA_STATUSCODE_GOOD)
        goto cleanup_dsm;

    memset(&networkMessage, 0, sizeof(networkMessage));
    res = generateNetworkMessage(pubSubConnection, wg, dsmStore, dsWriterIds,
                                 (UA_Byte) dsmCount, &wg->config.messageSettings,
                                 &wg->config.transportSettings, &networkMessage);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup_dsm;

    /* Generate the offset-buffer (done inside calcSizeBinary) */
    memset(&wg->bufferedMessage, 0, sizeof(UA_NetworkMessageOffsetBuffer));
    UA_NetworkMessage_calcSizeBinary(&networkMessage, &wg->bufferedMessage);

    /* Allocate the buffer. Allocate on the stack if the buffer is small. */
    msgSize = UA_NetworkMessage_calcSizeBinary(&networkMessage, NULL);
    res = UA_ByteString_allocBuffer(&buf, msgSize);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    wg->bufferedMessage.buffer = buf;

    /* Encode the NetworkMessage */
    bufEnd = &wg->bufferedMessage.buffer.data[wg->bufferedMessage.buffer.length];
    bufPos = wg->bufferedMessage.buffer.data;
    UA_NetworkMessage_encodeBinary(&networkMessage, &bufPos, bufEnd, NULL);

 cleanup:
    UA_free(networkMessage.payload.dataSetPayload.sizes);

    /* Clean up DSM */
 cleanup_dsm:
    for(size_t i = 0; i < dsmCount; i++){
        UA_free(dsmStore[i].data.keyFrameData.dataSetFields);
#ifdef UA_ENABLE_JSON_ENCODING
        UA_Array_delete(dsmStore[i].data.keyFrameData.fieldNames,
                        dsmStore[i].data.keyFrameData.fieldCount,
                        &UA_TYPES[UA_TYPES_STRING]);
#endif
    }

    return res;
}

UA_StatusCode
UA_Server_unfreezeWriterGroupConfiguration(UA_Server *server,
                                           const UA_NodeId writerGroup) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;
    //if(wg->config.rtLevel == UA_PUBSUB_RT_NONE){
    //    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
    //                   "PubSub configuration freeze without RT configuration has no effect.");
    //    return UA_STATUSCODE_BADCONFIGURATIONERROR;
    //}
    //PubSubConnection freezeCounter--
    UA_PubSubConnection *pubSubConnection =  wg->linkedConnection;
    pubSubConnection->configurationFreezeCounter--;
    if(pubSubConnection->configurationFreezeCounter == 0){
        pubSubConnection->configurationFrozen = UA_FALSE;
    }
    //WriterGroup unfreeze
    wg->configurationFrozen = UA_FALSE;
    //DataSetWriter unfreeze
    UA_DataSetWriter *dataSetWriter;
    LIST_FOREACH(dataSetWriter, &wg->writers, listEntry) {
        UA_PublishedDataSet *publishedDataSet =
            UA_PublishedDataSet_findPDSbyId(server, dataSetWriter->connectedDataSet);
        //PublishedDataSet freezeCounter--
        publishedDataSet->configurationFreezeCounter--;
        if(publishedDataSet->configurationFreezeCounter == 0){
            publishedDataSet->configurationFrozen = UA_FALSE;
            UA_DataSetField *dataSetField;
            TAILQ_FOREACH(dataSetField, &publishedDataSet->fields, listEntry){
                dataSetField->configurationFrozen = UA_FALSE;
            }
        }
        dataSetWriter->configurationFrozen = UA_FALSE;
    }

    UA_NetworkMessageOffsetBuffer_clear(&wg->bufferedMessage);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setWriterGroupOperational(UA_Server *server,
                                    const UA_NodeId writerGroup) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_OPERATIONAL, wg);
}

UA_StatusCode
UA_Server_setWriterGroupDisabled(UA_Server *server,
                                 const UA_NodeId writerGroup) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_DISABLED, wg);
}

UA_StatusCode
UA_WriterGroupConfig_copy(const UA_WriterGroupConfig *src,
                          UA_WriterGroupConfig *dst){
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    memcpy(dst, src, sizeof(UA_WriterGroupConfig));
    res |= UA_String_copy(&src->name, &dst->name);
    res |= UA_ExtensionObject_copy(&src->transportSettings, &dst->transportSettings);
    res |= UA_ExtensionObject_copy(&src->messageSettings, &dst->messageSettings);
    if(src->groupPropertiesSize > 0) {
        dst->groupProperties = (UA_KeyValuePair*)
            UA_calloc(src->groupPropertiesSize, sizeof(UA_KeyValuePair));
        if(!dst->groupProperties)
            return UA_STATUSCODE_BADOUTOFMEMORY;
        for(size_t i = 0; i < src->groupPropertiesSize; i++) {
            res |= UA_KeyValuePair_copy(&src->groupProperties[i], &dst->groupProperties[i]);
        }
    }
    if(res != UA_STATUSCODE_GOOD)
        UA_WriterGroupConfig_clear(dst);
    return res;
}

UA_StatusCode
UA_Server_getWriterGroupConfig(UA_Server *server, const UA_NodeId writerGroup,
                               UA_WriterGroupConfig *config) {
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_WriterGroup *currentWG = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!currentWG)
        return UA_STATUSCODE_BADNOTFOUND;
    return UA_WriterGroupConfig_copy(&currentWG->config, config);
}

UA_StatusCode
UA_Server_updateWriterGroupConfig(UA_Server *server, UA_NodeId writerGroupIdentifier,
                                  const UA_WriterGroupConfig *config){
    if(!config)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_WriterGroup *currentWriterGroup =
        UA_WriterGroup_findWGbyId(server, writerGroupIdentifier);
    if(!currentWriterGroup)
        return UA_STATUSCODE_BADNOTFOUND;

    if(currentWriterGroup->configurationFrozen){
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Modify WriterGroup failed. WriterGroup is frozen.");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    //The update functionality will be extended during the next PubSub batches.
    //Currently is only a change of the publishing interval possible.
    if(currentWriterGroup->config.maxEncapsulatedDataSetMessageCount != config->maxEncapsulatedDataSetMessageCount) {
        currentWriterGroup->config.maxEncapsulatedDataSetMessageCount = config->maxEncapsulatedDataSetMessageCount;
        if(currentWriterGroup->config.messageSettings.encoding == UA_EXTENSIONOBJECT_ENCODED_NOBODY) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "MaxEncapsulatedDataSetMessag need enabled 'PayloadHeader' within the message settings.");
        }
    }

    if(currentWriterGroup->config.publishingInterval != config->publishingInterval) {
        if(currentWriterGroup->config.rtLevel == UA_PUBSUB_RT_NONE &&
           currentWriterGroup->state == UA_PUBSUBSTATE_OPERATIONAL) {
            if(currentWriterGroup->config.pubsubManagerCallback.removeCustomCallback)
                currentWriterGroup->config.pubsubManagerCallback.
                    removeCustomCallback(server, currentWriterGroup->identifier,
                                         currentWriterGroup->publishCallbackId);
            else
                UA_PubSubManager_removeRepeatedPubSubCallback(server, currentWriterGroup->publishCallbackId);

            currentWriterGroup->config.publishingInterval = config->publishingInterval;
            UA_WriterGroup_addPublishCallback(server, currentWriterGroup);
        } else {
            currentWriterGroup->config.publishingInterval = config->publishingInterval;
        }
    }

    if(currentWriterGroup->config.priority != config->priority) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "No or unsupported WriterGroup update.");
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_WriterGroup_getState(UA_Server *server, UA_NodeId writerGroupIdentifier,
                               UA_PubSubState *state) {
    if((server == NULL) || (state == NULL))
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_WriterGroup *currentWriterGroup =
        UA_WriterGroup_findWGbyId(server, writerGroupIdentifier);
    if(currentWriterGroup == NULL)
        return UA_STATUSCODE_BADNOTFOUND;
    *state = currentWriterGroup->state;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_WriterGroup_publish(UA_Server *server, const UA_NodeId writerGroupIdentifier) {

    // search WriterGroup ToDo create lookup table for more efficiency
    UA_WriterGroup *writerGroup;
    writerGroup = UA_WriterGroup_findWGbyId(server, writerGroupIdentifier);
    if(writerGroup == NULL) {
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_WriterGroup_publishCallback(server, writerGroup);
    return UA_STATUSCODE_GOOD;
}

UA_WriterGroup *
UA_WriterGroup_findWGbyId(UA_Server *server, UA_NodeId identifier) {
    UA_PubSubConnection *tmpConnection;
    TAILQ_FOREACH(tmpConnection, &server->pubSubManager.connections, listEntry) {
        UA_WriterGroup *tmpWriterGroup;
        LIST_FOREACH(tmpWriterGroup, &tmpConnection->writerGroups, listEntry) {
            if(UA_NodeId_equal(&identifier, &tmpWriterGroup->identifier))
                return tmpWriterGroup;
        }
    }
    return NULL;
}

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
UA_StatusCode
UA_Server_setWriterGroupEncryptionKeys(UA_Server *server, const UA_NodeId writerGroup,
                                       UA_UInt32 securityTokenId,
                                       const UA_ByteString signingKey,
                                       const UA_ByteString encryptingKey,
                                       const UA_ByteString keyNonce) {
    UA_WriterGroup *wg = UA_WriterGroup_findWGbyId(server, writerGroup);
    if(!wg)
        return UA_STATUSCODE_BADNOTFOUND;

    if(!wg->config.securityPolicy) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "No SecurityPolicy configured for the WriterGroup");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    if(securityTokenId != wg->securityTokenId) {
        wg->securityTokenId = securityTokenId;
        wg->nonceSequenceNumber = 1;
    }

    /* Create a new context */
    if(!wg->securityPolicyContext) {
        return wg->config.securityPolicy->
            newContext(wg->config.securityPolicy->policyContext,
                       &signingKey, &encryptingKey, &keyNonce,
                       &wg->securityPolicyContext);
    }

    /* Update the context */
    return wg->config.securityPolicy->
        setSecurityKeys(wg->securityPolicyContext, &signingKey, &encryptingKey, &keyNonce);
}
#endif

void
UA_WriterGroupConfig_clear(UA_WriterGroupConfig *writerGroupConfig){
    UA_String_clear(&writerGroupConfig->name);
    UA_ExtensionObject_clear(&writerGroupConfig->transportSettings);
    UA_ExtensionObject_clear(&writerGroupConfig->messageSettings);
    UA_Array_delete(writerGroupConfig->groupProperties,
                    writerGroupConfig->groupPropertiesSize,
                    &UA_TYPES[UA_TYPES_KEYVALUEPAIR]);
    writerGroupConfig->groupProperties = NULL;
}

static void
UA_WriterGroup_clear(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_WriterGroupConfig_clear(&writerGroup->config);
    UA_NodeId_clear(&writerGroup->identifier);

    /* Delete all writers */
    UA_DataSetWriter *dataSetWriter, *tmpDataSetWriter;
    LIST_FOREACH_SAFE(dataSetWriter, &writerGroup->writers, listEntry, tmpDataSetWriter){
        UA_Server_removeDataSetWriter(server, dataSetWriter->identifier);
    }

    UA_NetworkMessageOffsetBuffer_clear(&writerGroup->bufferedMessage);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(writerGroup->config.securityPolicy && writerGroup->securityPolicyContext) {
        writerGroup->config.securityPolicy->deleteContext(writerGroup->securityPolicyContext);
        writerGroup->securityPolicyContext = NULL;
    }
#endif
}

UA_StatusCode
UA_WriterGroup_setPubSubState(UA_Server *server, UA_PubSubState state,
                              UA_WriterGroup *writerGroup) {
    UA_DataSetWriter *dataSetWriter;
    switch(state) {
        case UA_PUBSUBSTATE_DISABLED:
            switch (writerGroup->state){
                case UA_PUBSUBSTATE_DISABLED:
                    return UA_STATUSCODE_GOOD;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    if(writerGroup->config.pubsubManagerCallback.removeCustomCallback)
                        writerGroup->config.pubsubManagerCallback.
                            removeCustomCallback(server, writerGroup->identifier, writerGroup->publishCallbackId);
                    else
                        UA_PubSubManager_removeRepeatedPubSubCallback(server, writerGroup->publishCallbackId);

                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                        UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_DISABLED, dataSetWriter);
                    }
                    writerGroup->state = UA_PUBSUBSTATE_DISABLED;
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_PAUSED:
            switch (writerGroup->state) {
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    return UA_STATUSCODE_GOOD;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_OPERATIONAL:
            switch (writerGroup->state) {
                case UA_PUBSUBSTATE_DISABLED:
                    writerGroup->state = UA_PUBSUBSTATE_OPERATIONAL;
                    if(writerGroup->config.pubsubManagerCallback.removeCustomCallback)
                        writerGroup->config.pubsubManagerCallback.
                            removeCustomCallback(server, writerGroup->identifier,
                                                 writerGroup->publishCallbackId);
                    else
                        UA_PubSubManager_removeRepeatedPubSubCallback(server, writerGroup->publishCallbackId);

                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                        UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_OPERATIONAL,
                                                        dataSetWriter);
                    }
                    UA_WriterGroup_addPublishCallback(server, writerGroup);
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    return UA_STATUSCODE_GOOD;
                case UA_PUBSUBSTATE_ERROR:
                    break;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Received unknown PubSub state!");
            }
            break;
        case UA_PUBSUBSTATE_ERROR: {
            switch (writerGroup->state){
                case UA_PUBSUBSTATE_DISABLED:
                    break;
                case UA_PUBSUBSTATE_PAUSED:
                    break;
                case UA_PUBSUBSTATE_OPERATIONAL:
                    UA_PubSubManager_removeRepeatedPubSubCallback(server, writerGroup->publishCallbackId);
                    LIST_FOREACH(dataSetWriter, &writerGroup->writers, listEntry){
                        UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dataSetWriter);
                    }
                    break;
                case UA_PUBSUBSTATE_ERROR:
                    return UA_STATUSCODE_GOOD;
                default:
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                    "Received unknown PubSub state!");
            }
            writerGroup->state = UA_PUBSUBSTATE_ERROR;
            /* TODO: WIP - example usage of pubsubStateChangeCallback -> inform
             * application about error state, reason param necessary */
            UA_ServerConfig *pConfig = UA_Server_getConfig(server);
            if(pConfig->pubSubConfig.stateChangeCallback != 0) {
                pConfig->pubSubConfig.
                    stateChangeCallback(&writerGroup->identifier, UA_PUBSUBSTATE_ERROR,
                                        UA_STATUSCODE_BADINTERNALERROR);
            }
            break;
        }
        default:
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Received unknown PubSub state!");
    }
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
static UA_StatusCode
encryptAndSign(UA_WriterGroup *wg, const UA_NetworkMessage *nm,
               UA_Byte *signStart, UA_Byte *encryptStart,
               UA_Byte *msgEnd) {
    UA_StatusCode rv;
    void *channelContext = wg->securityPolicyContext;

    if(nm->securityHeader.networkMessageEncrypted) {
        /* Set the temporary MessageNonce in the SecurityPolicy */
        rv = wg->config.securityPolicy->setMessageNonce(channelContext,
                                                        &nm->securityHeader.messageNonce);
        UA_CHECK_STATUS(rv, return rv);

        /* The encryption is done in-place, no need to encode again */
        UA_ByteString toBeEncrypted =
            {(uintptr_t)msgEnd - (uintptr_t)encryptStart, encryptStart};
        rv = wg->config.securityPolicy->symmetricModule.cryptoModule.encryptionAlgorithm.
            encrypt(channelContext, &toBeEncrypted);
        UA_CHECK_STATUS(rv, return rv);
    }

    if(nm->securityHeader.networkMessageSigned) {
        UA_ByteString toBeSigned = {(uintptr_t)msgEnd - (uintptr_t)signStart,
                                    signStart};

        size_t sigSize = wg->config.securityPolicy->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(channelContext);
        UA_ByteString signature = {sigSize, msgEnd};

        rv = wg->config.securityPolicy->symmetricModule.cryptoModule.
            signatureAlgorithm.sign(channelContext, &toBeSigned, &signature);
        UA_CHECK_STATUS(rv, return rv);
    }
    return UA_STATUSCODE_GOOD;
}
#endif

static UA_StatusCode
encodeNetworkMessage(UA_WriterGroup *wg, UA_NetworkMessage *nm,
                     UA_ByteString *buf) {
    UA_Byte *bufPos = buf->data;
    UA_Byte *bufEnd = &buf->data[buf->length];

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_Byte *networkMessageStart = bufPos;
#endif
    UA_StatusCode rv = UA_NetworkMessage_encodeHeaders(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    UA_Byte *payloadStart = bufPos;
#endif
    rv = UA_NetworkMessage_encodePayload(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

    rv = UA_NetworkMessage_encodeFooters(nm, &bufPos, bufEnd);
    UA_CHECK_STATUS(rv, return rv);

#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    /* Encrypt and Sign the message */
    UA_Byte *footerEnd = bufPos;
    rv = encryptAndSign(wg, nm, networkMessageStart, payloadStart, footerEnd);
    UA_CHECK_STATUS(rv, return rv);
#endif

    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_JSON_ENCODING
static UA_StatusCode
sendNetworkMessageJson(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount) {
    /* Prepare the NetworkMessage */
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));
    nm.version = 1;
    nm.networkMessageType = UA_NETWORKMESSAGE_DATASET;
    nm.payloadHeaderEnabled = true;
    nm.payloadHeader.dataSetPayloadHeader.count = dsmCount;
    nm.payloadHeader.dataSetPayloadHeader.dataSetWriterIds = writerIds;
    nm.payload.dataSetPayload.dataSetMessages = dsm;

    /* Compute the message length */
    size_t msgSize = UA_NetworkMessage_calcSizeJson(&nm, NULL, 0, NULL, 0, true);

    /* Allocate the buffer. Allocate on the stack if the buffer is small. */
    UA_ByteString buf;
    UA_Byte stackBuf[UA_MAX_STACKBUF];
    buf.data = stackBuf;
    buf.length = msgSize;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(msgSize > UA_MAX_STACKBUF) {
        res = UA_ByteString_allocBuffer(&buf, msgSize);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }

    /* Encode the message */
    UA_Byte *bufPos = buf.data;
    const UA_Byte *bufEnd = &buf.data[msgSize];
    res = UA_NetworkMessage_encodeJson(&nm, &bufPos, &bufEnd, NULL, 0, NULL, 0, true);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;
    UA_assert(bufPos == bufEnd);

    /* Send the prepared messages */
    res = connection->channel->send(connection->channel,
                                    &wg->config.transportSettings, &buf);

 cleanup:
    if(msgSize > UA_MAX_STACKBUF)
        UA_ByteString_clear(&buf);
    return res;
}
#endif

static UA_StatusCode
generateNetworkMessage(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                       UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount,
                       UA_ExtensionObject *messageSettings,
                       UA_ExtensionObject *transportSettings,
                       UA_NetworkMessage *networkMessage) {
    if(messageSettings->content.decoded.type !=
       &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE])
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_UadpWriterGroupMessageDataType *wgm = (UA_UadpWriterGroupMessageDataType*)
            messageSettings->content.decoded.data;

    networkMessage->publisherIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID) != 0;
    networkMessage->groupHeaderEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER) != 0;
    networkMessage->groupHeader.writerGroupIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID) != 0;
    networkMessage->groupHeader.groupVersionEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPVERSION) != 0;
    networkMessage->groupHeader.networkMessageNumberEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_NETWORKMESSAGENUMBER) != 0;
    networkMessage->groupHeader.sequenceNumberEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_SEQUENCENUMBER) != 0;
    networkMessage->payloadHeaderEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER) != 0;
    networkMessage->timestampEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_TIMESTAMP) != 0;
    networkMessage->picosecondsEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PICOSECONDS) != 0;
    networkMessage->dataSetClassIdEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_DATASETCLASSID) != 0;
    networkMessage->promotedFieldsEnabled =
        ((u64)wgm->networkMessageContentMask &
         (u64)UA_UADPNETWORKMESSAGECONTENTMASK_PROMOTEDFIELDS) != 0;

    /* Set the SecurityHeader */
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        networkMessage->securityEnabled = true;
        networkMessage->securityHeader.networkMessageSigned = true;
        if(wg->config.securityMode >= UA_MESSAGESECURITYMODE_SIGNANDENCRYPT)
            networkMessage->securityHeader.networkMessageEncrypted = true;
        networkMessage->securityHeader.securityTokenId = wg->securityTokenId;

        /* Generate the MessageNonce */
        UA_ByteString_allocBuffer(&networkMessage->securityHeader.messageNonce, 8);
        if(networkMessage->securityHeader.messageNonce.length == 0)
            return UA_STATUSCODE_BADOUTOFMEMORY;

        networkMessage->securityHeader.messageNonce.length = 4; /* Generate 4 random bytes */
        UA_StatusCode rv = wg->config.securityPolicy->symmetricModule.
            generateNonce(wg->config.securityPolicy->policyContext,
                          &networkMessage->securityHeader.messageNonce);
        if(rv != UA_STATUSCODE_GOOD)
            return rv;
        networkMessage->securityHeader.messageNonce.length = 8;
        UA_Byte *pos = &networkMessage->securityHeader.messageNonce.data[4];
        const UA_Byte *end = &networkMessage->securityHeader.messageNonce.data[8];
        UA_UInt32_encodeBinary(&wg->nonceSequenceNumber, &pos, end);
    }
#endif

    networkMessage->version = 1;
    networkMessage->networkMessageType = UA_NETWORKMESSAGE_DATASET;
    if(connection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_NUMERIC) {
        networkMessage->publisherIdType = UA_PUBLISHERDATATYPE_UINT16;
        networkMessage->publisherId.publisherIdUInt32 =
            connection->config->publisherId.numeric;
    } else if(connection->config->publisherIdType == UA_PUBSUB_PUBLISHERID_STRING) {
        networkMessage->publisherIdType = UA_PUBLISHERDATATYPE_STRING;
        networkMessage->publisherId.publisherIdString =
            connection->config->publisherId.string;
    }

    if(networkMessage->groupHeader.sequenceNumberEnabled)
        networkMessage->groupHeader.sequenceNumber = wg->sequenceNumber;

    if(networkMessage->groupHeader.groupVersionEnabled)
        networkMessage->groupHeader.groupVersion = wgm->groupVersion;

    /* Compute the length of the dsm separately for the header */
    UA_UInt16 *dsmLengths = (UA_UInt16 *) UA_calloc(dsmCount, sizeof(UA_UInt16));
    if(!dsmLengths)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(UA_Byte i = 0; i < dsmCount; i++)
        dsmLengths[i] = (UA_UInt16) UA_DataSetMessage_calcSizeBinary(&dsm[i], NULL, 0);

    networkMessage->payloadHeader.dataSetPayloadHeader.count = dsmCount;
    networkMessage->payloadHeader.dataSetPayloadHeader.dataSetWriterIds = writerIds;
    networkMessage->groupHeader.writerGroupId = wg->config.writerGroupId;
    /* number of the NetworkMessage inside a PublishingInterval */
    networkMessage->groupHeader.networkMessageNumber = 1;
    networkMessage->payload.dataSetPayload.sizes = dsmLengths;
    networkMessage->payload.dataSetPayload.dataSetMessages = dsm;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
sendNetworkMessageBinary(UA_PubSubConnection *connection, UA_WriterGroup *wg,
                         UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount) {
    UA_NetworkMessage nm;
    memset(&nm, 0, sizeof(UA_NetworkMessage));

    /* Fill the message structure */
    UA_StatusCode rv =
        generateNetworkMessage(connection, wg, dsm, writerIds, dsmCount,
                               &wg->config.messageSettings,
                               &wg->config.transportSettings, &nm);
    UA_CHECK_STATUS(rv, return rv);

    /* Compute the message size. Add the overhead for the security signature.
     * There is no padding and the encryption incurs no size overhead. */
    size_t msgSize = UA_NetworkMessage_calcSizeBinary(&nm, NULL);
#ifdef UA_ENABLE_PUBSUB_ENCRYPTION
    if(wg->config.securityMode > UA_MESSAGESECURITYMODE_NONE) {
        UA_PubSubSecurityPolicy *sp = wg->config.securityPolicy;
        msgSize += sp->symmetricModule.cryptoModule.
            signatureAlgorithm.getLocalSignatureSize(sp->policyContext);
    }
#endif

    /* Allocate the buffer. Allocate on the stack if the buffer is small. */
    UA_ByteString buf;
    UA_Byte stackBuf[UA_MAX_STACKBUF];
    buf.data = stackBuf;
    buf.length = msgSize;
    if(msgSize > UA_MAX_STACKBUF) {
        rv = UA_ByteString_allocBuffer(&buf, msgSize);
        UA_CHECK_STATUS(rv, goto cleanup);
    }

    /* Encode and encrypt the message */
    rv = encodeNetworkMessage(wg, &nm, &buf);
    UA_CHECK_STATUS(rv, goto cleanup_with_msg_size);

    /* Send out the message */
    rv = connection->channel->send(connection->channel,
                                   &wg->config.transportSettings, &buf);
    UA_CHECK_STATUS(rv, goto cleanup_with_msg_size);

cleanup_with_msg_size:
    if(msgSize > UA_MAX_STACKBUF) {
        UA_ByteString_clear(&buf);
    }
cleanup:
    UA_ByteString_clear(&nm.securityHeader.messageNonce);
    UA_free(nm.payload.dataSetPayload.sizes);
    return rv;
}

static void
sendNetworkMessage(UA_Server *server, UA_WriterGroup *wg, UA_PubSubConnection *connection,
                   UA_DataSetMessage *dsm, UA_UInt16 *writerIds, UA_Byte dsmCount) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    switch(wg->config.encodingMimeType) {
    case UA_PUBSUB_ENCODING_UADP:
        res = sendNetworkMessageBinary(connection, wg, dsm, writerIds, dsmCount);
        break;
#ifdef UA_ENABLE_JSON_ENCODING
    case UA_PUBSUB_ENCODING_JSON:
        res = sendNetworkMessageJson(connection, wg, dsm, writerIds, dsmCount);
        break;
#endif
    default:
        res = UA_STATUSCODE_BADNOTSUPPORTED;
        break;
    }

    /* If sending failed, disable all writer of the writergroup */
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub Publish: Could not send a NetworkMessage "
                     "with status code %s", UA_StatusCode_name(res));
        UA_DataSetWriter *dsw;
        LIST_FOREACH(dsw, &wg->writers, listEntry) {
            UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
        }
        return;
    }

    /* Increase the sequence number */
    wg->sequenceNumber++;
}

static void
sendBufferedNetworkMessage(UA_Server *server, UA_WriterGroup *wg,
                           UA_PubSubConnection *connection) {
    UA_NetworkMessageOffsetBuffer *buf = &wg->bufferedMessage;
    UA_StatusCode res = UA_NetworkMessage_updateBufferedMessage(buf);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "PubSub sending failed - could not update the message");
        UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_ERROR, wg);
        return;
    }

    res = connection->channel->send(connection->channel,
                                    &wg->config.transportSettings, &buf->buffer);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Publish failed. RT fixed size. sendBufferedNetworkMessage failed");
        UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_ERROR, wg);
        return;
    }

    /* Sending successful - increase the sequence number */
    wg->sequenceNumber++;
}

/* This callback triggers the collection and publish of NetworkMessages and the
 * contained DataSetMessages. */
void
UA_WriterGroup_publishCallback(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER, "Publish Callback");

    // TODO: review if its okay to force correct value from caller side instead
    // UA_assert(writerGroup != NULL);
    // UA_assert(server != NULL);

    if(!writerGroup) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                     "Publish failed. WriterGroup not found");
        return;
    }

    /* Nothing to do? */
    if(writerGroup->writersCount == 0)
        return;

    /* Find the connection associated with the writer */
    UA_PubSubConnection *connection = writerGroup->linkedConnection;
    if(!connection) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Publish failed. PubSubConnection invalid.");
        UA_WriterGroup_setPubSubState(server, UA_PUBSUBSTATE_ERROR, writerGroup);
        return;
    }

    /* Realtime path - update the buffer message and send directly */
    if(writerGroup->config.rtLevel == UA_PUBSUB_RT_FIXED_SIZE) {
        sendBufferedNetworkMessage(server, writerGroup, connection);
        return;
    }

    /* How many DSM can be sent in one NM? */
    UA_Byte maxDSM = (UA_Byte)writerGroup->config.maxEncapsulatedDataSetMessageCount;
    if(writerGroup->config.maxEncapsulatedDataSetMessageCount > UA_BYTE_MAX)
        maxDSM = UA_BYTE_MAX;

    /* If the maxEncapsulatedDataSetMessageCount is set to 0 -> 1 */
    if(maxDSM == 0)
        maxDSM = 1;

    /* It is possible to put several DataSetMessages into one NetworkMessage.
     * But only if they do not contain promoted fields. NM with promoted fields
     * are sent out right away. The others are kept in a buffer for
     * "batching". */
    size_t dsmCount = 0;
    UA_STACKARRAY(UA_UInt16, dsWriterIds, writerGroup->writersCount);
    UA_STACKARRAY(UA_DataSetMessage, dsmStore, writerGroup->writersCount);

    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &writerGroup->writers, listEntry) {
        if(dsw->state != UA_PUBSUBSTATE_OPERATIONAL)
            continue;

        /* Find the dataset */
        UA_PublishedDataSet *pds =
            UA_PublishedDataSet_findPDSbyId(server, dsw->connectedDataSet);
        if(!pds) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "PubSub Publish: PublishedDataSet not found");
            UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
            continue;
        }

        /* Generate the DSM */
        dsWriterIds[dsmCount] = dsw->config.dataSetWriterId;
        UA_StatusCode res =
            UA_DataSetWriter_generateDataSetMessage(server, &dsmStore[dsmCount], dsw);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                         "PubSub Publish: DataSetMessage creation failed");
            UA_DataSetWriter_setPubSubState(server, UA_PUBSUBSTATE_ERROR, dsw);
            continue;
        }

        /* There is no promoted field -> send right away */
        if(pds->promotedFieldsCount > 0) {
            sendNetworkMessage(server, writerGroup, connection, &dsmStore[dsmCount],
                               &dsWriterIds[dsmCount], 1);

            /* Clean up the current store entry */
            if(writerGroup->config.rtLevel == UA_PUBSUB_RT_DIRECT_VALUE_ACCESS) {
                for(size_t i = 0; i < dsmStore[dsmCount].data.keyFrameData.fieldCount; ++i) {
                    dsmStore[dsmCount].data.keyFrameData.dataSetFields[i].value.data = NULL;
                }
            }
            UA_DataSetMessage_clear(&dsmStore[dsmCount]);

            continue; /* Don't increase the dsmCount, reuse the slot */
        }

        dsmCount++;
    }

    /* Send the NetworkMessages with batched DataSetMessages */
    UA_Byte nmDsmCount = 0;
    for(size_t i = 0; i < dsmCount; i += nmDsmCount) {
        /* How many dsm are batched in this iteration? */
        nmDsmCount = (i + maxDSM > dsmCount) ? (UA_Byte)(dsmCount - i) : maxDSM;

        /* Send the batched messages */
        sendNetworkMessage(server, writerGroup, connection, &dsmStore[i],
                           &dsWriterIds[i], nmDsmCount);
    }

    /* Clean up DSM */
    for(size_t i = 0; i < dsmCount; i++) {
        if(writerGroup->config.rtLevel == UA_PUBSUB_RT_DIRECT_VALUE_ACCESS) {
            for(size_t j = 0; j < dsmStore[i].data.keyFrameData.fieldCount; ++j) {
                dsmStore[i].data.keyFrameData.dataSetFields[j].value.data = NULL;
            }
        }
        UA_DataSetMessage_clear(&dsmStore[i]);
    }
}

/* Add new publishCallback. The first execution is triggered directly after
 * creation. */
UA_StatusCode
UA_WriterGroup_addPublishCallback(UA_Server *server, UA_WriterGroup *writerGroup) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if(writerGroup->config.pubsubManagerCallback.addCustomCallback)
        retval |= writerGroup->config.pubsubManagerCallback.
            addCustomCallback(server, writerGroup->identifier,
                              (UA_ServerCallback) UA_WriterGroup_publishCallback,
                              writerGroup, writerGroup->config.publishingInterval,
                              NULL,                                        // TODO: Send base time from writer group config
                              UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,  // TODO: Send timer policy from writer group config
                              &writerGroup->publishCallbackId);
    else
        retval |= UA_PubSubManager_addRepeatedCallback(server,
                     (UA_ServerCallback) UA_WriterGroup_publishCallback,
                     writerGroup, writerGroup->config.publishingInterval,
                     NULL,                                        // TODO: Send base time from writer group config
                     UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,  // TODO: Send timer policy from writer group config
                     &writerGroup->publishCallbackId);

    if(retval == UA_STATUSCODE_GOOD)
        writerGroup->publishCallbackIsRegistered = true;

    /* Run once after creation */
    UA_WriterGroup_publishCallback(server, writerGroup);
    return retval;
}

#endif /* UA_ENABLE_PUBSUB */
