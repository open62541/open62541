/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Sameer AL-Qadasi)
 *    Copyright 2020 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

typedef enum {
  UA_INACTIVE,
  UA_ACTIVE,
  UA_ACTIVE_HIGHHIGH,
  UA_ACTIVE_HIGH,
  UA_ACTIVE_LOW,
  UA_ACTIVE_LOWLOW
} UA_ActiveState;

typedef struct {
    UA_TwoStateVariableChangeCallback enableStateCallback;
    UA_TwoStateVariableChangeCallback ackStateCallback;
    UA_Boolean ackedRemoveBranch;
    UA_TwoStateVariableChangeCallback confirmStateCallback;
    UA_Boolean confirmedRemoveBranch;
    UA_TwoStateVariableChangeCallback activeStateCallback;
} UA_ConditionCallbacks;

/* In Alarms and Conditions first implementation, conditionBranchId is always
 * equal to NULL NodeId (UA_NODEID_NULL). That ConditionBranch represents the
 * current state Condition. The current state is determined by the last Event
 * triggered (lastEventId). See Part 9, 5.5.2, BranchId. */
typedef struct UA_ConditionBranch {
    LIST_ENTRY(UA_ConditionBranch) listEntry;
    UA_NodeId conditionBranchId;
    UA_ByteString lastEventId;
    UA_Boolean isCallerAC;
} UA_ConditionBranch;

/* In Alarms and Conditions first implementation, A Condition
 * have only one ConditionBranch entry. */
typedef struct UA_Condition {
    LIST_ENTRY(UA_Condition) listEntry;
    LIST_HEAD(, UA_ConditionBranch) conditionBranches;
    UA_NodeId conditionId;
    UA_UInt16 lastSeverity;
    UA_DateTime lastSeveritySourceTimeStamp;
    UA_ConditionCallbacks callbacks;
    UA_ActiveState lastActiveState;
    UA_ActiveState currentActiveState;
    UA_Boolean isLimitAlarm;
} UA_Condition;

/* A ConditionSource can have multiple Conditions. */
struct UA_ConditionSource {
    LIST_ENTRY(UA_ConditionSource) listEntry;
    LIST_HEAD(, UA_Condition) conditions;
    UA_NodeId conditionSourceId;
};

#define CONDITIONOPTIONALFIELDS_SUPPORT // change array size!
#define CONDITION_SEVERITYCHANGECALLBACK_ENABLE

/* Condition Field Names */
#define CONDITION_FIELD_EVENTID                                "EventId"
#define CONDITION_FIELD_EVENTTYPE                              "EventType"
#define CONDITION_FIELD_SOURCENODE                             "SourceNode"
#define CONDITION_FIELD_SOURCENAME                             "SourceName"
#define CONDITION_FIELD_TIME                                   "Time"
#define CONDITION_FIELD_RECEIVETIME                            "ReceiveTime"
#define CONDITION_FIELD_MESSAGE                                "Message"
#define CONDITION_FIELD_SEVERITY                               "Severity"
#define CONDITION_FIELD_CONDITIONNAME                          "ConditionName"
#define CONDITION_FIELD_BRANCHID                               "BranchId"
#define CONDITION_FIELD_RETAIN                                 "Retain"
#define CONDITION_FIELD_ENABLEDSTATE                           "EnabledState"
#define CONDITION_FIELD_TWOSTATEVARIABLE_ID                    "Id"
#define CONDITION_FIELD_QUALITY                                "Quality"
#define CONDITION_FIELD_LASTSEVERITY                           "LastSeverity"
#define CONDITION_FIELD_COMMENT                                "Comment"
#define CONDITION_FIELD_CLIENTUSERID                           "ClientUserId"
#define CONDITION_FIELD_CONDITIONVARIABLE_SOURCETIMESTAMP      "SourceTimestamp"
#define CONDITION_FIELD_DISABLE                                "Disable"
#define CONDITION_FIELD_ENABLE                                 "Enable"
#define CONDITION_FIELD_ADDCOMMENT                             "AddComment"
#define CONDITION_FIELD_CONDITIONREFRESH                       "ConditionRefresh"
#define CONDITION_FIELD_ACKEDSTATE                             "AckedState"
#define CONDITION_FIELD_CONFIRMEDSTATE                         "ConfirmedState"
#define CONDITION_FIELD_ACKNOWLEDGE                            "Acknowledge"
#define CONDITION_FIELD_CONFIRM                                "Confirm"
#define CONDITION_FIELD_ACTIVESTATE                            "ActiveState"
#define CONDITION_FIELD_INPUTNODE                              "InputNode"
#define CONDITION_FIELD_SUPPRESSEDORSHELVED                    "SuppressedOrShelved"
#define CONDITION_FIELD_NORMALSTATE                            "NormalState"
#define CONDITION_FIELD_HIGHHIGHLIMIT                          "HighHighLimit"
#define CONDITION_FIELD_HIGHLIMIT                              "HighLimit"
#define CONDITION_FIELD_LOWLIMIT                               "LowLimit"
#define CONDITION_FIELD_LOWLOWLIMIT                            "LowLowLimit"
#define CONDITION_FIELD_PROPERTY_EFFECTIVEDISPLAYNAME          "EffectiveDisplayName"
#define CONDITION_FIELD_LIMITSTATE                             "LimitState"
#define CONDITION_FIELD_CURRENTSTATE                           "CurrentState"
#define CONDITION_FIELD_HIGHHIGHSTATE                          "HighHighState"
#define CONDITION_FIELD_HIGHSTATE                              "HighState"
#define CONDITION_FIELD_LOWSTATE                               "LowState"
#define CONDITION_FIELD_LOWLOWSTATE                            "LowLowState"
#define CONDITION_FIELD_DIALOGSTATE                            "DialogState"
#define CONDITION_FIELD_PROMPT                                 "Prompt"
#define CONDITION_FIELD_RESPONSEOPTIONSET                      "ResponseOptionSet"
#define CONDITION_FIELD_DEFAULTRESPONSE                        "DefaultResponse"
#define CONDITION_FIELD_LASTRESPONSE                           "LastResponse"
#define CONDITION_FIELD_OKRESPONSE                             "OkResponse"
#define CONDITION_FIELD_CANCELRESPONSE                         "CancelResponse"
#define CONDITION_FIELD_RESPOND                                "Respond"

#define REFRESHEVENT_START_IDX                                 0
#define REFRESHEVENT_END_IDX                                   1
#define REFRESHEVENT_SEVERITY_DEFAULT                          100

#define LOCALE                                                 "en"
#define ENABLED_TEXT                                           "Enabled"
#define DISABLED_TEXT                                          "Disabled"
#define ENABLED_MESSAGE                                        "The alarm was enabled"
#define DISABLED_MESSAGE                                       "The alarm was disabled"
#define COMMENT_MESSAGE                                        "A comment was added"
#define SEVERITY_INCREASED_MESSAGE                             "The alarm severity has increased"
#define SEVERITY_DECREASED_MESSAGE                             "The alarm severity has decreased"
#define ACKED_TEXT                                             "Acknowledged"
#define UNACKED_TEXT                                           "Unacknowledged"
#define CONFIRMED_TEXT                                         "Confirmed"
#define UNCONFIRMED_TEXT                                       "Unconfirmed"
#define ACKED_MESSAGE                                          "The alarm was acknowledged"
#define CONFIRMED_MESSAGE                                      "The alarm was confirmed"
#define ACTIVE_TEXT                                            "Active"
#define ACTIVE_HIGHHIGH_TEXT                                   "HighHigh active"
#define ACTIVE_HIGH_TEXT                                       "High active"
#define ACTIVE_LOW_TEXT                                        "Low active"
#define ACTIVE_LOWLOW_TEXT                                     "LowLow active"
#define INACTIVE_TEXT                                          "Inactive"

#define STATIC_QN(name) {0, UA_STRING_STATIC(name)}
static const UA_QualifiedName fieldEnabledStateQN = STATIC_QN(CONDITION_FIELD_ENABLEDSTATE);
static const UA_QualifiedName fieldRetainQN = STATIC_QN(CONDITION_FIELD_RETAIN);
static const UA_QualifiedName twoStateVariableIdQN = STATIC_QN(CONDITION_FIELD_TWOSTATEVARIABLE_ID);
static const UA_QualifiedName fieldMessageQN = STATIC_QN(CONDITION_FIELD_MESSAGE);
static const UA_QualifiedName fieldAckedStateQN = STATIC_QN(CONDITION_FIELD_ACKEDSTATE);
static const UA_QualifiedName fieldConfirmedStateQN = STATIC_QN(CONDITION_FIELD_CONFIRMEDSTATE);
static const UA_QualifiedName fieldActiveStateQN = STATIC_QN(CONDITION_FIELD_ACTIVESTATE);
static const UA_QualifiedName fieldTimeQN = STATIC_QN(CONDITION_FIELD_TIME);
static const UA_QualifiedName fieldSourceQN = STATIC_QN(CONDITION_FIELD_SOURCENODE);

#define CONDITION_ASSERT_RETURN_RETVAL(retval, logMessage, deleteFunction)                \
    {                                                                                     \
        if(retval != UA_STATUSCODE_GOOD) {                                                \
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,                  \
                         logMessage". StatusCode %s", UA_StatusCode_name(retval));        \
            deleteFunction                                                                \
            return retval;                                                                \
        }                                                                                 \
    }

#define CONDITION_ASSERT_RETURN_VOID(retval, logMessage, deleteFunction)                  \
    {                                                                                     \
        if(retval != UA_STATUSCODE_GOOD) {                                                \
            UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,                  \
                         logMessage". StatusCode %s", UA_StatusCode_name(retval));        \
            deleteFunction                                                                \
            return;                                                                       \
        }                                                                                 \
    }

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

static UA_NodeId refreshEvents[2] =
    {{0, UA_NODEIDTYPE_NUMERIC, {0}},
     {0, UA_NODEIDTYPE_NUMERIC, {0}}};

static UA_ConditionSource *
getConditionSource(UA_Server *server, const UA_NodeId *sourceId) {
    UA_ConditionSource *cs;
    LIST_FOREACH(cs, &server->conditionSources, listEntry) {
        if(UA_NodeId_equal(&cs->conditionSourceId, sourceId))
            return cs;
    }
    return NULL;
}

static UA_Condition *
getCondition(UA_Server *server, const UA_NodeId *sourceId,
             const UA_NodeId *conditionId) {
    UA_ConditionSource *cs = getConditionSource(server, sourceId);
    if(!cs)
        return NULL;

    UA_Condition *c;
    LIST_FOREACH(c, &cs->conditions, listEntry) {
        if(UA_NodeId_equal(&c->conditionId, conditionId))
            return c;
    }
    return NULL;
}

/* Function used to set a user specific callback to TwoStateVariable Fields of a
 * condition. The callbacks will be called before triggering the events when
 * transition to true State of EnabledState/Id, AckedState/Id, ConfirmedState/Id
 * and ActiveState/Id occurs.
 * @param removeBranch is not used for the first implementation */
