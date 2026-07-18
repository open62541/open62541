/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

#define UA_MODELCHANGE_VALID_VERBS ((UA_Byte)0x1Fu)
#define UA_MODELCHANGE_SEMANTIC UA_CHANGESTRUCTUREVERBMASK_SEMANTIC_INTERNAL
#define UA_MODELCHANGE_INTERNAL_VERBS \
    (UA_MODELCHANGE_VALID_VERBS | UA_MODELCHANGE_SEMANTIC)
#define UA_MODELCHANGE_INITIAL_CAPACITY 4u

void
UA_ModelChangeAccumulator_init(UA_ModelChangeAccumulator *acc) {
    memset(acc, 0, sizeof(*acc));
}

void
UA_ModelChangeAccumulator_clear(UA_ModelChangeAccumulator *acc) {
    for(size_t i = 0; i < acc->changesSize; ++i) {
        UA_ModelChangeStructureDataType_clear(&acc->changes[i].change);
        UA_NodeId_clear(&acc->changes[i].nodeVersionId);
    }
    UA_free(acc->changes);
    memset(acc, 0, sizeof(*acc));
}

static UA_StatusCode
reserveModelChanges(UA_ModelChangeAccumulator *acc) {
    if(acc->changesSize < acc->changesCapacity)
        return UA_STATUSCODE_GOOD;

    size_t newCapacity = acc->changesCapacity;
    if(newCapacity == 0)
        newCapacity = UA_MODELCHANGE_INITIAL_CAPACITY;
    else if(newCapacity <= SIZE_MAX / 2)
        newCapacity *= 2;
    else
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if(newCapacity > SIZE_MAX / sizeof(UA_ChangeEntry))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    void *newChanges =
        UA_realloc(acc->changes,
                   newCapacity * sizeof(UA_ChangeEntry));
    if(!newChanges)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    acc->changes = (UA_ChangeEntry*)newChanges;
    acc->changesCapacity = newCapacity;
    return UA_STATUSCODE_GOOD;
}

static UA_Int64
nextNodeVersion(UA_Server *server) {
    if(server->nodeVersionCounter == INT64_MAX)
        server->nodeVersionCounter = 1;
    else
        server->nodeVersionCounter++;
    return server->nodeVersionCounter;
}

static UA_StatusCode
writeNodeVersion(UA_Server *server, const UA_NodeId propertyId,
                 UA_Int64 nodeVersion) {
    UA_Byte versionBuffer[32];
    UA_String version = {sizeof(versionBuffer), versionBuffer};
    UA_String_format(&version, "%lld", (long long)nodeVersion);
    UA_Variant value;
    UA_Variant_setScalar(&value, &version, &UA_TYPES[UA_TYPES_STRING]);
    return writeValueAttribute(server, propertyId, &value);
}

struct TypeDefinitionContext {
    UA_StatusCode res;
    UA_NodeId *typeId;
};

static void *
copyTypeDefinition(void *context, UA_ReferenceTarget *target) {
    if(!UA_NodePointer_isLocal(target->targetId))
        return NULL;
    struct TypeDefinitionContext *ctx =
        (struct TypeDefinitionContext*)context;
    UA_NodeId typeId = UA_NodePointer_toNodeId(target->targetId);
    ctx->res = UA_NodeId_copy(&typeId, ctx->typeId);
    return (void*)0x01;
}

static UA_StatusCode
getTypeDefinition(const UA_NodeHead *head, UA_NodeId *typeId) {
    struct TypeDefinitionContext context =
        {UA_STATUSCODE_BADNOTFOUND, typeId};
    for(size_t i = 0; i < head->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &head->references[i];
        if(rk->isInverse || rk->referenceTypeIndex !=
                            UA_REFERENCETYPEINDEX_HASTYPEDEFINITION)
            continue;
        UA_NodeReferenceKind_iterate(rk, copyTypeDefinition, &context);
        break;
    }
    return context.res;
}

/* Add NodeVersion bookkeeping when an existing semantic-only entry later also
 * receives a structural ModelChange verb. */
static UA_StatusCode
addNodeVersionTracking(UA_Server *server, UA_ModelChangeAccumulator *acc,
                       size_t index, const UA_NodeId *affected) {
    UA_ChangeEntry *entry = &acc->changes[index];
    if(!UA_NodeId_isNull(&entry->nodeVersionId))
        return UA_STATUSCODE_GOOD;

    const UA_Node *node = UA_NODESTORE_GET(server, affected);
    if(!node)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_StatusCode res =
        getNodeVersionProperty(server, &node->head, &entry->nodeVersionId);
    UA_NODESTORE_RELEASE(server, node);
    if(res == UA_STATUSCODE_GOOD)
        entry->nodeVersion = nextNodeVersion(server);
    return res;
}

UA_StatusCode
UA_ModelChangeAccumulator_record(UA_Server *server,
                                 UA_ModelChangeAccumulator *acc,
                                 const UA_NodeId *affected,
                                 UA_Byte verb) {
    /* Accept standard ModelChange verbs and the private SemanticChange marker.
     * Reserved wire-level verb bits must not enter the accumulator. */
    if(!server || !acc || !affected || UA_NodeId_isNull(affected) || verb == 0 ||
       (verb & (UA_Byte)~UA_MODELCHANGE_INTERNAL_VERBS) != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Coalesce all changes for an affected Node into one transaction entry.
     * This also lets a single entry participate in both emitted EventTypes. */
    for(size_t i = 0; i < acc->changesSize; ++i) {
        UA_ChangeEntry *entry = &acc->changes[i];
        UA_ModelChangeStructureDataType *change = &entry->change;
        if(!UA_NodeId_equal(&change->affected, affected))
            continue;

        /* A semantic-only entry has no NodeVersion bookkeeping. If a model
         * verb arrives later, promote the entry before merging the verbs. */
        if((verb & UA_MODELCHANGE_VALID_VERBS) != 0 &&
           (change->verb & UA_MODELCHANGE_VALID_VERBS) == 0) {
            UA_StatusCode res =
                addNodeVersionTracking(server, acc, i, affected);
            if(res == UA_STATUSCODE_BADNOTFOUND)
                verb &= UA_MODELCHANGE_SEMANTIC;
            else if(res != UA_STATUSCODE_GOOD)
                return res;
        }

        /* A deleted node and its NodeVersion Property are no longer available
         * during finalization. Write the already assigned version now. */
        if((verb & UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED) &&
           !(change->verb & UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED)) {
            UA_StatusCode res =
                writeNodeVersion(server, entry->nodeVersionId,
                                 entry->nodeVersion);
            if(res != UA_STATUSCODE_GOOD)
                return res;
        }
        change->verb |= verb;
        return UA_STATUSCODE_GOOD;
    }

    /* A new entry needs metadata from the affected Node. Missing Nodes are
     * ignored: this can occur for changes reported after external teardown. */
    const UA_Node *node = UA_NODESTORE_GET(server, affected);
    if(!node)
        return UA_STATUSCODE_GOOD;

    /* Capture the TypeDefinition while the Node is available. This is needed
     * in the Event payload and cannot be recovered after a Node deletion. */
    UA_NodeId affectedType;
    UA_NodeId_init(&affectedType);
    if(node->head.nodeClass == UA_NODECLASS_OBJECT ||
       node->head.nodeClass == UA_NODECLASS_VARIABLE) {
        UA_StatusCode typeRes = getTypeDefinition(&node->head, &affectedType);
        if(typeRes != UA_STATUSCODE_GOOD &&
           typeRes != UA_STATUSCODE_BADNOTFOUND) {
            UA_NODESTORE_RELEASE(server, node);
            return typeRes;
        }
    }

    /* ModelChangeEvents and NodeVersion are inseparable. Semantic-only entries
     * deliberately skip this lookup and remain valid without NodeVersion. */
    UA_NodeId nodeVersionId;
    UA_NodeId_init(&nodeVersionId);
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    if(verb & UA_MODELCHANGE_VALID_VERBS)
        res = getNodeVersionProperty(server, &node->head, &nodeVersionId);
    UA_NODESTORE_RELEASE(server, node);
    if(res == UA_STATUSCODE_BADNOTFOUND) {
        /* Keep a simultaneous semantic change even if the model change cannot
         * be tracked because the affected node has no NodeVersion. */
        verb &= UA_MODELCHANGE_SEMANTIC;
        if(verb == 0) {
            UA_NodeId_clear(&affectedType);
            return UA_STATUSCODE_GOOD;
        }
        res = UA_STATUSCODE_GOOD;
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&affectedType);
        return res;
    }

    /* Allocate only after the entry is known to be eligible. */
    res = reserveModelChanges(acc);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Commit the new entry atomically: ownership of copied NodeIds moves into
     * the accumulator only after every copy has succeeded. */
    UA_ChangeEntry *entry = &acc->changes[acc->changesSize];
    memset(entry, 0, sizeof(*entry));
    UA_ModelChangeStructureDataType *change = &entry->change;
    UA_ModelChangeStructureDataType_init(change);
    res = UA_NodeId_copy(affected, &change->affected);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeId_copy(&affectedType, &change->affectedType);
    UA_NodeId_clear(&affectedType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ModelChangeStructureDataType_clear(change);
        goto cleanup;
    }
    entry->nodeVersionId = nodeVersionId;
    UA_NodeId_init(&nodeVersionId);
    entry->nodeVersion =
        (verb & UA_MODELCHANGE_VALID_VERBS) ? nextNodeVersion(server) : 0;
    change->verb = verb;

    /* A deleted Node and its NodeVersion Property disappear before outermost
     * finalization. Persist the assigned version while both still exist. */
    if(verb & UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED) {
        res = writeNodeVersion(server, entry->nodeVersionId,
                               entry->nodeVersion);
        if(res != UA_STATUSCODE_GOOD) {
            UA_ModelChangeStructureDataType_clear(change);
            UA_NodeId_clear(&entry->nodeVersionId);
            goto cleanup;
        }
    }
    acc->changesSize++;
    res = UA_STATUSCODE_GOOD;

 cleanup:
    UA_NodeId_clear(&nodeVersionId);
    return res;
}

/* Update NodeVersion and retain any semantic portion on failure. */
static UA_StatusCode
updateNodeVersions(UA_Server *server, UA_ModelChangeAccumulator *acc) {
    UA_StatusCode result = UA_STATUSCODE_GOOD;
    size_t out = 0;
    for(size_t i = 0; i < acc->changesSize; ++i) {
        UA_ChangeEntry *entry = &acc->changes[i];
        UA_ModelChangeStructureDataType *change = &entry->change;
        UA_StatusCode res = UA_STATUSCODE_GOOD;
        UA_Byte modelVerbs = change->verb & UA_MODELCHANGE_VALID_VERBS;
        if(modelVerbs != 0 &&
           !(modelVerbs & UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED))
            res = writeNodeVersion(server, entry->nodeVersionId,
                                   entry->nodeVersion);
        if(res != UA_STATUSCODE_GOOD) {
            if(res != UA_STATUSCODE_BADNOTFOUND && result == UA_STATUSCODE_GOOD)
                result = res;
            UA_NodeId_clear(&entry->nodeVersionId);
            if(change->verb & UA_MODELCHANGE_SEMANTIC) {
                /* A failed NodeVersion write suppresses only the model part. */
                change->verb = UA_MODELCHANGE_SEMANTIC;
            } else {
                UA_ModelChangeStructureDataType_clear(change);
                continue;
            }
        }

        if(out != i) {
            acc->changes[out] = *entry;
            memset(entry, 0, sizeof(*entry));
        }
        out++;
    }
    acc->changesSize = out;
    return result;
}

/* Project the model portion into the contiguous wire representation. The
 * accumulator retains ownership of all NodeIds. */
static UA_ModelChangeStructureDataType *
prepareModelChanges(UA_ModelChangeAccumulator *acc, size_t *changesSize) {
    *changesSize = 0;
    for(size_t i = 0; i < acc->changesSize; ++i) {
        if(acc->changes[i].change.verb & UA_MODELCHANGE_VALID_VERBS)
            (*changesSize)++;
    }
    if(*changesSize == 0)
        return NULL;

    UA_ModelChangeStructureDataType *changes = (UA_ModelChangeStructureDataType*)
        UA_malloc(*changesSize * sizeof(UA_ModelChangeStructureDataType));
    if(!changes)
        return NULL;

    size_t out = 0;
    for(size_t i = 0; i < acc->changesSize; ++i) {
        UA_ModelChangeStructureDataType change = acc->changes[i].change;
        change.verb &= UA_MODELCHANGE_VALID_VERBS;
        if(change.verb != 0)
            changes[out++] = change;
    }
    return changes;
}

/* Project the semantic portion into the contiguous wire representation. The
 * accumulator retains ownership of all NodeIds. */
static UA_SemanticChangeStructureDataType *
prepareSemanticChanges(UA_ModelChangeAccumulator *acc, size_t *changesSize) {
    *changesSize = 0;
    for(size_t i = 0; i < acc->changesSize; ++i) {
        if(acc->changes[i].change.verb & UA_MODELCHANGE_SEMANTIC)
            (*changesSize)++;
    }
    if(*changesSize == 0)
        return NULL;

    UA_SemanticChangeStructureDataType *changes =
        (UA_SemanticChangeStructureDataType*)
        UA_malloc(*changesSize * sizeof(UA_SemanticChangeStructureDataType));
    if(!changes)
        return NULL;

    size_t out = 0;
    for(size_t i = 0; i < acc->changesSize; ++i) {
        UA_ModelChangeStructureDataType *change = &acc->changes[i].change;
        if(!(change->verb & UA_MODELCHANGE_SEMANTIC))
            continue;
        changes[out].affected = change->affected;
        changes[out].affectedType = change->affectedType;
        out++;
    }
    return changes;
}

static void
emitChangeEvent(UA_Server *server, UA_NodeId eventType,
                const UA_DataType *changesType, void *changes,
                size_t changesSize, const char *eventName) {
    UA_KeyValuePair payload = {{0, UA_STRING_STATIC("/Changes")}, {0}};
    UA_Variant_setArray(&payload.value, changes, changesSize, changesType);
    UA_KeyValueMap eventFields = {1, &payload};

    UA_EventDescription ed;
    memset(&ed, 0, sizeof(UA_EventDescription));
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = eventType;
    ed.eventFields = &eventFields;

    UA_StatusCode res = createEvent(server, &ed, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not emit %s: %s", eventName,
                       UA_StatusCode_name(res));
}

void
UA_ModelChangeAccumulator_finalize(UA_Server *server,
                                   UA_ModelChangeAccumulator *acc) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_assert(acc);
    if(acc->changesSize == 0) {
        UA_ModelChangeAccumulator_clear(acc);
        return;
    }

    UA_StatusCode versionResult = updateNodeVersions(server, acc);
    if(versionResult != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not update all NodeVersion Properties: %s",
                       UA_StatusCode_name(versionResult));
    size_t modelChangesSize;
    UA_ModelChangeStructureDataType *modelChanges =
        prepareModelChanges(acc, &modelChangesSize);
    if(modelChangesSize > 0 && !modelChanges)
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not allocate ModelChangeEvent payload");

    size_t semanticChangesSize;
    UA_SemanticChangeStructureDataType *semanticChanges =
        prepareSemanticChanges(acc, &semanticChangesSize);
    if(semanticChangesSize > 0 && !semanticChanges)
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not allocate SemanticChangeEvent payload");

    if(modelChanges)
        emitChangeEvent(server, UA_NS0ID(GENERALMODELCHANGEEVENTTYPE),
                        &UA_TYPES[UA_TYPES_MODELCHANGESTRUCTUREDATATYPE],
                        modelChanges, modelChangesSize, "ModelChangeEvent");
    if(semanticChanges)
        emitChangeEvent(server, UA_NS0ID(SEMANTICCHANGEEVENTTYPE),
                        &UA_TYPES[UA_TYPES_SEMANTICCHANGESTRUCTUREDATATYPE],
                        semanticChanges, semanticChangesSize,
                        "SemanticChangeEvent");

