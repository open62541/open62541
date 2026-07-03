/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Sameer AL-Qadasi)
 *    Copyright 2020-2022 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

#include <open62541/driver/alarms_conditions.h>

#include "open62541_queue.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

/**************************************
 * Types and Constants
 **************************************/

typedef enum {
    UA_INACTIVE = 0,
    UA_ACTIVE,
    UA_ACTIVE_HIGHHIGH,
    UA_ACTIVE_HIGH,
    UA_ACTIVE_LOW,
    UA_ACTIVE_LOWLOW
} UA_ActiveState;

/* In Alarms and Conditions first implementation, conditionBranchId is always
 * equal to NULL NodeId (UA_NODEID_NULL). That ConditionBranch represents the
 * current state Condition. The current state is determined by the last Event
 * triggered (lastEventId). See Part 9, 5.5.2, BranchId. */
typedef struct UA_ConditionBranch {
    LIST_ENTRY(UA_ConditionBranch) listEntry;
    UA_NodeId conditionBranchId;
    UA_ByteString lastEventId;
} UA_ConditionBranch;

/* In Alarms and Conditions first implementation, A Condition
 * have only one ConditionBranch entry. */
typedef struct UA_Condition {
    LIST_ENTRY(UA_Condition) listEntry;
    LIST_HEAD(, UA_ConditionBranch) conditionBranches;
    UA_NodeId conditionId;
    UA_UInt16 lastSeverity;
    UA_DateTime lastSeveritySourceTimeStamp;

    /* These callbacks are defined by the user and must not be called with a
     * locked server mutex */
    struct {
        UA_TwoStateVariableChangeCallback enableStateCallback;
        UA_TwoStateVariableChangeCallback ackStateCallback;
        UA_Boolean ackedRemoveBranch;
        UA_TwoStateVariableChangeCallback confirmStateCallback;
        UA_Boolean confirmedRemoveBranch;
        UA_TwoStateVariableChangeCallback activeStateCallback;
    } callbacks;

    UA_ActiveState lastActiveState;
    UA_ActiveState activeState;
    UA_Boolean isLimitAlarm;
} UA_Condition;

/* A ConditionSource can have multiple Conditions. */
typedef struct UA_ConditionSource {
    LIST_ENTRY(UA_ConditionSource) listEntry;
    LIST_HEAD(, UA_Condition) conditions;
    UA_NodeId conditionSourceId;
} UA_ConditionSource;

typedef struct UA_ACMonitoredItemTarget {
    LIST_ENTRY(UA_ACMonitoredItemTarget) listEntry;
    UA_NodeId sessionId;
    UA_UInt32 subscriptionId;
    UA_UInt32 monitoredItemId;
    UA_NodeId itemToMonitorId;
    UA_UInt32 attributeId;
} UA_ACMonitoredItemTarget;

typedef struct AlarmsConditionsDriver {
    UA_AlarmConditionsDriver driver;
    UA_Logger *logging;
    LIST_HEAD(, UA_ConditionSource) conditionSources;
    LIST_HEAD(, UA_ACMonitoredItemTarget) monitoredItemTargets;
} AlarmsConditionsDriver;

#define UA_DRIVER_ALARMS_CONDITIONS_NAME "alarms-conditions"

#define REFRESHEVENT_SEVERITY_DEFAULT                          100
#define EXPIRATION_LIMIT_DEFAULT_VALUE                         15

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
static const UA_QualifiedName fieldEventTypeQN = STATIC_QN("EventType");
static const UA_QualifiedName fieldSourceNameQN = STATIC_QN("SourceName");
static const UA_QualifiedName fieldSeverityQN = STATIC_QN("Severity");
static const UA_QualifiedName fieldConditionNameQN = STATIC_QN("ConditionName");
static const UA_QualifiedName fieldEnabledStateQN = STATIC_QN("EnabledState");
static const UA_QualifiedName fieldRetainQN = STATIC_QN("Retain");
static const UA_QualifiedName twoStateVariableIdQN = STATIC_QN("Id");
static const UA_QualifiedName fieldQualityQN = STATIC_QN("Quality");
static const UA_QualifiedName fieldLastSeverityQN = STATIC_QN("LastSeverity");
static const UA_QualifiedName fieldCommentQN = STATIC_QN("Comment");
static const UA_QualifiedName fieldSourceTimestampQN = STATIC_QN("SourceTimestamp");
static const UA_QualifiedName fieldMessageQN = STATIC_QN("Message");
static const UA_QualifiedName fieldAckedStateQN = STATIC_QN("AckedState");
static const UA_QualifiedName fieldConfirmedStateQN = STATIC_QN("ConfirmedState");
static const UA_QualifiedName fieldActiveStateQN = STATIC_QN("ActiveState");
static const UA_QualifiedName fieldTimeQN = STATIC_QN("Time");
static const UA_QualifiedName fieldSourceQN = STATIC_QN("SourceNode");
static const UA_QualifiedName fieldLimitStateQN = STATIC_QN("LimitState");
static const UA_QualifiedName fieldLowLimitQN = STATIC_QN("LowLimit");
static const UA_QualifiedName fieldLowLowLimitQN = STATIC_QN("LowLowLimit");
static const UA_QualifiedName fieldHighLimitQN = STATIC_QN("HighLimit");
static const UA_QualifiedName fieldHighHighLimitQN = STATIC_QN("HighHighLimit");
static const UA_QualifiedName fieldEngineeringUnitsQN = STATIC_QN("EngineeringUnits");
static const UA_QualifiedName fieldExpirationDateQN = STATIC_QN("ExpirationDate");
static const UA_QualifiedName fieldExpirationLimitQN = STATIC_QN("ExpirationLimit");

#define CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, logMessage, deleteFunction) \
    {                                                                   \
        if(res != UA_STATUSCODE_GOOD) {                                 \
            UA_LOG_ERROR((acd)->logging, UA_LOGCATEGORY_SERVER,         \
                         logMessage". StatusCode %s", UA_StatusCode_name(res)); \
            deleteFunction                                              \
                return res;                                             \
        }                                                               \
    }

#define CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, logMessage, deleteFunction) \
    {                                                                   \
        if(res != UA_STATUSCODE_GOOD) {                                 \
            UA_LOG_ERROR(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER, \
                         logMessage". StatusCode %s", UA_StatusCode_name(res)); \
            deleteFunction                                              \
                return res;                                             \
        }                                                               \
    }

#define CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, logMessage, deleteFunction) \
    {                                                                   \
        if(res != UA_STATUSCODE_GOOD) {                                 \
            UA_LOG_ERROR((acd)->logging, UA_LOGCATEGORY_SERVER,         \
                         logMessage". StatusCode %s", UA_StatusCode_name(res)); \
            deleteFunction                                              \
                return;                                                 \
        }                                                               \
    }

static UA_StatusCode
AlarmsConditionsDriver_start(UA_Driver *drv);

/**************************************
 * Browse and Type Helpers
 **************************************/

static UA_Boolean
hasRecursiveBrowsePath(UA_Server *server, const UA_NodeId *start,
                       const UA_NodeId *target, UA_UInt32 referenceTypeId,
                       UA_BrowseDirection direction, UA_Boolean includeSubtypes) {
    if(UA_NodeId_equal(start, target))
        return true;

    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *start;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, referenceTypeId);
    bd.browseDirection = direction;
    bd.includeSubtypes = includeSubtypes;
    bd.nodeClassMask = 0;
    bd.resultMask = UA_BROWSERESULTMASK_NONE;

    size_t resultsSize = 0;
    UA_ExpandedNodeId *results = NULL;
    UA_StatusCode res = UA_Server_browseRecursive(server, &bd, &resultsSize, &results);
    if(res != UA_STATUSCODE_GOOD)
        return false;

    UA_Boolean found = false;
    for(size_t i = 0; i < resultsSize; i++) {
        if(!UA_ExpandedNodeId_isLocal(&results[i]))
            continue;
        if(UA_NodeId_equal(&results[i].nodeId, target)) {
            found = true;
            break;
        }
    }

    UA_Array_delete(results, resultsSize, &UA_TYPES[UA_TYPES_EXPANDEDNODEID]);
    return found;
}

static UA_Boolean
isSubtypeOf(UA_Server *server, const UA_NodeId *node, const UA_NodeId *type) {
    return hasRecursiveBrowsePath(server, node, type, UA_NS0ID_HASSUBTYPE,
                                  UA_BROWSEDIRECTION_INVERSE, false);
}

static UA_StatusCode
getTypeDefinitionId(UA_Server *server, const UA_NodeId *nodeId,
                    UA_NodeId *typeDefinitionId) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = *nodeId;
    bd.referenceTypeId = UA_NS0ID(HASTYPEDEFINITION);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = false;
    bd.nodeClassMask = 0;
    bd.resultMask = UA_BROWSERESULTMASK_NONE;

    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    if(br.statusCode != UA_STATUSCODE_GOOD) {
        UA_StatusCode res = br.statusCode;
        UA_BrowseResult_clear(&br);
        return res;
    }

    UA_StatusCode res = UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    for(size_t i = 0; i < br.referencesSize; i++) {
        if(!UA_ExpandedNodeId_isLocal(&br.references[i].nodeId))
            continue;
        res = UA_NodeId_copy(&br.references[i].nodeId.nodeId, typeDefinitionId);
        break;
    }

    UA_BrowseResult_clear(&br);
    return res;
}

/**************************************
 * Driver Lookup and Notifications
 **************************************/

static UA_Boolean
isAlarmsConditionsDriver(const UA_Driver *drv) {
    return drv && drv->start == AlarmsConditionsDriver_start;
}

static AlarmsConditionsDriver *
findAlarmsConditionsDriver(UA_Server *server) {
    for(UA_Driver *drv = UA_Server_getDrivers(server); drv; drv = drv->next) {
        if(isAlarmsConditionsDriver(drv) &&
           drv->state == UA_LIFECYCLESTATE_STARTED)
            return (AlarmsConditionsDriver*)drv;
    }

    return NULL;
}

static UA_ACMonitoredItemTarget *
getMonitoredItemTarget(AlarmsConditionsDriver *acd, const UA_NodeId *sessionId,
                       UA_UInt32 subscriptionId, UA_UInt32 monitoredItemId) {
    if(!acd)
        return NULL;

    UA_ACMonitoredItemTarget *target;
    LIST_FOREACH(target, &acd->monitoredItemTargets, listEntry) {
        if(target->subscriptionId == subscriptionId &&
           target->monitoredItemId == monitoredItemId &&
           UA_NodeId_equal(&target->sessionId, sessionId))
            return target;
    }
    return NULL;
}

static UA_StatusCode
upsertMonitoredItemTarget(AlarmsConditionsDriver *acd, UA_ACMonitoredItemTarget *target,
                          const UA_NodeId *sessionId, UA_UInt32 subscriptionId,
                          UA_UInt32 monitoredItemId, const UA_NodeId *itemToMonitorId,
                          UA_UInt32 attributeId) {
    if(target) {
        UA_NodeId_clear(&target->itemToMonitorId);
        UA_StatusCode res = UA_NodeId_copy(itemToMonitorId, &target->itemToMonitorId);
        if(res == UA_STATUSCODE_GOOD)
            target->attributeId = attributeId;
        return res;
    }

    target = (UA_ACMonitoredItemTarget*)UA_calloc(1, sizeof(UA_ACMonitoredItemTarget));
    if(!target)
        return UA_STATUSCODE_BADOUTOFMEMORY;

    UA_StatusCode res = UA_NodeId_copy(sessionId, &target->sessionId);
    if(res == UA_STATUSCODE_GOOD)
        res = UA_NodeId_copy(itemToMonitorId, &target->itemToMonitorId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_NodeId_clear(&target->sessionId);
        UA_NodeId_clear(&target->itemToMonitorId);
        UA_free(target);
        return res;
    }

    target->subscriptionId = subscriptionId;
    target->monitoredItemId = monitoredItemId;
    target->attributeId = attributeId;
    LIST_INSERT_HEAD(&acd->monitoredItemTargets, target, listEntry);
    return UA_STATUSCODE_GOOD;
}

static void
removeMonitoredItemTarget(UA_ACMonitoredItemTarget *target) {
    if(!target)
        return;
    UA_NodeId_clear(&target->sessionId);
    UA_NodeId_clear(&target->itemToMonitorId);
    LIST_REMOVE(target, listEntry);
    UA_free(target);
}

