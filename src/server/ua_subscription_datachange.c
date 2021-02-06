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

#define UA_VALUENCODING_MAXSTACK 512

/* Convert to double first. We might loose differences for large Int64 that
 * cannot be precisely expressed as double. */
static UA_Boolean
outOfDeadBand(const void *data1, const void *data2,
              const UA_DataType *type, const UA_Double deadband) {
    UA_Double v;
    if(type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
        v = (UA_Double)*(const UA_Boolean*)data1 - (UA_Double)*(const UA_Boolean*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_SBYTE]) {
        v = (UA_Double)*(const UA_SByte*)data1 - (UA_Double)*(const UA_SByte*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_BYTE]) {
        v = (UA_Double)*(const UA_Byte*)data1 - (UA_Double)*(const UA_Byte*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_INT16]) {
        v = (UA_Double)*(const UA_Int16*)data1 - (UA_Double)*(const UA_Int16*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_UINT16]) {
        v = (UA_Double)*(const UA_UInt16*)data1 - (UA_Double)*(const UA_UInt16*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_INT32]) {
        v = (UA_Double)*(const UA_Int32*)data1 - (UA_Double)*(const UA_Int32*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_UINT32]) {
        v = (UA_Double)*(const UA_UInt32*)data1 - (UA_Double)*(const UA_UInt32*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_INT64]) {
        v = (UA_Double)*(const UA_Int64*)data1 - (UA_Double)*(const UA_Int64*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_UINT64]) {
        v = (UA_Double)*(const UA_UInt64*)data1 - (UA_Double)*(const UA_UInt64*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_FLOAT]) {
        v = (UA_Double)*(const UA_Float*)data1 - (UA_Double)*(const UA_Float*)data2;
    } else if(type == &UA_TYPES[UA_TYPES_DOUBLE]) {
        v = (UA_Double)*(const UA_Double*)data1 - (UA_Double)*(const UA_Double*)data2;
    } else {
        return false;
    }
    if(v < 0.0)
        v = -v;
    return (v > deadband);
}

static UA_Boolean
updateNeededForFilteredValue(const UA_Variant *value, const UA_Variant *oldValue,
                             const UA_Double deadbandValue) {
    if(value->arrayLength != oldValue->arrayLength)
        return true;

    if(value->type != oldValue->type)
        return true;

    size_t length = 1;
    if(!UA_Variant_isScalar(value))
        length = value->arrayLength;
    uintptr_t data = (uintptr_t)value->data;
    for(size_t i = 0; i < length; ++i) {
        if(outOfDeadBand((const void*)data, oldValue->data, value->type, deadbandValue))
            return true;
        data += value->type->memSize;
    }

    return false;
}

/* When a change is detected, encoding contains the heap-allocated binary
 * encoded value. The default for changed is false. */
