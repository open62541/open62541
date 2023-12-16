/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

/* We use a 16-Byte ByteString as an identifier */
UA_StatusCode
generateEventId(UA_ByteString *generatedId) {
    /* EventId is a ByteString, which is basically just a string
     * We will use a 16-Byte ByteString as an identifier */
    UA_StatusCode res = UA_ByteString_allocBuffer(generatedId, 16 * sizeof(UA_Byte));
    if(res != UA_STATUSCODE_GOOD)
        return res;
    UA_UInt32 *ids = (UA_UInt32*)generatedId->data;
    ids[0] = UA_UInt32_random();
    ids[1] = UA_UInt32_random();
    ids[2] = UA_UInt32_random();
    ids[3] = UA_UInt32_random();
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
createEvent(UA_Server *server, const UA_NodeId eventType, UA_NodeId *outNodeId) {
    if(!outNodeId) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                     "outNodeId must not be NULL. The event's NodeId must be returned "
                     "so it can be triggered.");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Make sure the eventType is a subtype of BaseEventType */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    if(!isNodeInTree_singleRef(server, &eventType, &baseEventTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                     "Event type must be a subtype of BaseEventType!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Create an ObjectNode which represents the event */
    UA_QualifiedName name;
    /*  set a dummy name. This is not used. */
    name = UA_QUALIFIEDNAME(0,"E");
    UA_NodeId newNodeId = UA_NODEID_NULL;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_StatusCode retval = addNode(server, UA_NODECLASS_OBJECT,
                                   UA_NODEID_NULL, /* Set a random unused NodeId */
                                   UA_NODEID_NULL, /* No parent */
                                   UA_NODEID_NULL, /* No parent reference */
                                   name,           /* an event does not have a name */
                                   eventType,      /* the type of the event */
                                   &oAttr,         /* default attributes are fine */
                                   &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                                   NULL,           /* no node context */
                                   &newNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                     "Adding event failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* Find the eventType variable */
    name = UA_QUALIFIEDNAME(0, "EventType");
    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, newNodeId, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_clear(&bpr);
        deleteNode(server, newNodeId, true);
        UA_NodeId_clear(&newNodeId);
        return retval;
    }

    /* Set the EventType */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&eventType, &UA_TYPES[UA_TYPES_NODEID]);
    retval = writeValueAttribute(server, bpr.targets[0].targetId.nodeId, &value);
    UA_BrowsePathResult_clear(&bpr);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteNode(server, newNodeId, true);
        UA_NodeId_clear(&newNodeId);
        return retval;
    }

    *outNodeId = newNodeId;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_createEvent(UA_Server *server, const UA_NodeId eventType,
                      UA_NodeId *outNodeId) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res = createEvent(server, eventType, outNodeId);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}

static UA_StatusCode
eventSetStandardFields(UA_Server *server, const UA_NodeId *event,
                       const UA_NodeId *origin, UA_ByteString *outEventId) {
    /* Set the SourceNode */
    UA_StatusCode retval;
    UA_QualifiedName name = UA_QUALIFIEDNAME(0, "SourceNode");
    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, *event, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_clear(&bpr);
        return retval;
    }
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalarCopy(&value, origin, &UA_TYPES[UA_TYPES_NODEID]);
    retval = writeValueAttribute(server, bpr.targets[0].targetId.nodeId, &value);
    UA_Variant_clear(&value);
    UA_BrowsePathResult_clear(&bpr);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the ReceiveTime */
    name = UA_QUALIFIEDNAME(0, "ReceiveTime");
    bpr = browseSimplifiedBrowsePath(server, *event, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_clear(&bpr);
        return retval;
    }

    UA_EventLoop *el = server->config.eventLoop;
    UA_DateTime rcvTime = el->dateTime_now(el);
    UA_Variant_setScalar(&value, &rcvTime, &UA_TYPES[UA_TYPES_DATETIME]);
    retval = writeValueAttribute(server, bpr.targets[0].targetId.nodeId, &value);
    UA_BrowsePathResult_clear(&bpr);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the EventId */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = generateEventId(&eventId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    name = UA_QUALIFIEDNAME(0, "EventId");
    bpr = browseSimplifiedBrowsePath(server, *event, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_ByteString_clear(&eventId);
        UA_BrowsePathResult_clear(&bpr);
        return retval;
    }
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    retval = writeValueAttribute(server, bpr.targets[0].targetId.nodeId, &value);
    UA_BrowsePathResult_clear(&bpr);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_clear(&eventId);
        return retval;
    }

    /* Return the EventId */
    if(outEventId)
        *outEventId = eventId;
    else
        UA_ByteString_clear(&eventId);

    return UA_STATUSCODE_GOOD;
}

/* Filters an event according to the filter specified by mon and then adds it to
 * mons notification queue */
