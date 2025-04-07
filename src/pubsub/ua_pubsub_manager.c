/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2017-2025 Fraunhofer IOSB (Author: Andreas Ebner)
 * Copyright (c) 2018 Fraunhofer IOSB (Author: Julius Pfrommer)
 * Copyright (c) 2021 Fraunhofer IOSB (Author: Jan Hermes)
 * Copyright (c) 2022 Siemens AG (Author: Thomas Fischer)
 * Copyright (c) 2022 Fraunhofer IOSB (Author: Noel Graf)
 * Copyright (c) 2022 Linutronix GmbH (Author: Muddasir Shakil)
 */

#include "ua_pubsub_internal.h"

#ifdef UA_ENABLE_PUBSUB /* conditional compilation */

#ifdef UA_ENABLE_PUBSUB_SKS
#include "ua_pubsub_keystorage.h"
#endif

#define UA_DATETIMESTAMP_2000 125911584000000000
#define UA_RESERVEID_FIRST_ID 0x8000

static const char *pubSubStateNames[6] = {
    "Disabled", "Paused", "Operational", "Error", "PreOperational", "Invalid"
};

static void
UA_PubSubManager_stop(UA_ServerComponent *sc);

static UA_StatusCode
UA_PubSubManager_start(UA_ServerComponent *sc, UA_Server *server);

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

UA_ConnectionManager *
getCM(UA_EventLoop *el, UA_String protocol) {
    for(UA_EventSource *es = el->eventSources; es != NULL; es = es->next) {
        if(es->eventSourceType != UA_EVENTSOURCETYPE_CONNECTIONMANAGER)
            continue;
        UA_ConnectionManager *cm = (UA_ConnectionManager*)es;
        if(UA_String_equal(&protocol, &cm->protocol))
            return cm;
    }
    return NULL;
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
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
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
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
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
UA_PubSubManager_generateUniqueGuid(UA_PubSubManager *psm) {
    while(true) {
        UA_NodeId testId = UA_NODEID_GUID(1, UA_Guid_random());
        const UA_Node *testNode = UA_NODESTORE_GET(psm->sc.server, &testId);
        if(!testNode)
            return testId.identifier.guid;
        UA_NODESTORE_RELEASE(psm->sc.server, testNode);
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

UA_StatusCode
UA_Server_enableAllPubSubComponents(UA_Server *server) {
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    if(!psm) {
        unlockServer(server);
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_StatusCode res = UA_PubSubManager_start(&psm->sc, server);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            UA_DataSetWriter *dsw;
            LIST_FOREACH(dsw, &wg->writers, listEntry) {
                res |= UA_DataSetWriter_setPubSubState(psm, dsw, UA_PUBSUBSTATE_OPERATIONAL);
            }
            res |= UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_OPERATIONAL);
        }

        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            UA_DataSetReader *dsr;
            LIST_FOREACH(dsr, &rg->readers, listEntry) {
                UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_OPERATIONAL,
                                                UA_STATUSCODE_GOOD);
            }
            res |= UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_OPERATIONAL);
        }

        res |= UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_OPERATIONAL);
    }

    unlockServer(server);
    return res;
}

static void
disableAllPubSubComponents(UA_PubSubManager *psm) {
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            UA_DataSetWriter *dsw;
            LIST_FOREACH(dsw, &wg->writers, listEntry) {
                UA_DataSetWriter_setPubSubState(psm, dsw, UA_PUBSUBSTATE_DISABLED);
            }
            UA_WriterGroup_setPubSubState(psm, wg, UA_PUBSUBSTATE_DISABLED);
        }

        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            UA_DataSetReader *dsr;
            LIST_FOREACH(dsr, &rg->readers, listEntry) {
                UA_DataSetReader_setPubSubState(psm, dsr, UA_PUBSUBSTATE_DISABLED,
                                                UA_STATUSCODE_BADSHUTDOWN);
            }
            UA_ReaderGroup_setPubSubState(psm, rg, UA_PUBSUBSTATE_DISABLED);
        }

        UA_PubSubConnection_setPubSubState(psm, c, UA_PUBSUBSTATE_DISABLED);
    }
}