static UA_StatusCode
detectValueChangeWithFilter(UA_Server *server, UA_Session *session, UA_MonitoredItem *mon,
                            UA_DataValue *value, UA_ByteString *encoding,
                            UA_Boolean *changed) {
    if(!value->value.type) {
        *changed = UA_ByteString_equal(encoding, &mon->lastSampledValue);
        return UA_STATUSCODE_GOOD;
    }

    /* Test absolute deadband */
    if(UA_DataType_isNumeric(value->value.type) &&
       mon->parameters.filter.content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGEFILTER]) {
        UA_DataChangeFilter *filter = (UA_DataChangeFilter*)
            mon->parameters.filter.content.decoded.data;
        if(filter->deadbandType == UA_DEADBANDTYPE_ABSOLUTE &&
           (filter->trigger == UA_DATACHANGETRIGGER_STATUSVALUE ||
            filter->trigger == UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP)) {
            *changed = updateNeededForFilteredValue(&value->value,
                                                    &mon->lastValue.value,
                                                    filter->deadbandValue);
            return UA_STATUSCODE_GOOD;
        }
    }

    /* Stack-allocate some memory for the value encoding. We might heap-allocate
     * more memory if needed. This is just enough for scalars and small
     * structures. */
    UA_Byte stackValueEncoding[UA_VALUENCODING_MAXSTACK];
    UA_ByteString valueEncoding;
    valueEncoding.data = stackValueEncoding;
    valueEncoding.length = UA_VALUENCODING_MAXSTACK;
    UA_Byte *bufPos = stackValueEncoding;
    const UA_Byte *bufEnd = &stackValueEncoding[UA_VALUENCODING_MAXSTACK];

    /* Encode the value */
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    size_t binsize = 0;
    if(mon->lastSampledValue.length <= UA_VALUENCODING_MAXSTACK) {
        /* Encode without using calcSizeBinary first. This will fail with
         * UA_STATUSCODE_BADENCODINGERROR once the end of the buffer is
         * reached. */
        retval = UA_encodeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE],
                                 &bufPos, &bufEnd, NULL, NULL);
        if(retval == UA_STATUSCODE_BADENCODINGERROR)
            goto encodeOnHeap; /* The buffer was not large enough */
    } else {
        /* Allocate a fitting buffer on the heap. This requires us to iterate
         * twice over the value (calcSizeBinary and encodeBinary) */
    encodeOnHeap:
        binsize = UA_calcSizeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE]);
        if(binsize == 0) {
            retval = UA_STATUSCODE_BADENCODINGERROR;
            goto cleanup;
        }
        retval = UA_ByteString_allocBuffer(&valueEncoding, binsize);
        if(retval != UA_STATUSCODE_GOOD)
            goto cleanup;
        bufPos = valueEncoding.data;
        bufEnd = &valueEncoding.data[valueEncoding.length];
        retval = UA_encodeBinary(value, &UA_TYPES[UA_TYPES_DATAVALUE],
                                 &bufPos, &bufEnd, NULL, NULL);
    }
    if(retval != UA_STATUSCODE_GOOD)
        goto cleanup;

    /* Has the value changed? */
    valueEncoding.length = (uintptr_t)bufPos - (uintptr_t)valueEncoding.data;
    *changed = !UA_String_equal(&valueEncoding, &mon->lastSampledValue);

    /* Change detected */
    if(*changed) {
        /* Move the heap-allocated encoding to the output and return */
        if(valueEncoding.data != stackValueEncoding) {
            *encoding = valueEncoding;
            return UA_STATUSCODE_GOOD;
        }
        /* Copy stack-allocated encoding to the output */
        retval = UA_ByteString_copy(&valueEncoding, encoding);
    }

    cleanup:
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR_SUBSCRIPTION(&server->config.logger, mon->subscription,
                                  "MonitoredItem %" PRIi32 " | "
                                  "Encoding the value failed with StatusCode %s",
                                  mon->monitoredItemId, UA_StatusCode_name(retval));
    }
    if(valueEncoding.data != stackValueEncoding)
        UA_ByteString_clear(&valueEncoding);
    return retval;
}

/* Has this sample changed from the last one? The method may allocate additional
 * space for the encoding buffer. Detect the change in encoding->data. */
static UA_StatusCode
detectValueChange(UA_Server *server, UA_Session *session, UA_MonitoredItem *mon,
                  UA_DataValue value, UA_ByteString *encoding, UA_Boolean *changed) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    /* Default trigger is statusvalue */
    UA_DataChangeTrigger trigger = UA_DATACHANGETRIGGER_STATUSVALUE;
    if(mon->parameters.filter.content.decoded.type == &UA_TYPES[UA_TYPES_DATACHANGEFILTER])
        trigger = ((UA_DataChangeFilter*)
                   mon->parameters.filter.content.decoded.data)->trigger;

    /* Apply Filter */
    if(trigger == UA_DATACHANGETRIGGER_STATUS)
        value.hasValue = false;

    value.hasServerTimestamp = false;
    value.hasServerPicoseconds = false;
    if(trigger < UA_DATACHANGETRIGGER_STATUSVALUETIMESTAMP) {
        value.hasSourceTimestamp = false;
        value.hasSourcePicoseconds = false;
    }

    /* Detect the value change */
    return detectValueChangeWithFilter(server, session, mon, &value, encoding, changed);
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
    UA_Notification_enqueueAndTrigger(server, newNotification);
    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                              "MonitoredItem %" PRIi32 " | "
                              "Enqueued a new notification", mon->monitoredItemId);
    return UA_STATUSCODE_GOOD;
}

/* Moves the value to the MonitoredItem if successful */
static UA_StatusCode
sampleCallbackWithValue(UA_Server *server, UA_Session *session,
                        UA_Subscription *sub, UA_MonitoredItem *mon,
                        UA_DataValue *value) {
    UA_assert(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);

    /* Contains heap-allocated binary encoding of the value if a change was detected */
    UA_ByteString binValueEncoding = UA_BYTESTRING_NULL;

    /* Has the value changed with the filter applied? Allocates memory in
     * binValueEncoding if necessary. The value structure is edited internally.
     * So we don't give a pointer argument and make a shallow copy instead. */
    UA_Boolean changed = false;
    UA_StatusCode retval = detectValueChange(server, session, mon, *value,
                                             &binValueEncoding, &changed);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SUBSCRIPTION(&server->config.logger, sub,
                                    "MonitoredItem %" PRIi32 " | "
                                    "Value change detection failed with StatusCode %s",
                                    mon->monitoredItemId, UA_StatusCode_name(retval));
        UA_DataValue_clear(value);
        return retval;
    }

    /* No change detected */
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
        retval = UA_MonitoredItem_createDataChangeNotification(server, sub, mon, value);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_ByteString_clear(&binValueEncoding);
            UA_DataValue_clear(value);
            return retval;
        }
    }

    /* <-- Point of no return --> */

    /* Store the encoding for comparison */
    UA_ByteString_clear(&mon->lastSampledValue);
    mon->lastSampledValue = binValueEncoding;

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
        UA_UNLOCK(server->serviceMutex);
        localMon->callback.dataChangeCallback(server,
                                              mon->monitoredItemId, localMon->context,
                                              &mon->itemToMonitor.nodeId, nodeContext,
                                              mon->itemToMonitor.attributeId, value);
        UA_LOCK(server->serviceMutex);
    }

    return UA_STATUSCODE_GOOD;
}

void
UA_MonitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    UA_LOCK(server->serviceMutex);
    monitoredItem_sampleCallback(server, monitoredItem);
    UA_UNLOCK(server->serviceMutex);
}

void
monitoredItem_sampleCallback(UA_Server *server, UA_MonitoredItem *monitoredItem) {
    UA_LOCK_ASSERT(server->serviceMutex, 1);

    UA_Subscription *sub = monitoredItem->subscription;
    UA_Session *session = &server->adminSession;
    if(sub)
        session = sub->session;

    UA_LOG_DEBUG_SUBSCRIPTION(&server->config.logger, sub,
                              "MonitoredItem %" PRIi32 " | "
                              "Sample callback called", monitoredItem->monitoredItemId);

    UA_assert(monitoredItem->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER);

    /* Get the node */
    const UA_Node *node = UA_NODESTORE_GET(server, &monitoredItem->itemToMonitor.nodeId);

    /* Sample the value. The sample can still point into the node. */
    UA_DataValue value;
    UA_DataValue_init(&value);
    if(node) {
        ReadWithNode(node, server, session, monitoredItem->timestampsToReturn,
                     &monitoredItem->itemToMonitor, &value);
    } else {
        value.hasStatus = true;
        value.status = UA_STATUSCODE_BADNODEIDUNKNOWN;
    }

    /* Operate on the sample. Don't touch value after this. */
    UA_StatusCode retval = sampleCallbackWithValue(server, session, sub,
                                                   monitoredItem, &value);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING_SUBSCRIPTION(&server->config.logger, sub,
                                    "MonitoredItem %" PRIi32 " | "
                                    "Sampling returned the statuscode %s",
                                    monitoredItem->monitoredItemId,
                                    UA_StatusCode_name(retval));
    }

    if(node)
        UA_NODESTORE_RELEASE(server, node);
}

#endif /* UA_ENABLE_SUBSCRIPTIONS */