static void
AlarmsConditionsDriver_notification(UA_Driver *drv, UA_ApplicationNotificationType type,
                                    const UA_KeyValueMap payload) {
    AlarmsConditionsDriver *acd = (AlarmsConditionsDriver*)drv;
    if((type & UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM) == 0)
        return;

    const UA_NodeId *sessionId =
        (const UA_NodeId*)UA_KeyValueMap_getScalar(&payload,
            UA_QUALIFIEDNAME(0, "session-id"), &UA_TYPES[UA_TYPES_NODEID]);
    const UA_UInt32 *subscriptionId =
        (const UA_UInt32*)UA_KeyValueMap_getScalar(&payload,
            UA_QUALIFIEDNAME(0, "subscription-id"), &UA_TYPES[UA_TYPES_UINT32]);
    const UA_UInt32 *monitoredItemId =
        (const UA_UInt32*)UA_KeyValueMap_getScalar(&payload,
            UA_QUALIFIEDNAME(0, "monitoreditem-id"), &UA_TYPES[UA_TYPES_UINT32]);
    if(!sessionId || !subscriptionId || !monitoredItemId)
        return;

    UA_ACMonitoredItemTarget *target =
        getMonitoredItemTarget(acd, sessionId, *subscriptionId, *monitoredItemId);

    if(type == UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM_DELETED) {
        removeMonitoredItemTarget(target);
        return;
    }

    const UA_NodeId *itemToMonitorId =
        (const UA_NodeId*)UA_KeyValueMap_getScalar(&payload,
            UA_QUALIFIEDNAME(0, "target-node"), &UA_TYPES[UA_TYPES_NODEID]);
    const UA_UInt32 *attributeId =
        (const UA_UInt32*)UA_KeyValueMap_getScalar(&payload,
            UA_QUALIFIEDNAME(0, "attribute-id"), &UA_TYPES[UA_TYPES_UINT32]);
    if(!itemToMonitorId || !attributeId)
        return;

    upsertMonitoredItemTarget(acd, target, sessionId, *subscriptionId,
                              *monitoredItemId, itemToMonitorId, *attributeId);
}

/**************************************
 * Condition State Lists
 **************************************/

static UA_ConditionSource *
getConditionSource(AlarmsConditionsDriver *acd, const UA_NodeId *sourceId) {
    if(!acd)
        return NULL;

    UA_ConditionSource *cs;
    LIST_FOREACH(cs, &acd->conditionSources, listEntry) {
        if(UA_NodeId_equal(&cs->conditionSourceId, sourceId))
            return cs;
    }
    return NULL;
}

static UA_Condition *
getCondition(AlarmsConditionsDriver *acd, const UA_NodeId *sourceId,
             const UA_NodeId *conditionId) {
    UA_ConditionSource *cs = getConditionSource(acd, sourceId);
    if(!cs)
        return NULL;

    UA_Condition *c;
    LIST_FOREACH(c, &cs->conditions, listEntry) {
        if(UA_NodeId_equal(&c->conditionId, conditionId))
            return c;
    }
    return NULL;
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
deleteConditionState(UA_Condition *cond) {
    deleteAllBranchesFromCondition(cond);
    UA_NodeId_clear(&cond->conditionId);
    LIST_REMOVE(cond, listEntry);
    UA_free(cond);
}

static void
deleteConditionSources(AlarmsConditionsDriver *acd) {
    UA_ConditionSource *source, *tmp_source;
    LIST_FOREACH_SAFE(source, &acd->conditionSources, listEntry, tmp_source) {
        UA_Condition *condition, *tmp_condition;
        LIST_FOREACH_SAFE(condition, &source->conditions, listEntry, tmp_condition) {
            deleteConditionState(condition);
        }
        UA_NodeId_clear(&source->conditionSourceId);
        LIST_REMOVE(source, listEntry);
        UA_free(source);
    }
}

static void
deleteMonitoredItemTargets(AlarmsConditionsDriver *acd) {
    UA_ACMonitoredItemTarget *target, *tmp_target;
    LIST_FOREACH_SAFE(target, &acd->monitoredItemTargets, listEntry, tmp_target) {
        UA_NodeId_clear(&target->sessionId);
        UA_NodeId_clear(&target->itemToMonitorId);
        LIST_REMOVE(target, listEntry);
        UA_free(target);
    }
}

static UA_StatusCode
setConditionInConditionList(UA_Server *server, const UA_NodeId *conditionNodeId,
                            UA_ConditionSource *conditionSourceEntry) {
    UA_Condition *conditionListEntry = (UA_Condition*)UA_malloc(sizeof(UA_Condition));
    if(!conditionListEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(conditionListEntry, 0, sizeof(UA_Condition));

    /* Set ConditionId with given ConditionNodeId */
    UA_StatusCode res = UA_NodeId_copy(conditionNodeId, &conditionListEntry->conditionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(conditionListEntry);
        return res;
    }

    UA_ConditionBranch *conditionBranchListEntry = (UA_ConditionBranch*)
        UA_malloc(sizeof(UA_ConditionBranch));
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
appendConditionEntry(AlarmsConditionsDriver *acd, const UA_NodeId *conditionNodeId,
                     const UA_NodeId *sourceNodeId) {
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    UA_Server *server = acd->driver.drv.server;

    /* See if the ConditionSource Entry already exists*/
    UA_ConditionSource *source = getConditionSource(acd, sourceNodeId);
    if(source)
        return setConditionInConditionList(server, conditionNodeId, source);

    /* ConditionSource not found in list, so we create a new ConditionSource Entry */
    UA_ConditionSource *sourceListEntry;
    sourceListEntry = (UA_ConditionSource*)UA_malloc(sizeof(UA_ConditionSource));
    if(!sourceListEntry)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    memset(sourceListEntry, 0, sizeof(UA_ConditionSource));

    /* Set ConditionSourceId with given ConditionSourceNodeId */
    UA_StatusCode res =
        UA_NodeId_copy(sourceNodeId, &sourceListEntry->conditionSourceId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(sourceListEntry);
        return res;
    }

    LIST_INSERT_HEAD(&acd->conditionSources, sourceListEntry, listEntry);
    return setConditionInConditionList(server, conditionNodeId, sourceListEntry);
}

/**************************************
 * Condition Field Helpers
 **************************************/

/* Call the user callback registered for a TwoStateVariable transition. */
static UA_StatusCode
getConditionTwoStateVariableCallback(AlarmsConditionsDriver *acd, const UA_NodeId *branch,
                                     UA_Condition *condition, UA_Boolean *removeBranch,
                                     UA_TwoStateVariableCallbackType callbackType) {
    UA_Server *server = acd->driver.drv.server;
    UA_StatusCode res = UA_STATUSCODE_GOOD;
    /* TODO log warning when the callback wasn't set */
    switch(callbackType) {
    case UA_ENTERING_ENABLEDSTATE:
        if(condition->callbacks.enableStateCallback)
            res = condition->callbacks.enableStateCallback(server, branch);
        break;

    case UA_ENTERING_ACKEDSTATE:
        if(condition->callbacks.ackStateCallback) {
            *removeBranch = condition->callbacks.ackedRemoveBranch;
            res = condition->callbacks.ackStateCallback(server, branch);
        }
        break;

    case UA_ENTERING_CONFIRMEDSTATE:
        if(condition->callbacks.confirmStateCallback) {
            *removeBranch = condition->callbacks.confirmedRemoveBranch;
            res = condition->callbacks.confirmStateCallback(server, branch);
        }
        break;

    case UA_ENTERING_ACTIVESTATE:
        if(condition->callbacks.activeStateCallback)
            res = condition->callbacks.activeStateCallback(server, branch);
        break;

    default:
        res = UA_STATUSCODE_BADNOTFOUND;
        break;
    }

    return res;
}

static UA_StatusCode
callConditionTwoStateVariableCallback(AlarmsConditionsDriver *acd, const UA_NodeId *condition,
                                      const UA_NodeId *conditionSource, UA_Boolean *removeBranch,
                                      UA_TwoStateVariableCallbackType callbackType) {
    UA_ConditionSource *source =
        getConditionSource(acd, conditionSource);
    if(!source)
        return UA_STATUSCODE_BADNOTFOUND;

    UA_Condition *cond;
    LIST_FOREACH(cond, &source->conditions, listEntry) {
        if(UA_NodeId_equal(&cond->conditionId, condition)) {
            return getConditionTwoStateVariableCallback(acd, condition, cond,
                                                        removeBranch, callbackType);
        }
        UA_ConditionBranch *branch;
        LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
            if(!UA_NodeId_equal(&branch->conditionBranchId, condition))
                continue;
            return getConditionTwoStateVariableCallback(acd, &branch->conditionBranchId,
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

    UA_UInt32 referenceTypes[2] = {
        UA_NS0ID_HASPROPERTY,
        UA_NS0ID_HASCOMPONENT
    };

    for(size_t i = 0; i < 2; i++) {
        UA_BrowseDescription bd;
        UA_BrowseDescription_init(&bd);
        bd.nodeId = *field;
        bd.referenceTypeId = UA_NODEID_NUMERIC(0, referenceTypes[i]);
        bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
        bd.includeSubtypes = false;
        bd.nodeClassMask = 0;
        bd.resultMask = UA_BROWSERESULTMASK_NONE;

        UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
        if(br.statusCode != UA_STATUSCODE_GOOD) {
            UA_BrowseResult_clear(&br);
            continue;
        }

        for(size_t j = 0; j < br.referencesSize; j++) {
            if(!UA_ExpandedNodeId_isLocal(&br.references[j].nodeId))
                continue;

            UA_StatusCode res =
                UA_NodeId_copy(&br.references[j].nodeId.nodeId, parent);
            UA_BrowseResult_clear(&br);
            return res;
        }

        UA_BrowseResult_clear(&br);
    }

    return UA_STATUSCODE_BADNOTFOUND;
}

/* Gets the NodeId of a Field (e.g. Severity) */
static UA_StatusCode
getConditionFieldNodeId(UA_Server *server, const UA_NodeId *conditionNodeId,
                        const UA_QualifiedName* fieldName, UA_NodeId *outFieldNodeId) {

    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, *conditionNodeId, 1, fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;
    UA_StatusCode res = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, outFieldNodeId);
    UA_BrowsePathResult_clear(&bpr);
    return res;
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
    UA_StatusCode res = getConditionFieldNodeId(server, condition, &fieldName, &nodeIdValue);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Field not found",);

    /* Read the Value of SourceNode Property Node (the Value is a NodeId) */
    UA_Variant tOutVariant;
    res = UA_Server_readValue(server, nodeIdValue, &tOutVariant);
    if(res != UA_STATUSCODE_GOOD ||
       !UA_Variant_hasScalarType(&tOutVariant, &UA_TYPES[UA_TYPES_NODEID])) {
        UA_NodeId_clear(&nodeIdValue);
        UA_Variant_clear(&tOutVariant);
        return res;
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
getConditionBranchNodeId(AlarmsConditionsDriver *acd, const UA_ByteString *eventId,
                         UA_NodeId *outConditionBranchNodeId) {
    *outConditionBranchNodeId = UA_NODEID_NULL;
    /* The function checks the BranchId based on the event Id, if BranchId ==
       NULL -> outConditionId = ConditionId */
    /* Get ConditionSource Entry */
    UA_StatusCode res = UA_STATUSCODE_BADEVENTIDUNKNOWN;
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_ConditionSource *source;
    LIST_FOREACH(source, &acd->conditionSources, listEntry) {
        /* Get Condition Entry */
        UA_Condition *cond;
        LIST_FOREACH(cond, &source->conditions, listEntry) {
            /* Get Branch Entry*/
            UA_ConditionBranch *branch;
            LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
                if(!UA_ByteString_equal(&branch->lastEventId, eventId))
                    continue;
                if(UA_NodeId_isNull(&branch->conditionBranchId)) {
                    res = UA_NodeId_copy(&cond->conditionId, outConditionBranchNodeId);
                    return res;
                } else {
                    res = UA_NodeId_copy(&branch->conditionBranchId, outConditionBranchNodeId);
                    return res;
                }
                return res;
            }
        }
    }

    return res;
}

static UA_StatusCode
updateConditionLastEventId(AlarmsConditionsDriver *acd,
                           UA_Condition *cond,
                           const UA_ByteString *lastEventId) {
    UA_ConditionBranch *branch;
    LIST_FOREACH(branch, &cond->conditionBranches, listEntry) {
        if(UA_NodeId_isNull(&branch->conditionBranchId)) {
            /* update main condition branch */
            UA_ByteString_clear(&branch->lastEventId);
            return UA_ByteString_copy(lastEventId, &branch->lastEventId);
        }
    }
    UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                 "Condition Branch not implemented");
    return UA_STATUSCODE_BADNOTFOUND;
}

static UA_Boolean
isRetained(UA_Server *server, const UA_NodeId *condition) {
    /* Get EnabledStateId NodeId */
    UA_NodeId retainNodeId;
    UA_StatusCode res = getConditionFieldNodeId(server, condition, &fieldRetainQN, &retainNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                       "Retain not found. StatusCode %s", UA_StatusCode_name(res));
        return false; //TODO maybe a better error handling?
    }

    /* Read Retain value */
    UA_Variant tOutVariant;
    res = UA_Server_readValue(server, retainNodeId, &tOutVariant);
    if(res != UA_STATUSCODE_GOOD ||
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
    UA_StatusCode res = getConditionFieldPropertyNodeId(server, condition, twoStateVariable,
                                                        &twoStateVariableIdQN,
                                                        &twoStateVariableIdNodeId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                       "TwoStateVariable/Id not found. StatusCode %s", UA_StatusCode_name(res));
        return false; //TODO maybe a better error handling?
    }

    /* Read Id value */
    UA_Variant tOutVariant;
    res = UA_Server_readValue(server, twoStateVariableIdNodeId, &tOutVariant);
    if(res != UA_STATUSCODE_GOOD ||
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

static AlarmsConditionsDriver *
getStartedDriver(UA_AlarmConditionsDriver *driver) {
    if(!driver || driver->drv.state != UA_LIFECYCLESTATE_STARTED ||
       !driver->drv.server)
        return NULL;

    return (AlarmsConditionsDriver*)driver;
}

static UA_StatusCode
setConditionField(UA_AlarmConditionsDriver *driver,
                  const UA_NodeId condition, const UA_Variant *value,
                  const UA_QualifiedName fieldName) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    if(value->arrayLength != 0 || value->data <= UA_EMPTY_ARRAY_SENTINEL) {
        //TODO implement logic for array variants!
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, UA_STATUSCODE_BADNOTIMPLEMENTED,
            "Set Condition Field with Array value not implemented",);
    }

    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, condition, 1, &fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;

    UA_StatusCode res =
        UA_Server_writeValue(server, bpr.targets[0].targetId.nodeId, *value);
    UA_BrowsePathResult_clear(&bpr);

    return res;
}

static UA_StatusCode
setConditionVariableFieldProperty(UA_AlarmConditionsDriver *driver, const UA_NodeId condition,
                                  const UA_Variant *value, const UA_QualifiedName variableFieldName,
                                  const UA_QualifiedName variablePropertyName) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    if(value->arrayLength != 0 || value->data <= UA_EMPTY_ARRAY_SENTINEL) {
        //TODO implement logic for array variants!
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, UA_STATUSCODE_BADNOTIMPLEMENTED,
            "Set Property of Condition Field with Array value not implemented",);
    }

    UA_BrowsePathResult bprConditionVariableField =
        UA_Server_browseSimplifiedBrowsePath(server, condition, 1, &variableFieldName);
    if(bprConditionVariableField.statusCode != UA_STATUSCODE_GOOD)
        return bprConditionVariableField.statusCode;

    UA_BrowsePathResult bprVariableFieldProperty =
        UA_Server_browseSimplifiedBrowsePath(server,
                                             bprConditionVariableField.targets->targetId.nodeId,
                                             1, &variablePropertyName);
    if(bprVariableFieldProperty.statusCode != UA_STATUSCODE_GOOD) {
        UA_BrowsePathResult_clear(&bprConditionVariableField);
        return bprVariableFieldProperty.statusCode;
    }

    UA_StatusCode res =
        UA_Server_writeValue(server, bprVariableFieldProperty.targets[0].targetId.nodeId, *value);
    UA_BrowsePathResult_clear(&bprConditionVariableField);
    UA_BrowsePathResult_clear(&bprVariableFieldProperty);
    return res;
}

/* triggers an event only for an enabled condition. The condition list is
 * updated then with the last generated EventId. */
static UA_StatusCode
triggerConditionEvent(UA_AlarmConditionsDriver *driver,
                      const UA_NodeId condition, const UA_NodeId conditionSource,
                      UA_ByteString *outEventId) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    /* Check if enabled */
    UA_ByteString eventId = UA_BYTESTRING_NULL;
    if(!isTwoStateVariableInTrueState(server, &condition, &fieldEnabledStateQN)) {
        UA_LOG_WARNING(acd->logging, UA_LOGCATEGORY_SERVER,
                       "Cannot trigger condition event when "
                       "EnabledState.Id is false.");
        return UA_STATUSCODE_BADCONDITIONALREADYDISABLED;
    }

    UA_Condition *cond = getCondition(acd, &conditionSource, &condition);
    if(!cond) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER, "Entry not found in list!");
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_EventDescription ed = {0};
    ed.sourceNode = conditionSource;
    ed.eventInstance = &condition;

    /* Trigger the event for Condition*/
    //Condition Nodes should not be deleted after triggering the event
    UA_StatusCode res = UA_Server_createEventEx(server, &ed, &eventId);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Triggering condition event failed",);

    /* Update list */
    res = updateConditionLastEventId(acd, cond, &eventId);
    if(outEventId)
        *outEventId = eventId;
    else
        UA_ByteString_clear(&eventId);
    return res;
}

/**************************************
 * State Change Callbacks
 **************************************/

static UA_StatusCode
enteringDisabledState(AlarmsConditionsDriver *acd, const UA_NodeId *conditionId,
                      const UA_NodeId *conditionSource) {
    UA_Server *server = acd->driver.drv.server;

    UA_Condition *cond = getCondition(acd, conditionSource, conditionId);
    if(!cond) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
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
        UA_StatusCode res = setConditionField(&acd->driver, triggeredNode, &value, fieldMessageQN);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Condition Message failed",);

        UA_Variant_setScalar(&value, &enableText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, triggeredNode, &value, fieldEnabledStateQN);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Condition EnabledState text failed",);

        UA_Boolean retain = false;
        UA_Variant_setScalar(&value, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
        res = setConditionField(&acd->driver, triggeredNode, &value, fieldRetainQN);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Condition Retain failed",);

        /* Trigger event */
        UA_ByteString lastEventId = UA_BYTESTRING_NULL;
        /* Condition Nodes should not be deleted after triggering the event */
        UA_EventDescription ed = {0};
        ed.eventInstance = &triggeredNode;
        ed.sourceNode = *conditionSource;
        res = UA_Server_createEventEx(server, &ed, &lastEventId);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Triggering condition event failed",);

        /* Update list */
        res = updateConditionLastEventId(acd, cond, &lastEventId);
        UA_ByteString_clear(&lastEventId);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "updating condition event failed",);
    }

    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
enteringEnabledState(AlarmsConditionsDriver *acd, const UA_NodeId *conditionId,
                     const UA_NodeId *conditionSource) {
    /* Get Condition */
    UA_Condition *cond = getCondition(acd, conditionSource, conditionId);
    if(!cond) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
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
        UA_StatusCode res = setConditionField(&acd->driver, triggeredNode,
                                                       &value, fieldMessageQN);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "set Condition Message failed",);

        UA_Variant_setScalar(&value, &enableText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, triggeredNode, &value, fieldEnabledStateQN);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "set Condition EnabledState text failed",);

        /* User callback TODO how should branches be evaluated? see p.19 (5.5.2) */
        UA_Boolean removeBranch = false;//not used
        res = callConditionTwoStateVariableCallback(acd, &triggeredNode, conditionSource,
                                                    &removeBranch, UA_ENTERING_ENABLEDSTATE);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "calling condition callback failed",);

        /* Trigger event */
        //Condition Nodes should not be deleted after triggering the event
        res = triggerConditionEvent(&acd->driver, triggeredNode, *conditionSource, NULL);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "triggering condition event failed",);
    }

    return UA_STATUSCODE_GOOD;
}

static void
afterWriteCallbackEnabledStateChange(UA_Server *server,
                                     const UA_NodeId *sessionId, void *sessionContext,
                                     const UA_NodeId *nodeId, void *nodeContext,
                                     const UA_NumericRange *range, const UA_DataValue *data) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return;

    /* Callback for change in EnabledState/Id property.
     * First we get the EnabledState NodeId then The Condition NodeId */
    UA_NodeId twoStateVariableNode;
    UA_StatusCode res = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent TwoStateVariable found for given EnabledState/Id",);

    UA_NodeId conditionNode;
    res = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_clear(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent Condition found for given EnabledState",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    res = getNodeIdValueOfConditionField(server, &conditionNode, fieldSourceQN,
                                                      &conditionSource);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "ConditionSource not found",
                                 UA_NodeId_clear(&conditionNode););

    /* Set disabling/enabling time */
    UA_DateTime eventTime = data->sourceTimestamp;
    /* If setConditionVariableFieldProperty() was used, sourceTimestamp is 0 */
    if (eventTime == 0)
        eventTime = UA_DateTime_now();
    res = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                  &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set enabling/disabling Time failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););

    if(false == (*((UA_Boolean *)data->value.data))) {
        /* Disable all branches and update list */
        res = enteringDisabledState(acd, &conditionNode, &conditionSource);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Entering disabled state failed",
                                         UA_NodeId_clear(&conditionSource););
    } else {
        /* Enable all branches and update list */
        res = enteringEnabledState(acd, &conditionNode, &conditionSource);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Entering enabled state failed",
                                         UA_NodeId_clear(&conditionSource););
    }

    UA_NodeId_clear(&conditionSource);
}

static void
afterWriteCallbackAckedStateChange(UA_Server *server,
                                   const UA_NodeId *sessionId, void *sessionContext,
                                   const UA_NodeId *nodeId, void *nodeContext,
                                   const UA_NumericRange *range, const UA_DataValue *data) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return;

    /* Get the AckedState NodeId then The Condition NodeId */
    UA_NodeId twoStateVariableNode;
    UA_StatusCode res = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent TwoStateVariable found for given AckedState/Id",);

    UA_NodeId conditionNode;
    res = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_clear(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent Condition found for given AckedState",);

    /* Callback for change to true in AckedState/Id property.
     * First check whether the value is true (ackedState/Id == true).
     * That check makes it possible to set ackedState/Id to false, without triggering an event */
    if(*((UA_Boolean *)data->value.data) == false) {
        /* Set unacknowledging time */
        UA_DateTime eventTime = data->sourceTimestamp;
        if (eventTime == 0)
            eventTime = UA_DateTime_now();
        res = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                   &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set deactivating Time failed",
                                         UA_NodeId_clear(&conditionNode););

        /* Set AckedState text to Unacknowledged*/
        UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, UNACKED_TEXT);
        UA_Variant value;
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, conditionNode, &value, fieldAckedStateQN);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition AckedState failed",);
        return;
    }

    /* Check if enabled and retained */
    if(!isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledStateQN) ||
       !isRetained(server, &conditionNode)) {
        /* Set AckedState/Id to false*/
        UA_Boolean idValue = false;
        UA_Variant value;
        UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        res = setConditionVariableFieldProperty(&acd->driver, conditionNode, &value,
                                                fieldAckedStateQN, twoStateVariableIdQN);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set AckedState/Id failed",);
        return;
    }

    /* Set Message */
    UA_LocalizedText message = UA_LOCALIZEDTEXT(LOCALE, ACKED_MESSAGE);
    UA_Variant value;
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, conditionNode, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition Message failed",
                                     UA_NodeId_clear(&conditionNode););

    /* Set AckedState text */
    UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, ACKED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, conditionNode, &value, fieldAckedStateQN);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition AckedState failed",
                                     UA_NodeId_clear(&conditionNode););

    /* Get conditionSource */
    UA_NodeId conditionSource;
    res = getNodeIdValueOfConditionField(server, &conditionNode, fieldSourceQN,
                                         &conditionSource);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "ConditionSource not found",
                                     UA_NodeId_clear(&conditionNode););

    /* User callback*/
    UA_Boolean removeBranch = false;
    res = callConditionTwoStateVariableCallback(acd, &conditionNode, &conditionSource,
                                                &removeBranch, UA_ENTERING_ACKEDSTATE);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Calling condition callback failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););

    /* Trigger event */
    //Condition Nodes should not be deleted after triggering the event
    res = triggerConditionEvent(&acd->driver, conditionNode, conditionSource, NULL);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Triggering condition event failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););

    UA_NodeId_clear(&conditionNode);
    UA_NodeId_clear(&conditionSource);
}

static void
afterWriteCallbackConfirmedStateChange(UA_Server *server,
                                       const UA_NodeId *sessionId, void *sessionContext,
                                       const UA_NodeId *nodeId, void *nodeContext,
                                       const UA_NumericRange *range, const UA_DataValue *data) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return;

    UA_Variant value;
    UA_NodeId twoStateVariableNode;
    UA_StatusCode res = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent TwoStateVariable found for given ConfirmedState/Id",);

    UA_NodeId conditionNode;
    res = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_clear(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent Condition found for given ConfirmedState",);

    /* Callback to change to true in ConfirmedState/Id property.
     * First check whether the value is true (ConfirmedState/Id == true).
     * That check makes it possible to set ConfirmedState/Id to false, without triggering an event */
    if(*((UA_Boolean *)data->value.data) == false) {
        /* Set unconfirming time */
        UA_DateTime eventTime = data->sourceTimestamp;
        if (eventTime == 0)
            eventTime = UA_DateTime_now();
        res = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                   &eventTime, &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set deactivating Time failed",
                                     UA_NodeId_clear(&conditionNode););

        /* Set ConfirmedState text to (Unconfirmed)*/
        UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, UNCONFIRMED_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, conditionNode, &value, fieldConfirmedStateQN);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition ConfirmedState failed",);
        return;
    }

    /* Check if enabled and retained */
    if(!isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledStateQN) ||
       !isRetained(server, &conditionNode)) {
        /* Set confirmedState/Id to false*/
        UA_Boolean idValue = false;
        UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
        res = setConditionVariableFieldProperty(&acd->driver, conditionNode, &value,
                                                fieldConfirmedStateQN, twoStateVariableIdQN);
        UA_NodeId_clear(&conditionNode);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set ConfirmedState/Id failed",);
        return;
    }

    /* Set confirming time */
    UA_DateTime confirmingTime = data->sourceTimestamp;
    if (confirmingTime == 0)
        confirmingTime = UA_DateTime_now();
    res = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                               &confirmingTime, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Confirming Time failed",
                                     UA_NodeId_clear(&conditionNode););

    /* Set Message */
    UA_LocalizedText message = UA_LOCALIZEDTEXT(LOCALE, CONFIRMED_MESSAGE);
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, conditionNode, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition Message failed",
                                 UA_NodeId_clear(&conditionNode););

    /* Set ConfirmedState text */
    UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, CONFIRMED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, conditionNode, &value, fieldConfirmedStateQN);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition ConfirmedState failed",
                                     UA_NodeId_clear(&conditionNode););

    /* Get conditionSource */
    UA_NodeId conditionSource;
    res = getNodeIdValueOfConditionField(server, &conditionNode,
                                         fieldSourceQN, &conditionSource);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "ConditionSource not found",
                                     UA_NodeId_clear(&conditionNode););

    /* User callback*/
    UA_Boolean removeBranch = false;
    res = callConditionTwoStateVariableCallback(acd, &conditionNode, &conditionSource,
                                                &removeBranch, UA_ENTERING_CONFIRMEDSTATE);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Calling condition callback failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););

    /* Trigger event */
    //Condition Nodes should not be deleted after triggering the event
    res = triggerConditionEvent(&acd->driver, conditionNode, conditionSource, NULL);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Triggering condition event failed",
                                 UA_NodeId_clear(&conditionNode);
                                 UA_NodeId_clear(&conditionSource););
}

static void
afterWriteCallbackActiveStateChange(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionContext,
                                    const UA_NodeId *nodeId, void *nodeContext,
                                    const UA_NumericRange *range, const UA_DataValue *data) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return;

    UA_Variant value;
    UA_NodeId twoStateVariableNode;
    UA_StatusCode res = getFieldParentNodeId(server, nodeId, &twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent TwoStateVariable found for given ActiveState/Id",);

    UA_NodeId conditionNode;
    res = getFieldParentNodeId(server, &twoStateVariableNode, &conditionNode);
    UA_NodeId_clear(&twoStateVariableNode);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent Condition found for given ActiveState",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    res = getNodeIdValueOfConditionField(server, &conditionNode,
                                                      fieldSourceQN, &conditionSource);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "ConditionSource not found",
                                 UA_NodeId_clear(&conditionNode););

    UA_Condition *cond = getCondition(acd, &conditionSource, &conditionNode);
    if(!cond) {
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, UA_STATUSCODE_BADNOTFOUND,
                                     "ActiveState transition check failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););
    }

    UA_ActiveState lastActiveState = cond->lastActiveState;
    UA_ActiveState activeState = cond->activeState;
    UA_Boolean isLimitalarm = cond->isLimitAlarm;

    UA_Boolean active = *((UA_Boolean *)data->value.data);
    if(!isLimitalarm) {
        UA_ActiveState nextActiveState = UA_INACTIVE;
        if(active)
            nextActiveState = UA_ACTIVE;
        cond->lastActiveState = activeState;
        cond->activeState = nextActiveState;
        cond->isLimitAlarm = false;
        lastActiveState = activeState;
        activeState = nextActiveState;
    }

    /* callback for change to true in ActiveState/Id property. first check
     * whether the value is true (ActiveState/Id == true). That check makes it
     * possible to set ActiveState/Id to false, without triggering an event */
    if(active && lastActiveState != activeState) {

        /* Check if enabled and retained */
        if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldEnabledStateQN) &&
           isRetained(server, &conditionNode)) {
            /* Set activating time */
            UA_DateTime activatingTime = data->sourceTimestamp;
            if (activatingTime == 0)
                activatingTime = UA_DateTime_now();
            res = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                          &activatingTime,
                                                          &UA_TYPES[UA_TYPES_DATETIME]);
            CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set activating Time failed",
                                         UA_NodeId_clear(&conditionNode);
                                         UA_NodeId_clear(&conditionSource););

            /* Set ActiveState text */
            UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, ACTIVE_TEXT);
            UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            res = setConditionField(&acd->driver, conditionNode, &value, fieldActiveStateQN);
            CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition ActiveState failed",
                                         UA_NodeId_clear(&conditionNode);
                                         UA_NodeId_clear(&conditionSource););

            /* User callback*/
            UA_Boolean removeBranch = false;//not used
            res = callConditionTwoStateVariableCallback(acd, &conditionNode,
                                                                     &conditionSource, &removeBranch,
                                                                     UA_ENTERING_ACTIVESTATE);
            CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Calling condition callback failed",
                                         UA_NodeId_clear(&conditionNode);
                                         UA_NodeId_clear(&conditionSource););

            /* Trigger event */
            //Condition Nodes should not be deleted after triggering the event
            res = triggerConditionEvent(&acd->driver, conditionNode, conditionSource, NULL);
            UA_NodeId_clear(&conditionNode);
            UA_NodeId_clear(&conditionSource);
            CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Triggering condition event failed",);
        } else {
            /* Set ActiveState/Id to false -> don't apply changes in case of
             * disabled or not retained*/
            UA_Boolean idValue = false;
            UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
            res = setConditionVariableFieldProperty(&acd->driver, conditionNode, &value,
                                                                 fieldActiveStateQN,
                                                                 twoStateVariableIdQN);
            UA_NodeId_clear(&conditionSource);
            UA_NodeId_clear(&conditionNode);
            CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set ActiveState/Id failed",);
        }
        return;
    }

    if(!active && lastActiveState != activeState) {
        /* Set ActiveState text to (Inactive) */
        UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, INACTIVE_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, conditionNode, &value, fieldActiveStateQN);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition ActiveState failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););

        /* Set deactivating time */
        UA_DateTime deactivatingTime = data->sourceTimestamp;
        if (deactivatingTime == 0)
            deactivatingTime = UA_DateTime_now();
        res = UA_Server_writeObjectProperty_scalar(server, conditionNode, fieldTimeQN,
                                                      &deactivatingTime,
                                                      &UA_TYPES[UA_TYPES_DATETIME]);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set deactivating Time failed",
                                     UA_NodeId_clear(&conditionNode);
                                     UA_NodeId_clear(&conditionSource););

        cond->lastActiveState = activeState;
        cond->activeState = UA_INACTIVE;
        cond->isLimitAlarm = isLimitalarm;
        UA_NodeId_clear(&conditionSource);
        UA_NodeId_clear(&conditionNode);
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
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return;

    UA_Variant value;

    UA_NodeId condition;
    UA_StatusCode res = getFieldParentNodeId(server, nodeId, &condition);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "No Parent Condition found for given Severity Field",);

    /* Get conditionSource */
    UA_NodeId conditionSource;
    res = getNodeIdValueOfConditionField(server, &condition,
                                                      fieldSourceQN, &conditionSource);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "ConditionSource not found",
                                 UA_NodeId_clear(&condition););

    UA_Condition *cond = getCondition(acd, &conditionSource, &condition);
    if(!cond) {
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, UA_STATUSCODE_BADNOTFOUND,
                                     "Get Condition LastSeverity failed",
                                     UA_NodeId_clear(&condition);
                                     UA_NodeId_clear(&conditionSource););
    }

    UA_UInt16 lastSeverity = cond->lastSeverity;
    UA_DateTime lastSeveritySourceTimeStamp = cond->lastSeveritySourceTimeStamp;

    /* Set message dependent on compare result*/
    UA_LocalizedText message;
    if(lastSeverity < (*(UA_UInt16 *)data->value.data))
        message = UA_LOCALIZEDTEXT(LOCALE, SEVERITY_INCREASED_MESSAGE);
    else
        message = UA_LOCALIZEDTEXT(LOCALE, SEVERITY_DECREASED_MESSAGE);

    /* Set LastSeverity */
    UA_Variant_setScalar(&value, &lastSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    res = setConditionField(&acd->driver, condition, &value, fieldLastSeverityQN);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition LAstSeverity failed",
                                 UA_NodeId_clear(&condition);
                                 UA_NodeId_clear(&conditionSource););

    /* Set SourceTimestamp */
    UA_Variant_setScalar(&value, &lastSeveritySourceTimeStamp, &UA_TYPES[UA_TYPES_DATETIME]);
    res = setConditionVariableFieldProperty(&acd->driver, condition, &value,
                                                  fieldLastSeverityQN,
                                                  fieldSourceTimestampQN);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set LastSeverity SourceTimestamp failed",
                                 UA_NodeId_clear(&condition);
                                 UA_NodeId_clear(&conditionSource););

    /* Update lastSeverity in list */
    cond->lastSeverity = *(UA_UInt16 *)data->value.data;
    cond->lastSeveritySourceTimeStamp = data->sourceTimestamp;

    /* Set Time (Time of Value Change) */
    UA_DateTime severityTime = data->sourceTimestamp;
    if (severityTime == 0)
        severityTime = UA_DateTime_now();
    UA_Variant_setScalar(&value, &severityTime, &UA_TYPES[UA_TYPES_DATETIME]);
    res = setConditionField(&acd->driver, condition, &value, fieldTimeQN);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition Time failed",
                                 UA_NodeId_clear(&condition););

    /* Set Message */
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, condition, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Set Condition Message failed",
                                 UA_NodeId_clear(&condition););

    /* Check if retained */
    if(isRetained(server, &condition)) {
        /* Trigger event */
        //Condition Nodes should not be deleted after triggering the event
        res = triggerConditionEvent(&acd->driver, condition, conditionSource, NULL);
        CONDITION_ASSERT_RETURN_VOID_ACD(acd, res, "Triggering condition event failed",
                                     UA_NodeId_clear(&condition);
        UA_NodeId_clear(&conditionSource););
    }
    UA_NodeId_clear(&conditionSource);
    UA_NodeId_clear(&condition);
}

/**************************************
 * Condition Methods and Refresh
 **************************************/

static UA_StatusCode
disableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize, UA_Variant *output) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_NodeId conditionTypeNodeId = UA_NS0ID(CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(acd->logging, UA_LOGCATEGORY_SERVER,
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
    UA_StatusCode res =
        setConditionVariableFieldProperty(&acd->driver, *objectId, &value,
                                                    fieldEnabledStateQN, twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Disable Condition failed",);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
enableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize,
                     const UA_Variant *input, size_t outputSize,
                     UA_Variant *output) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_NodeId conditionTypeNodeId = UA_NS0ID(CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(acd->logging, UA_LOGCATEGORY_SERVER,
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
    UA_StatusCode res =
        setConditionVariableFieldProperty(&acd->driver, *objectId, &value,
                                                    fieldEnabledStateQN, twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Enable Condition failed",);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addCommentMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *methodId,
                         void *methodContext, const UA_NodeId *objectId,
                         void *objectContext, size_t inputSize,
                         const UA_Variant *input, size_t outputSize,
                         UA_Variant *output) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    UA_EventLoop *el = UA_Server_getConfig(server)->eventLoop;

    UA_LocalizedText message;
    UA_NodeId triggerEvent;
    UA_Variant value;
    UA_DateTime fieldSourceTimeStampValue = el->dateTime_now(el);

    UA_NodeId conditionTypeNodeId = UA_NS0ID(CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(acd->logging, UA_LOGCATEGORY_SERVER,
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
    UA_StatusCode res =
        getConditionBranchNodeId(acd, (UA_ByteString *)input[0].data, &triggerEvent);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "ConditionId based on EventId not found",);

    /* Check if enabled */
    if(!isRetained(server, &triggerEvent))
        return UA_STATUSCODE_BADCONDITIONDISABLED;

    /* Set SourceTimestamp */
    UA_Variant_setScalar(&value, &fieldSourceTimeStampValue, &UA_TYPES[UA_TYPES_DATETIME]);
    res = setConditionVariableFieldProperty(&acd->driver, triggerEvent, &value,
                                                  fieldCommentQN,
                                                  fieldSourceTimestampQN);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Condition EnabledState text failed",
                                   UA_NodeId_clear(&triggerEvent););

    /* Set adding comment time (the same value of SourceTimestamp) */
    res = UA_Server_writeObjectProperty_scalar(server, triggerEvent, fieldTimeQN,
                                                  &fieldSourceTimeStampValue,
                                                  &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set enabling/disabling Time failed",
                                   UA_NodeId_clear(&triggerEvent););

    /* Set Message */
    message = UA_LOCALIZEDTEXT(LOCALE, COMMENT_MESSAGE);
    UA_Variant_setScalar(&value, &message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, triggerEvent, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Condition Message failed",
                                   UA_NodeId_clear(&triggerEvent););

    /* Set Comment. Check whether comment is empty -> leave the last value as is*/
    UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
    UA_String nullString = UA_STRING_NULL;
    if(!UA_String_equal(&inputComment->locale, &nullString) &&
       !UA_String_equal(&inputComment->text, &nullString)) {
        UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, triggerEvent, &value, fieldCommentQN);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Condition Comment failed",
                                       UA_NodeId_clear(&triggerEvent););
    }

    /* Get conditionSource */
    UA_NodeId conditionSource;
    res = getNodeIdValueOfConditionField(server, &triggerEvent,
                                                      fieldSourceQN, &conditionSource);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "ConditionSource not found",
                                   UA_NodeId_clear(&triggerEvent););

    /* Trigger event */
    //Condition Nodes should not be deleted after triggering the event
    res = triggerConditionEvent(&acd->driver, triggerEvent, conditionSource, NULL);
    UA_NodeId_clear(&conditionSource);
    UA_NodeId_clear(&triggerEvent);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Triggering condition event failed",);
    return res;
}

static UA_StatusCode
acknowledgeMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_Variant value;

    UA_NodeId conditionTypeNodeId = UA_NS0ID(CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(acd->logging, UA_LOGCATEGORY_SERVER,
                       "Cannot call method of ConditionType Node. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Get condition branch to trigger the correct event */
    UA_NodeId conditionNode;
    UA_StatusCode res =
        getConditionBranchNodeId(acd, (UA_ByteString *)input[0].data, &conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "ConditionId based on EventId not found",);

    /* Check if retained */
    if(!isRetained(server, &conditionNode))
        return UA_STATUSCODE_BADCONDITIONDISABLED;

    /* Check if already acknowledged */
    if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldAckedStateQN))
        return UA_STATUSCODE_BADCONDITIONBRANCHALREADYACKED;

    /* Get EventType */
    UA_NodeId eventType;
    res = getNodeIdValueOfConditionField(server, &conditionNode,
                                                      fieldEventTypeQN,
                                                      &eventType);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "EventType not found",
                                   UA_NodeId_clear(&conditionNode););

    /* Check if ConditionType is subType of AcknowledgeableConditionType TODO Over Kill*/
    UA_NodeId AcknowledgeableConditionTypeId = UA_NS0ID(ACKNOWLEDGEABLECONDITIONTYPE);
    UA_Boolean found = isSubtypeOf(server, &eventType, &AcknowledgeableConditionTypeId);
    if(!found) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                     "Condition Type must be a subtype of AcknowledgeableConditionType!");
        UA_NodeId_clear(&conditionNode);
        UA_NodeId_clear(&eventType);
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    UA_NodeId_clear(&eventType);

    /* Set Comment. Check whether comment is empty -> leave the last value as is*/
    UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
    UA_String nullString = UA_STRING_NULL;
    if(!UA_String_equal(&inputComment->locale, &nullString) &&
       !UA_String_equal(&inputComment->text, &nullString)) {
        UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, conditionNode, &value, fieldCommentQN);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Condition Comment failed",
                                       UA_NodeId_clear(&conditionNode););
    }

    /* Set AcknowledgeableStateId */
    UA_Boolean idValue = true;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = setConditionVariableFieldProperty(&acd->driver, conditionNode, &value,
                                               fieldAckedStateQN, twoStateVariableIdQN);
    UA_NodeId_clear(&conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Acknowledge Condition failed",);
    return res;
}

static UA_StatusCode
confirmMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_Variant value;

    UA_NodeId conditionTypeNodeId = UA_NS0ID(CONDITIONTYPE);
    if(UA_NodeId_equal(objectId, &conditionTypeNodeId)) {
        UA_LOG_WARNING(acd->logging, UA_LOGCATEGORY_SERVER,
                       "Cannot call method of ConditionType Node. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNODEIDINVALID));
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    /* Get condition branch to trigger the correct event */
    UA_NodeId conditionNode;
    UA_StatusCode res = getConditionBranchNodeId(acd, (UA_ByteString *)input[0].data, &conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "ConditionId based on EventId not found",);

    /* Check if retained */
    if(!isRetained(server, &conditionNode))
        return UA_STATUSCODE_BADCONDITIONDISABLED;

    /* Check if already confirmed */
    if(isTwoStateVariableInTrueState(server, &conditionNode, &fieldConfirmedStateQN))
        return UA_STATUSCODE_BADCONDITIONBRANCHALREADYCONFIRMED;

    /* Get EventType */
    UA_NodeId eventType;
    res = getNodeIdValueOfConditionField(server, &conditionNode, fieldEventTypeQN, &eventType);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "EventType not found",
                                       UA_NodeId_clear(&conditionNode););

    /* Check if ConditionType is subType of AcknowledgeableConditionType. */
    UA_NodeId AcknowledgeableConditionTypeId = UA_NS0ID(ACKNOWLEDGEABLECONDITIONTYPE);
    UA_Boolean found = isSubtypeOf(server, &eventType, &AcknowledgeableConditionTypeId);
    if(!found) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                     "Condition Type must be a subtype of AcknowledgeableConditionType!");
        UA_NodeId_clear(&conditionNode);
        UA_NodeId_clear(&eventType);
        return UA_STATUSCODE_BADNODEIDINVALID;
    }

    UA_NodeId_clear(&eventType);

    /* Set Comment. Check whether comment is empty -> leave the last value as is*/
    UA_LocalizedText *inputComment = (UA_LocalizedText *)input[1].data;
    UA_String nullString = UA_STRING_NULL;
    if(!UA_String_equal(&inputComment->locale, &nullString) &&
       !UA_String_equal(&inputComment->text, &nullString)) {
        UA_Variant_setScalar(&value, inputComment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, conditionNode, &value, fieldCommentQN);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Condition Comment failed",
                                           UA_NodeId_clear(&conditionNode););
    }

    /* Set ConfirmedStateId */
    UA_Boolean idValue = true;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = setConditionVariableFieldProperty(&acd->driver, conditionNode, &value,
                                            fieldConfirmedStateQN, twoStateVariableIdQN);
    UA_NodeId_clear(&conditionNode);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Acknowledge Condition failed",);
    return res;
}

static UA_Boolean
isConditionSourceInMonitoredItem(UA_Server *server, const UA_NodeId *itemToMonitorId,
                                 const UA_NodeId *conditionSource) {
    return hasRecursiveBrowsePath(server, conditionSource, itemToMonitorId,
                                  UA_NS0ID_ORGANIZES, UA_BROWSEDIRECTION_INVERSE, false) ||
           hasRecursiveBrowsePath(server, conditionSource, itemToMonitorId,
                                  UA_NS0ID_HASCOMPONENT, UA_BROWSEDIRECTION_INVERSE, false) ||
           hasRecursiveBrowsePath(server, conditionSource, itemToMonitorId,
                                  UA_NS0ID_HASEVENTSOURCE, UA_BROWSEDIRECTION_INVERSE, false) ||
           hasRecursiveBrowsePath(server, conditionSource, itemToMonitorId,
                                  UA_NS0ID_HASNOTIFIER, UA_BROWSEDIRECTION_INVERSE, false);
}

static UA_StatusCode
refreshLogic(UA_Server *server, AlarmsConditionsDriver *acd,
             const UA_NodeId *sessionId, const UA_UInt32 *subscriptionId,
             const UA_UInt32 *monitoredItemId, const UA_NodeId *itemToMonitorId) {
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    /* 1. Trigger RefreshStartEvent */
    UA_EventDescription ed = {0};
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = UA_NS0ID(REFRESHSTARTEVENTTYPE);
    ed.severity = REFRESHEVENT_SEVERITY_DEFAULT;
    ed.sessionId = sessionId;
    ed.subscriptionId = subscriptionId;
    ed.monitoredItemId = monitoredItemId;
    UA_StatusCode res = UA_Server_createEventEx(server, &ed, NULL);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Events: Could not add the event to a listening node",);

    /* 2. Refresh (see 5.5.7) */
    /* Get ConditionSource Entry */
    UA_ConditionSource *source;
    LIST_FOREACH(source, &acd->conditionSources, listEntry) {
        UA_NodeId conditionSource = source->conditionSourceId;
        UA_NodeId serverObjectNodeId = UA_NS0ID(SERVER);
        /* Check if the conditionSource is being monitored. If the Server Object
         * is being monitored, then all Events of all monitoredItems should be
         * refreshed */
        if(!UA_NodeId_equal(itemToMonitorId, &conditionSource) &&
           !UA_NodeId_equal(itemToMonitorId, &serverObjectNodeId) &&
           !isConditionSourceInMonitoredItem(server, itemToMonitorId, &conditionSource))
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

                UA_ByteString_clear(&branch->lastEventId);

                /* Add the event */
                ed.eventInstance = &triggeredNode;
                ed.sourceNode = conditionSource;
                ed.eventType = UA_NODEID_NULL; /* overwritten by the EventInstance */
                res = UA_Server_createEventEx(server, &ed, &branch->lastEventId);
                CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Events: Could not add the event to a listening node",);
            }
        }
    }

    /* 3. Trigger RefreshEndEvent */
    ed.eventInstance = NULL;
    ed.sourceNode = UA_NS0ID(SERVER);
    ed.eventType = UA_NS0ID(REFRESHENDEVENTTYPE);
    return UA_Server_createEventEx(server, &ed, NULL);
}

static UA_StatusCode
refresh2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    UA_UInt32 subscriptionId = *((UA_UInt32 *)input[0].data);
    UA_UInt32 monitoredItemId = *((UA_UInt32 *)input[1].data);

    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_ACMonitoredItemTarget *target =
        getMonitoredItemTarget(acd, sessionId, subscriptionId, monitoredItemId);
    if(!target)
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;

    // TODO when there are a lot of monitoreditems (not only events)?
    UA_StatusCode res =
        refreshLogic(server, acd, sessionId, &target->subscriptionId,
                     &target->monitoredItemId, &target->itemToMonitorId);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Could not refresh Condition",);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
refreshMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    UA_UInt32 subscriptionId = *((UA_UInt32 *)input[0].data);
    AlarmsConditionsDriver *acd = findAlarmsConditionsDriver(server);
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;

    UA_Boolean subscriptionFound = false;
    UA_ACMonitoredItemTarget *target;
    LIST_FOREACH(target, &acd->monitoredItemTargets, listEntry) {
        if(target->subscriptionId != subscriptionId ||
           !UA_NodeId_equal(&target->sessionId, sessionId))
            continue;

        subscriptionFound = true;
        if(target->attributeId != UA_ATTRIBUTEID_EVENTNOTIFIER)
            continue;

        UA_StatusCode res =
            refreshLogic(server, acd, sessionId, &target->subscriptionId,
                         &target->monitoredItemId, &target->itemToMonitorId);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Could not refresh Condition",);
    }

    if(!subscriptionFound)
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    return UA_STATUSCODE_GOOD;
}

/**************************************
 * Condition Setup Helpers
 **************************************/

/* Check whether the Condition Source Node has "EventSource" or one of its
 * subtypes inverse reference. */
static UA_Boolean
doesHasEventSourceReferenceExist(UA_Server *server, const UA_NodeId nodeToCheck) {
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.nodeId = nodeToCheck;
    bd.referenceTypeId = UA_NS0ID(HASEVENTSOURCE);
    bd.browseDirection = UA_BROWSEDIRECTION_INVERSE;
    bd.includeSubtypes = true;
    bd.nodeClassMask = 0;
    bd.resultMask = UA_BROWSERESULTMASK_NONE;
    UA_BrowseResult br = UA_Server_browse(server, 0, &bd);
    UA_Boolean found =
        (br.statusCode == UA_STATUSCODE_GOOD && br.referencesSize > 0);
    UA_BrowseResult_clear(&br);
    return found;
}

static UA_StatusCode
addOptionalVariableField(UA_Server *server, const UA_NodeId *originCondition,
                         const UA_QualifiedName *fieldName,
                         const UA_NodeId *optionalVariableFieldNodeId,
                         UA_NodeId *outOptionalVariable) {
    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    UA_StatusCode res =
        UA_Server_readDisplayName(server, *optionalVariableFieldNodeId, &vAttr.displayName);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    res = UA_Server_readValueRank(server, *optionalVariableFieldNodeId, &vAttr.valueRank);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LocalizedText_clear(&vAttr.displayName);
        return res;
    }

    res = UA_Server_readDataType(server, *optionalVariableFieldNodeId, &vAttr.dataType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LocalizedText_clear(&vAttr.displayName);
        return res;
    }

    UA_NodeId typeDefinitionId = UA_NODEID_NULL;
    res = getTypeDefinitionId(server, optionalVariableFieldNodeId, &typeDefinitionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                       "Invalid VariableType. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADTYPEDEFINITIONINVALID));
        UA_NodeId_clear(&vAttr.dataType);
        UA_LocalizedText_clear(&vAttr.displayName);
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

    /* Set referenceType to parent */
    UA_NodeId referenceToParent;
    UA_NodeId propertyTypeNodeId = UA_NS0ID(PROPERTYTYPE);
    if(UA_NodeId_equal(&typeDefinitionId, &propertyTypeNodeId))
        referenceToParent = UA_NS0ID(HASPROPERTY);
    else
        referenceToParent = UA_NS0ID(HASCOMPONENT);

    /* Set a random unused NodeId with specified Namespace Index*/
    UA_NodeId optionalVariable = {originCondition->namespaceIndex, UA_NODEIDTYPE_NUMERIC, {0}};
    res = UA_Server_addVariableNode(server, optionalVariable, *originCondition, referenceToParent,
                                    *fieldName, typeDefinitionId, vAttr, NULL, outOptionalVariable);
    UA_NodeId_clear(&typeDefinitionId);
    UA_NodeId_clear(&vAttr.dataType);
    UA_LocalizedText_clear(&vAttr.displayName);
    return res;
}

static UA_StatusCode
addOptionalObjectField(UA_Server *server, const UA_NodeId *originCondition,
                       const UA_QualifiedName* fieldName,
                       const UA_NodeId *optionalObjectFieldNodeId,
                       UA_NodeId *outOptionalObject) {

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    UA_StatusCode res =
        UA_Server_readDisplayName(server, *optionalObjectFieldNodeId, &oAttr.displayName);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_NodeId typeDefinitionId = UA_NODEID_NULL;
    res = getTypeDefinitionId(server, optionalObjectFieldNodeId, &typeDefinitionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                       "Invalid ObjectType. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADTYPEDEFINITIONINVALID));
        UA_LocalizedText_clear(&oAttr.displayName);
        return UA_STATUSCODE_BADTYPEDEFINITIONINVALID;
    }

    /* Set referenceType to parent */
    UA_NodeId referenceToParent;
    UA_NodeId propertyTypeNodeId = UA_NS0ID(PROPERTYTYPE);
    if(UA_NodeId_equal(&typeDefinitionId, &propertyTypeNodeId))
        referenceToParent = UA_NS0ID(HASPROPERTY);
    else
        referenceToParent = UA_NS0ID(HASCOMPONENT);

    UA_NodeId optionalObject = {originCondition->namespaceIndex, UA_NODEIDTYPE_NUMERIC, {0}};
    res = UA_Server_addObjectNode(server, optionalObject, *originCondition, referenceToParent,
                                  *fieldName, typeDefinitionId, oAttr, NULL, outOptionalObject);
    UA_NodeId_clear(&typeDefinitionId);
    UA_LocalizedText_clear(&oAttr.displayName);
    return res;
}

static UA_StatusCode
addConditionOptionalField(UA_AlarmConditionsDriver *driver,
                          const UA_NodeId condition, const UA_NodeId conditionType,
                          const UA_QualifiedName fieldName, UA_NodeId *outOptionalNode) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    /* Get optional Field NodId from ConditionType -> user should give the
     * correct ConditionType or Subtype!!!! */
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, conditionType, 1, &fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;

    UA_NodeId optionalFieldNodeId = bpr.targets[0].targetId.nodeId;
    UA_NodeClass optionalFieldNodeClass = UA_NODECLASS_UNSPECIFIED;
    UA_StatusCode res =
        UA_Server_readNodeClass(server, optionalFieldNodeId, &optionalFieldNodeClass);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(acd->logging, UA_LOGCATEGORY_SERVER,
                       "Couldn't find optional Field Node in ConditionType. StatusCode %s",
                       UA_StatusCode_name(res));
        UA_BrowsePathResult_clear(&bpr);
        return res;
    }

    switch(optionalFieldNodeClass) {
    case UA_NODECLASS_VARIABLE:
        res = addOptionalVariableField(server, &condition, &fieldName,
                                       &optionalFieldNodeId, outOptionalNode);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                         "Adding Condition Optional Variable Field failed. StatusCode %s",
                         UA_StatusCode_name(res));
        }
        UA_BrowsePathResult_clear(&bpr);
        return res;
    case UA_NODECLASS_OBJECT:
        res = addOptionalObjectField(server, &condition, &fieldName,
                                     &optionalFieldNodeId, outOptionalNode);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                         "Adding Condition Optional Object Field failed. StatusCode %s",
                         UA_StatusCode_name(res));
        }
        UA_BrowsePathResult_clear(&bpr);
        return res;
    case UA_NODECLASS_METHOD:
        /* TODO method: Check first logic of creating methods at all (should
         * we create a new method or just reference it from the ConditionType?) */
        UA_BrowsePathResult_clear(&bpr);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    default:
        UA_BrowsePathResult_clear(&bpr);
        return UA_STATUSCODE_BADNOTSUPPORTED;
    }
}

static UA_StatusCode
setStandardConditionFields(AlarmsConditionsDriver *acd, const UA_NodeId* condition,
                           const UA_NodeId* conditionType, const UA_NodeId* conditionSource,
                           const UA_QualifiedName* conditionName) {
    UA_Server *server = acd->driver.drv.server;

    /* Set Fields */
    /* 1.Set EventType */
    UA_Variant value;
    UA_Variant_setScalar(&value, (void*)(uintptr_t)conditionType, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode res = setConditionField(&acd->driver, *condition, &value, fieldEventTypeQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set EventType Field failed",);

    /* 2.Set ConditionName */
    UA_Variant_setScalar(&value, (void*)(uintptr_t)&conditionName->name, &UA_TYPES[UA_TYPES_STRING]);
    res = setConditionField(&acd->driver, *condition, &value, fieldConditionNameQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set ConditionName Field failed",);

    /* 3.Set EnabledState (Disabled by default -> Retain Field = false) */
    UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, DISABLED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, *condition, &value, fieldEnabledStateQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set EnabledState Field failed",);

    /* 4.Set EnabledState/Id */
    UA_Boolean stateId = false;
    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = setConditionVariableFieldProperty(&acd->driver, *condition, &value,
                                            fieldEnabledStateQN, twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set EnabledState/Id Field failed",);

    /* 5.Set Retain*/
    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = setConditionField(&acd->driver, *condition, &value, fieldRetainQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set Retain Field failed",);

    /* 6.Set SourceName*/
    UA_QualifiedName sourceName;
    res = UA_Server_readBrowseName(server, *conditionSource, &sourceName);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                       "Couldn't find ConditionSourceNode. StatusCode %s", UA_StatusCode_name(res));
        return UA_STATUSCODE_BADNOTFOUND;
    }

    UA_Variant_setScalar(&value, (void*)(uintptr_t)&sourceName.name, &UA_TYPES[UA_TYPES_STRING]);
    res = setConditionField(&acd->driver, *condition, &value, fieldSourceNameQN);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                     "Set SourceName Field failed. StatusCode %s",
                     UA_StatusCode_name(res));
        UA_QualifiedName_clear(&sourceName);
        return res;
    }
    UA_QualifiedName_clear(&sourceName);

    /* 7.Set SourceNode*/
    UA_Variant_setScalar(&value, (void*)(uintptr_t)conditionSource, &UA_TYPES[UA_TYPES_NODEID]);
    res = setConditionField(&acd->driver, *condition, &value, fieldSourceQN);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Server_getConfig(server)->logging, UA_LOGCATEGORY_SERVER,
                     "Set SourceNode Field failed. StatusCode %s", UA_StatusCode_name(res));
        return res;
    }

    /* 8. Set Quality (TODO not supported, thus set with Status Good) */
    UA_StatusCode qualityValue = UA_STATUSCODE_GOOD;
    UA_Variant_setScalar(&value, &qualityValue, &UA_TYPES[UA_TYPES_STATUSCODE]);
    res = setConditionField(&acd->driver, *condition, &value, fieldQualityQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set Quality Field failed",);

    /* 9. Set Severity */
    UA_UInt16 severityValue = 0;
    UA_Variant_setScalar(&value, &severityValue, &UA_TYPES[UA_TYPES_UINT16]);
    res = setConditionField(&acd->driver, *condition, &value, fieldSeverityQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set Severity Field failed",);

    /* Check subTypes of ConditionType to set further Fields*/

    /* 1. Check if ConditionType is subType of AcknowledgeableConditionType */
    UA_NodeId acknowledgeableConditionTypeId = UA_NS0ID(ACKNOWLEDGEABLECONDITIONTYPE);
    if(!isSubtypeOf(server, conditionType, &acknowledgeableConditionTypeId))
        return UA_STATUSCODE_GOOD;

    /* Set AckedState (Id = false by default) */
    text = UA_LOCALIZEDTEXT(LOCALE, UNACKED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, *condition, &value, fieldAckedStateQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set AckedState Field failed",);

    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = setConditionVariableFieldProperty(&acd->driver, *condition, &value,
                                            fieldAckedStateQN, twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set AckedState/Id Field failed",);

    /* add optional field ConfirmedState*/
    res = addConditionOptionalField(&acd->driver, *condition, acknowledgeableConditionTypeId,
                                    fieldConfirmedStateQN, NULL);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Adding ConfirmedState optional Field failed",);

    /* Set ConfirmedState (Id = false by default) */
    text = UA_LOCALIZEDTEXT(LOCALE, UNCONFIRMED_TEXT);
    UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res = setConditionField(&acd->driver, *condition, &value, fieldConfirmedStateQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set ConfirmedState Field failed",);

    UA_Variant_setScalar(&value, &stateId, &UA_TYPES[UA_TYPES_BOOLEAN]);
    res = setConditionVariableFieldProperty(&acd->driver, *condition, &value,
                                            fieldConfirmedStateQN, twoStateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set EnabledState/Id Field failed",);

    /* Add  optional property for certificate expiration alarm type*/
    UA_NodeId certificateConditionTypeId = UA_NS0ID(CERTIFICATEEXPIRATIONALARMTYPE);
    if(isSubtypeOf(server, conditionType, &certificateConditionTypeId)) {
        res = addConditionOptionalField(&acd->driver, *condition, certificateConditionTypeId,
                                        fieldExpirationLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Adding Expiration Limit optional field failed",);

        /* Set the default value for the Expiration limit property */
        UA_Duration defaultValue = EXPIRATION_LIMIT_DEFAULT_VALUE;
        res |= UA_Server_writeObjectProperty_scalar(server, *condition, fieldExpirationLimitQN,
                                                    &defaultValue, &UA_TYPES[UA_TYPES_DURATION]);

    }

    /* 2. Check if ConditionType is subType of AlarmConditionType */
    UA_NodeId alarmConditionTypeId = UA_NS0ID(ALARMCONDITIONTYPE);
    if(isSubtypeOf(server, conditionType, &alarmConditionTypeId)) {
        /* Set ActiveState (Id = false by default) */
        text = UA_LOCALIZEDTEXT(LOCALE, INACTIVE_TEXT);
        UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
        res = setConditionField(&acd->driver, *condition, &value, fieldActiveStateQN);
        CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set ActiveState Field failed",);
    }

    /* 3. Check if the ConditionType is subType of LimitAlarmType */
    UA_NodeId LimitAlarmTypeId = UA_NS0ID(LIMITALARMTYPE);
    if(!isSubtypeOf(server, conditionType, &LimitAlarmTypeId))
        return res;

    /* Set optional field property. For the LimitAlarm and its subtypes, atleast
     * one limit is mandatory */
    /* Add optional field LowLimit */
    res = addConditionOptionalField(&acd->driver, *condition, LimitAlarmTypeId, fieldLowLimitQN, NULL);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Adding LowLimit optional Field failed",);

    /* Add optional field HighLimit */
    res = addConditionOptionalField(&acd->driver, *condition, LimitAlarmTypeId, fieldHighLimitQN, NULL);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Adding HighLimit optional Field failed",);

    /* Add optional field HighHighLimit */
    res = addConditionOptionalField(&acd->driver, *condition, LimitAlarmTypeId, fieldHighHighLimitQN, NULL);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Adding HighLimit optional Field failed",);

    /* Add optional field LowLowLimit */
    res = addConditionOptionalField(&acd->driver, *condition, LimitAlarmTypeId, fieldLowLowLimitQN, NULL);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Adding LowLowLimit optional Field failed",);

    /* 4. Check if the ConditionType is subType of RateOfChangeAlarmType */
    UA_NodeId RateOfChangeAlarmTypeId = UA_NS0ID(EXCLUSIVERATEOFCHANGEALARMTYPE);
    if(!isSubtypeOf(server, conditionType, &RateOfChangeAlarmTypeId))
        return res;

    res = addConditionOptionalField(&acd->driver, *condition, RateOfChangeAlarmTypeId,
                                    fieldEngineeringUnitsQN, NULL);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Adding EngineeringUnit optional Field failed",);

    return res;
}

/* Set callbacks for TwoStateVariable Fields of a condition */
static UA_StatusCode
setTwoStateVariableCallbacks(UA_Server *server, const UA_NodeId* condition,
                             const UA_NodeId* conditionType) {

    /* Set EnabledState Callback */
    UA_NodeId twoStateVariableIdNodeId = UA_NODEID_NULL;
    UA_StatusCode res = getConditionFieldPropertyNodeId(server, condition, &fieldEnabledStateQN,
                                                        &twoStateVariableIdQN, &twoStateVariableIdNodeId);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Id Property of TwoStateVariable not found",);

    UA_ValueSourceNotifications callback;
    callback.onRead = NULL;
    callback.onWrite = afterWriteCallbackEnabledStateChange;
    res = UA_Server_setVariableNode_internalValueSource(server, twoStateVariableIdNodeId,
                                                        NULL, &callback);
    CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set EnabledState Callback failed",
                                          UA_NodeId_clear(&twoStateVariableIdNodeId););

    /* Set AckedState Callback */
    /* Check if ConditionType is subType of AcknowledgeableConditionType */
    UA_NodeId acknowledgeableConditionTypeId = UA_NS0ID(ACKNOWLEDGEABLECONDITIONTYPE);
    if(isSubtypeOf(server, conditionType, &acknowledgeableConditionTypeId)) {
        UA_NodeId_clear(&twoStateVariableIdNodeId);
        res = getConditionFieldPropertyNodeId(server, condition, &fieldAckedStateQN,
                                              &twoStateVariableIdQN, &twoStateVariableIdNodeId);
        CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Id Property of TwoStateVariable not found",);

        callback.onWrite = afterWriteCallbackAckedStateChange;
        res = UA_Server_setVariableNode_internalValueSource(server, twoStateVariableIdNodeId,
                                                     NULL, &callback);
        CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set AckedState Callback failed",
                                              UA_NodeId_clear(&twoStateVariableIdNodeId););

        /* add callback */
        callback.onWrite = afterWriteCallbackConfirmedStateChange;
        UA_NodeId_clear(&twoStateVariableIdNodeId);
        res = getConditionFieldPropertyNodeId(server, condition, &fieldConfirmedStateQN,
                                              &twoStateVariableIdQN, &twoStateVariableIdNodeId);
        CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Id Property of TwoStateVariable not found",);

        /* add reference from Condition to Confirm Method */
        UA_NodeId hasComponent = UA_NS0ID(HASCOMPONENT);
        UA_NodeId confirm = UA_NS0ID(ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM);
        res = UA_Server_addReference(server, *condition, hasComponent,
                                        UA_EXPANDEDNODEID_NODEID(confirm), true);
        CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res,
                                              "Adding HasComponent Reference to Confirm Method failed",
                                              UA_NodeId_clear(&twoStateVariableIdNodeId););

        res = UA_Server_setVariableNode_internalValueSource(server, twoStateVariableIdNodeId,
                                                     NULL, &callback);
        CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Adding ConfirmedState/Id callback failed",
                                              UA_NodeId_clear(&twoStateVariableIdNodeId););

        /* Set ActiveState Callback */
        /* Check if ConditionType is subType of AlarmConditionType */
        UA_NodeId alarmConditionTypeId = UA_NS0ID(ALARMCONDITIONTYPE);
        if(isSubtypeOf(server, conditionType, &alarmConditionTypeId)) {
            UA_NodeId_clear(&twoStateVariableIdNodeId);
            res = getConditionFieldPropertyNodeId(server, condition, &fieldActiveStateQN,
                                                  &twoStateVariableIdQN, &twoStateVariableIdNodeId);
            CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Id Property of TwoStateVariable not found",);

            callback.onWrite = afterWriteCallbackActiveStateChange;
            res = UA_Server_setVariableNode_internalValueSource(server, twoStateVariableIdNodeId,
                                                                NULL, &callback);
            CONDITION_ASSERT_RETURN_RETVAL_SERVER(server, res, "Set ActiveState Callback failed",
                                                  UA_NodeId_clear(&twoStateVariableIdNodeId););
        }
    }

    UA_NodeId_clear(&twoStateVariableIdNodeId);
    return res;
}

/* Set callbacks for ConditionVariable Fields of a condition */
static UA_StatusCode
setConditionVariableCallbacks(UA_Server *server, const UA_NodeId *condition,
                              const UA_NodeId *conditionType) {

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    UA_QualifiedName conditionVariableName[2] = {
        fieldQualityQN,
        fieldSeverityQN
    };// extend array with other fields when needed

    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, *condition, 1, &conditionVariableName[0]);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;
    UA_ValueSourceNotifications callback ;
    callback.onRead = NULL;
    callback.onWrite = afterWriteCallbackQualityChange;
    res = UA_Server_setVariableNode_internalValueSource(server, bpr.targets[0].targetId.nodeId,
                                                        NULL, &callback);
    UA_BrowsePathResult_clear(&bpr);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    bpr = UA_Server_browseSimplifiedBrowsePath(server, *condition, 1, &conditionVariableName[1]);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;
    callback.onWrite = afterWriteCallbackSeverityChange;
    res = UA_Server_setVariableNode_internalValueSource(server, bpr.targets[0].targetId.nodeId,
                                                        NULL, &callback);
    UA_BrowsePathResult_clear(&bpr);
    return res;
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
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_ACKNOWLEDGE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM}}
    };

    UA_StatusCode res = UA_STATUSCODE_GOOD;
    res |= UA_Server_setMethodNodeCallback(server, methodId[0], disableMethodCallback);
    res |= UA_Server_setMethodNodeCallback(server, methodId[1], enableMethodCallback);
    res |= UA_Server_setMethodNodeCallback(server, methodId[2], addCommentMethodCallback);
    res |= UA_Server_setMethodNodeCallback(server, methodId[3], refreshMethodCallback);
    res |= UA_Server_setMethodNodeCallback(server, methodId[4], refresh2MethodCallback);
    res |= UA_Server_setMethodNodeCallback(server, methodId[5], acknowledgeMethodCallback);
    res |= UA_Server_setMethodNodeCallback(server, methodId[6], confirmMethodCallback);

    return res;
}

static UA_StatusCode
setStandardConditionCallbacks(AlarmsConditionsDriver *acd, const UA_NodeId* condition,
                              const UA_NodeId* conditionType) {
    if(!acd)
        return UA_STATUSCODE_BADNOTSUPPORTED;
    UA_Server *server = acd->driver.drv.server;

    UA_StatusCode res = setTwoStateVariableCallbacks(server, condition, conditionType);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set TwoStateVariable Callback failed",);

    res = setConditionVariableCallbacks(server, condition, conditionType);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set ConditionVariable Callback failed",);

    /* Set callbacks for Method Components (needs to be set only once!) */
    if(LIST_EMPTY(&acd->conditionSources)) {
        res = setConditionMethodCallbacks(server, condition, conditionType);
        CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Set Method Callback failed",);
    }

    return res;
}

/**************************************
 * Driver API
 **************************************/

static UA_StatusCode
addConditionBegin(UA_AlarmConditionsDriver *driver,
                  const UA_NodeId conditionId,
                  const UA_NodeId conditionType,
                  const UA_QualifiedName conditionName,
                  UA_NodeId *outNodeId) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    if(!outNodeId) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                     "outNodeId cannot be NULL!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Make sure the conditionType is a Subtype of ConditionType */
    UA_NodeId conditionTypeId = UA_NS0ID(CONDITIONTYPE);
    UA_Boolean found = isSubtypeOf(server, &conditionType, &conditionTypeId);
    if(!found) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                     "Condition Type must be a subtype of ConditionType!");
        return UA_STATUSCODE_BADNOMATCH;
    }

    /* Create an ObjectNode which represents the condition */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName.locale = UA_STRING("en");
    oAttr.displayName.text = conditionName.name;
    UA_StatusCode res =
        UA_Server_addNode_begin(server, UA_NODECLASS_OBJECT, conditionId,
                                UA_NODEID_NULL, UA_NODEID_NULL, conditionName,
                                conditionType, &oAttr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                                NULL, outNodeId);
    CONDITION_ASSERT_RETURN_RETVAL_ACD(acd, res, "Adding Condition failed", );
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
addConditionFinish(UA_AlarmConditionsDriver *driver,
                   const UA_NodeId conditionId,
                   const UA_NodeId conditionSource,
                   const UA_NodeId hierarchialReferenceType) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    UA_QualifiedName browseName;
    UA_StatusCode res =
        UA_Server_readBrowseName(server, conditionId, &browseName);
    if(res != UA_STATUSCODE_GOOD)
        return res;

    UA_NodeId conditionType = UA_NODEID_NULL;
    res = getTypeDefinitionId(server, &conditionId, &conditionType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_QualifiedName_clear(&browseName);
        return res;
    }

    res = UA_Server_addNode_finish(server, conditionId);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                     "Finish node failed. StatusCode %s", UA_StatusCode_name(res));
        goto cleanup;
    }

    /* Make sure the ConditionSource has HasEventSource or one of its SubTypes ReferenceType */
    UA_NodeId serverObject = UA_NS0ID(SERVER);
    if(!doesHasEventSourceReferenceExist(server, conditionSource) &&
       !UA_NodeId_equal(&serverObject, &conditionSource)) {
         UA_NodeId hasHasEventSourceId = UA_NS0ID(HASEVENTSOURCE);
         res = UA_Server_addReference(server, serverObject, hasHasEventSourceId,
                                      UA_EXPANDEDNODEID_NODEID(conditionSource), true);
         if(res != UA_STATUSCODE_GOOD) {
             UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                          "Creating HasHasEventSource Reference to the Server Object "
                          "failed. StatusCode %s", UA_StatusCode_name(res));
             goto cleanup;
         }
    }

    /* create HasCondition Reference (HasCondition should be forward from the
     * ConditionSourceNode to the Condition. else, HasCondition should be
     * forward from the ConditionSourceNode to the ConditionType Node) */
    UA_NodeId hasCondition = UA_NS0ID(HASCONDITION);
    if(!UA_NodeId_isNull(&hierarchialReferenceType)) {
        /* Create hierarchical Reference to ConditionSource to expose the
         * ConditionNode in Address Space */
        res = UA_Server_addReference(server, conditionSource, hierarchialReferenceType,
                                     UA_EXPANDEDNODEID_NODEID(conditionId), true);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                         "Creating hierarchical Reference to ConditionSource failed. "
                         "StatusCode %s", UA_StatusCode_name(res));
            goto cleanup;
        }

        res = UA_Server_addReference(server, conditionSource, hasCondition,
                                     UA_EXPANDEDNODEID_NODEID(conditionId), true);
        if(res != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                         "Creating HasCondition Reference failed. StatusCode %s",
                         UA_StatusCode_name(res));
            goto cleanup;
        }
    } else {
        res = UA_Server_addReference(server, conditionSource, hasCondition,
                                     UA_EXPANDEDNODEID_NODEID(conditionType), true);
        if(res != UA_STATUSCODE_GOOD &&
           res != UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED) {
            UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                         "Creating HasCondition Reference failed. StatusCode %s",
                         UA_StatusCode_name(res));
            goto cleanup;
        }
    }

    /* Set standard fields */
    res = setStandardConditionFields(acd, &conditionId, &conditionType,
                                     &conditionSource, &browseName);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                     "Set standard Condition Fields failed. StatusCode %s",
                     UA_StatusCode_name(res));
        goto cleanup;
    }

    /* Set Method Callbacks */
    res = setStandardConditionCallbacks(acd, &conditionId, &conditionType);
    if(res != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                     "Set Condition callbacks failed. StatusCode %s",
                     UA_StatusCode_name(res));
        goto cleanup;
    }

    /* change Refresh Events IsAbstract = false
     * so abstract Events : RefreshStart and RefreshEnd could be created */
    UA_NodeId refreshStartEventTypeNodeId = UA_NS0ID(REFRESHSTARTEVENTTYPE);
    UA_NodeId refreshEndEventTypeNodeId = UA_NS0ID(REFRESHENDEVENTTYPE);

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
    res = appendConditionEntry(acd, &conditionId, &conditionSource);

cleanup:
    UA_NodeId_clear(&conditionType);
    UA_QualifiedName_clear(&browseName);
    return res;
}

static UA_StatusCode
createCondition(UA_AlarmConditionsDriver *driver,
                const UA_NodeId conditionId, const UA_NodeId conditionType,
                const UA_QualifiedName conditionName, const UA_NodeId conditionSource,
                const UA_NodeId hierarchialReferenceType, UA_NodeId *outNodeId) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_StatusCode res = addConditionBegin(driver, conditionId, conditionType, conditionName, outNodeId);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    return addConditionFinish(driver, *outNodeId, conditionSource, hierarchialReferenceType);
}

static UA_StatusCode
setConditionTwoStateVariableCallback(UA_AlarmConditionsDriver *driver, const UA_NodeId condition,
                                     const UA_NodeId conditionSource, UA_Boolean removeBranch,
                                     UA_TwoStateVariableChangeCallback callback,
                                     UA_TwoStateVariableCallbackType callbackType) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;

    UA_Condition *c = getCondition(acd, &conditionSource, &condition);
    if(!c)
        return UA_STATUSCODE_BADNOTFOUND;

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
deleteCondition(UA_AlarmConditionsDriver *driver, const UA_NodeId condition,
                const UA_NodeId conditionSource) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    /* Get ConditionSource Entry */
    UA_Boolean found = false; /* Delete from internal list */
    UA_ConditionSource *source, *tmp_source;
    LIST_FOREACH_SAFE(source, &acd->conditionSources, listEntry, tmp_source) {
        if(!UA_NodeId_equal(&source->conditionSourceId, &conditionSource))
            continue;

        /* Get Condition Entry */
        UA_Condition *cond, *tmp_cond;
        LIST_FOREACH_SAFE(cond, &source->conditions, listEntry, tmp_cond) {
            if(!UA_NodeId_equal(&cond->conditionId, &condition))
                continue;
            deleteConditionState(cond);
            found = true;
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

typedef struct {
    const UA_QualifiedName *fieldName;
    char *activeText;
    UA_Boolean upperLimit;
} LimitStateDefinition;

static const LimitStateDefinition limitStateDefinitions[] = {
    {&fieldHighHighLimitQN, ACTIVE_HIGHHIGH_TEXT, true},
    {&fieldHighLimitQN, ACTIVE_HIGH_TEXT, true},
    {&fieldLowLowLimitQN, ACTIVE_LOWLOW_TEXT, false},
    {&fieldLowLimitQN, ACTIVE_LOW_TEXT, false}
};

static UA_StatusCode
getLimit(UA_Server *server, UA_NodeId conditionId,
         const UA_QualifiedName *limitName, UA_Double *limit) {
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode res =
        UA_Server_readObjectProperty(server, conditionId, *limitName, &value);
    if(res == UA_STATUSCODE_GOOD)
        *limit = *(UA_Double*)value.data;
    UA_Variant_clear(&value);
    return res;
}

static UA_Boolean
isLimitActive(UA_Double value, UA_Double limit, UA_Boolean upperLimit) {
    if(upperLimit)
        return value >= limit;
    return value <= limit;
}

static UA_StatusCode
setLimitState(UA_AlarmConditionsDriver *driver, const UA_NodeId conditionId,
              UA_Double limitValue) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    UA_NodeId limitState = UA_NODEID_NULL;
    UA_Variant value;
    UA_QualifiedName stateField = UA_QUALIFIEDNAME(0,"CurrentState");
    UA_QualifiedName stateIdField = UA_QUALIFIEDNAME(0,"Id");
    UA_StatusCode res = getConditionFieldNodeId(server, &conditionId,
                                                &fieldLimitStateQN, &limitState);

    for(size_t i = 0;
        i < sizeof(limitStateDefinitions) / sizeof(limitStateDefinitions[0]); i++) {
        const LimitStateDefinition *definition = &limitStateDefinitions[i];
        UA_Double limit;
        res |= getLimit(server, conditionId, definition->fieldName, &limit);
        if(res == UA_STATUSCODE_GOOD &&
           isLimitActive(limitValue, limit, definition->upperLimit)) {
            UA_NodeId activeLimitId = UA_NODEID_NULL;
            res |= getConditionFieldNodeId(server, &conditionId,
                                           definition->fieldName, &activeLimitId);
            UA_LocalizedText text = UA_LOCALIZEDTEXT(LOCALE, definition->activeText);
            UA_Variant_setScalar(&value, &text, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
            res |= setConditionField(&acd->driver, limitState, &value, stateField);
            UA_Variant_setScalar(&value, &activeLimitId, &UA_TYPES[UA_TYPES_NODEID]);
            res |= setConditionVariableFieldProperty(&acd->driver, limitState, &value,
                                                     stateField, stateIdField);
            return res;
        }
    }

    UA_LocalizedText textNull = UA_LOCALIZEDTEXT("", "");
    UA_Variant_setScalar(&value, &textNull, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    res |= setConditionField(&acd->driver, limitState, &value, stateField);

    UA_NodeId nodeIdNull = UA_NODEID_NULL;
    UA_Variant_setScalar(&value, &nodeIdNull, &UA_TYPES[UA_TYPES_NODEID]);
    res |= setConditionVariableFieldProperty(&acd->driver, limitState, &value,
                                             stateField, stateIdField);
    return res;
}

/* Currently supports only MBEDTLS and OpenSSL */
static UA_StatusCode
setExpirationDate(UA_AlarmConditionsDriver *driver,
                  const UA_NodeId conditionId, UA_ByteString cert) {
    AlarmsConditionsDriver *acd = getStartedDriver(driver);
    if(!acd)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Server *server = acd->driver.drv.server;

    UA_StatusCode res;
    if(cert.data == NULL) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                    "No Certificate found.");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    UA_CertificateGroup *cv = &UA_Server_getConfig(server)->sessionPKI;
    UA_DateTime getExpiryDateAndTime = 0;
    if(cv == NULL) {
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                    "Certificate verification is not registered");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    res = UA_CertificateUtils_getExpirationDate(&cert, &getExpiryDateAndTime);
    if(res != UA_STATUSCODE_GOOD || getExpiryDateAndTime == 0){
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                    "Failed to get certificate expiration date");
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    /* Write the expiry date to the propery of the condition instance */
    res = UA_Server_writeObjectProperty_scalar(server, conditionId, fieldExpirationDateQN,
                                               &getExpiryDateAndTime, &UA_TYPES[UA_TYPES_DATETIME]);
    return res;
}

/**************************************
 * Lifecycle and Constructor
 **************************************/

static UA_StatusCode
AlarmsConditionsDriver_start(UA_Driver *drv) {
    if(!drv->server)
        return UA_STATUSCODE_BADINTERNALERROR;
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    AlarmsConditionsDriver *acd = (AlarmsConditionsDriver*)drv;
    if(!acd->logging)
        acd->logging = UA_Server_getConfig(drv->server)->logging;

    for(UA_Driver *existing = UA_Server_getDrivers(drv->server);
        existing; existing = existing->next) {
        if(existing == drv ||
           existing->server != drv->server ||
           existing->state != UA_LIFECYCLESTATE_STARTED ||
           !isAlarmsConditionsDriver(existing))
            continue;
        UA_LOG_ERROR(acd->logging, UA_LOGCATEGORY_SERVER,
                     "Cannot add the driver \"%S\". The Alarms and Conditions "
                     "driver is already loaded", drv->name);
        return UA_STATUSCODE_BADALREADYEXISTS;
    }

    drv->state = UA_LIFECYCLESTATE_STARTED;
    return UA_STATUSCODE_GOOD;
}

static void
AlarmsConditionsDriver_stop(UA_Driver *drv) {
    drv->state = UA_LIFECYCLESTATE_STOPPED;
}

static UA_StatusCode
AlarmsConditionsDriver_free(UA_Driver *drv) {
    if(drv->state != UA_LIFECYCLESTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;

    AlarmsConditionsDriver *acd = (AlarmsConditionsDriver*)drv;
    deleteConditionSources(acd);
    deleteMonitoredItemTargets(acd);
    UA_KeyValueMap_clear(&drv->params);
    UA_free(acd);
    return UA_STATUSCODE_GOOD;
}

UA_AlarmConditionsDriver *
UA_AlarmsConditionsDriver(const UA_KeyValueMap params) {
    AlarmsConditionsDriver *acd =
        (AlarmsConditionsDriver*)UA_calloc(1, sizeof(AlarmsConditionsDriver));
    if(!acd)
        return NULL;

    UA_AlarmConditionsDriver *driver = &acd->driver;
    UA_Driver *base = &driver->drv;

    LIST_INIT(&acd->conditionSources);
    LIST_INIT(&acd->monitoredItemTargets);

    UA_StatusCode res = UA_KeyValueMap_copy(&params, &base->params);
    if(res != UA_STATUSCODE_GOOD) {
        UA_free(acd);
        return NULL;
    }

    base->name = UA_STRING(UA_DRIVER_ALARMS_CONDITIONS_NAME);
    base->notificationCallback = AlarmsConditionsDriver_notification;
    base->notificationFilter = UA_APPLICATIONNOTIFICATIONTYPE_MONITOREDITEM;
    base->start = AlarmsConditionsDriver_start;
    base->stop = AlarmsConditionsDriver_stop;
    base->free = AlarmsConditionsDriver_free;

    driver->createCondition = createCondition;
    driver->addConditionBegin = addConditionBegin;
    driver->addConditionFinish = addConditionFinish;
    driver->setConditionField = setConditionField;
    driver->setConditionVariableFieldProperty = setConditionVariableFieldProperty;
    driver->triggerConditionEvent = triggerConditionEvent;
    driver->addConditionOptionalField = addConditionOptionalField;
    driver->setConditionTwoStateVariableCallback = setConditionTwoStateVariableCallback;
    driver->deleteCondition = deleteCondition;
    driver->setLimitState = setLimitState;
    driver->setExpirationDate = setExpirationDate;
    return driver;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */
