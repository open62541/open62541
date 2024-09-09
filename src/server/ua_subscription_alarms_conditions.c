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

static UA_StatusCode UA_ConditionEventInfo_copy (const UA_ConditionEventInfo *src, UA_ConditionEventInfo *dest)
{
    *dest = *src;
    UA_StatusCode ret = UA_LocalizedText_copy(&src->message, &dest->message);
    return ret;
}

static UA_ConditionEventInfo *UA_ConditionEventInfo_new (void)
{
    UA_ConditionEventInfo *p = (UA_ConditionEventInfo*) UA_malloc(sizeof(*p));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p));
    return p;
}

static void UA_ConditionEventInfo_delete (UA_ConditionEventInfo *p)
{
    if (!p) return;
    UA_LocalizedText_clear(&p->message);
    UA_free (p);
}

typedef struct UA_ConditionBranch {
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
    UA_Boolean lastEventRetainValue;
    UA_ByteString eventId;
}UA_ConditionBranch;

typedef struct UA_Condition {
    ZIP_ENTRY (UA_Condition) zipEntry;
    LIST_HEAD (,UA_ConditionBranch) branches;
    UA_ConditionBranch *mainBranch;
    UA_NodeId sourceId;
    void *context;
    UA_ConditionFns fns;
    UA_UInt64 onDelayCallbackId;
    UA_UInt64 offDelayCallbackId;
    UA_ConditionEventInfo *_delayCallbackInfo;
    UA_UInt64 reAlarmCallbackId;
    UA_Int16 reAlarmCount;
    UA_UInt64 unshelveCallbackId;
    UA_Boolean canBranch;
    const UA_ConditionImplCallbacks *callbacks;
} UA_Condition;

static inline UA_StatusCode UA_Condition_UserCallback_onAcked (UA_Server *server, UA_Condition *condition, const UA_NodeId *id)
{
   if (!condition->callbacks || !condition->callbacks->onAcked) return UA_STATUSCODE_GOOD;
   UA_UNLOCK(&server->serviceMutex);
   UA_StatusCode ret = condition->callbacks->onAcked (server, id, condition->context);
   UA_LOCK(&server->serviceMutex);
   return ret;
}

static inline UA_StatusCode UA_Condition_UserCallback_onConfirmed (UA_Server *server, UA_Condition *condition, const UA_NodeId *id)
{
    if (!condition->callbacks || !condition->callbacks->onConfirmed) return UA_STATUSCODE_GOOD;
    UA_UNLOCK(&server->serviceMutex);
    UA_StatusCode ret = condition->callbacks->onConfirmed(server, id, condition->context);
    UA_LOCK(&server->serviceMutex);
    return ret;
}

static inline UA_StatusCode UA_Condition_UserCallback_onActive(UA_Server *server, UA_Condition *condition, const UA_NodeId *id)
{
    if (!condition->callbacks || !condition->callbacks->onActive) return UA_STATUSCODE_GOOD;
    UA_UNLOCK(&server->serviceMutex);
    UA_StatusCode ret = condition->callbacks->onActive(server, id, condition->context);
    UA_LOCK(&server->serviceMutex);
    return ret;
}

static inline UA_StatusCode UA_Condition_UserCallback_onInactive(UA_Server *server, UA_Condition *condition, const UA_NodeId *id)
{
    if (!condition->callbacks || !condition->callbacks->onInactive) return UA_STATUSCODE_GOOD;
    UA_UNLOCK(&server->serviceMutex);
    UA_StatusCode ret = condition->callbacks->onInactive(server, id, condition->context);
    UA_LOCK(&server->serviceMutex);
    return ret;
}

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

static UA_ConditionBranch *UA_Condition_GetBranchWithEventId (UA_Condition *condition, const UA_ByteString *eventId)
{
    UA_ConditionBranch *branch = NULL;
    if (UA_ByteString_equal(&condition->mainBranch->eventId, eventId)) return condition->mainBranch;
    LIST_FOREACH(branch, &condition->branches, listEntry)
    {
        if (UA_ByteString_equal(&branch->eventId, eventId)) return branch;
    }
    return NULL;
}

static inline void UA_Condition_removeUnshelveCallback (UA_Condition *condition, UA_Server *server)
{
    if (condition->unshelveCallbackId != 0) return;
    removeCallback(server, condition->unshelveCallbackId);
    condition->unshelveCallbackId = 0;
}

static size_t UA_Condition_getBranchCount (const UA_Condition *condition)
{
    size_t count = 0;
    UA_ConditionBranch *tmp = NULL;
    LIST_FOREACH(tmp, &condition->branches, listEntry)
    {
        count++;
    }
    return count;
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

static UA_StatusCode
UA_ConditionBranch_triggerEvent (UA_ConditionBranch *branch, UA_Server *server,
                              const UA_ConditionEventInfo *info);

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

/* Condition Field Names */
#define SHELVEDSTATE_METHOD_TIMEDSHELVE "TimedShelve"
#define SHELVEDSTATE_METHOD_ONESHOTSHELVE "OneShotShelve"
#define SHELVEDSTATE_METHOD_UNSHELVE "Unshelve"
#define SHELVEDSTATE_METHOD_TIMEDSHELVE2 SHELVEDSTATE_METHOD_TIMEDSHELVE"2"
#define SHELVEDSTATE_METHOD_ONESHOTSHELVE2 SHELVEDSTATE_METHOD_ONESHOTSHELVE"2"
#define SHELVEDSTATE_METHOD_UNSHELVE2 SHELVEDSTATE_METHOD_UNSHELVE"2"

#define FIELD_STATEVARIABLE_ID                    "Id"

#define FIELD_CURRENT_STATE "CurrentState"

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
#define CONDITION_FIELD_QUALITY                                "Quality"
#define CONDITION_FIELD_LASTSEVERITY                           "LastSeverity"
#define CONDITION_FIELD_COMMENT                                "Comment"
#define CONDITION_FIELD_EXPECTEDTIME                           "ExpectedTime"
#define CONDITION_FIELD_TARGETVALUENODE                        "TargetValueNode"
#define CONDITION_FIELD_TOLERANCE                              "Tolerance"
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
#define CONDITION_FIELD_LATCHEDSTATE                           "LatchedState"
#define CONDITION_FIELD_SUPPRESSEDSTATE                        "SuppressedState"
#define CONDITION_FIELD_OUTOFSERVICESTATE                      "OutOfServiceState"
#define CONDITION_FIELD_SHELVINGSTATE                          "ShelvingState"
#define CONDITION_FIELD_MAXTIMESHELVED                         "MaxTimeShelved"
#define CONDITION_FIELD_SUPPRESSEDORSHELVED                    "SuppressedOrShelved"
#define CONDITION_FIELD_NORMALSTATE                            "NormalState"
#define CONDITION_FIELD_ONDELAY                                "OnDelay"
#define CONDITION_FIELD_OFFDELAY                               "OffDelay"
#define CONDITION_FIELD_REALARMTIME                            "ReAlarmTime"
#define CONDITION_FIELD_REALARMREPEATCOUNT                     "ReAlarmRepeatCount"
#define CONDITION_FIELD_BASEHIGHHIGHLIMIT                      "BaseHighHighLimit"
#define CONDITION_FIELD_BASEHIGHLIMIT                          "BaseHighLimit"
#define CONDITION_FIELD_BASELOWLIMIT                           "BaseLowLimit"
#define CONDITION_FIELD_BASELOWLOWLIMIT                        "BaseLowLowLimit"
#define CONDITION_FIELD_SEVERITYHIGHHIGH                       "SeverityHighHigh"
#define CONDITION_FIELD_SEVERITYHIGH                           "SeverityHigh"
#define CONDITION_FIELD_SEVERITYLOW                            "SeverityLow"
#define CONDITION_FIELD_SEVERITYLOWLOW                         "SeverityLowLow"
#define CONDITION_FIELD_HIGHHIGHLIMIT                          "HighHighLimit"
#define CONDITION_FIELD_HIGHLIMIT                              "HighLimit"
#define CONDITION_FIELD_LOWLIMIT                               "LowLimit"
#define CONDITION_FIELD_LOWLOWLIMIT                            "LowLowLimit"
#define CONDITION_FIELD_HIGHHIGHSTATE                          "HighHighState"
#define CONDITION_FIELD_HIGHSTATE                              "HighState"
#define CONDITION_FIELD_LOWSTATE                               "LowState"
#define CONDITION_FIELD_LOWLOWSTATE                            "LowLowState"
#define CONDITION_FIELD_LOWDEADBAND                            "LowDeadband"
#define CONDITION_FIELD_LOWLOWDEADBAND                         "LowLowDeadband"
#define CONDITION_FIELD_HIGHDEADBAND                           "HighDeadband"
#define CONDITION_FIELD_HIGHHIGHDEADBAND                       "HighHighDeadband"
#define CONDITION_FIELD_PROPERTY_EFFECTIVEDISPLAYNAME          "EffectiveDisplayName"
#define CONDITION_FIELD_LIMITSTATE                             "LimitState"
#define CONDITION_FIELD_SETPOINTNODE                           "SetpointNode"
#define CONDITION_FIELD_BASESETPOINTNODE                       "BaseSetpointNode"
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
#define CONDITION_FIELD_ENGINEERINGUNITS                       "EngineeringUnits"
#define CONDITION_FIELD_EXPIRATION_DATE                        "ExpirationDate"
#define REFRESHEVENT_START_IDX                                 0
#define REFRESHEVENT_END_IDX                                   1
#define REFRESHEVENT_SEVERITY_DEFAULT                          100

#define CONDITION_FIELD_EXPIRATION_LIMIT                       "ExpirationLimit"
#define CONDITION_FIELD_EXPIRATION_DATE                        "ExpirationDate"
#define CONDITION_FIELD_CERTIFICATE                            "Certificate"
#define CONDITION_FIELD_CERTIFICATE_TYPE                       "CertificateType"

#define LOCALE                                                 "en"
#define LOCALE_NULL                                             ""
#define TEXT_NULL                                               ""
#define ENABLED_TEXT                                           "Enabled"
#define DISABLED_TEXT                                          "Disabled"
#define ENABLED_MESSAGE                                        "The alarm was enabled"
#define DISABLED_MESSAGE                                       "The alarm was disabled"
#define COMMENT_MESSAGE                                        "A comment was added"
#define RESET_MESSAGE                                          "The alarm was reset"
#define SUPPRESSED_MESSAGE                                     "The alarm was suppressed"
#define UNSUPPRESSED_MESSAGE                                   "The alarm was unsuppressed"
#define PLACEDINSERVICE_MESSAGE                                "The alarm was placed in service"
#define REMOVEDFROMSRVICE_MESSAGE                              "The alarm was removed from service"
#define REALARM_MESSAGE                                        "Re-alarm time expired"
#define SEVERITY_INCREASED_MESSAGE                             "The alarm severity has increased"
#define SEVERITY_DECREASED_MESSAGE                             "The alarm severity has decreased"
#define ACKED_TEXT                                             "Acknowledged"
#define UNACKED_TEXT                                           "Unacknowledged"
#define CONFIRMED_TEXT                                         "Confirmed"
#define UNCONFIRMED_TEXT                                       "Unconfirmed"
#define LATCHED_TEXT                                           "Latched"
#define NOT_LATCHED_TEXT                                       "Not Latched"
#define SUPPRESSED_TEXT                                        "Suppressed"
#define NOT_SUPPRESSED_TEXT                                    "Not Suppresssed"
#define IN_SERVICE_TEXT                                        "In Service"
#define OUT_OF_SERVICE_TEXT                                    "Out Of Service"
#define ACKED_MESSAGE                                          "The alarm was acknowledged"
#define CONFIRMED_MESSAGE                                      "The alarm was confirmed"
#define ACTIVE_TEXT                                            "Active"
#define ACTIVE_HIGHHIGH_TEXT                                   "HighHigh active"
#define ACTIVE_HIGH_TEXT                                       "High active"
#define ACTIVE_LOW_TEXT                                        "Low active"
#define ACTIVE_LOWLOW_TEXT                                     "LowLow active"
#define INACTIVE_HIGHHIGH_TEXT                                 "HighHigh inactive"
#define INACTIVE_HIGH_TEXT                                     "High inactive"
#define INACTIVE_LOW_TEXT                                      "Low inactive"
#define INACTIVE_LOWLOW_TEXT                                   "LowLow inactive"
#define INACTIVE_TEXT                                          "Inactive"
#define UNSHELVED_TEXT                                         "Unshelved"
#define ONESHOTSHELVED_TEXT                                    "OneShotShelved"
#define TIMEDSHELVED_TEXT                                      "TimedShelved"
#define SHELVEDTIMEEXPIRED_MESSAGE                             "The shelved alarm shelving time expired"
#define UNSHELVED_MESSAGE                                      "The alarm was unshelved"
#define TIMEDSHELVE_MESSAGE                                    "The alarm was shelved for a timed duration"
#define ONESHOTSHELVE_MESSAGE                                  "The alarm was shelved"

#define STATIC_QN(name) {0, UA_STRING_STATIC(name)}
static const UA_QualifiedName stateVariableIdQN = STATIC_QN(FIELD_STATEVARIABLE_ID);

static const UA_QualifiedName fieldEnabledStateQN = STATIC_QN(CONDITION_FIELD_ENABLEDSTATE);
static const UA_QualifiedName fieldRetainQN = STATIC_QN(CONDITION_FIELD_RETAIN);
static const UA_QualifiedName fieldSeverityQN = STATIC_QN(CONDITION_FIELD_SEVERITY);
static const UA_QualifiedName fieldQualityQN = STATIC_QN(CONDITION_FIELD_SEVERITY);
static const UA_QualifiedName fieldMessageQN = STATIC_QN(CONDITION_FIELD_MESSAGE);
static const UA_QualifiedName fieldAckedStateQN = STATIC_QN(CONDITION_FIELD_ACKEDSTATE);
static const UA_QualifiedName fieldConfirmedStateQN = STATIC_QN(CONDITION_FIELD_CONFIRMEDSTATE);
static const UA_QualifiedName fieldActiveStateQN = STATIC_QN(CONDITION_FIELD_ACTIVESTATE);
static const UA_QualifiedName fieldLatchedStateQN = STATIC_QN(CONDITION_FIELD_LATCHEDSTATE);
static const UA_QualifiedName fieldSuppressedStateQN = STATIC_QN(CONDITION_FIELD_SUPPRESSEDSTATE);
static const UA_QualifiedName fieldSuppressedOrShelvedQN = STATIC_QN(CONDITION_FIELD_SUPPRESSEDORSHELVED);
static const UA_QualifiedName fieldOutOfServiceStateQN = STATIC_QN(CONDITION_FIELD_OUTOFSERVICESTATE);
static const UA_QualifiedName fieldShelvingStateQN = STATIC_QN(CONDITION_FIELD_SHELVINGSTATE);
static const UA_QualifiedName fieldMaxTimeShelvedQN = STATIC_QN(CONDITION_FIELD_MAXTIMESHELVED);
static const UA_QualifiedName fieldOnDelayQN = STATIC_QN(CONDITION_FIELD_ONDELAY);
static const UA_QualifiedName fieldOffDelayQN = STATIC_QN(CONDITION_FIELD_OFFDELAY);
static const UA_QualifiedName fieldReAlarmTimeQN = STATIC_QN(CONDITION_FIELD_REALARMTIME);
static const UA_QualifiedName fieldReAlarmRepeatCountQN = STATIC_QN(CONDITION_FIELD_REALARMREPEATCOUNT);
static const UA_QualifiedName fieldTimeQN = STATIC_QN(CONDITION_FIELD_TIME);
static const UA_QualifiedName fieldCommentQN = STATIC_QN(CONDITION_FIELD_COMMENT);
static const UA_QualifiedName fieldEventIdQN = STATIC_QN(CONDITION_FIELD_EVENTID);
static const UA_QualifiedName fieldSourceNodeQN = STATIC_QN(CONDITION_FIELD_SOURCENODE);
static const UA_QualifiedName fieldInputNodeQN = STATIC_QN(CONDITION_FIELD_INPUTNODE);
static const UA_QualifiedName fieldLimitStateQN = STATIC_QN(CONDITION_FIELD_LIMITSTATE);
static const UA_QualifiedName fieldLowLimitQN = STATIC_QN(CONDITION_FIELD_LOWLIMIT);
static const UA_QualifiedName fieldLowLowLimitQN = STATIC_QN(CONDITION_FIELD_LOWLOWLIMIT);
static const UA_QualifiedName fieldHighLimitQN = STATIC_QN(CONDITION_FIELD_HIGHLIMIT);
static const UA_QualifiedName fieldHighHighLimitQN = STATIC_QN(CONDITION_FIELD_HIGHHIGHLIMIT);
static const UA_QualifiedName fieldLowStateQN = STATIC_QN(CONDITION_FIELD_LOWSTATE);
static const UA_QualifiedName fieldLowLowStateQN = STATIC_QN(CONDITION_FIELD_LOWLOWSTATE);
static const UA_QualifiedName fieldHighStateQN = STATIC_QN(CONDITION_FIELD_HIGHSTATE);
static const UA_QualifiedName fieldHighHighStateQN = STATIC_QN(CONDITION_FIELD_HIGHHIGHSTATE);
static const UA_QualifiedName fieldSeverityLowQN = STATIC_QN(CONDITION_FIELD_SEVERITYLOW);
static const UA_QualifiedName fieldSeverityLowLowQN = STATIC_QN(CONDITION_FIELD_SEVERITYLOWLOW);
static const UA_QualifiedName fieldSeverityHighQN = STATIC_QN(CONDITION_FIELD_SEVERITYHIGH);
static const UA_QualifiedName fieldSeverityHighHighQN = STATIC_QN(CONDITION_FIELD_SEVERITYHIGHHIGH);
static const UA_QualifiedName fieldBaseLowLimitQN = STATIC_QN(CONDITION_FIELD_BASELOWLIMIT);
static const UA_QualifiedName fieldBaseLowLowLimitQN = STATIC_QN(CONDITION_FIELD_BASELOWLOWLIMIT);
static const UA_QualifiedName fieldBaseHighLimitQN = STATIC_QN(CONDITION_FIELD_BASEHIGHLIMIT);
static const UA_QualifiedName fieldBaseHighHighLimitQN = STATIC_QN(CONDITION_FIELD_BASEHIGHHIGHLIMIT);
static const UA_QualifiedName fieldLowDeadbandQN = STATIC_QN(CONDITION_FIELD_LOWDEADBAND);
static const UA_QualifiedName fieldLowLowDeadbandQN = STATIC_QN(CONDITION_FIELD_LOWLOWDEADBAND);
static const UA_QualifiedName fieldHighDeadbandQN = STATIC_QN(CONDITION_FIELD_HIGHDEADBAND);
static const UA_QualifiedName fieldHighHighDeadbandQN = STATIC_QN(CONDITION_FIELD_HIGHHIGHDEADBAND);
static const UA_QualifiedName fieldEngineeringUnitsQN = STATIC_QN(CONDITION_FIELD_ENGINEERINGUNITS);
static const UA_QualifiedName fieldNormalStateQN = STATIC_QN(CONDITION_FIELD_NORMALSTATE);
static const UA_QualifiedName fieldExpectedTimeQN = STATIC_QN(CONDITION_FIELD_EXPECTEDTIME);
static const UA_QualifiedName fieldTargetValueNodeQN = STATIC_QN(CONDITION_FIELD_TARGETVALUENODE);
static const UA_QualifiedName fieldToleranceQN = STATIC_QN(CONDITION_FIELD_TOLERANCE);

#ifdef UA_ENABLE_ENCRYPTION
static const UA_QualifiedName fieldExpirationLimitQN = STATIC_QN(CONDITION_FIELD_EXPIRATION_LIMIT);
#endif

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

#define CONDITION_ASSERT_GOTOLABEL(retval, logMessage, label)                \
    {                                                                                     \
        if(retval != UA_STATUSCODE_GOOD) {                                                \
            CONDITION_LOG_ERROR(retval,logMessage)                                   \
            goto label;                                                                   \
        }                                                                                 \
    }

#define CONDITION_ASSERT_RETURN_VOID(retval, logMessage, deleteFunction)                  \
    {                                                                                     \
        if(retval != UA_STATUSCODE_GOOD) {                                                \
            UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,                 \
                         logMessage". StatusCode %s", UA_StatusCode_name(retval));        \
            deleteFunction                                                                \
            return;                                                                       \
        }                                                                                 \
    }

