/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2022 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Noel Graf)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include <open62541/types.h>
#include "ua_pubsub.h"
#include "ua_pubsub_ns0.h"
#include "server/ua_server_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_SKS
#include "ua_pubsub_keystorage.h"
#endif

#define UA_DATETIMESTAMP_2000 125911584000000000
#define UA_RESERVEID_FIRST_ID 0x8000

static const char *pubSubStateNames[6] = {
    "Disabled", "Paused", "Operational", "Error", "PreOperational", "Invalid"
};

const char *
UA_PubSubState_name(UA_PubSubState state) {
    if(state < UA_PUBSUBSTATE_DISABLED || state > UA_PUBSUBSTATE_PREOPERATIONAL)
        return pubSubStateNames[5];
    return pubSubStateNames[state];
}

UA_StatusCode
UA_PublisherId_copy(const UA_PublisherId *src,
                    UA_PublisherId *dst) {
    memcpy(dst, src, sizeof(UA_PublisherId));
    if(src->idType == UA_PUBLISHERIDTYPE_STRING)
        return UA_String_copy(&src->id.string, &dst->id.string);
    return UA_STATUSCODE_GOOD;
}

void
UA_PublisherId_clear(UA_PublisherId *p) {
    if(p->idType == UA_PUBLISHERIDTYPE_STRING)
        UA_String_clear(&p->id.string);
    memset(p, 0, sizeof(UA_PublisherId));
}

UA_StatusCode
UA_PublisherId_fromVariant(UA_PublisherId *p, const UA_Variant *src) {
    if(!UA_Variant_isScalar(src))
        return UA_STATUSCODE_BADINTERNALERROR;

    memset(p, 0, sizeof(UA_PublisherId));

    const void *data = (const void*)src->data;
    if(src->type == &UA_TYPES[UA_TYPES_BYTE]) {
        p->idType = UA_PUBLISHERIDTYPE_BYTE;
        p->id.byte = *(const UA_Byte*)data;
    } else if(src->type == &UA_TYPES[UA_TYPES_UINT16]) {
        p->idType  = UA_PUBLISHERIDTYPE_UINT16;
        p->id.uint16 = *(const UA_UInt16*)data;
    } else if(src->type == &UA_TYPES[UA_TYPES_UINT32]) {
        p->idType  = UA_PUBLISHERIDTYPE_UINT32;
        p->id.uint32 = *(const UA_UInt32*)data;
    } else if(src->type == &UA_TYPES[UA_TYPES_UINT64]) {
        p->idType  = UA_PUBLISHERIDTYPE_UINT64;
        p->id.uint64 = *(const UA_UInt64*)data;
    } else if(src->type == &UA_TYPES[UA_TYPES_STRING]) {
        p->idType  = UA_PUBLISHERIDTYPE_STRING;
        return UA_String_copy((const UA_String *)data, &p->id.string);
    } else {
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

void
UA_PublisherId_toVariant(const UA_PublisherId *p, UA_Variant *dst) {
    UA_PublisherId *p2 = (UA_PublisherId*)(uintptr_t)p;
    switch(p->idType) {
    case UA_PUBLISHERIDTYPE_BYTE:
        UA_Variant_setScalar(dst, &p2->id.byte, &UA_TYPES[UA_TYPES_BYTE]); break;
    case UA_PUBLISHERIDTYPE_UINT16:
        UA_Variant_setScalar(dst, &p2->id.uint16, &UA_TYPES[UA_TYPES_UINT16]); break;
    case UA_PUBLISHERIDTYPE_UINT32:
        UA_Variant_setScalar(dst, &p2->id.uint32, &UA_TYPES[UA_TYPES_UINT32]); break;
    case UA_PUBLISHERIDTYPE_UINT64:
        UA_Variant_setScalar(dst, &p2->id.uint64, &UA_TYPES[UA_TYPES_UINT64]); break;
    case UA_PUBLISHERIDTYPE_STRING:
        UA_Variant_setScalar(dst, &p2->id.string, &UA_TYPES[UA_TYPES_STRING]); break;
    default: break; /* This is not possible if the PublisherId is well-defined */
    }
}

static void
UA_PubSubManager_addTopic(UA_PubSubManager *pubSubManager, UA_TopicAssign *topicAssign) {
    TAILQ_INSERT_TAIL(&pubSubManager->topicAssign, topicAssign, listEntry);
    pubSubManager->topicAssignSize++;
}

static UA_TopicAssign *
UA_TopicAssign_new(UA_ReaderGroup *readerGroup,
                   UA_String topic, const UA_Logger *logger) {
    UA_TopicAssign *topicAssign = (UA_TopicAssign *) calloc(1, sizeof(UA_TopicAssign));
    if(!topicAssign) {
        UA_LOG_ERROR(logger, UA_LOGCATEGORY_SERVER,
                     "PubSub TopicAssign creation failed. Out of Memory.");
        return NULL;
    }
    topicAssign->rgIdentifier = readerGroup;
    topicAssign->topic = topic;
    return topicAssign;
}

UA_StatusCode
UA_PubSubManager_addPubSubTopicAssign(UA_Server *server, UA_ReaderGroup *readerGroup, UA_String topic) {
    UA_PubSubManager *pubSubManager = &server->pubSubManager;
    UA_TopicAssign *topicAssign = UA_TopicAssign_new(readerGroup, topic, server->config.logging);
    UA_PubSubManager_addTopic(pubSubManager, topicAssign);
    return UA_STATUSCODE_GOOD;
}

static enum ZIP_CMP
cmpReserveId(const void *a, const void *b) {
    const UA_ReserveId *aa = (const UA_ReserveId*)a;
    const UA_ReserveId *bb = (const UA_ReserveId*)b;
    if(aa->id != bb->id)
        return (aa->id < bb->id) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
    if(aa->reserveIdType != bb->reserveIdType)
        return (aa->reserveIdType < bb->reserveIdType) ? ZIP_CMP_LESS : ZIP_CMP_MORE;
    return (enum ZIP_CMP)UA_order(&aa->transportProfileUri,
                                  &bb->transportProfileUri, &UA_TYPES[UA_TYPES_STRING]);
}

ZIP_FUNCTIONS(UA_ReserveIdTree, UA_ReserveId, treeEntry, UA_ReserveId, id, cmpReserveId)

static UA_ReserveId *
UA_ReserveId_new(UA_Server *server, UA_UInt16 id, UA_String transportProfileUri,
                 UA_ReserveIdType reserveIdType, UA_NodeId sessionId) {
    UA_ReserveId *reserveId = (UA_ReserveId *) calloc(1, sizeof(UA_ReserveId));
    if(!reserveId) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "PubSub ReserveId creation failed. Out of Memory.");
        return NULL;
    }
    reserveId->id = id;
    reserveId->reserveIdType = reserveIdType;
    UA_String_copy(&transportProfileUri, &reserveId->transportProfileUri);
    reserveId->sessionId = sessionId;

    return reserveId;
}

static UA_Boolean
UA_ReserveId_isFree(UA_Server *server,  UA_UInt16 id,
                    UA_String transportProfileUri, UA_ReserveIdType reserveIdType) {
    UA_PubSubManager *pubSubManager = &server->pubSubManager;

    /* Is the id already in use? */
    UA_ReserveId compare;
    compare.id = id;
    compare.reserveIdType = reserveIdType;
    compare.transportProfileUri = transportProfileUri;
    if(ZIP_FIND(UA_ReserveIdTree, &pubSubManager->reserveIds, &compare))
        return false;

    UA_PubSubConnection *tmpConnection;
    TAILQ_FOREACH(tmpConnection, &server->pubSubManager.connections, listEntry) {
        UA_WriterGroup *writerGroup;
        LIST_FOREACH(writerGroup, &tmpConnection->writerGroups, listEntry) {
            if(reserveIdType == UA_WRITER_GROUP) {
                if(UA_String_equal(&tmpConnection->config.transportProfileUri,
                                   &transportProfileUri) &&
                   writerGroup->config.writerGroupId == id)
                    return false;
            /* reserveIdType == UA_DATA_SET_WRITER */
            } else {
                UA_DataSetWriter *currentWriter;
                LIST_FOREACH(currentWriter, &writerGroup->writers, listEntry) {
                    if(UA_String_equal(&tmpConnection->config.transportProfileUri,
                                       &transportProfileUri) &&
                       currentWriter->config.dataSetWriterId == id)
                        return false;
                }
            }
        }
    }
    return true;
}

