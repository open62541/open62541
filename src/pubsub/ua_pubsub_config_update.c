/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2026 (c) o6 Automation GmbH (Author: Andreas Ebner)
 */

#include <open62541/server_pubsub.h>

#if defined(UA_ENABLE_PUBSUB) && defined(UA_ENABLE_PUBSUB_FILE_CONFIG)

#include "ua_pubsub_internal.h"

/* Update of the PubSub configuration with the semantics of the Part 14 v1.05
 * CloseAndUpdate method (9.1.3.7.6). The update file provides the
 * configuration elements, the references select the elements and the
 * operation (add/match/modify/remove).
 *
 * Limitations of this implementation (documented in the result codes):
 * - SecurityGroup and PushTarget references are not supported
 *   (Bad_ResourceUnavailable per element).
 * - Modify of PublishedDataSets and SubscribedDataSets is not supported
 *   (Bad_NotImplemented per element) -- changing the field list of a PDS
 *   requires recreating it (remove + add in one call).
 * - With requireCompleteUpdate the referenced elements are converted first.
 *   Nothing is changed if an element cannot be converted. Later failures roll
 *   back the applied operations. The rollback recreates removed components
 *   with new NodeIds, restarts their sequence numbers and does not call the
 *   componentLifecycleCallback.
 * - The automatic WriterGroupId/DataSetWriterId assignment skips the ids
 *   reserved with ReserveIds, but the caller cannot use its own reservations
 *   (no session context in the C API). Reserved ids can be given explicitly
 *   in the file elements. */

#define UA_REFMASK_OPBITS                                   \
    (UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |             \
     UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH |           \
     UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY |          \
     UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE)

#define UA_REFMASK_REFBITS                                  \
    (UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER |        \
     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER |        \
     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP |   \
     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP |   \
     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION |    \
     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET |    \
     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESUBDATASET |    \
     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESECURITYGROUP | \
     UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUSHTARGET)

#define UA_REF_CONNECTION UA_PUBSUBCONFIGURATIONREFMASK_REFERENCECONNECTION
#define UA_REF_WRITERGROUP UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITERGROUP
#define UA_REF_READERGROUP UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADERGROUP
#define UA_REF_WRITER UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEWRITER
#define UA_REF_READER UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEREADER
#define UA_REF_PUBDATASET UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUBDATASET
#define UA_REF_SUBDATASET UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESUBDATASET

/* The configs of the components. For a PublishedDataSet also the fields and
 * the metadata. */
typedef union {
    UA_PubSubConnectionConfig conn;
    UA_WriterGroupConfig wg;
    UA_DataSetWriterConfig dsw;
    UA_ReaderGroupConfig rg;
    UA_DataSetReaderConfig dsr;
    struct {
        UA_PublishedDataSetConfig config;
        UA_PublishedDataSetDataType data;
    } pds;
    UA_SubscribedDataSetConfig sds;
} UA_ComponentConfig;

/* One reference resolved into the update file */
typedef struct {
    const UA_PubSubConfigurationRefDataType *ref;
    size_t inputIndex;
    UA_UInt32 op;     /* the operation bits */
    UA_UInt32 refbit; /* the single reference bit */
    UA_StatusCode status;
    UA_Boolean indicesValid; /* the indices resolved into the file */
    UA_Boolean matched;      /* applied as a match of an existing element */
    size_t valuesBegin;      /* configurationValues added by the operation */
    size_t valuesEnd;

    /* Resolved file elements. The connection/group entries are also resolved
     * for child references (they provide the parent names). */
    const UA_PubSubConnectionDataType *fileConn;
    const UA_WriterGroupDataType *fileWg;
    const UA_ReaderGroupDataType *fileRg;
    const UA_DataSetWriterDataType *fileDsw;
    const UA_DataSetReaderDataType *fileDsr;
    const UA_PublishedDataSetDataType *filePds;
    const UA_StandaloneSubscribedDataSetDataType *fileSsds;

    /* The element converted for Add and Modify (a view into the file) */
    UA_StatusCode convertStatus;
    UA_ComponentConfig config;
} UA_ConfigUpdateOp;

static UA_PubSubComponentType
refType(UA_UInt32 refbit) {
    switch(refbit) {
    case UA_REF_CONNECTION: return UA_PUBSUBCOMPONENT_CONNECTION;
    case UA_REF_WRITERGROUP: return UA_PUBSUBCOMPONENT_WRITERGROUP;
    case UA_REF_READERGROUP: return UA_PUBSUBCOMPONENT_READERGROUP;
    case UA_REF_WRITER: return UA_PUBSUBCOMPONENT_DATASETWRITER;
    case UA_REF_READER: return UA_PUBSUBCOMPONENT_DATASETREADER;
    case UA_REF_PUBDATASET: return UA_PUBSUBCOMPONENT_PUBLISHEDDATASET;
    default: return UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET;
    }
}

/* The name of the referenced file element */
static UA_String
opName(const UA_ConfigUpdateOp *op) {
    switch(op->refbit) {
    case UA_REF_CONNECTION: return op->fileConn->name;
    case UA_REF_WRITERGROUP: return op->fileWg->name;
    case UA_REF_READERGROUP: return op->fileRg->name;
    case UA_REF_WRITER: return op->fileDsw->name;
    case UA_REF_READER: return op->fileDsr->name;
    case UA_REF_PUBDATASET: return op->filePds->name;
    default: return op->fileSsds->name;
    }
}

/* The name and the enabled flag in the converted config */
static UA_String *
configName(UA_ComponentConfig *config, UA_PubSubComponentType type) {
    switch(type) {
    case UA_PUBSUBCOMPONENT_CONNECTION: return &config->conn.name;
    case UA_PUBSUBCOMPONENT_WRITERGROUP: return &config->wg.name;
    case UA_PUBSUBCOMPONENT_READERGROUP: return &config->rg.name;
    case UA_PUBSUBCOMPONENT_DATASETWRITER: return &config->dsw.name;
    case UA_PUBSUBCOMPONENT_DATASETREADER: return &config->dsr.name;
    case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET: return &config->pds.config.name;
    default: return &config->sds.name;
    }
}

static UA_Boolean *
configEnabled(UA_ComponentConfig *config, UA_PubSubComponentType type) {
    switch(type) {
    case UA_PUBSUBCOMPONENT_CONNECTION: return &config->conn.enabled;
    case UA_PUBSUBCOMPONENT_WRITERGROUP: return &config->wg.enabled;
    case UA_PUBSUBCOMPONENT_READERGROUP: return &config->rg.enabled;
    case UA_PUBSUBCOMPONENT_DATASETWRITER: return &config->dsw.enabled;
    case UA_PUBSUBCOMPONENT_DATASETREADER: return &config->dsr.enabled;
    default: return NULL; /* The datasets have no state */
    }
}

/* The config of a live component */
static UA_ComponentConfig *
componentConfig(UA_PubSubComponentHead *head) {
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION:
        return (UA_ComponentConfig*)&((UA_PubSubConnection*)head)->config;
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        return (UA_ComponentConfig*)&((UA_WriterGroup*)head)->config;
    case UA_PUBSUBCOMPONENT_READERGROUP:
        return (UA_ComponentConfig*)&((UA_ReaderGroup*)head)->config;
    case UA_PUBSUBCOMPONENT_DATASETWRITER:
        return (UA_ComponentConfig*)&((UA_DataSetWriter*)head)->config;
    case UA_PUBSUBCOMPONENT_DATASETREADER:
        return (UA_ComponentConfig*)&((UA_DataSetReader*)head)->config;
    case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET:
        return (UA_ComponentConfig*)&((UA_PublishedDataSet*)head)->config;
    default:
        return (UA_ComponentConfig*)&((UA_SubscribedDataSet*)head)->config;
    }
}

/* Connections and groups with open channels are removed later. They are no
 * longer part of the configuration. */
static UA_Boolean
pendingDelete(const UA_PubSubComponentHead *head) {
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION:
        return ((const UA_PubSubConnection*)head)->deleteFlag;
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        return ((const UA_WriterGroup*)head)->deleteFlag;
    case UA_PUBSUBCOMPONENT_READERGROUP:
        return ((const UA_ReaderGroup*)head)->deleteFlag;
    default:
        return false;
    }
}

static UA_PubSubComponentHead *
parentOf(UA_PubSubComponentHead *head) {
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        return &((UA_WriterGroup*)head)->linkedConnection->head;
    case UA_PUBSUBCOMPONENT_READERGROUP:
        return &((UA_ReaderGroup*)head)->linkedConnection->head;
    case UA_PUBSUBCOMPONENT_DATASETWRITER:
        return &((UA_DataSetWriter*)head)->linkedWriterGroup->head;
    case UA_PUBSUBCOMPONENT_DATASETREADER:
        return &((UA_DataSetReader*)head)->linkedReaderGroup->head;
    default:
        return NULL;
    }
}

/* Find a component by name in the parent. The top-level components have no
 * parent. An empty name never matches. */
static UA_PubSubComponentHead *
findByName(UA_PubSubManager *psm, UA_PubSubComponentHead *parent,
           UA_UInt32 refbit, const UA_String name) {
    void *found = NULL;
    if(UA_String_isEmpty(&name))
        return NULL;
    switch(refbit) {
    case UA_REF_CONNECTION:
        found = UA_PubSubConnection_findByName(psm, name);
        break;
    case UA_REF_WRITERGROUP:
        found = UA_WriterGroup_findByName((UA_PubSubConnection*)parent, name);
        break;
    case UA_REF_READERGROUP:
        found = UA_ReaderGroup_findByName((UA_PubSubConnection*)parent, name);
        break;
    case UA_REF_WRITER:
        found = UA_DataSetWriter_findByName((UA_WriterGroup*)parent, name);
        break;
    case UA_REF_READER:
        found = UA_DataSetReader_findByName((UA_ReaderGroup*)parent, name);
        break;
    case UA_REF_PUBDATASET:
        found = UA_PublishedDataSet_findByName(psm, name);
        break;
    default:
        found = UA_SubscribedDataSet_findByName(psm, name);
        break;
    }
    return (UA_PubSubComponentHead*)found;
}

static void
setState(UA_PubSubManager *psm, UA_PubSubComponentHead *head,
         UA_PubSubState state) {
    UA_PubSubComponent_setPubSubState(psm, head, head->componentType,
                                      state, UA_STATUSCODE_GOOD);
}

/* Enable a component. The connection opens the shared channels for the
 * groups. Trigger it before a group is enabled. */