#define CONDITION_PRINT_NODE_DEBUG(message,nodeId) \
{                                          \
    UA_String _nodeIdPrint;                \
    UA_String_init(&_nodeIdPrint);                \
    UA_NodeId_print (nodeId, &_nodeIdPrint); \
    UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,      \
        "%s %u "message" node: " UA_PRINTF_STRING_FORMAT, \
        __func__, __LINE__, UA_PRINTF_STRING_DATA(_nodeIdPrint));\
}\

static inline UA_Condition *getCondition (UA_Server *server, const UA_NodeId *conditionId);

static inline UA_ConditionBranch *getConditionBranch (UA_Server *server, const UA_NodeId *branchId);

UA_StatusCode
UA_getConditionId(UA_Server *server, const UA_NodeId *conditionNodeId,
                  UA_NodeId *outConditionId)
{
    UA_ConditionBranch * branch = getConditionBranch(server, conditionNodeId);
    if (!branch) return UA_STATUSCODE_BADNODEIDUNKNOWN;
    *outConditionId = branch->condition->mainBranch->id;
    return UA_STATUSCODE_GOOD;
}

/* Get the node id of the condition state */
static UA_ConditionBranch *
getConditionBranchFromConditionAndEvent(
    UA_Server *server,
    const UA_NodeId *conditionId,
    const UA_ByteString *eventId
)
{
    UA_Condition *condition = getCondition(server, conditionId);
    if (!condition) return NULL;
    return UA_Condition_GetBranchWithEventId(condition, eventId);
}

static UA_StatusCode
setConditionField(UA_Server *server, const UA_NodeId condition,
                  const UA_Variant* value, const UA_QualifiedName fieldName);

static UA_StatusCode
setConditionVariableFieldProperty(UA_Server *server, const UA_NodeId condition,
                                  const UA_Variant* value,
                                  const UA_QualifiedName variableFieldName,
                                  const UA_QualifiedName variablePropertyName);

static UA_StatusCode
addOptionalField(UA_Server *server, const UA_NodeId object,
                 const UA_NodeId conditionType, const UA_QualifiedName fieldName,
                 UA_NodeId *outOptionalNode);

static UA_StatusCode
getConditionFieldNodeId(UA_Server *server, const UA_NodeId *conditionNodeId,
                        const UA_QualifiedName* fieldName, UA_NodeId *outFieldNodeId);

static UA_StatusCode
getConditionFieldPropertyNodeId(UA_Server *server, const UA_NodeId *originCondition,
                                const UA_QualifiedName* variableFieldName,
                                const UA_QualifiedName* variablePropertyName,
                                UA_NodeId *outFieldPropertyNodeId);

static UA_StatusCode
getNodeIdValueOfNodeField(UA_Server *server, const UA_NodeId *nodeId,
                          UA_QualifiedName fieldName, UA_NodeId *outNodeId);

static UA_NodeId getTypeDefinitionId(UA_Server *server, const UA_NodeId *targetId)
{
    UA_BrowseDescription bd;
    UA_BrowseDescription_init(&bd);
    bd.browseDirection = UA_BROWSEDIRECTION_FORWARD;
    bd.includeSubtypes = false;
    bd.referenceTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASTYPEDEFINITION);
    bd.nodeId = *targetId;
    bd.resultMask = UA_BROWSERESULTMASK_TYPEDEFINITION;
    UA_UInt32 maxRefs = 1;
    UA_BrowseResult br;
    UA_BrowseResult_init (&br);
    Operation_Browse(server, &server->adminSession, &maxRefs, &bd, &br);
    if (br.statusCode != UA_STATUSCODE_GOOD || br.referencesSize != 1)
    {
        return UA_NODEID_NULL;
    }
    UA_NodeId id = br.references->nodeId.nodeId;
    br.references->nodeId.nodeId = UA_NODEID_NULL;
    UA_BrowseResult_clear(&br);
    return id;
}

static UA_Boolean
isTwoStateVariableInTrueState(UA_Server *server, const UA_NodeId *condition,
                              const UA_QualifiedName *twoStateVariable) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Get TwoStateVariableId NodeId */
    UA_NodeId twoStateVariableIdNodeId;
    UA_StatusCode retval = getConditionFieldPropertyNodeId(server, condition, twoStateVariable,
                                                           &stateVariableIdQN,
                                                           &twoStateVariableIdNodeId);
    if(retval != UA_STATUSCODE_GOOD) {
        return false;
    }

    /* Read Id value */
    UA_Variant tOutVariant;
    retval = readWithReadValue(server, &twoStateVariableIdNodeId, UA_ATTRIBUTEID_VALUE, &tOutVariant);
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

static UA_Boolean
fieldExists (UA_Server *server, const UA_NodeId *condition, const UA_QualifiedName *field)
{
    UA_NodeId tmp;
    UA_NodeId_init (&tmp);
    UA_StatusCode status = getConditionFieldNodeId(server, condition, field, &tmp);
    UA_Boolean exists = status == UA_STATUSCODE_GOOD && !UA_NodeId_isNull(&tmp);
    UA_NodeId_clear (&tmp);
    return exists;
}

static UA_StatusCode
readObjectPropertyDouble (UA_Server *server, UA_NodeId id, UA_QualifiedName property, UA_Double *valueOut)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant value;
    UA_StatusCode retval = readObjectProperty(server, id, property, &value);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    if (!UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE]))
    {
        UA_Variant_clear(&value);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    *valueOut = *(UA_Double*)value.data;
    UA_Variant_clear(&value);
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

    UA_LocalizedText stateText = UA_LOCALIZEDTEXT(LOCALE, (char *) (uintptr_t) state);
    UA_Variant_setScalar(&value, &stateText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = setConditionField (server, *condition, &value, field);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "set State text failed",);
    return retval;
}


static UA_StatusCode
setOptionalTwoStateVariable (UA_Server *server, const UA_NodeId *condition, UA_QualifiedName field,
                                UA_Boolean idValue, const char *state)
{
   if (!fieldExists(server, condition, &field))return UA_STATUSCODE_GOOD;
   return setTwoStateVariable (server, condition, field, idValue, state);
}

static UA_StatusCode
updateShelvedStateMachineState (UA_Server *server, const UA_NodeId *shelvedStateId,
                                UA_NodeId currentStateIdValue, const char *currentStateText)
{
    UA_NodeId currentStateId;
    UA_Variant value;
    UA_StatusCode retval = getNodeIdWithBrowseName(server, shelvedStateId, UA_QUALIFIEDNAME(0, FIELD_CURRENT_STATE), &currentStateId);
    CONDITION_ASSERT_GOTOLABEL(retval, "Get CurrentState Id failed", done);

    UA_LocalizedText stateText = UA_LOCALIZEDTEXT(LOCALE, (char *) (uintptr_t) currentStateText);
    UA_Variant_setScalar(&value, &stateText, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = writeValueAttribute(server, currentStateId, &value);
    CONDITION_ASSERT_GOTOLABEL(retval, "Set CurrentState value failed", done);

    UA_Variant_setScalar(&value, &currentStateIdValue, &UA_TYPES[UA_TYPES_NODEID]);
    retval = setConditionField (server, currentStateId, &value, stateVariableIdQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set CurrentState Id failed",);

    //TODO update last transition

done:
    UA_NodeId_clear (&currentStateId);
    return retval;
}

static UA_StatusCode
setShelvedStateMachineUnshelved (UA_Server *server, const UA_NodeId *shelvedStateId)
{
    return updateShelvedStateMachineState(server, shelvedStateId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SHELVEDSTATEMACHINETYPE_UNSHELVED),
        UNSHELVED_TEXT
    );
}

static UA_StatusCode
setShelvedStateMachineTimedShelved (UA_Server *server, const UA_NodeId *shelvedStateId)
{
    return updateShelvedStateMachineState(server, shelvedStateId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SHELVEDSTATEMACHINETYPE_TIMEDSHELVED),
        TIMEDSHELVED_TEXT
    );
}

static UA_StatusCode
setShelvedStateMachineOneShotShelved (UA_Server *server, const UA_NodeId *shelvedStateId)
{
    return updateShelvedStateMachineState(server, shelvedStateId,
        UA_NODEID_NUMERIC(0, UA_NS0ID_SHELVEDSTATEMACHINETYPE_ONESHOTSHELVED),
        ONESHOTSHELVED_TEXT
    );
}

static UA_StatusCode
getShelvedStateMachineStateId (UA_Server *server, const UA_NodeId *shelvedStateId, UA_NodeId *shelvedId)
{
    UA_NodeId currentStateId;
    UA_StatusCode status = getNodeIdWithBrowseName (server, shelvedStateId, UA_QUALIFIEDNAME(0, FIELD_CURRENT_STATE), &currentStateId);
    if (status != UA_STATUSCODE_GOOD) goto done;
    status = getNodeIdValueOfNodeField(server, shelvedStateId, stateVariableIdQN, shelvedId);
    if (status != UA_STATUSCODE_GOOD) goto done;
done:
    UA_NodeId_clear (&currentStateId);
    return status;
}

/* Gets the NodeId of a Field (e.g. Severity) */
static inline UA_StatusCode
getConditionFieldNodeId(UA_Server *server, const UA_NodeId *conditionNodeId,
                        const UA_QualifiedName* fieldName, UA_NodeId *outFieldNodeId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    return getNodeIdWithBrowseName(server, conditionNodeId, *fieldName, outFieldNodeId);
}