static UA_UInt16
UA_ReserveId_createId(UA_Server *server,  UA_NodeId sessionId,
                      UA_String transportProfileUri, UA_ReserveIdType reserveIdType) {
    /* Total number of possible Ids */
    UA_UInt16 numberOfIds = 0x8000;
    /* Contains next possible free Id */
    static UA_UInt16 next_id_writerGroup = UA_RESERVEID_FIRST_ID;
    static UA_UInt16 next_id_writer = UA_RESERVEID_FIRST_ID;
    UA_UInt16 next_id;
    UA_Boolean is_free = false;

    if(reserveIdType == UA_WRITER_GROUP)
        next_id = next_id_writerGroup;
    else
        next_id = next_id_writer;

    for(;numberOfIds > 0;numberOfIds--) {
        if(next_id < UA_RESERVEID_FIRST_ID)
            next_id = UA_RESERVEID_FIRST_ID;
        is_free = UA_ReserveId_isFree(server, next_id, transportProfileUri, reserveIdType);
        if(is_free)
            break;
        next_id++;
    }
    if(!is_free) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "PubSub ReserveId creation failed. No free ID could be found.");
        return 0;
    }

    if(reserveIdType == UA_WRITER_GROUP)
        next_id_writerGroup = (UA_UInt16)(next_id + 1);
    else
        next_id_writer = (UA_UInt16)(next_id + 1);

    UA_ReserveId *reserveId =
        UA_ReserveId_new(server, next_id, transportProfileUri, reserveIdType, sessionId);
    if(!reserveId)
        return 0;
    UA_PubSubManager *pubSubManager = &server->pubSubManager;
    ZIP_INSERT(UA_ReserveIdTree, &pubSubManager->reserveIds, reserveId);
    pubSubManager->reserveIdsSize++;
    return next_id;
}

static void *
removeReserveId(void *context, UA_ReserveId *elem) {
    UA_String_clear(&elem->transportProfileUri);
    UA_free(elem);
    return NULL;
}

struct RemoveInactiveReserveIdContext {
    UA_Server *server;
    UA_ReserveIdTree newTree;
};

/* Remove ReserveIds that are not attached to any session */
static void *
removeInactiveReserveId(void *context, UA_ReserveId *elem) {
    struct RemoveInactiveReserveIdContext *ctx =
        (struct RemoveInactiveReserveIdContext*)context;

    if(UA_NodeId_equal(&ctx->server->adminSession.sessionId, &elem->sessionId))
        goto still_active;

    session_list_entry *session;
    LIST_FOREACH(session, &ctx->server->sessions, pointers) {
        if(UA_NodeId_equal(&session->session.sessionId, &elem->sessionId))
            goto still_active;
    }

    ctx->server->pubSubManager.reserveIdsSize--;
    UA_String_clear(&elem->transportProfileUri);
    UA_free(elem);
    return NULL;

 still_active:
    ZIP_INSERT(UA_ReserveIdTree, &ctx->newTree, elem);
    return NULL;
}

void
UA_PubSubManager_freeIds(UA_Server *server) {
    struct RemoveInactiveReserveIdContext removeCtx;
    removeCtx.server = server;
    removeCtx.newTree.root = NULL;
    ZIP_ITER(UA_ReserveIdTree, &server->pubSubManager.reserveIds,
             removeInactiveReserveId, &removeCtx);
    server->pubSubManager.reserveIds = removeCtx.newTree;
}

UA_StatusCode
UA_PubSubManager_reserveIds(UA_Server *server, UA_NodeId sessionId, UA_UInt16 numRegWriterGroupIds,
                            UA_UInt16 numRegDataSetWriterIds, UA_String transportProfileUri,
                            UA_UInt16 **writerGroupIds, UA_UInt16 **dataSetWriterIds) {
    UA_PubSubManager_freeIds(server);

    /* Check the validation of the transportProfileUri */
    UA_String profile_1 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    UA_String profile_2 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json");
    UA_String profile_3 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    if(!UA_String_equal(&transportProfileUri, &profile_1) &&
       !UA_String_equal(&transportProfileUri, &profile_2) &&
       !UA_String_equal(&transportProfileUri, &profile_3)) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "PubSub ReserveId creation failed. No valid transport profile uri.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    *writerGroupIds = (UA_UInt16*)UA_Array_new(numRegWriterGroupIds, &UA_TYPES[UA_TYPES_UINT16]);
    *dataSetWriterIds = (UA_UInt16*)UA_Array_new(numRegDataSetWriterIds, &UA_TYPES[UA_TYPES_UINT16]);

    for(int i = 0; i < numRegWriterGroupIds; i++) {
        (*writerGroupIds)[i] =
            UA_ReserveId_createId(server, sessionId, transportProfileUri, UA_WRITER_GROUP);
    }
    for(int i = 0; i < numRegDataSetWriterIds; i++) {
        (*dataSetWriterIds)[i] =
            UA_ReserveId_createId(server, sessionId, transportProfileUri, UA_DATA_SET_WRITER);
    }
    return UA_STATUSCODE_GOOD;
}

