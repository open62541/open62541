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
    acc->changesCapacity = newCapacity;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
updateAffectedType(UA_ModelChangeStructureDataType *change,
                   const UA_NodeId *affectedType) {
    if(!affectedType || UA_NodeId_isNull(affectedType) ||
       UA_NodeId_equal(&change->affectedType, affectedType))
        return UA_STATUSCODE_GOOD;

    /* Keep the old value intact if the allocation for the replacement fails. */
    UA_NodeId copy;
    UA_NodeId_init(&copy);
    UA_StatusCode res = UA_NodeId_copy(affectedType, &copy);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_NodeId_clear(&change->affectedType);
    change->affectedType = copy;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ModelChangeAccumulator_record(UA_ModelChangeAccumulator *acc,
                                 const UA_NodeId *affected,
                                 const UA_NodeId *affectedType,
                                 UA_Byte verb) {
    if(!acc || !affected || UA_NodeId_isNull(affected) || verb == 0 ||
       (verb & (UA_Byte)~UA_MODELCHANGE_VALID_VERBS) != 0)
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    for(size_t i = 0; i < acc->changesSize; ++i) {
        UA_ModelChangeStructureDataType *change = &acc->changes[i];
        if(!UA_NodeId_equal(&change->affected, affected))
            continue;
        UA_StatusCode res = updateAffectedType(change, affectedType);
        if(res != UA_STATUSCODE_GOOD)
            return res;
        change->verb |= verb;
        return UA_STATUSCODE_GOOD;
    }

    UA_StatusCode res = reserveModelChanges(acc);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_ModelChangeStructureDataType *change = &acc->changes[acc->changesSize];
    UA_ModelChangeStructureDataType_init(change);
    res = UA_NodeId_copy(affected, &change->affected);
    if(res == UA_STATUSCODE_GOOD && affectedType)
        res = UA_NodeId_copy(affectedType, &change->affectedType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_ModelChangeStructureDataType_clear(change);
        return res;
    }
    change->verb = verb;
    acc->changesSize++;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_ModelChangeAccumulator_finalize(UA_Server *server,
                                   UA_ModelChangeAccumulator *acc) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    if(!acc)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    if(acc->changesSize == 0) {
        UA_ModelChangeAccumulator_clear(acc);
        return UA_STATUSCODE_GOOD;
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
    UA_ModelChangeAccumulator_clear(acc);
    return res;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
