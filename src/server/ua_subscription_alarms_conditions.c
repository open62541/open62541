/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Hilscher Gesellschaft für Systemautomation mbH (Author: Sameer AL-Qadasi)
 *    Copyright 2020-2022 (c) Christian von Arnim, ISW University of Stuttgart (for VDW and umati)
 *    Copyright 2024 (c) IOTechSystems (Author: Joe Riemersma)
 */

#include "ua_server_internal.h"

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

struct UA_ConditionBranch {
    ZIP_ENTRY(UA_ConditionBranch) zipEntry;
    //Each condition has a list of branches
    LIST_ENTRY (UA_ConditionBranch) listEntry;
    /*Id will be either the branchId or if is the main branch
     * of a condition it will be the conditionId */
    UA_NodeId id;
    UA_UInt32 idHash;
    /* Pointer to the Condition - will always outlive its branches*/
    struct UA_Condition *condition;
    UA_Boolean isMainBranch;
    UA_ByteString eventId;
};

typedef struct UA_Condition {
    ZIP_ENTRY (UA_Condition) zipEntry;
    LIST_HEAD (,UA_ConditionBranch) branches;
    UA_ConditionBranch *mainBranch;
    UA_NodeId sourceId;
    void *context;
    UA_Boolean canBranch;
} UA_Condition;

static UA_Condition *UA_Condition_new (void)
{
    UA_Condition *condition = (UA_Condition *) UA_malloc (sizeof(*condition));
    if (!condition) return NULL;
    memset(condition, 0, sizeof(*condition));
    return condition;
}

static void UA_Condition_delete (UA_Condition *condition)
{
    UA_free (condition);
}

static enum ZIP_CMP
cmpCondition (const void *a, const void *b) {
    const UA_ConditionBranch *aa = *((UA_ConditionBranch *const *)a);
    const UA_ConditionBranch *bb = *((UA_ConditionBranch *const *)b);

    /* Compare hash */
    if(aa->idHash < bb->idHash)
        return ZIP_CMP_LESS;
    if(aa->idHash > bb->idHash)
        return ZIP_CMP_MORE;
    /* Compore nodes in detail */
    return (enum ZIP_CMP)UA_NodeId_order(&aa->id, &bb->id);
}

ZIP_FUNCTIONS(UA_ConditionTree, UA_Condition, zipEntry, UA_ConditionBranch *, mainBranch, cmpCondition)

static UA_ConditionBranch *UA_ConditionBranch_new (void)
{
    UA_ConditionBranch *cb = (UA_ConditionBranch *) UA_malloc (sizeof(*cb));
    if (!cb) return NULL;
    memset(cb, 0, sizeof(*cb));
    return cb;
}

static void UA_ConditionBranch_delete (UA_ConditionBranch *cb)
{
    LIST_REMOVE(cb, listEntry);
    UA_NodeId_clear (&cb->id);
    UA_ByteString_clear(&cb->eventId);
    UA_free (cb);
}

static enum ZIP_CMP
cmpConditionBranch (const void *a, const void *b) {
    const UA_ConditionBranch *aa = (const UA_ConditionBranch *)a;
    const UA_ConditionBranch *bb = (const UA_ConditionBranch *)b;

    /* Compare hash */
    if(aa->idHash < bb->idHash)
        return ZIP_CMP_LESS;
    if(aa->idHash > bb->idHash)
        return ZIP_CMP_MORE;
    /* Compore nodes in detail */
    return (enum ZIP_CMP)UA_NodeId_order(&aa->id, &bb->id);
}

ZIP_FUNCTIONS(UA_ConditionBranchTree, UA_ConditionBranch, zipEntry, UA_ConditionBranch, zipEntry, cmpConditionBranch)

#define FIELD_STATEVARIABLE_ID                    "Id"

#define CONDITION_FIELD_EVENTID                                "EventId"
#define CONDITION_FIELD_EVENTTYPE                              "EventType"
#define CONDITION_FIELD_SOURCENODE                             "SourceNode"
#define CONDITION_FIELD_SOURCENAME                             "SourceName"
#define CONDITION_FIELD_TIME                                   "Time"
#define CONDITION_FIELD_RECEIVETIME                            "ReceiveTime"
#define CONDITION_FIELD_MESSAGE                                "Message"
#define CONDITION_FIELD_SEVERITY                               "Severity"
#define CONDITION_FIELD_RETAIN                                 "Retain"
#define CONDITION_FIELD_ENABLEDSTATE                           "EnabledState"
#define CONDITION_FIELD_LASTSEVERITY                           "LastSeverity"

#define REFRESHEVENT_START_IDX                                 0
#define REFRESHEVENT_END_IDX                                   1

#define EN_US_LOCALE                                           "en-US"
#define DISABLED_TEXT                                          "Disabled"

#define STATIC_QN(name) {0, UA_STRING_STATIC(name)}
static const UA_QualifiedName stateVariableIdQN = STATIC_QN(FIELD_STATEVARIABLE_ID);

static const UA_QualifiedName fieldEnabledStateQN = STATIC_QN(CONDITION_FIELD_ENABLEDSTATE);
static const UA_QualifiedName fieldRetainQN = STATIC_QN(CONDITION_FIELD_RETAIN);
static const UA_QualifiedName fieldSeverityQN = STATIC_QN(CONDITION_FIELD_SEVERITY);
static const UA_QualifiedName fieldQualityQN = STATIC_QN(CONDITION_FIELD_SEVERITY);
static const UA_QualifiedName fieldMessageQN = STATIC_QN(CONDITION_FIELD_MESSAGE);
static const UA_QualifiedName fieldTimeQN = STATIC_QN(CONDITION_FIELD_TIME);
static const UA_QualifiedName fieldEventIdQN = STATIC_QN(CONDITION_FIELD_EVENTID);
static const UA_QualifiedName fieldSourceNodeQN = STATIC_QN(CONDITION_FIELD_SOURCENODE);

#define CONDITION_LOG_ERROR(retval, logMessage)                \
    {                                                                                     \
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,                 \
                         logMessage". StatusCode %s", UA_StatusCode_name(retval));        \
    }

#define CONDITION_ASSERT_RETURN_RETVAL(retval, logMessage, deleteFunction)                \
    {                                                                                     \
        if(retval != UA_STATUSCODE_GOOD) {                                                \
            CONDITION_LOG_ERROR(retval,logMessage)                                   \
            deleteFunction                                                                \
            return retval;                                                                \
        }                                                                                 \
    }

/* Set the value of condition field (only scalar). */
static UA_StatusCode
setConditionField(UA_Server *server, const UA_NodeId condition,
                  const UA_Variant* value, const UA_QualifiedName fieldName) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, condition, 1, &fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;

    UA_StatusCode retval = writeValueAttribute(server, bpr.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_clear(&bpr);

    return retval;
}

static UA_StatusCode
setConditionVariableFieldProperty(UA_Server *server, const UA_NodeId condition,
                                  const UA_Variant* value,
                                  const UA_QualifiedName variableFieldName,
                                  const UA_QualifiedName variablePropertyName) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* 1) find Variable Field of the Condition*/
    UA_BrowsePathResult bprConditionVariableField =
        browseSimplifiedBrowsePath(server, condition, 1, &variableFieldName);
    if(bprConditionVariableField.statusCode != UA_STATUSCODE_GOOD)
        return bprConditionVariableField.statusCode;

    /* 2) find Property of the Variable Field of the Condition*/
    UA_BrowsePathResult bprVariableFieldProperty =
        browseSimplifiedBrowsePath(server, bprConditionVariableField.targets->targetId.nodeId,
                                   1, &variablePropertyName);
    if(bprVariableFieldProperty.statusCode != UA_STATUSCODE_GOOD) {
        UA_BrowsePathResult_clear(&bprConditionVariableField);
        return bprVariableFieldProperty.statusCode;
    }

    UA_StatusCode retval =
        writeValueAttribute(server, bprVariableFieldProperty.targets[0].targetId.nodeId, value);
    UA_BrowsePathResult_clear(&bprConditionVariableField);
    UA_BrowsePathResult_clear(&bprVariableFieldProperty);
    return retval;
}

static UA_StatusCode
readObjectPropertyUInt16 (UA_Server *server, UA_NodeId id, UA_QualifiedName property, UA_UInt16 *valueOut)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant value;
    UA_StatusCode retval = readObjectProperty(server, id, property, &value);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    if (!UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16]))
    {
        UA_Variant_clear(&value);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    *valueOut = *(UA_UInt16*)value.data;
    UA_Variant_clear(&value);
    return retval;
}

/* Gets the NodeId of a Field (e.g. Severity) */
static inline UA_StatusCode
getConditionFieldNodeId(UA_Server *server, const UA_NodeId *conditionNodeId,
                        const UA_QualifiedName* fieldName, UA_NodeId *outFieldNodeId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    return getNodeIdWithBrowseName(server, conditionNodeId, *fieldName, outFieldNodeId);
}

static UA_StatusCode
getValueOfConditionField (UA_Server *server, const UA_NodeId *condition,
                         UA_QualifiedName fieldName, UA_Variant *outValue)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant_init (outValue);
    UA_NodeId fieldId;
    UA_StatusCode retval = getConditionFieldNodeId(server, condition, &fieldName, &fieldId);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = readWithReadValue(server, &fieldId, UA_ATTRIBUTEID_VALUE, outValue);
    UA_NodeId_clear(&fieldId);
    return retval;
}

