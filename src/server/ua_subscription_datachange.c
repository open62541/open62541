/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. 
 *
 *    Copyright 2017 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"
#include "ua_types_encoding_binary.h"

#ifdef UA_ENABLE_DA
#include <math.h> // fabs
#endif

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

#define UA_VALUENCODING_MAXSTACK 512

#define ABS_SUBTRACT_TYPE_INDEPENDENT(a,b) ((a)>(b)?(a)-(b):(b)-(a))

static UA_Boolean
outOfDeadBand(const void *data1, const void *data2, const size_t arrayPos,
              const UA_DataType *type, const UA_Double deadbandValue) {
    if(type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Boolean*)data1)[arrayPos],
                                         ((const UA_Boolean*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_SBYTE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_SByte*)data1)[arrayPos],
                                         ((const UA_SByte*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_BYTE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Byte*)data1)[arrayPos],
                                         ((const UA_Byte*)data2)[arrayPos]) <= deadbandValue)
                return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT16]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int16*)data1)[arrayPos],
                                         ((const UA_Int16*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT16]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt16*)data1)[arrayPos],
                                         ((const UA_UInt16*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT32]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int32*)data1)[arrayPos],
                                         ((const UA_Int32*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT32]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt32*)data1)[arrayPos],
                                         ((const UA_UInt32*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT64]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int64*)data1)[arrayPos],
                                         ((const UA_Int64*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT64]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt64*)data1)[arrayPos],
                                         ((const UA_UInt64*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_FLOAT]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Float*)data1)[arrayPos],
                                         ((const UA_Float*)data2)[arrayPos]) <= deadbandValue)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Double*)data1)[arrayPos],
                                         ((const UA_Double*)data2)[arrayPos]) <= deadbandValue)
            return false;
    }
    return true;
}

#ifdef UA_ENABLE_DA
static UA_INLINE UA_Boolean
outOfPercentDeadBand(const void *data1, const void *data2, const size_t index,
                     const UA_DataType *type, const UA_Double deadbandValue, UA_Range* range) {
    if(type == &UA_TYPES[UA_TYPES_SBYTE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_SByte*)data1)[index],
                                         ((const UA_SByte*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_SByte*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_BYTE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Byte*)data1)[index],
                                         ((const UA_Byte*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_Byte*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT16]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int16*)data1)[index],
                                         ((const UA_Int16*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_Int16*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT16]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt16*)data1)[index],
                                         ((const UA_UInt16*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_UInt16*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT32]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int32*)data1)[index],
                                         ((const UA_Int32*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_Int32*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT32]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt32*)data1)[index],
                                         ((const UA_UInt32*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_UInt32*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_INT64]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Int64*)data1)[index],
                                         ((const UA_Int64*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_Int64*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_UINT64]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_UInt64*)data1)[index],
                                         ((const UA_UInt64*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_UInt64*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_FLOAT]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Float*)data1)[index],
                                         ((const UA_Float*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_Float*)data1 > range->high)
            return false;
    } else if(type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        if(ABS_SUBTRACT_TYPE_INDEPENDENT(((const UA_Double*)data1)[index],
                                         ((const UA_Double*)data2)[index]) <= (deadbandValue/100.0)*(fabs(range->high - range->low)) ||
           *(const UA_Double*)data1 > range->high)
            return false;
    }
    return true;
}
#endif /* UA_ENABLE_DA */

static UA_INLINE UA_Boolean
updateNeededForFilteredValue(const UA_Variant *value, const UA_Variant *oldValue,
                             const UA_Double deadbandValue) {
    if(value->arrayLength != oldValue->arrayLength)
        return true;

    if(value->type != oldValue->type)
        return true;

    if (UA_Variant_isScalar(value)) {
        return outOfDeadBand(value->data, oldValue->data, 0, value->type, deadbandValue);
    }
    for (size_t i = 0; i < value->arrayLength; ++i) {
        if (outOfDeadBand(value->data, oldValue->data, i, value->type, deadbandValue))
            return true;
    }
    return false;
}

