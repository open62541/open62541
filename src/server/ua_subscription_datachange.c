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
detectValueChange(UA_Server *server, UA_MonitoredItem *mon, const UA_DataValue *dv) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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
    if(dv->hasValue != mon->lastValue.hasValue)
        return true;
    return !UA_equal(&dv->value, &mon->lastValue.value,
                     &UA_TYPES[UA_TYPES_VARIANT]);
}

UA_StatusCode
UA_MonitoredItem_createDataChangeNotification(UA_Server *server, UA_MonitoredItem *mon,
                                              const UA_DataValue *dv) {
    /* Allocate a new notification */
    UA_Notification *newNot = UA_Notification_new();
    if(!newNot)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Prepare the notification */
    newNot->mon = mon;
    newNot->data.dataChange.clientHandle = mon->parameters.clientHandle;
    UA_StatusCode retval = UA_DataValue_copy(dv, &newNot->data.dataChange.value);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(newNot);
        return retval;
    }

    /* Enqueue the notification */
    UA_Notification_enqueueAndTrigger(server, newNot);
    return UA_STATUSCODE_GOOD;
}

void
UA_MonitoredItem_processSampledValue(UA_Server *server, UA_MonitoredItem *mon,
                                     UA_DataValue *value) {
    UA_assert(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Has the value changed (with the filters applied)? */
    UA_Boolean changed = detectValueChange(server, mon, value);
    if(!changed) {
        UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, mon->subscription,
                                  "MonitoredItem %" PRIi32 " | "
                                  "The value has not changed", mon->monitoredItemId);
        UA_DataValue_clear(value);
        return;
    }

    /* Prepare a notification and enqueue it */
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
}

void
UA_MonitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK(&server->serviceMutex);
    monitoredItem_sampleCallback(server, mon);
    UA_UNLOCK(&server->serviceMutex);
}

void
monitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *mon) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_assert(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);

    UA_Subscription *sub = mon->subscription;
    UA_LOG_DEBUG_SUBSCRIPTION(server->config.logging, sub, "MonitoredItem %" PRIi32
                              " | Sample callback called", mon->monitoredItemId);

    /* Sample the current value */
    UA_Session *session = (sub) ? sub->session : &server->adminSession;
    UA_DataValue dv = readWithSession(server, session, &mon->itemToMonitor,
                                      mon->timestampsToReturn);

    /* Process the sample. This always clears the value. */
    UA_MonitoredItem_processSampledValue(server, mon, &dv);
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