static UA_StatusCode
getBooleanValueOfConditionField(UA_Server *server, const UA_NodeId *condition,
                                UA_QualifiedName fieldName, UA_Boolean*outValue) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant value;
    UA_StatusCode retval = getValueOfConditionField(server, condition, fieldName, &value);
    if(retval != UA_STATUSCODE_GOOD) return retval;
    if(!UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_BOOLEAN])) {
        UA_Variant_clear(&value);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    *outValue = *(UA_Boolean*)value.data;
    UA_Variant_clear(&value);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setTwoStateVariable (UA_Server *server, const UA_NodeId *condition, UA_QualifiedName field,
                     UA_Boolean idValue, const char *state)
{
    /* Update Enabled State */
    UA_Variant value;
    UA_Variant_setScalar(&value, &idValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = setConditionVariableFieldProperty(server, *condition, &value,
                                               field, stateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting State Id failed",);

    UA_LocalizedText stateText = UA_LOCALIZEDTEXT(EN_US_LOCALE, (char *) (uintptr_t) state);
    UA_Variant_setScalar(&value, &stateText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = setConditionField (server, *condition, &value, field);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "set State text failed",);
    return retval;
}

static inline UA_Boolean
UA_ConditionBranch_State_Retain (const UA_ConditionBranch *branch, UA_Server *server)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Boolean value = false;
    UA_StatusCode retval = getBooleanValueOfConditionField(server, &branch->id, fieldRetainQN, &value);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_USERLAND,
                       "Retain not found. StatusCode %s", UA_StatusCode_name(retval));
        return false;
    }
    return value;
}

