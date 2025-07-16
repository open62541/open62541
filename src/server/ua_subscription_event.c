/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2021 (c) Fraunhofer IOSB (Author: Andreas Ebner)
 *    Copyright 2025 (c) Fraunhofer IOSB (Author: Julius Pfrommer)
 */

#include "ua_server_internal.h"
#include "ua_subscription.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS

#ifdef UA_ENABLE_HISTORIZING
static void
setHistoricalEvent(UA_Server *server, const UA_NodeId *emitNode,
                   const UA_EventDescription *ed) {
    UA_Variant historicalEventFilterValue;
    UA_Variant_init(&historicalEventFilterValue);

    /* A HistoricalEventNode that has event history available will provide this property */
    UA_StatusCode retval =
        readObjectProperty(server, *emitNode,
                           UA_QUALIFIEDNAME(0, "HistoricalEventFilter"),
                           &historicalEventFilterValue);
    if(retval != UA_STATUSCODE_GOOD) {
        if(retval != UA_STATUSCODE_BADNOMATCH)
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Cannot read the HistoricalEventFilter property of a "
                           "listening node. StatusCode %s",
                           UA_StatusCode_name(retval));
        return;
    }

    /* If found then check if HistoricalEventFilter property has a valid value */
    if(!UA_Variant_hasScalarType(&historicalEventFilterValue,
                                 &UA_TYPES[UA_TYPES_EVENTFILTER])) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "HistoricalEventFilter property of a listening node "
                       "does not have a valid value");
        UA_Variant_clear(&historicalEventFilterValue);
        return;
    }
    UA_EventFilter *ef = (UA_EventFilter*)historicalEventFilterValue.data;

    /* Evaluate the where clause */
    UA_StatusCode res =
        evaluateWhereClause(server, &server->adminSession, &ef->whereClause, ed);
    if(res != UA_STATUSCODE_GOOD) {
        UA_Variant_clear(&historicalEventFilterValue);
        return;
    }

    /* Get the event fields for the select clause */
    UA_EventFieldList efl;
    UA_EventFieldList_init(&efl);
    res = setEventFields(server, &server->adminSession, ed, ef, &efl);

    if(UA_LIKELY(res == UA_STATUSCODE_GOOD)) {
        server->config.historyDatabase.
            setEvent(server, server->config.historyDatabase.context,
                     &ed->sourceNode, emitNode, ef, &efl);
    }

    UA_Variant_clear(&historicalEventFilterValue);
    UA_EventFieldList_clear(&efl);
}
#endif

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

/* Names of the mandatory event properties (in the BaseEventType) */
#define MANDATORY_EVENT_PROPERTIES_COUNT 8
static UA_String mandatoryEventProperties[MANDATORY_EVENT_PROPERTIES_COUNT] = {
    UA_STRING_STATIC("/EventId"),
    UA_STRING_STATIC("/EventType"),
    UA_STRING_STATIC("/SourceNode"),
    UA_STRING_STATIC("/SourceName"),
    UA_STRING_STATIC("/Time"),
    UA_STRING_STATIC("/ReceiveTime"),
    UA_STRING_STATIC("/Message"),
    UA_STRING_STATIC("/Severity")
};

UA_StatusCode
resolveSimpleAttributeOperand(UA_Server *server, UA_Session *session,
                              const UA_EventDescription *ed,
                              const UA_SimpleAttributeOperand *sao,
                              UA_Variant *out) {
    /* Print the SAO as a human-readable string. We are using the
     * TypeDefinitionId initially to disambiguate properties with the same
     * BrowseName but from different EventTypes. The we try again wtih the
     * TypeDefinitionId disabled. */
    UA_Byte pathBuf[512];
    UA_QualifiedName pathString = {0, {512, pathBuf}};
    UA_SimpleAttributeOperand tmp_sao = *sao;
    UA_String_init(&tmp_sao.indexRange);
    UA_StatusCode res = UA_STATUSCODE_GOOD;

 search_again:
    res = UA_SimpleAttributeOperand_print(&tmp_sao, &pathString.name);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    /* The value is explicitly defined */
    const UA_Variant *found = UA_KeyValueMap_get(&ed->otherEventFields, pathString);
    if(found) {
        if(sao->indexRange.length == 0) {
            *out = *found;
            out->storageType = UA_VARIANT_DATA_NODELETE;
        } else {
            UA_NumericRange range;
            res = UA_NumericRange_parse(&range, sao->indexRange);
            if(res != UA_STATUSCODE_GOOD)
               return res;
            res = UA_Variant_copyRange(found, out, range);
            UA_free(range.dimensions);
        }
        return res;
    }

    /* Use a default for the mandatory fields of the BaseEventType.
     * Here we ignore the IndexRange. */
    if(UA_String_equal(&pathString.name, &mandatoryEventProperties[0])) {
        /* EventId */
        UA_ByteString eventid = UA_BYTESTRING_NULL;
        res |= generateEventId(&eventid);
        res |= UA_Variant_setScalarCopy(out, &eventid, &UA_TYPES[UA_TYPES_BYTESTRING]);
        UA_ByteString_clear(&eventid);
        return res;
    } else if(UA_String_equal(&pathString.name, &mandatoryEventProperties[1])) {
        /* EventType */
        return UA_Variant_setScalarCopy(out, &ed->eventType, &UA_TYPES[UA_TYPES_NODEID]);
    } else if(UA_String_equal(&pathString.name, &mandatoryEventProperties[2])) {
        /* SourceNode */
        return UA_Variant_setScalarCopy(out, &ed->sourceNode, &UA_TYPES[UA_TYPES_NODEID]);
    } else if(UA_String_equal(&pathString.name, &mandatoryEventProperties[3])) {
        /* SourceName. Read the DisplayName from the information model. This
         * uses the locale of the session. */
        UA_ReadValueId rvi;
        UA_ReadValueId_init(&rvi);
        rvi.nodeId = ed->sourceNode;
        rvi.attributeId = UA_ATTRIBUTEID_DISPLAYNAME;
        UA_DataValue dv = readWithSession(server, session, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
        if(dv.status != UA_STATUSCODE_GOOD) {
            res = dv.status;
            UA_DataValue_clear(&dv);
            return res;
        }
        if(dv.value.type != &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]) {
            UA_DataValue_clear(&dv);
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        UA_LocalizedText *displayName = (UA_LocalizedText*)dv.value.data;
        res = UA_Variant_setScalarCopy(out, &displayName->text, &UA_TYPES[UA_TYPES_STRING]);
        UA_DataValue_clear(&dv);
        return res;
    } else if(UA_String_equal(&pathString.name, &mandatoryEventProperties[4]) ||
              UA_String_equal(&pathString.name, &mandatoryEventProperties[5])) {
        /* Time / ReceiveTime */
        UA_EventLoop *el = server->config.eventLoop;
        UA_DateTime rcvTime = el->dateTime_now(el);
        return UA_Variant_setScalarCopy(out, &rcvTime, &UA_TYPES[UA_TYPES_DATETIME]);
    } else if(UA_String_equal(&pathString.name, &mandatoryEventProperties[6])) {
        /* Message */
        UA_LocalizedText message;
        UA_LocalizedText_init(&message);
        return UA_Variant_setScalarCopy(out, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    } else if(UA_String_equal(&pathString.name, &mandatoryEventProperties[7])) {
        /* Severity */
        return UA_Variant_setScalarCopy(out, &ed->severity, &UA_TYPES[UA_TYPES_UINT16]);
    }

    /* Try again with the TypeDefinitionId */
    if(!UA_NodeId_isNull(&tmp_sao.typeDefinitionId)) {
        UA_NodeId_init(&tmp_sao.typeDefinitionId);
        goto search_again;
    }

    /* Not found, return an empty Variant */
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
setEventFields(UA_Server *server, UA_Session *session,
               const UA_EventDescription *ed, UA_EventFilter *filter,
               UA_EventFieldList *efl) {
    /* Nothing to do */
    if(filter->selectClausesSize == 0)
        return UA_STATUSCODE_GOOD;

    /* Allocate the event fields list */
    efl->eventFields = (UA_Variant*)
        UA_calloc(filter->selectClausesSize, sizeof(UA_Variant));
    if(!efl->eventFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    efl->eventFieldsSize = filter->selectClausesSize;

    /* Resolve the select clauses */
    for(size_t i = 0; i < filter->selectClausesSize; i++) {
        const UA_SimpleAttributeOperand *sao = &filter->selectClauses[i];
        UA_Variant *field = &efl->eventFields[i];
        resolveSimpleAttributeOperand(server, session, ed, sao, field);

        /* Ensure a deep copy */
        if(field->storageType == UA_VARIANT_DATA_NODELETE) {
            UA_Variant tmp_val;
            UA_StatusCode res = UA_Variant_copy(field, &tmp_val);
            (void)res; /* Ignore the result - returns an empty variant if copying fails */
            *field = tmp_val;
        }
    }

    return UA_STATUSCODE_GOOD;
}

#define EMIT_REFS_ROOT_COUNT 4
static const UA_NodeId emitReferencesRoots[EMIT_REFS_ROOT_COUNT] =
    {{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ORGANIZES}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASEVENTSOURCE}},
     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASNOTIFIER}}};

UA_StatusCode
createEvent(UA_Server *server, const UA_NodeId eventType,
            const UA_NodeId sourceNode, UA_UInt16 severity,
            UA_KeyValueMap otherEventFields) {
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_LOCK_ASSERT(&server->serviceMutex);

    UA_LOG_DEBUG(server->config.logging, UA_LOGCATEGORY_SERVER,
                 "Events: An event of type %N and severity %su is created on node %N",
                 eventType, severity, sourceNode);

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    UA_Boolean isCallerAC = false;
    if(isConditionOrBranch(server, &eventType, &sourceNode, &isCallerAC) &&
       !isCallerAC) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Condition Events: Please use A&C API to trigger Condition Events 0x%08X",
                       UA_STATUSCODE_BADINVALIDARGUMENT);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */

    /* The user must ensure that only nodes from the Objects folder (and Views)
     * emit events. The following commented code could enforce this, but is
     * computationally expensive. */

    /* static const UA_NodeId objectsFolderId = */
    /*     {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_OBJECTSFOLDER}}; */
    /* static const UA_NodeId isInFolderReferences[2] = */
    /*     {{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ORGANIZES}}, */
    /*      {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_HASCOMPONENT}}}; */
    /* UA_ReferenceTypeSet refTypes; */
    /* UA_ReferenceTypeSet_init(&refTypes); */
    /* for(int i = 0; i < 2; ++i) { */
    /*     UA_ReferenceTypeSet tmpRefTypes; */
    /*     res = referenceTypeIndices(server, &isInFolderReferences[i], &tmpRefTypes, true); */
    /*     if(res != UA_STATUSCODE_GOOD) { */
    /*         UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER, */
    /*                        "Events: Could not create the list of references and their subtypes " */
    /*                        "with StatusCode %s", UA_StatusCode_name(res)); */
    /*     } */
    /*     refTypes = UA_ReferenceTypeSet_union(refTypes, tmpRefTypes); */
    /* } */
    /* if(!isNodeInTree(server, &origin, &objectsFolderId, &refTypes)) { */
    /*     UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND, */
    /*                  "Node for event must be in ObjectsFolder!"); */
    /*     return UA_STATUSCODE_BADINVALIDARGUMENT; */
    /* } */

    /* Prepare the data structure passed for each node */
    UA_EventDescription ed;
    ed.eventType = eventType;
    ed.sourceNode = sourceNode;
    ed.severity = severity;
    ed.otherEventFields = otherEventFields;

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
    emitStartNodes[0] = sourceNode;
    emitStartNodes[1] = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);

    /* Get all ReferenceTypes over which the events propagate */
    UA_ReferenceTypeSet emitRefTypes;
    UA_ReferenceTypeSet_init(&emitRefTypes);
    for(size_t i = 0; i < EMIT_REFS_ROOT_COUNT; i++) {
        UA_ReferenceTypeSet tmpRefTypes;
        res = referenceTypeIndices(server, &emitReferencesRoots[i], &tmpRefTypes, true);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                           "Events: Could not create the list of references for event "
                           "propagation with StatusCode %s", UA_StatusCode_name(res));
            goto cleanup;
        }
        emitRefTypes = UA_ReferenceTypeSet_union(emitRefTypes, tmpRefTypes);
    }

    /* Get the list of nodes in the hierarchy that emits the event. */
    res = browseRecursive(server, 2, emitStartNodes, UA_BROWSEDIRECTION_INVERSE,
                          &emitRefTypes, UA_NODECLASS_UNSPECIFIED, true,
                          &emitNodesSize, &emitNodes);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                       "Events: Could not create the list of nodes listening on the "
                       "event with StatusCode %s", UA_StatusCode_name(res));
        goto cleanup;
    }

    /* Loop over all nodes that emit this event instance */
    for(size_t i = 0; i < emitNodesSize; i++) {
        /* We can only emit on local nodes */
        if(!UA_ExpandedNodeId_isLocal(&emitNodes[i]))
            continue;

        /* Get the node */
        const UA_Node *node = UA_NODESTORE_GET(server, &emitNodes[i].nodeId);
        if(!node)
            continue;

        /* Only consider objects */
        if(node->head.nodeClass != UA_NODECLASS_OBJECT) {
            UA_NODESTORE_RELEASE(server, node);
            continue;
        }

        /* Iterate over all MonitoredItems registered in the node  */
        for(UA_MonitoredItem *mon = node->head.monitoredItems;
            mon != NULL; mon = mon->sampling.nodeListNext) {
            /* Is this an Event-MonitoredItem? */
            if(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER)
                continue;

            /* Select the session. If the subscription is not bound to a
             * session, use the AdminSession. This has no security implications,
             * as only values from the EventDescription and the DisplayName of
             * the source node are used. */
            UA_Session *session = (mon->subscription->session) ?
                mon->subscription->session : &server->adminSession;

            /* Evaluate the where-clause and create a notification */
            res = UA_MonitoredItem_addEvent(server, session, mon, &ed);
            if(res != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_SERVER,
                               "Events: Could not add the event to a listening "
                               "node with StatusCode %s", UA_StatusCode_name(res));
                res = UA_STATUSCODE_GOOD; /* Only log problems with individual emit nodes */
            }
        }

        UA_NODESTORE_RELEASE(server, node);

        /* Add event entry in the historical database */
#ifdef UA_ENABLE_HISTORIZING
        if(server->config.historyDatabase.setEvent)
            setHistoricalEvent(server, &emitNodes[i].nodeId, &ed);
#endif
    }

 cleanup:
    UA_Array_delete(emitNodes, emitNodesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return res;
}

UA_StatusCode
UA_Server_createEvent(UA_Server *server, const UA_NodeId eventType,
                      const UA_NodeId sourceNode, UA_UInt16 severity,
                      UA_KeyValueMap otherEventFields) {
    lockServer(server);
    UA_StatusCode res =
        createEvent(server, eventType, sourceNode, severity, otherEventFields);
    unlockServer(server);
    return res;
}

/* Filters an event according to the filter specified by mon and then adds it to
 * mons notification queue */
UA_StatusCode
UA_MonitoredItem_addEvent(UA_Server *server, UA_Session *session,
                          UA_MonitoredItem *mon, const UA_EventDescription *ed) {
    /* Get the filter */
    if(mon->parameters.filter.content.decoded.type != &UA_TYPES[UA_TYPES_EVENTFILTER])
        return UA_STATUSCODE_BADFILTERNOTALLOWED;
    UA_EventFilter *ef = (UA_EventFilter*)
        mon->parameters.filter.content.decoded.data;

    /* A MonitoredItem is always attached to a (local) Subscription.
     * A Subscription can be not attached to any Session. */
    UA_Subscription *sub = mon->subscription;
    UA_assert(sub);

    /* Evaluate the where clause */
    UA_StatusCode res =
        evaluateWhereClause(server, session, &ef->whereClause, ed);
    if(res != UA_STATUSCODE_GOOD) {
        if(res == UA_STATUSCODE_BADNOMATCH)
            res = UA_STATUSCODE_GOOD;
        return res;
    }

    /* Get the event fields for the select clause */
    UA_EventFieldList efl;
    UA_EventFieldList_init(&efl);
    res = setEventFields(server, session, ed, ef, &efl);
    if(res != UA_STATUSCODE_GOOD) {
        UA_EventFieldList_clear(&efl);
        return res;
    }

    /* Allocate memory for the notification */
    UA_Notification *n = UA_Notification_new();
    if(!n) {
        UA_EventFieldList_clear(&efl);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    /* Prepare and enqueue the notification */
    n->data.event = efl;
    n->data.event.clientHandle = mon->parameters.clientHandle;
    n->mon = mon;
    UA_Notification_enqueueAndTrigger(server, n);

    return UA_STATUSCODE_GOOD;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_EVENTS */