/* Gets the NodeId of a Field Property (e.g. EnabledState/Id) */
static UA_StatusCode
getConditionFieldPropertyNodeId(UA_Server *server, const UA_NodeId *originCondition,
                                const UA_QualifiedName* variableFieldName,
                                const UA_QualifiedName* variablePropertyName,
                                UA_NodeId *outFieldPropertyNodeId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* 1) Find Variable Field of the Condition */
    UA_BrowsePathResult bprConditionVariableField =
        browseSimplifiedBrowsePath(server, *originCondition, 1, variableFieldName);
    if(bprConditionVariableField.statusCode != UA_STATUSCODE_GOOD)
        return bprConditionVariableField.statusCode;

    /* 2) Find Property of the Variable Field of the Condition */
    UA_BrowsePathResult bprVariableFieldProperty =
        browseSimplifiedBrowsePath(server, bprConditionVariableField.targets->targetId.nodeId,
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
getNodeIdValueOfNodeField(UA_Server *server, const UA_NodeId *condition,
                          UA_QualifiedName fieldName, UA_NodeId *outNodeId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant value;
    UA_StatusCode retval = getValueOfConditionField (server, condition, fieldName, &value);
    if(retval != UA_STATUSCODE_GOOD) return retval;
    if (!UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_NODEID]))
    {
        UA_Variant_clear(&value);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    *outNodeId = *(UA_NodeId*)value.data;
    UA_NodeId_init((UA_NodeId*)value.data);
    UA_Variant_clear(&value);
    return UA_STATUSCODE_GOOD;
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
getDurationValueOfConditionField(UA_Server *server, const UA_NodeId *condition,
                                UA_QualifiedName fieldName, UA_Duration *outValue) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant value;
    UA_StatusCode retval = getValueOfConditionField(server, condition, fieldName, &value);
    if(retval != UA_STATUSCODE_GOOD) return retval;
    if(!UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DURATION])) {
        UA_Variant_clear(&value);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    *outValue = *(UA_Duration *)value.data;
    UA_Variant_clear(&value);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
getInt16ValueOfConditionField(UA_Server *server, const UA_NodeId *condition,
                                 UA_QualifiedName fieldName, UA_Int16 *outValue) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Variant value;
    UA_StatusCode retval = getValueOfConditionField(server, condition, fieldName, &value);
    if(retval != UA_STATUSCODE_GOOD) return retval;
    if(!UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT16])) {
        UA_Variant_clear(&value);
        return UA_STATUSCODE_BADTYPEMISMATCH;
    }
    *outValue = *(UA_Int16 *)value.data;
    UA_Variant_clear(&value);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setRefreshMethodEventFields(UA_Server *server, const UA_NodeId *refreshEventNodId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_QualifiedName fieldSeverity = UA_QUALIFIEDNAME(0, CONDITION_FIELD_SEVERITY);
    UA_QualifiedName fieldSourceName = UA_QUALIFIEDNAME(0, CONDITION_FIELD_SOURCENAME);
    UA_QualifiedName fieldReceiveTime = UA_QUALIFIEDNAME(0, CONDITION_FIELD_RECEIVETIME);
    UA_String sourceNameString = UA_STRING("Server"); //server is the source of Refresh Events
    UA_UInt16 severityValue = REFRESHEVENT_SEVERITY_DEFAULT;
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

static inline UA_Boolean
UA_ConditionBranch_State_Acked(const UA_ConditionBranch *branch, UA_Server *server)
{
    return isTwoStateVariableInTrueState(server, &branch->id, &fieldAckedStateQN);
}

static inline UA_Boolean
UA_ConditionBranch_State_Confirmed(const UA_ConditionBranch *branch, UA_Server *server)
{
    return isTwoStateVariableInTrueState(server, &branch->id, &fieldConfirmedStateQN);
}

static inline UA_Boolean
UA_ConditionBranch_State_isConfirmable(const UA_ConditionBranch *branch, UA_Server *server)
{
    return fieldExists(server, &branch->id, &fieldConfirmedStateQN);
}

static inline UA_StatusCode
UA_ConditionBranch_State_setSuppressedOrShelved (UA_ConditionBranch *branch, UA_Server *server, UA_Boolean suppressedOrShelved)
{
    UA_Variant value;
    UA_Variant_setScalar(&value, &suppressedOrShelved, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = setConditionField (server, branch->id, &value, fieldSuppressedOrShelvedQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "set Condition SuppressedOrShelved failed",);
    return retval;
}

static inline UA_StatusCode
UA_ConditionBranch_State_setRetain(UA_ConditionBranch *branch, UA_Server *server, UA_Boolean retain)
{
    UA_Variant value;
    UA_Variant_setScalar(&value, &retain, &UA_TYPES[UA_TYPES_BOOLEAN]);
    UA_StatusCode retval = setConditionField (server, branch->id, &value, fieldRetainQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "set Condition Retain failed",);
    return retval;
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
UA_ConditionBranch_State_setQuality(UA_ConditionBranch *branch, UA_Server *server, UA_StatusCode quality)
{
    UA_Variant value;
    UA_Variant_setScalar(&value, &quality, &UA_TYPES[UA_TYPES_STATUSCODE]);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = setConditionField (server, branch->id, &value, fieldQualityQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting Condition Quality failed",);
    return retval;
}

static UA_StatusCode
UA_ConditionBranch_State_setComment(UA_ConditionBranch *branch, UA_Server *server, const UA_LocalizedText*comment)
{
    UA_Variant value;
    UA_Variant_setScalar(&value, (void*)(uintptr_t) comment, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = setConditionField (server, branch->id, &value, fieldCommentQN);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting Condition Comment failed",);
    return retval;
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

static inline UA_StatusCode
UA_ConditionBranch_State_setAckedState(UA_ConditionBranch *branch, UA_Server *server, UA_Boolean acked)
{
    return setTwoStateVariable (
        server, &branch->id, fieldAckedStateQN, acked, acked ? ACKED_TEXT : UNACKED_TEXT
    );
}

static inline UA_StatusCode
UA_ConditionBranch_State_setConfirmedState(UA_ConditionBranch *branch, UA_Server *server, UA_Boolean confirmed)
{
    return setOptionalTwoStateVariable (
        server, &branch->id, fieldConfirmedStateQN, confirmed, confirmed ? CONFIRMED_TEXT: UNCONFIRMED_TEXT
    );
}

static inline UA_Boolean
UA_Condition_State_Latched(const UA_Condition *condition, UA_Server *server)
{
    return isTwoStateVariableInTrueState(server, &condition->mainBranch->id, &fieldLatchedStateQN);
}

static inline UA_Boolean
UA_Condition_State_Enabled(const UA_Condition *condition, UA_Server *server)
{
    return isTwoStateVariableInTrueState(server, &condition->mainBranch->id, &fieldEnabledStateQN);
}

static inline UA_Boolean
UA_Condition_State_Active(const UA_Condition *condition, UA_Server *server)
{
    return isTwoStateVariableInTrueState(server, &condition->mainBranch->id, &fieldActiveStateQN);
}

static inline UA_StatusCode
UA_Condition_State_getOnDelay (const UA_Condition *condition, UA_Server *server, UA_Duration *outValue)
{
    return getDurationValueOfConditionField (
        server,
        &condition->mainBranch->id,
        fieldOnDelayQN,
        outValue
    );
}

static inline UA_Boolean
UA_Condition_State_hasOnDelay(UA_Condition *condition, UA_Server *server)
{
    return fieldExists (server, &condition->mainBranch->id, &fieldOnDelayQN);
}

static inline UA_StatusCode
UA_Condition_State_getOffDelay (const UA_Condition *condition, UA_Server *server, UA_Duration *outValue)
{
    return getDurationValueOfConditionField (
        server,
        &condition->mainBranch->id,
        fieldOffDelayQN,
        outValue
    );
}

static inline UA_StatusCode
UA_Condition_State_getReAlarmTime(UA_Condition *condition, UA_Server *server, UA_Duration *outValue)
{
    return getDurationValueOfConditionField (
        server,
        &condition->mainBranch->id,
        fieldReAlarmTimeQN,
        outValue
    );
}

static inline UA_StatusCode
UA_Condition_State_getReAlarmRepeatCount(UA_Condition *condition, UA_Server *server, UA_Int16 *reAlarmRepeatCount)
{
    return getInt16ValueOfConditionField(
        server,
        &condition->mainBranch->id,
        fieldReAlarmRepeatCountQN,
        reAlarmRepeatCount
    );
}

static inline UA_Boolean
UA_Condition_State_hasOffDelay(UA_Condition *condition, UA_Server *server)
{
    return fieldExists (server, &condition->mainBranch->id, &fieldOffDelayQN);
}

static inline UA_StatusCode
UA_Condition_State_setEnabledState(UA_Condition *condition, UA_Server *server, UA_Boolean enabled)
{
    return setTwoStateVariable (
        server, &condition->mainBranch->id, fieldEnabledStateQN, enabled,
        enabled ? ENABLED_TEXT : DISABLED_TEXT
    );
}

static inline UA_StatusCode
UA_Condition_State_setActiveState (UA_Condition *condition, UA_Server *server, UA_Boolean active)
{
    return setTwoStateVariable (
        server, &condition->mainBranch->id, fieldActiveStateQN, active,
        active ? ACTIVE_TEXT : INACTIVE_TEXT
    );
}

static inline UA_StatusCode
UA_Condition_State_setLatchedState (UA_Condition *condition, UA_Server *server, UA_Boolean latched)
{
    return setOptionalTwoStateVariable (
        server, &condition->mainBranch->id, fieldLatchedStateQN, latched,
        latched ? LATCHED_TEXT: NOT_LATCHED_TEXT
    );
}

static inline UA_StatusCode
UA_Condition_State_setSuppressedState (UA_Condition *condition, UA_Server *server, UA_Boolean suppressed)
{
    return setOptionalTwoStateVariable (
        server, &condition->mainBranch->id, fieldSuppressedStateQN, suppressed,
        suppressed ? SUPPRESSED_TEXT : NOT_SUPPRESSED_TEXT
    );
}

static inline UA_StatusCode
UA_Condition_State_setOutOfServiceState (UA_Condition *condition, UA_Server *server, UA_Boolean outOfService)
{
    return setOptionalTwoStateVariable (
        server, &condition->mainBranch->id, fieldOutOfServiceStateQN, outOfService,
        outOfService ? OUT_OF_SERVICE_TEXT : IN_SERVICE_TEXT
    );
}

static UA_StatusCode
UA_Condition_State_setShelvingStateUnshelved(const UA_Condition *condition, UA_Server *server)
{
    UA_NodeId shelvingStateId;
    UA_StatusCode retval = getNodeIdWithBrowseName(server, &condition->mainBranch->id, fieldShelvingStateQN, &shelvingStateId);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = setShelvedStateMachineUnshelved(server, &shelvingStateId);
    UA_NodeId_clear (&shelvingStateId);
    return retval;
}

static UA_StatusCode
UA_Condition_State_setShelvingStateOneShot(const UA_Condition *condition, UA_Server *server, const UA_Duration *shelvedTime)
{
    UA_NodeId shelvingStateId;
    UA_StatusCode retval = getNodeIdWithBrowseName(server, &condition->mainBranch->id, fieldShelvingStateQN, &shelvingStateId);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = setShelvedStateMachineOneShotShelved(server, &shelvingStateId);
    UA_NodeId_clear (&shelvingStateId);
    return retval;
}

static UA_StatusCode
UA_Condition_State_setShelvingStateTimed(UA_Condition *condition, UA_Server *server, UA_Duration shelvedTime)
{
    UA_NodeId shelvingStateId;
    UA_StatusCode retval = getNodeIdWithBrowseName(server, &condition->mainBranch->id, fieldShelvingStateQN, &shelvingStateId);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    retval = setShelvedStateMachineTimedShelved(server, &shelvingStateId);
    UA_NodeId_clear (&shelvingStateId);
    return retval;
}

static UA_NodeId
UA_Condition_State_getShelvingStateId(UA_Condition *condition, UA_Server *server)
{
    UA_NodeId shelvingStateId;
    UA_StatusCode retval = getNodeIdWithBrowseName(server, &condition->mainBranch->id, fieldShelvingStateQN, &shelvingStateId);
    if (retval != UA_STATUSCODE_GOOD) return UA_NODEID_NULL;
    UA_NodeId stateIdValue;
    retval = getShelvedStateMachineStateId(server, &shelvingStateId, &stateIdValue);
    UA_NodeId_clear (&shelvingStateId);
    if (retval != UA_STATUSCODE_GOOD) return UA_NODEID_NULL;
    return stateIdValue;
}

static inline UA_Boolean
UA_Condition_State_Suppressed(const UA_Condition*condition, UA_Server *server)
{
    return isTwoStateVariableInTrueState(server, &condition->mainBranch->id, &fieldSuppressedStateQN);
}

static inline UA_Boolean
UA_Condition_State_OutOfService(const UA_Condition *condition, UA_Server *server)
{
    return isTwoStateVariableInTrueState(server, &condition->mainBranch->id, &fieldOutOfServiceStateQN);
}

static inline UA_Boolean
UA_Condition_State_isShelved (UA_Condition *condition, UA_Server *server)
{
    UA_NodeId shelvingStateId = UA_Condition_State_getShelvingStateId(condition, server);
    UA_NodeId timedShelveId = UA_NODEID_NUMERIC(0, UA_NS0ID_SHELVEDSTATEMACHINETYPE_TIMEDSHELVED);
    UA_NodeId oneShotShelveId = UA_NODEID_NUMERIC(0, UA_NS0ID_SHELVEDSTATEMACHINETYPE_ONESHOTSHELVED);
    UA_Boolean shelved = UA_NodeId_equal(&shelvingStateId, &timedShelveId) ||
        UA_NodeId_equal(&shelvingStateId, &oneShotShelveId);
    UA_NodeId_clear (&shelvingStateId);
    return shelved;
}

static inline UA_Boolean
UA_Condition_State_isOneShotShelved (UA_Condition *condition, UA_Server *server)
{
    UA_NodeId shelvingStateId = UA_Condition_State_getShelvingStateId(condition, server);
    UA_NodeId oneShotShelveId = UA_NODEID_NUMERIC(0, UA_NS0ID_SHELVEDSTATEMACHINETYPE_ONESHOTSHELVED);
    UA_Boolean oneShotShelved = UA_NodeId_equal(&shelvingStateId, &oneShotShelveId);
    UA_NodeId_clear (&shelvingStateId);
    return oneShotShelved;
}

static inline UA_StatusCode
UA_Condition_State_updateSuppressedOrShelved (UA_Condition *condition, UA_Server *server)
{
    UA_Boolean shelved = UA_Condition_State_isShelved(condition, server);
    UA_Boolean suppressed = UA_Condition_State_Suppressed(condition, server);
    UA_Boolean outOfService = UA_Condition_State_OutOfService(condition, server);
    UA_Boolean value = suppressed || outOfService || shelved;
    return UA_ConditionBranch_State_setSuppressedOrShelved(condition->mainBranch, server, value);
}

static UA_StatusCode
UA_Condition_State_getMaxTimeShelved (UA_Condition *condition, UA_Server *server, UA_Duration *maxTimeShelved)
{
    return getDurationValueOfConditionField(server, &condition->mainBranch->id, fieldMaxTimeShelvedQN, maxTimeShelved);
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

    /* Trigger the event for Condition*/
    UA_Boolean eventRetain = UA_ConditionBranch_State_Retain(branch,server);
    UA_Boolean lastEventRetain = branch->lastEventRetainValue;
    branch->lastEventRetainValue = eventRetain;
    /* Events are only generated for Conditions that have their Retain field set to True and for the initial transition
     * of the Retain field from True to False. */
    if (lastEventRetain == false && eventRetain == false)
    {
        return UA_STATUSCODE_GOOD;
    }

    UA_ByteString_clear(&branch->eventId);
    retval = triggerEvent(server, branch->id, branch->condition->sourceId, &branch->eventId, false);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Triggering condition event failed",);
    return retval;
}

static UA_StatusCode removeConditionBranch (UA_Server *server, UA_ConditionBranch *branch);

static void
UA_ConditionBranch_evaluateRetainState(UA_ConditionBranch *branch, UA_Server *server);

static UA_StatusCode
UA_ConditionBranch_notifyNewBranchState (UA_ConditionBranch *branch, UA_Server *server, const UA_ConditionEventInfo *info) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_ConditionBranch_evaluateRetainState(branch, server);
    UA_StatusCode status = UA_ConditionBranch_triggerEvent (branch, server, info);
    if (status != UA_STATUSCODE_GOOD) return status;
    if (!branch->isMainBranch && !UA_ConditionBranch_State_Retain(branch, server))
    {
        UA_Condition *condition = branch->condition;
        status = removeConditionBranch(server, branch);
        UA_ConditionBranch_evaluateRetainState(condition->mainBranch, server);
        if (!UA_ConditionBranch_State_Retain(condition->mainBranch, server))
        {
            status = UA_ConditionBranch_triggerEvent (condition->mainBranch, server, info);
        }
    }
    return status;
}

static UA_StatusCode
addTimedCallback(UA_Server *server, UA_ServerCallback callback,
                 void *data, UA_DateTime date, UA_UInt64 *callbackId) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    return server->config.eventLoop->
        addTimedCallback(server->config.eventLoop, (UA_Callback)callback,
                         server, data, date, callbackId);
}

static void
enabledEvaluateCondition (UA_Server *server, UA_Condition* condition)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if (!condition->fns.getInput) return;
    UA_UNLOCK(&server->serviceMutex);
    void *input = condition->fns.getInput(server, &condition->mainBranch->id, condition->context);
    if (input && condition->fns.evaluate) (void) condition->fns.evaluate (server, &condition->mainBranch->id, condition->context, input);
    if (condition->fns.inputFree) condition->fns.inputFree (input, condition->context);
    UA_LOCK(&server->serviceMutex);
}

static UA_StatusCode
conditionEnable (UA_Server *server, UA_Condition *condition, UA_Boolean enable)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    /* Cant enable/disable branches - only main condition */
    if (UA_Condition_State_Enabled(condition, server) == enable)
        return enable ? UA_STATUSCODE_BADCONDITIONALREADYENABLED : UA_STATUSCODE_BADCONDITIONALREADYDISABLED;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = UA_Condition_State_setEnabledState(condition, server, enable);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    /* Set condition message */
    UA_LocalizedText message = UA_LOCALIZEDTEXT(LOCALE, enable ? ENABLED_MESSAGE : DISABLED_MESSAGE);
    UA_ConditionBranch_State_setMessage(condition->mainBranch, server, &message);
    /**
     * When the Condition instance enters the Disabled state, the Retain Property of this
     * Condition shall be set to False by the Server to indicate to the Client that the
     * Condition instance is currently not of interest to Clients. This includes all
     * ConditionBranches if any branches exist.
     * https://reference.opcfoundation.org/Core/Part9/v105/docs/5.5
     */
    if (enable == false)
    {
        UA_ConditionBranch *branch;
        LIST_FOREACH(branch, &condition->branches, listEntry) {
            UA_ConditionBranch_State_setRetain(branch, server, false);
            retval = UA_ConditionBranch_triggerEvent (branch, server, NULL);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "triggering condition event failed",);
        }
    }
    else
    {
        enabledEvaluateCondition (server, condition);
        //resend notifications for any branch where retain = true
        size_t resent_count = 0;
        UA_ConditionBranch *branch;
        LIST_FOREACH(branch, &condition->branches, listEntry) {
            //check retain property for each branch
            if (!UA_ConditionBranch_State_Retain(branch, server))
                continue;

            retval = UA_ConditionBranch_triggerEvent (branch, server, NULL);
            if (retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                             "triggering condition event failed. StatusCode %s",
                             UA_StatusCode_name(retval));
            }
            resent_count++;
        }

        //always send a notification
        if (resent_count == 0)
        {
            retval = UA_ConditionBranch_triggerEvent(condition->mainBranch, server, NULL);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "triggering condition event failed",);
        }
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_Condition_enable (UA_Server *server, UA_NodeId conditionId, UA_Boolean enable)
{
    UA_LOCK (&server->serviceMutex);
    UA_Condition *condition = getCondition(server, &conditionId);
    if (!condition)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDINVALID;
    }
    UA_StatusCode retval = conditionEnable(server, condition, enable);
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
conditionBranch_addComment(UA_Server *server, UA_ConditionBranch *branch, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    if (!UA_Condition_State_Enabled(branch->condition, server))
    {
        return UA_STATUSCODE_BADCONDITIONDISABLED;
    }

    /* Set Comment. Check whether comment is empty -> leave the last value as is*/
    if(!UA_ByteString_equal(&comment->locale, &UA_STRING_NULL) &&
       !UA_ByteString_equal(&comment->text, &UA_STRING_NULL)) {
        retval = UA_ConditionBranch_State_setComment(branch, server, comment);
        CONDITION_ASSERT_RETURN_RETVAL (retval, "Set Condition Comment failed",);
    }
    return retval;
}

static UA_StatusCode
conditionBranch_addCommentAndEvent (UA_Server *server, UA_ConditionBranch *branch, const UA_LocalizedText *comment)
{
    UA_StatusCode ret = conditionBranch_addComment(server, branch, comment);
    if (ret != UA_STATUSCODE_GOOD) return ret;
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, COMMENT_MESSAGE)
    };
    return UA_ConditionBranch_triggerEvent (branch, server, &info);
}

static void
UA_ConditionBranch_evaluateRetainState(UA_ConditionBranch *branch, UA_Server *server)
{
    UA_Boolean retain = false;
    if (branch->isMainBranch)
    {
        /*Retain for main branch should remain true if there are still branches active*/
        size_t count = UA_Condition_getBranchCount(branch->condition);
        if (count > 1)
        {
            retain = true;
            goto done;
        }

        /*Active and latched only applicable for main condition*/
        if (UA_Condition_State_Active(branch->condition, server) ||
            UA_Condition_State_Latched(branch->condition, server))
        {
            retain = true;
            goto done;
        }
    }

    //branch still requires action
    retain = !UA_ConditionBranch_State_Acked(branch, server) ||
        (UA_ConditionBranch_State_isConfirmable(branch,server) && !UA_ConditionBranch_State_Confirmed(branch,server));
done:
    UA_ConditionBranch_State_setRetain(branch, server, retain);
}

static UA_StatusCode
conditionBranchAcknowledge(UA_Server *server, UA_ConditionBranch *branch, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if(UA_ConditionBranch_State_Acked(branch, server))
        return UA_STATUSCODE_BADCONDITIONBRANCHALREADYACKED;

    UA_StatusCode retval = UA_Condition_UserCallback_onAcked(server, branch->condition, &branch->id);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    UA_ConditionBranch_State_setAckedState(branch, server, true);
    UA_ConditionBranch_evaluateRetainState(branch, server);

    if (comment) conditionBranch_addComment (server, branch, comment);

    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, ACKED_MESSAGE)
    };
    return UA_ConditionBranch_notifyNewBranchState(branch, server, &info);
}

UA_StatusCode
UA_Server_Condition_acknowledge(UA_Server *server, UA_NodeId conditionId, const UA_LocalizedText *comment)
{
    UA_LOCK (&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranch(server, &conditionId);
    if (!branch)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = conditionBranchAcknowledge(server, branch, comment);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

static UA_StatusCode
conditionBranchConfirm(UA_Server *server, UA_ConditionBranch *branch, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if (UA_ConditionBranch_State_Confirmed(branch, server))
        return UA_STATUSCODE_BADCONDITIONBRANCHALREADYCONFIRMED;

    UA_StatusCode retval = UA_Condition_UserCallback_onConfirmed(server, branch->condition, &branch->id);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    UA_ConditionBranch_State_setConfirmedState(branch, server, true);
    UA_ConditionBranch_evaluateRetainState(branch, server);

    if (comment) conditionBranch_addComment (server, branch, comment);
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, CONFIRMED_MESSAGE)
    };
    return UA_ConditionBranch_notifyNewBranchState(branch, server, &info);
}