UA_StatusCode
UA_MonitoredItem_addEvent(UA_Server *server, UA_MonitoredItem *mon,
                          const UA_NodeId *event) {
    /* Get the filter */
    if(mon->parameters.filter.content.decoded.type != &UA_TYPES[UA_TYPES_EVENTFILTER])
        return UA_STATUSCODE_BADFILTERNOTALLOWED;
    UA_EventFilter *eventFilter = (UA_EventFilter*)
        mon->parameters.filter.content.decoded.data;

    /* Allocate memory for the notification */
    UA_Notification *notification = UA_Notification_new();
    if(!notification)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* The MonitoredItem must be attached to a Subscription. This code path is
     * not taken for local MonitoredItems (once they are enabled for Events). */
    UA_assert(mon->subscription);
    UA_Subscription *sub = mon->subscription;
    UA_Session *session = sub->session;

    UA_EventFilterResult res; /* FilterResult contains only statuscodes. Ignored
                               * outside the initial setup/validation. */
    UA_EventFilterResult_init(&res);
    UA_StatusCode retval = filterEvent(server, session, event, eventFilter,
                                       &notification->data.event, &res);
    UA_EventFilterResult_clear(&res);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Notification_delete(notification);
        if(retval == UA_STATUSCODE_BADNOMATCH)
            return UA_STATUSCODE_GOOD;
        return retval;
    }

    notification->data.event.clientHandle = mon->parameters.clientHandle;
    notification->mon = mon;

    UA_Notification_enqueueAndTrigger(server, notification);
    return UA_STATUSCODE_GOOD;
}

#ifdef UA_ENABLE_HISTORIZING
static void
setHistoricalEvent(UA_Server *server, const UA_NodeId *origin,
                   const UA_NodeId *emitNodeId, const UA_NodeId *eventNodeId) {
    UA_Variant historicalEventFilterValue;
    UA_Variant_init(&historicalEventFilterValue);

    /* A HistoricalEventNode that has event history available will provide this property */
    UA_StatusCode retval =
        readObjectProperty(server, *emitNodeId,
                           UA_QUALIFIEDNAME(0, "HistoricalEventFilter"),
                           &historicalEventFilterValue);
    if(retval != UA_STATUSCODE_GOOD) {
        /* Do not vex users with no match errors */
        if(retval != UA_STATUSCODE_BADNOMATCH)
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Cannot read the HistoricalEventFilter property of a "
                           "listening node. StatusCode %s",
                           UA_StatusCode_name(retval));
        return;
    }

    /* If found then check if HistoricalEventFilter property has a valid value */
    if(UA_Variant_isEmpty(&historicalEventFilterValue) ||
       !UA_Variant_isScalar(&historicalEventFilterValue) ||
       historicalEventFilterValue.type != &UA_TYPES[UA_TYPES_EVENTFILTER]) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "HistoricalEventFilter property of a listening node "
                       "does not have a valid value");
        UA_Variant_clear(&historicalEventFilterValue);
        return;
    }

    /* Finally, if found and valid then filter */
    UA_EventFilter *filter = (UA_EventFilter*) historicalEventFilterValue.data;
    UA_EventFieldList efl;
    UA_EventFilterResult result;
    retval = filterEvent(server, &server->adminSession, eventNodeId, filter, &efl, &result);
    if(retval == UA_STATUSCODE_GOOD)
        server->config.historyDatabase.setEvent(server, server->config.historyDatabase.context,
                                                origin, emitNodeId, filter, &efl);
    UA_EventFilterResult_clear(&result);
    UA_Variant_clear(&historicalEventFilterValue);
    UA_EventFieldList_clear(&efl);
}
#endif

static const UA_NodeId objectsFolderId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_OBJECTSFOLDER}};
#define EMIT_REFS_ROOT_COUNT 4
static const UA_NodeId emitReferencesRoots[EMIT_REFS_ROOT_COUNT] =
    {{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ORGANIZES}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASEVENTSOURCE}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASNOTIFIER}}};

static const UA_NodeId isInFolderReferences[2] =
    {{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ORGANIZES}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}}};

UA_StatusCode
triggerEvent(UA_Server *server, const UA_NodeId eventNodeId,
             const UA_NodeId origin, UA_ByteString *outEventId,
             const UA_Boolean deleteEventNode) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_LOG_NODEID_DEBUG(&origin,
        UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
            "Events: An event is triggered on node %.*s",
            (int)nodeIdStr.length, nodeIdStr.data));

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    UA_Boolean isCallerAC = false;
    if(isConditionOrBranch(server, &eventNodeId, &origin, &isCallerAC)) {
        if(!isCallerAC) {
          UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                                 "Condition Events: Please use A&C API to trigger Condition Events 0x%08X",
                                  UA_STATUSCODE_BADINVALIDARGUMENT);
          return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    }
#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */

    /* Check that the origin node exists */
    const UA_Node *originNode = UA_NODESTORE_GET(server, &origin);
    if(!originNode) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                     "Origin node for event does not exist.");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_NODESTORE_RELEASE(server, originNode);

    /* Make sure the origin is in the ObjectsFolder (TODO: or in the ViewsFolder) */
    /* Only use Organizes and HasComponent to check if we are below the ObjectsFolder */
    UA_StatusCode retval;
    UA_ReferenceTypeSet refTypes;
    UA_ReferenceTypeSet_init(&refTypes);
    for(int i = 0; i < 2; ++i) {
        UA_ReferenceTypeSet tmpRefTypes;
        retval = referenceTypeIndices(server, &isInFolderReferences[i], &tmpRefTypes, true);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Events: Could not create the list of references and their subtypes "
                           "with StatusCode %s", UA_StatusCode_name(retval));
        }
        refTypes = UA_ReferenceTypeSet_union(refTypes, tmpRefTypes);
    }

    if(!isNodeInTree(server, &origin, &objectsFolderId, &refTypes)) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                     "Node for event must be in ObjectsFolder!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Update the standard fields of the event */
    retval = eventSetStandardFields(server, &eventNodeId, &origin, outEventId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Events: Could not set the standard event fields with StatusCode %s",
                       UA_StatusCode_name(retval));
        return retval;
    }

    /* List of nodes that emit the node. Events propagate upwards (bubble up) in
     * the node hierarchy. */
    UA_ExpandedNodeId *emitNodes = NULL;
    size_t emitNodesSize = 0;

    /* Add the server node to the list of nodes from which the event is emitted.
     * The server node emits all events.
     *
     * Part 3, 7.17: In particular, the root notifier of a Server, the Server
     * Object defined in Part 5, is always capable of supplying all Events from
     * a Server and as such has implied HasEventSource References to every event
     * source in a Server. */
    UA_NodeId emitStartNodes[2];
    emitStartNodes[0] = origin;
    emitStartNodes[1] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    /* Get all ReferenceTypes over which the events propagate */
    UA_ReferenceTypeSet emitRefTypes;
    UA_ReferenceTypeSet_init(&emitRefTypes);
    for(size_t i = 0; i < EMIT_REFS_ROOT_COUNT; i++) {
        UA_ReferenceTypeSet tmpRefTypes;
        retval = referenceTypeIndices(server, &emitReferencesRoots[i], &tmpRefTypes, true);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Events: Could not create the list of references for event "
                           "propagation with StatusCode %s", UA_StatusCode_name(retval));
            goto cleanup;
        }
        emitRefTypes = UA_ReferenceTypeSet_union(emitRefTypes, tmpRefTypes);
    }

    /* Get the list of nodes in the hierarchy that emits the event. */
    retval = browseRecursive(server, 2, emitStartNodes, UA_BROWSEDIRECTION_INVERSE,
                             &emitRefTypes, UA_NODECLASS_UNSPECIFIED, true,
                             &emitNodesSize, &emitNodes);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Events: Could not create the list of nodes listening on the "
                       "event with StatusCode %s", UA_StatusCode_name(retval));
        goto cleanup;
    }

    /* Add the event to the listening MonitoredItems at each relevant node */
    for(size_t i = 0; i < emitNodesSize; i++) {
        /* Get the node */
        const UA_Node *node = UA_NODESTORE_GET(server, &emitNodes[i].nodeId);
        if(!node)
            continue;

        /* Only consider objects */
        if(node->head.nodeClass != UA_NODECLASS_OBJECT) {
            UA_NODESTORE_RELEASE(server, node);
            continue;
        }

        /* Add event to monitoreditems */
        UA_MonitoredItem *mon = node->head.monitoredItems;
        for(; mon != NULL; mon = mon->sampling.nodeListNext) {
            /* Is this an Event-MonitoredItem? */
            if(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER)
                continue;
            retval = UA_MonitoredItem_addEvent(server, mon, &eventNodeId);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                               "Events: Could not add the event to a listening "
                               "node with StatusCode %s", UA_StatusCode_name(retval));
                retval = UA_STATUSCODE_GOOD; /* Only log problems with individual emit nodes */
            }
        }

        UA_NODESTORE_RELEASE(server, node);

        /* Add event entry in the historical database */
#ifdef UA_ENABLE_HISTORIZING
        if(server->config.historyDatabase.setEvent)
            setHistoricalEvent(server, &origin, &emitNodes[i].nodeId, &eventNodeId);
#endif
    }

    /* Delete the node representation of the event */
    if(deleteEventNode) {
        retval = deleteNode(server, eventNodeId, true);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Attempt to remove event using deleteNode failed. StatusCode %s",
                           UA_StatusCode_name(retval));
        }
    }

 cleanup:
    UA_Array_delete(emitNodes, emitNodesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return retval;
}

UA_StatusCode
UA_Server_triggerEvent(UA_Server *server, const UA_NodeId eventNodeId,
                       const UA_NodeId origin, UA_ByteString *outEventId,
                       const UA_Boolean deleteEventNode) {
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode res =
        triggerEvent(server, eventNodeId, origin, outEventId, deleteEventNode);
    UA_UNLOCK(&server->serviceMutex);
    return res;
}
#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