#ifdef UA_ENABLE_DA
static UA_INLINE UA_Boolean
updateNeededForFilteredPercentValue(const UA_Variant *value, const UA_Variant *oldValue,
                                    const UA_Double deadbandValue, UA_Range* euRange) {
    if(value->arrayLength != oldValue->arrayLength)
        return true;

    if(value->type != oldValue->type)
        return true;

    if (UA_Variant_isScalar(value)) {
        return outOfPercentDeadBand(value->data, oldValue->data, 0, value->type, deadbandValue, euRange);
    }
    for (size_t i = 0; i < value->arrayLength; ++i) {
        if (outOfPercentDeadBand(value->data, oldValue->data, i, value->type, deadbandValue, euRange))
            return true;
    }
    return false;
}

static UA_Boolean
updateNeededForStatusCode(const UA_DataValue *value, const UA_MonitoredItem *mon) {
    if (UA_Variant_isScalar(&value->value)) {
        if(value->status != mon->lastStatus)
          return true;
    }
    return false;
}
#endif


/* When a change is detected, encoding contains the heap-allocated binary
 * encoded value. The default for changed is false. */
static UA_StatusCode
detectValueChangeWithFilter(UA_Server *server, UA_MonitoredItem *mon, UA_DataValue *value,
                            UA_ByteString *encoding, UA_Boolean *changed) {
    if(UA_DataType_isNumeric(value->value.type) &&
       (mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUSVALUE ||
        mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP)) {
        if(mon->filter.dataChangeFilter.deadbandType == UA_DEADBANDTYPE_ABSOLUTE) {
            if(!updateNeededForFilteredValue(&value->value, &mon->lastValue,
                                             mon->filter.dataChangeFilter.deadbandValue))
                return UA_STATUSCODE_GOOD;
        }
#ifdef UA_ENABLE_DA
        else if(mon->filter.dataChangeFilter.deadbandType == UA_DEADBANDTYPE_PERCENT) {
            UA_QualifiedName qn = UA_QUALIFIEDNAME(0, "EURange");
            UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, mon->monitoredNodeId, 1, &qn);
            if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) { //if branch is not entried, property has been found
                  UA_BrowsePathResult_deleteMembers(&bpr);
                  return UA_STATUSCODE_GOOD;
            }
            const UA_VariableNode* node =
                (const UA_VariableNode*) UA_Nodestore_getNode(server->nsCtx, &bpr.targets->targetId.nodeId);
            UA_Range* euRange = (UA_Range*) node->value.data.value.value.data;
            if(!updateNeededForFilteredPercentValue(&value->value, &mon->lastValue,
                                                    mon->filter.dataChangeFilter.deadbandValue, euRange)) {
                if(!updateNeededForStatusCode(value, mon)) //when same value, but different status code is written
                  return UA_STATUSCODE_GOOD;
            }
        }
#endif
    }

    /* Stack-allocate some memory for the value encoding. We might heap-allocate
     * more memory if needed. This is just enough for scalars and small
     * structures. */
    UA_STACKARRAY(UA_Byte, stackValueEncoding, UA_VALUENCODING_MAXSTACK);
    UA_ByteString valueEncoding;
    valueEncoding.data = stackValueEncoding;
    valueEncoding.length = UA_VALUENCODING_MAXSTACK;

    /* Encode the value */
    UA_Byte *bufPos = valueEncoding.data;
    const UA_Byte *bufEnd = &valueEncoding.data[valueEncoding.length];
    UA_StatusCode retval = UA_encodeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE],
                                           &bufPos, &bufEnd, NULL, NULL);
    if(retval == UA_STATUSCODE_BADENCODINGERROR) {
        size_t binsize = UA_calcSizeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE]);
        if(binsize == 0)
            return UA_STATUSCODE_BADENCODINGERROR;

        if(binsize > UA_VALUENCODING_MAXSTACK) {
            retval = UA_ByteString_allocBuffer(&valueEncoding, binsize);
            if(retval == UA_STATUSCODE_GOOD) {
                bufPos = valueEncoding.data;
                bufEnd = &valueEncoding.data[valueEncoding.length];
                retval = UA_encodeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE],
                                         &bufPos, &bufEnd, NULL, NULL);
            }
        }
    }
    if(retval != UA_STATUSCODE_GOOD) {
        if(valueEncoding.data != stackValueEncoding)
            UA_ByteString_deleteMembers(&valueEncoding);
        return retval;
    }

    /* Has the value changed? */
    valueEncoding.length = (uintptr_t)bufPos - (uintptr_t)valueEncoding.data;
    *changed = (!mon->lastSampledValue.data ||
                !UA_String_equal(&valueEncoding, &mon->lastSampledValue));

    /* No change */
    if(!(*changed)) {
        if(valueEncoding.data != stackValueEncoding)
            UA_ByteString_deleteMembers(&valueEncoding);
        return UA_STATUSCODE_GOOD;
    }

    /* Change detected. Copy encoding on the heap if necessary. */
    if(valueEncoding.data == stackValueEncoding)
        return UA_ByteString_copy(&valueEncoding, encoding);

    *encoding = valueEncoding;
    return UA_STATUSCODE_GOOD;
}

/* Has this sample changed from the last one? The method may allocate additional
 * space for the encoding buffer. Detect the change in encoding->data. */
static UA_StatusCode
detectValueChange(UA_Server *server, UA_MonitoredItem *mon,
                  UA_DataValue value, UA_ByteString *encoding, UA_Boolean *changed) {
    /* Apply Filter */
    if(mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUS)
        value.hasValue = false;

    value.hasServerTimestamp = false;
    value.hasServerPicoseconds = false;
    if(mon->filter.dataChangeFilter.trigger < UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP) {
        value.hasSourceTimestamp = false;
        value.hasSourcePicoseconds = false;
    }

    /* Detect the value change */
    return detectValueChangeWithFilter(server, mon, &value, encoding, changed);
}

/* movedValue returns whether the sample was moved to the notification. The
 * default is false. */
static UA_StatusCode
sampleCallbackWithValue(UA_Server *server, UA_Session *session,
                        UA_Subscription *sub, UA_MonitoredItem *mon,
                        UA_DataValue *value, UA_Boolean *movedValue) {
    UA_assert(mon->attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);

    /* Contains heap-allocated binary encoding of the value if a change was detected */
    UA_ByteString binValueEncoding = UA_BYTESTRING_NULL;

    /* Has the value changed? Allocates memory in binValueEncoding if necessary.
     * value is edited internally so we make a shallow copy. */
    UA_Boolean changed = false;
    UA_StatusCode retval = detectValueChange(server, mon, *value, &binValueEncoding, &changed);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(&server->config.logger, session, "Subscription %u | "
                               "MonitoredItem %i | Value change detection failed with StatusCode %s",
                               sub ? sub->subscriptionId : 0, mon->monitoredItemId,
                               UA_StatusCode_name(retval));
        return retval;
    }
    if(!changed) {
        UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Subscription %u | "
                             "MonitoredItem %i | The value has not changed",
                             sub ? sub->subscriptionId : 0, mon->monitoredItemId);
        return UA_STATUSCODE_GOOD;
    }

    /* The MonitoredItem is attached to a subscription (not server-local).
     * Prepare a notification and enqueue it. */
    if(sub) {
        /* Allocate a new notification */
        UA_Notification *newNotification = (UA_Notification *)UA_malloc(sizeof(UA_Notification));
        if(!newNotification) {
            UA_ByteString_deleteMembers(&binValueEncoding);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }

        if(value->value.storageType == UA_VARIANT_DATA) {
            newNotification->data.value = *value; /* Move the value to the notification */
            *movedValue = true;
        } else { /* => (value->value.storageType == UA_VARIANT_DATA_NODELETE) */
            retval = UA_DataValue_copy(value, &newNotification->data.value);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_ByteString_deleteMembers(&binValueEncoding);
                UA_free(newNotification);
                return retval;
            }
        }

        /* <-- Point of no return --> */

        UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Subscription %u | "
                             "MonitoredItem %i | Enqueue a new notification",
                             sub ? sub->subscriptionId : 0, mon->monitoredItemId);

        newNotification->mon = mon;
        UA_Notification_enqueue(server, sub, mon, newNotification);
    }

    /* Store the encoding for comparison */
    UA_ByteString_deleteMembers(&mon->lastSampledValue);
    mon->lastSampledValue = binValueEncoding;

    /* Store the value for filter comparison (we don't want to decode
     * lastSampledValue in every iteration). Don't test the return code here. If
     * this fails, lastValue is empty and a notification will be forced for the
     * next deadband comparison. */
    if((mon->filter.dataChangeFilter.deadbandType == UA_DEADBANDTYPE_NONE ||
        mon->filter.dataChangeFilter.deadbandType == UA_DEADBANDTYPE_ABSOLUTE ||
        mon->filter.dataChangeFilter.deadbandType == UA_DEADBANDTYPE_PERCENT) &&
       (mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUS ||
        mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUSVALUE ||
        mon->filter.dataChangeFilter.trigger == UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP)) {
        UA_Variant_deleteMembers(&mon->lastValue);
        UA_Variant_copy(&value->value, &mon->lastValue);
#ifdef UA_ENABLE_DA
        UA_StatusCode_deleteMembers(&mon->lastStatus);
        UA_StatusCode_copy(&value->status, &mon->lastStatus);
#endif
    }

    /* Call the local callback if the MonitoredItem is not attached to a
     * subscription. Do this at the very end. Because the callback might delete
     * the subscription. */
    if(!sub) {
        UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*) mon;
        void *nodeContext = NULL;
        UA_Server_getNodeContext(server, mon->monitoredNodeId, &nodeContext);
        localMon->callback.dataChangeCallback(server, mon->monitoredItemId,
                                              localMon->context,
                                              &mon->monitoredNodeId,
                                              nodeContext, mon->attributeId,
                                              value);
    }

    return UA_STATUSCODE_GOOD;
}

void
UA_MonitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    UA_Subscription *sub = monitoredItem->subscription;
    UA_Session *session = &server->adminSession;
    if(sub)
        session = sub->session;

    UA_LOG_DEBUG_SESSION(&server->config.logger, session, "Subscription %u | "
                         "MonitoredItem %i | Sample callback called",
                         sub ? sub->subscriptionId : 0, monitoredItem->monitoredItemId);

    UA_assert(monitoredItem->attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);

    /* Get the node */
    const UA_Node *node = UA_Nodestore_getNode(server->nsCtx, &monitoredItem->monitoredNodeId);

    /* Sample the value. The sample can still point into the node. */
    UA_DataValue value;
    UA_DataValue_init(&value);
    if(node) {
        UA_ReadValueId rvid;
        UA_ReadValueId_init(&rvid);
        rvid.nodeId = monitoredItem->monitoredNodeId;
        rvid.attributeId = monitoredItem->attributeId;
        rvid.indexRange = monitoredItem->indexRange;
        ReadWithNode(node, server, session, monitoredItem->timestampsToReturn, &rvid, &value);
    } else {
        value.hasStatus = true;
        value.status = UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Operate on the sample */
    UA_Boolean movedValue = false;
    UA_StatusCode retval = sampleCallbackWithValue(server, session, sub, monitoredItem, &value, &movedValue);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SESSION(&server->config.logger, session, "Subscription %u | "
                               "MonitoredItem %i | Sampling returned the statuscode %s",
                               sub ? sub->subscriptionId : 0, monitoredItem->monitoredItemId,
                               UA_StatusCode_name(retval));
    }

    /* Delete the sample if it was not moved to the notification. */
    if(!movedValue)
        UA_DataValue_deleteMembers(&value); /* Does nothing for UA_VARIANT_DATA_NODELETE */
    if(node)
        UA_Nodestore_releaseNode(server->nsCtx, node);
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