UA_StatusCode
UA_Server_Condition_confirm(UA_Server *server, UA_NodeId conditionId, const UA_LocalizedText *comment)
{
    UA_LOCK (&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranch(server, &conditionId);
    if (!branch)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = conditionBranchConfirm(server, branch, comment);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_Condition_setConfirmRequired(UA_Server *server, UA_NodeId conditionId)
{
    UA_LOCK (&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranch(server, &conditionId);
    if (!branch)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = UA_ConditionBranch_State_setConfirmedState (branch, server, false);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_Condition_setAcknowledgeRequired(UA_Server *server, UA_NodeId conditionId)
{
    UA_LOCK (&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranch(server, &conditionId);
    if (!branch)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    ret = UA_ConditionBranch_State_setAckedState(branch, server, false);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

static UA_StatusCode
condition_reset (UA_Server *server, UA_Condition *condition, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    /* For an Alarm Instance to be reset it must have been in Alarm, and returned to
     * normal and have been acknowledged/confirmed prior to being reset. */
    UA_Boolean validState = !UA_Condition_State_Active(condition, server) &&
                            UA_ConditionBranch_State_Acked(condition->mainBranch, server) &&
                            (!UA_ConditionBranch_State_isConfirmable(condition->mainBranch, server) || UA_ConditionBranch_State_Confirmed(condition->mainBranch, server));
    if (!validState) return UA_STATUSCODE_BADINVALIDSTATE;
    UA_Condition_State_setLatchedState(condition, server, false);
    UA_ConditionBranch_State_setRetain(condition->mainBranch, server, false);
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, RESET_MESSAGE)
    };
    if (comment) conditionBranch_addComment(server, condition->mainBranch, comment);
    return UA_ConditionBranch_triggerEvent (condition->mainBranch, server, &info);
}

static UA_StatusCode
condition_suppress (UA_Server *server, UA_Condition *condition, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Condition_State_setSuppressedState (condition, server, true);
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, SUPPRESSED_MESSAGE)
    };
    UA_Condition_State_updateSuppressedOrShelved (condition, server);

    if (comment) conditionBranch_addComment(server, condition->mainBranch, comment);
    return UA_ConditionBranch_triggerEvent (condition->mainBranch, server, &info);
}

static UA_StatusCode
condition_unsuppress (UA_Server *server, UA_Condition *condition, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Condition_State_setSuppressedState (condition, server, false);
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, UNSUPPRESSED_MESSAGE)
    };
    UA_Condition_State_updateSuppressedOrShelved (condition, server);
    if (comment) conditionBranch_addComment(server, condition->mainBranch, comment);
    return UA_ConditionBranch_triggerEvent (condition->mainBranch, server, &info);
}

UA_StatusCode
UA_Server_Condition_suppress(UA_Server *server, UA_NodeId conditionId, const UA_LocalizedText *comment)
{
    UA_LOCK (&server->serviceMutex);
    UA_Condition *cond= getCondition (server, &conditionId);
    if (!cond)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = condition_suppress (server, cond, comment);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode UA_EXPORT
UA_Server_Condition_unsuppress(UA_Server *server, UA_NodeId conditionId, const UA_LocalizedText *comment)
{
    UA_LOCK (&server->serviceMutex);
    UA_Condition *cond= getCondition (server, &conditionId);
    if (!cond)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = condition_unsuppress (server, cond, comment);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

static UA_StatusCode
condition_removeFromService (UA_Server *server, UA_Condition *condition, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Condition_State_setOutOfServiceState (condition, server, true);
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, REMOVEDFROMSRVICE_MESSAGE)
    };
    UA_Condition_State_updateSuppressedOrShelved (condition, server);
    if (comment) conditionBranch_addComment(server, condition->mainBranch, comment);
    return UA_ConditionBranch_triggerEvent (condition->mainBranch, server, &info);
}

static UA_StatusCode
condition_placeInService (UA_Server *server, UA_Condition *condition, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Condition_State_setOutOfServiceState (condition, server, false);
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, PLACEDINSERVICE_MESSAGE)
    };
    UA_Condition_State_updateSuppressedOrShelved (condition, server);
    if (comment) conditionBranch_addComment(server, condition->mainBranch, comment);
    return UA_ConditionBranch_triggerEvent (condition->mainBranch, server, &info);
}


UA_StatusCode
UA_Server_Condition_removeFromService(UA_Server *server, UA_NodeId conditionId, const UA_LocalizedText *comment)
{
    UA_LOCK (&server->serviceMutex);
    UA_Condition *cond= getCondition (server, &conditionId);
    if (!cond)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = condition_removeFromService(server, cond, comment);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_Condition_placeInService(UA_Server *server, UA_NodeId conditionId, const UA_LocalizedText *comment)
{
    UA_LOCK (&server->serviceMutex);
    UA_Condition *cond= getCondition (server, &conditionId);
    if (!cond)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = condition_placeInService(server, cond, comment);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

static void
condition_clearShelve (UA_Server *server, UA_Condition *condition)
{
    UA_Condition_removeUnshelveCallback(condition, server);
    UA_Condition_State_setShelvingStateUnshelved(condition, server);
}

static UA_StatusCode
condition_unshelve (UA_Server *server, UA_Condition *condition, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    if (!UA_Condition_State_isShelved(condition, server))
    {
        return UA_STATUSCODE_BADCONDITIONNOTSHELVED;
    }
    condition_clearShelve(server, condition);
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, UNSHELVED_MESSAGE)
    };
    if (comment) conditionBranch_addComment(server, condition->mainBranch, comment);
    return UA_ConditionBranch_triggerEvent(condition->mainBranch, server, &info);
}

static void onShelvedTimeExpireCallback (UA_Server *server, void *data)
{
    UA_Condition *condition = (UA_Condition *) data;
    UA_LOCK(&server->serviceMutex);
    UA_Condition_State_setShelvingStateUnshelved(condition, server);
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, SHELVEDTIMEEXPIRED_MESSAGE)
    };
    UA_ConditionBranch_triggerEvent(condition->mainBranch, server, &info);
    UA_UNLOCK(&server->serviceMutex);
}

static UA_StatusCode
createUnshelveTimedCallback (UA_Server *server, UA_Condition *condition, UA_Duration shelveTime)
{
    UA_Condition_removeUnshelveCallback(condition, server);
    return addTimedCallback(
        server,
        onShelvedTimeExpireCallback,
        condition,
        UA_DateTime_nowMonotonic() + ((UA_DateTime) shelveTime * UA_DATETIME_MSEC),
        &condition->unshelveCallbackId
    );
}

static UA_StatusCode
condition_timedShelve (UA_Server *server, UA_Condition *condition,
                       UA_Duration shelvingTime, const UA_LocalizedText *comment)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Condition_removeUnshelveCallback(condition, server);
    UA_Duration maxTimeShelved;
    UA_StatusCode status = UA_Condition_State_getMaxTimeShelved(condition, server, &maxTimeShelved);
    if (status == UA_STATUSCODE_GOOD && shelvingTime > maxTimeShelved) return UA_STATUSCODE_BADSHELVINGTIMEOUTOFRANGE;
    status = UA_Condition_State_setShelvingStateTimed(condition, server, shelvingTime);
    if (status != UA_STATUSCODE_GOOD) return status;
    status = createUnshelveTimedCallback(server, condition, shelvingTime);
    if (status != UA_STATUSCODE_GOOD) return status;
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, TIMEDSHELVE_MESSAGE)
    };
    if (comment) conditionBranch_addComment(server, condition->mainBranch, comment);
    return UA_ConditionBranch_triggerEvent(condition->mainBranch, server, &info);
}

static UA_StatusCode
condition_oneShotShelveStart (UA_Server *server, UA_Condition *condition)
{
    UA_Condition_removeUnshelveCallback(condition, server);
    UA_Duration maxTimeShelved;
    UA_StatusCode getMaxTimeStatus = UA_Condition_State_getMaxTimeShelved(condition, server, &maxTimeShelved);
    UA_StatusCode status = UA_Condition_State_setShelvingStateOneShot(
        condition,
        server,
        getMaxTimeStatus == UA_STATUSCODE_GOOD ? &maxTimeShelved : NULL
    );
    if (status != UA_STATUSCODE_GOOD) return status;
    if (getMaxTimeStatus == UA_STATUSCODE_GOOD)
    {
        status = createUnshelveTimedCallback(server, condition, maxTimeShelved);
        if (status != UA_STATUSCODE_GOOD) return status;
    }
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
condition_oneShotStateInactive (UA_Server *server, UA_Condition *condition)
{
    UA_Condition_removeUnshelveCallback(condition, server);
    return UA_Condition_State_setShelvingStateOneShot(
        condition,
        server,
        NULL
    );
}

static UA_StatusCode
condition_oneShotShelve (UA_Server *server, UA_Condition *condition, const UA_LocalizedText *comment)
{
    if (UA_Condition_State_isOneShotShelved(condition, server)) return UA_STATUSCODE_BADCONDITIONALREADYSHELVED;
    UA_StatusCode status = UA_Condition_State_Active(condition, server) ?
        condition_oneShotShelveStart(server, condition) : condition_oneShotStateInactive(server, condition);
    if (status != UA_STATUSCODE_GOOD) return status;
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, ONESHOTSHELVE_MESSAGE)
    };
    if (comment) conditionBranch_addComment(server, condition->mainBranch, comment);
    return UA_ConditionBranch_triggerEvent(condition->mainBranch, server, &info);
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

/* Check whether the Condition Source Node has "EventSource" or one of its
 * subtypes inverse reference. */
static UA_Boolean
doesHasEventSourceReferenceExist(UA_Server *server, const UA_NodeId nodeToCheck) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

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
    if (condition->unshelveCallbackId) removeCallback(server, condition->unshelveCallbackId);
    if (condition->onDelayCallbackId) removeCallback (server, condition->onDelayCallbackId);
    if (condition->offDelayCallbackId) removeCallback (server, condition->offDelayCallbackId);
    UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
    if (condition->reAlarmCallbackId) removeCallback (server, condition->reAlarmCallbackId);
    UA_Condition_delete (condition);
    return ret;
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
                   const UA_CreateConditionProperties *conditionProperties, UA_ConditionFns conditionFns,
                   UA_Condition **out)
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
    condition->fns = conditionFns;
    condition->canBranch = conditionProperties->canBranch;
    *out = condition;
    return UA_STATUSCODE_GOOD;
fail:
    UA_Condition_delete(condition);
    return status;
}

static UA_StatusCode
newConditionInstanceEntry (UA_Server *server, const UA_NodeId *conditionNodeId, const UA_CreateConditionProperties *conditionProperties,
                           UA_ConditionFns conditionFns)
{
    UA_Condition *condition = NULL;
    UA_StatusCode status = newConditionEntry(server, conditionNodeId, conditionProperties, conditionFns, &condition);
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
    const UA_NodeId conditionId,
    const UA_NodeId *conditionType,
    const UA_CreateConditionProperties *conditionProperties,
    UA_ConditionFns conditionFns,
    UA_ConditionTypeSetupFn setupNodesFn,
    const void *setupNodesUserData
) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_StatusCode retval = addNode_finish(server, &server->adminSession, &conditionId);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Finish node failed",);

    retval = setConditionProperties(server, conditionType, &conditionId, conditionProperties);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    UA_UNLOCK(&server->serviceMutex);
    retval = setupNodesFn ? setupNodesFn (server, &conditionId, setupNodesUserData) : UA_STATUSCODE_GOOD;
    UA_LOCK(&server->serviceMutex);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setup Nodes failed",);

    if (!UA_NodeId_isNull(&conditionProperties->sourceNode))
    {
        /* Make sure the ConditionSource has HasEventSource or one of its SubTypes ReferenceType. If the source has no
         * reference type then create a has event source from the server to the source */
        UA_NodeId serverObject = UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER);
        if (!UA_NodeId_equal(&serverObject, &conditionProperties->sourceNode) &&
            !doesHasEventSourceReferenceExist(server, conditionProperties->sourceNode))
        {
            UA_NodeId hasEventSourceId = UA_NODEID_NUMERIC(0, UA_NS0ID_HASEVENTSOURCE);
            retval = addRef(server, serverObject, hasEventSourceId, conditionProperties->sourceNode, true);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Creating HasHasEventSource Reference to the Server Object failed",);
        }

        /* create HasCondition Reference (HasCondition should be forward from the
         * ConditionSourceNode to the Condition. else, HasCondition should be
         * forward from the ConditionSourceNode to the ConditionType Node) */
        UA_NodeId hasCondition = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCONDITION);
        if(!UA_NodeId_isNull(&conditionProperties->hierarchialReferenceType)) {
            retval = addRef(server, conditionProperties->sourceNode, hasCondition, conditionId, true);
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
    return newConditionInstanceEntry (server, &conditionId, conditionProperties, conditionFns);
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
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding Condition failed", );


    return UA_STATUSCODE_GOOD;
}

/* Create condition instance. The function checks first whether the passed
 * conditionType is a subType of ConditionType. Then checks whether the
 * condition source has HasEventSource reference to its parent. If not, a
 * HasEventSource reference will be created between condition source and server
 * object. To expose the condition in address space, a hierarchical
 * ReferenceType should be passed to create the reference to condition source.
 * Otherwise, UA_NODEID_NULL should be passed to make the condition unexposed. */
UA_StatusCode
__UA_Server_createCondition(UA_Server *server,
                          const UA_NodeId conditionId,
                          const UA_NodeId conditionType,
                          const UA_CreateConditionProperties *conditionProperties,
                          UA_ConditionFns conditionFns,
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
    UA_UNLOCK(&server->serviceMutex);
    if(retval != UA_STATUSCODE_GOOD)
        return retval;

    UA_LOCK(&server->serviceMutex);
    retval = addCondition_finish(server, *outNodeId, &conditionType, conditionProperties,
                                 conditionFns, setupFn, setupData);
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

UA_StatusCode
UA_Server_Condition_setImplCallbacks(UA_Server *server, UA_NodeId conditionId, const UA_ConditionImplCallbacks *callbacks)
{
    UA_LOCK(&server->serviceMutex);
    UA_Condition *cond = getCondition(server, &conditionId);
    if (!cond)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    cond->callbacks = callbacks;
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_Condition_setContext(UA_Server *server, UA_NodeId conditionId, void *conditionContext)
{
    UA_LOCK(&server->serviceMutex);
    UA_Condition *cond = getCondition(server, &conditionId);
    if (!cond)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    cond->context = conditionContext;
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_Condition_getContext(UA_Server *server, UA_NodeId conditionId, void **conditionContext)
{
    UA_LOCK(&server->serviceMutex);
    UA_Condition *cond = getCondition(server, &conditionId);
    if (!cond)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    *conditionContext = cond->context;
    UA_UNLOCK(&server->serviceMutex);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode UA_ConditionBranch_createBranch (UA_ConditionBranch *branch, UA_Server *server)
{
    UA_NodeId sourceId;
    UA_NodeId_init (&sourceId);
    UA_ByteString lastEventId;
    UA_ByteString_init (&lastEventId);

    UA_NodeId conditionType = getTypeDefinitionId (server, &branch->id);
    if (UA_NodeId_isNull(&conditionType)) return UA_STATUSCODE_BADINTERNALERROR;
    conditionType = UA_NODEID_NUMERIC(0, UA_NS0ID_CONDITIONTYPE);

    UA_ObjectAttributes oa = UA_ObjectAttributes_default;
    oa.displayName = UA_LOCALIZEDTEXT("en", "ConditionBranch");
    const UA_QualifiedName qn = STATIC_QN("ConditionBranch");
    UA_NodeId branchId;
    UA_StatusCode retval = addNode_begin (server, UA_NODECLASS_OBJECT, UA_NODEID_NULL,
                                          UA_NODEID_NULL, UA_NODEID_NULL, qn, conditionType, &oa, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                                          NULL,
                                          &branchId
    );
    CONDITION_ASSERT_GOTOLABEL(retval,"Adding ConditionBranch failed", fail);

    /* Copy the state of the condition */
    retval = copyAllChildren (server, &server->adminSession, &branch->id, &branchId, true);
    CONDITION_ASSERT_GOTOLABEL(retval, "Copying Condition State failed", fail);

    /* Update branchId property */
    UA_Variant value;
    UA_Variant_setScalar(&value, (void*)(uintptr_t) &branchId, &UA_TYPES[UA_TYPES_NODEID]);
    retval = setConditionField(server, branchId, &value,
                               UA_QUALIFIEDNAME(0,CONDITION_FIELD_BRANCHID));
    CONDITION_ASSERT_GOTOLABEL(retval, "Set BranchId Field failed", fail);

    retval = newConditionBranchEntry (server, &branchId, &branch->eventId, branch->condition, false);
    UA_ByteString_clear(&lastEventId);
    CONDITION_ASSERT_GOTOLABEL(retval, "Creating ConditionBranch entry failed", fail);
    return UA_STATUSCODE_GOOD;
fail:
    UA_ByteString_clear(&lastEventId);
    deleteNode(server, branchId, true);
    return retval;
}

static UA_StatusCode
addOptionalVariableField(UA_Server *server, const UA_NodeId *originCondition,
                         const UA_QualifiedName *fieldName,
                         const UA_VariableNode *optionalVariableFieldNode,
                         UA_NodeId *outOptionalVariable) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_VariableAttributes vAttr = UA_VariableAttributes_default;
    vAttr.valueRank = optionalVariableFieldNode->valueRank;
    vAttr.displayName = UA_Session_getNodeDisplayName(&server->adminSession,
                                                      &optionalVariableFieldNode->head);
    vAttr.dataType = optionalVariableFieldNode->dataType;

    /* Get typedefintion */
    const UA_Node *type = getNodeType(server, &optionalVariableFieldNode->head);
    if(!type) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_USERLAND,
                       "Invalid VariableType. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADTYPEDEFINITIONINVALID));
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
    UA_StatusCode retval =
        addNode(server, UA_NODECLASS_VARIABLE, optionalVariable,
                *originCondition, referenceToParent, *fieldName,
                type->head.nodeId, &vAttr, &UA_TYPES[UA_TYPES_VARIABLEATTRIBUTES],
                NULL, outOptionalVariable);
    UA_NODESTORE_RELEASE(server, type);
    return retval;
}

static UA_StatusCode
addOptionalObjectField(UA_Server *server, const UA_NodeId *originCondition,
                       const UA_QualifiedName* fieldName,
                       const UA_ObjectNode *optionalObjectFieldNode,
                       UA_NodeId *outOptionalObject) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
    oAttr.displayName = UA_Session_getNodeDisplayName(&server->adminSession,
                                                      &optionalObjectFieldNode->head);

    /* Get typedefintion */
    const UA_Node *type = getNodeType(server, &optionalObjectFieldNode->head);
    if(!type) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_USERLAND,
                       "Invalid ObjectType. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADTYPEDEFINITIONINVALID));
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
    UA_StatusCode retval = addNode(server, UA_NODECLASS_OBJECT, optionalObject,
                                   *originCondition, referenceToParent, *fieldName,
                                   type->head.nodeId, &oAttr, &UA_TYPES[UA_TYPES_OBJECTATTRIBUTES],
                                   NULL, outOptionalObject);
    UA_NODESTORE_RELEASE(server, type);
    return retval;
}

static UA_StatusCode
addOptionalMethodField(UA_Server *server, const UA_NodeId *origin,
                       const UA_QualifiedName* fieldName,
                       const UA_MethodNode* optionalMethodFieldNode,
                       UA_NodeId *outOptionalObject) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_NodeId hasComponent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
    return addRef(server, *origin, hasComponent, optionalMethodFieldNode->head.nodeId, true);
}

static UA_StatusCode
addOptionalField(UA_Server *server, const UA_NodeId object,
                 const UA_NodeId type, const UA_QualifiedName fieldName,
                 UA_NodeId *outOptionalNode) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    /* Get optional Field NodId from Type -> user should give the
     * correct ConditionType or Subtype!!!! */
    UA_BrowsePathResult bpr = browseSimplifiedBrowsePath(server, type, 1, &fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
        return bpr.statusCode;

    /* Get Node */
    UA_NodeId optionalFieldNodeId = bpr.targets[0].targetId.nodeId;
    const UA_Node *optionalFieldNode = UA_NODESTORE_GET(server, &optionalFieldNodeId);
    if(NULL == optionalFieldNode) {
        UA_LOG_WARNING(server->config.logging, UA_LOGCATEGORY_USERLAND,
                       "Couldn't find optional Field Node in ConditionType. StatusCode %s",
                       UA_StatusCode_name(UA_STATUSCODE_BADNOTFOUND));
        UA_BrowsePathResult_clear(&bpr);
        return UA_STATUSCODE_BADNOTFOUND;
    }

    switch(optionalFieldNode->head.nodeClass) {
        case UA_NODECLASS_VARIABLE: {
            UA_StatusCode retval =
                addOptionalVariableField(server, &object, &fieldName,
                                         (const UA_VariableNode *)optionalFieldNode, outOptionalNode);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                             "Adding Condition Optional Variable Field failed. StatusCode %s",
                             UA_StatusCode_name(retval));
            }
            UA_BrowsePathResult_clear(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return retval;
        }
        case UA_NODECLASS_OBJECT:{
          UA_StatusCode retval =
              addOptionalObjectField(server, &object, &fieldName,
                                     (const UA_ObjectNode *)optionalFieldNode, outOptionalNode);
          if(retval != UA_STATUSCODE_GOOD) {
              UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                           "Adding Condition Optional Object Field failed. StatusCode %s",
                           UA_StatusCode_name(retval));
          }
          UA_BrowsePathResult_clear(&bpr);
          UA_NODESTORE_RELEASE(server, optionalFieldNode);
          return retval;
        }
        case UA_NODECLASS_METHOD: {
            UA_StatusCode retval =
                addOptionalMethodField(server, &object, &fieldName,
                                       (const UA_MethodNode *)optionalFieldNode, outOptionalNode);
            if(retval != UA_STATUSCODE_GOOD) {
                UA_LOG_ERROR(server->config.logging, UA_LOGCATEGORY_USERLAND,
                             "Adding Condition Optional Method Field failed. StatusCode %s",
                             UA_StatusCode_name(retval));
            }
            UA_BrowsePathResult_clear(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return retval;
        }
        default:
            UA_BrowsePathResult_clear(&bpr);
            UA_NODESTORE_RELEASE(server, optionalFieldNode);
            return UA_STATUSCODE_BADNOTSUPPORTED;
    }
}

/* Set the value of condition field (only scalar). */
static UA_StatusCode
setConditionField(UA_Server *server, const UA_NodeId condition,
                  const UA_Variant* value, const UA_QualifiedName fieldName) {
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    if(value->arrayLength != 0 || value->data <= UA_EMPTY_ARRAY_SENTINEL) {
      //TODO implement logic for array variants!
      CONDITION_ASSERT_RETURN_RETVAL(UA_STATUSCODE_BADNOTIMPLEMENTED,
                                     "Set Condition Field with Array value not implemented",);
    }

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

    if(value->arrayLength != 0 || value->data <= UA_EMPTY_ARRAY_SENTINEL) {
        //TODO implement logic for array variants!
        CONDITION_ASSERT_RETURN_RETVAL(UA_STATUSCODE_BADNOTIMPLEMENTED,
                                       "Set Property of Condition Field with Array value not implemented",);
    }

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

// -------- Interact with condition

static void reAlarmCallback (UA_Server *server, void *data);

static void UA_Condition_createReAlarmCallback (UA_Condition *condition, UA_Server *server)
{
    UA_Duration reAlarmTime;
    UA_StatusCode retval = UA_Condition_State_getReAlarmTime (condition, server, &reAlarmTime);
    if (retval != UA_STATUSCODE_GOOD) return;

    UA_Int16 repeatCount = 0;
    retval = UA_Condition_State_getReAlarmRepeatCount(condition, server, &repeatCount);
    if (retval != UA_STATUSCODE_GOOD || condition->reAlarmCount >= repeatCount)
    {
        return;
    }

    retval = addTimedCallback(
        server,
        reAlarmCallback,
        condition,
        UA_DateTime_nowMonotonic() + ((UA_DateTime) reAlarmTime * UA_DATETIME_MSEC),
        &condition->reAlarmCallbackId
    );
    if (retval != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR (retval, "Could not add timedCallback for ReAlarm");
        return;
    }
}

static void alarmTryBranch (UA_Server *server, UA_Condition *condition)
{
    if (!condition->canBranch) return;
    /* if condition requires acknowledgement branch*/
    if (!UA_ConditionBranch_State_Acked(condition->mainBranch, server) && UA_ConditionBranch_State_Retain(condition->mainBranch, server))
    {
        UA_StatusCode retval = UA_ConditionBranch_createBranch(condition->mainBranch, server);
        if (retval != UA_STATUSCODE_GOOD)
        {
            CONDITION_LOG_ERROR(retval, "Could not create new ConditionBranch")
        }
        UA_ConditionBranch_State_setAckedState(condition->mainBranch, server, true);
        UA_ConditionBranch_State_setConfirmedState(condition->mainBranch, server, true);
    }
}

static void alarmActivate (UA_Server *server, UA_Condition *condition, const UA_ConditionEventInfo *info)
{
    alarmTryBranch(server, condition);

    (void) UA_Condition_UserCallback_onActive(server, condition, &condition->mainBranch->id);
    //TODO check error

    /* 5.8.17 In OneShotShelving, a user requests that an Alarm be Shelved for
     * its current Active state or if not Active its next Active state*/
    if (UA_Condition_State_isOneShotShelved(condition, server))
    {
        condition_oneShotShelveStart(server, condition);
    }
    UA_Condition_State_setActiveState(condition, server, true);
    UA_Condition_State_setLatchedState(condition, server, true);
    UA_ConditionBranch_evaluateRetainState(condition->mainBranch, server);
    UA_ConditionBranch_triggerEvent(condition->mainBranch, server, info);
    UA_Condition_createReAlarmCallback(condition, server);
}

static void reAlarmCallback (UA_Server *server, void *data)
{
    UA_LOCK(&server->serviceMutex);
    UA_Condition *condition = (UA_Condition*) data;
    condition->reAlarmCount++;
    UA_ConditionEventInfo info = {
        .message = UA_LOCALIZEDTEXT(LOCALE, REALARM_MESSAGE)
    };
    alarmActivate(server, condition, &info);
    UA_UNLOCK(&server->serviceMutex);
}

static void onDelayExpiredCallback (UA_Server *server, void *data)
{
    UA_Condition *condition = (UA_Condition *) data;
    UA_LOCK(&server->serviceMutex);
    condition->onDelayCallbackId = 0;
    alarmActivate(server, condition, condition->_delayCallbackInfo);
    UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
    condition->_delayCallbackInfo = NULL;
    UA_UNLOCK(&server->serviceMutex);
}

static UA_StatusCode
alarmEnteringActive (UA_Server *server, UA_Condition *condition, const UA_ConditionEventInfo *info)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Boolean currentlyActive = UA_Condition_State_Active(condition, server);
    //Alarm already active
    if (currentlyActive)
    {
        /* The alarm shall stay active for the OffDelay time and shall not regenerate if
        * it returns to active */
        if (condition->offDelayCallbackId)
        {
            removeCallback (server, condition->offDelayCallbackId);
            condition->offDelayCallbackId = 0;
            UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
            condition->_delayCallbackInfo = NULL;
            return UA_STATUSCODE_GOOD;
        }
        /* Create event for the update in state */
        return UA_ConditionBranch_triggerEvent (condition->mainBranch, server, info);
    }

    /* OnDelay task is in progress - dont regenerate */
    if (condition->onDelayCallbackId) return UA_STATUSCODE_GOOD;

    if (!currentlyActive && UA_Condition_State_hasOnDelay(condition, server))
    {
        UA_Duration onDelay;
        retval = UA_Condition_State_getOnDelay(condition, server, &onDelay);
        if (retval != UA_STATUSCODE_GOOD)
        {
            CONDITION_LOG_ERROR (retval, "Could not get OnDelay");
            return retval;
        }

        if (info)
        {
            condition->_delayCallbackInfo = UA_ConditionEventInfo_new();
            if (!condition->_delayCallbackInfo) return UA_STATUSCODE_BADOUTOFMEMORY;
            retval = UA_ConditionEventInfo_copy(info, condition->_delayCallbackInfo);
        }

        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
            condition->_delayCallbackInfo = NULL;
            return retval;
        }

        retval = addTimedCallback (
            server,
            onDelayExpiredCallback,
            condition,
            UA_DateTime_nowMonotonic() + ((UA_DateTime) onDelay * UA_DATETIME_MSEC),
            &condition->onDelayCallbackId
        );
        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
            condition->_delayCallbackInfo = NULL;
            CONDITION_LOG_ERROR (retval, "Could not add timedCallback for onDelay");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        return UA_STATUSCODE_GOOD;
    }

    alarmActivate (server, condition, info);
    return UA_STATUSCODE_GOOD;
}

static void alarmSetInactive(UA_Server *server, UA_Condition *condition,
                            const UA_ConditionEventInfo *info)
{
    alarmTryBranch(server, condition);

    (void) UA_Condition_UserCallback_onInactive(server, condition, &condition->mainBranch->id);

    /* 5.8.17 The OneShotShelving will automatically clear when an Alarm returns to an inactive state. */
    if (UA_Condition_State_isOneShotShelved(condition, server))
    {
        condition_clearShelve (server, condition);
    }

    removeCallback(server, condition->reAlarmCallbackId);
    condition->reAlarmCount = 0;

    UA_Condition_State_setActiveState (condition, server, false);
    UA_ConditionBranch_evaluateRetainState(condition->mainBranch, server);
    UA_ConditionBranch_triggerEvent(condition->mainBranch, server, info);
}

static void offDelayExpiredCallback (UA_Server *server, void *data)
{
    UA_Condition*condition = (UA_Condition*) data;
    UA_LOCK(&server->serviceMutex);
    condition->offDelayCallbackId = 0;
    alarmSetInactive(server, condition, condition->_delayCallbackInfo);
    UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
    condition->_delayCallbackInfo = NULL;
    UA_UNLOCK(&server->serviceMutex);
}

static UA_StatusCode
alarmEnteringInactive (UA_Server *server, UA_Condition *condition, const UA_ConditionEventInfo *info)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* Alarm should stay deactivated and not regenerate - cancel any onDelay to active */
    if (condition->onDelayCallbackId)
    {
        removeCallback(server, condition->onDelayCallbackId);
        condition->onDelayCallbackId = 0;
        UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
        condition->_delayCallbackInfo = NULL;
        return UA_STATUSCODE_GOOD;
    }
    //off delay already in progress
    if (condition->offDelayCallbackId) return UA_STATUSCODE_GOOD;

    if (UA_Condition_State_Active(condition, server) &&
        UA_Condition_State_hasOffDelay(condition, server))
    {
        UA_Duration offDelay;
        retval = UA_Condition_State_getOffDelay(condition, server, &offDelay);
        if (retval != UA_STATUSCODE_GOOD)
        {
            CONDITION_LOG_ERROR (retval, "Could not get OnDelay");
            return retval;
        }

        if (info)
        {
            condition->_delayCallbackInfo = UA_ConditionEventInfo_new();
            if (!condition->_delayCallbackInfo) return UA_STATUSCODE_BADOUTOFMEMORY;
            retval = UA_ConditionEventInfo_copy(info, condition->_delayCallbackInfo);
        }

        if (retval != UA_STATUSCODE_GOOD)
        {
            UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
            condition->_delayCallbackInfo = NULL;
            return retval;
        }
        retval = addTimedCallback (
            server,
            offDelayExpiredCallback,
            condition,
            UA_DateTime_nowMonotonic() + ((UA_DateTime) offDelay * UA_DATETIME_MSEC),
            &condition->onDelayCallbackId
        );
        if (retval != UA_STATUSCODE_GOOD)
        {
            condition->_delayCallbackInfo = NULL;
            UA_ConditionEventInfo_delete(condition->_delayCallbackInfo);
            CONDITION_LOG_ERROR (retval, "Could not add timedCallback for onDelay");
            return UA_STATUSCODE_BADINTERNALERROR;
        }
        return UA_STATUSCODE_GOOD;
    }

    alarmSetInactive (server, condition, info);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
condition_updateAlarmActive (UA_Server *server, UA_Condition *condition,
                             const UA_ConditionEventInfo *info, UA_Boolean isActive)
{
    UA_LOCK_ASSERT(&server->serviceMutex,1);
    if (!condition) return UA_STATUSCODE_BADNODEIDINVALID;
    return isActive ? alarmEnteringActive (server, condition, info) :
           alarmEnteringInactive( server, condition, info);
}

static UA_StatusCode
conditionUpdateActive (UA_Server *server, const UA_NodeId *conditionId,
                                      const UA_ConditionEventInfo *info, UA_Boolean isActive)
{
    UA_LOCK_ASSERT(&server->serviceMutex, 1);
    UA_Condition *cond = getCondition(server, conditionId);
    if (!cond) return UA_STATUSCODE_BADNODEIDUNKNOWN;
    return condition_updateAlarmActive(server, cond, info, isActive);
}

UA_StatusCode
UA_Server_Condition_updateActive(UA_Server *server, UA_NodeId conditionId,
                                      const UA_ConditionEventInfo *info, UA_Boolean isActive)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = conditionUpdateActive(server, &conditionId, info, isActive);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_Condition_notifyStateChange(UA_Server *server, UA_NodeId condition, const UA_ConditionEventInfo *info)
{
    UA_LOCK(&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranch(server, &condition);
    if (!branch)
    {
        UA_UNLOCK(&server->serviceMutex);
        return UA_STATUSCODE_BADNODEIDUNKNOWN;
    }
    UA_StatusCode ret = UA_ConditionBranch_notifyNewBranchState(branch, server, info);
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

// -------- Method Node Callbacks

static UA_StatusCode
enableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize,
                     const UA_Variant *input, size_t outputSize,
                     UA_Variant *output)
{
    //TODO check object id type
    return UA_Server_Condition_enable(server, *objectId, true);
}

static UA_StatusCode
disableMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize, UA_Variant *output)
{
    //TODO check object id type
    return UA_Server_Condition_enable(server, *objectId, false);
}

