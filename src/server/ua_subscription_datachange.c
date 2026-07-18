/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2017-2020 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 *    Copyright 2017 (c) Stefan Profanter, fortiss GmbH
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2018 (c) Thomas Stalder, Blue Time Concept SA
 *    Copyright 2018 (c) Fabian Arndt, Root-Core
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"
#include "../ua_types_encoding_binary.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

void
markSemanticsChanged(UA_Server *server, const UA_NodeId *affected) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    const UA_Node *node = UA_NODESTORE_GET(server, affected);
    if(!node)
        return;

    if(node->head.nodeClass != UA_NODECLASS_VARIABLE &&
       node->head.nodeClass != UA_NODECLASS_VARIABLETYPE) {
        UA_NODESTORE_RELEASE(server, node);
        return;
    }

    UA_MonitoredItem *mon = node->head.monitoredItems;
    for(; mon != NULL; mon = mon->nodeListNext) {
        if(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_VALUE)
            continue;
        mon->semanticsChangedPending = true;
        if(mon->samplingType == UA_MONITOREDITEMSAMPLINGTYPE_EVENT)
            UA_MonitoredItem_sample(server, mon);
    }

    UA_NODESTORE_RELEASE(server, node);
}

/* Detect value changes outside the deadband.
 *
 * Integer types: compute the absolute difference in a wide unsigned type
 * (UA_UInt64) first, then widen only the *difference* to UA_Double for the
 * deadband comparison. This avoids both signed integer overflow (e.g.
 * (UA_SByte)(100 - (-50)) wraps to -106 instead of 150) and the 2^53
 * precision loss that comes from casting the operands to UA_Double before
 * subtracting: two huge but close Int64/UInt64 values would each round to a
 * nearby representable double and the subtraction would accumulate the
 * rounding error. Computing the exact integer magnitude first keeps the
 * difference exact (it fits in UA_Double's 53-bit mantissa for any realistic
 * deadband), so only the (usually small) delta is widened. The (UA_UInt64)
 * cast of a negative signed value yields the correct unsigned magnitude on
 * every platform. */
#define UA_DETECT_DEADBAND(TYPE) do {                                      \
    TYPE v1 = *(const TYPE*)data1;                                         \
    TYPE v2 = *(const TYPE*)data2;                                         \
    UA_UInt64 mag = (v1 > v2) ? (UA_UInt64)v1 - (UA_UInt64)v2               \
                              : (UA_UInt64)v2 - (UA_UInt64)v1;              \
    UA_Double diff = (UA_Double)mag;                                       \
    return (diff > deadband);                                              \
} while(false)

/* Floating-point types: the integer magnitude approach would truncate the
 * fractional part, so subtract directly in UA_Double. This is safe from
 * overflow and exact (Float widens to Double without loss). */
#define UA_DETECT_DEADBAND_FLOAT(TYPE) do {                                 \
    TYPE v1 = *(const TYPE*)data1;                                         \
    TYPE v2 = *(const TYPE*)data2;                                         \
    UA_Double diff = (v1 > v2) ? (UA_Double)v1 - (UA_Double)v2             \
                               : (UA_Double)v2 - (UA_Double)v1;            \
    return (diff > deadband);                                              \
} while(false)

static UA_Boolean
detectScalarDeadBand(const void *data1, const void *data2,
                     const UA_DataType *type, const UA_Double deadband) {
    if(type->typeKind == UA_DATATYPEKIND_SBYTE) {
        UA_DETECT_DEADBAND(UA_SByte);
    } else if(type->typeKind == UA_DATATYPEKIND_BYTE) {
        UA_DETECT_DEADBAND(UA_Byte);
    } else if(type->typeKind == UA_DATATYPEKIND_INT16) {
        UA_DETECT_DEADBAND(UA_Int16);
    } else if(type->typeKind == UA_DATATYPEKIND_UINT16) {
        UA_DETECT_DEADBAND(UA_UInt16);
    } else if(type->typeKind == UA_DATATYPEKIND_INT32) {
        UA_DETECT_DEADBAND(UA_Int32);
    } else if(type->typeKind == UA_DATATYPEKIND_UINT32) {
        UA_DETECT_DEADBAND(UA_UInt32);
    } else if(type->typeKind == UA_DATATYPEKIND_INT64) {
        UA_DETECT_DEADBAND(UA_Int64);
    } else if(type->typeKind == UA_DATATYPEKIND_UINT64) {
        UA_DETECT_DEADBAND(UA_UInt64);
    } else if(type->typeKind == UA_DATATYPEKIND_FLOAT) {
        UA_DETECT_DEADBAND_FLOAT(UA_Float);
    } else if(type->typeKind == UA_DATATYPEKIND_DOUBLE) {
        UA_DETECT_DEADBAND_FLOAT(UA_Double);
    } else {
        return false; /* Not a known numerical type */
    }
}

