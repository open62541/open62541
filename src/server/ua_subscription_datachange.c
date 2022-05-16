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
#include "ua_types_encoding_binary.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS /* conditional compilation */

/* Detect value changes outside the deadband */
#define UA_DETECT_DEADBAND(TYPE) do {                           \
    TYPE v1 = *(const TYPE*)data1;                              \
    TYPE v2 = *(const TYPE*)data2;                              \
    TYPE diff = (v1 > v2) ? (TYPE)(v1 - v2) : (TYPE)(v2 - v1);  \
    return ((UA_Double)diff > deadband);                        \
} while(false);

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
        UA_DETECT_DEADBAND(UA_Float);
    } else if(type->typeKind == UA_DATATYPEKIND_DOUBLE) {
        UA_DETECT_DEADBAND(UA_Double);
    } else {
        return false; /* Not a known numerical type */
    }
}

static UA_Boolean
detectVariantDeadband(const UA_Variant *value, const UA_Variant *oldValue,
                      const UA_Double deadbandValue) {
    if(value->arrayLength != oldValue->arrayLength)
        return true;
    if(value->type != oldValue->type)
        return true;
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
detectValueChange(UA_Server *server, UA_MonitoredItem *mon,
                  const UA_DataValue *value) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Status changes are always reported */
    if(value->hasStatus != mon->lastValue.hasStatus ||
       value->status != mon->lastValue.status) {
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

    /* Test absolute deadband */
    if(dcf && dcf->deadbandType == UA_DEADBANDTYPE_ABSOLUTE &&
       value->value.type != NULL && UA_DataType_isNumeric(value->value.type))
        return detectVariantDeadband(&value->value, &mon->lastValue.value,
                                     dcf->deadbandValue);

    /* Compare the source timestamp if the trigger requires that */
    if(trigger == UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP) {
        if(value->hasSourceTimestamp != mon->lastValue.hasSourceTimestamp)
            return true;
        if(value->hasSourceTimestamp &&
           value->sourceTimestamp != mon->lastValue.sourceTimestamp)
            return true;
    }

    /* Has the value changed? */
    if(value->hasValue != mon->lastValue.hasValue)
        return true;
    return (UA_order(&value->value, &mon->lastValue.value,
                     &UA_TYPES[UA_TYPES_VARIANT]) != UA_ORDER_EQ);
}

UA_StatusCode
UA_MonitoredItem_createDataChangeNotification(UA_Server *server, UA_Subscription *sub,
                                              UA_MonitoredItem *mon,
                                              const UA_DataValue *value) {
    /* Allocate a new notification */
    UA_Notification *newNotification = UA_Notification_new();
    if(!newNotification)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Prepare the notification */
    newNotification->mon = mon;
    newNotification->data.dataChange.clientHandle = mon->parameters.clientHandle;
    UA_StatusCode retval = UA_DataValue_copy(value, &newNotification->data.dataChange.value);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newNotification);
        return retval;
    }

    /* Enqueue the notification */
    UA_assert(sub);
    UA_Notification_enqueueAndTrigger(server, newNotification);
    return UA_STATUSCODE_GOOD;
}

/* Moves the value to the MonitoredItem if successful */
UA_StatusCode
sampleCallbackWithValue(UA_Server *server, UA_Subscription *sub,
                        UA_MonitoredItem *mon, UA_DataValue *value) {
    UA_assert(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);

    /* Has the value changed (with the filters applied)? */
    UA_Boolean changed = detectValueChange(server, mon, value);
    if(!changed) {
        UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                                  "MonitoredItem %" PRIi32 " | "
                                  "The value has not changed", mon->monitoredItemId);
        UA_DataValue_clear(value);
        return UA_STATUSCODE_GOOD;
    }

    /* The MonitoredItem is attached to a subscription (not server-local).
     * Prepare a notification and enqueue it. */
    if(sub) {
        UA_StatusCode retval =
            UA_MonitoredItem_createDataChangeNotification(server, sub, mon, value);
        if(retval != UA_STATUSCODE_GOOD)
            return retval;
    }

    /* <-- Point of no return --> */

    /* Move/store the value for filter comparison and TransferSubscription */
    UA_DataValue_clear(&mon->lastValue);
    mon->lastValue = *value;

    /* Call the local callback if the MonitoredItem is not attached to a
     * subscription. Do this at the very end. Because the callback might delete
     * the subscription. */
    if(!sub) {
        UA_LocalMonitoredItem *localMon = (UA_LocalMonitoredItem*) mon;
        void *nodeContext = NULL;
        getNodeContext(server, mon->itemToMonitor.nodeId, &nodeContext);
        UA_UNLOCK(&server->serviceMutex);
        localMon->callback.dataChangeCallback(server,
                                              mon->monitoredItemId, localMon->context,
                                              &mon->itemToMonitor.nodeId, nodeContext,
                                              mon->itemToMonitor.attributeId, value);
        UA_LOCK(&server->serviceMutex);
    }

    return UA_STATUSCODE_GOOD;
}

void
UA_MonitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    UA_LOCK(&server->serviceMutex);
    monitoredItem_sampleCallback(server, monitoredItem);
    UA_UNLOCK(&server->serviceMutex);
}

void
monitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_Subscription *sub = monitoredItem->subscription;
    UA_Session *session = &server->adminSession;
    if(sub)
        session = sub->session;

    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                              "MonitoredItem %" PRIi32 " | "
                              "Sample callback called", monitoredItem->monitoredItemId);

    UA_assert(monitoredItem->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);

    /* Sample the value. The sample can still point into the node. */
    UA_DataValue value = UA_Server_readWithSession(server, session,
                                                   &monitoredItem->itemToMonitor,
                                                   monitoredItem->timestampsToReturn);

    /* Operate on the sample. The sample is consumed when the status is good. */
    UA_StatusCode res = sampleCallbackWithValue(server, sub, monitoredItem, &value);
    if(res != UA_STATUSCODE_GOOD) {
        UA_DataValue_clear(&value);
        UA_LOG_WARNING_SUBSCRIPTION(&server->config.logger, sub,
                                    "MonitoredItem %" PRIi32 " | "
                                    "Sampling returned the statuscode %s",
                                    monitoredItem->monitoredItemId,
                                    UA_StatusCode_name(res));
    }
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