static void
enableComponent(UA_PubSubManager *psm, UA_PubSubComponentHead *head,
                UA_PubSubState state) {
    UA_PubSubComponentType type = head->componentType;
    if(type == UA_PUBSUBCOMPONENT_WRITERGROUP ||
       type == UA_PUBSUBCOMPONENT_READERGROUP) {
        UA_PubSubComponentHead *c = parentOf(head);
        if(UA_PubSubState_isEnabled(c->state))
            setState(psm, c, c->state);
    }
    setState(psm, head, state);
}

/* Update the config of a disabled component */
static UA_StatusCode
updateComponentConfig(UA_PubSubManager *psm, UA_PubSubComponentHead *head,
                      const UA_ComponentConfig *config) {
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION:
        return UA_PubSubConnection_updateConfig(psm, (UA_PubSubConnection*)head,
                                                &config->conn);
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        return UA_WriterGroup_updateConfig(psm, (UA_WriterGroup*)head, &config->wg);
    case UA_PUBSUBCOMPONENT_DATASETWRITER:
        return UA_DataSetWriter_updateConfig(psm, (UA_DataSetWriter*)head,
                                             &config->dsw);
    case UA_PUBSUBCOMPONENT_READERGROUP:
        return UA_ReaderGroup_updateConfig(psm, (UA_ReaderGroup*)head, &config->rg);
    case UA_PUBSUBCOMPONENT_DATASETREADER:
        return UA_DataSetReader_updateConfig(psm, (UA_DataSetReader*)head,
                                             &config->dsr);
    default:
        return UA_STATUSCODE_BADNOTIMPLEMENTED;
    }
}

/*****************************/
/* Update context / undo log */
/*****************************/

/* The undo log records the changes of one update call. When the call
 * completes, the ADDED components are enabled and the temporarily disabled
 * components (STATE) get their prior state back. When a complete update
 * fails, the changes are undone in reverse order: ADDED components are
 * removed, MODIFIED components get their prior config back and REMOVED
 * components are recreated from a snapshot (with new NodeIds). */
typedef enum {
    UA_UNDO_ADDED,
    UA_UNDO_STATE,
    UA_UNDO_MODIFIED,
    UA_UNDO_REMOVED
} UA_UndoKind;

/* The config of a component. For a removed component also the state, the
 * list position and the children (the groups of a connection, the
 * writers/readers of a group). */
typedef struct UA_Snapshot {
    UA_PubSubComponentType type;
    UA_NodeId id;
    UA_PubSubState state;
    UA_ComponentConfig config;
    size_t position;       /* in the list of the parent */
    UA_String dataSetName; /* DataSetWriter: the connected PDS */
    size_t childrenSize;
    struct UA_Snapshot *children;
} UA_Snapshot;

typedef struct {
    UA_UndoKind kind;
    UA_PubSubComponentType type;
    UA_NodeId id;
    UA_NodeId parent;           /* REMOVED */
    UA_PubSubState priorState;  /* STATE */
    UA_Boolean enableOnCommit;  /* ADDED */
    UA_Snapshot *snapshot;      /* MODIFIED, REMOVED */
} UA_UndoEntry;

/* NodeIds of the components recreated in a rollback */
typedef struct {
    UA_NodeId oldId;
    UA_NodeId newId;
} UA_IdRemap;

/* The file elements that can be parents (connections and groups) are bound to
 * the live component they were added, matched or modified to in this call.
 * Children resolve their parent through the binding (Part 14 9.1.3.7.3). */
typedef enum {
    UA_BIND_UNREFERENCED = 0, /* not referenced (yet): resolved by name */
    UA_BIND_BOUND,            /* added, matched or modified */
    UA_BIND_REMOVED,          /* removed in this call */
    UA_BIND_FAILED            /* the operation on the element failed */
} UA_BindState;

typedef struct {
    UA_BindState state;
    UA_NodeId id;
} UA_ElementBinding;

typedef struct {
    UA_ElementBinding conn;
    UA_ElementBinding *wgs; /* writerGroupsSize of the file connection */
    UA_ElementBinding *rgs; /* readerGroupsSize of the file connection */
} UA_ConnectionBinding;

typedef struct {
    UA_PubSubManager *psm;
    const UA_PubSubConfiguration2DataType *cfg;
    UA_PubSubConfigurationUpdateResult *result;
    UA_ConnectionBinding *bind; /* cfg->connectionsSize entries */
    UA_Boolean atomic; /* record the changes for a rollback */
    size_t logSize;
    size_t logCapacity;
    UA_UndoEntry *log;
    size_t remapSize;
    UA_IdRemap *remap;
} UA_UpdateCtx;

/* Ensure that the log can take more entries. Then a change is never made
 * without being recorded. */
static UA_StatusCode
logReserve(UA_UpdateCtx *ctx, size_t entries) {
    if(ctx->logSize + entries <= ctx->logCapacity)
        return UA_STATUSCODE_GOOD;
    size_t newCapacity = (ctx->logCapacity == 0) ? 16 : ctx->logCapacity * 2;
    while(newCapacity < ctx->logSize + entries)
        newCapacity *= 2;
    UA_UndoEntry *newLog = (UA_UndoEntry*)
        UA_realloc(ctx->log, newCapacity * sizeof(UA_UndoEntry));
    if(!newLog)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    ctx->log = newLog;
    ctx->logCapacity = newCapacity;
    return UA_STATUSCODE_GOOD;
}

static UA_UndoEntry *
logAppend(UA_UpdateCtx *ctx, UA_UndoKind kind, UA_PubSubComponentType type,
          const UA_NodeId id) {
    if(logReserve(ctx, 1) != UA_STATUSCODE_GOOD)
        return NULL;
    UA_UndoEntry *e = &ctx->log[ctx->logSize++];
    memset(e, 0, sizeof(UA_UndoEntry));
    e->kind = kind;
    e->type = type;
    e->id = id; /* Numeric NodeIds of the components, no deep copy needed */
    return e;
}

/* Disable an enabled component for the duration of the update call. The
 * prior state is recorded once and restored when the call completes. */
static UA_StatusCode
ensureDisabled(UA_UpdateCtx *ctx, UA_PubSubComponentHead *head) {
    if(!UA_PubSubState_isEnabled(head->state))
        return UA_STATUSCODE_GOOD;
    UA_UndoEntry *e = logAppend(ctx, UA_UNDO_STATE, head->componentType,
                                head->identifier);
    if(!e)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    e->priorState = head->state;
    setState(ctx->psm, head, UA_PUBSUBSTATE_DISABLED);
    return UA_STATUSCODE_GOOD;
}

/* The state of a component before the update call */
static UA_PubSubState
effectiveState(const UA_UpdateCtx *ctx, const UA_PubSubComponentHead *head) {
    for(size_t i = 0; i < ctx->logSize; i++) {
        const UA_UndoEntry *e = &ctx->log[i];
        if(e->kind == UA_UNDO_STATE && UA_NodeId_equal(&e->id, &head->identifier))
            return e->priorState;
    }
    return head->state;
}

static UA_Boolean
isAddedInCall(const UA_UpdateCtx *ctx, const UA_NodeId id) {
    for(size_t i = 0; i < ctx->logSize; i++) {
        if(ctx->log[i].kind == UA_UNDO_ADDED &&
           UA_NodeId_equal(&ctx->log[i].id, &id))
            return true;
    }
    return false;
}

/* Enable the added components (in the order they were added, parents before
 * children) and restore the prior state of the temporarily disabled
 * components (in reverse order). The NodeId of a removed component can be
 * reused for an added component. Such STATE entries are skipped. */
static void
commitUpdate(UA_UpdateCtx *ctx) {
    UA_PubSubManager *psm = ctx->psm;
    for(size_t i = 0; i < ctx->logSize; i++) {
        UA_UndoEntry *e = &ctx->log[i];
        if(e->kind != UA_UNDO_ADDED || !e->enableOnCommit)
            continue;
        UA_PubSubComponentHead *head = UA_PubSubComponent_find(psm, e->type, e->id);
        if(head)
            enableComponent(psm, head, UA_PUBSUBSTATE_OPERATIONAL);
    }

    for(size_t i = ctx->logSize; i > 0; i--) {
        UA_UndoEntry *e = &ctx->log[i - 1];
        if(e->kind != UA_UNDO_STATE || isAddedInCall(ctx, e->id))
            continue;
        UA_PubSubComponentHead *head = UA_PubSubComponent_find(psm, e->type, e->id);
        if(head)
            setState(psm, head, e->priorState);
    }
}

/*************/
/* Snapshots */
/*************/

/* The position of a component in the list of its parent. Components pending
 * deletion are not counted. The writers are ordered by their id. */
#define UA_LIST_REMOVE_ENTRY(LISTHEAD, ELM) LIST_REMOVE(ELM, listEntry)
#define UA_TAILQ_REMOVE_ENTRY(LISTHEAD, ELM) TAILQ_REMOVE(LISTHEAD, ELM, listEntry)
#define UA_LIST_INSERT_AFTER_ENTRY(LISTHEAD, PREV, ELM) \
    LIST_INSERT_AFTER(PREV, ELM, listEntry)
#define UA_TAILQ_INSERT_AFTER_ENTRY(LISTHEAD, PREV, ELM) \
    TAILQ_INSERT_AFTER(LISTHEAD, PREV, ELM, listEntry)

#define UA_LISTPOS_FIND(LISTTYPE, ELMTYPE, LISTHEAD, ELM, POS) do { \
        ELMTYPE *it_;                                               \
        LISTTYPE##_FOREACH(it_, LISTHEAD, listEntry) {              \
            if(it_ == (ELMTYPE*)(ELM))                              \
                break;                                              \
            (POS) += (pendingDelete(&it_->head)) ? 0 : 1;           \
        }                                                           \
    } while(0)

/* Insert behind the element with the position-th count */
#define UA_LISTPOS_MOVE(LISTTYPE, ELMTYPE, LISTHEAD, ELM, POSITION) do {   \
        ELMTYPE *it_, *prev_ = NULL, *elm_ = (ELMTYPE*)(ELM);              \
        size_t pos_ = 0;                                                   \
        UA_##LISTTYPE##_REMOVE_ENTRY(LISTHEAD, elm_);                      \
        LISTTYPE##_FOREACH(it_, LISTHEAD, listEntry) {                     \
            if(pos_ == (POSITION))                                         \
                break;                                                     \
            prev_ = it_;                                                   \
            pos_ += (pendingDelete(&it_->head)) ? 0 : 1;                   \
        }                                                                  \
        if(prev_)                                                          \
            UA_##LISTTYPE##_INSERT_AFTER_ENTRY(LISTHEAD, prev_, elm_);     \
        else                                                               \
            LISTTYPE##_INSERT_HEAD(LISTHEAD, elm_, listEntry);             \
    } while(0)