/* Calculate the time difference between current time and UTC (00:00) on January
 * 1, 2000. */
UA_UInt32
UA_PubSubConfigurationVersionTimeDifference(UA_DateTime now) {
    UA_UInt32 timeDiffSince2000 = (UA_UInt32)(now - UA_DATETIMESTAMP_2000);
    return timeDiffSince2000;
}

static UA_StatusCode
addStandaloneSubscribedDataSet(UA_Server *server,
                               const UA_StandaloneSubscribedDataSetConfig *sdsConfig,
                               UA_NodeId *sdsIdentifier) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(!sdsConfig){
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "SubscribedDataSet creation failed. No config passed in.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StandaloneSubscribedDataSetConfig tmpSubscribedDataSetConfig;
    memset(&tmpSubscribedDataSetConfig, 0, sizeof(UA_StandaloneSubscribedDataSetConfig));
    if(UA_StandaloneSubscribedDataSetConfig_copy(sdsConfig, &tmpSubscribedDataSetConfig) != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "SubscribedDataSet creation failed. Configuration copy failed.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    //create new PDS and add to UA_PubSubManager
    UA_StandaloneSubscribedDataSet *newSubscribedDataSet = (UA_StandaloneSubscribedDataSet *)
            UA_calloc(1, sizeof(UA_StandaloneSubscribedDataSet));
    if(!newSubscribedDataSet) {
        UA_StandaloneSubscribedDataSetConfig_clear(&tmpSubscribedDataSetConfig);
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                     "SubscribedDataSet creation failed. Out of Memory.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    newSubscribedDataSet->config = tmpSubscribedDataSetConfig;
    newSubscribedDataSet->connectedReader = UA_NODEID_NULL;

    TAILQ_INSERT_TAIL(&server->pubSubManager.subscribedDataSets, newSubscribedDataSet, listEntry);
    server->pubSubManager.subscribedDataSetsSize++;

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    addStandaloneSubscribedDataSetRepresentation(server, newSubscribedDataSet);
#else
    UA_PubSubManager_generateUniqueNodeId(&server->pubSubManager, &newSubscribedDataSet->identifier);
#endif

    if(sdsIdentifier)
        UA_NodeId_copy(&newSubscribedDataSet->identifier, sdsIdentifier);

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addStandaloneSubscribedDataSet(UA_Server *server,
                                         const UA_StandaloneSubscribedDataSetConfig *sdsConfig,
                                         UA_NodeId *sdsIdentifier) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = addStandaloneSubscribedDataSet(server, sdsConfig, sdsIdentifier);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
removeStandaloneSubscribedDataSet(UA_Server *server, const UA_NodeId sds) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_StandaloneSubscribedDataSet *subscribedDataSet =
        UA_StandaloneSubscribedDataSet_findSDSbyId(server, sds);
    if(!subscribedDataSet){
        return UA_STATUSCODE_BADNOTFOUND;
    }

    //search for referenced readers.
    UA_PubSubConnection *tmpConnectoin;
    TAILQ_FOREACH(tmpConnectoin, &server->pubSubManager.connections, listEntry){
        UA_ReaderGroup *readerGroup;
        LIST_FOREACH(readerGroup, &tmpConnectoin->readerGroups, listEntry){
            UA_DataSetReader *currentReader, *tmpReader;
            LIST_FOREACH_SAFE(currentReader, &readerGroup->readers, listEntry, tmpReader){
                if(UA_NodeId_equal(&currentReader->identifier, &subscribedDataSet->connectedReader)){
                    UA_DataSetReader_remove(server, currentReader);
                    goto done;
                }
            }
        }
    }

 done:

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    deleteNode(server, subscribedDataSet->identifier, true);
#endif

    UA_StandaloneSubscribedDataSet_clear(server, subscribedDataSet);
    server->pubSubManager.subscribedDataSetsSize--;

    TAILQ_REMOVE(&server->pubSubManager.subscribedDataSets, subscribedDataSet, listEntry);
    UA_free(subscribedDataSet);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_removeStandaloneSubscribedDataSet(UA_Server *server, const UA_NodeId sds) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = removeStandaloneSubscribedDataSet(server, sds);
    UA_UNLOCK(&server->serviceMutex);
    return res;
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

static UA_UInt64
generateRandomUInt64(UA_Server *server) {
    UA_UInt64 id = 0;
    UA_Guid ident = UA_Guid_random();

    id = id + ident.data1;
    id = (id << 32) + ident.data2;
    id = (id << 16) + ident.data3;
    return id;
}

/* Initialization the PubSub configuration. */
void
UA_PubSubManager_init(UA_Server *server, UA_PubSubManager *pubSubManager) {
    //TODO: Using the Mac address to generate the defaultPublisherId.
    // In the future, this can be retrieved from the eventloop.
    pubSubManager->defaultPublisherId = generateRandomUInt64(server);

    TAILQ_INIT(&pubSubManager->connections);
    TAILQ_INIT(&pubSubManager->publishedDataSets);
    TAILQ_INIT(&pubSubManager->subscribedDataSets);
    TAILQ_INIT(&pubSubManager->topicAssign);

#ifdef UA_ENABLE_PUBSUB_SKS
    TAILQ_INIT(&pubSubManager->securityGroups);
#endif
}

void
UA_PubSubManager_shutdown(UA_Server *server, UA_PubSubManager *pubSubManager) {
    UA_PubSubConnection *tmpConnection;
    TAILQ_FOREACH(tmpConnection, &server->pubSubManager.connections, listEntry) {
        UA_PubSubConnection_setPubSubState(server, tmpConnection, UA_PUBSUBSTATE_DISABLED);
    }
}

/* Delete the current PubSub configuration including all nested members. This
 * action also delete the configured PubSub transport Layers. */
void
UA_PubSubManager_delete(UA_Server *server, UA_PubSubManager *pubSubManager) {
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "PubSub cleanup was called.");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Remove Connections - this also remove WriterGroups and ReaderGroups */
    UA_PubSubConnection *tmpConnection1, *tmpConnection2;
    TAILQ_FOREACH_SAFE(tmpConnection1, &server->pubSubManager.connections,
                       listEntry, tmpConnection2) {
        UA_PubSubConnection_delete(server, tmpConnection1);
    }

    /* Remove the DataSets */
    UA_PublishedDataSet *tmpPDS1, *tmpPDS2;
    TAILQ_FOREACH_SAFE(tmpPDS1, &server->pubSubManager.publishedDataSets,
                       listEntry, tmpPDS2){
        UA_PublishedDataSet_remove(server, tmpPDS1);
    }

    /* Remove the TopicAssigns */
    UA_TopicAssign *tmpTopicAssign1, *tmpTopicAssign2;
    TAILQ_FOREACH_SAFE(tmpTopicAssign1, &server->pubSubManager.topicAssign,
                       listEntry, tmpTopicAssign2){
        server->pubSubManager.topicAssignSize--;
        TAILQ_REMOVE(&server->pubSubManager.topicAssign, tmpTopicAssign1, listEntry);
        UA_free(tmpTopicAssign1);
    }

    /* Remove the ReserveIds*/
    ZIP_ITER(UA_ReserveIdTree, &server->pubSubManager.reserveIds, removeReserveId, NULL);
    server->pubSubManager.reserveIdsSize = 0;

    /* Delete subscribed datasets */
    UA_StandaloneSubscribedDataSet *tmpSDS1, *tmpSDS2;
    TAILQ_FOREACH_SAFE(tmpSDS1, &server->pubSubManager.subscribedDataSets, listEntry, tmpSDS2){
        removeStandaloneSubscribedDataSet(server, tmpSDS1->identifier);
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    /* Remove the SecurityGroups */
    UA_SecurityGroup *tmpSG1, *tmpSG2;
    TAILQ_FOREACH_SAFE(tmpSG1, &server->pubSubManager.securityGroups, listEntry, tmpSG2) {
        removeSecurityGroup(server, tmpSG1);
    }

    /* Remove the keyStorages */
    UA_PubSubKeyStorage *ks, *ksTmp;
    LIST_FOREACH_SAFE(ks, &server->pubSubManager.pubSubKeyList, keyStorageList, ksTmp) {
        UA_PubSubKeyStorage_delete(server, ks);
    }
#endif
}

#ifdef UA_ENABLE_PUBSUB_MONITORING

static UA_StatusCode
UA_PubSubComponent_createMonitoring(UA_Server *server, UA_NodeId Id,
                                    UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType,
                                    void *data, UA_ServerCallback callback) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT:
                    UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "UA_PubSubComponent_createMonitoring(): DataSetReader '%.*s' "
                                 "- MessageReceiveTimeout",
                                 (UA_Int32) reader->config.name.length,
                                 reader->config.name.data);
                    reader->msgRcvTimeoutTimerCallback = callback;
                    break;
                default:
                    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "UA_PubSubComponent_createMonitoring(): DataSetReader '%.*s' "
                                 "DataSetReader does not support timeout type '%i'",
                                 (UA_Int32) reader->config.name.length,
                                 reader->config.name.data, eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "Error UA_PubSubComponent_createMonitoring(): "
                         "PubSub component type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

static void
monitoringReceiveTimeoutOnce(UA_Server *server, void *data) {
    UA_LOCK(&server->serviceMutex);
    UA_DataSetReader *reader = (UA_DataSetReader*)data;
    reader->msgRcvTimeoutTimerCallback(server, reader);
    UA_EventLoop *el = server->config.eventLoop;
    el->removeCyclicCallback(el, reader->msgRcvTimeoutTimerId);
    reader->msgRcvTimeoutTimerId = 0;
    UA_UNLOCK(&server->serviceMutex);
}

static UA_StatusCode
UA_PubSubComponent_startMonitoring(UA_Server *server, UA_NodeId Id,
                                   UA_PubSubComponentEnumType eComponentType,
                                   UA_PubSubMonitoringType eMonitoringType, void *data) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT: {
                    /* No timeout configured */
                    if(reader->config.messageReceiveTimeout <= 0.0)
                        return UA_STATUSCODE_GOOD;

                    /* use a timed callback, because one notification is enough,
                     * we assume that MessageReceiveTimeout configuration is in
                     * [ms], we do not handle or check fractions */
                    UA_EventLoop *el = server->config.eventLoop;
                    ret = el->addCyclicCallback(el, (UA_Callback)monitoringReceiveTimeoutOnce,
                                                server, reader,
                                                reader->config.messageReceiveTimeout, NULL,
                                                UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
                                                &reader->msgRcvTimeoutTimerId);
                    if(ret == UA_STATUSCODE_GOOD) {
                        UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                                     "UA_PubSubComponent_startMonitoring(): DataSetReader "
                                     "'%.*s'- MessageReceiveTimeout: "
                                     "MessageReceiveTimeout = '%f' Timer Id = '%u'",
                                     (UA_Int32)reader->config.name.length,
                                     reader->config.name.data,
                                     reader->config.messageReceiveTimeout,
                                     (UA_UInt32)reader->msgRcvTimeoutTimerId);
                    } else {
                        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                     "Error UA_PubSubComponent_startMonitoring(): "
                                     "DataSetReader '%.*s' - MessageReceiveTimeout: "
                                     "start timer failed",
                                     (UA_Int32)reader->config.name.length,
                                     reader->config.name.data);
                    }
                    break;
                }
                default:
                    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "UA_PubSubComponent_startMonitoring(): DataSetReader '%.*s' "
                                 "DataSetReader does not support timeout type '%i'",
                                 (UA_Int32)reader->config.name.length,
                                 reader->config.name.data,
                                 eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "Error UA_PubSubComponent_startMonitoring(): PubSub component "
                         "type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

static UA_StatusCode
UA_PubSubComponent_stopMonitoring(UA_Server *server, UA_NodeId Id,
                                  UA_PubSubComponentEnumType eComponentType,
                                  UA_PubSubMonitoringType eMonitoringType, void *data) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT: {
                    UA_EventLoop *el = server->config.eventLoop;
                    el->removeCyclicCallback(el, reader->msgRcvTimeoutTimerId);
                    UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "UA_PubSubComponent_stopMonitoring(): DataSetReader '%.*s' - "
                                 "MessageReceiveTimeout: MessageReceiveTimeout = '%f' "
                                 "Timer Id = '%u'", (UA_Int32) reader->config.name.length,
                                 reader->config.name.data,
                                 reader->config.messageReceiveTimeout,
                                 (UA_UInt32) reader->msgRcvTimeoutTimerId);
                    break;
                }
                default:
                    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "UA_PubSubComponent_stopMonitoring(): DataSetReader '%.*s' "
                                 "DataSetReader does not support timeout type '%i'",
                                 (UA_Int32) reader->config.name.length,
                                 reader->config.name.data,
                        eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "Error UA_PubSubComponent_stopMonitoring(): "
                         "PubSub component type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

static UA_StatusCode
UA_PubSubComponent_updateMonitoringInterval(UA_Server *server, UA_NodeId Id,
                                            UA_PubSubComponentEnumType eComponentType,
                                            UA_PubSubMonitoringType eMonitoringType,
                                            void *data) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT: {
                    UA_EventLoop *el = server->config.eventLoop;
                    ret = el->modifyCyclicCallback(el, reader->msgRcvTimeoutTimerId,
                                                   reader->config.messageReceiveTimeout, NULL,
                                                   UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME);
                    if(ret == UA_STATUSCODE_GOOD) {
                        UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                                     "UA_PubSubComponent_updateMonitoringInterval(): "
                                     "DataSetReader '%.*s' - MessageReceiveTimeout: new "
                                     "MessageReceiveTimeout = '%f' Timer Id = '%u'",
                                     (UA_Int32) reader->config.name.length,
                                     reader->config.name.data,
                                     reader->config.messageReceiveTimeout,
                                     (UA_UInt32) reader->msgRcvTimeoutTimerId);
                    } else {
                        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                     "Error UA_PubSubComponent_updateMonitoringInterval(): "
                                     "DataSetReader '%.*s': update timer interval failed",
                                     (UA_Int32) reader->config.name.length,
                                     reader->config.name.data);
                    }
                    break;
                }
                default:
                    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "UA_PubSubComponent_createMonitoring(): DataSetReader '%.*s' "
                                 "DataSetReader does not support timeout type '%i'",
                                 (UA_Int32) reader->config.name.length,
                                 reader->config.name.data,
                                 eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "Error UA_PubSubComponent_updateMonitoringInterval(): "
                         "PubSub component type '%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

static UA_StatusCode
UA_PubSubComponent_deleteMonitoring(UA_Server *server, UA_NodeId Id,
                                    UA_PubSubComponentEnumType eComponentType,
                                    UA_PubSubMonitoringType eMonitoringType, void *data) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    switch (eComponentType) {
        case UA_PUBSUB_COMPONENT_DATASETREADER: {
            UA_DataSetReader *reader = (UA_DataSetReader*) data;
            switch (eMonitoringType) {
                case UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT:
                    UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "UA_PubSubComponent_deleteMonitoring(): DataSetReader "
                                 "'%.*s' - MessageReceiveTimeout: Timer Id = '%u'",
                                 (UA_Int32)reader->config.name.length,
                                 reader->config.name.data,
                                 (UA_UInt32)reader->msgRcvTimeoutTimerId);
                    break;
                default:
                    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "UA_PubSubComponent_deleteMonitoring(): DataSetReader '%.*s' "
                                 "DataSetReader does not support timeout type '%i'",
                                 (UA_Int32)reader->config.name.length,
                                 reader->config.name.data,
                        eMonitoringType);
                    ret = UA_STATUSCODE_BADNOTSUPPORTED;
                    break;
            }
            break;
        }
        default:
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_SERVER,
                         "Error UA_PubSubComponent_deleteMonitoring(): PubSub component type "
                         "'%i' is not supported", eComponentType);
            ret = UA_STATUSCODE_BADNOTSUPPORTED;
            break;
    }
    return ret;
}

UA_StatusCode
UA_PubSubManager_setDefaultMonitoringCallbacks(UA_PubSubMonitoringInterface *mif) {
    if(!mif)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    mif->createMonitoring = UA_PubSubComponent_createMonitoring;
    mif->startMonitoring = UA_PubSubComponent_startMonitoring;
    mif->stopMonitoring = UA_PubSubComponent_stopMonitoring;
    mif->updateMonitoringInterval = UA_PubSubComponent_updateMonitoringInterval;
    mif->deleteMonitoring = UA_PubSubComponent_deleteMonitoring;
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_PUBSUB_MONITORING */

#endif /* UA_ENABLE_PUBSUB */
