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

//forward declaration
static UA_StatusCode
evaluateWhereClauseContentFilter(UA_Server *server, UA_Session *session,
                                 const UA_NodeId *eventNode,
                                 const UA_ContentFilter *contentFilter,
                                 UA_ContentFilterResult *contentFilterResult,
                                 UA_Variant* valueResult, UA_UInt16 index);

/* We use a 16-Byte ByteString as an identifier */
UA_StatusCode
UA_Event_generateEventId(UA_ByteString *generatedId) {
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
UA_Server_createEvent(UA_Server *server, const UA_NodeId eventType,
                      UA_NodeId *outNodeId) {
    UA_LOCK(&server->serviceMutex);
    if(!outNodeId) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "outNodeId must not be NULL. The event's NodeId must be returned "
                     "so it can be triggered.");
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Make sure the eventType is a subtype of BaseEventType */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    if(!isNodeInTree_singleRef(server, &eventType, &baseEventTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Event type must be a subtype of BaseEventType!");
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Create an ObjectNode which represents the event */
    UA_QualifiedName name;
    // set a dummy name. This is not used.
    name = UA_QUALIFIEDNAME(0,"E");
    UA_NodeId newNodeId = UA_NODEID_NULL;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_StatusCode retval = addNode(server, UA_NODECLASS_OBJECT,
                                   &UA_NODEID_NULL, /* Set a random unused NodeId */
                                   &UA_NODEID_NULL, /* No parent */
                                   &UA_NODEID_NULL, /* No parent reference */
                                   name,            /* an event does not have a name */
                                   &eventType,      /* the type of the event */
                                   (const UA_NodeAttributes*)&oAttr, /* default attributes are fine */
                                   &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                                   NULL,           /* no node context */
                                   &newNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
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
        UA_UNLOCK(&server->serviceMutex);
        return retval;
    }

    /* Set the EventType */
    UA_Variant value;
    UA_Variant_init(&value);
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&eventType, &UA_TYPES[UA_TYPES_NODEID]);
    retval = writeValueAttribute(server, &server->adminSession,
                                 &bpr.targets[0].targetId.nodeId, &value);
    UA_BrowsePathResult_clear(&bpr);
    if(retval != UA_STATUSCODE_GOOD) {
        deleteNode(server, newNodeId, true);
        UA_NodeId_clear(&newNodeId);
        UA_UNLOCK(&server->serviceMutex);
        return retval;
    }

    *outNodeId = newNodeId;
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_Boolean
isValidEvent(UA_Server *server, const UA_NodeId *validEventParent,
             const UA_NodeId *eventId) {
    /* find the eventType variableNode */
    UA_QualifiedName findName = UA_QUALIFIEDNAME(0, "EventType");
    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, *eventId, 1, &findName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize < 1) {
        UA_BrowsePathResult_clear(&bpr);
        return false;
    }

    /* Get the EventType Property Node */
    UA_Variant tOutVariant;
    UA_Variant_init(&tOutVariant);

    /* Read the Value of EventType Property Node (the Value should be a NodeId) */
    UA_StatusCode retval = readWithReadValue(server, &bpr.targets[0].targetId.nodeId,
                                             UA_ATTRIBUTEID_VALUE, &tOutVariant);
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_NODEID])) {
        UA_BrowsePathResult_clear(&bpr);
        return false;
    }

    const UA_NodeId *tEventType = (UA_NodeId*)tOutVariant.data;

    /* check whether the EventType is a Subtype of CondtionType
     * (Part 9 first implementation) */
    UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(validEventParent, &conditionTypeId) &&
       isNodeInTree_singleRef(server, tEventType, &conditionTypeId,
                              UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_BrowsePathResult_clear(&bpr);
        UA_Variant_clear(&tOutVariant);
        return true;
    }

    /*EventType is not a Subtype of CondtionType
     *(ConditionId Clause won't be present in Events, which are not Conditions)*/
    /* check whether Valid Event other than Conditions */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    UA_Boolean isSubtypeOfBaseEvent =
        isNodeInTree_singleRef(server, tEventType, &baseEventTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE);

    UA_BrowsePathResult_clear(&bpr);
    UA_Variant_clear(&tOutVariant);
    return isSubtypeOfBaseEvent;
}

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

    UA_DataValue v;

    if(sao->browsePathSize == 0) {
        /* If this list (browsePath) is empty, the Node is the instance of the
         * TypeDefinition. */
        rvi.nodeId = sao->typeDefinitionId;

        /* A Condition is an indirection. Look up the target node. */
        /* TODO: check for Branches! One Condition could have multiple Branches */
        UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
        if(UA_NodeId_equal(&sao->typeDefinitionId, &conditionTypeId)) {
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
            UA_StatusCode res = UA_getConditionId(server, origin, &rvi.nodeId);
            if(res != UA_STATUSCODE_GOOD)
                return res;
#else
            return UA_STATUSCODE_BADNOTSUPPORTED;
#endif
        }

        v = UA_Server_readWithSession(server, session, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);

    } else {
        /* Resolve the browse path, starting from the event-source (and not the
         * typeDefinitionId). */
        UA_BrowsePathResult bpr =
            browseSimplifiedBrowsePath(server, *origin, sao->browsePathSize, sao->browsePath);
        if(bpr.targetsSize == 0 && bpr.statusCode == UA_STATUSCODE_GOOD)
            bpr.statusCode = UA_STATUSCODE_BADNOTFOUND;
        if(bpr.statusCode != UA_STATUSCODE_GOOD) {
            UA_StatusCode res = bpr.statusCode;
            UA_BrowsePathResult_clear(&bpr);
            return res;
        }

        /* Use the first match */
        rvi.nodeId = bpr.targets[0].targetId.nodeId;
        v = UA_Server_readWithSession(server, session, &rvi, UA_TIMESTAMPSTORETURN_NEITHER);
        UA_BrowsePathResult_clear(&bpr);
    }

    /* Move the result to the output */
    if(v.status == UA_STATUSCODE_GOOD && v.hasValue)
        *value = v.value;
    else
        UA_Variant_clear(&v.value);
    return v.status;
}

/* Resolve operands to variants according to the operand type.
 * Part 4: 7.17.3 Table 142 specifies the allowed types. */
static UA_Variant
resolveOperand(UA_Server *server, UA_Session *session, const UA_NodeId *origin,
               const UA_ContentFilter *contentFilter,
               UA_ContentFilterResult *contentFilterResult, UA_Variant *valueResult,
               UA_UInt16 index, UA_UInt16 nr) {

    UA_StatusCode res;
    UA_Variant variant;
    /*SimpleAttributeOperands*/
    if(contentFilter->elements[index].filterOperands[nr].content.decoded.type ==
       &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]) {
        res = resolveSimpleAttributeOperand(
            server, session, origin,
            (UA_SimpleAttributeOperand *)contentFilter->elements[index]
                .filterOperands[nr]
                .content.decoded.data,
            &variant);
        /*LiteralAttribute*/
    } else if(contentFilter->elements[index].filterOperands[nr].content.decoded.type ==
              &UA_TYPES[UA_TYPES_LITERALOPERAND]) {
        variant = ((UA_LiteralOperand *)contentFilter->elements[index]
            .filterOperands[nr]
            .content.decoded.data)
            ->value;
        res = UA_STATUSCODE_GOOD;
    } else if(contentFilter->elements[index].filterOperands[nr].content.decoded.type ==
              &UA_TYPES[UA_TYPES_ELEMENTOPERAND]) {
        res = evaluateWhereClauseContentFilter(
            server, session, origin, contentFilter, contentFilterResult, valueResult,
            (UA_UInt16)((UA_ElementOperand *)contentFilter->elements[index]
                .filterOperands[nr]
                .content.decoded.data)
                ->index);
        variant =
            valueResult[(UA_UInt16)((UA_ElementOperand *)contentFilter->elements[index]
                .filterOperands[nr]
                .content.decoded.data)
                ->index];
        /*ElementOperands*/
    } else {
        res = UA_STATUSCODE_BADFILTEROPERANDINVALID;
    }
    if(res != UA_STATUSCODE_GOOD && res != UA_STATUSCODE_BADNOMATCH) {
        variant.type = NULL;
        contentFilterResult->elementResults[index].operandStatusCodes[nr] = res;
    }
    return variant;
}

static UA_StatusCode
evaluateWhereClauseContentFilter(UA_Server *server, UA_Session *session,
                                 const UA_NodeId *eventNode,
                                 const UA_ContentFilter *contentFilter,
                                 UA_ContentFilterResult *contentFilterResult,
                                 UA_Variant* valueResult, UA_UInt16 index) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(contentFilter->elements == NULL || contentFilter->elementsSize == 0) {
        /* Nothing to do.*/
        return UA_STATUSCODE_GOOD;
    }

    /* The first element needs to be evaluated, this might be linked to other
     * elements, which are evaluated in these cases. See 7.4.1 in Part 4. */
    UA_ContentFilterElement *pElement = &contentFilter->elements[0];
    switch(pElement->filterOperator) {
        case UA_FILTEROPERATOR_INVIEW:
        case UA_FILTEROPERATOR_RELATEDTO: {
            /* Not allowed for event WhereClause according to 7.17.3 in Part 4 */
            return UA_STATUSCODE_BADEVENTFILTERINVALID;
        }
        case UA_FILTEROPERATOR_EQUALS:
        case UA_FILTEROPERATOR_ISNULL: {
            /* Checking if operand is NULL. This is done by reducing the operand to a
            * variant and then checking if it is empty. */
            UA_Variant operand =
                resolveOperand(server, session, eventNode, contentFilter,
                               contentFilterResult, valueResult, index, 0);
            valueResult[index].type = &UA_TYPES[UA_TYPES_BOOLEAN];
            if(UA_Variant_isEmpty(&operand)) {
                contentFilterResult->elementResults[index].statusCode =
                    UA_STATUSCODE_GOOD;
                break;
            }
            contentFilterResult->elementResults[index].statusCode =
                UA_STATUSCODE_BADNOMATCH;
            break;
        }
        case UA_FILTEROPERATOR_GREATERTHAN:
        case UA_FILTEROPERATOR_LESSTHAN:
        case UA_FILTEROPERATOR_GREATERTHANOREQUAL:
        case UA_FILTEROPERATOR_LESSTHANOREQUAL:
        case UA_FILTEROPERATOR_LIKE:
        case UA_FILTEROPERATOR_NOT:
        case UA_FILTEROPERATOR_BETWEEN:
        case UA_FILTEROPERATOR_INLIST:
        case UA_FILTEROPERATOR_AND:
        case UA_FILTEROPERATOR_OR:
        case UA_FILTEROPERATOR_CAST:
        case UA_FILTEROPERATOR_BITWISEAND:
        case UA_FILTEROPERATOR_BITWISEOR:
            return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;

        case UA_FILTEROPERATOR_OFTYPE: {
            UA_Boolean result = UA_FALSE;
            if(pElement->filterOperandsSize != 1)
                return UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
            if(pElement->filterOperands[0].content.decoded.type !=
                &UA_TYPES[UA_TYPES_LITERALOPERAND])
                return UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;

            UA_LiteralOperand *pOperand =
                (UA_LiteralOperand *) pElement->filterOperands[0].content.decoded.data;
            if(!UA_Variant_isScalar(&pOperand->value))
                return UA_STATUSCODE_BADEVENTFILTERINVALID;

            if(pOperand->value.type != &UA_TYPES[UA_TYPES_NODEID] ||
               pOperand->value.data == NULL) {
                result = UA_FALSE;
            } else {
                UA_NodeId *pOperandNodeId = (UA_NodeId *) pOperand->value.data;
                UA_QualifiedName eventTypeQualifiedName = UA_QUALIFIEDNAME(0, "EventType");
                UA_Variant typeNodeIdVariant;
                UA_Variant_init(&typeNodeIdVariant);
                UA_StatusCode readStatusCode =
                    readObjectProperty(server, *eventNode, eventTypeQualifiedName,
                                       &typeNodeIdVariant);
                if(readStatusCode != UA_STATUSCODE_GOOD)
                    return readStatusCode;

                if(!UA_Variant_isScalar(&typeNodeIdVariant) ||
                   typeNodeIdVariant.type != &UA_TYPES[UA_TYPES_NODEID] ||
                   typeNodeIdVariant.data == NULL) {
                    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                 "EventType has an invalid type.");
                    UA_Variant_clear(&typeNodeIdVariant);
                    return UA_STATUSCODE_BADINTERNALERROR;
                }

                result = isNodeInTree_singleRef(server,
                                                (UA_NodeId*) typeNodeIdVariant.data,
                                                pOperandNodeId,
                                                UA_REFERENCETYPEINDEX_HASSUBTYPE);
                UA_Variant_clear(&typeNodeIdVariant);
            }

            if(result)
                return UA_STATUSCODE_GOOD;
            else
                return UA_STATUSCODE_BADNOMATCH;
        }
            break;
    default:
        return UA_STATUSCODE_BADFILTEROPERATORINVALID;
        break;
    }
    return contentFilterResult->elementResults[index].statusCode;
}

UA_StatusCode
UA_Server_evaluateWhereClauseContentFilter(UA_Server *server, UA_Session *session,
                                           const UA_NodeId *eventNode,
                                           const UA_ContentFilter *contentFilter,
                                           UA_ContentFilterResult *contentFilterResult) {
    if(contentFilter->elementsSize == 0)
        return UA_STATUSCODE_GOOD;
    //TODO add maximum lenth size to the server config
    if (contentFilter->elementsSize > 256)
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    UA_STACKARRAY(UA_Variant, valueResult, contentFilter->elementsSize);
    for(size_t i = 0; i < contentFilter->elementsSize; ++i) {
        UA_Variant_init(&valueResult[i]);
    }
    UA_StatusCode res = evaluateWhereClauseContentFilter(
        server, session, eventNode, contentFilter, contentFilterResult, valueResult, 0);
    for(size_t i = 0; i < contentFilter->elementsSize; i++) {
        if(!UA_Variant_isEmpty(&valueResult[i]))
            UA_Variant_clear(&valueResult[i]);
    }
    return res;
}

/* Filters the given event with the given filter and writes the results into a
 * notification */
static UA_StatusCode
UA_Server_filterEvent(UA_Server *server, UA_Session *session,
                      const UA_NodeId *eventNode, UA_EventFilter *filter,
                      UA_EventFieldList *efl, UA_EventFilterResult *result) {
    if(filter->selectClausesSize == 0)
        return UA_STATUSCODE_BADEVENTFILTERINVALID;

    UA_EventFieldList_init(efl);
    efl->eventFields = (UA_Variant *)
        UA_Array_new(filter->selectClausesSize, &UA_TYPES[UA_TYPES_VARIANT]);
    if(!efl->eventFields)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    efl->eventFieldsSize = filter->selectClausesSize;

    //empty event filter result
    UA_EventFilterResult_init(result);
    result->selectClauseResultsSize = filter->selectClausesSize;
    result->selectClauseResults = (UA_StatusCode *)
        UA_Array_new(filter->selectClausesSize, &UA_TYPES[UA_TYPES_STATUSCODE]);
    if(!result->selectClauseResults) {
        UA_EventFieldList_clear(efl);
        UA_EventFilterResult_clear(result);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    //prepare content filter result structure
    if(filter->whereClause.elementsSize != 0) {
        result->whereClauseResult.elementResultsSize = filter->whereClause.elementsSize;
        result->whereClauseResult.elementResults = (UA_ContentFilterElementResult *)
            UA_Array_new(filter->whereClause.elementsSize,
            &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENTRESULT]);
        if(!result->whereClauseResult.elementResults) {
            UA_EventFieldList_clear(efl);
            UA_EventFilterResult_clear(result);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        for(size_t i = 0; i < result->whereClauseResult.elementResultsSize; ++i) {
            result->whereClauseResult.elementResults[i].operandStatusCodesSize =
            filter->whereClause.elements->filterOperandsSize;
            result->whereClauseResult.elementResults[i].operandStatusCodes =
                (UA_StatusCode *)UA_Array_new(
                    filter->whereClause.elements->filterOperandsSize,
                    &UA_TYPES[UA_TYPES_STATUSCODE]);
            if(!result->whereClauseResult.elementResults[i].operandStatusCodes) {
                UA_EventFieldList_clear(efl);
                UA_EventFilterResult_clear(result);
                return UA_STATUSCODE_BADOUTOFMEMORY;
            }
        }
    }

    /* Apply the content (where) filter */
    UA_StatusCode res =
        UA_Server_evaluateWhereClauseContentFilter(server, session, eventNode,
                                                   &filter->whereClause, &result->whereClauseResult);
    if(res != UA_STATUSCODE_GOOD){
        UA_EventFieldList_clear(efl);
        UA_EventFilterResult_clear(result);
        return res;
    }

    /* Apply the select filter */
    /* Check if the browsePath is BaseEventType, in which case nothing more
     * needs to be checked */
    UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    for(size_t i = 0; i < filter->selectClausesSize; i++) {
        if(!UA_NodeId_equal(&filter->selectClauses[i].typeDefinitionId, &baseEventTypeId) &&
           !isValidEvent(server, &filter->selectClauses[i].typeDefinitionId, eventNode)) {
            UA_Variant_init(&efl->eventFields[i]);
            /* EventFilterResult currently isn't being used
            notification->result.selectClauseResults[i] = UA_STATUSCODE_BADTYPEDEFINITIONINVALID; */
            continue;
        }

        /* TODO: Put the result into the selectClausResults */
        resolveSimpleAttributeOperand(server, session, eventNode,
                                      &filter->selectClauses[i], &efl->eventFields[i]);
    }

    return UA_STATUSCODE_GOOD;
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
    retval = writeValueAttribute(server, &server->adminSession,
                                 &bpr.targets[0].targetId.nodeId, &value);
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
    UA_DateTime rcvTime = UA_DateTime_now();
    UA_Variant_setScalar(&value, &rcvTime, &UA_TYPES[UA_TYPES_DATETIME]);
    retval = writeValueAttribute(server, &server->adminSession,
                                 &bpr.targets[0].targetId.nodeId, &value);
    UA_BrowsePathResult_clear(&bpr);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    /* Set the EventId */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    retval = UA_Event_generateEventId(&eventId);
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
    retval = writeValueAttribute(server, &server->adminSession,
                                 &bpr.targets[0].targetId.nodeId, &value);
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
UA_Event_addEventToMonitoredItem(UA_Server *server, const UA_NodeId *event,
                                 UA_MonitoredItem *mon) {
    UA_Notification *notification = UA_Notification_new();
    if(!notification)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    if(mon->parameters.filter.content.decoded.type != &UA_TYPES[UA_TYPES_EVENTFILTER])
        return UA_STATUSCODE_BADFILTERNOTALLOWED;
    UA_EventFilter *eventFilter = (UA_EventFilter*)
        mon->parameters.filter.content.decoded.data;

    UA_Subscription *sub = mon->subscription;
    UA_Session *session = sub->session;
    UA_StatusCode retval = UA_Server_filterEvent(server, session, event,
                                                 eventFilter, &notification->data.event,
                                                 &notification->result);
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
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Cannot read the HistoricalEventFilter property of a "
                           "listening node. StatusCode %s",
                           UA_StatusCode_name(retval));
        return;
    }

    /* If found then check if HistoricalEventFilter property has a valid value */
    if(UA_Variant_isEmpty(&historicalEventFilterValue) ||
       !UA_Variant_isScalar(&historicalEventFilterValue) ||
       historicalEventFilterValue.type != &UA_TYPES[UA_TYPES_EVENTFILTER]) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                       "HistoricalEventFilter property of a listening node "
                       "does not have a valid value");
        UA_Variant_clear(&historicalEventFilterValue);
        return;
    }

    /* Finally, if found and valid then filter */
    UA_EventFilter *filter = (UA_EventFilter*) historicalEventFilterValue.data;
    UA_EventFieldList efl;
    UA_EventFilterResult result;
    retval = UA_Server_filterEvent(server, &server->adminSession,
                                   eventNodeId, filter, &efl, &result);
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
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_SERVER,
            "Events: An event is triggered on node %.*s",
            (int)nodeIdStr.length, nodeIdStr.data));

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    UA_Boolean isCallerAC = false;
    if(isConditionOrBranch(server, &eventNodeId, &origin, &isCallerAC)) {
        if(!isCallerAC) {
          UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                                 "Condition Events: Please use A&C API to trigger Condition Events 0x%08X",
                                  UA_STATUSCODE_BADINVALIDARGUMENT);
          return UA_STATUSCODE_BADINVALIDARGUMENT;
        }
    }
#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */

    /* Check that the origin node exists */
    const UA_Node *originNode = UA_NODESTORE_GET(server, &origin);
    if(!originNode) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
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
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Events: Could not create the list of references and their subtypes "
                           "with StatusCode %s", UA_StatusCode_name(retval));
        }
        refTypes = UA_ReferenceTypeSet_union(refTypes, tmpRefTypes);
    }

    if(!isNodeInTree(server, &origin, &objectsFolderId, &refTypes)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Node for event must be in ObjectsFolder!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Update the standard fields of the event */
    retval = eventSetStandardFields(server, &eventNodeId, &origin, outEventId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
        for(UA_MonitoredItem *mon = node->head.monitoredItems; mon != NULL; mon = mon->next) {
            /* Is this an Event-MonitoredItem? */
            if(mon->itemToMonitor.attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER)
                continue;
            retval = UA_Event_addEventToMonitoredItem(server, &eventNodeId, mon);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
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
            UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_SERVER,
                           "Attempt to remove event using deleteNode failed. StatusCode %s",
                           UA_StatusCode_name(retval));
        }
    }

 cleanup:
    UA_Array_delete(emitNodes, emitNodesSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return retval;
}

/*
 * Initial select clause validation. The following checks are currently performed:
 * - Check if typedefenitionid or browsepath of any clause is NULL
 * - Check if the eventType is a subtype of BaseEventType
 * - Check if attributeId is valid
 * - Check if browsePath contains null
 * - Check if indexRange is defined and if it is parsable
 * - Check if attributeId is value
 */
void
UA_Event_staticSelectClauseValidation(UA_Server *server,
                                      const UA_EventFilter *eventFilter,
                                      UA_StatusCode *result) {
    /* The selectClause only has to be checked, if the size is not zero */
    if(eventFilter->selectClausesSize == 0)
        return;
    for(size_t i = 0; i < eventFilter->selectClausesSize; ++i) {
        result[i] = UA_STATUSCODE_GOOD;
        ///typedefenitionid or browsepath of any clause is not NULL ?
        if(UA_NodeId_isNull(&eventFilter->selectClauses[i].typeDefinitionId)) {
            result[i] = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            continue;
        }
        /*ToDo: Check the following workaround. In UaExpert Event View the selection
        * of the Server Object set up 7 select filter entries by default. The last
        * element ist  from node 2782 (A&C ConditionType). Since the reduced
        * information model dos not contain this type, the result has a brows path of
        * "null" which results in an error. */
        UA_NodeId ac_conditionType = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
        if(UA_NodeId_equal(&eventFilter->selectClauses[i].typeDefinitionId, &ac_conditionType)) {
            continue;
        }
        if(&eventFilter->selectClauses[i].browsePath[0] == NULL) {
            result[i] = UA_STATUSCODE_BADBROWSENAMEINVALID;
            continue;
        }
        //eventType is a subtype of BaseEventType ?
        UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
        if(!isNodeInTree_singleRef(
            server, &eventFilter->selectClauses[i].typeDefinitionId,
            &baseEventTypeId, UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
            result[i] = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
            continue;
        }
        //attributeId is valid ?
        if(!((0 < eventFilter->selectClauses[i].attributeId) &&
             (eventFilter->selectClauses[i].attributeId < 28))) {
            result[i] = UA_STATUSCODE_BADATTRIBUTEIDINVALID;
            continue;
        }
        //browsePath contains null ?
        for(size_t j = 0; j < eventFilter->selectClauses[i].browsePathSize; ++j) {
            if(UA_QualifiedName_isNull(
                &eventFilter->selectClauses[i].browsePath[j])) {
                result[i] = UA_STATUSCODE_BADBROWSENAMEINVALID;
                break;
            }
        }
        if(result[i] != UA_STATUSCODE_GOOD)
            continue;
        //indexRange is defined ?
        if(!UA_String_equal(&eventFilter->selectClauses[i].indexRange,
                            &UA_STRING_NULL)) {
            //indexRange is parsable ?
            UA_NumericRange numericRange = UA_NUMERICRANGE("");
            if(UA_NumericRange_parse(&numericRange,
                                     eventFilter->selectClauses[i].indexRange) !=
               UA_STATUSCODE_GOOD) {
                result[i] = UA_STATUSCODE_BADINDEXRANGEINVALID;
                continue;
            }
            UA_free(numericRange.dimensions);
            //attributeId is value ?
            if(eventFilter->selectClauses[i].attributeId != UA_ATTRIBUTEID_VALUE) {
                result[i] = UA_STATUSCODE_BADTYPEMISMATCH;
                continue;
            }
        }
    }
}

/*
 * Initial content filter (where clause) check. Current checks:
 * - Number of operands for each (supported) operator
 */
UA_StatusCode
UA_Event_staticWhereClauseValidation(UA_Server *server,
                                     const UA_ContentFilter *filter,
                                     UA_ContentFilterResult *result) {
    UA_ContentFilterResult_init(result);
    result->elementResultsSize = filter->elementsSize;
    if(result->elementResultsSize == 0)
        return UA_STATUSCODE_GOOD;
    result->elementResults =
        (UA_ContentFilterElementResult *)UA_Array_new(
            result->elementResultsSize,
            &UA_TYPES[UA_TYPES_CONTENTFILTERELEMENTRESULT]);
    if(!result->elementResults)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    for(size_t i = 0; i < result->elementResultsSize; ++i) {
        UA_ContentFilterElementResult *er = &result->elementResults[i];
        UA_ContentFilterElement ef = filter->elements[i];
        UA_ContentFilterElementResult_init(er);
        er->operandStatusCodes =
            (UA_StatusCode *)UA_Array_new(
                ef.filterOperandsSize,
                &UA_TYPES[UA_TYPES_STATUSCODE]);
        er->operandStatusCodesSize = ef.filterOperandsSize;

        switch(ef.filterOperator) {
            case UA_FILTEROPERATOR_INVIEW:
            case UA_FILTEROPERATOR_RELATEDTO: {
                /* Not allowed for event WhereClause according to 7.17.3 in Part 4 */
                er->statusCode =
                    UA_STATUSCODE_BADEVENTFILTERINVALID;
                break;
            }
            case UA_FILTEROPERATOR_EQUALS:
            case UA_FILTEROPERATOR_GREATERTHAN:
            case UA_FILTEROPERATOR_LESSTHAN:
            case UA_FILTEROPERATOR_GREATERTHANOREQUAL:
            case UA_FILTEROPERATOR_LESSTHANOREQUAL:
            case UA_FILTEROPERATOR_LIKE:
            case UA_FILTEROPERATOR_CAST:
            case UA_FILTEROPERATOR_BITWISEAND:
            case UA_FILTEROPERATOR_BITWISEOR: {
                if(ef.filterOperandsSize != 2) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_AND:
            case UA_FILTEROPERATOR_OR: {
                if(ef.filterOperandsSize != 2) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                for(size_t j = 0; j < 2; ++j) {
                    if(ef.filterOperands[j].content.decoded.type !=
                       &UA_TYPES[UA_TYPES_ELEMENTOPERAND]) {
                        er->operandStatusCodes[j] =
                            UA_STATUSCODE_BADFILTEROPERANDINVALID;
                        er->statusCode =
                            UA_STATUSCODE_BADFILTEROPERANDINVALID;
                        break;
                    }
                    if(((UA_ElementOperand *)ef.filterOperands[j]
                        .content.decoded.data)->index > filter->elementsSize - 1) {
                        er->operandStatusCodes[j] =
                            UA_STATUSCODE_BADINDEXRANGEINVALID;
                        er->statusCode =
                            UA_STATUSCODE_BADINDEXRANGEINVALID;
                        break;
                    }
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_ISNULL:
            case UA_FILTEROPERATOR_NOT: {
                if(ef.filterOperandsSize != 1) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_INLIST: {
                if(ef.filterOperandsSize >= 2) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_BETWEEN: {
                if(ef.filterOperandsSize != 3) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            case UA_FILTEROPERATOR_OFTYPE: {
                if(ef.filterOperandsSize != 1) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDCOUNTMISMATCH;
                    break;
                }
                er->operandStatusCodesSize = ef.filterOperandsSize;
                if(ef.filterOperands[0].content.decoded.type !=
                   &UA_TYPES[UA_TYPES_LITERALOPERAND]) {
                    er->statusCode =
                        UA_STATUSCODE_BADFILTEROPERANDINVALID;
                    break;
                }
                UA_LiteralOperand *literalOperand =
                    (UA_LiteralOperand *)ef.filterOperands[0]
                        .content.decoded.data;

                if(((UA_NodeId *)literalOperand->value.data)->identifierType !=
                   UA_NODEIDTYPE_NUMERIC) {
                    er->statusCode =
                        UA_STATUSCODE_BADATTRIBUTEIDINVALID;
                    break;
                }
                /* Make sure the &pOperand->nodeId is a subtype of BaseEventType */
                UA_NodeId baseEventTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
                if(!isNodeInTree_singleRef(
                    server, (UA_NodeId *)literalOperand->value.data, &baseEventTypeId,
                    UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
                    er->statusCode =
                        UA_STATUSCODE_BADNODEIDINVALID;
                    break;
                }
                er->statusCode = UA_STATUSCODE_GOOD;
                break;
            }
            default:
                er->statusCode =
                    UA_STATUSCODE_BADFILTEROPERATORUNSUPPORTED;
                break;
        }
    }
    return UA_STATUSCODE_GOOD;
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