static UA_StatusCode
UA_ConditionBranch_State_setSeverity(UA_ConditionBranch *branch, UA_Server *server, UA_UInt16 severity)
{
    UA_Variant value;
    UA_Variant_setScalar(&value, &severity, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = setConditionField (server, branch->id, &value, fieldSeverityQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting Condition Severity failed",);
    return retval;
}

static UA_StatusCode
UA_ConditionBranch_State_updateSeverity(UA_ConditionBranch *branch, UA_Server *server, UA_UInt16 severity)
{
    UA_UInt16 currentSeverity;
    UA_StatusCode retval = readObjectPropertyUInt16 (server, branch->id, fieldSeverityQN, &currentSeverity);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Read current condition Severity failed",);
    UA_Variant value;
    UA_Variant_setScalar(&value, &currentSeverity, &UA_TYPES[UA_TYPES_UINT16]);
    retval = setConditionField (server, branch->id, &value, UA_QUALIFIEDNAME(0, CONDITION_FIELD_LASTSEVERITY));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "set Condition LastSeverity failed",);
    return UA_ConditionBranch_State_setSeverity (branch, server, severity);
}

static UA_StatusCode
UA_ConditionBranch_State_setMessage(UA_ConditionBranch *branch, UA_Server *server, const UA_LocalizedText* message)
{
    UA_Variant value;
    UA_Variant_setScalar(&value, (void*)(uintptr_t) message, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = setConditionField (server, branch->id, &value, fieldMessageQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting Condition Message failed",);
    return retval;
}

static UA_StatusCode
UA_ConditionBranch_State_setQuality(UA_ConditionBranch *branch, UA_Server *server, UA_StatusCode quality)
{
    UA_Variant value;
    UA_Variant_setScalar(&value, &quality, &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = setConditionField (server, branch->id, &value, fieldQualityQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting Condition Quality failed",);
    return retval;
}

static UA_StatusCode removeCondition (UA_Server *server, UA_Condition *condition);

static void *deleteConditionsWrapper (void *ctx, UA_Condition *condition)
{
    UA_Server *server = (UA_Server*) ctx;
    removeCondition(server, condition);
    return NULL;
}

void
UA_ConditionList_delete(UA_Server *server) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    ZIP_ITER (UA_ConditionTree, &server->conditions, deleteConditionsWrapper, server);
    /* Free memory allocated for RefreshEvents NodeIds */
    UA_NodeId_clear(&server->refreshEvents[REFRESHEVENT_START_IDX]);
    UA_NodeId_clear(&server->refreshEvents[REFRESHEVENT_END_IDX]);
}

static inline UA_Condition *getCondition (UA_Server *server, const UA_NodeId *conditionId)
{
    UA_ConditionBranch dummy;
    dummy.id = *conditionId;
    dummy.idHash = UA_NodeId_hash(conditionId);
    const UA_ConditionBranch *dummy_ptr = &dummy;
    return ZIP_FIND(UA_ConditionTree, &server->conditions, &dummy_ptr);
}

static inline UA_ConditionBranch *getConditionBranch (UA_Server *server, const UA_NodeId *branchId)
{
    UA_ConditionBranch dummy;
    dummy.id = *branchId;
    dummy.idHash = UA_NodeId_hash(branchId);
    return ZIP_FIND(UA_ConditionBranchTree, &server->conditionBranches, &dummy);
}

UA_StatusCode
UA_getConditionId(UA_Server *server, const UA_NodeId *conditionNodeId,
                  UA_NodeId *outConditionId)
{
    UA_ConditionBranch * branch = getConditionBranch(server, conditionNodeId);
    if (!branch) return UA_STATUSCODE_BADNODEIDUNKNOWN;
    *outConditionId = branch->condition->mainBranch->id;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode removeConditionBranch (UA_Server *server, UA_ConditionBranch *branch)
{
    UA_StatusCode ret = deleteNode(server, branch->id, true);
    ZIP_REMOVE(UA_ConditionBranchTree, &server->conditionBranches, branch);
    UA_ConditionBranch_delete(branch);
    return ret;
}

static UA_StatusCode removeCondition (UA_Server *server, UA_Condition *condition)
{
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ZIP_REMOVE(UA_ConditionTree, &server->conditions ,condition);
    condition->mainBranch = NULL;
    UA_ConditionBranch *branch = NULL;
    UA_ConditionBranch *tmp= NULL;
    LIST_FOREACH_SAFE (branch, &condition->branches, listEntry, tmp)
    {
        ret |= removeConditionBranch (server, branch);
    }
    UA_Condition_delete (condition);
    return ret;
}

static UA_StatusCode
UA_ConditionBranch_triggerEvent (UA_ConditionBranch *branch, UA_Server *server,
                                 const UA_ConditionEventInfo *info)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Set time */
    UA_DateTime time = UA_DateTime_now();
    UA_StatusCode retval = writeObjectProperty_scalar(server, branch->id, fieldTimeQN,
                                                      &time,
                                                      &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set Condition Time failed",);

    if (info)
    {
        if (!UA_String_equal(&info->message.locale, &UA_STRING_NULL) &&
            !UA_String_equal(&info->message.text, &UA_STRING_NULL))
        {
            UA_ConditionBranch_State_setMessage(branch, server, &info->message);
        }
        if (info->hasQuality) UA_ConditionBranch_State_setQuality(branch, server, info->quality);
        if (info->hasSeverity) UA_ConditionBranch_State_updateSeverity (branch, server, info->severity);
    }

    UA_ByteString_clear(&branch->eventId);
    //Condition Nodes should not be deleted after triggering the event
    retval = triggerEvent(server, branch->id, branch->condition->sourceId, &branch->eventId, false);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Triggering condition event failed",);
    return retval;
}


static UA_StatusCode
newConditionBranchEntry (
    UA_Server* server,
    const UA_NodeId *branchId,
    const UA_ByteString *lastEventId,
    UA_Condition *condition,
    UA_Boolean isMainBranch
)
{
    /*make sure entry doesn't exist*/
    if (getConditionBranch(server, branchId))
    {
        return UA_STATUSCODE_BADNODEIDEXISTS;
    }
    if (isMainBranch && condition->mainBranch) return UA_STATUSCODE_BADINTERNALERROR;

    UA_StatusCode status = UA_STATUSCODE_GOOD;
    UA_ConditionBranch *branch = UA_ConditionBranch_new();
    status = UA_NodeId_copy (branchId, (UA_NodeId *) &branch->id);
    if (status != UA_STATUSCODE_GOOD) goto fail;
    branch->idHash = UA_NodeId_hash(&branch->id);
    status = UA_ByteString_copy (lastEventId, &branch->eventId);
    if (status != UA_STATUSCODE_GOOD) goto fail;
    branch->condition = condition;
    branch->isMainBranch = isMainBranch;
    ZIP_INSERT (UA_ConditionBranchTree, &server->conditionBranches, branch);
    //add the condition branch to the conditions branch list
    LIST_INSERT_HEAD(&condition->branches, branch, listEntry);
    if (branch->isMainBranch)
    {
        condition->mainBranch = branch;
        ZIP_INSERT (UA_ConditionTree, &server->conditions, condition);
    }
    else
        UA_ConditionBranch_triggerEvent(branch, server, NULL);

    return UA_STATUSCODE_GOOD;
fail:
    UA_ConditionBranch_delete(branch);
    return status;
}

static UA_StatusCode
newConditionEntry (UA_Server *server, const UA_NodeId *conditionNodeId,
                   const UA_CreateConditionProperties *conditionProperties, UA_Condition **out)
{
    /*make sure entry doesn't exist*/
    if (getCondition(server, conditionNodeId))
    {
        return UA_STATUSCODE_BADNODEIDEXISTS;
    }

    UA_StatusCode status = UA_STATUSCODE_GOOD;
    UA_Condition *condition = UA_Condition_new();
    status = UA_NodeId_copy (&conditionProperties->sourceNode, (UA_NodeId *) &condition->sourceId);
    if (status != UA_STATUSCODE_GOOD) goto fail;
    condition->canBranch = conditionProperties->canBranch;
    *out = condition;
    return UA_STATUSCODE_GOOD;
fail:
    UA_Condition_delete(condition);
    return status;
}

static UA_StatusCode
newConditionInstanceEntry (UA_Server *server, const UA_NodeId *conditionNodeId, const UA_CreateConditionProperties *conditionProperties)
{
    UA_Condition *condition = NULL;
    UA_StatusCode status = newConditionEntry(server, conditionNodeId, conditionProperties, &condition);
    if (status != UA_STATUSCODE_GOOD) return status;
    /*Could just pass this out of newConditionEntry*/
    if (!condition) return UA_STATUSCODE_BADINTERNALERROR;
    status = newConditionBranchEntry(server, conditionNodeId, &UA_BYTESTRING_NULL, condition, true);
    if (status != UA_STATUSCODE_GOOD)
    {
        removeCondition(server, condition);
    }

    return status;
}

static UA_StatusCode setConditionProperties (
    UA_Server *server,
    const UA_NodeId *conditionType,
    const UA_NodeId *conditionId,
    const UA_CreateConditionProperties *properties
)
{
    UA_Variant value;
    UA_Variant_setScalar(&value, (void*)(uintptr_t) conditionType, &UA_TYPES[UA_TYPES_NODEID]);
    UA_StatusCode retval = setConditionField(server, *conditionId, &value,
                                             UA_QUALIFIEDNAME(0,CONDITION_FIELD_EVENTTYPE));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EventType Field failed",);

    if (!UA_NodeId_isNull(&properties->sourceNode))
    {
        /* Get Condition SourceNode*/
        const UA_Node *conditionSourceNode = UA_NODESTORE_GET(server, &properties->sourceNode);
        if(!conditionSourceNode) {
            UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_USERLAND,
                           "Couldn't find Condition SourceNode. StatusCode %s", UA_StatusCode_name(retval));
            return UA_STATUSCODE_BADNOTFOUND;
        }

        /* Set SourceNode*/
        UA_Variant_setScalar(&value, (void*)(uintptr_t)&conditionSourceNode->head.nodeId,
                             &UA_TYPES[UA_TYPES_NODEID]);
        retval = setConditionField(server, *conditionId, &value, fieldSourceNodeQN);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                         "Set SourceNode Field failed. StatusCode %s", UA_StatusCode_name(retval));
            UA_NODESTORE_RELEASE(server, conditionSourceNode);
            return retval;
        }

        UA_NODESTORE_RELEASE(server, conditionSourceNode);
    }

    /* Set EnabledState */
    retval = setTwoStateVariable (server, conditionId, fieldEnabledStateQN, false, DISABLED_TEXT);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting initial Enabled state failed",);
    return retval;
}

static UA_StatusCode
addCondition_finish(
    UA_Server *server,
    const UA_NodeId *conditionId,
    const UA_NodeId *conditionType,
    const UA_CreateConditionProperties *conditionProperties,
    UA_ConditionTypeSetupFn setupNodesFn,
    const void *setupNodesUserData
) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_StatusCode retval = addNode_finish(server, &server->adminSession, conditionId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Finish node failed",);

    retval = setConditionProperties(server, conditionType, conditionId, conditionProperties);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    UA_UNLOCK(&server->serviceMutex);
    retval = setupNodesFn ? setupNodesFn (server, conditionId, setupNodesUserData) : UA_STATUSCODE_GOOD;
    UA_LOCK(&server->serviceMutex);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setup Nodes failed",);

    if (!UA_NodeId_isNull(&conditionProperties->sourceNode))
    {
        /* create HasCondition Reference (HasCondition should be forward from the
         * ConditionSourceNode to the Condition. else, HasCondition should be
         * forward from the ConditionSourceNode to the ConditionType Node) */
        UA_NodeId hasCondition = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCONDITION);
        if(!UA_NodeId_isNull(&conditionProperties->hierarchialReferenceType)) {
            retval = addRef(server, conditionProperties->sourceNode, hasCondition, *conditionId, true);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating HasCondition Reference failed",);
        } else {
            retval = addRef(server, conditionProperties->sourceNode, hasCondition, *conditionType, true);
            if(retval != UA_STATUSCODE_GOOD && retval != UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED)
            {
                CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating HasCondition Reference failed",);
            }
            retval = UA_STATUSCODE_GOOD;
        }
    }

    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setup Condition failed",);
    return newConditionInstanceEntry (server, conditionId, conditionProperties);
}

UA_StatusCode
__UA_Server_addCondition_finish(
    UA_Server *server,
    const UA_NodeId *conditionId,
    const UA_NodeId *conditionType,
    const UA_CreateConditionProperties *conditionProperties,
    UA_ConditionTypeSetupFn setupNodesFn,
    const void *setupNodesUserData
)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = addCondition_finish(server, conditionId, conditionType, conditionProperties, setupNodesFn, setupNodesUserData);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
addCondition_begin(UA_Server *server, const UA_NodeId conditionId,
                   const UA_NodeId conditionType,
                   const UA_CreateConditionProperties *properties, UA_NodeId *outNodeId) {
    if(!outNodeId) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                     "outNodeId cannot be NULL!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    /* Make sure the conditionType is a Subtype of ConditionType */
    UA_NodeId conditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);
    UA_Boolean found = isNodeInTree_singleRef(server, &conditionType, &conditionTypeId,
                                              UA_REFERENCETYPEINDEX_HASSUBTYPE);
    if(!found) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                     "Condition Type must be a subtype of ConditionType!");
        return UA_STATUSCODE_BADNOMATCH;
    }

    /* Create an ObjectNode which represents the condition */
    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = properties->displayName;
    oAttr.description = properties->description;
    UA_StatusCode retval = addNode_begin(
        server, UA_NODECLASS_OBJECT, conditionId,
        properties->parentNodeId, properties->hierarchialReferenceType,
        properties->browseName, conditionType, &oAttr,
        &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES], NULL, outNodeId
    );
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding Condition failed",);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
__UA_Server_addCondition_begin(UA_Server *server, const UA_NodeId conditionId,
                               const UA_NodeId conditionType,
                               const UA_CreateConditionProperties *properties, UA_NodeId *outNodeId)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = addCondition_begin(server, conditionId, conditionType, properties, outNodeId);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