static UA_StatusCode
addCommentMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *methodId,
                         void *methodContext, const UA_NodeId *objectId,
                         void *objectContext, size_t inputSize,
                         const UA_Variant *input, size_t outputSize,
                         UA_Variant *output)
{
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_LOCK(&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranchFromConditionAndEvent(
        server,
        objectId,
        (UA_ByteString *)input[0].data
    );
    if (!branch) {
        retval = UA_STATUSCODE_BADNODEIDUNKNOWN;
        CONDITION_LOG_ERROR(retval, "ConditionBranch based on EventId not found");
        goto done;
    }
    retval = conditionBranch_addCommentAndEvent (server, branch, (UA_LocalizedText *)input[1].data);
done:
    UA_UNLOCK(&server->serviceMutex);
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

static UA_StatusCode
acknowledgeMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output)
{
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_LOCK(&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranchFromConditionAndEvent(
        server,
        objectId,
        (UA_ByteString *)input[0].data
    );
    if (!branch) {
        retval = UA_STATUSCODE_BADNODEIDUNKNOWN;
        CONDITION_LOG_ERROR(retval, "ConditionBranch based on EventId not found");
        goto done;
    }

    UA_LocalizedText *comment = (UA_LocalizedText *)input[1].data;
    retval = conditionBranchAcknowledge(server, branch, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
confirmMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                      void *sessionContext, const UA_NodeId *methodId,
                      void *methodContext, const UA_NodeId *objectId,
                      void *objectContext, size_t inputSize,
                      const UA_Variant *input, size_t outputSize,
                      UA_Variant *output)
{
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_LOCK(&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranchFromConditionAndEvent(
        server,
        objectId,
        (UA_ByteString *)input[0].data
    );
    if (!branch) {
        retval = UA_STATUSCODE_BADNODEIDUNKNOWN;
        CONDITION_LOG_ERROR(retval, "ConditionBranch based on EventId not found");
        goto done;
    }
    UA_LocalizedText *comment = (UA_LocalizedText *)input[1].data;
    retval = conditionBranchConfirm (server, branch, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}


static UA_StatusCode
resetMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                    void *sessionContext, const UA_NodeId *methodId,
                    void *methodContext, const UA_NodeId *objectId,
                    void *objectContext, size_t inputSize,
                    const UA_Variant *input, size_t outputSize,
                    UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    retval = condition_reset(server, condition, NULL);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
reset2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                     void *sessionContext, const UA_NodeId *methodId,
                     void *methodContext, const UA_NodeId *objectId,
                     void *objectContext, size_t inputSize,
                     const UA_Variant *input, size_t outputSize,
                     UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    UA_LocalizedText *comment = (UA_LocalizedText *)input[0].data;
    retval = condition_reset(server, condition, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
suppressMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                       void *sessionContext, const UA_NodeId *methodId,
                       void *methodContext, const UA_NodeId *objectId,
                       void *objectContext, size_t inputSize,
                       const UA_Variant *input, size_t outputSize,
                       UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    retval = condition_suppress(server, condition, NULL);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
suppress2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                        void *sessionContext, const UA_NodeId *methodId,
                        void *methodContext, const UA_NodeId *objectId,
                        void *objectContext, size_t inputSize,
                        const UA_Variant *input, size_t outputSize,
                        UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    UA_LocalizedText *comment = (UA_LocalizedText *)input[0].data;
    retval = condition_suppress(server, condition, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
unsuppressMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                         void *sessionContext, const UA_NodeId *methodId,
                         void *methodContext, const UA_NodeId *objectId,
                         void *objectContext, size_t inputSize,
                         const UA_Variant *input, size_t outputSize,
                         UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    retval = condition_unsuppress(server, condition, NULL);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
unsuppress2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    UA_LocalizedText *comment = (UA_LocalizedText *)input[0].data;
    retval = condition_unsuppress(server, condition, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
placeInServiceMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                             void *sessionContext, const UA_NodeId *methodId,
                             void *methodContext, const UA_NodeId *objectId,
                             void *objectContext, size_t inputSize,
                             const UA_Variant *input, size_t outputSize,
                             UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    retval = condition_placeInService(server, condition, NULL);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
placeInService2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                              void *sessionContext, const UA_NodeId *methodId,
                              void *methodContext, const UA_NodeId *objectId,
                              void *objectContext, size_t inputSize,
                              const UA_Variant *input, size_t outputSize,
                              UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    UA_LocalizedText *comment = (UA_LocalizedText *)input[0].data;
    retval = condition_placeInService(server, condition, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
removeFromServiceMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                                void *sessionContext, const UA_NodeId *methodId,
                                void *methodContext, const UA_NodeId *objectId,
                                void *objectContext, size_t inputSize,
                                const UA_Variant *input, size_t outputSize,
                                UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    retval = condition_removeFromService(server, condition, NULL);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
removeFromService2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                                 void *sessionContext, const UA_NodeId *methodId,
                                 void *methodContext, const UA_NodeId *objectId,
                                 void *objectContext, size_t inputSize,
                                 const UA_Variant *input, size_t outputSize,
                                 UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    UA_LocalizedText *comment = (UA_LocalizedText *)input[0].data;
    retval = condition_removeFromService(server, condition, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
timedShelveMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                                 void *sessionContext, const UA_NodeId *methodId,
                                 void *methodContext, const UA_NodeId *objectId,
                                 void *objectContext, size_t inputSize,
                                 const UA_Variant *input, size_t outputSize,
                                 UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    UA_Duration shelvingTime = *(UA_Duration *) input[0].data;
    retval = condition_timedShelve(server, condition, shelvingTime, NULL);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
oneShotShelveMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    retval = condition_oneShotShelve(server, condition, NULL);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
unshelveMethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                            void *sessionContext, const UA_NodeId *methodId,
                            void *methodContext, const UA_NodeId *objectId,
                            void *objectContext, size_t inputSize,
                            const UA_Variant *input, size_t outputSize,
                            UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    retval = condition_unshelve(server, condition, NULL);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;

}

static UA_StatusCode
timedShelve2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                          void *sessionContext, const UA_NodeId *methodId,
                          void *methodContext, const UA_NodeId *objectId,
                          void *objectContext, size_t inputSize,
                          const UA_Variant *input, size_t outputSize,
                          UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    UA_Duration shelvingTime = *(UA_Duration *) input[0].data;
    const UA_LocalizedText *comment = (UA_LocalizedText *) input[1].data;
    retval = condition_timedShelve(server, condition, shelvingTime, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
oneShotShelve2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                            void *sessionContext, const UA_NodeId *methodId,
                            void *methodContext, const UA_NodeId *objectId,
                            void *objectContext, size_t inputSize,
                            const UA_Variant *input, size_t outputSize,
                            UA_Variant *output)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    const UA_LocalizedText *comment = (UA_LocalizedText *) input[0].data;
    retval = condition_oneShotShelve(server, condition, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}

static UA_StatusCode
unshelve2MethodCallback(UA_Server *server, const UA_NodeId *sessionId,
                       void *sessionContext, const UA_NodeId *methodId,
                       void *methodContext, const UA_NodeId *objectId,
                       void *objectContext, size_t inputSize,
                       const UA_Variant *input, size_t outputSize,
                       UA_Variant *output)
{
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_Condition *condition = getCondition(server, objectId);
    if (!condition) {
        retval = UA_STATUSCODE_BADNODEIDINVALID;
        goto done;
    }
    const UA_LocalizedText *comment = (UA_LocalizedText *) input[0].data;
    retval = condition_unshelve(server, condition, comment);
done:
    UA_UNLOCK(&server->serviceMutex);
    return retval;
}


static UA_StatusCode
setupAcknowledgeableConditionNodes (UA_Server *server, const UA_NodeId *condition,
                                              const UA_AcknowledgeableConditionProperties *properties)
{
    UA_NodeId acknowledgeableConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE);
    UA_StatusCode retval = setTwoStateVariable (server, condition, fieldAckedStateQN, true, ACKED_TEXT);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting initial Acked state failed",);
    /* add optional field ConfirmedState*/
    if (properties->confirmable)
    {
        retval = addOptionalField(server, *condition, acknowledgeableConditionTypeId,
                                  fieldConfirmedStateQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding ConfirmedState optional Field failed",);

        retval = setTwoStateVariable (server, condition, fieldConfirmedStateQN, true, CONFIRMED_TEXT);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting initial Confirmed state failed",);

        /* add reference from Condition to Confirm Method */
        UA_NodeId hasComponent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
        UA_NodeId confirm = UA_NODEID_NUMERIC(0, UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM);
        retval = addRef(server, *condition, hasComponent, confirm, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to Confirm Method failed",);
    }
    return retval;
}

UA_StatusCode UA_EXPORT
UA_Server_setupAcknowledgeableConditionNodes (UA_Server *server, const UA_NodeId *conditionId,
                                              const UA_AcknowledgeableConditionProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupAcknowledgeableConditionNodes(server, conditionId, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

static UA_StatusCode
setupAlarmConditionShelvingState(UA_Server *server, const UA_NodeId *condition)
{
    UA_NodeId alarmConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE);
    UA_NodeId shelvingStateId;
    UA_NodeId_init (&shelvingStateId);
    UA_StatusCode retval = addOptionalField(server, *condition, alarmConditionTypeId,
                              fieldShelvingStateQN, &shelvingStateId);
    CONDITION_ASSERT_GOTOLABEL(retval, "Adding ShelvingState optional Field failed",done);

    UA_NodeId shelvedStateType = UA_NODEID_NUMERIC(0, UA_NS0ID_SHELVEDSTATEMACHINETYPE);
    retval = addOptionalField(server, shelvingStateId, shelvedStateType, UA_QUALIFIEDNAME(0, SHELVEDSTATE_METHOD_TIMEDSHELVE2), NULL);
    CONDITION_ASSERT_GOTOLABEL(retval, "Adding ShelvingState optional TimedShelve2 Method failed",done);
    retval = addOptionalField(server, shelvingStateId, shelvedStateType, UA_QUALIFIEDNAME(0, SHELVEDSTATE_METHOD_ONESHOTSHELVE2), NULL);
    CONDITION_ASSERT_GOTOLABEL(retval, "Adding ShelvingState optional OneShotShelve2 Method failed",done);
    retval = addOptionalField(server, shelvingStateId, shelvedStateType, UA_QUALIFIEDNAME(0, SHELVEDSTATE_METHOD_UNSHELVE2), NULL);
    CONDITION_ASSERT_GOTOLABEL(retval, "Adding ShelvingState optional Unshelve2 Method failed", done);

    retval = setShelvedStateMachineUnshelved(server, &shelvingStateId);
    CONDITION_ASSERT_GOTOLABEL(retval, "Could not set the initial state",done);

done:
    UA_NodeId_clear (&shelvingStateId);
    return retval;
}

static UA_StatusCode
setupAlarmConditionNodes (UA_Server *server, const UA_NodeId *condition,
                                    const UA_AlarmConditionProperties *properties)
{
    UA_StatusCode retval = setupAcknowledgeableConditionNodes(server, condition, &properties->acknowledgeableConditionProperties);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    UA_NodeId alarmConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE);
    UA_Variant value;
    setTwoStateVariable (server, condition, fieldActiveStateQN, false, INACTIVE_TEXT);
    if (!UA_NodeId_isNull(&properties->inputNode))
    {
        UA_Variant_setScalar(&value,(void *)(uintptr_t) &properties->inputNode, &UA_TYPES[UA_TYPES_NODEID]);
        retval = setConditionField (server, *condition, &value, fieldInputNodeQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set InputNode Field failed",);
    }

    if (properties->isLatching)
    {
        retval = addOptionalField(server, *condition, alarmConditionTypeId,
                                  fieldLatchedStateQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding LatchedState optional Field failed",);
        setTwoStateVariable (server, condition, fieldLatchedStateQN, false, NOT_LATCHED_TEXT);

        /* add reference from Condition to Reset Method */
        UA_NodeId hasComponent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
        UA_NodeId reset = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_RESET);
        retval = addRef(server, *condition, hasComponent, reset, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to Reset Method failed",);

        /* add reference from Condition to Reset2 Method */
        UA_NodeId reset2 = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_RESET2);
        retval = addRef(server, *condition, hasComponent, reset2, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to Reset2 Method failed",);
    }

    if (properties->isSuppressible)
    {
        retval = addOptionalField(server, *condition, alarmConditionTypeId,
                                  fieldSuppressedStateQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding SuppressedState optional Field failed",);
        setTwoStateVariable(server, condition, fieldSuppressedStateQN, false, NOT_SUPPRESSED_TEXT);

        /* add reference from Condition to Suppress Method */
        UA_NodeId hasComponent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
        UA_NodeId suppress = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_SUPPRESS);
        retval = addRef(server, *condition, hasComponent, suppress, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to Suppress Method failed",);

        /* add reference from Condition to Suppress2 Method */
        UA_NodeId suppress2 = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_SUPPRESS2);
        retval = addRef(server, *condition, hasComponent, suppress2, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to Suppress2 Method failed",);

        /* add reference from Condition to UnSuppress Method */
        UA_NodeId unsuppress = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_UNSUPPRESS);
        retval = addRef(server, *condition, hasComponent, unsuppress, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to UnSuppress Method failed",);

        /* add reference from Condition to UnSuppress2 Method */
        UA_NodeId unsuppress2 = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_UNSUPPRESS2);
        retval = addRef(server, *condition, hasComponent, unsuppress2, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to UnSuppress2 Method failed",);
    }

    if (properties->isServiceable)
    {
        retval = addOptionalField(server, *condition, alarmConditionTypeId,
                                  fieldOutOfServiceStateQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding OutOfServiceState optional Field failed",);
        setTwoStateVariable(server, condition, fieldOutOfServiceStateQN, false, IN_SERVICE_TEXT);

        UA_NodeId hasComponent = UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT);
        UA_NodeId place = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_PLACEINSERVICE);
        retval = addRef(server, *condition, hasComponent, place, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to PlaceInService Method failed",);

        UA_NodeId place2 = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_PLACEINSERVICE2);
        retval = addRef(server, *condition, hasComponent, place2, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to PlaceInService2 Method failed",);

        UA_NodeId remove = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_REMOVEFROMSERVICE);
        retval = addRef(server, *condition, hasComponent, remove, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to RemoveFromService Method failed",);

        UA_NodeId remove2 = UA_NODEID_NUMERIC(0, UA_NS0ID_ALARMCONDITIONTYPE_REMOVEFROMSERVICE2);
        retval = addRef(server, *condition, hasComponent, remove2, true);
        CONDITION_ASSERT_RETURN_RETVAL(retval,
                                       "Adding HasComponent Reference to RemoveFromService2 Method failed",);
    }

    if (properties->isShelvable)
    {
        retval = setupAlarmConditionShelvingState (server, condition);
        if (retval != UA_STATUSCODE_GOOD) return retval;

        if (properties->maxTimeShelved)
        {
            retval = addOptionalField(server, *condition, alarmConditionTypeId,
                                      fieldMaxTimeShelvedQN, NULL);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding MaxTimeShelved optional Field failed",);
            UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->maxTimeShelved, &UA_TYPES[UA_TYPES_DURATION]);
            retval = setConditionField (server, *condition, &value, fieldMaxTimeShelvedQN);
            CONDITION_ASSERT_RETURN_RETVAL(retval, "Set MaxTimeShelved Field failed",);
        }
    }

    if (properties->onDelay)
    {
        retval = addOptionalField(server, *condition, alarmConditionTypeId,
                                  fieldOnDelayQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding OnDelay optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->onDelay, &UA_TYPES[UA_TYPES_DURATION]);
        retval = setConditionField (server, *condition, &value, fieldOnDelayQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set OnDelay Field failed",);
    }

    if (properties->offDelay)
    {
        retval = addOptionalField(server, *condition, alarmConditionTypeId,
                                  fieldOffDelayQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding OffDelay optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->offDelay, &UA_TYPES[UA_TYPES_DURATION]);
        retval = setConditionField (server, *condition, &value, fieldOffDelayQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set OffDelay Field failed",);
    }

    if (properties->reAlarmTime)
    {
        retval = addOptionalField(server, *condition, alarmConditionTypeId,
                                  fieldReAlarmTimeQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding ReAlarmTime optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->reAlarmTime, &UA_TYPES[UA_TYPES_DURATION]);
        retval = setConditionField (server, *condition, &value, fieldReAlarmTimeQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ReAlarmTime Field failed",);
    }
    if (properties->reAlarmRepeatCount)
    {
        retval = addOptionalField(server, *condition, alarmConditionTypeId,
                                  fieldReAlarmRepeatCountQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding ReAlarmRepeatCount optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->reAlarmRepeatCount, &UA_TYPES[UA_TYPES_INT16]);
        retval = setConditionField (server, *condition, &value, fieldReAlarmRepeatCountQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set ReAlarmTimeRepeatCount Field failed",);
    }

    //TODO add support for alarm audio
    //TODO alarm suppression groups
    return retval;
}

UA_StatusCode
UA_Server_setupAlarmConditionNodes (UA_Server *server, const UA_NodeId *conditionId,
                                              const UA_AlarmConditionProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupAlarmConditionNodes (server, conditionId, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

static UA_StatusCode
setupDiscrepancyAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                                      const UA_DiscrepancyAlarmProperties *properties)
{
    UA_StatusCode ret = setupAlarmConditionNodes (server, condition, &properties->alarmConditionProperties);
    if (ret != UA_STATUSCODE_GOOD) return ret;

    ret = writeObjectProperty_scalar(server, *condition, fieldExpectedTimeQN, &properties->expectedTime, &UA_TYPES[UA_TYPES_DURATION]);
    CONDITION_ASSERT_RETURN_RETVAL(ret, "Setting ExpectedTime value failed",);

    ret = writeObjectProperty_scalar(server, *condition, fieldTargetValueNodeQN, &properties->targetValue, &UA_TYPES[UA_TYPES_NODEID]);
    CONDITION_ASSERT_RETURN_RETVAL(ret, "Setting TargetValueNode value failed",);

    if (properties->tolerance)
    {
        UA_NodeId type = UA_NODEID_NUMERIC(0, UA_NS0ID_DISCREPANCYALARMTYPE);
        ret = addOptionalField(server, *condition, type, fieldToleranceQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(ret, "Adding Tolerance optional field failed",);

        ret = writeObjectProperty_scalar(server, *condition, fieldToleranceQN, properties->tolerance, &UA_TYPES[UA_TYPES_DOUBLE]);
        CONDITION_ASSERT_RETURN_RETVAL(ret, "Setting Tolerance value failed",);
    }
    return ret;
}

UA_StatusCode
UA_Server_setupDiscrepancyAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                            const UA_DiscrepancyAlarmProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupDiscrepancyAlarmNodes (server, condition, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

static UA_StatusCode
setupOffNormalAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                                    const UA_OffNormalAlarmProperties *properties)
{
    UA_StatusCode ret = setupAlarmConditionNodes (server, condition, &properties->alarmConditionProperties);
    if (ret != UA_STATUSCODE_GOOD) return ret;

    ret = writeObjectProperty_scalar(server, *condition, fieldNormalStateQN, &properties->normalState, &UA_TYPES[UA_TYPES_NODEID]);
    CONDITION_ASSERT_RETURN_RETVAL(ret, "Setting NormalState value failed",);
    return ret;
}

UA_StatusCode
UA_Server_setupOffNormalAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                          const UA_OffNormalAlarmProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupOffNormalAlarmNodes (server, condition, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

static UA_StatusCode
setupCertificateExpirationAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                                                const UA_CertificateExpirationAlarmProperties *properties)
{
    UA_StatusCode retval = setupOffNormalAlarmNodes (server, condition, &properties->offNormalAlarmProperties);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    if (properties->expirationLimit)
    {
        UA_NodeId certificateConditionTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_CERTIFICATEEXPIRATIONALARMTYPE);
        retval = addOptionalField(server, *condition, certificateConditionTypeId,
                                  UA_QUALIFIEDNAME(0,CONDITION_FIELD_EXPIRATION_LIMIT), NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding Expiration Limit optional field failed",);

        /* Set the default value for the Expiration limit property */
        retval = writeObjectProperty_scalar (server, *condition, UA_QUALIFIEDNAME(0, CONDITION_FIELD_EXPIRATION_LIMIT),
                                              properties->expirationLimit, &UA_TYPES[UA_TYPES_DURATION]);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting Expiration Limit value failed",);
    }

    retval = writeObjectProperty_scalar (server, *condition, UA_QUALIFIEDNAME(0, CONDITION_FIELD_CERTIFICATE),
                                         &properties->certificate, &UA_TYPES[UA_TYPES_BYTESTRING]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting Certificate value failed",);

    retval = writeObjectProperty_scalar (server, *condition, UA_QUALIFIEDNAME(0, CONDITION_FIELD_CERTIFICATE_TYPE),
                                         &properties->certificateType, &UA_TYPES[UA_TYPES_NODEID]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting CertificateType value failed",);

    retval = writeObjectProperty_scalar (server, *condition, UA_QUALIFIEDNAME(0, CONDITION_FIELD_EXPIRATION_DATE),
                                         &properties->expirationDate, &UA_TYPES[UA_TYPES_DATETIME]);
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Setting ExpirationDate value failed",);

    return retval;
}

UA_StatusCode
UA_Server_setupCertificateExpirationAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                                      const UA_CertificateExpirationAlarmProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupCertificateExpirationAlarmNodes (server, condition, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

static UA_StatusCode
setupLimitAlarmNodes(UA_Server *server, const UA_NodeId *condition, const UA_LimitAlarmProperties *properties)
{
    UA_StatusCode retval = setupAlarmConditionNodes (server, condition, &properties->alarmConditionProperties);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    if (!properties->lowLimit && !properties->lowLowLimit && !properties->highLimit && !properties->highHighLimit)
    {
        CONDITION_LOG_ERROR (retval, "At least one limit field is mandatory");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_NodeId LimitAlarmTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_LIMITALARMTYPE);
    UA_Variant value;
    /* Limits */
    if (properties->lowLowLimit)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldLowLowLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding LowLowLimit optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->lowLowLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField (server, *condition, &value, fieldLowLowLimitQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set LowLowLimit Field failed",);
    }

    if (properties->lowLimit)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldLowLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding LowLimit optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->lowLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField (server, *condition, &value, fieldLowLimitQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set LowLimit Field failed",);
    }

    if (properties->highLimit)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldHighLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding HighLimit optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->highLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField (server, *condition, &value, fieldHighLimitQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set HighLimit Field failed",);
    }

    if (properties->highHighLimit)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldHighHighLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding HighHighLimit optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->highHighLimit, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField (server, *condition, &value, fieldHighHighLimitQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set HighHighLimit Field failed",);
    }

    /* Base Limits */
    if (properties->base_low_low_limit)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldBaseLowLowLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding BaseLowLowLimit optional Field failed",);
        UA_Variant_setScalar(&value, (void *) (uintptr_t) properties->base_low_low_limit, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField (server, *condition, &value, fieldBaseLowLowLimitQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set BaseLowLowLimit Field failed",);
    }

    if (properties->base_low_limit)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldBaseLowLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding BaseLowLimit optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->base_low_low_limit, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField(server, *condition, &value, fieldBaseLowLimitQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set BaseLowLimit Field failed",);
    }

    if (properties->base_high_limit)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldBaseHighLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding BaseHighLimit optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->base_high_limit, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField(server, *condition, &value, fieldBaseHighLimitQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set BaseHighLimit Field failed",);
    }

    if (properties->base_high_high_limit)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldBaseHighHighLimitQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding BaseHighHighLimit optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->base_high_high_limit, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField(server, *condition, &value, fieldBaseHighHighLimitQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set BaseHighHighLimit Field failed",);
    }

    /* Deadband */
    if (properties->low_low_deadband)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldLowLowDeadbandQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding LowLowDeadband optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->low_low_deadband, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField(server, *condition, &value, fieldLowLowDeadbandQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set LowLowDeadband Field failed",);
    }

    if (properties->low_deadband)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldLowDeadbandQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding LowDeadband optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->low_deadband, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField(server, *condition, &value, fieldLowDeadbandQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set LowDeadband Field failed",);
    }

    if (properties->high_deadband)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldHighDeadbandQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding HighDeadband optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->high_deadband, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField(server, *condition, &value, fieldHighDeadbandQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set HighDeadband Field failed",);
    }

    if (properties->high_high_deadband)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldHighHighDeadbandQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding HighHighDeadband optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->high_high_deadband, &UA_TYPES[UA_TYPES_DOUBLE]);
        retval = setConditionField(server, *condition, &value, fieldHighHighDeadbandQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set HighHighDeadband Field failed",);
    }

    /* Limit Severity */

    if (properties->severity_low_low)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldSeverityLowLowQN , NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding SeverityLowLow optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->severity_low_low, &UA_TYPES[UA_TYPES_UINT16]);
        retval = setConditionField(server, *condition, &value, fieldSeverityLowLowQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set SeverityLowLow Field failed",);
    }

    if (properties->severity_low)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldSeverityLowQN , NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding SeverityLow optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->severity_low, &UA_TYPES[UA_TYPES_UINT16]);
        retval = setConditionField(server, *condition, &value, fieldSeverityLowQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set SeverityLow Field failed",);
    }

    if (properties->severity_high)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldSeverityHighQN , NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding SeverityHigh optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t)properties->severity_high, &UA_TYPES[UA_TYPES_UINT16]);
        retval = setConditionField(server, *condition, &value, fieldSeverityHighQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set SeverityHigh Field failed",);
    }

    if (properties->severity_high_high)
    {
        retval = addOptionalField(server, *condition, LimitAlarmTypeId, fieldSeverityHighHighQN , NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding SeverityHighHigh optional Field failed",);
        UA_Variant_setScalar(&value, (void *)(uintptr_t) properties->severity_high_high, &UA_TYPES[UA_TYPES_UINT16]);
        retval = setConditionField(server, *condition, &value, fieldSeverityHighHighQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set SeverityHighHigh Field failed",);
    }


    return retval;
}

UA_StatusCode
UA_Server_setupLimitAlarmNodes(UA_Server *server, const UA_NodeId *condition, const UA_LimitAlarmProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupLimitAlarmNodes (server, condition, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

static UA_StatusCode
setupNonExclusiveLimitAlarmNodes(UA_Server *server, const UA_NodeId *condition, const UA_LimitAlarmProperties *properties)
{
    UA_StatusCode retval = setupLimitAlarmNodes (server, condition, properties);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    if (!properties->lowLimit && !properties->highLimit)
    {
        CONDITION_LOG_ERROR (retval, "At least LowLimit or HighLimit must be provided");
        return UA_STATUSCODE_BADCONFIGURATIONERROR;
    }

    UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_NONEXCLUSIVELEVELALARMTYPE);

    if (properties->lowLowLimit)
    {
        retval = addOptionalField(server, *condition, typeId, fieldLowLowStateQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding optional LowLowState Field failed",);
        setTwoStateVariable(server, condition, fieldLowLowStateQN, false, INACTIVE_LOWLOW_TEXT);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set LowLowState failed",);
    }

    if (properties->lowLimit)
    {
        retval = addOptionalField(server, *condition, typeId, fieldLowStateQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding optional LowState Field failed",);
        setTwoStateVariable(server, condition, fieldLowStateQN, false, INACTIVE_LOW_TEXT);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set LowState failed",);
    }

    if (properties->highLimit)
    {
        retval = addOptionalField(server, *condition, typeId, fieldHighStateQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding optional HighState Field failed",);
        setTwoStateVariable(server, condition, fieldHighStateQN, false, INACTIVE_HIGH_TEXT);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set HighState failed",);

    }

    if (properties->highHighLimit)
    {
        retval = addOptionalField(server, *condition, typeId, fieldHighHighStateQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding optional HighHighState Field failed",);
        setTwoStateVariable(server, condition, fieldHighHighStateQN, false, INACTIVE_HIGHHIGH_TEXT);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set HighHighState failed",);
    }
    return retval;
}

UA_StatusCode
UA_Server_setupNonExclusiveLimitAlarmNodes(UA_Server *server, const UA_NodeId *condition, const UA_LimitAlarmProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupNonExclusiveLimitAlarmNodes (server, condition, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

static UA_StatusCode
setupDeviationAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                          const UA_DeviationAlarmProperties *properties)
{
    UA_StatusCode retval = setupLimitAlarmNodes (server, condition, &properties->limitAlarmProperties);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    UA_Variant value;
    UA_Variant_setScalar(&value, (void *)(uintptr_t)&properties->setpointNode, &UA_TYPES[UA_TYPES_NODEID]);
    retval = setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0, CONDITION_FIELD_SETPOINTNODE));
    CONDITION_ASSERT_RETURN_RETVAL(retval, "Set SetpointNode Field failed",);

    if (properties->baseSetpointNode)
    {
        UA_NodeId typeId = UA_NODEID_NUMERIC(0, UA_NS0ID_NONEXCLUSIVEDEVIATIONALARMTYPE);
        retval = addOptionalField(server, *condition, typeId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_BASESETPOINTNODE), NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding optional BaseSetpointNode Field failed",);

        UA_Variant_setScalar(&value, (void *)(uintptr_t)&properties->baseSetpointNode, &UA_TYPES[UA_TYPES_NODEID]);
        retval = setConditionField(server, *condition, &value, UA_QUALIFIEDNAME(0, CONDITION_FIELD_BASESETPOINTNODE));
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set BaseSetpointNode Field failed",);
    }

    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
UA_Server_setupDeviationAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                          const UA_DeviationAlarmProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupDeviationAlarmNodes (server, condition, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

static UA_StatusCode
setupRateOfChangeAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                             const UA_RateOfChangeAlarmProperties *properties)
{
    UA_StatusCode retval = setupLimitAlarmNodes (server, condition, &properties->limitAlarmProperties);
    if (retval != UA_STATUSCODE_GOOD) return retval;

    UA_NodeId RateOfChangeAlarmTypeId = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVERATEOFCHANGEALARMTYPE);
    if (properties->engineeringUnits)
    {
        retval = addOptionalField(server, *condition, RateOfChangeAlarmTypeId,
                                                fieldEngineeringUnitsQN, NULL);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Adding optional EngineeringUnits Field failed",);

        UA_Variant value;
        UA_Variant_setScalar(&value, (void *)(uintptr_t)&properties->engineeringUnits, &UA_TYPES[UA_TYPES_EUINFORMATION]);
        retval = setConditionField(server, *condition, &value, fieldEngineeringUnitsQN);
        CONDITION_ASSERT_RETURN_RETVAL(retval, "Set EngineeringUnits Field failed",);
    }
    return retval;
}

UA_StatusCode
UA_Server_setupRateOfChangeAlarmNodes (UA_Server *server, const UA_NodeId *condition,
                             const UA_RateOfChangeAlarmProperties *properties)
{
    UA_LOCK (&server->serviceMutex);
    UA_StatusCode retval = setupRateOfChangeAlarmNodes (server, condition, properties);
    UA_UNLOCK (&server->serviceMutex);
    return retval;
}

void initNs0ConditionAndAlarms (UA_Server *server)
{
    /* Set callbacks for Method Fields of a condition. The current implementation
     * references methods without copying them when creating objects. So the
     * callbacks will be attached to the methods of the conditionType. */

    UA_NodeId methodId[] = {
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_DISABLE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_ENABLE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_ADDCOMMENT}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_CONDITIONTYPE_CONDITIONREFRESH2}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_ACKNOWLEDGE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ACKNOWLEDGEABLECONDITIONTYPE_CONFIRM}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_RESET}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_RESET2}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_SUPPRESS}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_SUPPRESS2}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_UNSUPPRESS}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_UNSUPPRESS}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_PLACEINSERVICE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_PLACEINSERVICE2}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_REMOVEFROMSERVICE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_ALARMCONDITIONTYPE_REMOVEFROMSERVICE2}},

        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SHELVEDSTATEMACHINETYPE_TIMEDSHELVE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SHELVEDSTATEMACHINETYPE_ONESHOTSHELVE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SHELVEDSTATEMACHINETYPE_UNSHELVE}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SHELVEDSTATEMACHINETYPE_TIMEDSHELVE2}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SHELVEDSTATEMACHINETYPE_ONESHOTSHELVE2}},
        {0, UA_NODEIDTYPE_NUMERIC, {UA_NS0ID_SHELVEDSTATEMACHINETYPE_UNSHELVE2}}
    };

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval |= setMethodNode_callback(server, methodId[0], disableMethodCallback);
    retval |= setMethodNode_callback(server, methodId[1], enableMethodCallback);
    retval |= setMethodNode_callback(server, methodId[2], addCommentMethodCallback);
    retval |= setMethodNode_callback(server, methodId[3], refreshMethodCallback);
    retval |= setMethodNode_callback(server, methodId[4], refresh2MethodCallback);
    retval |= setMethodNode_callback(server, methodId[5], acknowledgeMethodCallback);
    retval |= setMethodNode_callback(server, methodId[6], confirmMethodCallback);
    retval |= setMethodNode_callback(server, methodId[7], resetMethodCallback);
    retval |= setMethodNode_callback(server, methodId[8], reset2MethodCallback);
    retval |= setMethodNode_callback(server, methodId[9], suppressMethodCallback);
    retval |= setMethodNode_callback(server, methodId[10], suppress2MethodCallback);
    retval |= setMethodNode_callback(server, methodId[11], unsuppressMethodCallback);
    retval |= setMethodNode_callback(server, methodId[12], unsuppress2MethodCallback);
    retval |= setMethodNode_callback(server, methodId[13], placeInServiceMethodCallback);
    retval |= setMethodNode_callback(server, methodId[14], placeInService2MethodCallback);
    retval |= setMethodNode_callback(server, methodId[15], removeFromServiceMethodCallback);
    retval |= setMethodNode_callback(server, methodId[16], removeFromService2MethodCallback);

    retval |= setMethodNode_callback(server, methodId[17], timedShelveMethodCallback);
    retval |= setMethodNode_callback(server, methodId[18], oneShotShelveMethodCallback);
    retval |= setMethodNode_callback(server, methodId[19], unshelveMethodCallback);
    retval |= setMethodNode_callback(server, methodId[20], timedShelve2MethodCallback);
    retval |= setMethodNode_callback(server, methodId[21], oneShotShelve2MethodCallback);
    retval |= setMethodNode_callback(server, methodId[22], unshelve2MethodCallback);

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

UA_StatusCode
UA_Server_Condition_getInputNodeValue (UA_Server *server, UA_NodeId conditionId, UA_Variant *out)
{
    UA_QualifiedName inputNodeQN = UA_QUALIFIEDNAME(0, "InputNode");
    UA_BrowsePathResult bpr =  UA_Server_browseSimplifiedBrowsePath(server, conditionId, 1, &inputNodeQN);
    if (bpr.statusCode != UA_STATUSCODE_GOOD || bpr.targetsSize != 1) return bpr.statusCode;
    UA_Variant sourceNodeId;
    UA_StatusCode status = UA_Server_readValue(server, bpr.targets[0].targetId.nodeId, &sourceNodeId);
    UA_BrowsePathResult_clear(&bpr);
    if (status != UA_STATUSCODE_GOOD || sourceNodeId.type != &UA_TYPES[UA_TYPES_NODEID]) return status;
    status = UA_Server_readValue(server, *(UA_NodeId*) sourceNodeId.data, out);
    UA_Variant_clear(&sourceNodeId);
    return status;
}

void
UA_Server_Condition_iterBranches (UA_Server *server, UA_NodeId conditionId,
                                  UA_ConditionBranchIterCb iterFn, void *iterCtx)
{
    UA_LOCK(&server->serviceMutex);
    UA_ConditionBranch *branch = getConditionBranch(server, &conditionId);
    UA_Condition *cond = branch->condition;
    UA_ConditionBranch *tmp = NULL;
    LIST_FOREACH_SAFE(branch, &cond->branches, listEntry, tmp)
    {
        UA_UNLOCK(&server->serviceMutex);
        iterFn (server, &branch->id, cond->context, iterCtx);
        UA_LOCK(&server->serviceMutex);
    }
    UA_UNLOCK(&server->serviceMutex);
}

/* Condition Types */


static UA_StatusCode calculateNewLimitState (
    UA_Server *server,
    const UA_NodeId *conditionId,
    UA_Double inputValue,
    UA_LimitState prevState,
    UA_LimitState *stateOut,
    UA_Boolean exclusive
)
{
    UA_LimitState state = 0;
    UA_Double highHighLimit;
    UA_Double highLimit;
    UA_Double lowLimit;
    UA_Double lowLowLimit;

    UA_Double highHighDeadband = 0;
    UA_Double highDeadband = 0;
    UA_Double lowDeadband = 0;
    UA_Double lowLowDeadband = 0;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    readObjectPropertyDouble (server, *conditionId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_HIGHHIGHDEADBAND), &highHighDeadband);
    retval = readObjectPropertyDouble (server, *conditionId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_HIGHHIGHLIMIT), &highHighLimit);
    if (retval == UA_STATUSCODE_GOOD)
    {
        UA_Double limit = UA_LIMITSTATE_CHECK(prevState, UA_LIMITSTATE_HIGHHIGHSTATEBIT) ?
                          (highHighLimit - highHighDeadband) :  highHighLimit;
        if (inputValue >= limit)
        {
            UA_LIMITSTATE_SET(state, UA_LIMITSTATE_HIGHHIGHSTATEBIT);
            if (exclusive) goto done;
        }
    }

    readObjectPropertyDouble (server, *conditionId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_HIGHDEADBAND), &highDeadband);
    retval = readObjectPropertyDouble (server, *conditionId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_HIGHLIMIT), &highLimit);
    if (retval == UA_STATUSCODE_GOOD)
    {
        UA_Double limit = UA_LIMITSTATE_CHECK(prevState, UA_LIMITSTATE_HIGHSTATEBIT) ?
                          (highLimit - highDeadband) :  highLimit;
        if (inputValue >= limit)
        {
            UA_LIMITSTATE_SET(state, UA_LIMITSTATE_HIGHSTATEBIT);
            if (exclusive) goto done;
        }
    }

    readObjectPropertyDouble (server, *conditionId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_LOWDEADBAND), &lowDeadband);
    retval = readObjectPropertyDouble (server, *conditionId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_LOWLIMIT), &lowLimit);
    if (retval == UA_STATUSCODE_GOOD)
    {
        UA_Double limit = UA_LIMITSTATE_CHECK(prevState, UA_LIMITSTATE_LOWSTATEBIT) ?
                          (lowLimit + lowDeadband) :  lowLimit;
        if (inputValue <= limit)
        {
            UA_LIMITSTATE_SET(state, UA_LIMITSTATE_LOWSTATEBIT);
            if (exclusive) goto done;
        }
    }

    readObjectPropertyDouble (server, *conditionId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_LOWLOWDEADBAND), &lowLowDeadband);
    retval = readObjectPropertyDouble (server, *conditionId, UA_QUALIFIEDNAME(0, CONDITION_FIELD_LOWLOWLIMIT), &lowLowLimit);
    if (retval == UA_STATUSCODE_GOOD)
    {
        UA_Double limit = UA_LIMITSTATE_CHECK(prevState, UA_LIMITSTATE_LOWLOWSTATEBIT) ?
                          (lowLowLimit + lowLowDeadband) :  lowLowLimit;
        if (inputValue <= limit)
        {
            UA_LIMITSTATE_SET(state, UA_LIMITSTATE_LOWLOWSTATEBIT);
            if (exclusive) goto done;
        }
    }
    retval = UA_STATUSCODE_GOOD;
done:
    *stateOut = state;
    return retval;
}

static UA_StatusCode
exclusiveLimitStateMachine_getState (UA_Server *server, const UA_NodeId *limitStateId,
                             UA_LimitState* stateOut)
{
    UA_Variant outValue;
    UA_Variant_init (&outValue);
    UA_NodeId currentStateId;
    UA_NodeId_init (&currentStateId);
    UA_NodeId currentStateIdId;
    UA_NodeId_init (&currentStateIdId);
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    retval = getNodeIdWithBrowseName(server, limitStateId, UA_QUALIFIEDNAME(0, "CurrentState"), &currentStateId);
    if (retval != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR(retval, "Could not get LimitState CurrentState nodeId")
        goto done;
    }
    retval = getNodeIdWithBrowseName(server, &currentStateId, UA_QUALIFIEDNAME(0, "Id"), &currentStateIdId);
    if (retval != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR(retval, "Could not get LimitState CurrentState Id nodeId")
        goto done;
    }

    retval = readWithReadValue(server, &currentStateIdId, UA_ATTRIBUTEID_VALUE, &outValue);
    if (retval != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR(retval, "Could not read LimitState CurrentState Id value");
        goto done;
    }

    UA_LimitState state = 0;
    const UA_NodeId *idValue = (const UA_NodeId *) outValue.data;
    UA_NodeId highhigh = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITSTATEMACHINETYPE_HIGHHIGH);
    UA_NodeId high = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITSTATEMACHINETYPE_HIGH);
    UA_NodeId low = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITSTATEMACHINETYPE_LOW);
    UA_NodeId lowlow = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITSTATEMACHINETYPE_LOWLOW);
    if (UA_NodeId_equal(idValue, &highhigh)) UA_LIMITSTATE_SET(state, UA_LIMITSTATE_HIGHHIGHSTATEBIT);
    else if (UA_NodeId_equal(idValue, &high)) UA_LIMITSTATE_SET(state, UA_LIMITSTATE_HIGHSTATEBIT);
    else if (UA_NodeId_equal(idValue, &low)) UA_LIMITSTATE_SET(state, UA_LIMITSTATE_LOWSTATEBIT);
    else if (UA_NodeId_equal(idValue, &lowlow)) UA_LIMITSTATE_SET(state, UA_LIMITSTATE_LOWLOWSTATEBIT);
    *stateOut = state;