static size_t
listPosition(UA_PubSubManager *psm, UA_PubSubComponentHead *head) {
    UA_PubSubComponentHead *parent = parentOf(head);
    size_t pos = 0;
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION:
        UA_LISTPOS_FIND(TAILQ, UA_PubSubConnection, &psm->connections, head, pos);
        break;
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        UA_LISTPOS_FIND(LIST, UA_WriterGroup,
                        &((UA_PubSubConnection*)parent)->writerGroups, head, pos);
        break;
    case UA_PUBSUBCOMPONENT_READERGROUP:
        UA_LISTPOS_FIND(LIST, UA_ReaderGroup,
                        &((UA_PubSubConnection*)parent)->readerGroups, head, pos);
        break;
    case UA_PUBSUBCOMPONENT_DATASETREADER:
        UA_LISTPOS_FIND(LIST, UA_DataSetReader,
                        &((UA_ReaderGroup*)parent)->readers, head, pos);
        break;
    case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET:
        UA_LISTPOS_FIND(TAILQ, UA_PublishedDataSet, &psm->publishedDataSets,
                        head, pos);
        break;
    case UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET:
        UA_LISTPOS_FIND(TAILQ, UA_SubscribedDataSet, &psm->subscribedDataSets,
                        head, pos);
        break;
    default:
        break; /* The writers are ordered by their id */
    }
    return pos;
}

static void
moveToPosition(UA_PubSubManager *psm, UA_PubSubComponentHead *head,
               size_t position) {
    UA_PubSubComponentHead *parent = parentOf(head);
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION:
        UA_LISTPOS_MOVE(TAILQ, UA_PubSubConnection, &psm->connections,
                        head, position);
        break;
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        UA_LISTPOS_MOVE(LIST, UA_WriterGroup,
                        &((UA_PubSubConnection*)parent)->writerGroups, head, position);
        break;
    case UA_PUBSUBCOMPONENT_READERGROUP:
        UA_LISTPOS_MOVE(LIST, UA_ReaderGroup,
                        &((UA_PubSubConnection*)parent)->readerGroups, head, position);
        break;
    case UA_PUBSUBCOMPONENT_DATASETREADER:
        UA_LISTPOS_MOVE(LIST, UA_DataSetReader,
                        &((UA_ReaderGroup*)parent)->readers, head, position);
        break;
    case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET:
        UA_LISTPOS_MOVE(TAILQ, UA_PublishedDataSet, &psm->publishedDataSets,
                        head, position);
        break;
    case UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET:
        UA_LISTPOS_MOVE(TAILQ, UA_SubscribedDataSet, &psm->subscribedDataSets,
                        head, position);
        break;
    default:
        break;
    }
}

static void
snapshotClear(UA_Snapshot *s) {
    for(size_t i = 0; i < s->childrenSize; i++)
        snapshotClear(&s->children[i]);
    UA_free(s->children);
    UA_ComponentConfig *c = &s->config;
    switch(s->type) {
    case UA_PUBSUBCOMPONENT_CONNECTION: UA_PubSubConnectionConfig_clear(&c->conn); break;
    case UA_PUBSUBCOMPONENT_WRITERGROUP: UA_WriterGroupConfig_clear(&c->wg); break;
    case UA_PUBSUBCOMPONENT_DATASETWRITER: UA_DataSetWriterConfig_clear(&c->dsw); break;
    case UA_PUBSUBCOMPONENT_READERGROUP: UA_ReaderGroupConfig_clear(&c->rg); break;
    case UA_PUBSUBCOMPONENT_DATASETREADER: UA_DataSetReaderConfig_clear(&c->dsr); break;
    case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET:
        UA_PublishedDataSetConfig_clear(&c->pds.config);
        UA_PublishedDataSetDataType_clear(&c->pds.data);
        break;
    case UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET:
        UA_SubscribedDataSetConfig_clear(&c->sds);
        break;
    default: break;
    }
    UA_String_clear(&s->dataSetName);
}

static UA_StatusCode
snapshotTake(UA_PubSubManager *psm, UA_PubSubComponentHead *head,
             UA_Boolean withChildren, UA_Snapshot *s);

/* Snapshot the children in the list, pending deletions are skipped. The
 * children array is zeroed, a partially filled snapshot can be deleted. */
#define UA_SNAPSHOT_CHILDREN(LISTHEAD, ELMTYPE) do {                         \
        ELMTYPE *it_;                                                        \
        size_t size_ = s->childrenSize;                                      \
        LIST_FOREACH(it_, LISTHEAD, listEntry)                               \
            size_ += (pendingDelete(&it_->head)) ? 0 : 1;                    \
        if(size_ == s->childrenSize)                                         \
            break;                                                           \
        UA_Snapshot *c_ = (UA_Snapshot*)                                     \
            UA_realloc(s->children, size_ * sizeof(UA_Snapshot));            \
        if(!c_)                                                              \
            return UA_STATUSCODE_BADOUTOFMEMORY;                             \
        memset(&c_[s->childrenSize], 0,                                      \
               (size_ - s->childrenSize) * sizeof(UA_Snapshot));             \
        s->children = c_;                                                    \
        LIST_FOREACH(it_, LISTHEAD, listEntry) {                             \
            if(pendingDelete(&it_->head))                                    \
                continue;                                                    \
            UA_StatusCode res_ = snapshotTake(psm, &it_->head, true,         \
                                              &s->children[s->childrenSize]); \
            s->childrenSize++;                                               \
            if(res_ != UA_STATUSCODE_GOOD)                                   \
                return res_;                                                 \
        }                                                                    \
    } while(0)