UA_StatusCode
__UA_Server_createCondition(UA_Server *server,
                            const UA_NodeId conditionId,
                            const UA_NodeId conditionType,
                            const UA_CreateConditionProperties *conditionProperties,
                            UA_ConditionTypeSetupFn setupFn,
                            const void *setupData,
                            UA_NodeId *outNodeId) {
    if(!outNodeId) {
        UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                     "outNodeId cannot be NULL!");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }

    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = addCondition_begin(server, conditionId, conditionType,
                                              conditionProperties, outNodeId);
    if(retval != UA_STATUSCODE_GOOD)
    {
        UA_UNLOCK(&server->serviceMutex);
        return retval;
    }

    retval = addCondition_finish (server, outNodeId, &conditionType, conditionProperties, setupFn, setupData);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

UA_StatusCode
UA_Server_deleteCondition(UA_Server *server, const UA_NodeId conditionId)
{
    UA_LOCK(&server->serviceMutex);
    UA_Condition *cond = getCondition(server, &conditionId);
    if (!cond)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = removeCondition(server, cond);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

struct refreshIterCtx
{
    UA_Server *server;
    UA_MonitoredItem *item;
};

static UA_Boolean
isConditionSourceInMonitoredItem(UA_Server *server, const UA_MonitoredItem *monitoredItem,
                                 const UA_NodeId *conditionSource){
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    /* TODO: check also other hierarchical references */
    UA_ReferenceTypeSet refs = UA_REFTYPESET(UA_REFERENCETYPEINDEX_ORGANIZES);
    refs = UA_ReferenceTypeSet_union(refs, UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASCOMPONENT));
    refs = UA_ReferenceTypeSet_union(refs, UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASEVENTSOURCE));
    refs = UA_ReferenceTypeSet_union(refs, UA_REFTYPESET(UA_REFERENCETYPEINDEX_HASNOTIFIER));
    return isNodeInTree(server, conditionSource, &monitoredItem->itemToMonitor.nodeId, &refs);
}

static void *refreshIterCallback (void *userdata, UA_ConditionBranch *branch)
{
    struct refreshIterCtx *ctx = (struct refreshIterCtx *) userdata;
    UA_MonitoredItem *item = ctx->item;
    UA_Server *server = ctx->server;
    UA_NodeId serverObjectNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
    /* is conditionSource or server object being monitored? */
    if (!UA_NodeId_equal(&item->itemToMonitor.nodeId, &branch->condition->sourceId) &&
       !UA_NodeId_equal(&item->itemToMonitor.nodeId, &serverObjectNodeId) &&
       !isConditionSourceInMonitoredItem(server, ctx->item, &branch->condition->sourceId))
    {
        return NULL;
    }

    if (UA_ByteString_equal(&branch->eventId, &UA_BYTESTRING_NULL)) return NULL;
    if (!UA_ConditionBranch_State_Retain(branch, server)) return NULL;
    UA_StatusCode retval = UA_MonitoredItem_addEvent(server, item, &branch->id);
    if (retval != UA_STATUSCODE_GOOD)
        CONDITION_LOG_ERROR(retval, "Events: Could not add the event to a listening node");
    return NULL;
}

static UA_StatusCode
refreshLogic(UA_Server *server, const UA_NodeId *refreshStartNodId,
             const UA_NodeId *refreshEndNodId, UA_MonitoredItem *monitoredItem) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_assert(monitoredItem != NULL);

    /* 1. Trigger RefreshStartEvent */
    UA_DateTime fieldTimeValue = UA_DateTime_now();
    UA_StatusCode retval =
        writeObjectProperty_scalar(server, *refreshStartNodId, fieldTimeQN,
                                   &fieldTimeValue, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Write Object Property scalar failed",);

    retval = UA_MonitoredItem_addEvent(server, monitoredItem, refreshStartNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Events: Could not add the event to a listening node",);

    /* 2. Refresh (see 5.5.7) */
    struct refreshIterCtx ctx = {.server = server, .item = monitoredItem};
    ZIP_ITER (UA_ConditionBranchTree, &server->conditionBranches, refreshIterCallback, &ctx);

    /* 3. Trigger RefreshEndEvent */
    fieldTimeValue = UA_DateTime_now();
    retval = writeObjectProperty_scalar(server, *refreshEndNodId, fieldTimeQN,
                                        &fieldTimeValue, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Write Object Property scalar failed",);
    return UA_MonitoredItem_addEvent(server, monitoredItem, refreshEndNodId);
}

static UA_StatusCode
setRefreshMethodEventFields(UA_Server *server, const UA_NodeId *refreshEventNodId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_QualifiedName fieldSeverity = UA_QUALIFIEDNAME(0, CONDITION_FIELD_SEVERITY);
    UA_QualifiedName fieldSourceName = UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENAME);
    UA_QualifiedName fieldReceiveTime = UA_QUALIFIEDNAME(0, CONDITION_FIELD_RECEIVETIME);
    UA_String sourceNameString = UA_STRING("Server"); //server is the source of Refresh Events
    UA_UInt16 severityValue = 0;
    UA_ByteString eventId  = UA_BYTESTRING_NULL;
    UA_Variant value;

    /* Set Severity */
    UA_Variant_setScalar(&value, &severityValue, &UA_TYPES[UA_TYPES_UINT16]);
    UA_StatusCode retval = setConditionField(server, *refreshEventNodId,
                                             &value, fieldSeverity);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set RefreshEvent Severity failed",);

    /* Set SourceName */
    UA_Variant_setScalar(&value, &sourceNameString, &UA_TYPES[UA_TYPES_STRING]);
    retval = setConditionField(server, *refreshEventNodId, &value, fieldSourceName);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set RefreshEvent Source failed",);

    /* Set ReceiveTime */
    UA_DateTime fieldReceiveTimeValue = UA_DateTime_now();
    UA_Variant_setScalar(&value, &fieldReceiveTimeValue, &UA_TYPES[UA_TYPES_DATETIME]);
    retval = setConditionField(server, *refreshEventNodId, &value, fieldReceiveTime);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set RefreshEvent ReceiveTime failed",);

    /* Set EventId */
    retval = generateEventId(&eventId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Generating EventId failed",);

    UA_Variant_setScalar(&value, &eventId, &UA_TYPES[UA_TYPES_BYTESTRING]);
    retval = setConditionField(server, *refreshEventNodId, &value, fieldEventIdQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set RefreshEvent EventId failed",);

    UA_ByteString_clear(&eventId);

    return retval;
}

static UA_StatusCode
setRefreshMethodEvents(UA_Server *server, const UA_NodeId *refreshStartNodId,
                       const UA_NodeId *refreshEndNodId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Set Standard Fields for RefreshStart */
    UA_StatusCode retval = setRefreshMethodEventFields(server, refreshStartNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set standard Fields of RefreshStartEvent failed",);

    /* Set Standard Fields for RefreshEnd*/
    retval = setRefreshMethodEventFields(server, refreshEndNodId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set standard Fields of RefreshEndEvent failed",);
    return retval;
}

static UA_StatusCode
refreshMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output) {
    UA_LOCK(&server->serviceMutex);

    //TODO implement logic for subscription array
    /* Check if valid subscriptionId */
    UA_Session *session = getSessionById(server, sessionId);
    UA_Subscription *subscription =
        UA_Session_getSubscriptionById(session, *((UA_UInt32 *)input[0].data));
    if(!subscription) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }

    /* set RefreshStartEvent and RefreshEndEvent */
    UA_StatusCode retval =
        setRefreshMethodEvents(server, &server->refreshEvents[REFRESHEVENT_START_IDX],
                               &server->refreshEvents[REFRESHEVENT_END_IDX]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Create Event RefreshStart or RefreshEnd failed",
                                   UA_UNLOCK(&server->serviceMutex););

    /* Trigger RefreshStartEvent and RefreshEndEvent for the each monitoredItem
     * in the subscription */
    //TODO when there are a lot of monitoreditems (not only events)?
    UA_MonitoredItem *monitoredItem = NULL;
    LIST_FOREACH(monitoredItem, &subscription->monitoredItems, listEntry) {
        retval = refreshLogic(server, &server->refreshEvents[REFRESHEVENT_START_IDX],
                              &server->refreshEvents[REFRESHEVENT_END_IDX], monitoredItem);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Could not refresh Condition",
                                       UA_UNLOCK(&server->serviceMutex););
    }
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
refresh2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                       void *sessionContext, const UA_NodeId *methodId,
                       void *methodContext, const UA_NodeId *objectId,
                       void *objectContext, size_t inputSize,
                       const UA_Variant *input, size_t outputSize,
                       UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    //TODO implement logic for subscription array
    /* Check if valid subscriptionId */
    UA_Session *session = getSessionById(server, sessionId);
    UA_Subscription *subscription =
        UA_Session_getSubscriptionById(session, *((UA_UInt32 *)input[0].data));
    if(!subscription) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADSUBSCRIPTIONIDINVALID;
    }

    /* set RefreshStartEvent and RefreshEndEvent */
    UA_StatusCode retval = setRefreshMethodEvents(server,
                                                  &server->refreshEvents[REFRESHEVENT_START_IDX],
                                                  &server->refreshEvents[REFRESHEVENT_END_IDX]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Create Event RefreshStart or RefreshEnd failed",
                                   UA_UNLOCK(&server->serviceMutex););

    /* Trigger RefreshStartEvent and RefreshEndEvent for the each monitoredItem
     * in the subscription */
    UA_MonitoredItem *monitoredItem =
        UA_Subscription_getMonitoredItem(subscription, *((UA_UInt32 *)input[1].data));
    if(!monitoredItem) {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADMONITOREDITEMIDINVALID;
    }

    //TODO when there are a lot of monitoreditems (not only events)?
    retval = refreshLogic(server, &server->refreshEvents[REFRESHEVENT_START_IDX],
                          &server->refreshEvents[REFRESHEVENT_END_IDX], monitoredItem);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Could not refresh Condition",
                                   UA_UNLOCK(&server->serviceMutex););
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}

