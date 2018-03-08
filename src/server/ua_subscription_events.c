/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

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
        UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_USERLAND,
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

UA_StatusCode UA_EXPORT
UA_Server_createEvent(UA_Server *server, const UA_NodeId eventType, UA_NodeId *outNodeId) {
    if (!outNodeId) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_USERLAND, "outNodeId cannot be NULL!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* make sure the eventType is a subtype of BaseEventType */
    UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    if (!isNodeInTree(&server->config.nodestore, &eventType, &baseEventTypeId, &hasSubtypeId, 1)) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_USERLAND, "Event type must be a subtype of BaseEventType!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName.locale = UA_STRING_NULL;
    oAttr.displayName.text = UA_STRING_NULL;
    oAttr.description.locale = UA_STRING_NULL;
    oAttr.description.text = UA_STRING_NULL;

    UA_QualifiedName name;
    UA_QualifiedName_init(&name);

    /* create an ObjectNode which represents the event */
    UA_StatusCode retval =
        UA_Server_addObjectNode(server,
                                UA_NODEID_NULL, /* the user may not have control over the nodeId */
                                UA_NODEID_NULL, /* an event does not have a parent */
                                UA_NODEID_NULL, /* an event does not have any references */
                                name,           /* an event does not have a name */
                                eventType,      /* the type of the event */
                                oAttr,          /* default attributes are fine */
                                NULL,           /* no node context */
                                outNodeId);

    if (retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Adding event failed. StatusCode %s", UA_StatusCode_name(retval));
        return retval;
    }

    /* find the eventType variableNode */
    name = UA_QUALIFIEDNAME(0, "EventType");
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, *outNodeId, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_deleteMembers(&bpr);
        return bpr.statusCode;
    }
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalarCopy(&value, &eventType, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_Variant_deleteMembers(&value);
    UA_BrowsePathResult_deleteMembers(&bpr);

    /* the object is not put in any queues until it is triggered */
    return retval;
}

static UA_Boolean
isValidEvent(UA_Server *server, const UA_NodeId *validEventParent, const UA_NodeId *eventId) {
    /* find the eventType variableNode */
    UA_QualifiedName findName = UA_QUALIFIEDNAME(0, "EventType");
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, *eventId, 1, &findName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_deleteMembers(&bpr);
        return UA_FALSE;
    }
    UA_NodeId hasSubtypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE);
    UA_Boolean tmp = isNodeInTree(&server->config.nodestore, &bpr.targets[0].targetId.nodeId,
                                  validEventParent, &hasSubtypeId, 1);
    UA_BrowsePathResult_deleteMembers(&bpr);
    return tmp;
}

/* static UA_StatusCode */
/* whereClausesApply(UA_Server *server, const UA_ContentFilter whereClause, */
/*                   UA_EventFieldList *efl, UA_Boolean *result) { */
/*     /\* if the where clauses aren't specified leave everything as is *\/ */
/*     if(whereClause.elementsSize == 0) { */
/*         *result = UA_TRUE; */
/*         return UA_STATUSCODE_GOOD; */
/*     } */

/*     /\* where clauses were specified *\/ */
/*     UA_LOG_WARNING(server->config.logger, UA_LOGCATEGORY_USERLAND, */
/*                    "Where clauses are not supported by the server."); */
/*     *result = UA_TRUE; */
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
           !isValidEvent(server, &filter->selectClauses[0].typeDefinitionId, eventNode)) {
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
    /* UA_Boolean whereClauseResult = UA_TRUE; */
    /* return whereClausesApply(server, filter->whereClause, &notification->fields, &whereClauseResult); */
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
eventSetConstants(UA_Server *server, const UA_NodeId *event,
                  const UA_NodeId *origin, UA_ByteString *outEventId) {
    /* set the source */
    UA_QualifiedName name = UA_QUALIFIEDNAME(0, "SourceNode");
    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, *event, 1, &name);
    if (bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_StatusCode tmp = bpr.statusCode;
        UA_BrowsePathResult_deleteMembers(&bpr);
        return tmp;
    }
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalarCopy(&value, origin, &UA_TYPES[UA_TYPES_NODEID]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_Variant_deleteMembers(&value);
    UA_BrowsePathResult_deleteMembers(&bpr);

    /* set the receive time */
    name = UA_QUALIFIEDNAME(0, "ReceiveTime");
    bpr = UA_Server_browseSimplifiedBrowsePath(server, *event, 1, &name);
    if (bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_StatusCode tmp = bpr.statusCode;
        UA_BrowsePathResult_deleteMembers(&bpr);
        return tmp;
    }
    UA_DateTime time = UA_DateTime_now();
    UA_Variant_setScalar(&value, &time, &UA_TYPES[UA_TYPES_DATETIME]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_deleteMembers(&bpr);

    /* set the eventId attribute */
    UA_ByteString eventId;
    UA_ByteString_init(&eventId);
    UA_StatusCode retval = UA_Event_generateEventId(server, &eventId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    if (outEventId) {
        UA_ByteString_copy(&eventId, outEventId);
    }

    name = UA_QUALIFIEDNAME(0, "EventId");
    bpr = UA_Server_browseSimplifiedBrowsePath(server, *event, 1, &name);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_StatusCode tmp = bpr.statusCode;
        UA_ByteString_deleteMembers(&eventId);
        UA_BrowsePathResult_deleteMembers(&bpr);
        return tmp;
    }
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, value);
    UA_ByteString_deleteMembers(&eventId);
    UA_BrowsePathResult_deleteMembers(&bpr);

    return UA_STATUSCODE_GOOD;
}


/* insert each node into the list (passed as handle) */
static UA_StatusCode
getParentsNodeIteratorCallback(UA_NodeId parentId, UA_Boolean isInverse,
                               UA_NodeId referenceTypeId, void *handle) {
    /* parents have an inverse reference */
    if(!isInverse)
        return UA_STATUSCODE_GOOD;

    Events_nodeListElement *entry = (Events_nodeListElement *) UA_malloc(sizeof(Events_nodeListElement));
    if (!entry) {
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    UA_StatusCode retval = UA_NodeId_copy(&parentId, &entry->nodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(entry);
        return retval;
    }

    LIST_INSERT_HEAD(&((struct getNodesHandle *) handle)->nodes, entry, listEntry);

    /* recursion */
    UA_Server_forEachChildNodeCall(((struct getNodesHandle *) handle)->server,
                                   parentId, getParentsNodeIteratorCallback, handle);
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

/* make sure the origin is in the ObjectsFolder (TODO: or in the ViewsFolder) */
static const UA_NodeId objectsFolderId = {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_OBJECTSFOLDER}};
static const UA_NodeId references[2] =
    {{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ORGANIZES}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}}};

UA_StatusCode UA_EXPORT
UA_Server_triggerEvent(UA_Server *server, const UA_NodeId eventNodeId, const UA_NodeId origin,
                       UA_ByteString *outEventId, const UA_Boolean deleteEventNode) {
    if(!isNodeInTree(&server->config.nodestore, &origin, &objectsFolderId, references, 2)) {
        UA_LOG_ERROR(server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Node for event must be in ObjectsFolder!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval = eventSetConstants(server, &eventNodeId, &origin, outEventId);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* get an array with all parents */
    struct getNodesHandle parentHandle;
    parentHandle.server = server;
    LIST_INIT(&parentHandle.nodes);
    retval = getParentsNodeIteratorCallback(origin, UA_TRUE, UA_NODEID_NULL, &parentHandle);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* add the event to each node's monitored items */
    Events_nodeListElement *parentIter, *tmp_parentIter;
    LIST_FOREACH_SAFE(parentIter, &parentHandle.nodes, listEntry, tmp_parentIter) {
        const UA_ObjectNode *node = (const UA_ObjectNode *) UA_Nodestore_get(server, &parentIter->nodeId);
        /* SLIST_FOREACH */
        for (UA_MonitoredItem *monIter = node->monitoredItemQueue; monIter != NULL; monIter = monIter->next) {
            retval = UA_Event_addEventToMonitoredItem(server, &eventNodeId, monIter);
            if (retval != UA_STATUSCODE_GOOD) {
                UA_Nodestore_release(server, (const UA_Node *) node);
                return retval;
            }
        }
        UA_Nodestore_release(server, (const UA_Node *) node);

        LIST_REMOVE(parentIter, listEntry);
        UA_NodeId_deleteMembers(&parentIter->nodeId);
        UA_free(parentIter);
    }

    /* delete the node representation of the event */
    if (deleteEventNode) {
        retval = UA_Server_deleteNode(server, eventNodeId, UA_TRUE);
        if (retval != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logger,
                           UA_LOGCATEGORY_SERVER,
                           "Attempt to remove event using deleteNode failed. StatusCode %s",
                           UA_StatusCode_name(retval));
            return retval;
        }
    }
    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