/* Deep copy of a component (optionally with its children) */
static UA_StatusCode
snapshotTake(UA_PubSubManager *psm, UA_PubSubComponentHead *head,
             UA_Boolean withChildren, UA_Snapshot *s) {
    memset(s, 0, sizeof(UA_Snapshot));
    s->type = head->componentType;
    s->id = head->identifier;
    s->state = head->state;
    s->position = listPosition(psm, head);
    UA_ComponentConfig *live = componentConfig(head);
    UA_StatusCode res;
    switch(s->type) {
    case UA_PUBSUBCOMPONENT_CONNECTION: {
        res = UA_PubSubConnectionConfig_copy(&live->conn, &s->config.conn);
        UA_PubSubConnection *c = (UA_PubSubConnection*)head;
        if(res == UA_STATUSCODE_GOOD && withChildren) {
            UA_SNAPSHOT_CHILDREN(&c->writerGroups, UA_WriterGroup);
            UA_SNAPSHOT_CHILDREN(&c->readerGroups, UA_ReaderGroup);
        }
        return res;
    }
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        res = UA_WriterGroupConfig_copy(&live->wg, &s->config.wg);
        if(res == UA_STATUSCODE_GOOD && withChildren)
            UA_SNAPSHOT_CHILDREN(&((UA_WriterGroup*)head)->writers, UA_DataSetWriter);
        return res;
    case UA_PUBSUBCOMPONENT_READERGROUP:
        res = UA_ReaderGroupConfig_copy(&live->rg, &s->config.rg);
        if(res == UA_STATUSCODE_GOOD && withChildren)
            UA_SNAPSHOT_CHILDREN(&((UA_ReaderGroup*)head)->readers, UA_DataSetReader);
        return res;
    case UA_PUBSUBCOMPONENT_DATASETWRITER: {
        UA_DataSetWriter *dsw = (UA_DataSetWriter*)head;
        res = UA_DataSetWriterConfig_copy(&live->dsw, &s->config.dsw);
        if(res == UA_STATUSCODE_GOOD && dsw->connectedDataSet)
            res = UA_String_copy(&dsw->connectedDataSet->config.name, &s->dataSetName);
        return res;
    }
    case UA_PUBSUBCOMPONENT_DATASETREADER:
        return UA_DataSetReaderConfig_copy(&live->dsr, &s->config.dsr);
    case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET:
        /* The fields and the metadata as in the file */
        res = UA_PublishedDataSetConfig_copy(&live->pds.config, &s->config.pds.config);
        if(res == UA_STATUSCODE_GOOD)
            res = UA_PublishedDataSet_toDataType((UA_PublishedDataSet*)head,
                                                 &s->config.pds.data);
        return res;
    case UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET:
        return UA_SubscribedDataSetConfig_copy(&live->sds, &s->config.sds);
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
}

static UA_Snapshot *
snapshotNew(UA_UpdateCtx *ctx, UA_PubSubComponentHead *head,
            UA_Boolean withChildren, UA_StatusCode *res) {
    UA_Snapshot *s = (UA_Snapshot*)UA_calloc(1, sizeof(UA_Snapshot));
    if(!s) {
        *res = UA_STATUSCODE_BADOUTOFMEMORY;
        return NULL;
    }
    *res = snapshotTake(ctx->psm, head, withChildren, s);
    if(*res != UA_STATUSCODE_GOOD) {
        snapshotClear(s);
        UA_free(s);
        return NULL;
    }
    return s;
}

static void
snapshotFree(UA_Snapshot *s) {
    snapshotClear(s);
    UA_free(s);
}

/* Remove a component with its children. For a complete update the component
 * is recorded first so that the removal can be undone. */
static UA_StatusCode
removeComponent(UA_UpdateCtx *ctx, UA_PubSubComponentHead *head) {
    const UA_NodeId id = head->identifier;
    const UA_PubSubComponentType type = head->componentType;
    UA_PubSubComponentHead *parent = parentOf(head);
    const UA_NodeId parentId = (parent) ? parent->identifier : UA_NODEID_NULL;
    UA_Snapshot *snapshot = NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(ctx->atomic) {
        res = logReserve(ctx, 1);
        if(res == UA_STATUSCODE_GOOD)
            snapshot = snapshotNew(ctx, head, true, &res);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    res = UA_PubSubComponent_remove(ctx->psm, head);
    if(res != UA_STATUSCODE_GOOD) {
        if(snapshot)
            snapshotFree(snapshot);
        return res;
    }
    if(snapshot) {
        UA_UndoEntry *e = logAppend(ctx, UA_UNDO_REMOVED, type, id); /* reserved */
        e->parent = parentId;
        e->snapshot = snapshot;
    }
    return UA_STATUSCODE_GOOD;
}

/* Update the config of a disabled component. For a complete update the prior
 * config is recorded. */
static UA_StatusCode
modifyComponent(UA_UpdateCtx *ctx, UA_PubSubComponentHead *head,
                const UA_ComponentConfig *config) {
    UA_Snapshot *saved = NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(ctx->atomic) {
        res = logReserve(ctx, 1);
        if(res == UA_STATUSCODE_GOOD)
            saved = snapshotNew(ctx, head, false, &res);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    res = updateComponentConfig(ctx->psm, head, config);
    if(res != UA_STATUSCODE_GOOD) {
        if(saved)
            snapshotFree(saved);
        return res;
    }
    if(saved) {
        UA_UndoEntry *e = logAppend(ctx, UA_UNDO_MODIFIED, saved->type,
                                    saved->id); /* reserved */
        e->snapshot = saved;
    }
    return UA_STATUSCODE_GOOD;
}

/************/
/* Rollback */
/************/

static void
remapAdd(UA_UpdateCtx *ctx, const UA_NodeId oldId, const UA_NodeId newId) {
    UA_IdRemap *newRemap = (UA_IdRemap*)
        UA_realloc(ctx->remap, (ctx->remapSize + 1) * sizeof(UA_IdRemap));
    if(!newRemap)
        return; /* Later entries do not find the component */
    ctx->remap = newRemap;
    ctx->remap[ctx->remapSize].oldId = oldId;
    ctx->remap[ctx->remapSize].newId = newId;
    ctx->remapSize++;
}

/* The current NodeId of a component that might have been recreated. A
 * component that could not be recreated maps to the null NodeId. */
static UA_NodeId
remapId(const UA_UpdateCtx *ctx, const UA_NodeId id) {
    for(size_t i = ctx->remapSize; i > 0; i--) {
        if(UA_NodeId_equal(&ctx->remap[i - 1].oldId, &id))
            return ctx->remap[i - 1].newId;
    }
    return id;
}

static void
remapGone(UA_UpdateCtx *ctx, const UA_Snapshot *s) {
    remapAdd(ctx, s->id, UA_NODEID_NULL);
    for(size_t i = 0; i < s->childrenSize; i++)
        remapGone(ctx, &s->children[i]);
}

/* Create a component from its config (the enabled flag is ignored). A
 * PublishedDataSet gets the fields and the metadata of the file element. */
static UA_StatusCode
createComponent(UA_PubSubManager *psm, UA_PubSubComponentType type,
                UA_ComponentConfig *config, const UA_NodeId parent,
                const UA_NodeId pdsId, UA_NodeId *newId) {
    UA_Boolean *enabled = configEnabled(config, type);
    UA_Boolean enable = (enabled) ? *enabled : false;
    if(enabled)
        *enabled = false;
    UA_StatusCode res;
    switch(type) {
    case UA_PUBSUBCOMPONENT_CONNECTION:
        res = UA_PubSubConnection_create(psm, &config->conn, newId);
        break;
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        res = UA_WriterGroup_create(psm, parent, &config->wg, newId);
        break;
    case UA_PUBSUBCOMPONENT_DATASETWRITER:
        res = UA_DataSetWriter_create(psm, parent, pdsId, &config->dsw, newId);
        break;
    case UA_PUBSUBCOMPONENT_READERGROUP:
        res = UA_ReaderGroup_create(psm, parent, &config->rg, newId);
        break;
    case UA_PUBSUBCOMPONENT_DATASETREADER:
        res = UA_DataSetReader_create(psm, parent, &config->dsr, newId);
        break;
    case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET: {
        /* The fields of a template are added from the data */
        UA_PublishedDataSetConfig pc = config->pds.config;
        pc.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
        res = UA_PublishedDataSet_create(psm, &pc, newId).addResult;
        if(res != UA_STATUSCODE_GOOD)
            break;
        res = UA_PublishedDataSet_addFieldsFromDataType(psm, *newId,
                                                        &config->pds.data);
        if(res != UA_STATUSCODE_GOOD) {
            UA_PublishedDataSet *pds = UA_PublishedDataSet_find(psm, *newId);
            if(pds)
                UA_PublishedDataSet_remove(psm, pds);
        }
        break;
    }
    case UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET:
        res = UA_SubscribedDataSet_create(psm, &config->sds, newId);
        break;
    default:
        res = UA_STATUSCODE_BADINTERNALERROR;
        break;
    }
    if(enabled)
        *enabled = enable;
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* Keep the configured enabled flag for the activation */
    UA_PubSubComponentHead *head = UA_PubSubComponent_find(psm, type, *newId);
    if(head && enabled)
        *configEnabled(componentConfig(head), type) = enable;
    return UA_STATUSCODE_GOOD;
}

/* Recreate a removed component with its children. The components are created
 * disabled. The states are restored when the subtree is complete. */
static UA_StatusCode
snapshotCreate(UA_UpdateCtx *ctx, UA_Snapshot *s, const UA_NodeId parent) {
    UA_PubSubManager *psm = ctx->psm;
    UA_NodeId pdsId = UA_NODEID_NULL;
    UA_NodeId newId = UA_NODEID_NULL;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(!UA_String_isEmpty(&s->dataSetName)) {
        UA_PublishedDataSet *pds = UA_PublishedDataSet_findByName(psm, s->dataSetName);
        if(pds)
            pdsId = pds->head.identifier;
        else
            res = UA_STATUSCODE_BADNOTFOUND;
    }
    if(res == UA_STATUSCODE_GOOD)
        res = createComponent(psm, s->type, &s->config, parent, pdsId, &newId);
    if(res != UA_STATUSCODE_GOOD) {
        remapGone(ctx, s);
        return res;
    }
    remapAdd(ctx, s->id, newId);

    UA_PubSubComponentHead *head = UA_PubSubComponent_find(psm, s->type, newId);
    if(head)
        moveToPosition(psm, head, s->position);

    for(size_t i = 0; i < s->childrenSize; i++) {
        UA_StatusCode childRes = snapshotCreate(ctx, &s->children[i], newId);
        if(childRes != UA_STATUSCODE_GOOD)
            res = childRes;
    }
    return res;
}

/* Restore the states of a recreated subtree, parents before children */
static void
snapshotRestoreStates(UA_UpdateCtx *ctx, const UA_Snapshot *s) {
    if(UA_PubSubState_isEnabled(s->state)) {
        UA_PubSubComponentHead *head =
            UA_PubSubComponent_find(ctx->psm, s->type, remapId(ctx, s->id));
        if(head)
            enableComponent(ctx->psm, head, s->state);
    }
    for(size_t i = 0; i < s->childrenSize; i++)
        snapshotRestoreStates(ctx, &s->children[i]);
}

/* Undo the changes of a failed complete update in reverse order. The
 * application is not asked (lifecycle callback) for the undo operations.
 * Returns false if a change could not be undone. */
static UA_Boolean
rollbackUpdate(UA_UpdateCtx *ctx) {
    UA_PubSubManager *psm = ctx->psm;
    UA_PubSubConfiguration *psc = &psm->drv.server->config.pubSubConfig;
    UA_StatusCode (*lifecycleCallback)(UA_Server *server, const UA_NodeId id,
                                       const UA_PubSubComponentType componentType,
                                       UA_Boolean remove) =
        psc->componentLifecycleCallback;
    psc->componentLifecycleCallback = NULL;

    UA_Boolean complete = true;
    for(size_t i = ctx->logSize; i > 0; i--) {
        UA_UndoEntry *e = &ctx->log[i - 1];
        UA_PubSubComponentHead *head =
            UA_PubSubComponent_find(psm, e->type, remapId(ctx, e->id));
        UA_StatusCode res = UA_STATUSCODE_GOOD;
        switch(e->kind) {
        case UA_UNDO_ADDED:
            if(head)
                res = UA_PubSubComponent_remove(psm, head);
            break;
        case UA_UNDO_MODIFIED:
            res = (head) ? updateComponentConfig(psm, head, &e->snapshot->config) :
                UA_STATUSCODE_BADNOTFOUND;
            break;
        case UA_UNDO_REMOVED:
            res = snapshotCreate(ctx, e->snapshot, remapId(ctx, e->parent));
            snapshotRestoreStates(ctx, e->snapshot);
            break;
        case UA_UNDO_STATE:
            if(head)
                setState(psm, head, e->priorState);
            break;
        default:
            break;
        }
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                         "PubSub configuration update: Undoing a change "
                         "failed with %s", UA_StatusCode_name(res));
            complete = false;
        }
    }

    psc->componentLifecycleCallback = lifecycleCallback;
    return complete;
}

static void
updateCtxClear(UA_UpdateCtx *ctx) {
    for(size_t i = 0; i < ctx->logSize; i++) {
        if(ctx->log[i].snapshot)
            snapshotFree(ctx->log[i].snapshot);
    }
    UA_free(ctx->log);
    UA_free(ctx->remap);
    if(ctx->bind) {
        for(size_t i = 0; i < ctx->cfg->connectionsSize; i++) {
            UA_free(ctx->bind[i].wgs);
            UA_free(ctx->bind[i].rgs);
        }
        UA_free(ctx->bind);
    }
    memset(ctx, 0, sizeof(UA_UpdateCtx));
}

/***********************/
/* Reference resolving */
/***********************/

/* Convert the referenced element for Add and Modify. The conversion does not
 * change the configuration. */
static UA_StatusCode
convertOp(UA_ConfigUpdateOp *op) {
    UA_ComponentConfig *c = &op->config;
    UA_StatusCode res;
    switch(op->refbit) {
    case UA_REF_CONNECTION:
        return UA_PubSubConnectionConfig_fromDataType(op->fileConn, &c->conn);
    case UA_REF_WRITERGROUP:
        return UA_WriterGroupConfig_fromDataType(op->fileWg, &c->wg);
    case UA_REF_READERGROUP:
        return UA_ReaderGroupConfig_fromDataType(op->fileRg, &c->rg);
    case UA_REF_WRITER:
        return UA_DataSetWriterConfig_fromDataType(op->fileDsw, &c->dsw);
    case UA_REF_READER:
        return UA_DataSetReaderConfig_fromDataType(op->fileDsr, &c->dsr);
    case UA_REF_PUBDATASET:
        c->pds.data = *op->filePds;
        res = UA_PublishedDataSetConfig_fromDataType(op->filePds, &c->pds.config);
        for(size_t i = 0; res == UA_STATUSCODE_GOOD &&
                i < op->filePds->dataSetMetaData.fieldsSize; i++) {
            UA_DataSetFieldConfig fc;
            res = UA_DataSetFieldConfig_fromDataType(op->filePds, i, &fc);
        }
        return res;
    default:
        return UA_SubscribedDataSetConfig_fromDataType(op->fileSsds, &c->sds);
    }
}

/* The PublisherId of connections and readers is allocated */
static void
opClear(UA_ConfigUpdateOp *op) {
    if(op->refbit == UA_REF_CONNECTION)
        UA_PubSubConnectionConfig_clearView(&op->config.conn);
    else if(op->refbit == UA_REF_READER)
        UA_DataSetReaderConfig_clearView(&op->config.dsr);
}

static UA_StatusCode
resolveOp(const UA_PubSubConfiguration2DataType *cfg,
          const UA_PubSubConfigurationRefDataType *ref,
          UA_ConfigUpdateOp *op) {
    op->ref = ref;
    op->op = ref->configurationMask & UA_REFMASK_OPBITS;
    op->refbit = ref->configurationMask & UA_REFMASK_REFBITS;

    /* Unknown bits set */
    if(ref->configurationMask & ~(UA_REFMASK_OPBITS | UA_REFMASK_REFBITS))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Exactly one reference bit */
    UA_UInt32 rb = op->refbit;
    if(rb == 0 || (rb & (rb - 1)) != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* SecurityGroups and PushTargets are not supported */
    if(rb == UA_PUBSUBCONFIGURATIONREFMASK_REFERENCESECURITYGROUP ||
       rb == UA_PUBSUBCONFIGURATIONREFMASK_REFERENCEPUSHTARGET)
        return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    /* Resolve the indices into the file configuration */
    switch(rb) {
    case UA_REF_CONNECTION:
    case UA_REF_WRITERGROUP:
    case UA_REF_READERGROUP:
    case UA_REF_WRITER:
    case UA_REF_READER:
        if(ref->connectionIndex >= cfg->connectionsSize)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        op->fileConn = &cfg->connections[ref->connectionIndex];
        break;
    default:
        break;
    }

    switch(rb) {
    case UA_REF_WRITERGROUP:
    case UA_REF_WRITER:
        if(ref->groupIndex >= op->fileConn->writerGroupsSize)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        op->fileWg = &op->fileConn->writerGroups[ref->groupIndex];
        if(rb == UA_REF_WRITER) {
            if(ref->elementIndex >= op->fileWg->dataSetWritersSize)
                return UA_STATUSCODE_BADINVALIDARGUMENT;
            op->fileDsw = &op->fileWg->dataSetWriters[ref->elementIndex];
        }
        break;
    case UA_REF_READERGROUP:
    case UA_REF_READER:
        if(ref->groupIndex >= op->fileConn->readerGroupsSize)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        op->fileRg = &op->fileConn->readerGroups[ref->groupIndex];
        if(rb == UA_REF_READER) {
            if(ref->elementIndex >= op->fileRg->dataSetReadersSize)
                return UA_STATUSCODE_BADINVALIDARGUMENT;
            op->fileDsr = &op->fileRg->dataSetReaders[ref->elementIndex];
        }
        break;
    case UA_REF_PUBDATASET:
        if(ref->elementIndex >= cfg->publishedDataSetsSize)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        op->filePds = &cfg->publishedDataSets[ref->elementIndex];
        break;
    case UA_REF_SUBDATASET:
        if(ref->elementIndex >= cfg->subscribedDataSetsSize)
            return UA_STATUSCODE_BADINVALIDARGUMENT;
        op->fileSsds = &cfg->subscribedDataSets[ref->elementIndex];
        break;
    default:
        break;
    }
    op->indicesValid = true;

    /* Validate the operation bits. Allowed: Add, Match, Add|Match, Modify,
     * Remove. */
    const UA_UInt32 addMatch = UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
        UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH;
    if(op->op != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD &&
       op->op != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH && op->op != addMatch &&
       op->op != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY &&
       op->op != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Match only for Connection, WriterGroup and ReaderGroup */
    if((op->op & UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH) &&
       rb != UA_REF_CONNECTION && rb != UA_REF_WRITERGROUP &&
       rb != UA_REF_READERGROUP)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* For a pure Match the name and the Id shall be null (Part 14
     * 9.1.3.7.2). With Add|Match they are used for the added element. */
    const UA_String name = opName(op);
    if(op->op == UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH &&
       (!UA_String_isEmpty(&name) ||
        (rb == UA_REF_CONNECTION && !UA_Variant_isEmpty(&op->fileConn->publisherId)) ||
        (rb == UA_REF_WRITERGROUP && op->fileWg->writerGroupId != 0)))
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    /* Changing the field list of a dataset requires recreating the component.
     * Use remove + add with the same name in one call. */
    if(op->op == UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY &&
       (rb == UA_REF_PUBDATASET || rb == UA_REF_SUBDATASET))
        return UA_STATUSCODE_BADNOTIMPLEMENTED;

    /* Convert the element. It only fails an Add|Match if it is added. */
    if(op->op & (UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTADD |
                 UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY)) {
        op->convertStatus = convertOp(op);
        if(op->op != addMatch)
            return op->convertStatus;
    }
    return UA_STATUSCODE_GOOD;
}

/*******************/
/* Element binding */
/*******************/

static UA_StatusCode
bindingInit(UA_UpdateCtx *ctx) {
    const UA_PubSubConfiguration2DataType *cfg = ctx->cfg;
    if(cfg->connectionsSize == 0)
        return UA_STATUSCODE_GOOD;
    ctx->bind = (UA_ConnectionBinding*)
        UA_calloc(cfg->connectionsSize, sizeof(UA_ConnectionBinding));
    if(!ctx->bind)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < cfg->connectionsSize; i++) {
        const UA_PubSubConnectionDataType *c = &cfg->connections[i];
        if(c->writerGroupsSize > 0) {
            ctx->bind[i].wgs = (UA_ElementBinding*)
                UA_calloc(c->writerGroupsSize, sizeof(UA_ElementBinding));
            if(!ctx->bind[i].wgs)
                return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        if(c->readerGroupsSize > 0) {
            ctx->bind[i].rgs = (UA_ElementBinding*)
                UA_calloc(c->readerGroupsSize, sizeof(UA_ElementBinding));
            if(!ctx->bind[i].rgs)
                return UA_STATUSCODE_BADOUTOFMEMORY;
        }
    }
    return UA_STATUSCODE_GOOD;
}

/* The binding of a connection or group element */
static UA_ElementBinding *
elementBinding(UA_UpdateCtx *ctx, UA_UInt32 refbit, UA_UInt16 connectionIndex,
               UA_UInt16 groupIndex) {
    switch(refbit) {
    case UA_REF_CONNECTION: return &ctx->bind[connectionIndex].conn;
    case UA_REF_WRITERGROUP: return &ctx->bind[connectionIndex].wgs[groupIndex];
    case UA_REF_READERGROUP: return &ctx->bind[connectionIndex].rgs[groupIndex];
    default: return NULL;
    }
}

static UA_ElementBinding *
opBinding(UA_UpdateCtx *ctx, const UA_ConfigUpdateOp *op) {
    if(!op->indicesValid || !ctx->bind)
        return NULL;
    return elementBinding(ctx, op->refbit, op->ref->connectionIndex,
                          op->ref->groupIndex);
}

/* Record the outcome of an operation for the children of the element. A
 * failure does not undo an earlier binding of the same element. */
static void
bindResult(UA_UpdateCtx *ctx, const UA_ConfigUpdateOp *op,
           UA_StatusCode status, const UA_NodeId objId) {
    UA_ElementBinding *b = opBinding(ctx, op);
    if(!b)
        return;
    if(status == UA_STATUSCODE_GOOD) {
        if(op->op == UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE) {
            b->state = UA_BIND_REMOVED;
            b->id = UA_NODEID_NULL;
        } else {
            b->state = UA_BIND_BOUND;
            b->id = objId;
        }
    } else if(b->state == UA_BIND_UNREFERENCED) {
        b->state = UA_BIND_FAILED;
    }
}

/* The live component of a bound element */
static UA_PubSubComponentHead *
boundComponent(UA_UpdateCtx *ctx, UA_UInt32 refbit, const UA_ElementBinding *b) {
    UA_PubSubComponentHead *head =
        UA_PubSubComponent_find(ctx->psm, refType(refbit), b->id);
    return (head && !pendingDelete(head)) ? head : NULL;
}

/* Resolve a connection or group element of the file (Part 14 9.1.3.7.3). An
 * element referenced in this call resolves to the component it was added,
 * matched or modified to. An element that is not referenced identifies the
 * component by its name. An element that was removed or failed in this call
 * is not available. */
static UA_PubSubComponentHead *
resolveElement(UA_UpdateCtx *ctx, UA_UInt32 refbit, UA_UInt16 connectionIndex,
               UA_UInt16 groupIndex) {
    const UA_ElementBinding *b =
        elementBinding(ctx, refbit, connectionIndex, groupIndex);
    if(b->state == UA_BIND_BOUND)
        return boundComponent(ctx, refbit, b);
    if(b->state != UA_BIND_UNREFERENCED)
        return NULL;
    const UA_PubSubConnectionDataType *fc = &ctx->cfg->connections[connectionIndex];
    if(refbit == UA_REF_CONNECTION)
        return findByName(ctx->psm, NULL, refbit, fc->name);
    UA_PubSubComponentHead *c =
        resolveElement(ctx, UA_REF_CONNECTION, connectionIndex, 0);
    if(!c)
        return NULL;
    return findByName(ctx->psm, c, refbit, (refbit == UA_REF_WRITERGROUP) ?
                      fc->writerGroups[groupIndex].name :
                      fc->readerGroups[groupIndex].name);
}

/* The live parent of a group, writer or reader reference */
static UA_PubSubComponentHead *
resolveParent(UA_UpdateCtx *ctx, const UA_ConfigUpdateOp *op) {
    const UA_PubSubConfigurationRefDataType *ref = op->ref;
    switch(op->refbit) {
    case UA_REF_WRITERGROUP:
    case UA_REF_READERGROUP:
        return resolveElement(ctx, UA_REF_CONNECTION, ref->connectionIndex, 0);
    case UA_REF_WRITER:
        return resolveElement(ctx, UA_REF_WRITERGROUP, ref->connectionIndex,
                              ref->groupIndex);
    case UA_REF_READER:
        return resolveElement(ctx, UA_REF_READERGROUP, ref->connectionIndex,
                              ref->groupIndex);
    default:
        return NULL;
    }
}

static UA_Boolean
hasParent(UA_UInt32 refbit) {
    return (refbit != UA_REF_CONNECTION && refbit != UA_REF_PUBDATASET &&
            refbit != UA_REF_SUBDATASET);
}

/* Find the live element of a Remove or Modify reference. An element that
 * failed earlier in this call is identified by its name. Returns Bad_NotFound
 * for a missing parent and Bad_NoMatch for a missing element. */
static UA_StatusCode
lookupElement(UA_UpdateCtx *ctx, const UA_ConfigUpdateOp *op,
              UA_PubSubComponentHead **out) {
    *out = NULL;
    const UA_ElementBinding *b = opBinding(ctx, op);
    if(b && b->state == UA_BIND_REMOVED)
        return UA_STATUSCODE_BADNOMATCH;
    if(b && b->state == UA_BIND_BOUND) {
        *out = boundComponent(ctx, op->refbit, b);
    } else {
        UA_PubSubComponentHead *parent = NULL;
        if(hasParent(op->refbit)) {
            parent = resolveParent(ctx, op);
            if(!parent)
                return UA_STATUSCODE_BADNOTFOUND;
        }
        *out = findByName(ctx->psm, parent, op->refbit, opName(op));
    }
    return (*out) ? UA_STATUSCODE_GOOD : UA_STATUSCODE_BADNOMATCH;
}

/**********************/
/* Result bookkeeping */
/**********************/

/* Report the name and the identifier (PublisherId, WriterGroupId or
 * DataSetWriterId) of an added or matched component */
static void
addConfigValue(UA_UpdateCtx *ctx, const UA_ConfigUpdateOp *op,
               UA_PubSubComponentHead *head) {
    UA_Variant identifier;
    UA_Variant_init(&identifier);
    UA_ComponentConfig *config = componentConfig(head);
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION:
        UA_PublisherId_toVariant(&config->conn.publisherId, &identifier);
        break;
    case UA_PUBSUBCOMPONENT_WRITERGROUP:
        UA_Variant_setScalar(&identifier, &config->wg.writerGroupId,
                             &UA_TYPES[UA_TYPES_UINT16]);
        break;
    case UA_PUBSUBCOMPONENT_DATASETWRITER:
        UA_Variant_setScalar(&identifier, &config->dsw.dataSetWriterId,
                             &UA_TYPES[UA_TYPES_UINT16]);
        break;
    default:
        break;
    }

    UA_PubSubConfigurationUpdateResult *result = ctx->result;
    UA_PubSubConfigurationValueDataType *v =
        &result->configurationValues[result->configurationValuesSize];
    UA_PubSubConfigurationValueDataType_init(v);
    UA_StatusCode res =
        UA_PubSubConfigurationRefDataType_copy(op->ref, &v->configurationElement);
    res |= UA_String_copy(configName(config, head->componentType), &v->name);
    res |= UA_Variant_copy(&identifier, &v->identifier);
    if(res != UA_STATUSCODE_GOOD) {
        UA_PubSubConfigurationValueDataType_clear(v);
        return;
    }
    result->configurationValuesSize++;
}

/*********************/
/* Remove operations */
/*********************/

static UA_Boolean
writerGroupUsesPds(UA_WriterGroup *wg, const UA_PublishedDataSet *pds) {
    UA_DataSetWriter *dsw;
    LIST_FOREACH(dsw, &wg->writers, listEntry) {
        if(dsw->connectedDataSet == pds)
            return true;
    }
    return false;
}

/* Remove a PublishedDataSet with its connected DataSetWriters (Part 14
 * 9.1.3.7.2). Writers can only be removed from a disabled WriterGroup, so the
 * affected groups are disabled for the update call. The writers are removed
 * explicitly, because the writers of a disabled group are paused and keep the
 * PDS configuration frozen. */
static UA_StatusCode
removePublishedDataSet(UA_UpdateCtx *ctx, UA_PublishedDataSet *pds) {
    UA_PubSubManager *psm = ctx->psm;
    UA_PubSubConnection *c;
    UA_WriterGroup *wg;
    UA_StatusCode res;
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            if(!writerGroupUsesPds(wg, pds))
                continue;
            res = ensureDisabled(ctx, &wg->head);
            if(res != UA_STATUSCODE_GOOD)
                return res;
        }
    }
    TAILQ_FOREACH(c, &psm->connections, listEntry) {
        LIST_FOREACH(wg, &c->writerGroups, listEntry) {
            UA_DataSetWriter *dsw, *tmp;
            LIST_FOREACH_SAFE(dsw, &wg->writers, listEntry, tmp) {
                if(dsw->connectedDataSet != pds)
                    continue;
                res = removeComponent(ctx, &dsw->head);
                if(res != UA_STATUSCODE_GOOD)
                    return res;
            }
        }
    }
    return removeComponent(ctx, &pds->head);
}

static UA_StatusCode
applyRemove(UA_UpdateCtx *ctx, UA_ConfigUpdateOp *op, UA_NodeId *objId) {
    UA_PubSubComponentHead *head;
    UA_StatusCode res = lookupElement(ctx, op, &head);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    *objId = head->identifier;
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_PUBLISHEDDATASET:
        return removePublishedDataSet(ctx, (UA_PublishedDataSet*)head);
    case UA_PUBSUBCOMPONENT_SUBSCRIBEDDDATASET: {
        /* Remove a standalone SubscribedDataSet with its connected reader */
        UA_DataSetReader *dsr = ((UA_SubscribedDataSet*)head)->connectedReader;
        if(dsr) {
            res = ensureDisabled(ctx, &dsr->linkedReaderGroup->head);
            if(res == UA_STATUSCODE_GOOD)
                res = removeComponent(ctx, &dsr->head);
            if(res != UA_STATUSCODE_GOOD)
                return res;
        }
        return removeComponent(ctx, head);
    }
    case UA_PUBSUBCOMPONENT_DATASETWRITER:
    case UA_PUBSUBCOMPONENT_DATASETREADER:
        /* Writers and readers are removed from a disabled group */
        res = ensureDisabled(ctx, parentOf(head));
        if(res != UA_STATUSCODE_GOOD)
            return res;
        return removeComponent(ctx, head);
    default:
        return removeComponent(ctx, head);
    }
}