UA_StatusCode
UA_Server_setConditionTwoStateVariableCallback(UA_Server *server, const UA_NodeId condition,
                                               const UA_NodeId conditionSource, UA_Boolean removeBranch,
                                               UA_TwoStateVariableChangeCallback callback,
                                               UA_TwoStateVariableCallbackType callbackType) {
    /* Get Condition */
    UA_Condition *c = getCondition(server, &conditionSource, &condition);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;

    /* Set the callback */
    switch(callbackType) {
    case UA_ENTERING_ENABLEDSTATE:
        c->callbacks.enableStateCallback = callback;
        break;
    case UA_ENTERING_ACKEDSTATE:
        c->callbacks.ackStateCallback = callback;
        c->callbacks.ackedRemoveBranch = removeBranch;
        break;
    case UA_ENTERING_CONFIRMEDSTATE:
        c->callbacks.confirmStateCallback = callback;
        c->callbacks.confirmedRemoveBranch = removeBranch;
        break;
    case UA_ENTERING_ACTIVESTATE:
        c->callbacks.activeStateCallback = callback;
        break;
    default:
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getConditionTwoStateVariableCallback(UA_Server *server, const UA_NodeId *branch,
                                    UA_Condition *condition, UA_Boolean *removeBranch,
                                    UA_TwoStateVariableCallbackType callbackType) {
    switch(callbackType) {
    case UA_ENTERING_ENABLEDSTATE:
        if(condition->callbacks.enableStateCallback != NULL)
            return condition->callbacks.enableStateCallback(server, branch);
        return UA_STATUSCODE_GOOD;//TODO log warning when the callback wasn't set

    case UA_ENTERING_ACKEDSTATE:
        if(condition->callbacks.ackStateCallback != NULL) {
            *removeBranch = condition->callbacks.ackedRemoveBranch;
            return condition->callbacks.ackStateCallback(server, branch);
        }
        return UA_STATUSCODE_GOOD;

    case UA_ENTERING_CONFIRMEDSTATE:
        if(condition->callbacks.confirmStateCallback != NULL) {
            *removeBranch = condition->callbacks.confirmedRemoveBranch;
            return condition->callbacks.confirmStateCallback(server, branch);
        }
        return UA_STATUSCODE_GOOD;

    case UA_ENTERING_ACTIVESTATE:
        if(condition->callbacks.activeStateCallback != NULL)
            return condition->callbacks.activeStateCallback(server, branch);
        return UA_STATUSCODE_GOOD;

    default:
        return UA_STATUSCODE_BADNOTFOUND;
    }
}

static UA_StatusCode
callConditionTwoStateVariableCallback(UA_Server *server, const UA_NodeId *condition,
                                      const UA_NodeId *conditionSource, UA_Boolean *removeBranch,
                                      UA_TwoStateVariableCallbackType callbackType) {
    UA_ConditionSource *source = getConditionSource(server, conditionSource);
    if(!source)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_Condition *cond;
    LIST_FOREACH(cond, &source->conditions, listEntry) {
        if(UA_NodeId_equal(&cond->conditionId, condition)) {
            return getConditionTwoStateVariableCallback(server, condition, cond,
                                                        removeBranch, callbackType);
        }
        UA_ConditionBranch *branch;
        LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
            if(!UA_NodeId_equal(&branch->conditionBranchId, condition))
                continue;
            return getConditionTwoStateVariableCallback(server, &branch->conditionBranchId,
                                                        cond, removeBranch, callbackType);
        }
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

/* Gets the parent NodeId of a Field (e.g. Severity) or Field Property (e.g.
 * EnabledState/Id) */
static UA_StatusCode
getFieldParentNodeId(UA_Server *server, const UA_NodeId *field, UA_NodeId *parent) {
    *parent = UA_NODEID_NULL;
    const UA_Node *fieldNode = UA_NODESTORE_GET(server, field);
    if(!fieldNode)
        return UA_STATUSCODE_BADNOTFOUND;
    UA_StatusCode retval = UA_STATUSCODE_BADNOTFOUND;
    for(size_t i = 0; i < fieldNode->head.referencesSize; i++) {
        UA_NodeReferenceKind *rk = &fieldNode->head.references[i];
        if(rk->referenceTypeIndex != UA_REFERENCETYPEINDEX_HASPROPERTY &&
           rk->referenceTypeIndex != UA_REFERENCETYPEINDEX_HASCOMPONENT)
            continue;
        if(!rk->isInverse)
            continue;
        /* Take the first hierarchical inverse reference */
        const UA_ReferenceTarget *target = NULL;
        while((target = UA_NodeReferenceKind_iterate(rk, target))) {
            if(!UA_NodePointer_isLocal(target->targetId))
                continue;
            UA_NodeId tmpNodeId = UA_NodePointer_toNodeId(target->targetId);
            retval = UA_NodeId_copy(&tmpNodeId, parent);
            goto finish;
        }
    }
 finish:
    UA_NODESTORE_RELEASE(server, (const UA_Node *)fieldNode);
    return retval;
}

/* Gets the NodeId of a Field (e.g. Severity) */
static UA_StatusCode
getConditionFieldNodeId(UA_Server *server, const UA_NodeId *conditionNodeId,
                        const UA_QualifiedName* fieldName, UA_NodeId *outFieldNodeId) {
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, *conditionNodeId, 1, fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;
    UA_StatusCode retval = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, outFieldNodeId);
    UA_BrowsePathResult_clear(&bpr);
    return retval;
}

/* Gets the NodeId of a Field Property (e.g. EnabledState/Id) */
static UA_StatusCode
getConditionFieldPropertyNodeId(UA_Server *server, const UA_NodeId *originCondition,
                                const UA_QualifiedName* variableFieldName,
                                const UA_QualifiedName* variablePropertyName,
                                UA_NodeId *outFieldPropertyNodeId) {
    /* 1) Find Variable Field of the Condition */
    UA_BrowsePathResult bprConditionVariableField =
        UA_Server_browseSimplifiedBrowsePath(server, *originCondition, 1, variableFieldName);
    if(bprConditionVariableField.statusCode != UA_STATUSCODE_GOOD)
        return bprConditionVariableField.statusCode;

    /* 2) Find Property of the Variable Field of the Condition */
    UA_BrowsePathResult bprVariableFieldProperty =
        UA_Server_browseSimplifiedBrowsePath(server, bprConditionVariableField.targets->targetId.nodeId,
                                             1, variablePropertyName);
    if(bprVariableFieldProperty.statusCode != UA_STATUSCODE_GOOD) {
        UA_BrowsePathResult_clear(&bprConditionVariableField);
        return bprVariableFieldProperty.statusCode;
    }

    *outFieldPropertyNodeId = bprVariableFieldProperty.targets[0].targetId.nodeId;
    UA_NodeId_init(&bprVariableFieldProperty.targets[0].targetId.nodeId);
    UA_BrowsePathResult_clear(&bprConditionVariableField);
    UA_BrowsePathResult_clear(&bprVariableFieldProperty);
    return UA_STATUSCODE_GOOD;
}

/* Gets NodeId value of a Field which has NodeId as DataType (e.g. EventType) */
static UA_StatusCode
getNodeIdValueOfConditionField(UA_Server *server, const UA_NodeId *condition,
                               UA_QualifiedName fieldName, UA_NodeId *outNodeId) {
    *outNodeId = UA_NODEID_NULL;
    UA_NodeId nodeIdValue;
    UA_StatusCode retval = getConditionFieldNodeId(server, condition, &fieldName, &nodeIdValue);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Field not found",);

    UA_Variant tOutVariant;
    UA_Variant_init(&tOutVariant);

    /* Read the Value of SourceNode Property Node (the Value is a NodeId) */
    retval = UA_Server_readValue(server, nodeIdValue, &tOutVariant);
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_NODEID])) {
        UA_NodeId_clear(&nodeIdValue);
        UA_Variant_clear(&tOutVariant);
        return retval;
    }

    *outNodeId = *(UA_NodeId*)tOutVariant.data;
    UA_NodeId_init((UA_NodeId*)tOutVariant.data);
    UA_NodeId_clear(&nodeIdValue);
    UA_Variant_clear(&tOutVariant);
    return UA_STATUSCODE_GOOD;
}

/* Gets the NodeId of a condition branch. In case of main branch (BranchId ==
 * UA_NODEID_NULL), ConditionId will be returned. */
static UA_StatusCode
getConditionBranchNodeId(UA_Server *server, const UA_ByteString *eventId,
                         UA_NodeId *outConditionBranchNodeId) {
    *outConditionBranchNodeId = UA_NODEID_NULL;
    /* The function checks the BranchId based on the event Id, if BranchId ==
       NULL -> outConditionId = ConditionId */
    /* Get ConditionSource Entry */
    UA_ConditionSource *source;
    LIST_FOREACH(source, &server->conditionSources, listEntry) {
        /* Get Condition Entry */
        UA_Condition *cond;
        LIST_FOREACH(cond, &source->conditions, listEntry) {
            /* Get Branch Entry*/
            UA_ConditionBranch *branch;
            LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
                if(!UA_ByteString_equal(&branch->lastEventId, eventId))
                    continue;
                if(UA_NodeId_isNull(&branch->conditionBranchId))
                    return UA_NodeId_copy(&cond->conditionId, outConditionBranchNodeId);
                return UA_NodeId_copy(&branch->conditionBranchId, outConditionBranchNodeId);
            }
        }
    }

    return UA_STATUSCODE_BADEVENTIDUNKNOWN;
}

static UA_StatusCode
getConditionLastSeverity(UA_Server *server, const UA_NodeId *conditionSource,
                         const UA_NodeId *conditionId, UA_UInt16 *outLastSeverity,
                         UA_DateTime *outLastSeveritySourceTimeStamp) {
    UA_Condition *cond = getCondition(server, conditionSource, conditionId);
    if(!cond) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    *outLastSeverity = cond->lastSeverity;
    *outLastSeveritySourceTimeStamp = cond->lastSeveritySourceTimeStamp;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
updateConditionLastSeverity(UA_Server *server, const UA_NodeId *conditionSource,
                            const UA_NodeId *conditionId, UA_UInt16 lastSeverity,
                            UA_DateTime lastSeveritySourceTimeStamp) {
    UA_Condition *cond = getCondition(server, conditionSource, conditionId);
    if(!cond) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    cond->lastSeverity = lastSeverity;
    cond->lastSeveritySourceTimeStamp =  lastSeveritySourceTimeStamp;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getConditionActiveState(UA_Server *server, const UA_NodeId *conditionSource,
                         const UA_NodeId *conditionId, UA_ActiveState *outLastActiveState,
                         UA_ActiveState *outCurrentActiveState, UA_Boolean *outIsLimitAlarm) {
    UA_Condition *cond = getCondition(server, conditionSource, conditionId);
    if(!cond) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    *outLastActiveState = cond->lastActiveState;
    *outCurrentActiveState = cond->currentActiveState;
    *outIsLimitAlarm = cond->isLimitAlarm;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
updateConditionActiveState(UA_Server *server, const UA_NodeId *conditionSource,
                            const UA_NodeId *conditionId, const UA_ActiveState lastActiveState,
                            const UA_ActiveState currentActiveState, UA_Boolean isLimitAlarm) {
    UA_Condition *cond = getCondition(server, conditionSource, conditionId);
    if(!cond) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    cond->lastActiveState = lastActiveState;
    cond->currentActiveState = currentActiveState;
    cond->isLimitAlarm = isLimitAlarm;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
updateConditionLastEventId(UA_Server *server, const UA_NodeId *triggeredEvent,
                           const UA_NodeId *conditionSource,
                           const UA_ByteString *lastEventId) {
    UA_Condition *cond = getCondition(server, conditionSource, triggeredEvent);
    if(!cond) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_ConditionBranch *branch;
    LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
        if(UA_NodeId_isNull(&branch->conditionBranchId)) {
            /* update main condition branch */
            UA_ByteString_clear(&branch->lastEventId);
            return UA_ByteString_copy(lastEventId, &branch->lastEventId);
        }
    }
    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                 "Condition Branch not implemented");
    return UA_STATUSCODE_BADNOTFOUND;
}

static void
setIsCallerAC(UA_Server *server, const UA_NodeId *condition,
              const UA_NodeId *conditionSource, UA_Boolean isCallerAC) {
    UA_Condition *cond = getCondition(server, conditionSource, condition);
    if(!cond) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return;
    }

    UA_ConditionBranch *branch;
    LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
        if(UA_NodeId_isNull(&branch->conditionBranchId)) {
            branch->isCallerAC = isCallerAC;
            return;
        }
    }
    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                 "Condition Branch not implemented");
}

UA_Boolean
isConditionOrBranch(UA_Server *server, const UA_NodeId *condition,
                    const UA_NodeId *conditionSource, UA_Boolean *isCallerAC) {
    UA_Condition *cond = getCondition(server, conditionSource, condition);
    if(!cond) {
        UA_LOG_DEBUG(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return false;
    }

    UA_ConditionBranch *branch;
    LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
        if(UA_NodeId_isNull(&branch->conditionBranchId)) {
            *isCallerAC = branch->isCallerAC;
            return true;
        }
    }
    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                 "Condition Branch not implemented");
    return false;
}

static UA_Boolean
isRetained(UA_Server *server, const UA_NodeId *condition) {
    /* Get EnabledStateId NodeId */
    UA_NodeId retainNodeId;
    UA_StatusCode retval = getConditionFieldNodeId(server, condition, &fieldRetainQN, &retainNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Retain not found. StatusCode %s", UA_StatusCode_name(retval));
        return false; //TODO maybe a better error handling?
    }

    /* Read Retain value */
    UA_Variant tOutVariant;
    UA_Variant_init(&tOutVariant);

    retval = UA_Server_readValue(server, retainNodeId, &tOutVariant);
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_BOOLEAN])) {
          UA_NodeId_clear(&retainNodeId);
          return false;
    }

    if(*(UA_Boolean *)tOutVariant.data == true) {
        UA_NodeId_clear(&retainNodeId);
        UA_Variant_clear(&tOutVariant);
        return true;
    }

    UA_NodeId_clear(&retainNodeId);
    UA_Variant_clear(&tOutVariant);
    return false;
}