void
UA_Server_disableAllPubSubComponents(UA_Server *server) {
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    if(psm)
        UA_PubSubManager_stop(&psm->sc); /* Calls disableAll internally */
    unlockServer(server);
}

static UA_StatusCode
getPubSubComponentType(UA_PubSubManager *psm, UA_NodeId componentId,
                       UA_PubSubComponentType *outType) {
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        if(UA_NodeId_equal(&componentId, &c->head.identifier)) {
            *outType = c->head.componentType;
            return UA_STATUSCODE_GOOD;
        }

        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            if(UA_NodeId_equal(&componentId, &wg->head.identifier)) {
                *outType = wg->head.componentType;
                return UA_STATUSCODE_GOOD;
            }

            UA_DataSetWriter *dsw;
            LIST_FOREACH(dsw, &wg->writers, listEntry) {
                if(UA_NodeId_equal(&componentId, &dsw->head.identifier)) {
                    *outType = dsw->head.componentType;
                    return UA_STATUSCODE_GOOD;
                }
            }
        }

        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            if(UA_NodeId_equal(&componentId, &rg->head.identifier)) {
                *outType = rg->head.componentType;
                return UA_STATUSCODE_GOOD;
            }

            UA_DataSetReader *dsr;
            LIST_FOREACH(dsr, &rg->readers, listEntry) {
                if(UA_NodeId_equal(&componentId, &dsr->head.identifier)) {
                    *outType = dsr->head.componentType;
                    return UA_STATUSCODE_GOOD;
                }
            }
        }
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_getPubSubComponentType(UA_Server *server, UA_NodeId componentId,
                                 UA_PubSubComponentType *outType) {
    if(!outType)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        getPubSubComponentType(psm, componentId, outType) : UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

static UA_StatusCode
getPubSubComponentParent(UA_PubSubManager *psm, UA_NodeId componentId,
                         UA_NodeId *outParent) {
    UA_PubSubConnection *c;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        if(UA_NodeId_equal(&componentId, &c->head.identifier))
            return UA_STATUSCODE_BADNOTSUPPORTED;

        UA_WriterGroup *wg;
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            if(UA_NodeId_equal(&componentId, &wg->head.identifier))
                return UA_NodeId_copy(&c->head.identifier, outParent);

            UA_DataSetWriter *dsw;
            LIST_FOREACH(dsw, &wg->writers, listEntry) {
                if(UA_NodeId_equal(&componentId, &dsw->head.identifier))
                    return UA_NodeId_copy(&wg->head.identifier, outParent);
            }
        }

        UA_ReaderGroup *rg;
        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            if(UA_NodeId_equal(&componentId, &rg->head.identifier))
                return UA_NodeId_copy(&c->head.identifier, outParent);

            UA_DataSetReader *dsr;
            LIST_FOREACH(dsr, &rg->readers, listEntry) {
                if(UA_NodeId_equal(&componentId, &dsr->head.identifier))
                    return UA_NodeId_copy(&rg->head.identifier, outParent);
            }
        }
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

UA_StatusCode
UA_Server_getPubSubComponentParent(UA_Server *server, UA_NodeId componentId,
                                   UA_NodeId *outParent) {
    if(!outParent)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        getPubSubComponentParent(psm, componentId, outParent) : UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

static UA_StatusCode
getPubSubComponentChildren(UA_PubSubManager *psm, UA_NodeId componentId,
                           size_t *outChildrenSize, UA_NodeId **outChildren) {
    UA_WriterGroup *wg;
    UA_ReaderGroup *rg;
    UA_DataSetWriter *dsw;
    UA_DataSetReader *dsr;
    UA_PubSubConnection *c;

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        if(UA_NodeId_equal(&componentId, &c->head.identifier)) {
            /* Count the children */
            size_t children = 0;
            LIST_FOREACH(wg, &c->writerGroups, listEntry)
                children++;
            LIST_FOREACH(rg, &c->readerGroups, listEntry)
                children++;

            /* Empty array? */
            if(children == 0) {
                *outChildren = NULL;
                *outChildrenSize = 0;
                return UA_STATUSCODE_GOOD;
            }

            /* Allocate the array */
            *outChildren = (UA_NodeId*)UA_calloc(children, sizeof(UA_NodeId));
            if(!*outChildren)
                return UA_STATUSCODE_BADOUTOFMEMORY;
            *outChildrenSize = children;

            /* Copy the NodeIds */
            size_t pos = 0;
            LIST_FOREACH(wg, &c->writerGroups, listEntry) {
                res |= UA_NodeId_copy(&wg->head.identifier, (*outChildren) + pos);
                pos++;
            }
            LIST_FOREACH(rg, &c->readerGroups, listEntry) {
                res |= UA_NodeId_copy(&rg->head.identifier, (*outChildren) + pos);
                pos++;
            }
            goto out;
        }

        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            if(UA_NodeId_equal(&componentId, &wg->head.identifier)) {
                /* Count the children */
                size_t children = 0;
                LIST_FOREACH(dsw, &wg->writers, listEntry)
                    children++;

                /* Empty array? */
                if(children == 0) {
                    *outChildren = NULL;
                    *outChildrenSize = 0;
                    return UA_STATUSCODE_GOOD;
                }

                /* Allocate the array */
                *outChildren = (UA_NodeId*)UA_calloc(children, sizeof(UA_NodeId));
                if(!*outChildren)
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                *outChildrenSize = children;

                /* Copy the NodeIds */
                size_t pos = 0;
                LIST_FOREACH(dsw, &wg->writers, listEntry) {
                    res |= UA_NodeId_copy(&dsw->head.identifier, (*outChildren) + pos);
                    pos++;
                }
                goto out;
            }

            /* DataSetWriter have no children (with a state machine) */
            LIST_FOREACH(dsw, &wg->writers, listEntry) {
                if(UA_NodeId_equal(&componentId, &dsw->head.identifier))
                    return UA_STATUSCODE_BADNOTSUPPORTED;
            }
        }

        LIST_FOREACH(rg, &c->readerGroups, listEntry) {
            if(UA_NodeId_equal(&componentId, &rg->head.identifier)) {
                /* Count the children */
                size_t children = 0;
                LIST_FOREACH(dsr, &rg->readers, listEntry)
                    children++;

                /* Empty array? */
                if(children == 0) {
                    *outChildren = NULL;
                    *outChildrenSize = 0;
                    return UA_STATUSCODE_GOOD;
                }

                /* Allocate the array */
                *outChildren = (UA_NodeId*)UA_calloc(children, sizeof(UA_NodeId));
                if(!*outChildren)
                    return UA_STATUSCODE_BADOUTOFMEMORY;
                *outChildrenSize = children;

                /* Copy the NodeIds */
                size_t pos = 0;
                LIST_FOREACH(dsr, &rg->readers, listEntry) {
                    res |= UA_NodeId_copy(&dsr->head.identifier, (*outChildren) + pos);
                    pos++;
                }
                goto out;
            }

            /* DataSetReader have no children (with a state machine) */
            LIST_FOREACH(dsr, &rg->readers, listEntry) {
                if(UA_NodeId_equal(&componentId, &dsr->head.identifier))
                    return UA_STATUSCODE_BADNOTSUPPORTED;
            }
        }
    }

    return UA_STATUSCODE_BADNOTFOUND;

 out:
    if(res != UA_STATUSCODE_GOOD)
        UA_Array_delete(*outChildren, *outChildrenSize, &UA_TYPES[UA_TYPES_NODEID]);
    return res;
}

UA_StatusCode
UA_Server_getPubSubComponentChildren(UA_Server *server, UA_NodeId componentId,
                                     size_t *outChildrenSize, UA_NodeId **outChildren) {
    if(!outChildrenSize || !outChildren)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        getPubSubComponentChildren(psm, componentId,
                                   outChildrenSize, outChildren) : UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

void
UA_PubSubManager_setState(UA_PubSubManager *psm, UA_LifecycleState state) {
    if(state == UA_LIFECYCLESTATE_STOPPED)
        state = UA_LIFECYCLESTATE_STOPPING;

    /* Check if all connections are closed if we are started */
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

    /* When we just started, trigger all connections to go from PAUSED to
     * OPERATIONAL */
    if(state == UA_LIFECYCLESTATE_STARTED) {
        UA_PubSubConnection *c;
        TAILQ_FOREACH(c, &psm->connections, listEntry) {
            UA_PubSubConnection_setPubSubState(psm, c, c->head.state);
        }
    }
}

static UA_StatusCode
UA_PubSubManager_start(UA_ServerComponent *sc, UA_Server *server) {
    UA_PubSubManager *psm = (UA_PubSubManager*)sc;
    if(psm->sc.state == UA_LIFECYCLESTATE_STOPPING) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "The PubSubManager is still stopping");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Re-cache for the case that the configuration has been updated */
    psm->logging = server->config.logging;

    UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STARTED);

    return UA_STATUSCODE_GOOD;
}

static void
UA_PubSubManager_stop(UA_ServerComponent *sc) {
    UA_PubSubManager *psm = (UA_PubSubManager*)sc;
    disableAllPubSubComponents(psm);
    UA_PubSubManager_setState(psm, UA_LIFECYCLESTATE_STOPPED);
}

UA_StatusCode
UA_PubSubManager_clear(UA_PubSubManager *psm) {
    if(psm->sc.state != UA_LIFECYCLESTATE_STOPPED) {
        UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                     "Cannot delete the PubSubManager because "
                     "it is not stopped");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_LOCK_ASSERT(&psm->sc.server->serviceMutex);

    /* Remove Connections - this also remove WriterGroups and ReaderGroups */
    UA_PubSubConnection *c, *tmpC;
    TAILQ_FOREACH_SAFE(c, &psm->connections, listEntry, tmpC) {
        UA_PubSubConnection_delete(psm, c);
    }

    /* Remove the DataSets */
    UA_PublishedDataSet *tmpPDS1, *tmpPDS2;
    TAILQ_FOREACH_SAFE(tmpPDS1, &psm->publishedDataSets, listEntry, tmpPDS2) {
        UA_PublishedDataSet_remove(psm, tmpPDS1);
    }

    /* Remove the ReserveIds*/
    ZIP_ITER(UA_ReserveIdTree, &psm->reserveIds, removeReserveId, NULL);
    psm->reserveIdsSize = 0;

    /* Delete subscribed datasets */
    UA_SubscribedDataSet *tmpSDS1, *tmpSDS2;
    TAILQ_FOREACH_SAFE(tmpSDS1, &psm->subscribedDataSets, listEntry, tmpSDS2) {
        UA_SubscribedDataSet_remove(psm, tmpSDS1);
    }

#ifdef UA_ENABLE_PUBSUB_SKS
    /* Remove the SecurityGroups */
    UA_SecurityGroup *tmpSG1, *tmpSG2;
    TAILQ_FOREACH_SAFE(tmpSG1, &psm->securityGroups, listEntry, tmpSG2) {
        UA_SecurityGroup_remove(psm, tmpSG1);
    }

    /* Remove the keyStorages */
    UA_PubSubKeyStorage *ks, *ksTmp;
    LIST_FOREACH_SAFE(ks, &psm->pubSubKeyList, keyStorageList, ksTmp) {
        UA_PubSubKeyStorage_delete(psm, ks);
    }
#endif

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
    psm->sc.clear = (UA_StatusCode (*)(UA_ServerComponent *))UA_PubSubManager_clear;

    /* Set the logging shortcut */
    psm->logging = server->config.logging;

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

    return &psm->sc;
}

#endif /* UA_ENABLE_PUBSUB */