/******************/
/* Add operations */
/******************/

/* The first free name "<prefix> <n>" in the parent */
static UA_String
generateUniqueName(UA_PubSubManager *psm, UA_PubSubComponentHead *parent,
                   UA_UInt32 refbit) {
    const char *prefix;
    switch(refbit) {
    case UA_REF_CONNECTION: prefix = "Connection"; break;
    case UA_REF_WRITERGROUP: prefix = "WriterGroup"; break;
    case UA_REF_READERGROUP: prefix = "ReaderGroup"; break;
    case UA_REF_WRITER: prefix = "DataSetWriter"; break;
    case UA_REF_READER: prefix = "DataSetReader"; break;
    case UA_REF_PUBDATASET: prefix = "PublishedDataSet"; break;
    default: prefix = "SubscribedDataSet"; break;
    }
    for(UA_UInt32 i = 1; i < 0xFFFF; i++) {
        UA_String candidate = UA_STRING_NULL;
        if(UA_String_format(&candidate, "%s %u", prefix,
                            (unsigned)i) != UA_STATUSCODE_GOOD)
            return UA_STRING_NULL;
        if(!findByName(psm, parent, refbit, candidate))
            return candidate; /* Allocated, the caller clears */
        UA_String_clear(&candidate);
    }
    return UA_STRING_NULL;
}

/* Assign the default PublisherId or a free WriterGroupId/DataSetWriterId if
 * not provided. Returns false if no id is free. */
static UA_Boolean
assignId(UA_UpdateCtx *ctx, UA_ConfigUpdateOp *op, UA_PubSubComponentHead *parent,
         UA_Boolean *assigned) {
    UA_PubSubManager *psm = ctx->psm;
    UA_ComponentConfig *c = &op->config;
    switch(op->refbit) {
    case UA_REF_CONNECTION:
        if(!UA_Variant_isEmpty(&op->fileConn->publisherId))
            return true;
        c->conn.publisherId.idType = UA_PUBLISHERIDTYPE_UINT64;
        c->conn.publisherId.id.uint64 = psm->defaultPublisherId;
        break;
    case UA_REF_WRITERGROUP:
        if(c->wg.writerGroupId != 0)
            return true;
        c->wg.writerGroupId = UA_ReserveId_findFreeId(psm,
            ((UA_PubSubConnection*)parent)->config.transportProfileUri,
            UA_WRITER_GROUP);
        if(c->wg.writerGroupId == 0)
            return false;
        break;
    case UA_REF_WRITER:
        if(c->dsw.dataSetWriterId != 0)
            return true;
        c->dsw.dataSetWriterId = UA_ReserveId_findFreeId(psm,
            ((UA_WriterGroup*)parent)->linkedConnection->config.transportProfileUri,
            UA_DATA_SET_WRITER);
        if(c->dsw.dataSetWriterId == 0)
            return false;
        break;
    default:
        return true;
    }
    *assigned = true;
    return true;
}

/* Create the component disabled. The added components are enabled when the
 * update call completes. The generated name or id is reported. */