done:
    UA_Variant_clear(&outValue);
    UA_NodeId_clear(&currentStateId);
    UA_NodeId_clear(&currentStateIdId);
    return retval;
}

static UA_StatusCode
exclusiveLimitStateMachine_setState (UA_Server *server, const UA_NodeId *limitStateId, UA_LimitState state)
{
    UA_NodeId currentStateId;
    UA_NodeId_init (&currentStateId);
    UA_NodeId currentStateIdId;
    UA_NodeId_init (&currentStateIdId);
    UA_StatusCode retval = getNodeIdWithBrowseName(server, limitStateId, UA_QUALIFIEDNAME(0, "CurrentState"), &currentStateId);
    if (retval != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR(retval, "Could not get LimitState CurrentState nodeId")
        goto done;
    }
    retval = getNodeIdWithBrowseName(server, &currentStateId, UA_QUALIFIEDNAME(0, "Id"), &currentStateIdId);
    if (retval != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR(retval, "Could not get LimitState CurrentState Id nodeId")
        goto done;
    }

    UA_LocalizedText currentStateValue;
    UA_NodeId currentStateIdValue;

    if (UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_HIGHHIGHSTATEBIT))
    {
        currentStateValue = UA_LOCALIZEDTEXT(LOCALE, ACTIVE_HIGHHIGH_TEXT);
        currentStateIdValue = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITSTATEMACHINETYPE_HIGHHIGH);
    }
    else if (UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_HIGHSTATEBIT))
    {
        currentStateValue = UA_LOCALIZEDTEXT(LOCALE, ACTIVE_HIGH_TEXT);
        currentStateIdValue = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITSTATEMACHINETYPE_HIGH);
    }
    else if (UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_LOWSTATEBIT))
    {
        currentStateValue = UA_LOCALIZEDTEXT(LOCALE, ACTIVE_LOW_TEXT);
        currentStateIdValue = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITSTATEMACHINETYPE_LOW);
    }
    else if (UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_LOWLOWSTATEBIT))
    {
        currentStateValue = UA_LOCALIZEDTEXT(LOCALE, ACTIVE_LOWLOW_TEXT);
        currentStateIdValue = UA_NODEID_NUMERIC(0, UA_NS0ID_EXCLUSIVELIMITSTATEMACHINETYPE_LOWLOW);
    }
    else
    {
        currentStateValue = UA_LOCALIZEDTEXT(LOCALE, "Normal");
        currentStateIdValue = UA_NODEID_NULL;
    }

    UA_Variant value;
    UA_Variant_setScalar(&value, &currentStateValue, &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]);
    retval = writeValueAttribute(server, currentStateId, &value);
    if (retval != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR(retval , "Could not write current state value");
        goto done;
    }
    UA_Variant_setScalar(&value, &currentStateIdValue, &UA_TYPES[UA_TYPES_NODEID]);
    retval = writeValueAttribute(server, currentStateIdId, &value);
    if (retval != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR(retval , "Could not write current state id value");
        goto done;
    }

    //TODO update last transition

done:
    UA_NodeId_clear(&currentStateId);
    UA_NodeId_clear(&currentStateIdId);
    return retval;
}

static UA_StatusCode
exclusiveLimitAlarmTypeEvaluateLimitState (UA_Server *server, const UA_NodeId *conditionId, UA_Double input,
                                           UA_LimitState *stateOut, UA_Boolean *stateChangedOut)
{
    UA_LimitState prevState = 0;
    UA_LimitState state = 0;
    UA_Boolean stateChanged = false;
    UA_NodeId limitStateId;
    UA_NodeId_init (&limitStateId);
    UA_StatusCode ret = getNodeIdWithBrowseName(server, conditionId, fieldLimitStateQN, &limitStateId);
    if (ret != UA_STATUSCODE_GOOD)
    {
        CONDITION_LOG_ERROR(ret, "Could not get LimitState nodeId")
        goto done;
    }
    ret = exclusiveLimitStateMachine_getState(server, &limitStateId, &prevState);
    if (ret != UA_STATUSCODE_GOOD) goto done;

    ret = calculateNewLimitState(server, conditionId, input, prevState, &state, true);
    if (ret != UA_STATUSCODE_GOOD) goto done;

    stateChanged = prevState != state;
    if (stateChanged) ret = exclusiveLimitStateMachine_setState(server, &limitStateId, state);
    if (ret != UA_STATUSCODE_GOOD) goto done;
    *stateChangedOut = stateChanged;
    *stateOut = state;
done:
    UA_NodeId_clear (&limitStateId);
    return ret;
}

UA_StatusCode
UA_Server_ExclusiveLimitAlarmEvaluateLimitState (UA_Server *server, const UA_NodeId *conditionId, UA_Double input,
                                                 UA_LimitState *stateOut, UA_Boolean *stateChanged)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = exclusiveLimitAlarmTypeEvaluateLimitState (server, conditionId, input, stateOut, stateChanged);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

static void
limitAlarmCalculateEventInfo (UA_Server *server, const UA_NodeId *conditionId,
                                       UA_LimitState state, UA_Double value,
                                       UA_ConditionEventInfo *info)
{
    if (state == 0)
    {
        info->message = UA_LOCALIZEDTEXT(LOCALE, "Alarm state is Normal");
        info->hasSeverity = true;
        info->severity = 0;
        return;
    }
    UA_UInt16 severity = 250; //Arbitrary default
    char *stateText = NULL;
    if (UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_HIGHHIGHSTATEBIT))
    {
        stateText = "HighHigh";
        readObjectPropertyUInt16(server, *conditionId, UA_QUALIFIEDNAME(0, "SeverityHighHigh"), &severity);
    }
    else if (UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_HIGHSTATEBIT))
    {
        stateText = "High";
        readObjectPropertyUInt16(server, *conditionId, UA_QUALIFIEDNAME(0, "SeverityHigh"), &severity);
    }
    else if (UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_LOWSTATEBIT))
    {
        stateText = "Low";
        readObjectPropertyUInt16(server, *conditionId, UA_QUALIFIEDNAME(0, "SeverityLow"), &severity);
    }
    else if (UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_LOWLOWSTATEBIT))
    {
        stateText = "LowLow";
        readObjectPropertyUInt16(server, *conditionId, UA_QUALIFIEDNAME(0, "SeverityLowLow"), &severity);
    }

    info->message.locale = UA_STRING(LOCALE);
    info->message.text = UA_String_fromFormat ("Alarm state is %s. Value %.2lf", stateText, value);
    info->hasSeverity = true;
    info->severity = severity;
}

UA_StatusCode UA_Server_exclusiveLimitAlarmEvaluate_default (
    UA_Server *server,
    const UA_NodeId *conditionId,
    void *ctx,
    const UA_Double *input
)
{
    UA_ConditionEventInfo info = {0};
    UA_LimitState currentState = 0;
    UA_Boolean stateChanged = false;
    UA_StatusCode retval = UA_Server_ExclusiveLimitAlarmEvaluateLimitState(server, conditionId, *input, &currentState, &stateChanged);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    UA_Boolean isActive = currentState != 0;
    if (stateChanged)
    {
        UA_LOCK(&server->serviceMutex);
        limitAlarmCalculateEventInfo (server, conditionId, currentState, *input, &info);
        UA_UNLOCK(&server->serviceMutex);
        UA_Server_Condition_updateActive (server, *conditionId, &info, isActive);
    }
    return retval;
}

static UA_StatusCode
nonExclusiveLimitAlarmGetState (UA_Server *server, const UA_NodeId *conditionId, UA_LimitState *stateOut)
{
    UA_LimitState state = 0;
    UA_Boolean inLowState = isTwoStateVariableInTrueState(server, conditionId, &fieldLowStateQN);
    if (inLowState)
    {
        UA_LIMITSTATE_SET(state, UA_LIMITSTATE_LOWSTATEBIT);
        if (isTwoStateVariableInTrueState(server, conditionId, &fieldLowLowStateQN))
        {
            UA_LIMITSTATE_SET(state, UA_LIMITSTATE_LOWLOWSTATEBIT);
        }

        goto done;
    }

    UA_Boolean inHighState = isTwoStateVariableInTrueState(server, conditionId, &fieldHighStateQN);
    if (inHighState)
    {
        UA_LIMITSTATE_SET(state, UA_LIMITSTATE_HIGHSTATEBIT);
        if (isTwoStateVariableInTrueState(server, conditionId, &fieldHighHighStateQN))
        {
            UA_LIMITSTATE_SET(state, UA_LIMITSTATE_HIGHHIGHSTATEBIT);
        }
        goto done;
    }
done:
    *stateOut = state;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
nonExclusiveLimitAlarmSetState (UA_Server *server, const UA_NodeId *conditionId, UA_LimitState prevState, UA_LimitState state)
{
    UA_StatusCode ret = UA_STATUSCODE_GOOD;
    UA_Boolean lowStateVal = UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_LOWSTATEBIT);
    if (UA_LIMITSTATE_CHECK(prevState, UA_LIMITSTATE_LOWSTATEBIT) != lowStateVal)
    {
        ret = setOptionalTwoStateVariable(server, conditionId, fieldLowStateQN, lowStateVal,
                                          lowStateVal ? ACTIVE_LOW_TEXT : INACTIVE_LOW_TEXT);
        if (ret != UA_STATUSCODE_GOOD) goto done;
    }

    UA_Boolean lowLowStateVal = UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_LOWLOWSTATEBIT);
    if (UA_LIMITSTATE_CHECK(prevState, UA_LIMITSTATE_LOWLOWSTATEBIT) != lowLowStateVal)
    {
        ret = setOptionalTwoStateVariable(server, conditionId, fieldLowLowStateQN, lowLowStateVal,
                                              lowLowStateVal ? ACTIVE_LOWLOW_TEXT : INACTIVE_LOWLOW_TEXT);
        if (ret != UA_STATUSCODE_GOOD) goto done;
    }


    UA_Boolean highStateVal = UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_HIGHSTATEBIT);
    if (UA_LIMITSTATE_CHECK(prevState, UA_LIMITSTATE_HIGHSTATEBIT) != highStateVal)
    {
        ret = setOptionalTwoStateVariable(server, conditionId, fieldLowStateQN, highStateVal,
                                          highStateVal ? ACTIVE_HIGH_TEXT : INACTIVE_HIGH_TEXT);
        if (ret != UA_STATUSCODE_GOOD) goto done;
    }

    UA_Boolean highHighStateVal = UA_LIMITSTATE_CHECK(state, UA_LIMITSTATE_HIGHHIGHSTATEBIT);
    if (UA_LIMITSTATE_CHECK(prevState, UA_LIMITSTATE_HIGHHIGHSTATEBIT) != highHighStateVal)
    {
        ret = setOptionalTwoStateVariable(server, conditionId, fieldLowLowStateQN, highHighStateVal,
                                          highHighStateVal ? ACTIVE_HIGHHIGH_TEXT : INACTIVE_HIGHHIGH_TEXT);
        if (ret != UA_STATUSCODE_GOOD) goto done;
    }

done:
    return ret;
}

static UA_StatusCode
nonExclusiveLimitAlarmTypeEvaluateLimitState (UA_Server *server, const UA_NodeId *conditionId, UA_Double input,
                                           UA_LimitState *stateOut, UA_Boolean *stateChangedOut)
{
    UA_LimitState prevState = 0;
    UA_LimitState state = 0;
    UA_Boolean stateChanged = false;

    UA_StatusCode ret = nonExclusiveLimitAlarmGetState (server, conditionId, &prevState);
    if (ret != UA_STATUSCODE_GOOD) goto done;

    ret = calculateNewLimitState(server, conditionId, input, prevState, &state, false);
    if (ret != UA_STATUSCODE_GOOD) goto done;

    stateChanged = prevState != state;
    if (stateChanged) ret = nonExclusiveLimitAlarmSetState (server, conditionId, prevState, state);
    if (ret != UA_STATUSCODE_GOOD) goto done;
    *stateChangedOut = stateChanged;
    *stateOut = state;
done:
    return ret;
}

UA_StatusCode
UA_Server_NonExclusiveLimitAlarmEvaluateLimitState (UA_Server *server, const UA_NodeId *conditionId, UA_Double input,
                                                 UA_LimitState *stateOut, UA_Boolean *stateChanged)
{
    UA_LOCK(&server->serviceMutex);
    UA_StatusCode ret = nonExclusiveLimitAlarmTypeEvaluateLimitState(server, conditionId, input, stateOut, stateChanged);
    UA_UNLOCK(&server->serviceMutex);
    return ret;
}

UA_StatusCode
UA_Server_nonExclusiveLimitAlarmEvaluate_default (
    UA_Server *server,
    const UA_NodeId *conditionId,
    const UA_Double *input
)
{
    UA_ConditionEventInfo info = {0};
    UA_LimitState currentState = 0;
    UA_Boolean stateChanged = false;
    UA_StatusCode retval = UA_Server_NonExclusiveLimitAlarmEvaluateLimitState(server, conditionId, *input, &currentState, &stateChanged);
    if (retval != UA_STATUSCODE_GOOD) return retval;
    UA_Boolean isActive = currentState != 0;
    if (stateChanged)
    {
        UA_LOCK(&server->serviceMutex);
        limitAlarmCalculateEventInfo (server, conditionId, currentState, *input, &info);
        UA_UNLOCK(&server->serviceMutex);
        UA_Server_Condition_updateActive (server, *conditionId, &info, isActive);
    }
    return retval;
}

#endif /* UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS */
