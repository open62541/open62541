/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

#define UA_MODELCHANGE_VALID_VERBS ((UA_Byte)0x1Fu)
#define UA_MODELCHANGE_INITIAL_CAPACITY 4u

void
UA_ModelChangeAccumulator_init(UA_ModelChangeAccumulator *acc) {
    memset(acc, 0, sizeof(*acc));
}

void
UA_ModelChangeAccumulator_clear(UA_ModelChangeAccumulator *acc) {
    UA_free(acc->nodeVersions);
    UA_Array_delete(acc->nodeVersionIds, acc->changesSize,
                    &UA_TYPES[UA_TYPES_NODEID]);
    UA_Array_delete(acc->changes, acc->changesSize,
                    &UA_TYPES[UA_TYPES_MODELCHANGESTRUCTUREDATATYPE]);
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

    if(newCapacity > SIZE_MAX / sizeof(UA_ModelChangeStructureDataType))
        return UA_STATUSCODE_BADOUTOFMEMORY;
    void *newChanges =
        UA_realloc(acc->changes,
                   newCapacity * sizeof(UA_ModelChangeStructureDataType));
    if(!newChanges)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    acc->changes = (UA_ModelChangeStructureDataType*)newChanges;

    void *newNodeVersionIds =
        UA_realloc(acc->nodeVersionIds, newCapacity * sizeof(UA_NodeId));
    if(!newNodeVersionIds)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    acc->nodeVersionIds = (UA_NodeId*)newNodeVersionIds;

    void *newNodeVersions =
        UA_realloc(acc->nodeVersions, newCapacity * sizeof(UA_Int64));
    if(!newNodeVersions)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    acc->nodeVersions = (UA_Int64*)newNodeVersions;
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

UA_StatusCode
UA_ModelChangeAccumulator_record(UA_Server *server,
                                 UA_ModelChangeAccumulator *acc,
                                 const UA_NodeId *affected,
                                 UA_Byte verb) {
    if(!server || !acc || !affected || UA_NodeId_isNull(affected) || verb == 0 ||
       (verb & (UA_Byte)~UA_MODELCHANGE_VALID_VERBS) != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_LOCK_ASSERT(&server->serviceMutex);

    for(size_t i = 0; i < acc->changesSize; ++i) {
        UA_ModelChangeStructureDataType *change = &acc->changes[i];
        if(!UA_NodeId_equal(&change->affected, affected))
            continue;

        /* A deleted node and its NodeVersion Property are no longer available
         * during finalization. Write the already assigned version now. */
        if((verb & UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED) &&
           !(change->verb & UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED)) {
            UA_StatusCode res =
                writeNodeVersion(server, acc->nodeVersionIds[i],
                                 acc->nodeVersions[i]);
            if(res != UA_STATUSCODE_GOOD)
                return res;
        }
        change->verb |= verb;
        return UA_STATUSCODE_GOOD;
    }

    /* Only nodes with a valid NodeVersion Property participate in
     * ModelChangeEvents. Capture the Property NodeId while the affected node is
     * still present, so finalization does not have to look it up again. */
    const UA_Node *node = UA_NODESTORE_GET(server, affected);
    if(!node)
        return UA_STATUSCODE_GOOD;

    UA_NodeId affectedType;
    UA_NodeId_init(&affectedType);
    if(node->head.nodeClass == UA_NODECLASS_OBJECT ||
       node->head.nodeClass == UA_NODECLASS_VARIABLE) {
        const UA_Node *type =
            getNodeType(server, &node->head, UA_NODEATTRIBUTESMASK_NONE,
                        UA_REFERENCETYPESET_NONE,
                        UA_BROWSEDIRECTION_INVALID);
        if(type) {
            UA_StatusCode typeRes =
                UA_NodeId_copy(&type->head.nodeId, &affectedType);
            UA_NODESTORE_RELEASE(server, type);
            if(typeRes != UA_STATUSCODE_GOOD) {
                UA_NODESTORE_RELEASE(server, node);
                return typeRes;
            }
        }
    }

    UA_NodeId nodeVersionId;
    UA_StatusCode res =
        getNodeVersionProperty(server, &node->head, &nodeVersionId);
    UA_NODESTORE_RELEASE(server, node);
    if(res == UA_STATUSCODE_BADNOTFOUND) {
        UA_NodeId_clear(&affectedType);
        return UA_STATUSCODE_GOOD;
    }
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&affectedType);
        return res;
    }

    res = reserveModelChanges(acc);
    if(res != UA_STATUSCODE_GOOD)
        goto cleanup;

    UA_ModelChangeStructureDataType *change = &acc->changes[acc->changesSize];
    UA_ModelChangeStructureDataType_init(change);
    res = UA_NodeId_copy(affected, &change->affected);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeId_copy(&affectedType, &change->affectedType);
    UA_NodeId_clear(&affectedType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ModelChangeStructureDataType_clear(change);
        goto cleanup;
    }
    acc->nodeVersionIds[acc->changesSize] = nodeVersionId;
    UA_NodeId_init(&nodeVersionId);
    acc->nodeVersions[acc->changesSize] = nextNodeVersion(server);
    change->verb = verb;

    if(verb & UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED) {
        res = writeNodeVersion(server, acc->nodeVersionIds[acc->changesSize],
                               acc->nodeVersions[acc->changesSize]);
        if(res != UA_STATUSCODE_GOOD) {
            UA_ModelChangeStructureDataType_clear(change);
            UA_NodeId_clear(&acc->nodeVersionIds[acc->changesSize]);
            goto cleanup;
        }
    }
    acc->changesSize++;
    res = UA_STATUSCODE_GOOD;

 cleanup:
    UA_NodeId_clear(&nodeVersionId);
    return res;
}

/* Update NodeVersion and discard changes for nodes that do not expose a valid
 * NodeVersion Property. Successfully updated entries are compacted in-place. */
static UA_StatusCode
updateNodeVersions(UA_Server *server, UA_ModelChangeAccumulator *acc) {
    UA_StatusCode result = UA_STATUSCODE_GOOD;
    size_t out = 0;
    for(size_t i = 0; i < acc->changesSize; ++i) {
        UA_ModelChangeStructureDataType *change = &acc->changes[i];
        UA_StatusCode res = UA_STATUSCODE_GOOD;
        if(!(change->verb & UA_MODELCHANGESTRUCTUREVERBMASK_NODEDELETED))
            res = writeNodeVersion(server, acc->nodeVersionIds[i],
                                   acc->nodeVersions[i]);
        if(res != UA_STATUSCODE_GOOD) {
            if(res != UA_STATUSCODE_BADNOTFOUND && result == UA_STATUSCODE_GOOD)
                result = res;
            UA_ModelChangeStructureDataType_clear(change);
            UA_NodeId_clear(&acc->nodeVersionIds[i]);
            continue;
        }

        if(out != i) {
            acc->changes[out] = *change;
            UA_ModelChangeStructureDataType_init(change);
            acc->nodeVersionIds[out] = acc->nodeVersionIds[i];
            UA_NodeId_init(&acc->nodeVersionIds[i]);
            acc->nodeVersions[out] = acc->nodeVersions[i];
        }
        out++;
    }
    acc->changesSize = out;
    return result;
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
    if(acc->changesSize == 0) {
        UA_ModelChangeAccumulator_clear(acc);
        return;
    }

    UA_STATIC_THREAD_LOCAL UA_KeyValuePair payload[1] = {
        {{0, UA_STRING_STATIC("/Changes")}, {0}}
    };
    UA_Variant_setArray(&payload[0].value, acc->changes, acc->changesSize,
                        &UA_TYPES[UA_TYPES_MODELCHANGESTRUCTUREDATATYPE]);
    UA_KeyValueMap eventFields = {1, payload};

    UA_EventDescription ed;
    memset(&ed, 0, sizeof(UA_EventDescription));
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = UA_NS0ID(GENERALMODELCHANGEEVENTTYPE);
    ed.eventFields = &eventFields;

    UA_StatusCode res = createEvent(server, &ed, NULL);
    if(res != UA_STATUSCODE_GOOD)
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Could not emit ModelChangeEvent: %s",
                       UA_StatusCode_name(res));
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

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