static UA_Boolean
isTwoStateVariableInTrueState(UA_Server *server, const UA_NodeId *condition,
                              const UA_QualifiedName *twoStateVariable) {
    /* Get TwoStateVariableId NodeId */
    UA_NodeId twoStateVariableIdNodeId;
    UA_StatusCode retval = getConditionFieldPropertyNodeId(server, condition, twoStateVariable,
                                                           &twoStateVariableIdQN,
                                                           &twoStateVariableIdNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "TwoStateVariable/Id not found. StatusCode %s", UA_StatusCode_name(retval));
        return false; //TODO maybe a better error handling?
    }

    /* Read Id value */
    UA_Variant tOutVariant;
    UA_Variant_init(&tOutVariant);

    retval = UA_Server_readValue(server, twoStateVariableIdNodeId, &tOutVariant);
    if(retval != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_NodeId_clear(&twoStateVariableIdNodeId);
        return false;
    }

    UA_NodeId_clear(&twoStateVariableIdNodeId);

    if(*(UA_Boolean *)tOutVariant.data == true) {
      UA_Variant_clear(&tOutVariant);
      return true;
    }

    UA_Variant_clear(&tOutVariant);
    return false;
}

static UA_StatusCode
enteringDisabledState(UA_Server *server, const UA_NodeId *conditionId,
                      const UA_NodeId *conditionSource) {
    UA_Condition *cond = getCondition(server, conditionSource, conditionId);
    if(!cond) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Get Branch Entry*/
    UA_ConditionBranch *branch;
    LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
        UA_NodeId triggeredNode;
        if(UA_NodeId_isNull(&branch->conditionBranchId))
            //disable main Condition Branch (BranchId == NULL)
            triggeredNode = cond->conditionId;
        else //disable all branches
            triggeredNode = branch->conditionBranchId;

        UA_LocalizedText message = UA_LOCALIZEDTEXT(LOCALE, DISABLED_MESSAGE);
        UA_LocalizedText enableText = UA_LOCALIZEDTEXT(LOCALE, DISABLED_TEXT);
        UA_Variant value;
        UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        UA_StatusCode retval = UA_Server_setConditionField(server, triggeredNode,
                                                           &value, fieldMessageQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Message failed",);

        UA_Variant_setScalar(&value, &enableText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, triggeredNode, &value, fieldEnabledStateQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition EnabledState text failed",);

        UA_Boolean retain = false;
        UA_Variant_setScalar(&value, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retval = UA_Server_setConditionField(server, triggeredNode, &value, fieldRetainQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Retain failed",);

        /* Trigger event */
        UA_ByteString lastEventId = UA_BYTESTRING_NULL;
        /* Trigger the event for Condition or its Branch */
        setIsCallerAC(server, &triggeredNode, conditionSource, true);
        /* Condition Nodes should not be deleted after triggering the event */
        retval = UA_Server_triggerEvent(server, triggeredNode, *conditionSource, &lastEventId, false);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Triggering condition event failed",);
        setIsCallerAC(server, &triggeredNode, conditionSource, false);

        /* Update list */
        retval = updateConditionLastEventId(server, &triggeredNode, conditionSource, &lastEventId);
        UA_ByteString_clear(&lastEventId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "updating condition event failed",);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
enteringEnabledState(UA_Server *server,
                     const UA_NodeId *conditionId,
                     const UA_NodeId *conditionSource) {
    /* Get Condition */
    UA_Condition *cond = getCondition(server, conditionSource, conditionId);
    if(!cond) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Entry not found in list!");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* Get Branch Entry*/
    UA_ConditionBranch *branch;
    LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
        UA_NodeId triggeredNode;
        UA_NodeId_init(&triggeredNode);
        if(UA_NodeId_isNull(&branch->conditionBranchId)) //enable main Condition
            triggeredNode = cond->conditionId;
        else //enable branches
            triggeredNode = branch->conditionBranchId;

        UA_LocalizedText message = UA_LOCALIZEDTEXT(LOCALE, ENABLED_MESSAGE);
        UA_LocalizedText enableText = UA_LOCALIZEDTEXT(LOCALE, ENABLED_TEXT);
        UA_Variant value;
        UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        UA_StatusCode retval = UA_Server_setConditionField(server, triggeredNode,
                                                           &value, fieldMessageQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "set Condition Message failed",);

        UA_Variant_setScalar(&value, &enableText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, triggeredNode, &value, fieldEnabledStateQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "set Condition EnabledState text failed",);

        /* User callback TODO how should branches be evaluated? see p.19 (5.5.2) */
        UA_Boolean removeBranch = false;//not used
        retval = callConditionTwoStateVariableCallback(server, &triggeredNode,
                                                       conditionSource, &removeBranch,
                                                       UA_ENTERING_ENABLEDSTATE);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "calling condition callback failed",);

        /* Trigger event */
        //Condition Nodes should not be deleted after triggering the event
        retval = UA_Server_triggerConditionEvent(server, triggeredNode, *conditionSource, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "triggering condition event failed",);
    }

    return UA_STATUSCODE_GOOD;
}

static void
afterWriteCallbackEnabledStateChange(UA_Server *server,
                                     const UA_NodeId *sessionId, void *sessionContext,
                                     const UA_NodeId *nodeId, void *nodeContext,
                                     const UA_NumericRange *range, const UA_DataValue *data) {
    /* Callback for change in EnabledState/Id property.
     * First we get the EnabledState NodeId then The Condition NodeId */
    UA_NodeId twoStateVariableNode;
    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent TwoStateVariable found for given EnabledState/Id",);

    UA_NodeId conditionNode;
    retval = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_clear(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given EnabledState",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &conditionNode, fieldSourceQN,
                                            &conditionSource);
    CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found",
                                 UA_NodeId_clear(&conditionNode););

    /* Set disabling/enabling time */
    retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                  (const UA_DateTime*)&data->sourceTimestamp,
                                                  &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set enabling/disabling Time failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););

    if(false == (*((UA_Boolean *)data->value.data))) {
        /* Disable all branches and update list */
        retval = enteringDisabledState(server, &conditionNode, &conditionSource);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Entering disabled state failed",
                                     UA_NodeId_clear(&conditionSource););
    } else {
        /* Enable all branches and update list */
        retval = enteringEnabledState(server, &conditionNode, &conditionSource);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Entering enabled state failed",
                                     UA_NodeId_clear(&conditionSource););
    }

    UA_NodeId_clear(&conditionSource);
}

static void
afterWriteCallbackAckedStateChange(UA_Server *server,
                                   const UA_NodeId *sessionId, void *sessionContext,
                                   const UA_NodeId *nodeId, void *nodeContext,
                                   const UA_NumericRange *range, const UA_DataValue *data) {
    /* Get the AckedState NodeId then The Condition NodeId */
    UA_NodeId twoStateVariableNode;
    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent TwoStateVariable found for given AckedState/Id",);

    UA_NodeId conditionNode;
    retval = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_clear(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given AckedState",);

    /* Callback for change to true in AckedState/Id property.
     * First check whether the value is true (ackedState/Id == true).
     * That check makes it possible to set ackedState/Id to false, without triggering an event */
    if(*((UA_Boolean *)data->value.data) == false) {
        /* Set unacknowledging time */
        retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                      &data->sourceTimestamp,
                                                      &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set deactivating Time failed",
                                     UA_NodeId_clear(&conditionNode););

        /* Set AckedState text to Unacknowledged*/
        UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, UNACKED_TEXT);
        UA_Variant value;
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, conditionNode, &value, fieldAckedStateQN);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition AckedState failed",);
        return;
    }

    /* Check if enabled and retained */
    if(!isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledStateQN) ||
       !isRetained(server, &conditionNode)) {
        /* Set AckedState/Id to false*/
        UA_Boolean idValue = false;
        UA_Variant value;
        UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode,
                                                             &value, fieldAckedStateQN,
                                                             twoStateVariableIdQN);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set AckedState/Id failed",);
        return;
    }

    /* Set Message */
    UA_LocalizedText message = UA_LOCALIZEDTEXT(LOCALE, ACKED_MESSAGE);
    UA_Variant value;
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, conditionNode, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition Message failed",
                                 UA_NodeId_clear(&conditionNode););

    /* Set AckedState text */
    UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, ACKED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, conditionNode, &value, fieldAckedStateQN);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition AckedState failed",
                                 UA_NodeId_clear(&conditionNode););

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &conditionNode, fieldSourceQN,
                                            &conditionSource);
    CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found",
                                 UA_NodeId_clear(&conditionNode););

    /* User callback*/
    UA_Boolean removeBranch = false;
    retval = callConditionTwoStateVariableCallback(server, &conditionNode, &conditionSource,
                                                   &removeBranch, UA_ENTERING_ACKEDSTATE);
    CONDITION_ASSERT_RETURN_VOID(retval, "Calling condition callback failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););

    /* Trigger event */
    //Condition Nodes should not be deleted after triggering the event
    retval = UA_Server_triggerConditionEvent(server, conditionNode, conditionSource, NULL);
    CONDITION_ASSERT_RETURN_VOID(retval, "Triggering condition event failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););
}

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT

static void
afterWriteCallbackConfirmedStateChange(UA_Server *server,
                                       const UA_NodeId *sessionId, void *sessionContext,
                                       const UA_NodeId *nodeId, void *nodeContext,
                                       const UA_NumericRange *range, const UA_DataValue *data) {
    UA_Variant value;
    UA_NodeId twoStateVariableNode;
    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent TwoStateVariable found for given ConfirmedState/Id",);

    UA_NodeId conditionNode;
    retval = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_clear(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given ConfirmedState",);

    /* Callback to change to true in ConfirmedState/Id property.
     * First check whether the value is true (ConfirmedState/Id == true).
     * That check makes it possible to set ConfirmedState/Id to false, without triggering an event */
    if(*((UA_Boolean *)data->value.data) == false) {
        /* Set unconfirming time */
        retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                      &data->sourceTimestamp,
                                                      &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set deactivating Time failed",
                                     UA_NodeId_clear(&conditionNode););

        /* Set ConfirmedState text to (Unconfirmed)*/
        UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, UNCONFIRMED_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, conditionNode, &value, fieldConfirmedStateQN);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition ConfirmedState failed",);
        return;
    }

    /* Check if enabled and retained */
    if(!isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledStateQN) ||
       !isRetained(server, &conditionNode)) {
        /* Set confirmedState/Id to false*/
        UA_Boolean idValue = false;
        UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode,
                                                             &value, fieldConfirmedStateQN,
                                                             twoStateVariableIdQN);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set ConfirmedState/Id failed",);
        return;
    }

    /* Set confirming time */
    retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                  &data->sourceTimestamp,
                                                  &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Confirming Time failed",
                                 UA_NodeId_clear(&conditionNode););

    /* Set Message */
    UA_LocalizedText message = UA_LOCALIZEDTEXT(LOCALE, CONFIRMED_MESSAGE);
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, conditionNode, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition Message failed",
                                 UA_NodeId_clear(&conditionNode););

    /* Set ConfirmedState text */
    UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, CONFIRMED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, conditionNode, &value, fieldConfirmedStateQN);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition ConfirmedState failed",
                                 UA_NodeId_clear(&conditionNode););

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &conditionNode, fieldSourceQN, &conditionSource);
    CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found",
                                 UA_NodeId_clear(&conditionNode););

    /* User callback*/
    UA_Boolean removeBranch = false;
    retval = callConditionTwoStateVariableCallback(server, &conditionNode,
                                                   &conditionSource, &removeBranch,
                                                   UA_ENTERING_CONFIRMEDSTATE);
    CONDITION_ASSERT_RETURN_VOID(retval, "Calling condition callback failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););

    /* Trigger event */
    //Condition Nodes should not be deleted after triggering the event
    retval = UA_Server_triggerConditionEvent(server, conditionNode, conditionSource, NULL);
    CONDITION_ASSERT_RETURN_VOID(retval, "Triggering condition event failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););
}
#endif//CONDITIONOPTIONALFIELDS_SUPPORT

static void
afterWriteCallbackActiveStateChange(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *nodeId, void *nodeContext,
                                    const UA_NumericRange *range, const UA_DataValue *data) {
    UA_Variant value;
    UA_NodeId twoStateVariableNode;
    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent TwoStateVariable found for given ActiveState/Id",);

    UA_NodeId conditionNode;
    retval = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_clear(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given ActiveState",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &conditionNode, fieldSourceQN, &conditionSource);
    CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found",
                                 UA_NodeId_clear(&conditionNode););

    UA_ActiveState lastActiveState = UA_INACTIVE;
    UA_ActiveState currentActiveState = UA_INACTIVE;
    UA_Boolean isLimitalarm = false;

    retval = getConditionActiveState(server, &conditionSource, &conditionNode,
                                     &lastActiveState, &currentActiveState, &isLimitalarm);
    CONDITION_ASSERT_RETURN_VOID(retval, "ActiveState transition check failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););

    if(isLimitalarm == false) {
      if(*((UA_Boolean *)data->value.data) == true) {
        retval = updateConditionActiveState(server, &conditionSource, &conditionNode,
                                            currentActiveState, UA_ACTIVE, false);
        CONDITION_ASSERT_RETURN_VOID(retval, "Updating ActiveState failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););
      } else {
        retval = updateConditionActiveState(server, &conditionSource, &conditionNode,
                                            currentActiveState, UA_INACTIVE, false);
        CONDITION_ASSERT_RETURN_VOID(retval, "Updating ActiveState failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););
      }

      retval = getConditionActiveState(server, &conditionSource, &conditionNode,
                                       &lastActiveState, &currentActiveState, &isLimitalarm);
      CONDITION_ASSERT_RETURN_VOID(retval, "ActiveState transition check failed",
                                   UA_NodeId_clear(&conditionNode);
                                   UA_NodeId_clear(&conditionSource););
    }

    /* callback for change to true in ActiveState/Id property.
     * first check whether the value is true (ActiveState/Id == true).
     * That check makes it possible to set ActiveState/Id to false, without triggering an event */
    if(*((UA_Boolean *)data->value.data) == true &&
       (lastActiveState != currentActiveState)) {

        /* Check if enabled and retained */
        if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledStateQN) &&
            isRetained(server, &conditionNode)) {
            /* Set activating time */
            retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                          &data->sourceTimestamp,
                                                          &UA_TYPES[UA_TYPES_DATETIME]);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set activating Time failed",
                                         UA_NodeId_clear(&conditionNode);
                                         UA_NodeId_clear(&conditionSource););

            /* Set ActiveState text */
            UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, ACTIVE_TEXT);
            UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            retval = UA_Server_setConditionField(server, conditionNode, &value, fieldActiveStateQN);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition ActiveState failed",
                                         UA_NodeId_clear(&conditionNode);
                                         UA_NodeId_clear(&conditionSource););

            /* User callback*/
            UA_Boolean removeBranch = false;//not used
            retval = callConditionTwoStateVariableCallback(server, &conditionNode, &conditionSource,
                                                           &removeBranch, UA_ENTERING_ACTIVESTATE);
            CONDITION_ASSERT_RETURN_VOID(retval, "Calling condition callback failed",
                                         UA_NodeId_clear(&conditionNode);
                                         UA_NodeId_clear(&conditionSource););

            /* Trigger event */
            //Condition Nodes should not be deleted after triggering the event
            retval = UA_Server_triggerConditionEvent(server, conditionNode, conditionSource, NULL);
            UA_NodeId_clear(&conditionNode);
            UA_NodeId_clear(&conditionSource);
            CONDITION_ASSERT_RETURN_VOID(retval, "Triggering condition event failed",);
        } else {
            /* Set ActiveState/Id to false -> don't apply changes in case of
             * disabled or not retained*/
            UA_Boolean idValue = false;
            UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode, &value,
                                                                 fieldActiveStateQN,
                                                                 twoStateVariableIdQN);
            UA_NodeId_clear(&conditionSource);
            UA_NodeId_clear(&conditionNode);
            CONDITION_ASSERT_RETURN_VOID(retval, "Set ActiveState/Id failed",);
        }
        return;
    }

    if((*((UA_Boolean *)data->value.data) == false) &&
       (lastActiveState != currentActiveState)) {
        /* Set ActiveState text to (Inactive) */
        UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, INACTIVE_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, conditionNode, &value, fieldActiveStateQN);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition ActiveState failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););

        /* Set deactivating time */
        retval = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                      &data->sourceTimestamp,
                                                      &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set deactivating Time failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););

        retval = updateConditionActiveState(server, &conditionSource, &conditionNode,
                                            currentActiveState, UA_INACTIVE, isLimitalarm);
        UA_NodeId_clear(&conditionSource);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID(retval, "Set ActiveState failed",);
    }
}

static void
afterWriteCallbackQualityChange(UA_Server *server,
                                const UA_NodeId *sessionId, void *sessionContext,
                                const UA_NodeId *nodeId, void *nodeContext,
                                const UA_NumericRange *range, const UA_DataValue *data) {
    //TODO
}

static void
afterWriteCallbackSeverityChange(UA_Server *server,
                                 const UA_NodeId *sessionId, void *sessionContext,
                                 const UA_NodeId *nodeId, void *nodeContext,
                                 const UA_NumericRange *range, const UA_DataValue *data) {
    UA_QualifiedName fieldLastSeverity = UA_QUALIFIEDNAME(0, CONDITION_FIELD_LASTSEVERITY);
    UA_QualifiedName fieldSourceTimeStamp =
        UA_QUALIFIEDNAME(0, CONDITION_FIELD_CONDITIONVARIABLE_SOURCETIMESTAMP);
    UA_Variant value;

    UA_NodeId condition;
    UA_StatusCode retval = getFieldParentNodeId(server, nodeId, &condition);
    CONDITION_ASSERT_RETURN_VOID(retval, "No Parent Condition found for given Severity Field",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &condition, fieldSourceQN, &conditionSource);
    CONDITION_ASSERT_RETURN_VOID(retval, "ConditionSource not found",
                                 UA_NodeId_clear(&condition););

    UA_UInt16 lastSeverity;
    UA_DateTime lastSeveritySourceTimeStamp;
    retval = getConditionLastSeverity(server, &conditionSource, &condition,
                                      &lastSeverity, &lastSeveritySourceTimeStamp);
    CONDITION_ASSERT_RETURN_VOID(retval, "Get Condition LastSeverity failed",
                                 UA_NodeId_clear(&condition);
                                 UA_NodeId_clear(&conditionSource););

    /* Set message dependent on compare result*/
    UA_LocalizedText message;
    if(lastSeverity < (*(UA_UInt16 *)data->value.data))
        message = UA_LOCALIZEDTEXT(LOCALE, SEVERITY_INCREASED_MESSAGE);
    else
        message = UA_LOCALIZEDTEXT(LOCALE, SEVERITY_DECREASED_MESSAGE);

    /* Set LastSeverity */
    UA_Variant_setScalar(&value, &lastSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Server_setConditionField(server, condition, &value, fieldLastSeverity);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition LAstSeverity failed",
                                 UA_NodeId_clear(&condition);
                                 UA_NodeId_clear(&conditionSource););

    /* Set SourceTimestamp */
    UA_Variant_setScalar(&value, &lastSeveritySourceTimeStamp, &UA_TYPES[UA_TYPES_DATETIME]);
    retval = UA_Server_setConditionVariableFieldProperty(server, condition, &value,
                                                         fieldLastSeverity, fieldSourceTimeStamp);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set LastSeverity SourceTimestamp failed",
                                 UA_NodeId_clear(&condition);
                                 UA_NodeId_clear(&conditionSource););

    /* Update lastSeverity in list */
    lastSeverity = *(UA_UInt16 *)data->value.data;
    lastSeveritySourceTimeStamp = data->sourceTimestamp;
    retval = updateConditionLastSeverity(server, &conditionSource, &condition,
                                         lastSeverity, lastSeveritySourceTimeStamp);
    CONDITION_ASSERT_RETURN_VOID(retval, "Update Condition LastSeverity failed",
                                 UA_NodeId_clear(&condition);
    UA_NodeId_clear(&conditionSource););

    /* Set Time (Time of Value Change) */
    UA_Variant_setScalar(&value, (void*)(uintptr_t)((const UA_DateTime*)&data->sourceTimestamp),
                         &UA_TYPES[UA_TYPES_DATETIME]);
    retval = UA_Server_setConditionField(server, condition, &value, fieldTimeQN);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition Time failed",
                                 UA_NodeId_clear(&condition););

    /* Set Message */
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, condition, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_VOID(retval, "Set Condition Message failed",
                                 UA_NodeId_clear(&condition););

#ifdef CONDITION_SEVERITYCHANGECALLBACK_ENABLE
    /* Check if retained */
    if(isRetained(server, &condition)) {
        /* Trigger event */
        //Condition Nodes should not be deleted after triggering the event
        retval = UA_Server_triggerConditionEvent(server, condition, conditionSource, NULL);
        CONDITION_ASSERT_RETURN_VOID(retval, "Triggering condition event failed",
                                     UA_NodeId_clear(&condition);
        UA_NodeId_clear(&conditionSource););
    }
#endif//CONDITION_SEVERITYCHANGECALLBACK_ENABLE
    UA_NodeId_clear(&conditionSource);
    UA_NodeId_clear(&condition);
}

static UA_StatusCode
disableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Check if enabled */
    if(!isTwoStateVariableInTrueState(server, objectId, &fieldEnabledStateQN))
        return UA_STATUSCODE_BADCONDITIONALREADYDISABLED;

    /* Disable by writing false to EnabledState/Id--> will cause a callback to
     * trigger the new events */
    UA_Variant value;
    UA_Boolean idValue = false;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = UA_Server_setConditionVariableFieldProperty(
        server, *objectId, &value, fieldEnabledStateQN, twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Disable Condition failed",);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
enableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize,
                     const UA_Variant *input, size_t outputSize,
                     UA_Variant *output) {
    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Check if disabled */
    if(isTwoStateVariableInTrueState(server, objectId, &fieldEnabledStateQN))
        return UA_STATUSCODE_BADCONDITIONALREADYENABLED;

    /* Enable by writing true to EnabledStateId--> will cause a callback to
     * trigger the new events */
    UA_Variant value;
    UA_Boolean idValue = true;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval =
        UA_Server_setConditionVariableFieldProperty(server, *objectId, &value,
                                                    fieldEnabledStateQN, twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Enable Condition failed",);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addCommentMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *methodId,
                         void *methodContext, const UA_NodeId *objectId,
                         void *objectContext, size_t inputSize,
                         const UA_Variant *input, size_t outputSize,
                         UA_Variant *output) {
    UA_QualifiedName fieldComment = UA_QUALIFIEDNAME(0, CONDITION_FIELD_COMMENT);
    UA_QualifiedName fieldSourceTimeStamp =
        UA_QUALIFIEDNAME(0, CONDITION_FIELD_CONDITIONVARIABLE_SOURCETIMESTAMP);
    UA_LocalizedText message;
    UA_NodeId triggerEvent;
    UA_Variant value;
    UA_DateTime fieldSourceTimeStampValue = UA_DateTime_now();

    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Get condition branch to trigger the correct event.
     * see error code Bad_NodeIdInvalid Table 16 p.22. It should be returned when the
     * Method called is from the ConditionType and not its instance, however
     * in current implementation, methods are only being referenced from their ObjectType Node.
     * Because of that, the correct instance (Condition) will be found through
     * its last EventId */
    UA_StatusCode retval = getConditionBranchNodeId(server, (UA_ByteString *)input[0].data,
                                                    &triggerEvent);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "ConditionId based on EventId not found",);

    /* Check if enabled */
    if(!isRetained(server, &triggerEvent))
        return UA_STATUSCODE_BADCONDITIONDISABLED;

    /* Set SourceTimestamp */
    UA_Variant_setScalar(&value, &fieldSourceTimeStampValue, &UA_TYPES[UA_TYPES_DATETIME]);
    retval = UA_Server_setConditionVariableFieldProperty(server, triggerEvent, &value,
                                                         fieldComment, fieldSourceTimeStamp);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition EnabledState text failed",
                                   UA_NodeId_clear(&triggerEvent););

    /* Set adding comment time (the same value of SourceTimestamp) */
    retval = UA_Server_writeObjectProperty_scalar(server, triggerEvent, fieldTimeQN,
                                                  &fieldSourceTimeStampValue,
                                                  &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set enabling/disabling Time failed",
                                   UA_NodeId_clear(&triggerEvent););

    /* Set Message */
    message = UA_LOCALIZEDTEXT(LOCALE, COMMENT_MESSAGE);
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, triggerEvent, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Message failed",
                                   UA_NodeId_clear(&triggerEvent););

    /* Set Comment. Check whether comment is empty -> leave the last value as is*/
    UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
    UA_String nullString = UA_STRING_NULL;
    if(!UA_ByteString_equal(&inputComment->locale, &nullString) &&
       !UA_ByteString_equal(&inputComment->text, &nullString)) {
        UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, triggerEvent, &value, fieldComment);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Comment failed",
                                       UA_NodeId_clear(&triggerEvent););
    }

    /* Get conditionSource */
    UA_NodeId conditionSource;
    retval = getNodeIdValueOfConditionField(server, &triggerEvent, fieldSourceQN, &conditionSource);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "ConditionSource not found",
                                   UA_NodeId_clear(&triggerEvent););

    /* Trigger event */
    //Condition Nodes should not be deleted after triggering the event
    retval = UA_Server_triggerConditionEvent(server, triggerEvent, conditionSource, NULL);
    UA_NodeId_clear(&conditionSource);
    UA_NodeId_clear(&triggerEvent);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Triggering condition event failed",);
    return retval;
}