static UA_StatusCode
applyAdd(UA_UpdateCtx *ctx, UA_ConfigUpdateOp *op, UA_NodeId *objId) {
    UA_PubSubManager *psm = ctx->psm;
    if(op->convertStatus != UA_STATUSCODE_GOOD)
        return op->convertStatus;

    /* The parent. The name is unique in the parent. */
    UA_PubSubComponentHead *parent = NULL;
    if(hasParent(op->refbit)) {
        parent = resolveParent(ctx, op);
        if(!parent)
            return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_PubSubComponentType type = refType(op->refbit);
    UA_String *name = configName(&op->config, type);
    if(findByName(psm, parent, op->refbit, *name))
        return UA_STATUSCODE_BADBROWSENAMEDUPLICATED;

    /* An empty DataSetName indicates a heartbeat writer */
    UA_NodeId pdsId = UA_NODEID_NULL;
    if(op->refbit == UA_REF_WRITER && !UA_String_isEmpty(&op->config.dsw.dataSetName)) {
        UA_PublishedDataSet *pds =
            UA_PublishedDataSet_findByName(psm, op->config.dsw.dataSetName);
        if(!pds)
            return UA_STATUSCODE_BADNOTFOUND;
        pdsId = pds->head.identifier;
    }

    /* Reserve the log entries for the disabled group and the added
     * component */
    UA_StatusCode res = logReserve(ctx, 2);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_Boolean assigned = false;
    UA_String assignedName = UA_STRING_NULL;
    if(UA_String_isEmpty(name)) {
        assignedName = generateUniqueName(psm, parent, op->refbit);
        if(UA_String_isEmpty(&assignedName))
            return UA_STATUSCODE_BADRESOURCEUNAVAILABLE;
        *name = assignedName;
        assigned = true;
    }
    if(!assignId(ctx, op, parent, &assigned))
        res = UA_STATUSCODE_BADRESOURCEUNAVAILABLE;

    /* Writers and readers can only be added to a disabled group */
    if(res == UA_STATUSCODE_GOOD &&
       (type == UA_PUBSUBCOMPONENT_DATASETWRITER ||
        type == UA_PUBSUBCOMPONENT_DATASETREADER))
        res = ensureDisabled(ctx, parent);

    UA_NodeId newId = UA_NODEID_NULL;
    if(res == UA_STATUSCODE_GOOD)
        res = createComponent(psm, type, &op->config,
                              (parent) ? parent->identifier : UA_NODEID_NULL,
                              pdsId, &newId);
    if(res == UA_STATUSCODE_GOOD) {
        UA_Boolean *enabled = configEnabled(&op->config, type);
        UA_UndoEntry *e = logAppend(ctx, UA_UNDO_ADDED, type, newId); /* reserved */
        e->enableOnCommit = (enabled) ? *enabled : false;
        *objId = newId;
        UA_PubSubComponentHead *head = UA_PubSubComponent_find(psm, type, newId);
        if(assigned && head)
            addConfigValue(ctx, op, head);
    }
    UA_String_clear(&assignedName);
    *name = UA_STRING_NULL;
    return res;
}

/********************/
/* Match operations */
/********************/

/* Compare only the provided KeyValuePairs against the live map */
static UA_Boolean
matchProperties(const UA_KeyValueMap *live, const UA_KeyValuePair *provided,
                size_t providedSize) {
    for(size_t i = 0; i < providedSize; i++) {
        const UA_Variant *value = UA_KeyValueMap_get(live, provided[i].key);
        if(!value || UA_order(value, &provided[i].value,
                              &UA_TYPES[UA_TYPES_VARIANT]) != UA_ORDER_EQ)
            return false;
    }
    return true;
}

static UA_Boolean
orderEqual(const void *a, const void *b, const UA_DataType *type) {
    return (UA_order(a, b, type) == UA_ORDER_EQ);
}

/* The SecurityKeyServices compared as Variant arrays */
static UA_Boolean
matchEndpoints(UA_EndpointDescription *a, size_t aSize,
               const UA_EndpointDescription *b, size_t bSize) {
    UA_Variant va, vb;
    UA_Variant_setArray(&va, a, aSize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Variant_setArray(&vb, (void*)(uintptr_t)b, bSize,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    return orderEqual(&va, &vb, &UA_TYPES[UA_TYPES_VARIANT]);
}

/* Field sets per Part 14 v1.05 Table 239 */
static UA_Boolean
matchComponent(UA_PubSubComponentHead *head, const UA_ConfigUpdateOp *op) {
    UA_Boolean res = false;
    switch(head->componentType) {
    case UA_PUBSUBCOMPONENT_CONNECTION: {
        UA_PubSubConnection *c = (UA_PubSubConnection*)head;
        const UA_PubSubConnectionDataType *p = op->fileConn;
        UA_PubSubConnectionDataType live;
        if(UA_PubSubConnectionConfig_toDataType(&c->config, &live) != UA_STATUSCODE_GOOD)
            return false;
        res = UA_String_equal(&live.transportProfileUri, &p->transportProfileUri) &&
            orderEqual(&live.address, &p->address, &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) &&
            orderEqual(&live.transportSettings, &p->transportSettings,
                       &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) &&
            matchProperties(&c->config.connectionProperties,
                            p->connectionProperties, p->connectionPropertiesSize);
        UA_PubSubConnectionDataType_clear(&live);
        return res;
    }
    case UA_PUBSUBCOMPONENT_WRITERGROUP: {
        const UA_WriterGroupConfig *wg = &((UA_WriterGroup*)head)->config;
        const UA_WriterGroupDataType *p = op->fileWg;
        return wg->securityMode == p->securityMode &&
            UA_String_equal(&wg->securityGroupId, &p->securityGroupId) &&
            matchEndpoints(wg->securityKeyServices, wg->securityKeyServicesSize,
                           p->securityKeyServices, p->securityKeyServicesSize) &&
            wg->maxNetworkMessageSize == p->maxNetworkMessageSize &&
            wg->publishingInterval == p->publishingInterval &&
            wg->keepAliveTime == p->keepAliveTime &&
            wg->priority == p->priority &&
            UA_String_equal(&wg->headerLayoutUri, &p->headerLayoutUri) &&
            orderEqual(&wg->transportSettings, &p->transportSettings,
                       &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) &&
            orderEqual(&wg->messageSettings, &p->messageSettings,
                       &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) &&
            matchProperties(&wg->groupProperties,
                            p->groupProperties, p->groupPropertiesSize);
    }
    default: {
        const UA_ReaderGroupConfig *rg = &((UA_ReaderGroup*)head)->config;
        const UA_ReaderGroupDataType *p = op->fileRg;
        return rg->securityMode == p->securityMode &&
            UA_String_equal(&rg->securityGroupId, &p->securityGroupId) &&
            matchEndpoints(rg->securityKeyServices, rg->securityKeyServicesSize,
                           p->securityKeyServices, p->securityKeyServicesSize) &&
            rg->maxNetworkMessageSize == p->maxNetworkMessageSize &&
            orderEqual(&rg->transportSettings, &p->transportSettings,
                       &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) &&
            orderEqual(&rg->messageSettings, &p->messageSettings,
                       &UA_TYPES[UA_TYPES_EXTENSIONOBJECT]) &&
            matchProperties(&rg->groupProperties,
                            p->groupProperties, p->groupPropertiesSize);
    }
    }
}

/* An enabled WriterGroup with the GroupHeader flag in the
 * NetworkMessageContentMask cannot be matched (Bad_InvalidState). The state
 * before the update call is relevant. */
static UA_Boolean
writerGroupHeaderActive(UA_UpdateCtx *ctx, UA_WriterGroup *wg) {
    if(!UA_PubSubState_isEnabled(effectiveState(ctx, &wg->head)))
        return false;
    const UA_ExtensionObject *ms = &wg->config.messageSettings;
    if(!UA_ExtensionObject_hasDecodedType(ms,
           &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE]))
        return false;
    UA_UadpWriterGroupMessageDataType *m =
        (UA_UadpWriterGroupMessageDataType*)ms->content.decoded.data;
    return (m->networkMessageContentMask &
            UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER) != 0;
}

/* Returns Bad_NoMatch to continue with the next candidate */
static UA_StatusCode
matchCandidate(UA_UpdateCtx *ctx, const UA_ConfigUpdateOp *op,
               UA_PubSubComponentHead *head, UA_NodeId *objId) {
    if(pendingDelete(head) || isAddedInCall(ctx, head->identifier) ||
       !matchComponent(head, op))
        return UA_STATUSCODE_BADNOMATCH;
    if(head->componentType == UA_PUBSUBCOMPONENT_WRITERGROUP &&
       writerGroupHeaderActive(ctx, (UA_WriterGroup*)head))
        return UA_STATUSCODE_BADINVALIDSTATE;
    *objId = head->identifier;
    addConfigValue(ctx, op, head);
    return UA_STATUSCODE_GOOD;
}

#define UA_MATCH_LIST(LISTTYPE, ELMTYPE, LISTHEAD) do {                     \
        ELMTYPE *it_;                                                       \
        LISTTYPE##_FOREACH(it_, LISTHEAD, listEntry) {                      \
            UA_StatusCode res_ = matchCandidate(ctx, op, &it_->head, objId); \
            if(res_ != UA_STATUSCODE_BADNOMATCH)                            \
                return res_;                                                \
        }                                                                   \
    } while(0)

/* Returns Good and the matched component, Bad_NoMatch when nothing matches,
 * Bad_NotFound for a missing parent and Bad_InvalidState for a WriterGroup
 * with active GroupHeader. Components added in this call are not matched. */
static UA_StatusCode
applyMatch(UA_UpdateCtx *ctx, UA_ConfigUpdateOp *op, UA_NodeId *objId) {
    if(op->refbit == UA_REF_CONNECTION) {
        UA_MATCH_LIST(TAILQ, UA_PubSubConnection, &ctx->psm->connections);
        return UA_STATUSCODE_BADNOMATCH;
    }
    UA_PubSubConnection *c = (UA_PubSubConnection*)resolveParent(ctx, op);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;
    if(op->refbit == UA_REF_WRITERGROUP)
        UA_MATCH_LIST(LIST, UA_WriterGroup, &c->writerGroups);
    else
        UA_MATCH_LIST(LIST, UA_ReaderGroup, &c->readerGroups);
    return UA_STATUSCODE_BADNOMATCH;
}

/*********************/
/* Modify operations */
/*********************/

/* The component is disabled for the update and restored to its prior state
 * when the call completes. The configured enabled flag is kept. */
static UA_StatusCode
applyModify(UA_UpdateCtx *ctx, UA_ConfigUpdateOp *op, UA_NodeId *objId) {
    UA_PubSubComponentHead *head;
    UA_StatusCode res = lookupElement(ctx, op, &head);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    *objId = head->identifier;
    UA_PubSubComponentType type = head->componentType;
    *configEnabled(&op->config, type) = *configEnabled(componentConfig(head), type);
    res = ensureDisabled(ctx, head);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return modifyComponent(ctx, head, &op->config);
}

/*************/
/* Top level */
/*************/

static UA_Boolean
applyTopLevelFields(UA_PubSubManager *psm,
                    const UA_PubSubConfiguration2DataType *cfg) {
    UA_Boolean changed = false;

    /* Replace the DefaultSecurityKeyServices if non-empty */
    if(cfg->defaultSecurityKeyServicesSize > 0) {
        UA_EndpointDescription *copy = NULL;
        UA_StatusCode res =
            UA_Array_copy(cfg->defaultSecurityKeyServices,
                          cfg->defaultSecurityKeyServicesSize, (void**)&copy,
                          &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        if(res == UA_STATUSCODE_GOOD) {
            UA_Array_delete(psm->defaultSecurityKeyServices,
                            psm->defaultSecurityKeyServicesSize,
                            &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
            psm->defaultSecurityKeyServices = copy;
            psm->defaultSecurityKeyServicesSize =
                cfg->defaultSecurityKeyServicesSize;
            changed = true;
        }
    }

    /* Merge the ConfigurationProperties. A null value deletes the key. */
    for(size_t i = 0; i < cfg->configurationPropertiesSize; i++) {
        const UA_KeyValuePair *kvp = &cfg->configurationProperties[i];
        if(UA_Variant_isEmpty(&kvp->value)) {
            if(UA_KeyValueMap_remove(&psm->configurationProperties,
                                     kvp->key) == UA_STATUSCODE_GOOD)
                changed = true;
        } else {
            if(UA_KeyValueMap_set(&psm->configurationProperties, kvp->key,
                                  &kvp->value) == UA_STATUSCODE_GOOD)
                changed = true;
        }
    }

    return changed;
}

/***************/
/* Entry point */
/***************/

/* After a rollback only the results of the matches and modifications remain.
 * The added and removed components no longer exist or have new NodeIds. */
static void
rollbackResult(UA_PubSubConfigurationUpdateResult *result, const UA_ConfigUpdateOp *ops,
               size_t opsSize) {
    for(size_t i = 0; i < opsSize; i++) {
        const UA_ConfigUpdateOp *op = &ops[i];
        if(op->op != UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY && !op->matched)
            UA_NodeId_clear(&result->configurationObjects[op->inputIndex]);
    }

    size_t kept = 0;
    for(size_t k = 0; k < result->configurationValuesSize; k++) {
        UA_Boolean drop = false;
        for(size_t i = 0; i < opsSize; i++) {
            const UA_ConfigUpdateOp *op = &ops[i];
            if(k >= op->valuesBegin && k < op->valuesEnd && !op->matched)
                drop = true;
        }
        if(drop) {
            UA_PubSubConfigurationValueDataType_clear(&result->configurationValues[k]);
            continue;
        }
        if(kept != k)
            result->configurationValues[kept] = result->configurationValues[k];
        kept++;
    }
    /* The entries beyond the size are zeroed (moved or cleared) */
    for(size_t k = kept; k < result->configurationValuesSize; k++)
        UA_PubSubConfigurationValueDataType_init(&result->configurationValues[k]);
    result->configurationValuesSize = kept;
}

/* The removes are processed first (Part 14 9.1.3.7.6), children before their
 * parents. The other operations run parents before children, so the
 * references can be given in any order. Within a rank the input order is
 * kept. */
#define UA_OPRANK_MAX 8

static size_t
opRank(UA_UInt32 mask) {
    UA_Boolean remove = (mask & UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE) != 0;
    switch(mask & UA_REFMASK_REFBITS) {
    case UA_REF_WRITER:
    case UA_REF_READER:
        return (remove) ? 0 : 7;
    case UA_REF_WRITERGROUP:
    case UA_REF_READERGROUP:
        return (remove) ? 1 : 6;
    case UA_REF_CONNECTION:
        return (remove) ? 2 : 5;
    case UA_REF_PUBDATASET:
    case UA_REF_SUBDATASET:
        return (remove) ? 3 : 4;
    default:
        return UA_OPRANK_MAX; /* Invalid or unsupported */
    }
}

static UA_StatusCode
applyOp(UA_UpdateCtx *ctx, UA_ConfigUpdateOp *op, UA_NodeId *objId) {
    switch(op->op) {
    case UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTREMOVE:
        return applyRemove(ctx, op, objId);
    case UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMODIFY:
        return applyModify(ctx, op, objId);
    case UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH:
        op->matched = true;
        return applyMatch(ctx, op, objId);
    default: { /* Add or Add|Match */
        if(op->op & UA_PUBSUBCONFIGURATIONREFMASK_ELEMENTMATCH) {
            UA_StatusCode res = applyMatch(ctx, op, objId);
            if(res != UA_STATUSCODE_BADNOMATCH) {
                op->matched = (res == UA_STATUSCODE_GOOD);
                return res;
            }
        }
        return applyAdd(ctx, op, objId);
    }
    }
}

static UA_StatusCode
updateConfig2(UA_PubSubManager *psm, const UA_PubSubConfiguration2DataType *cfg,
              size_t refsSize, const UA_PubSubConfigurationRefDataType *refs,
              UA_Boolean requireCompleteUpdate,
              UA_PubSubConfigurationUpdateResult *result) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);

    /* Allocate the result arrays. The configurationValues array is
     * over-allocated to the number of references and only partially used. */
    result->referencesResults = (UA_StatusCode*)
        UA_Array_new(refsSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    result->configurationObjects = (UA_NodeId*)
        UA_Array_new(refsSize, &UA_TYPES[UA_TYPES_NODEID]);
    result->configurationValues = (UA_PubSubConfigurationValueDataType*)
        UA_Array_new(refsSize, &UA_TYPES[UA_TYPES_PUBSUBCONFIGURATIONVALUEDATATYPE]);
    UA_ConfigUpdateOp *ops = (UA_ConfigUpdateOp*)
        UA_calloc(refsSize, sizeof(UA_ConfigUpdateOp));
    UA_UpdateCtx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.psm = psm;
    ctx.cfg = cfg;
    ctx.result = result;
    ctx.atomic = requireCompleteUpdate;
    if(!result->referencesResults || !result->configurationObjects ||
       !result->configurationValues || !ops ||
       bindingInit(&ctx) != UA_STATUSCODE_GOOD) {
        updateCtxClear(&ctx);
        UA_free(ops);
        UA_PubSubConfigurationUpdateResult_clear(result);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    result->referencesResultsSize = refsSize;
    result->configurationObjectsSize = refsSize;
    result->configurationValuesSize = 0; /* grows in addConfigValue */

    /* Resolve and convert all references in the execution order. A complete
     * update with an invalid reference changes nothing. */
    size_t opsSize = 0;
    UA_Boolean failed = false;
    for(size_t rank = 0; rank <= UA_OPRANK_MAX; rank++) {
        for(size_t i = 0; i < refsSize; i++) {
            if(opRank(refs[i].configurationMask) != rank)
                continue;
            UA_ConfigUpdateOp *op = &ops[opsSize++];
            op->inputIndex = i;
            op->status = resolveOp(cfg, &refs[i], op);
            failed |= (op->status != UA_STATUSCODE_GOOD);
        }
    }

    /* Apply the operations. A failed complete update is rolled back. */
    UA_Boolean anyApplied = false;
    if(!failed || !ctx.atomic) {
        for(size_t i = 0; i < opsSize; i++) {
            UA_ConfigUpdateOp *op = &ops[i];
            UA_NodeId objId = UA_NODEID_NULL;
            if(op->status == UA_STATUSCODE_GOOD) {
                op->valuesBegin = result->configurationValuesSize;
                op->status = applyOp(&ctx, op, &objId);
                op->valuesEnd = result->configurationValuesSize;
            }
            bindResult(&ctx, op, op->status, objId);
            if(op->status != UA_STATUSCODE_GOOD) {
                failed = true;
                continue;
            }
            UA_NodeId_copy(&objId, &result->configurationObjects[op->inputIndex]);
            if(!op->matched) /* A match does not modify the configuration */
                anyApplied = true;
        }

        if(failed && ctx.atomic) {
            /* Undo the complete update. The top-level fields are not
             * applied. */
            anyApplied = !rollbackUpdate(&ctx);
            if(anyApplied)
                UA_LOG_ERROR(psm->logging, UA_LOGCATEGORY_PUBSUB,
                             "PubSub configuration update: The rollback of the "
                             "complete update is incomplete");
            rollbackResult(result, ops, opsSize);
        } else {
            /* Top-level fields are applied when the operations were
             * processed */
            if(applyTopLevelFields(psm, cfg))
                anyApplied = true;

            /* Enable the added components and restore the prior states */
            commitUpdate(&ctx);
        }
    }
    updateCtxClear(&ctx);

    /* Copy the per-reference status codes into the result */
    for(size_t i = 0; i < opsSize; i++) {
        result->referencesResults[ops[i].inputIndex] = ops[i].status;
        opClear(&ops[i]);
    }

    result->changesApplied = anyApplied;
    if(anyApplied)
        psm->configurationVersion =
            UA_PubSubConfigurationVersionTimeDifference(UA_DateTime_now());

    UA_free(ops);
    return UA_STATUSCODE_GOOD;
}

void
UA_PubSubConfigurationUpdateResult_clear(UA_PubSubConfigurationUpdateResult *result) {
    UA_Array_delete(result->referencesResults, result->referencesResultsSize,
                    &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_Array_delete(result->configurationObjects, result->configurationObjectsSize,
                    &UA_TYPES[UA_TYPES_NODEID]);
    /* The values array is over-allocated to the number of references. The
     * entries beyond configurationValuesSize are zero-initialized, so the
     * array-delete with the full allocation size is not possible. Clear the
     * used entries and free the memory. */
    for(size_t i = 0; i < result->configurationValuesSize; i++)
        UA_PubSubConfigurationValueDataType_clear(&result->configurationValues[i]);
    UA_free(result->configurationValues);
    memset(result, 0, sizeof(UA_PubSubConfigurationUpdateResult));
}

/* The common path of CloseAndUpdate and UA_Server_updatePubSubConfiguration */
UA_StatusCode
UA_PubSubManager_updateConfigFile(UA_PubSubManager *psm, const UA_ByteString *file,
                                  size_t refsSize,
                                  const UA_PubSubConfigurationRefDataType *refs,
                                  UA_Boolean requireCompleteUpdate,
                                  UA_PubSubConfigurationUpdateResult *result) {
    UA_LOCK_ASSERT(&psm->drv.server->serviceMutex);
    memset(result, 0, sizeof(UA_PubSubConfigurationUpdateResult));
    if(refsSize == 0 || !refs)
        return UA_STATUSCODE_BADNOTHINGTODO;

    /* Decode the file content. The configuration is a view into eo. */
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_PubSubConfiguration2DataType cfg;
    UA_StatusCode res = UA_PubSubManager_decodeConfig2Blob(psm, file, &eo, &cfg);
    if(res != UA_STATUSCODE_GOOD)
        return UA_STATUSCODE_BADTYPEMISMATCH;

    res = updateConfig2(psm, &cfg, refsSize, refs, requireCompleteUpdate, result);
    UA_ExtensionObject_clear(&eo);
    return res;
}

UA_StatusCode
UA_Server_updatePubSubConfiguration(UA_Server *server, const UA_ByteString *file,
                                    size_t referencesSize,
                                    const UA_PubSubConfigurationRefDataType *references,
                                    UA_Boolean requireCompleteUpdate,
                                    UA_PubSubConfigurationUpdateResult *result) {
    if(!server || !file || !result)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    memset(result, 0, sizeof(UA_PubSubConfigurationUpdateResult));

    lockServer(server);
    UA_PubSubManager *psm = getPSM(server);
    UA_StatusCode res = (psm) ?
        UA_PubSubManager_updateConfigFile(psm, file, referencesSize, references,
                                          requireCompleteUpdate, result) :
        UA_STATUSCODE_BADINTERNALERROR;
    unlockServer(server);
    return res;
}

#endif /* UA_ENABLE_PUBSUB && UA_ENABLE_PUBSUB_FILE_CONFIG */