void initNs0ConditionAndAlarms (UA_Server *server)
{
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Set callbacks for Method Fields of a condition. The current implementationd54j
     * references methods without copying them when creating objects. So the
     * callbacks will be attached to the methods of the conditionType. */

    struct UA_MethodIdMethod {
        UA_NodeId methodId;
        UA_MethodCallback method;
    };

    struct UA_MethodIdMethod methods[] = {
        {{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH}}, refreshMethodCallback},
        {{0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2}}, refresh2MethodCallback},
    };

    for (size_t i=0; i<(sizeof(methods)/sizeof(methods[0]));i++)
    {
        setMethodNode_callback(server, methods[i].methodId, methods[i].method);
    }

    // Create RefreshEvents
    if(UA_NodeId_isNull(&server->refreshEvents[REFRESHEVENT_START_IDX]) &&
       UA_NodeId_isNull(&server->refreshEvents[REFRESHEVENT_END_IDX])) {

        UA_NodeId refreshStartEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHSTARTEVENTTYPE);
        UA_NodeId refreshEndEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHENDEVENTTYPE);

        /* Create RefreshStartEvent */
        retval = createEvent(server, refreshStartEventTypeNodeId, &server->refreshEvents[REFRESHEVENT_START_IDX]);
        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                         "CreateEvent RefreshStart failed. StatusCode %s", UA_StatusCode_name(retval));
        }

        /* Create RefreshEndEvent */
        retval = createEvent(server, refreshEndEventTypeNodeId, &server->refreshEvents[REFRESHEVENT_END_IDX]);
        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                         "CreateEvent RefreshEnd failed. StatusCode %s", UA_StatusCode_name(retval));
        }
    }

    /* change Refresh Events IsAbstract = false
     * so abstract Events : RefreshStart and RefreshEnd could be created */
    UA_NodeId refreshStartEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHSTARTEVENTTYPE);
    UA_NodeId refreshEndEventTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_REFRESHENDEVENTTYPE);

    UA_Boolean startAbstract = false;
    UA_Boolean endAbstract = false;
    readWithReadValue(server, &refreshStartEventTypeNodeId,
                      UA_ATTRIBUTEID_ISABSTRACT, &startAbstract);
    readWithReadValue(server, &refreshEndEventTypeNodeId,
                      UA_ATTRIBUTEID_ISABSTRACT, &endAbstract);

    UA_Boolean inner = (startAbstract == false && endAbstract == false);
    if(inner) {
        writeIsAbstractAttribute(server, refreshStartEventTypeNodeId, false);
        writeIsAbstractAttribute(server, refreshEndEventTypeNodeId, false);
    }
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */
