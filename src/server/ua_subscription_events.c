/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

UA_StatusCode
UA_MonitoredItem_removeNodeEventCallback(UA_Server *server, UA_Session *session,
                                         UA_Node *node, void *data) {
    if(node->nodeClass != UA_NODECLASS_OBJECT)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_ObjectNode *on = (UA_ObjectNode*)node;

    if(!on->monitoredItemQueue)
        return UA_STATUSCODE_GOOD;

    /* data is the monitoredItemID */
    /* catch edge case that it's the first element */
    if(data == on->monitoredItemQueue) {
        on->monitoredItemQueue = on->monitoredItemQueue->next;
        return UA_STATUSCODE_GOOD;
    }

    UA_MonitoredItem *prev = on->monitoredItemQueue;
    for(UA_MonitoredItem *entry = prev->next; entry != NULL; entry = entry->next) {
        if(entry == (UA_MonitoredItem *)data) {
            prev->next = entry->next;
            break;
        }
        prev = entry;
    }

    return UA_STATUSCODE_GOOD;
}

typedef struct Events_nodeListElement {
    LIST_ENTRY(Events_nodeListElement) listEntry;
    UA_NodeId nodeId;
} Events_nodeListElement;

struct getNodesHandle {
    UA_Server *server;
    LIST_HEAD(Events_nodeList, Events_nodeListElement) nodes;
};

/* generates a unique event id */
static UA_StatusCode
UA_Event_generateEventId(UA_Server *server, UA_ByteString *generatedId) {
    /* EventId is a ByteString, which is basically just a string
     * We will use a 16-Byte ByteString as an identifier */
    generatedId->length = 16;
    generatedId->data = (UA_Byte *) UA_malloc(16 * sizeof(UA_Byte));
    if(!generatedId->data) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Server unable to allocate memory for EventId data.");
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* GUIDs are unique, have a size of 16 byte and already have
     * a generator so use that.
     * Make sure GUIDs really do have 16 byte, in case someone may
     * have changed that struct */
    UA_assert(sizeof(UA_Guid) == 16);
    UA_Guid tmpGuid = UA_Guid_random();
    memcpy(generatedId->data, &tmpGuid, 16);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_createEvent(UA_Server *server, const UA_NodeId eventType, UA_NodeId *outNodeId) {
    if(!outNodeId) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND, "outNodeId cannot be NULL!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Make sure the eventType is a subtype of BaseEventType */
    UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    if(!isNodeInTree(server->nsCtx, &eventType, &baseEventTypeId, &hasSubtypeId, 1)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Event type must be a subtype of BaseEventType!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Create an ObjectNode which represents the event */
    UA_QualifiedName name;
    // set a dummy name. This is not used.
    name = UA_QUALIFIEDNAME(0,"E");
    UA_NodeId newNodeId = UA_NODEID_NULL;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_StatusCode retval =
        UA_Server_addObjectNode(server,
                                UA_NODEID_NULL, /* Set a random unused NodeId */
                                UA_NODEID_NULL, /* No parent */
                                UA_NODEID_NULL, /* No parent reference */
                                name,           /* an event does not have a name */
                                eventType,      /* the type of the event */
                                oAttr,          /* default attributes are fine */
                                NULL,           /* no node context */
                                &newNodeId);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Adding event failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* Find the eventType variable */
    name = UA_QUALIFIEDNAME(0, "EventType");
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, newNodeId, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_deleteMembers(&bpr);
        UA_Server_deleteNode(server, newNodeId, true);
        UA_NodeId_deleteMembers(&newNodeId);
        return retval;
    }

    /* Set the EventType */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&eventType, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_deleteMembers(&bpr);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Server_deleteNode(server, newNodeId, true);
        UA_NodeId_deleteMembers(&newNodeId);
        return retval;
    }

    *outNodeId = newNodeId;
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
isValidEvent(UA_Server *server, const UA_NodeId *validEventParent, const UA_NodeId *eventId) {
    /* find the eventType variableNode */
    UA_QualifiedName findName = UA_QUALIFIEDNAME(0, "EventType");
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, *eventId, 1, &findName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_deleteMembers(&bpr);
        return false;
    }
    
    /* Get the EventType Property Node */
    UA_Variant tOutVariant;
    UA_Variant_init(&tOutVariant);

    /* Read the Value of EventType Property Node (the Value should be a NodeId) */
    UA_StatusCode retval = UA_Server_readValue(server, bpr.targets[0].targetId.nodeId, &tOutVariant);
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_NODEID])) {
        UA_BrowsePathResult_deleteMembers(&bpr);
        return false;
    }

    const UA_NodeId *tEventType = (UA_NodeId*)tOutVariant.data;

    /* Make sure the EventType is not a Subtype of CondtionType
     * First check for filter set using UaExpert
     * (ConditionId Clause won't be present in Events, which are not Conditions)
     * Second check for Events which are Conditions or Alarms (Part 9 not supported yet) */
    UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    if(UA_NodeId_equal(validEventParent, &conditionTypeId) ||
       isNodeInTree(server->nsCtx, tEventType, &conditionTypeId, &hasSubtypeId, 1)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Alarms and Conditions are not supported yet!");
        UA_BrowsePathResult_deleteMembers(&bpr);
        UA_Variant_deleteMembers(&tOutVariant);
        return false;
    }

    /* check whether Valid Event other than Conditions */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    UA_Boolean isSubtypeOfBaseEvent = isNodeInTree(server->nsCtx, tEventType,
                                                   &baseEventTypeId, &hasSubtypeId, 1);

    UA_BrowsePathResult_deleteMembers(&bpr);
    UA_Variant_deleteMembers(&tOutVariant);
    return isSubtypeOfBaseEvent;
}

/* static UA_StatusCode */
/* whereClausesApply(UA_Server *server, const UA_ContentFilter whereClause, */
/*                   UA_EventFieldList *efl, UA_Boolean *result) { */
/*     /\* if the where clauses aren't specified leave everything as is *\/ */
/*     if(whereClause.elementsSize == 0) { */
/*         *result = true; */
/*         return UA_STATUSCODE_GOOD; */
/*     } */

/*     /\* where clauses were specified *\/ */
/*     UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND, */
/*                    "Where clauses are not supported by the server."); */
/*     *result = true; */
/*     return UA_STATUSCODE_BADNOTSUPPORTED; */
/* } */

/* Part 4: 7.4.4.5 SimpleAttributeOperand
 * The clause can point to any attribute of nodes. Either a child of the event
 * node and also the event type. */
static UA_StatusCode
resolveSimpleAttributeOperand(UA_Server *server, UA_Session *session, const UA_NodeId *origin,
                              const UA_SimpleAttributeOperand *sao, UA_Variant *value) {
    /* Prepare the ReadValueId */
    UA_ReadValueId rvi;
    UA_ReadValueId_init(&rvi);
    rvi.indexRange = sao->indexRange;
    rvi.attributeId = sao->attributeId;

    /* If this list (browsePath) is empty the Node is the instance of the
     * TypeDefinition. */
    if(sao->browsePathSize == 0) {
        rvi.nodeId = sao->typeDefinitionId;
        UA_DataValue v = UA_Server_readWithSession(server, session, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
        if(v.status == UA_STATUSCODE_GOOD && v.hasValue)
            *value = v.value;
        return v.status;
    }

    /* Resolve the browse path */
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, *origin, sao->browsePathSize, sao->browsePath);
    if(bpr.targetsSize == 0 && bpr.statusCode == UA_STATUSCODE_GOOD)
        bpr.statusCode = UA_STATUSCODE_BADNOTFOUND;
    if(bpr.statusCode != UA_STATUSCODE_GOOD) {
        UA_StatusCode retval = bpr.statusCode;
        UA_BrowsePathResult_deleteMembers(&bpr);
        return retval;
    }

    /* Read the first matching element. Move the value to the output. */
    rvi.nodeId = bpr.targets[0].targetId.nodeId;
    UA_DataValue v = UA_Server_readWithSession(server, session, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
    if(v.status == UA_STATUSCODE_GOOD && v.hasValue)
        *value = v.value;

    UA_BrowsePathResult_deleteMembers(&bpr);
    return v.status;
}

/* filters the given event with the given filter and writes the results into a notification */
static UA_StatusCode
UA_Server_filterEvent(UA_Server *server, UA_Session *session,
                      const UA_NodeId *eventNode, UA_EventFilter *filter,
                      UA_EventNotification *notification) {
    if (filter->selectClausesSize == 0)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    /* setup */
    UA_EventFieldList_init(&notification->fields);

    /* EventFilterResult isn't being used currently
    UA_EventFilterResult_init(&notification->result); */

    notification->fields.eventFieldsSize = filter->selectClausesSize;
    notification->fields.eventFields = (UA_Variant *) UA_Array_new(notification->fields.eventFieldsSize,
                                                                    &UA_TYPES[UA_TYPES_VARIANT]);
    if (!notification->fields.eventFields) {
        /* EventFilterResult currently isn't being used
        UA_EventFiterResult_deleteMembers(&notification->result); */
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    /* EventFilterResult currently isn't being used
    notification->result.selectClauseResultsSize = filter->selectClausesSize;
    notification->result.selectClauseResults = (UA_StatusCode *)
        UA_Array_new(filter->selectClausesSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if (!notification->result->selectClauseResults) {
        UA_EventFieldList_deleteMembers(&notification->fields);
        UA_EventFilterResult_deleteMembers(&notification->result);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    */

    /* ================ apply the filter ===================== */
    /* check if the browsePath is BaseEventType, in which case nothing more needs to be checked */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    /* iterate over the selectClauses */
    for(size_t i = 0; i < filter->selectClausesSize; i++) {
        if(!UA_NodeId_equal(&filter->selectClauses[i].typeDefinitionId, &baseEventTypeId) &&
           !isValidEvent(server, &filter->selectClauses[i].typeDefinitionId, eventNode)) {
            UA_Variant_init(&notification->fields.eventFields[i]);
            /* EventFilterResult currently isn't being used
            notification->result.selectClauseResults[i] = UA_STATUSCODE_BADTYPEDEFINITIONINVALID; */
            continue;
        }

        /* TODO: Put the result into the selectClausResults */
        resolveSimpleAttributeOperand(server, session, eventNode,
                                      &filter->selectClauses[i],
                                      &notification->fields.eventFields[i]);
    }
    /* UA_Boolean whereClauseResult = true; */
    /* return whereClausesApply(server, filter->whereClause, &notification->fields, &whereClauseResult); */
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
eventSetStandardFields(UA_Server *server, const UA_NodeId *event,
                       const UA_NodeId *origin, UA_ByteString *outEventId) {
    /* Set the SourceNode */
    UA_StatusCode retval;
    UA_QualifiedName name = UA_QUALIFIEDNAME(0, "SourceNode");
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, *event, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_deleteMembers(&bpr);
        return retval;
    }
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalarCopy(&value, origin, &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_Variant_deleteMembers(&value);
    UA_BrowsePathResult_deleteMembers(&bpr);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the ReceiveTime */
    name = UA_QUALIFIEDNAME(0, "ReceiveTime");
    bpr = UA_Server_browseSimplifiedBrowsePath(server, *event, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_BrowsePathResult_deleteMembers(&bpr);
        return retval;
    }
    UA_DateTime rcvTime = UA_DateTime_now();
    UA_Variant_setScalar(&value, &rcvTime, &UA_TYPES[UA_TYPES_DATETIME]);
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_deleteMembers(&bpr);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the EventId */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = UA_Event_generateEventId(server, &eventId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;
    name = UA_QUALIFIEDNAME(0, "EventId");
    bpr = UA_Server_browseSimplifiedBrowsePath(server, *event, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        retval = bpr.statusCode;
        UA_ByteString_deleteMembers(&eventId);
        UA_BrowsePathResult_deleteMembers(&bpr);
        return retval;
    }
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_deleteMembers(&bpr);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ByteString_deleteMembers(&eventId);
        return retval;
    }

    /* Return the EventId */
    if(outEventId)
        *outEventId = eventId;
    else
        UA_ByteString_deleteMembers(&eventId);

    return UA_STATUSCODE_GOOD;
}

/* Insert each node into the list (passed as handle) */
static UA_StatusCode
getParentsNodeIteratorCallback(UA_NodeId parentId, UA_Boolean isInverse,
                               UA_NodeId referenceTypeId, struct getNodesHandle *handle) {
    /* Parents have an inverse reference */
    if(!isInverse)
        return UA_STATUSCODE_GOOD;

    /* Is this a hierarchical reference? */
    if(!isNodeInTree(handle->server->nsCtx, &referenceTypeId,
                     &hierarchicalReferences, &subtypeId, 1))
        return UA_STATUSCODE_GOOD;

    Events_nodeListElement *entry = (Events_nodeListElement *)
        UA_malloc(sizeof(Events_nodeListElement));
    if(!entry)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode retval = UA_NodeId_copy(&parentId, &entry->nodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(entry);
        return retval;
    }

    LIST_INSERT_HEAD(&handle->nodes, entry, listEntry);

    /* Recursion */
    UA_Server_forEachChildNodeCall(handle->server, parentId, (UA_NodeIteratorCallback)getParentsNodeIteratorCallback, handle);
    return UA_STATUSCODE_GOOD;
}

/* Filters an event according to the filter specified by mon and then adds it to
 * mons notification queue */
static UA_StatusCode
UA_Event_addEventToMonitoredItem(UA_Server *server, const UA_NodeId *event,
                                 UA_MonitoredItem *mon) {
    UA_Notification *notification = (UA_Notification *) UA_malloc(sizeof(UA_Notification));
    if(!notification)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    /* Get the session */
    UA_Subscription *sub = mon->subscription;
    UA_Session *session = sub->session;

    /* Apply the filter */
    UA_StatusCode retval = UA_Server_filterEvent(server, session, event,
                                                 &mon->filter.eventFilter,
                                                 &notification->data.event);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(notification);
        return retval;
    }

    /* Enqueue the notification */
    notification->mon = mon;
    UA_Notification_enqueue(server, mon->subscription, mon, notification);
    return UA_STATUSCODE_GOOD;
}

static const UA_NodeId objectsFolderId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_OBJECTSFOLDER}};
static const UA_NodeId parentReferences_events[2] =
    {{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ORGANIZES}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}}};

UA_StatusCode
UA_Server_triggerEvent(UA_Server *server, const UA_NodeId eventNodeId, const UA_NodeId origin,
                       UA_ByteString *outEventId, const UA_Boolean deleteEventNode) {
    /* Check that the origin node exists */
    const UA_Node *originNode = UA_Nodestore_getNode(server->nsCtx, &origin);
    if(!originNode) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Origin node for event does not exist.");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_Nodestore_releaseNode(server->nsCtx, originNode);


    /* Make sure the origin is in the ObjectsFolder (TODO: or in the ViewsFolder) */
    UA_NodeId *parentTypeHierachy = NULL;
    size_t parentTypeHierachySize = 0;
    getTypesHierarchy(server->nsCtx, parentReferences_events, 2,
                      &parentTypeHierachy, &parentTypeHierachySize, true);
    UA_Boolean isInObjectsFolder = isNodeInTree(server->nsCtx, &origin, &objectsFolderId,
                                                parentTypeHierachy, parentTypeHierachySize);
    UA_Array_delete(parentTypeHierachy, parentTypeHierachySize, &UA_TYPES[UA_TYPES_NODEID]);
    if (!isInObjectsFolder) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Node for event must be in ObjectsFolder!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval = eventSetStandardFields(server, &eventNodeId, &origin, outEventId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Events: Could not set the standard event fields with StatusCode %s",
                       UA_StatusCode_name(retval));
        return retval;
    }

    /* Get an array with all parents. The first call to
     * getParentsNodeIteratorCallback adds the emitting node itself. */
    struct getNodesHandle parentHandle;
    parentHandle.server = server;
    LIST_INIT(&parentHandle.nodes);
    retval = getParentsNodeIteratorCallback(origin, true, parentReferences_events[1], &parentHandle);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "Events: Could not create the list of nodes listening on the "
                       "event with StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* Add the event to each node's monitored items */
    Events_nodeListElement *parentIter, *tmp_parentIter;
    LIST_FOREACH_SAFE(parentIter, &parentHandle.nodes, listEntry, tmp_parentIter) {
        const UA_ObjectNode *node = (const UA_ObjectNode*)
            UA_Nodestore_getNode(server->nsCtx, &parentIter->nodeId);
        if(node->nodeClass == UA_NODECLASS_OBJECT) {
            for(UA_MonitoredItem *monIter = node->monitoredItemQueue; monIter != NULL; monIter = monIter->next) {
                retval = UA_Event_addEventToMonitoredItem(server, &eventNodeId, monIter);
                if(retval != UA_STATUSCODE_GOOD) {
                    UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                   "Events: Could not add the event to a listening node with StatusCode %s",
                                   UA_StatusCode_name(retval));
                }
            }
        }
        UA_Nodestore_releaseNode(server->nsCtx, (const UA_Node*)node);

        LIST_REMOVE(parentIter, listEntry);
        UA_NodeId_deleteMembers(&parentIter->nodeId);
        UA_free(parentIter);
    }

    /* Delete the node representation of the event */
    if(deleteEventNode) {
        retval = UA_Server_deleteNode(server, eventNodeId, true);
        if (retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Attempt to remove event using deleteNode failed. StatusCode %s",
                           UA_StatusCode_name(retval));
            return retval;
        }
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