static UA_Boolean
detectVariantDeadband(const UA_Variant *value, const UA_Variant *oldValue,
                      const UA_Double deadbandValue) {
    /* Be careful to avoid a NULL access. We could have the value a scalar and
     * oldValue an empty array. Both define a type and arrayLength == 0. */
    if(value->arrayLength != oldValue->arrayLength)
        return true;
    if(value->type != oldValue->type)
        return true;
    if(UA_Variant_isScalar(value) != UA_Variant_isScalar(oldValue))
        return true;

    /* Treat scalars as an array of length 1 and iterate */
    size_t length = 1;
    if(!UA_Variant_isScalar(value))
        length = value->arrayLength;

    uintptr_t data = (uintptr_t)value->data;
    uintptr_t oldData = (uintptr_t)oldValue->data;
    UA_UInt32 memSize = value->type->memSize;
    for(size_t i = 0; i < length; ++i) {
        if(detectScalarDeadBand((const void*)data, (const void*)oldData,
                                value->type, deadbandValue))
            return true;
        data += memSize;
        oldData += memSize;
    }
    return false;
}

static UA_Boolean
detectValueChange(UA_Server *server, UA_MonitoredItem *mon, const UA_DataValue *dv) {
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Status changes are always reported */
    if(dv->hasStatus != mon->lastValue.hasStatus ||
       dv->status != mon->lastValue.status) {
        return true;
    }

    /* Default trigger is Status + Value */
    UA_DataChangeTrigger trigger = UA_DATACHANGETRIGGER_STATUSVALUE;

    /* Use the configured trigger */
    const UA_DataChangeFilter *dcf = NULL;
    const UA_ExtensionObject *filter = &mon->parameters.filter;
    if(filter->content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGEFILTER]) {
        dcf = (UA_DataChangeFilter*)filter->content.decoded.data;
        trigger = dcf->trigger;
    }

    /* The status was already tested above */
    if(trigger == UA_DATACHANGETRIGGER_STATUS)
        return false;

    UA_assert(trigger == UA_DATACHANGETRIGGER_STATUSVALUE ||
              trigger == UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP);

    /* Can we compare values? */
    if(dv->hasValue != mon->lastValue.hasValue)
       return true;

    /* Test absolute deadband */
    if(dcf && dcf->deadbandType == UA_DEADBANDTYPE_ABSOLUTE &&
       dv->value.type != NULL && UA_DataType_isNumeric(dv->value.type))
        return detectVariantDeadband(&dv->value, &mon->lastValue.value,
                                     dcf->deadbandValue);

    /* Compare the source timestamp if the trigger requires that */
    if(trigger == UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP) {
        if(dv->hasSourceTimestamp != mon->lastValue.hasSourceTimestamp)
            return true;
        if(dv->hasSourceTimestamp &&
           dv->sourceTimestamp != mon->lastValue.sourceTimestamp)
            return true;
    }

    /* Has the value changed? */
    return !UA_equal(&dv->value, &mon->lastValue.value,
                     &UA_TYPES[UA_TYPES_VARIANT]);
}

UA_StatusCode
UA_MonitoredItem_createDataChangeNotification(UA_Server *server, UA_MonitoredItem *mon,
                                              const UA_DataValue *dv) {
    /* Copy the value */
    UA_DataValue valueCopy;
    UA_StatusCode retval = UA_DataValue_copy(dv, &valueCopy);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* SemanticsChanged is a one-shot notification bit. Keep it out of
     * lastValue so it neither affects filtering nor causes a second status
     * change when the bit disappears. */
    if(mon->semanticsChangedPending) {
        valueCopy.hasStatus = true;
        valueCopy.status |= UA_STATUSCODE_SEMANTICSCHANGED;
    }

    /* Allocate a new notification */
    UA_Notification *n = UA_Notification_new();
    if(!n) {
        UA_DataValue_clear(&valueCopy);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Prepare and enqueue the notification */
    n->mon = mon;
    n->data.dataChange.value = valueCopy;
    n->data.dataChange.clientHandle = mon->parameters.clientHandle;
    UA_Notification_enqueueAndTrigger(server, n);
    mon->semanticsChangedPending = false;
    return UA_STATUSCODE_GOOD;
}

void
UA_MonitoredItem_processSampledValue(UA_Server *server, UA_MonitoredItem *mon,
                                     UA_DataValue *value) {
    UA_assert(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);
    UA_LOCK_ASSERT(&server->serviceMutex);

    /* Has the value changed (with the filters applied)? */
    UA_Boolean changed = mon->semanticsChangedPending ||
        detectValueChange(server, mon, value);
    if(!changed) {
        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, mon->subscription,
                                  "MonitoredItem %" PRIi32 " | "
                                  "The value has not changed", mon->monitoredItemId);
        UA_DataValue_clear(value);
        return;
    }

    /* Prepare a notification and enqueue it */
    UA_Boolean semanticsChanged = mon->semanticsChangedPending;
    UA_StatusCode res =
        UA_MonitoredItem_createDataChangeNotification(server, mon, value);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SUBSCRIPTION(server->config.logging, mon->subscription,
                                    "MonitoredItem %" PRIi32 " | "
                                    "Processing the sample returned the statuscode %s",
                                    mon->monitoredItemId, UA_StatusCode_name(res));
        UA_DataValue_clear(value);
        return;
    }

    /* Move/store the value for filter comparison and TransferSubscription */
    UA_DataValue_clear(&mon->lastValue);
    mon->lastValue = *value;

    /* Call the local callback if the MonitoredItem is not attached to a
     * subscription. Do this at the very end. Because the callback might delete
     * the subscription. */
    if(!mon->subscription) {
        if(semanticsChanged) {
            value->hasStatus = true;
            value->status |= UA_STATUSCODE_SEMANTICSCHANGED;
        }
        UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*) mon;
        void *nodeContext = NULL;
        getNodeContext(server, mon->itemToMonitor.nodeId, &nodeContext);
        localMon->callback.dataChangeCallback(server,
                                              mon->monitoredItemId, localMon->context,
                                              &mon->itemToMonitor.nodeId, nodeContext,
                                              mon->itemToMonitor.attributeId, value);
    }
}

/* We know the result is a deep-copy. So we can abuse the const result-pointer
 * and take ownership of the value. */
static void
processMonitoredItemAsyncRead(UA_Server *server, UA_MonitoredItem *mon,
                              const UA_DataValue *result) {
    mon->outstandingAsyncReads--;
    UA_DataValue *mut_result = (UA_DataValue*)(uintptr_t)result;
    if(mut_result->status == UA_STATUSCODE_BADREQUESTCANCELLEDBYREQUEST)
        return; /* Controlled shut-down */
    UA_MonitoredItem_processSampledValue(server, mon, mut_result);
    UA_DataValue_init(mut_result);
}

void
UA_MonitoredItem_sample(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex);
    UA_assert(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);

    UA_Subscription *sub = mon->subscription;
    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub, "MonitoredItem %" PRIi32
                              " | Sample callback called", mon->monitoredItemId);

    /* Sample the current value.
     * sub->session can be NULL when the subscription is detached. Then
     * readWithSession returns the error-code BADUSERACCESSDENIED. */
    UA_Session *session = (sub) ? sub->session : &server->adminSession;

    /* Read the value possibly asynchronous */
    UA_StatusCode res = UA_STATUSCODE_BADTOOMANYOPERATIONS;
    if(UA_LIKELY(mon->outstandingAsyncReads < UA_MONITOREDITEM_ASYNC_MAX)) {
        res = read_async(server, session, &mon->itemToMonitor, mon->timestampsToReturn,
                         (UA_ServerAsyncReadResultCallback)processMonitoredItemAsyncRead, mon, 0);
    }
    if(res == UA_STATUSCODE_GOOD) {
        mon->outstandingAsyncReads++;
    } else {
        /* Reading failed, process with the StatusCode */
        UA_DataValue dv;
        UA_DataValue_init(&dv);
        dv.hasStatus = true;
        dv.status = res;
        UA_MonitoredItem_processSampledValue(server, mon, &dv);
    }
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
