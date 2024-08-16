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

void
UA_PubSubComponentHead_clear(UA_PubSubComponentHead *psch) {
    UA_NodeId_clear(&psch->identifier);
    UA_String_clear(&psch->logIdString);
    memset(psch, 0, sizeof(UA_PubSubComponentHead));
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
UA_ReserveId_new(UA_UInt16 id, UA_String transportProfileUri,
                 UA_ReserveIdType reserveIdType, UA_NodeId sessionId) {
    UA_ReserveId *reserveId = (UA_ReserveId *)UA_calloc(1, sizeof(UA_ReserveId));
    if(!reserveId)
        return NULL;
    reserveId->id = id;
    reserveId->reserveIdType = reserveIdType;
    UA_String_copy(&transportProfileUri, &reserveId->transportProfileUri);
    reserveId->sessionId = sessionId;
    return reserveId;
}

static UA_Boolean
UA_ReserveId_isFree(UA_PubSubManager *psm, UA_UInt16 id, UA_String transportProfileUri,
                    UA_ReserveIdType reserveIdType) {
    /* Is the id already in use? */
    UA_ReserveId compare;
    compare.id = id;
    compare.reserveIdType = reserveIdType;
    compare.transportProfileUri = transportProfileUri;
    if(ZIP_FIND(UA_ReserveIdTree, &psm->reserveIds, &compare))
        return false;

    UA_PubSubConnection *tmpConnection;
    TAILQ_FOREACH(tmpConnection, &psm->connections, listEntry) {
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
UA_ReserveId_createId(UA_PubSubManager *psm,  UA_NodeId sessionId,
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
        is_free = UA_ReserveId_isFree(psm, next_id, transportProfileUri, reserveIdType);
        if(is_free)
            break;
        next_id++;
    }
    if(!is_free) {
        UA_LOG_ERROR(psm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                     "PubSub ReserveId creation failed. No free ID could be found.");
        return 0;
    }

    if(reserveIdType == UA_WRITER_GROUP)
        next_id_writerGroup = (UA_UInt16)(next_id + 1);
    else
        next_id_writer = (UA_UInt16)(next_id + 1);

    UA_ReserveId *reserveId =
        UA_ReserveId_new(next_id, transportProfileUri, reserveIdType, sessionId);
    if(!reserveId)
        return 0;

    ZIP_INSERT(UA_ReserveIdTree, &psm->reserveIds, reserveId);
    psm->reserveIdsSize++;
    return next_id;
}

static void *
removeReserveId(void *context, UA_ReserveId *elem) {
    UA_String_clear(&elem->transportProfileUri);
    UA_free(elem);
    return NULL;
}

struct RemoveInactiveReserveIdContext {
    UA_PubSubManager *psm;
    UA_ReserveIdTree newTree;
};

/* Remove ReserveIds that are not attached to any session */
static void *
removeInactiveReserveId(void *context, UA_ReserveId *elem) {
    struct RemoveInactiveReserveIdContext *ctx =
        (struct RemoveInactiveReserveIdContext*)context;

    if(UA_NodeId_equal(&ctx->psm->sc.server->adminSession.sessionId, &elem->sessionId))
        goto still_active;

    session_list_entry *session;
    LIST_FOREACH(session, &ctx->psm->sc.server->sessions, pointers) {
        if(UA_NodeId_equal(&session->session.sessionId, &elem->sessionId))
            goto still_active;
    }

    ctx->psm->reserveIdsSize--;
    UA_String_clear(&elem->transportProfileUri);
    UA_free(elem);
    return NULL;

 still_active:
    ZIP_INSERT(UA_ReserveIdTree, &ctx->newTree, elem);
    return NULL;
}

void
UA_PubSubManager_freeIds(UA_PubSubManager *psm) {
    struct RemoveInactiveReserveIdContext removeCtx;
    removeCtx.psm = psm;
    removeCtx.newTree.root = NULL;
    ZIP_ITER(UA_ReserveIdTree, &psm->reserveIds,
             removeInactiveReserveId, &removeCtx);
    psm->reserveIds = removeCtx.newTree;
}

UA_StatusCode
UA_PubSubManager_reserveIds(UA_PubSubManager *psm, UA_NodeId sessionId, UA_UInt16 numRegWriterGroupIds,
                            UA_UInt16 numRegDataSetWriterIds, UA_String transportProfileUri,
                            UA_UInt16 **writerGroupIds, UA_UInt16 **dataSetWriterIds) {
    UA_PubSubManager_freeIds(psm);

    /* Check the validation of the transportProfileUri */
    UA_String profile_1 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp");
    UA_String profile_2 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json");
    UA_String profile_3 = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    if(!UA_String_equal(&transportProfileUri, &profile_1) &&
       !UA_String_equal(&transportProfileUri, &profile_2) &&
       !UA_String_equal(&transportProfileUri, &profile_3)) {
        UA_LOG_ERROR(psm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                     "PubSub ReserveId creation failed. No valid transport profile uri.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    *writerGroupIds = (UA_UInt16*)UA_Array_new(numRegWriterGroupIds, &UA_TYPES[UA_TYPES_UINT16]);
    *dataSetWriterIds = (UA_UInt16*)UA_Array_new(numRegDataSetWriterIds, &UA_TYPES[UA_TYPES_UINT16]);

    for(int i = 0; i < numRegWriterGroupIds; i++) {
        (*writerGroupIds)[i] =
            UA_ReserveId_createId(psm, sessionId, transportProfileUri, UA_WRITER_GROUP);
    }
    for(int i = 0; i < numRegDataSetWriterIds; i++) {
        (*dataSetWriterIds)[i] =
            UA_ReserveId_createId(psm, sessionId, transportProfileUri, UA_DATA_SET_WRITER);
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
generateRandomUInt64(void) {
    UA_UInt64 id = 0;
    UA_Guid ident = UA_Guid_random();

    id = id + ident.data1;
    id = (id << 32) + ident.data2;
    id = (id << 16) + ident.data3;
    return id;
}

#ifdef UA_ENABLE_PUBSUB_MONITORING

static UA_StatusCode
PubSubMonitoring_create(UA_Server *server, UA_NodeId Id,
                        UA_PubSubComponentEnumType eComponentType,
                        UA_PubSubMonitoringType eMonitoringType,
                        void *data, UA_ServerCallback callback) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_DataSetReader *reader = (UA_DataSetReader*) data;

    if(eComponentType != UA_PUBSUB_COMPONENT_DATASETREADER) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_create: "
                            "PubSub component type '%i' is not supported",
                            eComponentType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    if(eMonitoringType != UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_create: "
                            "DataSetReader does not support timeout type '%i'",
                            eMonitoringType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    reader->msgRcvTimeoutTimerCallback = callback;

    UA_LOG_DEBUG_PUBSUB(server->config.logging, reader,
                        "PubSubMonitoring_create: "
                        "Set MessageReceiveTimeout callback");

    return UA_STATUSCODE_GOOD;
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
PubSubMonitoring_start(UA_Server *server, UA_NodeId Id,
                       UA_PubSubComponentEnumType eComponentType,
                       UA_PubSubMonitoringType eMonitoringType, void *data) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_DataSetReader *reader = (UA_DataSetReader*)data;

    if(eComponentType != UA_PUBSUB_COMPONENT_DATASETREADER) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_start: "
                            "Component type '%i' is not supported", eComponentType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    if(eMonitoringType != UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_start: "
                            "DataSetReader does not support timeout type '%i'",
                            eMonitoringType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    /* No timeout configured */
    if(reader->config.messageReceiveTimeout <= 0.0) {
        UA_LOG_WARNING_PUBSUB(server->config.logging, reader,
                              "PubSubMonitoring_start: "
                              "Cannot monitor timeout for messageReceiveTimeout <= 0");
        return UA_STATUSCODE_GOOD;
    }

    /* Use a timed callback, because one notification is enough, we assume that
     * MessageReceiveTimeout configuration is in [ms]. */
    UA_EventLoop *el = server->config.eventLoop;
    UA_StatusCode ret =
        el->addCyclicCallback(el, (UA_Callback)monitoringReceiveTimeoutOnce, server,
                              reader, reader->config.messageReceiveTimeout,
                              NULL, UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME,
                              &reader->msgRcvTimeoutTimerId);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_start: "
                            "MessageReceiveTimeout: Start timer failed");
        return ret;
    }

    UA_LOG_DEBUG_PUBSUB(server->config.logging, reader,
                        "PubSubMonitoring_start: "
                        "MessageReceiveTimeout = '%f' Timer Id = '%u'",
                        reader->config.messageReceiveTimeout,
                        (UA_UInt32)reader->msgRcvTimeoutTimerId);

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
PubSubMonitoring_stop(UA_Server *server, UA_NodeId Id,
                      UA_PubSubComponentEnumType eComponentType,
                      UA_PubSubMonitoringType eMonitoringType, void *data) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_DataSetReader *reader = (UA_DataSetReader*) data;

    if(eComponentType != UA_PUBSUB_COMPONENT_DATASETREADER) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_stop: "
                            "PubSub component type '%i' is not supported",
                            eComponentType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    if(eMonitoringType != UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_stop: "
                            "DataSetReader does not support timeout type '%i'",
                            eMonitoringType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    UA_LOG_DEBUG_PUBSUB(server->config.logging, reader,
                        "PubSubMonitoring_stop: "
                        "MessageReceiveTimeout: MessageReceiveTimeout = '%f' "
                        "Timer Id = '%u'", reader->config.messageReceiveTimeout,
                        (UA_UInt32)reader->msgRcvTimeoutTimerId);

    UA_EventLoop *el = server->config.eventLoop;
    el->removeCyclicCallback(el, reader->msgRcvTimeoutTimerId);
    reader->msgRcvTimeoutTimerId = 0;

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
PubSubMonitoring_updateMonitoringInterval(UA_Server *server, UA_NodeId Id,
                                          UA_PubSubComponentEnumType eComponentType,
                                          UA_PubSubMonitoringType eMonitoringType,
                                          void *data) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_DataSetReader *reader = (UA_DataSetReader*) data;

    if(eComponentType != UA_PUBSUB_COMPONENT_DATASETREADER) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_updateMonitoringInterval: "
                            "PubSub component type '%i' is not supported",
                            eComponentType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    if(eMonitoringType != UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_updateMonitoringInterval: "
                            "DataSetReader does not support timeout type '%i'",
                            eMonitoringType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    UA_EventLoop *el = server->config.eventLoop;
    UA_StatusCode ret =
        el->modifyCyclicCallback(el, reader->msgRcvTimeoutTimerId,
                                 reader->config.messageReceiveTimeout, NULL,
                                 UA_TIMER_HANDLE_CYCLEMISS_WITH_CURRENTTIME);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_updateMonitoringInterval: "
                            "Update timer interval failed");
        return ret;
    }

    UA_LOG_DEBUG_PUBSUB(server->config.logging, reader,
                        "PubSubMonitoring_updateMonitoringInterval: "
                        "MessageReceiveTimeout: new MessageReceiveTimeout "
                        "= '%f' Timer Id = '%u'",
                        reader->config.messageReceiveTimeout,
                        (UA_UInt32) reader->msgRcvTimeoutTimerId);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
PubSubMonitoring_delete(UA_Server *server, UA_NodeId Id,
                        UA_PubSubComponentEnumType eComponentType,
                        UA_PubSubMonitoringType eMonitoringType, void *data) {
    if(!server || !data)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    UA_DataSetReader *reader = (UA_DataSetReader*) data;

    if(eComponentType != UA_PUBSUB_COMPONENT_DATASETREADER) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_delete: "
                            "'%i' is not supported", eComponentType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    if(eMonitoringType != UA_PUBSUB_MONITORING_MESSAGE_RECEIVE_TIMEOUT) {
        UA_LOG_ERROR_PUBSUB(server->config.logging, reader,
                            "PubSubMonitoring_delete: "
                            "DataSetReader does not support timeout type '%i'",
                            eMonitoringType);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }

    /* This implementation only stops monitoring and does no other cleanup.
     * Other implementations might do it differently. */
    if(reader->msgRcvTimeoutTimerId != 0)
        PubSubMonitoring_stop(server, Id, eComponentType, eMonitoringType, data);

    UA_LOG_DEBUG_PUBSUB(server->config.logging, reader, "PubSubMonitoring_delete");

    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_PUBSUB_MONITORING */

void
UA_PubSubManager_setState(UA_PubSubManager *psm, UA_LifecycleState state) {
    if(state == UA_LIFECYCLESTATE_STOPPED)
        state = UA_LIFECYCLESTATE_STOPPING;

    /* Check if all connections are closed */
    if(state == UA_LIFECYCLESTATE_STOPPING) {
        UA_PubSubConnection *c;
        TAILQ_FOREACH(c, &psm->connections, listEntry) {
            if(c->sendChannel != 0 || c->recvChannelsSize > 0)
                goto set_state;

            UA_WriterGroup *wg;
            LIST_FOREACH(wg, &c->writerGroups, listEntry) {
                if(wg->sendChannel > 0)
                    goto set_state;
            }

            UA_ReaderGroup *rg;
            LIST_FOREACH(rg, &c->readerGroups, listEntry) {
                if(rg->recvChannelsSize > 0)
                    goto set_state;
            }
        }

        /* No open connections -> stopped */
        state = UA_LIFECYCLESTATE_STOPPED;
    }

 set_state:
    if(state == psm->sc.state)
        return;
    psm->sc.state = state;
    if(psm->sc.notifyState)
        psm->sc.notifyState(&psm->sc, state);
}

static UA_StatusCode
UA_PubSubManager_start(UA_ServerComponent *sc, UA_Server *server) {
    UA_PubSubManager *psm = (UA_PubSubManager*)sc;
    if(psm->sc.state == UA_LIFECYCLESTATE_STOPPING) {
        UA_LOG_ERROR(psm->sc.server->config.logging, UA_LOGCATEGORY_SERVER,
                     "The PubSubManager is still stopping");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STARTED);

    return UA_STATUSCODE_GOOD;
}

static void
UA_PubSubManager_stop(UA_ServerComponent *sc) {
    UA_PubSubManager *psm = (UA_PubSubManager*)sc;
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            UA_WriterGroup_setPubSubState(sc->server, wg, UA_PUBSUBSTATE_DISABLED);
        }

        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            UA_ReaderGroup_setPubSubState(sc->server, rg, UA_PUBSUBSTATE_DISABLED);
        }

        UA_PubSubConnection_setPubSubState(sc->server, c, UA_PUBSUBSTATE_DISABLED);
    }

    UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STOPPED);
}

void
UA_PubSubManager_clear(UA_PubSubManager *psm) {
    UA_Server *server = psm->sc.server;
    UA_LOG_INFO(server->config.logging, UA_LOGCATEGORY_SERVER,
                "PubSub cleanup was called.");
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Remove Connections - this also remove WriterGroups and ReaderGroups */
    UA_PubSubConnection *tmpConnection1, *tmpConnection2;
    TAILQ_FOREACH_SAFE(tmpConnection1, &psm->connections, listEntry, tmpConnection2) {
        UA_PubSubConnection_delete(server, tmpConnection1);
    }

    /* Remove the DataSets */
    UA_PublishedDataSet *tmpPDS1, *tmpPDS2;
    TAILQ_FOREACH_SAFE(tmpPDS1, &psm->publishedDataSets, listEntry, tmpPDS2){
        UA_PublishedDataSet_remove(server, tmpPDS1);
    }

    /* Remove the ReserveIds*/
    ZIP_ITER(UA_ReserveIdTree, &psm->reserveIds, removeReserveId, NULL);
    psm->reserveIdsSize = 0;

    /* Delete subscribed datasets */
    UA_StandaloneSubscribedDataSet *tmpSDS1, *tmpSDS2;
    TAILQ_FOREACH_SAFE(tmpSDS1, &psm->subscribedDataSets, listEntry, tmpSDS2) {
        UA_StandaloneSubscribedDataSet_remove(server, tmpSDS1);
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    /* Remove the SecurityGroups */
    UA_SecurityGroup *tmpSG1, *tmpSG2;
    TAILQ_FOREACH_SAFE(tmpSG1, &psm->securityGroups, listEntry, tmpSG2) {
        removeSecurityGroup(server, tmpSG1);
    }

    /* Remove the keyStorages */
    UA_PubSubKeyStorage *ks, *ksTmp;
    LIST_FOREACH_SAFE(ks, &psm->pubSubKeyList, keyStorageList, ksTmp) {
        UA_PubSubKeyStorage_delete(server, ks);
    }
#endif
}

/* Delete the current PubSub configuration including all nested members. This
 * action also delete the configured PubSub transport Layers. */
static UA_StatusCode
UA_PubSubManager_free(UA_ServerComponent *sc) {
    UA_PubSubManager *psm = (UA_PubSubManager*)sc;
    UA_PubSubManager_clear(psm);
    UA_free(psm);
    return UA_STATUSCODE_GOOD;
}

UA_ServerComponent *
UA_PubSubManager_new(UA_Server *server) {
    UA_PubSubManager *psm = (UA_PubSubManager*)UA_calloc(1, sizeof(UA_PubSubManager));
    if(!psm)
        return NULL;

    psm->sc.server = server;
    psm->sc.name = UA_STRING("pubsub");
    psm->sc.start = UA_PubSubManager_start;
    psm->sc.stop = UA_PubSubManager_stop;
    psm->sc.free = UA_PubSubManager_free;

    /* TODO: Using the Mac address to generate the defaultPublisherId.
     * In the future, this can be retrieved from the Eventloop. */
    psm->defaultPublisherId = generateRandomUInt64();

    TAILQ_INIT(&psm->connections);
    TAILQ_INIT(&psm->publishedDataSets);
    TAILQ_INIT(&psm->subscribedDataSets);

#ifdef UA_ENABLE_PUBSUB_SKS
    TAILQ_INIT(&psm->securityGroups);
#endif

#ifdef UA_ENABLE_PUBSUB_INFORMATIONMODEL
    /* Build PubSub information model */
    initPubSubNS0(server);
#endif

#ifdef UA_ENABLE_PUBSUB_MONITORING
    /* Setup default PubSub monitoring callbacks */
    UA_PubSubMonitoringInterface *mif = &server->config.pubSubConfig.monitoringInterface;
    mif->createMonitoring = PubSubMonitoring_create;
    mif->startMonitoring = PubSubMonitoring_start;
    mif->stopMonitoring = PubSubMonitoring_stop;
    mif->updateMonitoringInterval = PubSubMonitoring_updateMonitoringInterval;
    mif->deleteMonitoring = PubSubMonitoring_delete;
#endif /* UA_ENABLE_PUBSUB_MONITORING */

    return &psm->sc;
}

#endif /* UA_ENABLE_PUBSUB */