static UA_StatusCode
acknowledgeMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output) {
    UA_QualifiedName fieldComment = UA_QUALIFIEDNAME(0, CONDITION_FIELD_COMMENT);
    UA_Variant value;

    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Get condition branch to trigger the correct event */
    UA_NodeId conditionNode;
    UA_StatusCode retval = getConditionBranchNodeId(server, (UA_ByteString *)input[0].data,
                                                    &conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "ConditionId based on EventId not found",);

    /* Check if retained */
    if(!isRetained(server, &conditionNode))
        return UA_STATUSCODE_BADCONDITIONDISABLED;

    /* Check if already acknowledged */
    if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldAckedStateQN))
        return UA_STATUSCODE_BADCONDITIONBRANCHALREADYACKED;

    /* Get EventType */
    UA_NodeId eventType;
    retval = getNodeIdValueOfConditionField(server, &conditionNode,
                                            UA_QUALIFIEDNAME(0, CONDITION_FIELD_EVENTTYPE),
                                            &eventType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "EventType not found",
                                   UA_NodeId_clear(&conditionNode););

    /* Check if ConditionType is subType of AcknowledgeableConditionType TODO Over Kill*/
    UA_NodeId AcknowledgeableConditionTypeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
    if(!isNodeInTree_singleRef(server, &eventType, &AcknowledgeableConditionTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Condition Type must be a subtype of AcknowledgeableConditionType!");
        UA_NodeId_clear(&conditionNode);
        UA_NodeId_clear(&eventType);
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    UA_NodeId_clear(&eventType);

    /* Set Comment. Check whether comment is empty -> leave the last value as is*/
    UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
    UA_String nullString = UA_STRING_NULL;
    if(!UA_ByteString_equal(&inputComment->locale, &nullString) &&
       !UA_ByteString_equal(&inputComment->text, &nullString)) {
        UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, conditionNode, &value, fieldComment);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Comment failed",
                                       UA_NodeId_clear(&conditionNode););
    }

    /* Set AcknowledgeableStateId */
    UA_Boolean idValue = true;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode, &value,
                                                         fieldAckedStateQN, twoStateVariableIdQN);
    UA_NodeId_clear(&conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Acknowledge Condition failed",);
    return retval;
}

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
static UA_StatusCode
confirmMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    UA_QualifiedName fieldComment = UA_QUALIFIEDNAME(0, CONDITION_FIELD_COMMENT);
    UA_Variant value;

    UA_NodeId conditionTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot call method of ConditionType Node. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Get condition branch to trigger the correct event */
    UA_NodeId conditionNode;
    UA_StatusCode retval = getConditionBranchNodeId(server, (UA_ByteString *)input[0].data,
                                                    &conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "ConditionId based on EventId not found",);

    /* Check if retained */
    if(!isRetained(server, &conditionNode))
        return UA_STATUSCODE_BADCONDITIONDISABLED;

    /* Check if already confirmed */
    if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldConfirmedStateQN))
        return UA_STATUSCODE_BADCONDITIONBRANCHALREADYCONFIRMED;

    /* Get EventType */
    UA_NodeId eventType;
    retval = getNodeIdValueOfConditionField(server, &conditionNode,
                                            UA_QUALIFIEDNAME(0, CONDITION_FIELD_EVENTTYPE),
                                            &eventType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "EventType not found",
                                   UA_NodeId_clear(&conditionNode););

    /* Check if ConditionType is subType of AcknowledgeableConditionType. */
    UA_NodeId AcknowledgeableConditionTypeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
    if(!isNodeInTree_singleRef(server, &eventType, &AcknowledgeableConditionTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Condition Type must be a subtype of AcknowledgeableConditionType!");
        UA_NodeId_clear(&conditionNode);
        UA_NodeId_clear(&eventType);
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    UA_NodeId_clear(&eventType);

    /* Set Comment. Check whether comment is empty -> leave the last value as is*/
    UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
    UA_String nullString = UA_STRING_NULL;
    if(!UA_ByteString_equal(&inputComment->locale, &nullString) &&
       !UA_ByteString_equal(&inputComment->text, &nullString)) {
        UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, conditionNode, &value, fieldComment);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Comment failed",
                                       UA_NodeId_clear(&conditionNode););
    }

    /* Set ConfirmedStateId */
    UA_Boolean idValue = true;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(server, conditionNode, &value,
                                                         fieldConfirmedStateQN,
                                                         twoStateVariableIdQN);
    UA_NodeId_clear(&conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Acknowledge Condition failed",);
    return retval;
}
#endif//CONDITIONOPTIONALFIELDS_SUPPORT

static UA_StatusCode
setRefreshMethodEventFields(UA_Server *server, const UA_NodeId *refreshEventNodId) {
    UA_QualifiedName fieldSeverity = UA_QUALIFIEDNAME(0, CONDITION_FIELD_SEVERITY);
    UA_QualifiedName fieldSourceName = UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENAME);
    UA_QualifiedName fieldReceiveTime = UA_QUALIFIEDNAME(0, CONDITION_FIELD_RECEIVETIME);
    UA_QualifiedName fieldEventId = UA_QUALIFIEDNAME(0, CONDITION_FIELD_EVENTID);
    UA_String sourceNameString = UA_STRING("Server"); //server is the source of Refresh Events
    UA_UInt16 severityValue = REFRESHEVENT_SEVERITY_DEFAULT;
    UA_ByteString eventId  = UA_BYTESTRING_NULL;
    UA_Variant value;

    /* Set Severity */
    UA_Variant_setScalar(&value, &severityValue, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_Server_setConditionField(server, *refreshEventNodId, &value,
                                                       fieldSeverity);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set RefreshEvent Severity failed",);

    /* Set SourceName */
    UA_Variant_setScalar(&value, &sourceNameString, &UA_TYPES[UA_TYPES_STRING]);
    retval = UA_Server_setConditionField(server, *refreshEventNodId, &value, fieldSourceName);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set RefreshEvent Source failed",);

    /* Set ReceiveTime */
    UA_DateTime fieldReceiveTimeValue = UA_DateTime_now();
    UA_Variant_setScalar(&value, &fieldReceiveTimeValue, &UA_TYPES[UA_TYPES_DATETIME]);
    retval = UA_Server_setConditionField(server, *refreshEventNodId, &value, fieldReceiveTime);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set RefreshEvent ReceiveTime failed",);

    /* Set EventId */
    retval = UA_Event_generateEventId(&eventId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Generating EventId failed",);

    UA_Variant_setScalar(&value, &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    retval = UA_Server_setConditionField(server, *refreshEventNodId, &value, fieldEventId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set RefreshEvent EventId failed",);

    UA_ByteString_clear(&eventId);

    return retval;
}

static UA_StatusCode
createRefreshMethodEvents(UA_Server *server, UA_NodeId *outRefreshStartNodId,
                          UA_NodeId *outRefreshEndNodId) {
    UA_NodeId refreshStartEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHSTARTEVENTTYPE);
    UA_NodeId refreshEndEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHENDEVENTTYPE);
    /* create RefreshStartEvent */
    UA_StatusCode retval = UA_Server_createEvent(server, refreshStartEventTypeNodeId,
                                                 outRefreshStartNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "CreateEvent RefreshStart failed",);

    /* create RefreshEndEvent */
    retval = UA_Server_createEvent(server, refreshEndEventTypeNodeId, outRefreshEndNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "CreateEvent RefreshEnd failed",);

    return retval;
}

static UA_StatusCode
setRefreshMethodEvents(UA_Server *server, const UA_NodeId *refreshStartNodId,
                       const UA_NodeId *refreshEndNodId) {
    /* Set Standard Fields for RefreshStart */
    UA_StatusCode retval = setRefreshMethodEventFields(server, refreshStartNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set standard Fields of RefreshStartEvent failed",);

    /* Set Standard Fields for RefreshEnd*/
    retval = setRefreshMethodEventFields(server, refreshEndNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set standard Fields of RefreshEndEvent failed",);

    return retval;
}

static UA_Boolean
isConditionSourceInMonitoredItem(UA_Server *server, const UA_MonitoredItem *monitoredItem,
                                 const UA_NodeId *conditionSource){
    /* TODO: check also other hierarchical references */
    UA_ReferenceTypeSet refs = UA_REFTYPESET(UA_REFERENCETYPEINDEX_ORGANIZES);
    refs = UA_ReferenceTypeSet_union(refs, UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASCOMPONENT));
    refs = UA_ReferenceTypeSet_union(refs, UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASEVENTSOURCE));
    refs = UA_ReferenceTypeSet_union(refs, UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASNOTIFIER));
    return isNodeInTree(server, conditionSource,
                        &monitoredItem->itemToMonitor.nodeId, &refs);
}

static UA_StatusCode
refreshLogic(UA_Server *server, const UA_NodeId *refreshStartNodId,
             const UA_NodeId *refreshEndNodId, UA_MonitoredItem *monitoredItem) {
    UA_assert(monitoredItem != NULL);

    /* 1. Trigger RefreshStartEvent */
    UA_DateTime fieldTimeValue = UA_DateTime_now();
    UA_StatusCode retval =
        UA_Server_writeObjectProperty_scalar(server, *refreshStartNodId, fieldTimeQN,
                                             &fieldTimeValue, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Write Object Property scalar failed",);

    retval = UA_Event_addEventToMonitoredItem(server, refreshStartNodId, monitoredItem);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Events: Could not add the event to a listening node",);

    /* 2. Refresh (see 5.5.7) */
    /* Get ConditionSource Entry */
    UA_ConditionSource *source;
    LIST_FOREACH(source, &server->conditionSources, listEntry) {
        UA_NodeId conditionSource = source->conditionSourceId;
        UA_NodeId serverObjectNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
        /* Check if the conditionSource is being monitored. If the Server Object
         * is being monitored, then all Events of all monitoredItems should be
         * refreshed */
        if(!UA_NodeId_equal(&monitoredItem->itemToMonitor.nodeId, &conditionSource) &&
           !UA_NodeId_equal(&monitoredItem->itemToMonitor.nodeId, &serverObjectNodeId) &&
           !isConditionSourceInMonitoredItem(server, monitoredItem, &conditionSource))
            continue;

        /* Get Condition Entry */
        UA_Condition *cond;
        LIST_FOREACH(cond, &source->conditions, listEntry) {
            /* Get Branch Entry */
            UA_ConditionBranch *branch;
            LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
                /* If no event was triggered for that branch, then check next
                 * without refreshing */
                if(UA_ByteString_equal(&branch->lastEventId, &UA_BYTESTRING_NULL))
                    continue;

                UA_NodeId triggeredNode;
                if(UA_NodeId_isNull(&branch->conditionBranchId))
                    triggeredNode = cond->conditionId;
                else
                    triggeredNode = branch->conditionBranchId;

                /* Check if Retain is set to true */
                if(!isRetained(server, &triggeredNode))
                    continue;

                /* Add the event */
                retval = UA_Event_addEventToMonitoredItem(server, &triggeredNode, monitoredItem);
                CONDITION_ASSERT_RETURN_RETVAL(retval, "Events: Could not add the event to a listening node",);
            }
        }
    }

    /* 3. Trigger RefreshEndEvent */
    fieldTimeValue = UA_DateTime_now();
    retval = UA_Server_writeObjectProperty_scalar(server, *refreshEndNodId, fieldTimeQN,
                                                  &fieldTimeValue, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Write Object Property scalar failed",);
    return UA_Event_addEventToMonitoredItem(server, refreshEndNodId, monitoredItem);
}

static UA_StatusCode
refresh2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    //TODO implement logic for subscription array
    /* Check if valid subscriptionId */
    UA_Session *session = UA_Server_getSessionById(server, sessionId);
    UA_Subscription *subscription =
        UA_Session_getSubscriptionById(session, *((UA_UInt32 *)input[0].data));
    if(!subscription)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    /* set RefreshStartEvent and RefreshEndEvent */
    UA_StatusCode retval = setRefreshMethodEvents(server,
                                                  &refreshEvents[REFRESHEVENT_START_IDX],
                                                  &refreshEvents[REFRESHEVENT_END_IDX]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Create Event RefreshStart or RefreshEnd failed",);

    /* Trigger RefreshStartEvent and RefreshEndEvent for the each monitoredItem
     * in the subscription */
    UA_MonitoredItem *monitoredItem =
        UA_Subscription_getMonitoredItem(subscription, *((UA_UInt32 *)input[1].data));
    if(!monitoredItem)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;

    //TODO when there are a lot of monitoreditems (not only events)?
    retval = refreshLogic(server, &refreshEvents[REFRESHEVENT_START_IDX],
                          &refreshEvents[REFRESHEVENT_END_IDX], monitoredItem);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Could not refresh Condition",);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
refreshMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    //TODO implement logic for subscription array
    /* Check if valid subscriptionId */
    UA_Session *session = UA_Server_getSessionById(server, sessionId);
    UA_Subscription *subscription =
        UA_Session_getSubscriptionById(session, *((UA_UInt32 *)input[0].data));
    if(!subscription)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;

    /* set RefreshStartEvent and RefreshEndEvent */
    UA_StatusCode retval =
        setRefreshMethodEvents(server, &refreshEvents[REFRESHEVENT_START_IDX],
                               &refreshEvents[REFRESHEVENT_END_IDX]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Create Event RefreshStart or RefreshEnd failed",);

    /* Trigger RefreshStartEvent and RefreshEndEvent for the each monitoredItem
     * in the subscription */
    //TODO when there are a lot of monitoreditems (not only events)?
    UA_MonitoredItem *monitoredItem = NULL;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        retval = refreshLogic(server, &refreshEvents[REFRESHEVENT_START_IDX],
                              &refreshEvents[REFRESHEVENT_END_IDX], monitoredItem);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Could not refresh Condition",);
    }
    return UA_STATUSCODE_GOOD;
}

/*****************************************************************************/
/* Functions                                                                 */
/*****************************************************************************/

static UA_StatusCode
setConditionInConditionList(UA_Server *server, const UA_NodeId *conditionNodeId,
                            UA_ConditionSource *conditionSourceEntry) {
    UA_Condition *conditionListEntry = (UA_Condition*)UA_malloc(sizeof(UA_Condition));
    if(!conditionListEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(conditionListEntry, 0, sizeof(UA_Condition));

    /* Set ConditionId with given ConditionNodeId */
    UA_StatusCode retval = UA_NodeId_copy(conditionNodeId, &conditionListEntry->conditionId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(conditionListEntry);
        return retval;
    }

    UA_ConditionBranch *conditionBranchListEntry;
    conditionBranchListEntry = (UA_ConditionBranch*)UA_malloc(sizeof(UA_ConditionBranch));
    if(!conditionBranchListEntry) {
        UA_free(conditionListEntry);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }

    memset(conditionBranchListEntry, 0, sizeof(UA_ConditionBranch));
    LIST_INSERT_HEAD(&conditionSourceEntry->conditions, conditionListEntry, listEntry);
    LIST_INSERT_HEAD(&conditionListEntry->conditionBranches, conditionBranchListEntry, listEntry);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
appendConditionEntry(UA_Server *server, const UA_NodeId *conditionNodeId,
                     const UA_NodeId *conditionSourceNodeId) {
    /* See if the ConditionSource Entry already exists*/
    UA_ConditionSource *source = getConditionSource(server, conditionSourceNodeId);
    if(source)
        return setConditionInConditionList(server, conditionNodeId, source);

    /* ConditionSource not found in list, so we create a new ConditionSource Entry */
    UA_ConditionSource *conditionSourceListEntry;
    conditionSourceListEntry = (UA_ConditionSource*)UA_malloc(sizeof(UA_ConditionSource));
    if(!conditionSourceListEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(conditionSourceListEntry, 0, sizeof(UA_ConditionSource));

    /* Set ConditionSourceId with given ConditionSourceNodeId */
    UA_StatusCode retval = UA_NodeId_copy(conditionSourceNodeId,
                                          &conditionSourceListEntry->conditionSourceId);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_free(conditionSourceListEntry);
        return retval;
    }

    LIST_INSERT_HEAD(&server->conditionSources, conditionSourceListEntry, listEntry);
    return setConditionInConditionList(server, conditionNodeId, conditionSourceListEntry);
}

static void
deleteAllBranchesFromCondition(UA_Condition *cond) {
    UA_ConditionBranch *branch, *tmp_branch;
    LIST_FOREACH_SAFE(branch, &cond->conditionBranches, listEntry, tmp_branch) {
        UA_NodeId_clear(&branch->conditionBranchId);
        UA_ByteString_clear(&branch->lastEventId);
        LIST_REMOVE(branch, listEntry);
        UA_free(branch);
    }
}

static void
deleteCondition(UA_Condition *cond) {
    deleteAllBranchesFromCondition(cond);
    UA_NodeId_clear(&cond->conditionId);
    LIST_REMOVE(cond, listEntry);
    UA_free(cond);
}

void
UA_ConditionList_delete(UA_Server *server) {
    UA_ConditionSource *source, *tmp_source;
    LIST_FOREACH_SAFE(source, &server->conditionSources, listEntry, tmp_source) {
        UA_Condition *cond, *tmp_cond;
        LIST_FOREACH_SAFE(cond, &source->conditions, listEntry, tmp_cond) {
            deleteCondition(cond);
        }
        UA_NodeId_clear(&source->conditionSourceId);
        LIST_REMOVE(source, listEntry);
        UA_free(source);
    }
    /* Free memory allocated for RefreshEvents NodeIds */
    UA_NodeId_clear(&refreshEvents[REFRESHEVENT_START_IDX]);
    UA_NodeId_clear(&refreshEvents[REFRESHEVENT_END_IDX]);
}

/* Get the ConditionId based on the EventId (all branches of one condition
 * should have the same ConditionId) */
UA_StatusCode
UA_getConditionId(UA_Server *server, const UA_NodeId *conditionNodeId,
                  UA_NodeId *outConditionId) {
    /* Get ConditionSource Entry */
    UA_ConditionSource *source;
    LIST_FOREACH(source, &server->conditionSources, listEntry) {
        /* Get Condition Entry */
        UA_Condition *cond;
        LIST_FOREACH(cond, &source->conditions, listEntry) {
            if(UA_NodeId_equal(&cond->conditionId, conditionNodeId)) {
                *outConditionId = cond->conditionId;
                return UA_STATUSCODE_GOOD;
            }
            /* Get Branch Entry*/
            UA_ConditionBranch *branch;
            LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
                if(UA_NodeId_equal(&branch->conditionBranchId, conditionNodeId)) {
                    *outConditionId = cond->conditionId;
                    return UA_STATUSCODE_GOOD;
                }
            }
        }
    }
    return UA_STATUSCODE_BADNOTFOUND;
}

/* Check whether the Condition Source Node has "EventSource" or one of its
 * subtypes inverse reference. */
static UA_Boolean
doesHasEventSourceReferenceExist(UA_Server *server, const UA_NodeId nodeToCheck) {
    UA_NodeId hasEventSourceId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE);
    const UA_Node* node = UA_NODESTORE_GET(server, &nodeToCheck);
    if(!node)
        return false;
    for(size_t i = 0; i < node->head.referencesSize; i++) {
        UA_Byte refTypeIndex = node->head.references[i].referenceTypeIndex;
        if((refTypeIndex == UA_REFERENCETYPEINDEX_HASEVENTSOURCE ||
            isNodeInTree_singleRef(server, UA_NODESTORE_GETREFERENCETYPEID(server, refTypeIndex),
                                   &hasEventSourceId, UA_REFERENCETYPEINDEX_HASSUBTYPE)) &&
           (node->head.references[i].isInverse == true)) {
            UA_NODESTORE_RELEASE(server, node);
            return true;
        }
    }
    UA_NODESTORE_RELEASE(server, node);
    return false;
}

static UA_StatusCode
setStandardConditionFields(UA_Server *server, const UA_NodeId* condition,
                           const UA_NodeId* conditionType, const UA_NodeId* conditionSource,
                           const UA_QualifiedName* conditionName) {
    /* Set Fields */
    /* 1.Set EventType */
    UA_Variant value;
    UA_Variant_setScalar(&value, (void*)(uintptr_t)conditionType, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode retval = UA_Server_setConditionField(server, *condition, &value,
                                                       UA_QUALIFIEDNAME(0,CONDITION_FIELD_EVENTTYPE));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EventType Field failed",);

    /* 2.Set ConditionName */
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&conditionName->name,
                         &UA_TYPES[UA_TYPES_STRING]);
    retval = UA_Server_setConditionField(server, *condition, &value,
                                         UA_QUALIFIEDNAME(0,CONDITION_FIELD_CONDITIONNAME));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ConditionName Field failed",);

    /* 3.Set EnabledState (Disabled by default -> Retain Field = false) */
    UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, DISABLED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, *condition, &value,
                                         UA_QUALIFIEDNAME(0,CONDITION_FIELD_ENABLEDSTATE));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState Field failed",);

    /* 4.Set EnabledState/Id */
    UA_Boolean stateId = false;
    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(server, *condition, &value,
                                               UA_QUALIFIEDNAME(0,CONDITION_FIELD_ENABLEDSTATE),
                                                         twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState/Id Field failed",);

    /* 5.Set Retain*/
    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionField(server, *condition, &value, fieldRetainQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Retain Field failed",);

    /* Get ConditionSourceNode*/
    const UA_Node *conditionSourceNode = UA_NODESTORE_GET(server, conditionSource);
    if(!conditionSourceNode) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Couldn't find ConditionSourceNode. StatusCode %s", UA_StatusCode_name(retval));
        return UA_STATUSCODE_BADNOTFOUND;
    }

    /* 6.Set SourceName*/
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&conditionSourceNode->head.browseName.name,
                         &UA_TYPES[UA_TYPES_STRING]);
    retval = UA_Server_setConditionField(server, *condition, &value,
                                         UA_QUALIFIEDNAME(0,CONDITION_FIELD_SOURCENAME));
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Set SourceName Field failed. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_NODESTORE_RELEASE(server, conditionSourceNode);
        return retval;
    }

    /* 7.Set SourceNode*/
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&conditionSourceNode->head.nodeId,
                         &UA_TYPES[UA_TYPES_NODEID]);
    retval = UA_Server_setConditionField(server, *condition, &value, fieldSourceQN);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Set SourceNode Field failed. StatusCode %s", UA_StatusCode_name(retval));
        UA_NODESTORE_RELEASE(server, conditionSourceNode);
        return retval;
    }

    UA_NODESTORE_RELEASE(server, conditionSourceNode);

    /* 8. Set Quality (TODO not supported, thus set with Status Good) */
    UA_StatusCode qualityValue = UA_STATUSCODE_GOOD;
    UA_Variant_setScalar(&value, &qualityValue, &UA_TYPES[UA_TYPES_STATUSCODE]);
    retval = UA_Server_setConditionField(server, *condition, &value,
                                         UA_QUALIFIEDNAME(0,CONDITION_FIELD_QUALITY));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Quality Field failed",);

    /* 9. Set Severity */
    UA_UInt16 severityValue = 0;
    UA_Variant_setScalar(&value, &severityValue, &UA_TYPES[UA_TYPES_UINT16]);
    retval = UA_Server_setConditionField(server, *condition, &value,
                                         UA_QUALIFIEDNAME(0,CONDITION_FIELD_SEVERITY));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Severity Field failed",);

    /* Check subTypes of ConditionType to set further Fields*/

    /* 1. Check if ConditionType is subType of AcknowledgeableConditionType */
    UA_NodeId acknowledgeableConditionTypeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
    if(!isNodeInTree_singleRef(server, conditionType, &acknowledgeableConditionTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE))
        return UA_STATUSCODE_GOOD;

    /* Set AckedState (Id = false by default) */
    text = UA_LOCALIZEDTEXT(LOCALE, UNACKED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, *condition, &value, fieldAckedStateQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set AckedState Field failed",);

    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(server, *condition, &value,
                                                         fieldAckedStateQN, twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set AckedState/Id Field failed",);

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
    /* add optional field ConfirmedState*/
    retval = UA_Server_addConditionOptionalField(server, *condition, acknowledgeableConditionTypeId,
                                                 fieldConfirmedStateQN, NULL);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding ConfirmedState optional Field failed",);

    /* Set ConfirmedState (Id = false by default) */
    text = UA_LOCALIZEDTEXT(LOCALE, UNCONFIRMED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = UA_Server_setConditionField(server, *condition, &value, fieldConfirmedStateQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ConfirmedState Field failed",);

    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    retval = UA_Server_setConditionVariableFieldProperty(server, *condition, &value,
                                                         fieldConfirmedStateQN,
                                                         twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState/Id Field failed",);
#endif//CONDITIONOPTIONALFIELDS_SUPPORT

    /* 2. Check if ConditionType is subType of AlarmConditionType */
    UA_NodeId alarmConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE);
    if(isNodeInTree_singleRef(server, conditionType, &alarmConditionTypeId,
                              UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        /* Set ActiveState (Id = false by default) */
        text = UA_LOCALIZEDTEXT(LOCALE, INACTIVE_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        retval = UA_Server_setConditionField(server, *condition, &value,
                                             UA_QUALIFIEDNAME(0,CONDITION_FIELD_ACTIVESTATE));
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ActiveState Field failed",);
    }

    return retval;
}

/* Set callbacks for TwoStateVariable Fields of a condition */
static UA_StatusCode
setTwoStateVariableCallbacks(UA_Server *server, const UA_NodeId* condition,
                             const UA_NodeId* conditionType) {
    /* Set EnabledState Callback */
    UA_NodeId twoStateVariableIdNodeId = UA_NODEID_NULL;
    UA_StatusCode retval = getConditionFieldPropertyNodeId(server, condition, &fieldEnabledStateQN,
                                                           &twoStateVariableIdQN,
                                                           &twoStateVariableIdNodeId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Id Property of TwoStateVariable not found",);

    UA_ValueCallback callback;
    callback.onRead = NULL;
    callback.onWrite = afterWriteCallbackEnabledStateChange;
    retval = UA_Server_setVariableNode_valueCallback(server, twoStateVariableIdNodeId, callback);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EnabledState Callback failed",
                                   UA_NodeId_clear(&twoStateVariableIdNodeId););

    /* Set AckedState Callback */
    /* Check if ConditionType is subType of AcknowledgeableConditionType */
    UA_NodeId acknowledgeableConditionTypeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
    if(isNodeInTree_singleRef(server, conditionType, &acknowledgeableConditionTypeId,
                              UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_NodeId_clear(&twoStateVariableIdNodeId);
        retval = getConditionFieldPropertyNodeId(server, condition, &fieldAckedStateQN,
                                                 &twoStateVariableIdQN, &twoStateVariableIdNodeId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Id Property of TwoStateVariable not found",);

        callback.onWrite = afterWriteCallbackAckedStateChange;
        retval = UA_Server_setVariableNode_valueCallback(server, twoStateVariableIdNodeId, callback);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set AckedState Callback failed",
                                       UA_NodeId_clear(&twoStateVariableIdNodeId););

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
        /* add callback */
        callback.onWrite = afterWriteCallbackConfirmedStateChange;
        UA_NodeId_clear(&twoStateVariableIdNodeId);
        retval = getConditionFieldPropertyNodeId(server, condition, &fieldConfirmedStateQN,
                                                 &twoStateVariableIdQN, &twoStateVariableIdNodeId);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Id Property of TwoStateVariable not found",);

        /* add reference from Condition to Confirm Method */
        retval = UA_Server_addReference(server, *condition, UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                     UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM), true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to Confirm Method failed",
                                       UA_NodeId_clear(&twoStateVariableIdNodeId););

        retval = UA_Server_setVariableNode_valueCallback(server, twoStateVariableIdNodeId, callback);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding ConfirmedState/Id callback failed",
                                       UA_NodeId_clear(&twoStateVariableIdNodeId););
#endif//CONDITIONOPTIONALFIELDS_SUPPORT

        /* Set ActiveState Callback */
        /* Check if ConditionType is subType of AlarmConditionType */
        UA_NodeId alarmConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE);
        if(isNodeInTree_singleRef(server, conditionType, &alarmConditionTypeId,
                                  UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
            UA_NodeId_clear(&twoStateVariableIdNodeId);
            retval = getConditionFieldPropertyNodeId(server, condition, &fieldActiveStateQN,
                                                     &twoStateVariableIdQN, &twoStateVariableIdNodeId);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Id Property of TwoStateVariable not found",);

            callback.onWrite = afterWriteCallbackActiveStateChange;
            retval = UA_Server_setVariableNode_valueCallback(server, twoStateVariableIdNodeId,
                                                             callback);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ActiveState Callback failed",
                                           UA_NodeId_clear(&twoStateVariableIdNodeId););
        }
    }

    UA_NodeId_clear(&twoStateVariableIdNodeId);
    return retval;
}

/* Set callbacks for ConditionVariable Fields of a condition */
static UA_StatusCode
setConditionVariableCallbacks(UA_Server *server, const UA_NodeId *condition,
                              const UA_NodeId *conditionType) {
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_QualifiedName conditionVariableName[2] = {
        UA_QUALIFIEDNAME(0, CONDITION_FIELD_QUALITY),
        UA_QUALIFIEDNAME(0, CONDITION_FIELD_SEVERITY)
    };// extend array with other fields when needed

    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, *condition, 1, &conditionVariableName[0]);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;
    UA_ValueCallback callback ;
    callback.onRead = NULL;
    callback.onWrite = afterWriteCallbackQualityChange;
    retval = UA_Server_setVariableNode_valueCallback(server, bpr.targets[0].targetId.nodeId,
                                                     callback);
    UA_BrowsePathResult_clear(&bpr);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    bpr = UA_Server_browseSimplifiedBrowsePath(server, *condition, 1, &conditionVariableName[1]);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;
    callback.onWrite = afterWriteCallbackSeverityChange;
    retval = UA_Server_setVariableNode_valueCallback(server, bpr.targets[0].targetId.nodeId,
                                                     callback);
    UA_BrowsePathResult_clear(&bpr);
    return retval;
}

/* Set callbacks for Method Fields of a condition. The current implementation
 * references methods without copying them when creating objects. So the
 * callbacks will be attached to the methods of the conditionType. */
static UA_StatusCode
setConditionMethodCallbacks(UA_Server *server, const UA_NodeId* condition,
                            const UA_NodeId* conditionType) {
    UA_NodeId methodId[7] = {
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_DISABLE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_ENABLE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_ADDCOMMENT}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_ACKNOWLEDGE}}
#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
        ,{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM}}