#ifdef UA_ENABLE_SUBSCRIPTIONS
    for(size_t i = 0; i < acc->changesSize; i++) {
        UA_ModelChangeStructureDataType *change = &acc->changes[i].change;
        if(change->verb & UA_MODELCHANGE_SEMANTIC)
            markSemanticsChanged(server, &change->affected);
    }
#endif

    /* Both payloads are shallow projections. The accumulator owns NodeIds. */
    UA_free(modelChanges);
    UA_free(semanticChanges);
    UA_ModelChangeAccumulator_clear(acc);
}

void
beginModelChange(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(server->modelChangeSuppressionDepth == 0)
        server->modelChangeDepth++;
}

void
endModelChange(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(server->modelChangeSuppressionDepth > 0)
        return;
    UA_assert(server->modelChangeDepth > 0);
    server->modelChangeDepth--;
    if(server->modelChangeDepth > 0)
        return;

    /* Detach the completed accumulator before finalizing. Updating NodeVersion
     * uses the normal Write operation and therefore enters another model-change
     * scope. That nested scope must see a fresh accumulator. */
    UA_ModelChangeAccumulator completed = server->modelChanges;
    UA_ModelChangeAccumulator_init(&server->modelChanges);
    UA_ModelChangeAccumulator_finalize(server, &completed);
}

void
recordModelChangeEvent(UA_Server *server, const UA_NodeId *affected, UA_Byte verb) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(server->modelChangeSuppressionDepth > 0 || server->modelChangeDepth == 0)
        return;
    UA_StatusCode res =
        UA_ModelChangeAccumulator_record(server, &server->modelChanges,
                                         affected, verb);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not record model change for %N: %s",
                       *affected, UA_StatusCode_name(res));
}

static void *
recordSemanticOwner(void *context, UA_ReferenceTarget *target) {
    if(!UA_NodePointer_isLocal(target->targetId))
        return NULL;
    UA_Server *server = (UA_Server*)context;
    UA_NodeId owner = UA_NodePointer_toNodeId(target->targetId);
    recordModelChangeEvent(server, &owner, UA_MODELCHANGE_SEMANTIC);
    return NULL;
}

void
recordSemanticPropertyChange(UA_Server *server, const UA_NodeHead *property) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(server->modelChangeSuppressionDepth > 0 ||
       server->modelChangeDepth == 0)
        return;

    /* The common case has only a direct inverse HasProperty reference. */
    UA_Boolean resolveSubtypes = false;
    for(size_t i = 0; i < property->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &property->references[i];
        if(!rk->isInverse)
            continue;
        if(rk->referenceTypeIndex == UA_REFERENCETYPEINDEX_HASPROPERTY) {
            UA_NodeReferenceKind_iterate(rk, recordSemanticOwner, server);
            continue;
        }
        resolveSubtypes = true;
    }
    if(!resolveSubtypes)
        return;

    /* Resolve HasProperty subtypes only for other inverse reference kinds. */
    UA_ReferenceTypeSet propertyRefs;
    const UA_NodeId hasProperty = UA_NS0ID(HASPROPERTY);
    UA_StatusCode res =
        referenceTypeIndices(server, &hasProperty, &propertyRefs, true);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not resolve HasProperty subtypes: %s",
                       UA_StatusCode_name(res));
        return;
    }

    for(size_t i = 0; i < property->referencesSize; ++i) {
        UA_NodeReferenceKind *rk = &property->references[i];
        if(!rk->isInverse ||
           rk->referenceTypeIndex == UA_REFERENCETYPEINDEX_HASPROPERTY ||
           !UA_ReferenceTypeSet_contains(&propertyRefs,
                                         rk->referenceTypeIndex))
            continue;
        UA_NodeReferenceKind_iterate(rk, recordSemanticOwner, server);
    }
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