#endif
    };

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= UA_Server_setMethodNodeCallback(server, methodId[0], disableMethodCallback);
    retval |= UA_Server_setMethodNodeCallback(server, methodId[1], enableMethodCallback);
    retval |= UA_Server_setMethodNodeCallback(server, methodId[2], addCommentMethodCallback);
    retval |= UA_Server_setMethodNodeCallback(server, methodId[3], refreshMethodCallback);
    retval |= UA_Server_setMethodNodeCallback(server, methodId[4], refresh2MethodCallback);
    retval |= UA_Server_setMethodNodeCallback(server, methodId[5], acknowledgeMethodCallback);
#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
    retval |= UA_Server_setMethodNodeCallback(server, methodId[6], confirmMethodCallback);
#endif

    return retval;
}

static UA_StatusCode
setStandardConditionCallbacks(UA_Server *server, const UA_NodeId* condition,
                              const UA_NodeId* conditionType) {
    UA_StatusCode retval = setTwoStateVariableCallbacks(server, condition, conditionType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set TwoStateVariable Callback failed",);

    retval = setConditionVariableCallbacks(server, condition, conditionType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ConditionVariable Callback failed",);

    /* Set callbacks for Method Components (needs to be set only once!) */
    if(LIST_EMPTY(&server->conditionSources)) {
        retval = setConditionMethodCallbacks(server, condition, conditionType);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Method Callback failed",);

        // Create RefreshEvents
        if(UA_NodeId_isNull(&refreshEvents[REFRESHEVENT_START_IDX]) &&
           UA_NodeId_isNull(&refreshEvents[REFRESHEVENT_END_IDX])) {
            retval = createRefreshMethodEvents(server, &refreshEvents[REFRESHEVENT_START_IDX],
                                               &refreshEvents[REFRESHEVENT_END_IDX]);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Create RefreshEvents failed",);
        }
    }

    return retval;
}

static UA_StatusCode
addCondition_finish(UA_Server *server,
                    const UA_NodeId conditionId,
                    const UA_NodeId conditionType,
                    const UA_QualifiedName conditionName,
                    const UA_NodeId conditionSource,
                    const UA_NodeId hierarchialReferenceType) {

    UA_StatusCode retval = UA_Server_addNode_finish(server, conditionId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Finish node failed",);

    /* Make sure the ConditionSource has HasEventSource or one of its SubTypes ReferenceType */
    UA_NodeId serverObject = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    if(!doesHasEventSourceReferenceExist(server, conditionSource) &&
       !UA_NodeId_equal(&serverObject, &conditionSource)) {
         UA_NodeId hasHasEventSourceId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE);
         UA_ExpandedNodeId expandedConditionSource = UA_EXPANDEDNODEID_NULL;
         expandedConditionSource.nodeId = conditionSource;
          retval = UA_Server_addReference(server, serverObject, hasHasEventSourceId,
                                          expandedConditionSource, true);
          CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating HasHasEventSource Reference "
                                         "to the Server Object failed",);
    }

    /* create HasCondition Reference (HasCondition should be forward from the
     * ConditionSourceNode to the Condition. else, HasCondition should be
     * forward from the ConditionSourceNode to the ConditionType Node) */
    if(!UA_NodeId_isNull(&hierarchialReferenceType)) {
        /* Create hierarchical Reference to ConditionSource to expose the
         * ConditionNode in Address Space */
        // only Check hierarchialReferenceType
        UA_ExpandedNodeId expandedNewNodeId = UA_EXPANDEDNODEID_NULL;
        expandedNewNodeId.nodeId = conditionId;
        retval = UA_Server_addReference(server, conditionSource, hierarchialReferenceType,
                                        expandedNewNodeId, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating hierarchical Reference to "
                                       "ConditionSource failed",);

        retval = UA_Server_addReference(server, conditionSource,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCONDITION),
                                        expandedNewNodeId, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating HasCondition Reference failed",);
    } else {
        UA_ExpandedNodeId expandedConditionType = UA_EXPANDEDNODEID_NULL;
        expandedConditionType.nodeId = conditionType;
        retval = UA_Server_addReference(server, conditionSource,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCONDITION),
                                        expandedConditionType, true);
        if(retval != UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED)
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating HasCondition Reference failed",);
    }

    /* Set standard fields */
    retval = setStandardConditionFields(server, &conditionId, &conditionType,
                                        &conditionSource, &conditionName);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set standard Condition Fields failed",);

    /* Set Method Callbacks */
    retval = setStandardConditionCallbacks(server, &conditionId, &conditionType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition callbacks failed",);

    /* change Refresh Events IsAbstract = false
     * so abstract Events : RefreshStart and RefreshEnd could be created */
    UA_NodeId refreshStartEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHSTARTEVENTTYPE);
    UA_NodeId refreshEndEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHENDEVENTTYPE);

    UA_Boolean startAbstract = false;
    UA_Boolean endAbstract = false;
    UA_Server_readIsAbstract(server, refreshStartEventTypeNodeId, &startAbstract);
    UA_Server_readIsAbstract(server, refreshEndEventTypeNodeId, &endAbstract);

    UA_Boolean inner = (startAbstract == false && endAbstract == false);
    if(inner) {
        UA_Server_writeIsAbstract(server, refreshStartEventTypeNodeId, false);
        UA_Server_writeIsAbstract(server, refreshEndEventTypeNodeId, false);
    }

    /* append Condition to list */
    return appendConditionEntry(server, &conditionId, &conditionSource);
}

/* Create condition instance. The function checks first whether the passed
 * conditionType is a subType of ConditionType. Then checks whether the
 * condition source has HasEventSource reference to its parent. If not, a
 * HasEventSource reference will be created between condition source and server
 * object. To expose the condition in address space, a hierarchical
 * ReferenceType should be passed to create the reference to condition source.
 * Otherwise, UA_NODEID_NULL should be passed to make the condition unexposed. */
UA_StatusCode
UA_Server_createCondition(UA_Server *server,
                          const UA_NodeId conditionId, const UA_NodeId conditionType,
                          const UA_QualifiedName conditionName,
                          const UA_NodeId conditionSource,
                          const UA_NodeId hierarchialReferenceType,
                          UA_NodeId *outNodeId) {
    if(!outNodeId) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "outNodeId cannot be NULL!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_StatusCode retval = UA_Server_addCondition_begin(server, conditionId, conditionType,
                                                        conditionName, outNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        return retval;
    }
    return addCondition_finish(server, *outNodeId, conditionType, conditionName,
                               conditionSource, hierarchialReferenceType);
}

UA_StatusCode
UA_Server_addCondition_begin(UA_Server *server, const UA_NodeId conditionId,
                             const UA_NodeId conditionType,
                             const UA_QualifiedName conditionName, UA_NodeId *outNodeId) {
    if(!outNodeId) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "outNodeId cannot be NULL!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Make sure the conditionType is a Subtype of ConditionType */
    UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    if(!isNodeInTree_singleRef(server, &conditionType, &conditionTypeId,
                               UA_REFERENCETYPEINDEX_HASSUBTYPE)) {
        UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                     "Condition Type must be a subtype of ConditionType!");
        return UA_STATUSCODE_BADNOMATCH;
    }

    /* Create an ObjectNode which represents the condition */
    UA_StatusCode retval;
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName.locale = UA_STRING("en");
    oAttr.displayName.text = conditionName.name;
    retval = UA_Server_addNode_begin(server,
                                     UA_NODECLASS_OBJECT,
                                     conditionId,
                                     UA_NODEID_NULL,
                                     UA_NODEID_NULL,
                                     conditionName,
                                     conditionType,
                                     (const UA_NodeAttributes *)&oAttr,
                                     &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                                     NULL,
                                     outNodeId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding Condition failed", );
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_addCondition_finish(UA_Server *server,
                              const UA_NodeId conditionId,
                              const UA_NodeId conditionSource,
                              const UA_NodeId hierarchialReferenceType) {

    const UA_Node *node = UA_NODESTORE_GET(server, &conditionId);

    if(!node)
        return UA_STATUSCODE_BADNODEIDUNKNOWN;

    const UA_Node *type = getNodeType(server, &node->head);
    if(!type) {
        UA_NODESTORE_RELEASE(server, node);
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

    UA_StatusCode retval;
    retval = addCondition_finish(server, conditionId, type->head.nodeId, node->head.browseName,
                                conditionSource, hierarchialReferenceType);

    UA_NODESTORE_RELEASE(server, type);
    UA_NODESTORE_RELEASE(server, node);

    return retval;
}

#ifdef CONDITIONOPTIONALFIELDS_SUPPORT

static UA_StatusCode
addOptionalVariableField(UA_Server *server, const UA_NodeId *originCondition,
                         const UA_QualifiedName *fieldName,
                         const UA_VariableNode *optionalVariableFieldNode,
                         UA_NodeId *outOptionalVariable) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.valueRank = optionalVariableFieldNode->valueRank;
    UA_StatusCode retval = UA_LocalizedText_copy(&optionalVariableFieldNode->head.displayName,
                                                 &vAttr.displayName);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Copying LocalizedText failed",);

    retval = UA_NodeId_copy(&optionalVariableFieldNode->dataType, &vAttr.dataType);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Copying NodeId failed",);

    /* Get typedefintion */
    const UA_Node *type = getNodeType(server, &optionalVariableFieldNode->head);
    if(!type) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Invalid VariableType. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADTYPEDEFINITIONINVALID));
        UA_VariableAttributes_clear(&vAttr);
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

    /* Set referenceType to parent */
    UA_NodeId referenceToParent;
    UA_NodeId propertyTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE);
    if(UA_NodeId_equal(&type->head.nodeId, &propertyTypeNodeId))
        referenceToParent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    else
        referenceToParent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);

    /* Set a random unused NodeId with specified Namespace Index*/
    UA_NodeId optionalVariable = {originCondition->namespaceIndex, UA_NODEIDTYPE_NUMERIC, {0}};
    retval = UA_Server_addVariableNode(server, optionalVariable, *originCondition,
                                       referenceToParent, *fieldName, type->head.nodeId,
                                       vAttr, NULL, outOptionalVariable);
    UA_NODESTORE_RELEASE(server, type);
    UA_VariableAttributes_clear(&vAttr);
    return retval;
}

static UA_StatusCode
addOptionalObjectField(UA_Server *server, const UA_NodeId *originCondition,
                       const UA_QualifiedName* fieldName,
                       const UA_ObjectNode *optionalObjectFieldNode,
                       UA_NodeId *outOptionalObject) {
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_StatusCode retval = UA_LocalizedText_copy(&optionalObjectFieldNode->head.displayName,
                                                 &oAttr.displayName);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Copying LocalizedText failed",);

    /* Get typedefintion */
    const UA_Node *type = getNodeType(server, &optionalObjectFieldNode->head);
    if(!type) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Invalid ObjectType. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADTYPEDEFINITIONINVALID));
        UA_ObjectAttributes_clear(&oAttr);
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

    /* Set referenceType to parent */
    UA_NodeId referenceToParent;
    UA_NodeId propertyTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_PROPERTYTYPE);
    if(UA_NodeId_equal(&type->head.nodeId, &propertyTypeNodeId))
        referenceToParent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY);
    else
        referenceToParent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);

    UA_NodeId optionalObject = {originCondition->namespaceIndex, UA_NODEIDTYPE_NUMERIC, {0}};
    retval = UA_Server_addObjectNode(server, optionalObject, *originCondition,
                                     referenceToParent, *fieldName, type->head.nodeId,
                                     oAttr, NULL, outOptionalObject);

    UA_NODESTORE_RELEASE(server, type);
    UA_ObjectAttributes_clear(&oAttr);
    return retval;
}
#endif//CONDITIONOPTIONALFIELDS_SUPPORT

/**
 * add an optional condition field using its name. (Adding optional methods
 * is not implemented yet)
 */
UA_StatusCode
UA_Server_addConditionOptionalField(UA_Server *server, const UA_NodeId condition,
                                    const UA_NodeId conditionType, const UA_QualifiedName fieldName,
                                    UA_NodeId *outOptionalNode) {
#ifdef CONDITIONOPTIONALFIELDS_SUPPORT
    /* Get optional Field NodId from ConditionType -> user should give the
     * correct ConditionType or Subtype!!!! */
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, conditionType, 1, &fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;

    /* Get Node */
    UA_NodeId optionalFieldNodeId = bpr.targets[0].targetId.nodeId;
    const UA_Node *optionalFieldNode = UA_NODESTORE_GET(server, &optionalFieldNodeId);
    if(NULL == optionalFieldNode) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Couldn't find optional Field Node in ConditionType. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNOTFOUND));
        UA_BrowsePathResult_clear(&bpr);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    switch(optionalFieldNode->head.nodeClass) {
        case UA_NODECLASS_VARIABLE: {
            UA_StatusCode retval =
                addOptionalVariableField(server, &condition, &fieldName,
                                         (const UA_VariableNode *)optionalFieldNode, outOptionalNode);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                             "Adding Condition Optional Variable Field failed. StatusCode %s",
                             UA_StatusCode_name(retval));
            }
            UA_BrowsePathResult_clear(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return retval;
        }
        case UA_NODECLASS_OBJECT:{
          UA_StatusCode retval =
              addOptionalObjectField(server, &condition, &fieldName,
                                     (const UA_ObjectNode *)optionalFieldNode, outOptionalNode);
          if(retval != UA_STATUSCODE_GOOD) {
              UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                           "Adding Condition Optional Object Field failed. StatusCode %s",
                           UA_StatusCode_name(retval));
          }
          UA_BrowsePathResult_clear(&bpr);
          UA_NODESTORE_RELEASE(server, optionalFieldNode);
          return retval;
        }
        case UA_NODECLASS_METHOD:
            /*TODO method: Check first logic of creating methods at all (should
              we create a new method or just reference it from the
              ConditionType?)*/
            UA_BrowsePathResult_clear(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
        default:
            UA_BrowsePathResult_clear(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
    }

#else
    UA_LOG_ERROR(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                 "Adding Condition Optional Fields disabled. StatusCode %s",
                 UA_StatusCode_name(UA_STATUSCODE_BADNOTSUPPORTED));
    return UA_STATUSCODE_BADNOTSUPPORTED;
#endif//CONDITIONOPTIONALFIELDS_SUPPORT
}

/* Set the value of condition field (only scalar). */
UA_StatusCode
UA_Server_setConditionField(UA_Server *server, const UA_NodeId condition,
                            const UA_Variant* value, const UA_QualifiedName fieldName) {
    if(value->arrayLength != 0 || value->data <= UA_EMPTY_ARRAY_SENTINEL) {
      //TODO implement logic for array variants!
      CONDITION_ASSERT_RETURN_RETVAL(UA_STATUSCODE_BADNOTIMPLEMENTED,
                                     "Set Condition Field with Array value not implemented",);
    }

    UA_BrowsePathResult bpr = UA_Server_browseSimplifiedBrowsePath(server, condition, 1, &fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;

    UA_StatusCode retval = UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, *value);
    UA_BrowsePathResult_clear(&bpr);

    return retval;
}

/* Set the value of property of condition field. */
UA_StatusCode
UA_Server_setConditionVariableFieldProperty(UA_Server *server, const UA_NodeId condition,
                                            const UA_Variant* value,
                                            const UA_QualifiedName variableFieldName,
                                            const UA_QualifiedName variablePropertyName) {

  if(value->arrayLength != 0 || value->data <= UA_EMPTY_ARRAY_SENTINEL) {
      //TODO implement logic for array variants!
      CONDITION_ASSERT_RETURN_RETVAL(UA_STATUSCODE_BADNOTIMPLEMENTED,
                                     "Set Property of Condition Field with Array value not implemented",);
    }

    /*1) find Variable Field of the Condition*/
    UA_BrowsePathResult bprConditionVariableField =
        UA_Server_browseSimplifiedBrowsePath(server, condition, 1, &variableFieldName);
    if(bprConditionVariableField.statusCode != UA_STATUSCODE_GOOD)
        return bprConditionVariableField.statusCode;

    /*2) find Property of the Variable Field of the Condition*/
    UA_BrowsePathResult bprVariableFieldProperty =
        UA_Server_browseSimplifiedBrowsePath(server,
                                             bprConditionVariableField.targets->targetId.nodeId,
                                             1, &variablePropertyName);
    if(bprVariableFieldProperty.statusCode != UA_STATUSCODE_GOOD) {
        UA_BrowsePathResult_clear(&bprConditionVariableField);
        return bprVariableFieldProperty.statusCode;
    }

    UA_StatusCode retval =
        UA_Server_writeValue(server, bprVariableFieldProperty.targets[0].targetId.nodeId, *value);
    UA_BrowsePathResult_clear(&bprConditionVariableField);
    UA_BrowsePathResult_clear(&bprVariableFieldProperty);
    return retval;
}

/* triggers an event only for an enabled condition. The condition list is
 * updated then with the last generated EventId. */
UA_StatusCode
UA_Server_triggerConditionEvent(UA_Server *server, const UA_NodeId condition,
                                const UA_NodeId conditionSource, UA_ByteString *outEventId){
    /* Check if enabled */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    UA_QualifiedName enabledStateField = UA_QUALIFIEDNAME(0, CONDITION_FIELD_ENABLEDSTATE);
    if(!isTwoStateVariableInTrueState(server, &condition, &enabledStateField)) {
        UA_LOG_WARNING(&server->config.logger, UA_LOGCATEGORY_USERLAND,
                       "Cannot trigger condition event when "
                       CONDITION_FIELD_ENABLEDSTATE"."
                       CONDITION_FIELD_TWOSTATEVARIABLE_ID" is false.");
        return UA_STATUSCODE_BADCONDITIONALREADYDISABLED;
    }

    setIsCallerAC(server, &condition, &conditionSource, true);

    /* Trigger the event for Condition*/
    //Condition Nodes should not be deleted after triggering the event
    UA_StatusCode retval = UA_Server_triggerEvent(server, condition, conditionSource, &eventId, false);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Triggering condition event failed",);

    setIsCallerAC(server, &condition, &conditionSource, false);

    /* Update list */
    retval = updateConditionLastEventId(server, &condition, &conditionSource, &eventId);
    if(outEventId)
        *outEventId = eventId;
    else
        UA_ByteString_clear(&eventId);
    return retval;
}

UA_StatusCode UA_Server_deleteCondition(UA_Server *server, const UA_NodeId condition,
                                        const UA_NodeId conditionSource) {
    // Delete from internal list
    UA_Boolean found = UA_FALSE;
    /* Get ConditionSource Entry */
    UA_ConditionSource *source, *tmp_source;
    LIST_FOREACH_SAFE(source, &server->conditionSources, listEntry, tmp_source) {
        if(!UA_NodeId_equal(&source->conditionSourceId, &conditionSource))
            continue;
        /* Get Condition Entry */
        UA_Condition *cond, *tmp_cond;
        LIST_FOREACH_SAFE(cond, &source->conditions, listEntry, tmp_cond) {
            if(!UA_NodeId_equal(&cond->conditionId, &condition))
                continue;
            deleteCondition(cond);
            found = UA_TRUE;
            break;
        }

        if(LIST_EMPTY(&source->conditions)){
            UA_NodeId_clear(&source->conditionSourceId);
            LIST_REMOVE(source, listEntry);
            UA_free(source);
        }
        break;
    }
    if(!found)
        return UA_STATUSCODE_BADNOTFOUND;
    /* Delete from address space */
    return UA_Server_deleteNode(server, condition, true);
}
#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */
